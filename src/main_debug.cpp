/**
 * @file main_debug.cpp
 * @brief Auto-Start para Serial Plotter
 * @version 3.0.0
 * @date Diciembre 2025
 * 
 * Firmware ESP32 - BioSimulator Pro
 * 
 * Configuracion via #defines:
 * - Tipo de senal (ECG, EMG, PPG)
 * - Condicion/patologia
 * - Modo continuo o duracion fija
 * 
 * Frecuencias de muestreo (Nyquist 5x):
 * - ECG: 750 Hz (fmax=150Hz)
 * - PPG: 100 Hz (fmax=20Hz)
 * - EMG: 2000 Hz (fmax=400Hz)
 * 
 * Hardware: ESP32-WROOM-32
 * Salida DAC: GPIO25 (0-3.3V)
 * Nextion: UART2 @ 115200 baud (waveform 700x380)
 * 
 * # Compilar y subir
 * pio run -e esp32_debug --target upload
 */

#include <Arduino.h>
#include "config.h"
#include "data/signal_types.h"
#include "models/ecg_model.h"
#include "models/emg_model.h"
#include "models/ppg_model.h"

// ============================================================================
// CONFIGURACION AUTO-START
// ============================================================================

// --------------------------------------------------------------------------
// TIPO DE SENAL: 0=ECG, 1=EMG, 2=PPG
// --------------------------------------------------------------------------
#define AUTO_SIGNAL_TYPE        1

// --------------------------------------------------------------------------
// CONDICIONES ECG (solo si AUTO_SIGNAL_TYPE==0):
// 0=NORMAL, 1=TACHYCARDIA, 2=BRADYCARDIA, 3=ATRIAL_FIBRILLATION,
// 4=VENTRICULAR_FIBRILLATION, 5=AV_BLOCK_1, 6=ST_ELEVATION, 7=ST_DEPRESSION
// --------------------------------------------------------------------------
#define AUTO_ECG_CONDITION      2

// --------------------------------------------------------------------------
// CONDICIONES EMG (solo si AUTO_SIGNAL_TYPE==1):
// 0=REST, 1=LOW_CONTRACTION, 2=MODERATE_CONTRACTION, 
// 3=HIGH_CONTRACTION, 4=TREMOR, 5=FATIGUE
// --------------------------------------------------------------------------
#define AUTO_EMG_CONDITION      0

// --------------------------------------------------------------------------
// CONDICIONES PPG (solo si AUTO_SIGNAL_TYPE==2):
// 0=NORMAL, 1=ARRHYTHMIA, 2=WEAK_PERFUSION, 
// 3=VASODILATION, 4=STRONG_PERFUSION, 5=VASOCONSTRICTION
// --------------------------------------------------------------------------
#define AUTO_PPG_CONDITION      0

// --------------------------------------------------------------------------
// MODO DE EJECUCION
// --------------------------------------------------------------------------
#define AUTO_CONTINUOUS         1       // 1=continuo, 0=duracion fija
#define PLOT_DURATION_MS        10000   // Duracion en ms (si no es continuo)

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

unsigned long plotStartTime = 0;
bool isRunning = false;

// Variables para tracking
float minVal = 999.0f;
float maxVal = -999.0f;

// Contadores de muestras para downsampling
uint32_t sampleCounter = 0;
uint32_t timerTickCounter = 0;  // Contador de ticks del timer (para downsampling)

// ============================================================================
// ARQUITECTURA: Fs_timer = 4 kHz (igual que signal_engine.cpp)
// ============================================================================
// Timer tick interval en microsegundos
const uint32_t TIMER_TICK_US = 1000000 / FS_TIMER_HZ;  // 250 us = 4 kHz

// Variables para timing real e interpolación (igual que signal_engine)
static uint32_t lastModelTick_us = 0;
static uint8_t currentModelSample = 128;
static uint8_t previousModelSample = 128;
static uint16_t interpolationCounter = 0;
static uint32_t lastTimerTick_us = 0;

// Downsampling para Serial Plotter (respecto a Fs_timer, NO respecto al modelo)
// Ratio = Fs_timer / Fds
#define PLOT_DOWNSAMPLE_ECG  (FS_TIMER_HZ / FDS_ECG)   // 4000/200 = 20
#define PLOT_DOWNSAMPLE_EMG  (FS_TIMER_HZ / FDS_EMG)   // 4000/100 = 40
#define PLOT_DOWNSAMPLE_PPG  (FS_TIMER_HZ / FDS_PPG)   // 4000/100 = 40

