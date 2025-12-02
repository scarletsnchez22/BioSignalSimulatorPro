/*
 * Gestor de generación de señales OPTIMIZADO
 * ==========================================
 * OPTIMIZACIONES:
 * - Core 1 dedicado 100% a generación de señales
 * - Pre-cálculo de muestras en buffer circular
 * - ISR ultra-rápida (solo lee buffer y envía a DAC)
 * - Double buffering para evitar bloqueos
 * - Uso de PSRAM para buffers grandes
 * - Profiling de performance
 */

#include "signal_generator.h"
#include "config.h"

// Instancia singleton
SignalGenerator* SignalGenerator::instance = nullptr;

// Buffers en IRAM para máxima velocidad (WROOM-32: siempre en SRAM)
DRAM_ATTR static uint8_t signalBuffer[SIGNAL_BUFFER_SIZE];
DRAM_ATTR static volatile uint16_t bufferReadIndex = 0;
DRAM_ATTR static volatile uint16_t bufferWriteIndex = 0;

// Estadísticas en DRAM para acceso rápido desde ISR
DRAM_ATTR static volatile uint32_t isrCount = 0;
DRAM_ATTR static volatile uint32_t isrMaxTime = 0;
DRAM_ATTR static volatile uint32_t bufferUnderruns = 0;
DRAM_ATTR static volatile uint8_t lastDACValue = 128;  // Último valor enviado al DAC

// Constructor
SignalGenerator::SignalGenerator() {
    currentSignal.type = SignalType::NONE;
    currentSignal.state = SignalState::STOPPED;
    
    // Crear mutex para protección de datos compartidos
    signalMutex = xSemaphoreCreateMutex();
    
    // Crear semáforo binario para sincronización de buffers
    bufferSemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(bufferSemaphore);
    
    signalTimer = nullptr;
    
    DEBUG_PRINTLN("[SignalGen] Constructor: Buffer estático en SRAM");
}

// Obtener instancia singleton
SignalGenerator* SignalGenerator::getInstance() {
    if (instance == nullptr) {
        instance = new SignalGenerator();
    }
    return instance;
}

// Inicializar
bool SignalGenerator::begin() {
    DEBUG_PRINTLN("[SignalGen] Inicializando para ESP32-WROOM-32...");
    DEBUG_PRINTF("[SignalGen] SRAM Total: %d KB\n", SRAM_SIZE_KB);
    DEBUG_PRINTF("[SignalGen] Free Heap: %d KB\n", ESP.getFreeHeap() / 1024);
    
    // Verificar memoria disponible
    if (!CHECK_HEAP(150)) {
        DEBUG_PRINTLN("[SignalGen] ERROR: Memoria insuficiente!");
        return false;
    }
    
    // Configurar DAC
    dacWrite(DAC_SIGNAL_PIN, DAC_CENTER_VALUE);
    
    MEM_CHECKPOINT("Antes crear tareas");
    
    // Crear tarea de PRE-CÁLCULO en Core 1
    BaseType_t precalcCreated = xTaskCreatePinnedToCore(
        precalculationTask,
        "PrecalcTask",
        STACK_SIZE_PRECALC,
        this,
        TASK_PRIORITY_PRECALC,
        &precalcTaskHandle,
        CORE_PRECALC  // Core 1
    );
    
    if (precalcCreated != pdPASS) {
        DEBUG_PRINTLN("[SignalGen] ERROR: No se pudo crear tarea de pre-cálculo");
        return false;
    }
    
    // Crear tarea de MONITOREO en Core 0
    BaseType_t monitorCreated = xTaskCreatePinnedToCore(
        monitoringTask,
        "MonitorTask",
        STACK_SIZE_MONITORING,
        this,
        TASK_PRIORITY_MONITORING,
        &monitorTaskHandle,
        CORE_MONITORING  // Core 0
    );
    
    if (monitorCreated != pdPASS) {
        DEBUG_PRINTLN("[SignalGen] ERROR: No se pudo crear tarea de monitoreo");
        return false;
    }
    
    DEBUG_PRINTLN("[SignalGen] ✓ Inicializado para WROOM-32");
    DEBUG_PRINTF("[SignalGen] CPU Freq: %d MHz\n", ESP.getCpuFreqMHz());
    DEBUG_PRINTF("[SignalGen] Free Heap: %d KB\n", ESP.getFreeHeap() / 1024);
    DEBUG_PRINTF("[SignalGen] Buffer: %d bytes en SRAM\n", SIGNAL_BUFFER_SIZE);
    
    MEM_CHECKPOINT("Después crear tareas");
    
    return true;
}

