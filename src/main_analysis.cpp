/**
 * @file main_analysis.cpp
 * @brief Modo ANÁLISIS - Captura de datos para validación de tesis
 * @version 1.0.0
 * @date Enero 2026
 * 
 * PROPÓSITO:
 * Captura de señales SIN interfaz Nextion para análisis automatizado.
 * Genera datos limpios en formato CSV/JSON para procesamiento por scripts Python.
 * 
 * ARQUITECTURA:
 * - Genera señales biomédicas (ECG, EMG, PPG)
 * - Salida analógica al DAC (GPIO25) para medición con osciloscopio
 * - Salida serial en formato estructurado para análisis Python
 * - Sin comunicación Nextion, sin servidor web
 * 
 * FORMATO DE SALIDA SERIAL:
 * timestamp_ms,signal_type,condition,value_mV,dac_value,metric1,metric2,...
 * 
 * Compilar y subir:
 * pio run -e esp32_analysis --target upload
 */

#include <Arduino.h>
#include "config.h"
#include "data/signal_types.h"
#include "models/ecg_model.h"
#include "models/emg_model.h"
#include "models/ppg_model.h"

// ============================================================================
// CONFIGURACIÓN DE CAPTURA AUTOMÁTICA
// ============================================================================

// Modo de captura: 0=TODAS las señales en secuencia, 1=Solo una señal
#define CAPTURE_MODE            0

// Si CAPTURE_MODE=1, especificar: 0=ECG, 1=EMG, 2=PPG
#define CAPTURE_SINGLE_SIGNAL   0

// Duración por señal/condición (segundos)
#define CAPTURE_DURATION_SEC    30

// ============================================================================
// CONFIGURACIÓN DE CONDICIONES A CAPTURAR
// ============================================================================
// Poner 1 para incluir, 0 para omitir

// ECG (8 condiciones)
#define CAPTURE_ECG_NORMAL              1
#define CAPTURE_ECG_TACHYCARDIA         1
#define CAPTURE_ECG_BRADYCARDIA         1
#define CAPTURE_ECG_ATRIAL_FIB          1
#define CAPTURE_ECG_VENTRICULAR_FIB     1
#define CAPTURE_ECG_AV_BLOCK            1
#define CAPTURE_ECG_ST_ELEVATION        1
#define CAPTURE_ECG_ST_DEPRESSION       1

// EMG (6 condiciones)
#define CAPTURE_EMG_REST                1
#define CAPTURE_EMG_LOW                 1
#define CAPTURE_EMG_MODERATE            1
#define CAPTURE_EMG_HIGH                1
#define CAPTURE_EMG_TREMOR              1
#define CAPTURE_EMG_FATIGUE             1

// PPG (6 condiciones)
#define CAPTURE_PPG_NORMAL              1
#define CAPTURE_PPG_ARRHYTHMIA          1
#define CAPTURE_PPG_WEAK_PERFUSION      1
#define CAPTURE_PPG_VASOCONSTRICTION    1
#define CAPTURE_PPG_STRONG_PERFUSION    1
#define CAPTURE_PPG_VASODILATION        1

// ============================================================================
// INSTANCIAS GLOBALES
// ============================================================================
ECGModel ecgModel;
EMGModel emgModel;
PPGModel ppgModel;

SignalType currentSignalType = SignalType::ECG;
ECGCondition currentECGCondition = ECGCondition::NORMAL;
EMGCondition currentEMGCondition = EMGCondition::REST;
PPGCondition currentPPGCondition = PPGCondition::NORMAL;

unsigned long captureStartTime = 0;
bool isCapturing = false;
uint32_t sampleCounter = 0;
uint32_t timerTickCounter = 0;

// Variables para captura secuencial
uint8_t currentCaptureIndex = 0;
uint8_t totalCaptures = 0;
bool captureSequenceActive = false;

// Estructura para definir capturas
struct CaptureTask {
    SignalType signalType;
    uint8_t condition;
    const char* name;
};

CaptureTask captureTasks[20];  // Máximo 20 capturas (8 ECG + 6 EMG + 6 PPG)

// Variables para timing e interpolación
static uint32_t lastModelTick_us = 0;
static uint8_t currentModelSample = 128;
static uint8_t previousModelSample = 128;
static uint16_t interpolationCounter = 0;
static uint32_t lastTimerTick_us = 0;

const uint32_t TIMER_TICK_US = 1000000 / FS_TIMER_HZ;  // 500 us = 2 kHz