// ============================================================================
// NOMBRES DE CONDICIONES
// ============================================================================
const char* getECGConditionName(ECGCondition cond) {
    switch (cond) {
        case ECGCondition::NORMAL:                  return "Normal";
        case ECGCondition::TACHYCARDIA:             return "Taquicardia";
        case ECGCondition::BRADYCARDIA:             return "Bradicardia";
        case ECGCondition::ATRIAL_FIBRILLATION:     return "Fibrilacion Auricular";
        case ECGCondition::VENTRICULAR_FIBRILLATION: return "Fibrilacion Ventricular";
        case ECGCondition::AV_BLOCK_1:              return "Bloqueo AV 1er Grado";
        case ECGCondition::ST_ELEVATION:            return "Elevacion ST";
        case ECGCondition::ST_DEPRESSION:           return "Depresion ST";
        default:                                    return "Desconocido";
    }
}

const char* getEMGConditionName(EMGCondition cond) {
    switch (cond) {
        case EMGCondition::REST:                 return "Reposo";
        case EMGCondition::LOW_CONTRACTION:      return "Contraccion Baja";
        case EMGCondition::MODERATE_CONTRACTION: return "Contraccion Moderada";
        case EMGCondition::HIGH_CONTRACTION:     return "Contraccion Alta";
        case EMGCondition::TREMOR:               return "Temblor";
        case EMGCondition::FATIGUE:              return "Fatiga";
        default:                                 return "Desconocido";
    }
}

const char* getPPGConditionName(PPGCondition cond) {
    switch (cond) {
        case PPGCondition::NORMAL:            return "Normal";
        case PPGCondition::ARRHYTHMIA:        return "Arritmia";
        case PPGCondition::WEAK_PERFUSION:    return "Perfusion Debil";
        case PPGCondition::VASODILATION:      return "Vasodilatacion";
        case PPGCondition::STRONG_PERFUSION:  return "Perfusion Fuerte";
        case PPGCondition::VASOCONSTRICTION:  return "Vasoconstriccion";
        default:                              return "Desconocido";
    }
}

// ============================================================================
// RANGOS CLINICOS ESPERADOS
// ============================================================================
struct ExpectedRange {
    float valMin, valMax;
    float val2Min, val2Max;
    const char* description;
};

// Rangos clinicos ECG (indices alineados con ECGCondition enum)
// 0=NORMAL, 1=TACHYCARDIA, 2=BRADYCARDIA, 3=AFIB, 4=VFIB, 5=AVB1, 6=STE, 7=STD
static const ExpectedRange ECG_RANGES[] = {
    { 60.0f, 100.0f, 0.0f, 0.0f, "Normal: HR 60-100 BPM" },
    { 100.0f, 180.0f, 0.0f, 0.0f, "Taquicardia: HR 100-180 BPM" },
    { 30.0f, 59.0f, 0.0f, 0.0f, "Bradicardia: HR 30-59 BPM" },
    { 60.0f, 180.0f, 0.0f, 0.0f, "AFib: HR 60-180, RR irregular" },
    { 150.0f, 500.0f, 0.0f, 0.0f, "VFib: 4-10 Hz caotico" },
    { 60.0f, 100.0f, 0.0f, 0.0f, "BAV1: HR 60-100, PR>200ms" },
    { 50.0f, 110.0f, 0.0f, 0.0f, "STEMI: HR 50-110, ST>=0.2mV" },
    { 50.0f, 150.0f, 0.0f, 0.0f, "NSTEMI: HR 50-150, ST<=-0.05mV" }
};

// Rangos clinicos EMG (indices alineados con EMGCondition enum)
// 0=REST, 1=LOW, 2=MODERATE, 3=HIGH, 4=TREMOR, 5=FATIGUE
static const ExpectedRange EMG_RANGES[] = {
    { 0.001f, 0.01f, 0.0f, 0.0f, "Reposo: RMS <10uV" },
    { 0.05f, 0.60f, 0.0f, 0.0f, "Baja: RMS 50-600uV (5-20% MVC)" },
    { 0.50f, 2.00f, 0.0f, 0.0f, "Moderada: RMS 0.5-2mV (20-50% MVC)" },
    { 1.50f, 3.50f, 0.0f, 0.0f, "Alta: RMS 1.5-3.5mV (50-100% MVC)" },
    { 0.10f, 0.50f, 0.0f, 0.0f, "Temblor: RMS 0.1-0.5mV, 4-6Hz" },
    { 0.40f, 1.50f, 0.0f, 0.0f, "Fatiga: RMS decae 1.5->0.4mV" }
};

