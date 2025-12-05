/**
 * @file signal_engine.cpp
 * @brief Implementación del motor de generación de señales
 * @version 1.0.0
 */

#include "core/signal_engine.h"
#include "config.h"

// ============================================================================
// SINGLETON
// ============================================================================
SignalEngine* SignalEngine::instance = nullptr;

// ============================================================================
// BUFFERS EN RAM RÁPIDA
// ============================================================================
DRAM_ATTR static uint8_t signalBuffer[SIGNAL_BUFFER_SIZE];
DRAM_ATTR static volatile uint16_t bufferReadIndex = 0;
DRAM_ATTR static volatile uint16_t bufferWriteIndex = 0;
DRAM_ATTR static volatile uint32_t isrCount = 0;
DRAM_ATTR static volatile uint32_t isrMaxTime = 0;
DRAM_ATTR static volatile uint32_t bufferUnderruns = 0;
DRAM_ATTR static volatile uint8_t lastDACValue = 128;

// ============================================================================
// CONSTRUCTOR
// ============================================================================
SignalEngine::SignalEngine() {
    currentSignal.type = SignalType::NONE;
    currentSignal.state = SignalState::STOPPED;
    
    signalMutex = xSemaphoreCreateMutex();
    signalTimer = nullptr;
    generationTaskHandle = nullptr;
}

SignalEngine* SignalEngine::getInstance() {
    if (instance == nullptr) {
        instance = new SignalEngine();
    }
    return instance;
}

// ============================================================================
// INICIALIZACIÓN
// ============================================================================
bool SignalEngine::begin() {
    DEBUG_PRINTLN("[SignalEngine] Inicializando...");
    
    // Configurar DAC
    dacWrite(DAC_SIGNAL_PIN, DAC_CENTER_VALUE);
    
    // Crear tarea de generación en Core 1
    BaseType_t taskCreated = xTaskCreatePinnedToCore(
        generationTask,
        "SignalGen",
        STACK_SIZE_SIGNAL,
        this,
        TASK_PRIORITY_SIGNAL,
        &generationTaskHandle,
        CORE_SIGNAL_GENERATION
    );
    
    if (taskCreated != pdPASS) {
        DEBUG_PRINTLN("[SignalEngine] ERROR: No se pudo crear tarea");
        return false;
    }
    
    DEBUG_PRINTLN("[SignalEngine] Inicializado correctamente");
    return true;
}

// ============================================================================
// CONTROL DE SEÑALES
// ============================================================================
bool SignalEngine::startSignal(SignalType type, uint8_t condition) {
    if (xSemaphoreTake(signalMutex, portMAX_DELAY) == pdTRUE) {
        // Detener señal actual si existe
        if (currentSignal.state == SignalState::RUNNING) {
            stopTimer();
        }
        
        // Reset buffers
        bufferReadIndex = 0;
        bufferWriteIndex = 0;
        isrCount = 0;
        bufferUnderruns = 0;
        
        // Configurar tipo de señal
        currentSignal.type = type;
        currentSignal.sampleCount = 0;
        currentSignal.lastUpdateTime = millis();
        
        // Configurar modelo según tipo
        switch (type) {
            case SignalType::ECG: {
                ECGParameters params;
                params.condition = (ECGCondition)condition;
                ecgModel.setParameters(params);
                ecgModel.reset();
                break;
            }
            case SignalType::EMG: {
                EMGParameters params;
                params.condition = (EMGCondition)condition;
                emgModel.setParameters(params);
                emgModel.reset();
                break;
            }
            case SignalType::PPG: {
                PPGParameters params;
                params.condition = (PPGCondition)condition;
                ppgModel.setParameters(params);
                ppgModel.reset();
                break;
            }
            default:
                xSemaphoreGive(signalMutex);
                return false;
        }
        
        // Pre-llenar buffer
        prefillBuffer();
        
        // Iniciar timer
        setupTimer();
        
        currentSignal.state = SignalState::RUNNING;
        
        xSemaphoreGive(signalMutex);
        return true;
    }
    return false;
}

bool SignalEngine::stopSignal() {
    if (xSemaphoreTake(signalMutex, portMAX_DELAY) == pdTRUE) {
        stopTimer();
        currentSignal.state = SignalState::STOPPED;
        currentSignal.type = SignalType::NONE;
        dacWrite(DAC_SIGNAL_PIN, DAC_CENTER_VALUE);
        xSemaphoreGive(signalMutex);
        return true;
    }
    return false;
}

bool SignalEngine::pauseSignal() {
    if (currentSignal.state == SignalState::RUNNING) {
        stopTimer();
        currentSignal.state = SignalState::PAUSED;
        return true;
    }
    return false;
}

bool SignalEngine::resumeSignal() {
    if (currentSignal.state == SignalState::PAUSED) {
        setupTimer();
        currentSignal.state = SignalState::RUNNING;
        return true;
    }
    return false;
}

// ============================================================================
// TIMER
// ============================================================================
void SignalEngine::setupTimer() {
    // Timer a 1 kHz
    signalTimer = timerBegin(0, 80, true);  // 80 prescaler = 1 MHz
    timerAttachInterrupt(signalTimer, &timerISR, true);
    timerAlarmWrite(signalTimer, 1000, true);  // 1000 us = 1 kHz
    timerAlarmEnable(signalTimer);
}