// ============================================================================
// FUNCIONES DE CONFIGURACIÓN
// ============================================================================

const char* getSignalTypeName(SignalType type) {
    switch (type) {
        case SignalType::ECG: return "ECG";
        case SignalType::EMG: return "EMG";
        case SignalType::PPG: return "PPG";
        default: return "NONE";
    }
}

const char* getECGConditionName(ECGCondition cond) {
    switch (cond) {
        case ECGCondition::NORMAL: return "Normal";
        case ECGCondition::TACHYCARDIA: return "Taquicardia";
        case ECGCondition::BRADYCARDIA: return "Bradicardia";
        case ECGCondition::ATRIAL_FIBRILLATION: return "FibrilacionAuricular";
        case ECGCondition::VENTRICULAR_FIBRILLATION: return "FibrilacionVentricular";
        case ECGCondition::AV_BLOCK_1: return "BloqueoAV";
        case ECGCondition::ST_ELEVATION: return "ElevacionST";
        case ECGCondition::ST_DEPRESSION: return "DepresionST";
        default: return "Unknown";
    }
}

const char* getEMGConditionName(EMGCondition cond) {
    switch (cond) {
        case EMGCondition::REST: return "Reposo";
        case EMGCondition::LOW_CONTRACTION: return "Leve";
        case EMGCondition::MODERATE_CONTRACTION: return "Moderada";
        case EMGCondition::HIGH_CONTRACTION: return "Maxima";
        case EMGCondition::TREMOR: return "Temblor";
        case EMGCondition::FATIGUE: return "Fatiga";
        default: return "Unknown";
    }
}

const char* getPPGConditionName(PPGCondition cond) {
    switch (cond) {
        case PPGCondition::NORMAL: return "Normal";
        case PPGCondition::ARRHYTHMIA: return "Arritmia";
        case PPGCondition::WEAK_PERFUSION: return "PerfusionDebil";
        case PPGCondition::VASOCONSTRICTION: return "Vasoconstriccion";
        case PPGCondition::STRONG_PERFUSION: return "PerfusionFuerte";
        case PPGCondition::VASODILATION: return "Vasodilatacion";
        default: return "Unknown";
    }
}

void setupECG(ECGCondition cond) {
    currentSignalType = SignalType::ECG;
    currentECGCondition = cond;
    
    ECGParameters params;
    params.condition = cond;
    params.qrsAmplitude = 1.0f;
    params.noiseLevel = 0.02f;
    params.heartRate = 0;
    ecgModel.setParameters(params);
}

void setupEMG(EMGCondition cond) {
    currentSignalType = SignalType::EMG;
    currentEMGCondition = cond;
    
    EMGParameters params;
    params.condition = cond;
    params.noiseLevel = 0.02f;
    params.excitationLevel = 0.0f;
    params.amplitude = 1.0f;
    emgModel.setParameters(params);
}

void setupPPG(PPGCondition cond) {
    currentSignalType = SignalType::PPG;
    currentPPGCondition = cond;
    
    PPGParameters params;
    params.condition = cond;
    params.heartRate = 75.0f;
    params.perfusionIndex = 3.0f;
    params.noiseLevel = 0.02f;
    ppgModel.setParameters(params);
}