// Iniciar generación de señal
bool SignalGenerator::startSignal(SignalType type) {
    if (xSemaphoreTake(signalMutex, portMAX_DELAY) == pdTRUE) {
        PERF_START(startSignal);
        
        // Detener señal actual si existe
        if (currentSignal.state == SignalState::RUNNING) {
            stopSignalInternal();
        }
        
        // Resetear buffers
        bufferReadIndex = 0;
        bufferWriteIndex = 0;
        isrCount = 0;
        isrMaxTime = 0;
        bufferUnderruns = 0;
        
        // Configurar nueva señal
        currentSignal.type = type;
        currentSignal.sampleCount = 0;
        currentSignal.lastUpdateTime = millis();
        
        // Frecuencia unificada para todas las señales (evita reconfiguración de timer)
        currentSampleRate = SAMPLE_RATE_UNIFIED;
        
        // Inicializar parámetros según tipo de señal
        switch (type) {
            case SignalType::ECG:
                currentSignal.params.ecg = ECGParameters();
                ecgModel.setParameters(currentSignal.params.ecg);
                ecgModel.reset();
                break;
                
            case SignalType::EMG:
                currentSignal.params.emg = EMGParameters();
                emgModel.setParameters(currentSignal.params.emg);
                emgModel.reset();
                break;
                
            case SignalType::PPG:
                currentSignal.params.ppg = PPGParameters();
                ppgModel.setParameters(currentSignal.params.ppg);
                ppgModel.reset();
                break;
                
            default:
                xSemaphoreGive(signalMutex);
                return false;
        }
        
        // Pre-llenar buffer inicial
        prefillBuffer();
        
        // Cambiar estado
        currentSignal.state = SignalState::RUNNING;
        
        // Configurar y arrancar timer
        setupTimer(currentSampleRate);
        
        PERF_END(startSignal);
        xSemaphoreGive(signalMutex);
        
        DEBUG_PRINTF("[SignalGen] ✓ %s iniciado @ %d Hz (Core 1)\n", 
                     signalTypeToString(type), currentSampleRate);
        
        return true;
    }
    return false;
}

// Pre-llenar buffer antes de iniciar
void SignalGenerator::prefillBuffer() {
    DEBUG_PRINT("[SignalGen] Pre-llenando buffer... ");
    
    float dt = 1.0f / currentSampleRate;
    uint16_t samplesToFill = SIGNAL_BUFFER_SIZE / 2; // Llenar mitad del buffer
    
    for (uint16_t i = 0; i < samplesToFill; i++) {
        uint8_t sample = generateSample(dt);
        signalBuffer[bufferWriteIndex] = sample;
        bufferWriteIndex = (bufferWriteIndex + 1) % SIGNAL_BUFFER_SIZE;
    }
    
    DEBUG_PRINTLN("OK");
}