// Rangos clinicos PPG (indices alineados con PPGCondition enum)
// 0=NORMAL, 1=ARRHYTHMIA, 2=WEAK_PERFUSION, 3=VASODILATION, 4=STRONG_PERFUSION, 5=VASOCONSTRICTION
// Valores segun RANGOS_CLINICOS.md y ppg_model.cpp initConditionRanges()
static const ExpectedRange PPG_RANGES[] = {
    { 60.0f, 100.0f, 2.9f, 6.1f, "Normal: HR 60-100, PI 2.9-6.1%" },
    { 60.0f, 180.0f, 1.0f, 5.0f, "Arritmia: HR 60-180, PI 1-5%" },
    { 70.0f, 120.0f, 0.5f, 2.1f, "Perf.Debil: HR 70-120, PI 0.5-2.1%" },
    { 60.0f, 90.0f, 5.0f, 10.0f, "Vasodilatacion: HR 60-90, PI 5-10%" },
    { 60.0f, 90.0f, 7.0f, 20.0f, "Perf.Fuerte: HR 60-90, PI 7-20%" },
    { 65.0f, 110.0f, 0.7f, 0.8f, "Vasoconstric: HR 65-110, PI 0.7-0.8%" }
};

// ============================================================================
// CONFIGURAR MODELOS
// ============================================================================
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

// ============================================================================
// SETUP
// ============================================================================
void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // Inicializar DAC
    pinMode(DAC_SIGNAL_PIN, OUTPUT);
    dacWrite(DAC_SIGNAL_PIN, 128);
    
    // Banner de inicio
    Serial.println();
    Serial.println("------------------------------------------------------------");
    Serial.println("  BIOSIMULATOR PRO - DEBUG MODE v3.0");
    Serial.println("------------------------------------------------------------");
    
    // Configurar senal segun tipo seleccionado
    #if AUTO_SIGNAL_TYPE == 0
        setupECG((ECGCondition)AUTO_ECG_CONDITION);
        Serial.printf("  Senal: ECG - %s\n", getECGConditionName(currentECGCondition));
        Serial.printf("  Frecuencia modelo: %d Hz (dt=%.3f ms)\n", 
                     MODEL_SAMPLE_RATE_ECG, MODEL_DT_ECG * 1000.0f);
    #elif AUTO_SIGNAL_TYPE == 1
        setupEMG((EMGCondition)AUTO_EMG_CONDITION);
        Serial.printf("  Senal: EMG - %s\n", getEMGConditionName(currentEMGCondition));
        Serial.printf("  Frecuencia modelo: %d Hz (dt=%.3f ms)\n", 
                     MODEL_SAMPLE_RATE_EMG, MODEL_DT_EMG * 1000.0f);
    #else
        setupPPG((PPGCondition)AUTO_PPG_CONDITION);
        Serial.printf("  Senal: PPG - %s\n", getPPGConditionName(currentPPGCondition));
        Serial.printf("  Frecuencia modelo: %d Hz (dt=%.3f ms)\n", 
                     MODEL_SAMPLE_RATE_PPG, MODEL_DT_PPG * 1000.0f);
    #endif
    
    Serial.printf("  Nextion: %d Hz (downsampling 1:%d)\n", 
                 NEXTION_SEND_RATE, 
                 #if AUTO_SIGNAL_TYPE == 0
                     NEXTION_DOWNSAMPLE_ECG
                 #elif AUTO_SIGNAL_TYPE == 1
                     NEXTION_DOWNSAMPLE_EMG
                 #else
                     NEXTION_DOWNSAMPLE_PPG
                 #endif
                 );
    Serial.printf("  Waveform Nextion: %dx%d px, ~%.1f seg visibles\n",
                 NEXTION_WAVEFORM_WIDTH, NEXTION_WAVEFORM_HEIGHT,
                 (float)NEXTION_WAVEFORM_WIDTH / NEXTION_SEND_RATE);
    
    #if AUTO_CONTINUOUS
        Serial.println("  Modo: CONTINUO (presiona 'r' para reiniciar)");
    #else
        Serial.printf("  Modo: DURACION FIJA - %.1f segundos\n", PLOT_DURATION_MS / 1000.0f);
    #endif
    
    Serial.println("------------------------------------------------------------");
    Serial.println();
    delay(1500);
    
    plotStartTime = millis();
    isRunning = true;
    minVal = 999.0f;
    maxVal = -999.0f;
}