void buildCaptureSequence() {
    totalCaptures = 0;
    
    #if CAPTURE_MODE == 0  // TODAS las señales
    
    // ECG
    #if CAPTURE_ECG_NORMAL
    captureTasks[totalCaptures++] = {SignalType::ECG, 0, "ECG_Normal"};
    #endif
    #if CAPTURE_ECG_TACHYCARDIA
    captureTasks[totalCaptures++] = {SignalType::ECG, 1, "ECG_Taquicardia"};
    #endif
    #if CAPTURE_ECG_BRADYCARDIA
    captureTasks[totalCaptures++] = {SignalType::ECG, 2, "ECG_Bradicardia"};
    #endif
    #if CAPTURE_ECG_ATRIAL_FIB
    captureTasks[totalCaptures++] = {SignalType::ECG, 3, "ECG_FibrilacionAuricular"};
    #endif
    #if CAPTURE_ECG_VENTRICULAR_FIB
    captureTasks[totalCaptures++] = {SignalType::ECG, 4, "ECG_FibrilacionVentricular"};
    #endif
    #if CAPTURE_ECG_AV_BLOCK
    captureTasks[totalCaptures++] = {SignalType::ECG, 5, "ECG_BloqueoAV"};
    #endif
    #if CAPTURE_ECG_ST_ELEVATION
    captureTasks[totalCaptures++] = {SignalType::ECG, 6, "ECG_ElevacionST"};
    #endif
    #if CAPTURE_ECG_ST_DEPRESSION
    captureTasks[totalCaptures++] = {SignalType::ECG, 7, "ECG_DepresionST"};
    #endif
    
    // EMG
    #if CAPTURE_EMG_REST
    captureTasks[totalCaptures++] = {SignalType::EMG, 0, "EMG_Reposo"};
    #endif
    #if CAPTURE_EMG_LOW
    captureTasks[totalCaptures++] = {SignalType::EMG, 1, "EMG_Leve"};
    #endif
    #if CAPTURE_EMG_MODERATE
    captureTasks[totalCaptures++] = {SignalType::EMG, 2, "EMG_Moderada"};
    #endif
    #if CAPTURE_EMG_HIGH
    captureTasks[totalCaptures++] = {SignalType::EMG, 3, "EMG_Maxima"};
    #endif
    #if CAPTURE_EMG_TREMOR
    captureTasks[totalCaptures++] = {SignalType::EMG, 4, "EMG_Temblor"};
    #endif
    #if CAPTURE_EMG_FATIGUE
    captureTasks[totalCaptures++] = {SignalType::EMG, 5, "EMG_Fatiga"};
    #endif
    
    // PPG
    #if CAPTURE_PPG_NORMAL
    captureTasks[totalCaptures++] = {SignalType::PPG, 0, "PPG_Normal"};
    #endif
    #if CAPTURE_PPG_ARRHYTHMIA
    captureTasks[totalCaptures++] = {SignalType::PPG, 1, "PPG_Arritmia"};
    #endif
    #if CAPTURE_PPG_WEAK_PERFUSION
    captureTasks[totalCaptures++] = {SignalType::PPG, 2, "PPG_PerfusionDebil"};
    #endif
    #if CAPTURE_PPG_VASOCONSTRICTION
    captureTasks[totalCaptures++] = {SignalType::PPG, 3, "PPG_Vasoconstriccion"};
    #endif
    #if CAPTURE_PPG_STRONG_PERFUSION
    captureTasks[totalCaptures++] = {SignalType::PPG, 4, "PPG_PerfusionFuerte"};
    #endif
    #if CAPTURE_PPG_VASODILATION
    captureTasks[totalCaptures++] = {SignalType::PPG, 5, "PPG_Vasodilatacion"};
    #endif
    
    #else  // Solo una señal
    
    #if CAPTURE_SINGLE_SIGNAL == 0  // ECG
    captureTasks[totalCaptures++] = {SignalType::ECG, CAPTURE_ECG_CONDITION, "ECG"};
    #elif CAPTURE_SINGLE_SIGNAL == 1  // EMG
    if (now_ms - captureStartTime >= (CAPTURE_DURATION_SEC * 1000UL)) {
        isCapturing = false;
        
        Serial.println();
        Serial.println("# ----------------------------------------------------------------");
        Serial.printf("# Captura completada: %lu muestras en %.1f segundos\n", 
                     sampleCounter, 
                     (now_ms - captureStartTime) / 1000.0f);
        Serial.println("# ----------------------------------------------------------------");
        
        // Pasar a siguiente captura si estamos en modo secuencial
        if (captureSequenceActive) {
            currentCaptureIndex++;
            delay(1000);  // Pausa entre capturas
            startNextCapture();
        }
        return;
    }rial.println();
        Serial.println("# ================================================================");
        Serial.println("# SECUENCIA DE CAPTURA COMPLETADA");
        Serial.printf("# Total capturas: %d\n", totalCaptures);
        Serial.println("# ================================================================");
        return;
    }
    
    CaptureTask& task = captureTasks[currentCaptureIndex];
    
    // Configurar señal según tarea
    if (task.signalType == SignalType::ECG) {
        setupECG((ECGCondition)task.condition);
    } else if (task.signalType == SignalType::EMG) {
        setupEMG((EMGCondition)task.condition);
    } else {
        setupPPG((PPGCondition)task.condition);
    }
    
    // Banner de captura
    Serial.println();
    Serial.println("# ================================================================");
    Serial.printf("# CAPTURA %d/%d: %s\n", currentCaptureIndex + 1, totalCaptures, task.name);
    Serial.println("# ================================================================");
    Serial.printf("# Duración: %d segundos\n", CAPTURE_DURATION_SEC);
    Serial.println("#");
    
    // Encabezado CSV según tipo de señal
    if (task.signalType == SignalType::ECG) {
        Serial.println("# timestamp_ms,signal,condition,value_mV,dac_value,hr,rr,pr,qrs,qtc,r_amp,st");
    } else if (task.signalType == SignalType::EMG) {
        Serial.println("# timestamp_ms,signal,condition,raw_mV,env_mV,dac_raw,dac_env,rms,mus,fr,mvc");
    } else {
        Serial.println("# timestamp_ms,signal,condition,ac_mV,dac_value,hr,rr,pi,ac_amp,sys,dia");
    }
    
    Serial.println("# ================================================================");
    Serial.println();
    
    // Reiniciar contadores
    sampleCounter = 0;
    timerTickCounter = 0;
    lastModelTick_us = 0;
    lastTimerTick_us = 0;
    interpolationCounter = 0;
    
    captureStartTime = millis();
    isCapturing = true;
}