void SignalEngine::stopTimer() {
    if (signalTimer != nullptr) {
        timerAlarmDisable(signalTimer);
        timerDetachInterrupt(signalTimer);
        timerEnd(signalTimer);
        signalTimer = nullptr;
    }
}

// ============================================================================
// ISR DEL TIMER (en IRAM)
// ============================================================================
void IRAM_ATTR SignalEngine::timerISR() {
    uint32_t startTime = micros();
    
    // Leer del buffer circular
    if (bufferReadIndex != bufferWriteIndex) {
        lastDACValue = signalBuffer[bufferReadIndex];
        bufferReadIndex = (bufferReadIndex + 1) % SIGNAL_BUFFER_SIZE;
        dacWrite(DAC_SIGNAL_PIN, lastDACValue);
    } else {
        bufferUnderruns++;
    }
    
    isrCount++;
    
    uint32_t elapsed = micros() - startTime;
    if (elapsed > isrMaxTime) {
        isrMaxTime = elapsed;
    }
}

// ============================================================================
// TAREA DE GENERACIÓN
// ============================================================================
void SignalEngine::generationTask(void* parameter) {
    SignalEngine* engine = (SignalEngine*)parameter;
    const float deltaTime = 1.0f / SAMPLE_RATE_HZ;
    
    while (true) {
        if (engine->currentSignal.state == SignalState::RUNNING) {
            // Calcular espacio disponible en buffer
            uint16_t readIdx = bufferReadIndex;
            uint16_t writeIdx = bufferWriteIndex;
            uint16_t available = (readIdx - writeIdx - 1 + SIGNAL_BUFFER_SIZE) % SIGNAL_BUFFER_SIZE;
            
            // Generar muestras si hay espacio
            while (available > 0) {
                uint8_t sample = engine->generateSample();
                signalBuffer[writeIdx] = sample;
                writeIdx = (writeIdx + 1) % SIGNAL_BUFFER_SIZE;
                bufferWriteIndex = writeIdx;
                available--;
                engine->currentSignal.sampleCount++;
            }
        }
        
        // Pequeño delay para no saturar
        vTaskDelay(1);
    }
}

// ============================================================================
// GENERACIÓN DE MUESTRA
// ============================================================================
uint8_t SignalEngine::generateSample() {
    const float deltaTime = 1.0f / SAMPLE_RATE_HZ;
    
    switch (currentSignal.type) {
        case SignalType::ECG:
            return ecgModel.getDACValue(deltaTime);
        case SignalType::EMG:
            return emgModel.getDACValue(deltaTime);
        case SignalType::PPG:
            return ppgModel.getDACValue(deltaTime);
        default:
            return DAC_CENTER_VALUE;
    }
}

// ============================================================================
// PRE-LLENADO DE BUFFER
// ============================================================================
void SignalEngine::prefillBuffer() {
    for (int i = 0; i < SIGNAL_BUFFER_SIZE / 2; i++) {
        signalBuffer[i] = generateSample();
    }
    bufferWriteIndex = SIGNAL_BUFFER_SIZE / 2;
}

// ============================================================================
// GETTERS
// ============================================================================
uint8_t SignalEngine::getLastDACValue() const {
    return lastDACValue;
}

PerformanceStats SignalEngine::getStats() const {
    PerformanceStats stats;
    stats.isrCount = isrCount;
    stats.isrMaxTime = isrMaxTime;
    stats.bufferUnderruns = bufferUnderruns;
    stats.bufferLevel = (bufferWriteIndex - bufferReadIndex + SIGNAL_BUFFER_SIZE) % SIGNAL_BUFFER_SIZE;
    stats.freeHeap = ESP.getFreeHeap();
    return stats;
}

// ============================================================================
// ACTUALIZACIÓN DE PARÁMETROS
// ============================================================================
void SignalEngine::updateNoiseLevel(float noise) {
    // Parámetro Tipo A: aplicación inmediata
    noise = constrain(noise, 0.0f, 1.0f);
    
    switch (currentSignal.type) {
        case SignalType::ECG:
            currentSignal.ecg.noiseLevel = noise;
            ecgModel.setNoiseLevel(noise);
            break;
        case SignalType::EMG:
            currentSignal.emg.noiseLevel = noise;
            emgModel.setNoiseLevel(noise);
            break;
        case SignalType::PPG:
            currentSignal.ppg.noiseLevel = noise;
            ppgModel.setNoiseLevel(noise);
            break;
        default:
            break;
    }
}

void SignalEngine::updateAmplitude(float amplitude) {
    // Parámetro Tipo A: aplicación inmediata
    switch (currentSignal.type) {
        case SignalType::ECG:
            currentSignal.ecg.qrsAmplitude = amplitude;
            ecgModel.setAmplitude(amplitude);
            break;
        case SignalType::EMG:
            currentSignal.emg.amplitude = amplitude;
            emgModel.setAmplitude(amplitude);
            break;
        case SignalType::PPG:
            currentSignal.ppg.perfusionIndex = amplitude;
            ppgModel.setAmplitude(amplitude);
            break;
        default:
            break;
    }
}

void SignalEngine::setECGParameters(const ECGParameters& params) {
    ecgModel.setPendingParameters(params);
}

void SignalEngine::setEMGParameters(const EMGParameters& params) {
    emgModel.setPendingParameters(params);
}

void SignalEngine::setPPGParameters(const PPGParameters& params) {
    ppgModel.setPendingParameters(params);
}