// ============================================================================
// LOOP PRINCIPAL
// ============================================================================
void loop() {
    if (!isRunning) return;
    
    static unsigned long lastStats = 0;
    
    unsigned long now_us = micros();
    unsigned long now_ms = millis();
    
    // Verificar comandos serial
    if (Serial.available()) {
        char c = Serial.read();
        if (c == 'r' || c == 'R') {
            // Reiniciar
            setup();
            return;
        }
    }
    
    // Verificar duracion (modo no continuo)
    #if !AUTO_CONTINUOUS
    if (now_ms - plotStartTime >= PLOT_DURATION_MS) {
        isRunning = false;
        Serial.println("\n--- FIN DE CAPTURA ---");
        return;
    }
    #endif
    
    // ========== ARQUITECTURA IGUAL A signal_engine.cpp ==========
    // 1. Modelo genera a su propia Fs (con timing real)
    // 2. Interpolación lineal llena muestras a Fs_timer
    // 3. DAC escribe a Fs_timer (4 kHz)
    // 4. Downsampling = Fs_timer / Fds
    
    // Obtener parámetros según tipo de señal
    uint32_t modelTickInterval_us;
    uint8_t upsampleRatio;
    float modelDeltaTime;
    
    #if AUTO_SIGNAL_TYPE == 0  // ECG
        modelTickInterval_us = MODEL_TICK_US_ECG;  // 1333 us
        upsampleRatio = UPSAMPLE_RATIO_ECG;        // 5
        modelDeltaTime = MODEL_DT_ECG;             // 1.333 ms
    #elif AUTO_SIGNAL_TYPE == 1  // EMG
        modelTickInterval_us = MODEL_TICK_US_EMG;  // 500 us
        upsampleRatio = UPSAMPLE_RATIO_EMG;        // 2
        modelDeltaTime = MODEL_DT_EMG;             // 0.5 ms
    #else  // PPG
        modelTickInterval_us = MODEL_TICK_US_PPG;  // 10000 us
        upsampleRatio = UPSAMPLE_RATIO_PPG;        // 40
        modelDeltaTime = MODEL_DT_PPG;             // 10 ms
    #endif
    
    // ¿Es hora de generar nueva muestra del modelo? (timing real)
    if (now_us - lastModelTick_us >= modelTickInterval_us) {
        lastModelTick_us = now_us;
        
        // Guardar muestra anterior para interpolación
        previousModelSample = currentModelSample;
        
        // Generar nueva muestra del modelo con su deltaTime correcto
        #if AUTO_SIGNAL_TYPE == 0  // ECG
            currentModelSample = ecgModel.getDACValue(modelDeltaTime);
        #elif AUTO_SIGNAL_TYPE == 1  // EMG
            emgModel.tick(modelDeltaTime);
            currentModelSample = emgModel.getRawDACValue();
        #else  // PPG
            currentModelSample = ppgModel.getDACValue(modelDeltaTime);
        #endif
        
        // Resetear contador de interpolación
        interpolationCounter = 0;
        sampleCounter++;  // Contador de muestras del modelo
    }
    
    // ========== TIMER TICK @ 4 kHz (escribir DAC con interpolación) ==========
    bool timerTick = false;
    float value = 0.0f;
    uint8_t dacValue = 128;
    
    if (now_us - lastTimerTick_us >= TIMER_TICK_US) {
        lastTimerTick_us = now_us;
        timerTick = true;
        timerTickCounter++;
        
        // Interpolación lineal: sample = prev + (curr - prev) * t
        float t = (float)interpolationCounter / (float)upsampleRatio;
        int16_t interpolated = previousModelSample + 
                               (int16_t)((currentModelSample - previousModelSample) * t);
        
        // Clamp a rango DAC
        if (interpolated < 0) interpolated = 0;
        if (interpolated > 255) interpolated = 255;
        
        dacValue = (uint8_t)interpolated;
        dacWrite(DAC_SIGNAL_PIN, dacValue);
        
        // Avanzar contador de interpolación
        interpolationCounter++;
        if (interpolationCounter >= upsampleRatio) {
            interpolationCounter = 0;
        }
        
        // Obtener valor para tracking
        #if AUTO_SIGNAL_TYPE == 0
            value = ecgModel.getCurrentValueMV();
        #elif AUTO_SIGNAL_TYPE == 1
            value = emgModel.getRawSample();
        #else
            value = ppgModel.getLastACValue();
        #endif
        
        // Track min/max
        if (value < minVal) minVal = value;
        if (value > maxVal) maxVal = value;
    }
    
    // ========== ENVIO A SERIAL PLOTTER (con downsampling respecto a Fs_timer) ==========
    // Formato VS Code Serial Plotter: >var1:val,var2:val\r\n
    // Ratio = Fs_timer / Fds: ECG=20, EMG=40, PPG=40
    if (timerTick) {
        
        #if AUTO_SIGNAL_TYPE == 0  // ECG @ 200 Hz efectivo
        if (timerTickCounter % PLOT_DOWNSAMPLE_ECG == 0) {
            ECGDisplayMetrics m = ecgModel.getDisplayMetrics();
            // Señal instantánea
            Serial.print(">ecg:");
            Serial.print(ecgModel.getCurrentValueMV(), 2);
            // Frecuencia y intervalos
            Serial.print(",hr:");
            Serial.print(m.bpm, 0);
            Serial.print(",rr:");
            Serial.print(m.rrInterval_ms, 0);
            Serial.print(",pr:");
            Serial.print(m.prInterval_ms, 0);
            Serial.print(",qrs:");
            Serial.print(m.qrsDuration_ms, 0);
            Serial.print(",qt:");
            Serial.print(m.qtInterval_ms, 0);
            Serial.print(",qtc:");
            Serial.print(m.qtcInterval_ms, 0);
            // Amplitudes de ondas PQRST
            Serial.print(",p:");
            Serial.print(m.pAmplitude_mV, 2);
            Serial.print(",q:");
            Serial.print(m.qAmplitude_mV, 2);
            Serial.print(",r:");
            Serial.print(m.rAmplitude_mV, 2);
            Serial.print(",s:");
            Serial.print(m.sAmplitude_mV, 2);
            Serial.print(",t:");
            Serial.print(m.tAmplitude_mV, 2);
            Serial.print(",st:");
            Serial.print(m.stDeviation_mV, 2);
            Serial.print(",beats:");
            Serial.print(m.beatCount);
            Serial.println();
        }
            
        #elif AUTO_SIGNAL_TYPE == 1  // EMG @ 100 Hz efectivo
        if (timerTickCounter % PLOT_DOWNSAMPLE_EMG == 0) {
            // EMG: raw bipolar ±5mV, env 0-4mV, rms 0-3.5mV
            Serial.print(">raw:");
            Serial.print(emgModel.getRawSample(), 2);  // mV bipolar
            Serial.print(",env:");
            Serial.print(emgModel.getProcessedSample(), 2);  // mV envolvente
            Serial.print(",rms:");
            Serial.print(emgModel.getRMSAmplitude(), 3);  // mV RMS
            Serial.print(",mus:");
            Serial.print(emgModel.getActiveMotorUnits());
            Serial.print(",fr:");
            Serial.print(emgModel.getMeanFiringRate(), 1);
            Serial.print(",mvc:");
            Serial.print(emgModel.getContractionLevel(), 0);
            // Métricas de fatiga (solo relevantes en FATIGUE)
            Serial.print(",mdf:");
            Serial.print(emgModel.getFatigueMDF(), 0);
            Serial.print(",mfl:");
            Serial.print(emgModel.getFatigueMFL(), 2);
            Serial.println();
        }
            
        #else  // PPG @ 100 Hz efectivo (40:1 downsampling)
        if (timerTickCounter % PLOT_DOWNSAMPLE_PPG == 0) {
            Serial.print(">ppg:");
            Serial.print(ppgModel.getDCBaseline() + ppgModel.getLastACValue(), 0);
            Serial.print(",ac:");
            Serial.print(ppgModel.getLastACValue(), 1);
            Serial.print(",hr:");
            Serial.print(ppgModel.getMeasuredHR(), 0);
            Serial.print(",rr:");
            Serial.print(ppgModel.getMeasuredRRInterval(), 0);
            Serial.print(",pi:");
            Serial.print(ppgModel.getMeasuredPI(), 2);
            Serial.print(",sys:");
            Serial.print(ppgModel.getMeasuredSystoleTime(), 0);
            Serial.print(",dia:");
            Serial.print(ppgModel.getMeasuredDiastoleTime(), 0);
            Serial.print(",beats:");
            Serial.print(ppgModel.getBeatCount());
            Serial.println();
        }
        #endif
    }
    
    // ========== ESTADISTICAS PERIODICAS (cada 3 segundos) ==========
    if (now_ms - lastStats >= 3000) {
        lastStats = now_ms;
        
        Serial.println();
        Serial.println("============================================================");
        Serial.printf("  [%lu s] METRICAS - %lu muestras generadas\n", 
                     (now_ms - plotStartTime) / 1000, sampleCounter);
        Serial.println("------------------------------------------------------------");
        
        #if AUTO_SIGNAL_TYPE == 0  // ECG
            ECGDisplayMetrics m = ecgModel.getDisplayMetrics();
            uint8_t idx = AUTO_ECG_CONDITION;
            bool hrOK = (m.bpm >= ECG_RANGES[idx].valMin && m.bpm <= ECG_RANGES[idx].valMax);
            
            Serial.printf("  HR: %.1f BPM %s (esperado: %s)\n", 
                         m.bpm, hrOK ? "[OK]" : "[!]", ECG_RANGES[idx].description);
            Serial.printf("  RR: %.0f ms | PR: %.0f ms | QRS: %.0f ms\n",
                         m.rrInterval_ms, m.prInterval_ms, m.qrsDuration_ms);
            Serial.printf("  QT: %.0f ms | QTc: %.0f ms\n", m.qtInterval_ms, m.qtcInterval_ms);
            Serial.printf("  R: %.2f mV | ST: %.2f mV | T: %.2f mV\n",
                         m.rAmplitude_mV, m.stDeviation_mV, m.tAmplitude_mV);
            Serial.printf("  Latidos: %lu | Rango: [%.2f, %.2f] mV\n", 
                         m.beatCount, minVal, maxVal);
                         
        #elif AUTO_SIGNAL_TYPE == 1  // EMG
            float rms = emgModel.getRMSAmplitude();
            uint8_t idx = AUTO_EMG_CONDITION;
            bool rmsOK = (rms >= EMG_RANGES[idx].valMin && rms <= EMG_RANGES[idx].valMax);
            
            Serial.printf("  RMS: %.3f mV %s (esperado: %s)\n", 
                         rms, rmsOK ? "[OK]" : "[!]", EMG_RANGES[idx].description);
            Serial.printf("  MUs activas: %d/100 (Henneman)\n", emgModel.getActiveMotorUnits());
            Serial.printf("  FR media: %.1f Hz (fisiol: 6-50 Hz)\n", emgModel.getMeanFiringRate());
            Serial.printf("  Contraccion: %.0f%% MVC\n", emgModel.getContractionLevel());
            Serial.printf("  Rango senal: [%.3f, %.3f] mV\n", minVal, maxVal);
            
        #else  // PPG
            float hr = ppgModel.getMeasuredHR();
            float pi = ppgModel.getMeasuredPI();
            uint8_t idx = AUTO_PPG_CONDITION;
            bool hrOK = (hr >= PPG_RANGES[idx].valMin && hr <= PPG_RANGES[idx].valMax);
            bool piOK = (pi >= PPG_RANGES[idx].val2Min && pi <= PPG_RANGES[idx].val2Max);
            
            Serial.println("  --- METRICAS MEDIDAS (tiempo real) ---");
            Serial.printf("  HR: %.1f BPM %s\n", hr, hrOK ? "[OK]" : "[!]");
            Serial.printf("  RR: %.0f ms\n", ppgModel.getMeasuredRRInterval());
            Serial.printf("  AC: %.1f mV | PI: %.2f%% %s\n", 
                         ppgModel.getMeasuredACAmplitude(), pi, piOK ? "[OK]" : "[!]");
            Serial.printf("  Sistole: %.0f ms | Diastole: %.0f ms\n",
                         ppgModel.getMeasuredSystoleTime(), ppgModel.getMeasuredDiastoleTime());
            Serial.println("  --- MODELO ---");
            Serial.printf("  DC: %.0f mV | AC modelo: %.1f mV\n",
                         ppgModel.getDCBaseline(), ppgModel.getACAmplitude());
            Serial.printf("  Latidos: %lu | Esperado: %s\n", 
                         ppgModel.getBeatCount(), PPG_RANGES[idx].description);
        #endif
        
        Serial.println("============================================================");
        Serial.println();
    }
}