// ============================================================================
// SETUP
// ============================================================================
void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // Inicializar DAC
    pinMode(DAC_SIGNAL_PIN, OUTPUT);
    dacWrite(DAC_SIGNAL_PIN, 128);
    
    // Banner principal
    Serial.println();
    Serial.println("# ================================================================");
    Serial.println("# BIOSIGNALSIMULATOR PRO - MODO ANÁLISIS TESIS");
    Serial.println("# ================================================================");
    Serial.println("#");
    
    // Construir secuencia de capturas
    buildCaptureSequence();
    
    #if CAPTURE_MODE == 0
    Serial.println("# MODO: CAPTURA SECUENCIAL DE TODAS LAS SEÑALES");
    Serial.printf("# Total de capturas: %d\n", totalCaptures);
    Serial.printf("# Duración por captura: %d segundos\n", CAPTURE_DURATION_SEC);
    Serial.printf("# Tiempo total estimado: %d segundos (~%.1f minutos)\n", 
                 totalCaptures * CAPTURE_DURATION_SEC,
                 (totalCaptures * CAPTURE_DURATION_SEC) / 60.0f);
    #else
    Serial.println("# MODO: CAPTURA ÚNICA");
    Serial.printf("# Duración: %d segundos\n", CAPTURE_DURATION_SEC);
    #endif
    
    Serial.println("#");
    Serial.println("# FORMATO DE SALIDA: CSV");
    Serial.println("# Las líneas con '#' son comentarios y deben ignorarse al procesar");
    Serial.println("# ================================================================");
    
    delay(2000);
    
    // Iniciar primera captura
    currentCaptureIndex = 0;
    captureSequenceActive = true;
    startNextCapture();
}