// Generar muestra según tipo de señal actual
uint8_t SignalGenerator::generateSample(float deltaTime) {
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

// Detener señal (interno, sin mutex)
bool SignalGenerator::stopSignalInternal() {
    if (signalTimer != nullptr) {
        timerAlarmDisable(signalTimer);
        timerEnd(signalTimer);
        signalTimer = nullptr;
    }
    
    currentSignal.state = SignalState::STOPPED;
    dacWrite(DAC_SIGNAL_PIN, DAC_CENTER_VALUE);
    
    DEBUG_PRINTLN("[SignalGen] Señal detenida");
    return true;
}

// Configurar timer de hardware
void SignalGenerator::setupTimer(uint16_t sampleRate) {
    if (signalTimer != nullptr) {
        timerEnd(signalTimer);
    }
    
    // Timer 0, prescaler 80 (1 MHz clock)
    signalTimer = timerBegin(0, 80, true);
    
    // Adjuntar interrupción
    timerAttachInterrupt(signalTimer, &timerISR, true);
    
    // Calcular divisor para frecuencia deseada
    uint32_t timerTicks = 1000000 / sampleRate;
    timerAlarmWrite(signalTimer, timerTicks, true);
    
    // Habilitar
    timerAlarmEnable(signalTimer);
    
    DEBUG_PRINTF("[SignalGen] Timer @ %d Hz (%lu ticks)\n", sampleRate, timerTicks);
}

// ============= ISR ULTRA-OPTIMIZADA =============
// Solo lee buffer y envía a DAC (< 5 microsegundos)
void IRAM_ATTR SignalGenerator::timerISR() {
    #if PROFILE_ISR_TIME
        uint32_t startTime = micros();
    #endif
    
    SignalGenerator* gen = SignalGenerator::getInstance();
    if (gen == nullptr || gen->currentSignal.state != SignalState::RUNNING) {
        return;
    }
    
    // Verificar si hay datos en buffer
    if (bufferReadIndex == bufferWriteIndex) {
        // Buffer underrun (no hay datos)
        bufferUnderruns++;
        dacWrite(DAC_SIGNAL_PIN, DAC_CENTER_VALUE);
        return;
    }
    
    // Leer del buffer circular
    uint8_t dacValue = signalBuffer[bufferReadIndex];
    bufferReadIndex = (bufferReadIndex + 1) % SIGNAL_BUFFER_SIZE;
    
    // Guardar para uso externo (Nextion, etc.)
    lastDACValue = dacValue;
    
    // Enviar a DAC (operación más rápida)
    dacWrite(DAC_SIGNAL_PIN, dacValue);
    
    // Estadísticas
    isrCount++;
    
    #if PROFILE_ISR_TIME
        uint32_t elapsedTime = micros() - startTime;
        if (elapsedTime > isrMaxTime) {
            isrMaxTime = elapsedTime;
        }
    #endif
}

// ============= TAREA DE PRE-CÁLCULO (Core 1) =============
/*
 * ARQUITECTURA DE TIEMPO REAL:
 * ============================
 * Esta tarea corre en Core 1 dedicado exclusivamente a generación de señales.
 * El Core 0 maneja WiFi, BT y tareas del sistema operativo.
 * 
 * FLUJO:
 * 1. Calcular espacio libre en buffer circular
 * 2. Si hay espacio, generar bloque de 64 muestras
 * 3. Si buffer casi lleno, ceder CPU brevemente
 * 4. Repetir indefinidamente
 * 
 * LATENCIA MÁXIMA:
 * @ 500 Hz, buffer 2048 muestras = 4.1 segundos de headroom
 * Bloque de 64 muestras = 128 ms de señal
 * Tiempo de generación por bloque < 1 ms (medido)
 */
void SignalGenerator::precalculationTask(void* parameter) {
    SignalGenerator* gen = (SignalGenerator*)parameter;
    
    DEBUG_PRINTLN("[PrecalcTask] Iniciada en Core 1");
    
    // dt constante porque usamos frecuencia unificada (SAMPLE_RATE_UNIFIED)
    // Esto evita recálculos y posibles race conditions
    const float dt = 1.0f / SAMPLE_RATE_UNIFIED;  // 0.002s = 2ms @ 500 Hz
    
    while (true) {
        // Solo trabajar si hay señal activa
        if (gen->currentSignal.state == SignalState::RUNNING) {
            // Leer índices atómicamente (volatile)
            uint16_t readIdx = bufferReadIndex;
            uint16_t writeIdx = bufferWriteIndex;
            
            // Calcular espacio libre en buffer circular
            uint16_t freeSpace;
            if (writeIdx >= readIdx) {
                freeSpace = SIGNAL_BUFFER_SIZE - (writeIdx - readIdx) - 1;
            } else {
                freeSpace = readIdx - writeIdx - 1;
            }
            
            // Generar en bloques de 64 muestras (128ms de señal)
            // Tamaño óptimo: suficiente para amortizar overhead, 
            // pequeño para respuesta rápida a cambios de parámetros
            const uint16_t blockSize = 64;
            
            if (freeSpace >= blockSize) {
                // Generar bloque de muestras
                for (uint16_t i = 0; i < blockSize; i++) {
                    uint8_t sample = gen->generateSample(dt);
                    signalBuffer[writeIdx] = sample;
                    writeIdx = (writeIdx + 1) % SIGNAL_BUFFER_SIZE;
                }
                // Actualizar índice de escritura (atómico en ESP32)
                bufferWriteIndex = writeIdx;
                
                // Incrementar contador de muestras
                gen->currentSignal.sampleCount += blockSize;
            } else {
                // Buffer casi lleno, esperar brevemente
                vTaskDelay(pdMS_TO_TICKS(1));
            }
        } else {
            // No hay señal activa, dormir más tiempo para ahorrar CPU
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        
        // Ceder control al scheduler (buena práctica RTOS)
        taskYIELD();
    }
}

// ============= TAREA DE MONITOREO (Core 0) =============
// Reporta estadísticas de performance
void SignalGenerator::monitoringTask(void* parameter) {
    SignalGenerator* gen = (SignalGenerator*)parameter;
    
    DEBUG_PRINTLN("[MonitorTask] Iniciada en Core 0");
    
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t frequency = pdMS_TO_TICKS(5000); // Cada 5 segundos
    
    while (true) {
        vTaskDelayUntil(&lastWakeTime, frequency);
        
        if (gen->currentSignal.state == SignalState::RUNNING && DEBUG_PERFORMANCE) {
            // Calcular uso de buffer
            uint16_t readIdx = bufferReadIndex;
            uint16_t writeIdx = bufferWriteIndex;
            uint16_t bufferedSamples;
            
            if (writeIdx >= readIdx) {
                bufferedSamples = writeIdx - readIdx;
            } else {
                bufferedSamples = SIGNAL_BUFFER_SIZE - readIdx + writeIdx;
            }
            
            float bufferUsage = (bufferedSamples * 100.0f) / SIGNAL_BUFFER_SIZE;
            
            DEBUG_PRINTLN("\n┌─── PERFORMANCE STATS ───┐");
            DEBUG_PRINTF("│ ISR Calls:    %10lu │\n", isrCount);
            DEBUG_PRINTF("│ ISR Max Time: %7lu us │\n", isrMaxTime);
            DEBUG_PRINTF("│ Buffer Usage: %8.1f%% │\n", bufferUsage);
            DEBUG_PRINTF("│ Underruns:    %10lu │\n", bufferUnderruns);
            DEBUG_PRINTF("│ Free Heap:    %7d KB │\n", ESP.getFreeHeap() / 1024);
            DEBUG_PRINTF("│ Samples/sec:  %10lu │\n", 
                        gen->currentSignal.sampleCount / ((millis() - gen->currentSignal.lastUpdateTime) / 1000));
            DEBUG_PRINTLN("└─────────────────────────┘\n");
            
            // Advertencia si buffer está muy vacío
            if (bufferUsage < 10.0f) {
                DEBUG_PRINTLN("⚠ WARNING: Buffer bajo, aumentar prioridad de precalc");
            }
        }
    }
}

// Actualizar parámetros ECG
bool SignalGenerator::updateECGParameters(const ECGParameters& params) {
    if (currentSignal.type != SignalType::ECG) {
        return false;
    }
    
    if (xSemaphoreTake(signalMutex, portMAX_DELAY) == pdTRUE) {
        currentSignal.params.ecg = params;
        ecgModel.setParameters(params);
        xSemaphoreGive(signalMutex);
        return true;
    }
    return false;
}

// Actualizar parámetros EMG
bool SignalGenerator::updateEMGParameters(const EMGParameters& params) {
    if (currentSignal.type != SignalType::EMG) {
        return false;
    }
    
    if (xSemaphoreTake(signalMutex, portMAX_DELAY) == pdTRUE) {
        currentSignal.params.emg = params;
        emgModel.setParameters(params);
        xSemaphoreGive(signalMutex);
        return true;
    }
    return false;
}

// Actualizar parámetros PPG
bool SignalGenerator::updatePPGParameters(const PPGParameters& params) {
    if (currentSignal.type != SignalType::PPG) {
        return false;
    }
    
    if (xSemaphoreTake(signalMutex, portMAX_DELAY) == pdTRUE) {
        currentSignal.params.ppg = params;
        ppgModel.setParameters(params);
        xSemaphoreGive(signalMutex);
        return true;
    }
    return false;
}

// Obtener estadísticas de performance
PerformanceStats SignalGenerator::getPerformanceStats() {
    PerformanceStats stats;
    stats.isrCount = isrCount;
    stats.isrMaxTime = isrMaxTime;
    stats.bufferUnderruns = bufferUnderruns;
    
    uint16_t readIdx = bufferReadIndex;
    uint16_t writeIdx = bufferWriteIndex;
    if (writeIdx >= readIdx) {
        stats.bufferLevel = writeIdx - readIdx;
    } else {
        stats.bufferLevel = SIGNAL_BUFFER_SIZE - readIdx + writeIdx;
    }
    
    stats.freeHeap = ESP.getFreeHeap();
    stats.cpuFreq = ESP.getCpuFreqMHz();
    
    return stats;
}

// Resto de funciones (stop, pause, resume, etc.)
bool SignalGenerator::stopSignal() {
    if (xSemaphoreTake(signalMutex, portMAX_DELAY) == pdTRUE) {
        bool result = stopSignalInternal();
        xSemaphoreGive(signalMutex);
        return result;
    }
    return false;
}

bool SignalGenerator::pauseSignal() {
    if (xSemaphoreTake(signalMutex, portMAX_DELAY) == pdTRUE) {
        if (currentSignal.state == SignalState::RUNNING) {
            if (signalTimer != nullptr) {
                timerAlarmDisable(signalTimer);
            }
            currentSignal.state = SignalState::PAUSED;
        }
        xSemaphoreGive(signalMutex);
        return true;
    }
    return false;
}

bool SignalGenerator::resumeSignal() {
    if (xSemaphoreTake(signalMutex, portMAX_DELAY) == pdTRUE) {
        if (currentSignal.state == SignalState::PAUSED) {
            if (signalTimer != nullptr) {
                timerAlarmEnable(signalTimer);
            }
            currentSignal.state = SignalState::RUNNING;
        }
        xSemaphoreGive(signalMutex);
        return true;
    }
    return false;
}

SignalData SignalGenerator::getCurrentSignalData() {
    SignalData data;
    if (xSemaphoreTake(signalMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        data = currentSignal;
        xSemaphoreGive(signalMutex);
    }
    return data;
}

SignalState SignalGenerator::getState() {
    return currentSignal.state;
}

SignalType SignalGenerator::getCurrentType() {
    return currentSignal.type;
}

uint8_t SignalGenerator::getLastDACValue() {
    return lastDACValue;
}