// ============================================================================
// LOOP
// ============================================================================
void loop() {
    if (!isCapturing) return;
    
    unsigned long now_us = micros();
    unsigned long now_ms = millis();
    
    // Verificar duración
    #if CAPTURE_DURATION_SEC > 0
    if (now_ms - captureStartTime >= (CAPTURE_DURATION_SEC * 1000UL)) {
        isCapturing = false;
        Serial.println();
        Serial.println("# ================================================================");
        Serial.println("# CAPTURA COMPLETADA");
        Serial.printf("# Total muestras modelo: %lu\n", sampleCounter);
        Serial.printf("# Duración: %.1f segundos\n", (now_ms - captureStartTime) / 1000.0f);
        Serial.println("# ================================================================");
        return;
    }
    #endif
    
    // Determinar intervalos según tipo de señal
    uint32_t modelTickInterval_us;
    uint8_t upsampleRatio;
    float modelDeltaTime;
    uint8_t downsampleRatio;
    
    #if CAPTURE_SIGNAL_TYPE == 0  // ECG
        modelTickInterval_us = MODEL_TICK_US_ECG;
        upsampleRatio = UPSAMPLE_RATIO_ECG;
        modelDeltaTime = MODEL_DT_ECG;
        downsampleRatio = NEXTION_DOWNSAMPLE_ECG;
    #elif CAPTURE_SIGNAL_TYPE == 1  // EMG
        modelTickInterval_us = MODEL_TICK_US_EMG;
        upsampleRatio = UPSAMPLE_RATIO_EMG;
        modelDeltaTime = MODEL_DT_EMG;
        downsampleRatio = NEXTION_DOWNSAMPLE_EMG;
    #else  // PPG
        modelTickInterval_us = MODEL_TICK_US_PPG;
        upsampleRatio = UPSAMPLE_RATIO_PPG;
        modelDeltaTime = MODEL_DT_PPG;
        downsampleRatio = NEXTION_DOWNSAMPLE_PPG;
    #endif
    
    // Generar nueva muestra del modelo
    if (now_us - lastModelTick_us >= modelTickInterval_us) {
        lastModelTick_us = now_us;
        
        previousModelSample = currentModelSample;
        
        #if CAPTURE_SIGNAL_TYPE == 0  // ECG
            currentModelSample = ecgModel.getDACValue(modelDeltaTime);
        #elif CAPTURE_SIGNAL_TYPE == 1  // EMG
            emgModel.tick(modelDeltaTime);
            currentModelSample = emgModel.getRawDACValue();
        #else  // PPG
            currentModelSample = ppgModel.getDACValue(modelDeltaTime);
        #endif
        
        interpolationCounter = 0;
        sampleCounter++;
    }
    
    // Timer tick @ 2 kHz
    bool timerTick = false;
    uint8_t dacValue = 128;
    
    if (now_us - lastTimerTick_us >= TIMER_TICK_US) {
        lastTimerTick_us = now_us;
        timerTick = true;
        timerTickCounter++;
        
        // Interpolación lineal
        float t = (float)interpolationCounter / (float)upsampleRatio;
        int16_t interpolated = previousModelSample + 
                               (int16_t)((currentModelSample - previousModelSample) * t);
        
        if (interpolated < 0) interpolated = 0;
        if (interpolated > 255) interpolated = 255;
        
        dacValue = (uint8_t)interpolated;
        dacWrite(DAC_SIGNAL_PIN, dacValue);
        
        interpolationCounter++;
        if (interpolationCounter >= upsampleRatio) {
            interpolationCounter = 0;
        }
    }
    
    // ========================================================================
    // SALIDA SERIAL EN FORMATO CSV
    // ========================================================================
    if (timerTick && (timerTickCounter % downsampleRatio == 0)) {
        
        #if CAPTURE_SIGNAL_TYPE == 0  // ECG
            float mV = ecgModel.getCurrentValueMV();
            ECGDisplayMetrics m = ecgModel.getDisplayMetrics();
            
            // timestamp_ms,signal,condition,value_mV,dac_value,hr,rr,pr,qrs,qtc,r_amp,st
            Serial.printf("%lu,ECG,%s,%.4f,%d,%.1f,%.1f,%.1f,%.1f,%.1f,%.3f,%.3f\n",
                         now_ms - captureStartTime,
                         getECGConditionName(currentECGCondition),
                         mV,
                         dacValue,
                         m.bpm,
                         m.rrInterval_ms,
                         m.prInterval_ms,
                         m.qrsDuration_ms,
                         m.qtcInterval_ms,
                         m.rAmplitude_mV,
                         m.stDeviation_mV);
                         
        #elif CAPTURE_SIGNAL_TYPE == 1  // EMG
            float rawMV = emgModel.getRawSample();
            float envMV = emgModel.getProcessedSample();
            uint8_t dacRaw = emgModel.getRawDACValue();
            uint8_t dacEnv = emgModel.getEnvelopeDACValue();
            
            // timestamp_ms,signal,condition,raw_mV,env_mV,dac_raw,dac_env,rms,mus,fr,mvc
            Serial.printf("%lu,EMG,%s,%.4f,%.4f,%d,%d,%.4f,%d,%.1f,%.1f\n",
                         now_ms - captureStartTime,
                         getEMGConditionName(currentEMGCondition),
                         rawMV,
                         envMV,
                         dacRaw,
                         dacEnv,
                         emgModel.getRMSAmplitude(),
                         emgModel.getActiveMotorUnits(),
                         emgModel.getMeanFiringRate(),
                         emgModel.getContractionLevel());
                         
        #else  // PPG
            float acMV = ppgModel.getLastACValue();
            
            // timestamp_ms,signal,condition,ac_mV,dac_value,hr,rr,pi,ac_amp,sys,dia
            Serial.printf("%lu,PPG,%s,%.4f,%d,%.1f,%.1f,%.3f,%.3f,%.1f,%.1f\n",
                         now_ms - captureStartTime,
                         getPPGConditionName(currentPPGCondition),
                         acMV,
                         dacValue,
                         ppgModel.getMeasuredHR(),
                         ppgModel.getMeasuredRRInterval(),
                         ppgModel.getMeasuredPI(),
                         ppgModel.getMeasuredACAmplitude(),
                         ppgModel.getMeasuredSystoleTime(),
                         ppgModel.getMeasuredDiastoleTime());
        #endif
    }
}
