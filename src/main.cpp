/**
 * @file main.cpp
 * @brief BioSignal Simulator Pro - Punto de entrada principal
 * @version 2.0.0
 * @date 2025
 * 
 * @details
 * Sistema de generación de señales biomédicas (ECG, EMG, PPG) con interfaz
 * táctil Nextion y salida DAC para visualización en osciloscopio.
 * 
 * ARQUITECTURA:
 * - Core 0: UI (Nextion + Serial), monitoreo del sistema
 * - Core 1: Generación de señales (tarea precálculo + ISR timer)
 * 
 * HARDWARE:
 * - ESP32-WROOM-32 @ 240MHz, 320KB SRAM, 4MB Flash
 * - Nextion NX4024T032 (320x240) en Serial2 (GPIO16/17)
 * - DAC salida en GPIO25 (0-3.3V, 8-bit)
 * - LED status en GPIO2
 * 
 * @see docs/README_TECHNICAL.md para detalles de implementación
 * @see docs/README_HARDWARE.md para esquemas de conexión
 */

#include <Arduino.h>
#include "config.h"
#include "signal_types.h"
#include "signal_generator.h"
#include "nextion_ui.h"
#include "param_limits.h"

// ============================================================================
// VARIABLES GLOBALES
// ============================================================================
SignalGenerator* signalGen = nullptr;

// Estado del sistema
struct SystemState {
    SignalType selectedSignal;
    uint8_t selectedCondition;
    bool isSimulating;
    unsigned long lastMetricsUpdate;
} sysState = {
    .selectedSignal = SignalType::NONE,
    .selectedCondition = 0,
    .isSimulating = false,
    .lastMetricsUpdate = 0
};

// Intervalo de actualización de métricas (ms)
const uint16_t METRICS_UPDATE_INTERVAL = 250;  // 4 Hz

// ============================================================================
// PROTOTIPOS
// ============================================================================
void printHelp();
void printSystemInfo();
void processSerialCommand();
void handleUIEvent(UIEvent event, uint8_t param);
void startSimulation(SignalType type, uint8_t condition);
void stopSimulation();
void updateMetricsDisplay();
DisplayMetrics collectMetrics();

// ============================================================================
// LED RGB - Control de indicadores de estado
// ============================================================================
// Estados del LED RGB:
//   ROJO   = Sistema apagado/Error
//   VERDE  = Sistema encendido y listo
//   AZUL   = Simulación en progreso
//   CYAN   = Simulación pausada
// ============================================================================

enum class SystemLedState {
    OFF,        // Rojo - Sistema apagado
    READY,      // Verde - Listo para usar
    SIMULATING, // Azul parpadeante - Generando señal
    PAUSED,     // Cyan - Señal pausada
    ERROR       // Rojo parpadeante - Error
};

void initLedRGB() {
    #if LED_RGB_ENABLED
    pinMode(LED_RGB_RED, OUTPUT);
    pinMode(LED_RGB_GREEN, OUTPUT);
    pinMode(LED_RGB_BLUE, OUTPUT);
    // Apagar todos al inicio
    digitalWrite(LED_RGB_RED, LED_RGB_COMMON_ANODE ? HIGH : LOW);
    digitalWrite(LED_RGB_GREEN, LED_RGB_COMMON_ANODE ? HIGH : LOW);
    digitalWrite(LED_RGB_BLUE, LED_RGB_COMMON_ANODE ? HIGH : LOW);
    #endif
}

void setLedRGB(bool red, bool green, bool blue) {
    #if LED_RGB_ENABLED
    // Invertir lógica si es ánodo común
    if (LED_RGB_COMMON_ANODE) {
        digitalWrite(LED_RGB_RED, !red);
        digitalWrite(LED_RGB_GREEN, !green);
        digitalWrite(LED_RGB_BLUE, !blue);
    } else {
        digitalWrite(LED_RGB_RED, red);
        digitalWrite(LED_RGB_GREEN, green);
        digitalWrite(LED_RGB_BLUE, blue);
    }
    #endif
}

void setSystemLedState(SystemLedState state) {
    switch (state) {
        case SystemLedState::OFF:
            setLedRGB(true, false, false);   // Rojo
            break;
        case SystemLedState::READY:
            setLedRGB(false, true, false);   // Verde
            break;
        case SystemLedState::SIMULATING:
            setLedRGB(false, false, true);   // Azul
            break;
        case SystemLedState::PAUSED:
            setLedRGB(false, true, true);    // Cyan (Verde + Azul)
            break;
        case SystemLedState::ERROR:
            setLedRGB(true, false, false);   // Rojo
            break;
    }
}

// ============================================================================
// AYUDA E INFORMACIÓN
// ============================================================================
void printHelp() {
    Serial.println("\n╔═══════════════════════════════════════════════════════════╗");
    Serial.println("║        " DEVICE_NAME " v" FIRMWARE_VERSION "        ║");
    Serial.println("╠═══════════════════════════════════════════════════════════╣");
    Serial.println("║  SEÑALES:                                                 ║");
    Serial.println("║    e - Menú ECG    m - Menú EMG    g - Menú PPG          ║");
    Serial.println("║                                                           ║");
    Serial.println("║  ECG (1-9):        EMG (1-0):         PPG (1-7):         ║");
    Serial.println("║    1-Normal        1-Reposo           1-Normal           ║");
    Serial.println("║    2-Taquicardia   2-Leve 20%         2-Arritmia         ║");
    Serial.println("║    3-Bradicardia   3-Moderada 50%     3-Perfusion Baja   ║");
    Serial.println("║    4-Fib.Auric.    4-Fuerte 80%       4-Perfusion Alta   ║");
    Serial.println("║    5-Fib.Ventr.    5-Maxima 100%      5-Vasoconstriccion ║");
    Serial.println("║    6-PVC           6-Temblor          6-Artefacto Mov.   ║");
    Serial.println("║    7-Bloq.Rama     7-Miopatia         7-SpO2 Bajo        ║");
    Serial.println("║    8-ST Elev.      8-Neuropatia                          ║");
    Serial.println("║    9-ST Depr.      9-Fasciculacion                       ║");
    Serial.println("║                    0-Fatiga                              ║");
    Serial.println("║                                                           ║");
    Serial.println("║  CONTROL:                                                 ║");
    Serial.println("║    p - Pausar      r - Reanudar      s - Detener         ║");
    Serial.println("║    i - Info        h - Ayuda                             ║");
    Serial.println("╠═══════════════════════════════════════════════════════════╣");
    Serial.println("║  PARAMETROS EN TIEMPO REAL (mientras señal activa):      ║");
    Serial.println("║    b<val> - BPM (30-200)        Ej: b85  (ECG/PPG)       ║");
    Serial.println("║    a<val> - Amplitud (0.1-2.0)  Ej: a1.5 (todas)         ║");
    Serial.println("║    n<val> - Ruido (0.0-1.0)     Ej: n0.1 (todas)         ║");
    Serial.println("║    t<val> - ST shift (-2 a +2)  Ej: t0.5 (ECG)           ║");
    Serial.println("║    w<val> - Onda P (0.0-2.0)    Ej: w0.5 (ECG)           ║");
    Serial.println("║    f<val> - Frecuencia (10-150) Ej: f80  (EMG)           ║");
    Serial.println("║    d<val> - Dicrótica (0.0-1.0) Ej: d0.5 (PPG)           ║");
    Serial.println("╚═══════════════════════════════════════════════════════════╝\n");
}

void printSystemInfo() {
    Serial.println("\n--- Información del Sistema ---");
    Serial.printf("Firmware: %s\n", FIRMWARE_VERSION);
    Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("CPU Freq: %d MHz\n", ESP.getCpuFreqMHz());
    
    SignalData data = signalGen->getCurrentSignalData();
    Serial.printf("\nSeñal actual: %s\n", signalTypeToString(data.type));
    Serial.printf("Estado: %s\n", signalStateToString(data.state));
    Serial.printf("Muestras generadas: %lu\n", data.sampleCount);
    
    if (data.type == SignalType::ECG) {
        Serial.printf("\nParámetros ECG:\n");
        Serial.printf("  Frecuencia: %.1f BPM\n", data.params.ecg.heartRate);
        Serial.printf("  Condición: %s\n", ecgConditionToString(data.params.ecg.condition));
        Serial.printf("  Amplitud QRS: %.2f\n", data.params.ecg.qrsAmplitude);
        Serial.printf("  Ruido: %.2f\n", data.params.ecg.noiseLevel);
    }
    Serial.println("-------------------------------\n");
}

// ============================================================================
// FUNCIONES DE INICIO DE SEÑALES
// ============================================================================

// Modo de selección actual (para comandos numéricos)
enum class SelectionMode { ECG, EMG, PPG } currentMode = SelectionMode::ECG;

void startSignalWithCondition(SignalType type, uint8_t conditionIndex) {
    // Detener señal actual si existe
    if (signalGen->getState() == SignalState::RUNNING) {
        signalGen->stopSignal();
        delay(50);
    }
    
    // Iniciar señal según tipo
    if (!signalGen->startSignal(type)) {
        Serial.println("✗ Error al iniciar señal");
        return;
    }
    
    // Configurar condición según tipo
    switch (type) {
        case SignalType::ECG: {
            ECGParameters params;
            params.condition = (ECGCondition)conditionIndex;
            // Ajustes HR según condición
            switch ((ECGCondition)conditionIndex) {
                case ECGCondition::NORMAL: params.heartRate = 75.0f; break;
                case ECGCondition::TACHYCARDIA: params.heartRate = 130.0f; break;
                case ECGCondition::BRADYCARDIA: params.heartRate = 45.0f; break;
                case ECGCondition::ATRIAL_FIBRILLATION: 
                    params.heartRate = 110.0f; 
                    params.pWaveAmplitude = 0.1f; 
                    break;
                default: break;
            }
            signalGen->updateECGParameters(params);
            Serial.printf("✓ ECG %s iniciado\n", ecgConditionToString(params.condition));
            break;
        }
        
        case SignalType::EMG: {
            EMGParameters params;
            params.condition = (EMGCondition)conditionIndex;
            signalGen->updateEMGParameters(params);
            Serial.printf("✓ EMG %s iniciado\n", emgConditionToString(params.condition));
            break;
        }
        
        case SignalType::PPG: {
            PPGParameters params;
            params.condition = (PPGCondition)conditionIndex;
            signalGen->updatePPGParameters(params);
            Serial.printf("✓ PPG %s iniciado\n", ppgConditionToString(params.condition));
            break;
        }
        
        default:
            break;
    }
    
    sysState.selectedSignal = type;
    sysState.selectedCondition = conditionIndex;
    sysState.isSimulating = true;
    
    Serial.println("Salida en GPIO25 - Use osciloscopio para visualizar");
}

// ============================================================================
// PROCESAMIENTO DE COMANDOS SERIAL
// ============================================================================
void processSerialCommand() {
    if (!Serial.available()) return;
    
    char cmd = Serial.read();
    
    switch (cmd) {
        // === Selección de modo ===
        case 'e': case 'E':
            currentMode = SelectionMode::ECG;
            Serial.println("\n[MODO ECG] Presione 1-9 para condición:");
            Serial.println("  1-Normal 2-Taquicardia 3-Bradicardia 4-Fib.Auric.");
            Serial.println("  5-Fib.Ventr. 6-PVC 7-Bloq.Rama 8-ST+ 9-ST-");
            break;
            
        case 'm': case 'M':
            currentMode = SelectionMode::EMG;
            Serial.println("\n[MODO EMG] Presione 1-0 para condición:");
            Serial.println("  1-Reposo 2-Leve 3-Moderada 4-Fuerte 5-Maxima");
            Serial.println("  6-Temblor 7-Miopatia 8-Neuropatia 9-Fasciculacion 0-Fatiga");
            break;
            
        case 'g': case 'G':
            currentMode = SelectionMode::PPG;
            Serial.println("\n[MODO PPG] Presione 1-7 para condición:");
            Serial.println("  1-Normal 2-Arritmia 3-Perfusion Baja 4-Perfusion Alta");
            Serial.println("  5-Vasoconstriccion 6-Artefacto Mov. 7-SpO2 Bajo");
            break;
        
        // === Selección numérica de condición ===
        case '1': case '2': case '3': case '4': case '5':
        case '6': case '7': case '8': case '9': case '0': {
            uint8_t idx = (cmd == '0') ? 9 : (cmd - '1');
            switch (currentMode) {
                case SelectionMode::ECG:
                    if (idx < 9) startSignalWithCondition(SignalType::ECG, idx);
                    break;
                case SelectionMode::EMG:
                    if (idx < 10) startSignalWithCondition(SignalType::EMG, idx);
                    break;
                case SelectionMode::PPG:
                    if (idx < 7) startSignalWithCondition(SignalType::PPG, idx);
                    break;
            }
            break;
        }
        
        // === Control ===
        case 'p': case 'P':
            if (signalGen->pauseSignal()) {
                Serial.println("⏸ Señal pausada");
                sysState.isSimulating = false;
            }
            break;
            
        case 'r': case 'R':
            if (signalGen->resumeSignal()) {
                Serial.println("▶ Señal reanudada");
                sysState.isSimulating = true;
            }
            break;
            
        case 's': case 'S':
            if (signalGen->stopSignal()) {
                Serial.println("⏹ Señal detenida");
                sysState.isSimulating = false;
            }
            break;
            
        // === Información ===
        case 'i': case 'I':
            printSystemInfo();
            break;
            
        case 'h': case 'H':
            printHelp();
            break;
            
        // === Parámetros con límites según condición ===
        case 'b': case 'B': {  // HR
            float bpm = Serial.parseFloat();
            SignalData data = signalGen->getCurrentSignalData();
            if (data.type == SignalType::ECG) {
                ECGLimits limits = getECGLimits(data.params.ecg.condition);
                if (isInRange(bpm, limits.heartRate)) {
                    ECGParameters params = data.params.ecg;
                    params.heartRate = bpm;
                    signalGen->updateECGParameters(params);
                    Serial.printf("✓ HR: %.0f BPM\n", bpm);
                } else {
                    Serial.printf("✗ HR rango %.0f-%.0f para %s\n", 
                        limits.heartRate.minValue, limits.heartRate.maxValue,
                        ecgConditionToString(data.params.ecg.condition));
                }
            } else if (data.type == SignalType::PPG) {
                PPGLimits limits = getPPGLimits(data.params.ppg.condition);
                if (isInRange(bpm, limits.heartRate)) {
                    PPGParameters params = data.params.ppg;
                    params.heartRate = bpm;
                    signalGen->updatePPGParameters(params);
                    Serial.printf("✓ HR: %.0f BPM\n", bpm);
                } else {
                    Serial.printf("✗ HR rango %.0f-%.0f\n", 
                        limits.heartRate.minValue, limits.heartRate.maxValue);
                }
            }
            break;
        }
        
        case 'a': case 'A': {  // Amplitud
            float amp = Serial.parseFloat();
            SignalData data = signalGen->getCurrentSignalData();
            if (data.type == SignalType::ECG) {
                ECGLimits limits = getECGLimits(data.params.ecg.condition);
                ECGParameters params = data.params.ecg;
                params.qrsAmplitude = clampToLimits(amp, limits.qrsAmplitude);
                signalGen->updateECGParameters(params);
                Serial.printf("✓ Amp: %.2f\n", params.qrsAmplitude);
            } else if (data.type == SignalType::EMG) {
                EMGLimits limits = getEMGLimits(data.params.emg.condition);
                EMGParameters params = data.params.emg;
                params.amplitude = clampToLimits(amp, limits.amplitude);
                signalGen->updateEMGParameters(params);
                Serial.printf("✓ Amp: %.2f\n", params.amplitude);
            } else if (data.type == SignalType::PPG) {
                PPGLimits limits = getPPGLimits(data.params.ppg.condition);
                PPGParameters params = data.params.ppg;
                params.amplitude = clampToLimits(amp, limits.amplitude);
                signalGen->updatePPGParameters(params);
                Serial.printf("✓ Amp: %.2f\n", params.amplitude);
            }
            break;
        }
        
        case 'n': case 'N': {  // Ruido
            float n = Serial.parseFloat();
            SignalData data = signalGen->getCurrentSignalData();
            if (data.type == SignalType::ECG) {
                ECGLimits lim = getECGLimits(data.params.ecg.condition);
                ECGParameters p = data.params.ecg;
                p.noiseLevel = clampToLimits(n, lim.noiseLevel);
                signalGen->updateECGParameters(p);
            } else if (data.type == SignalType::EMG) {
                EMGLimits lim = getEMGLimits(data.params.emg.condition);
                EMGParameters p = data.params.emg;
                p.noiseLevel = clampToLimits(n, lim.noiseLevel);
                signalGen->updateEMGParameters(p);
            } else if (data.type == SignalType::PPG) {
                PPGLimits lim = getPPGLimits(data.params.ppg.condition);
                PPGParameters p = data.params.ppg;
                p.noiseLevel = clampToLimits(n, lim.noiseLevel);
                signalGen->updatePPGParameters(p);
            }
            Serial.printf("✓ Ruido: %.2f\n", n);
            break;
        }
        
        case 't': case 'T': {  // ST (ECG)
            float st = Serial.parseFloat();
            SignalData data = signalGen->getCurrentSignalData();
            if (data.type == SignalType::ECG) {
                ECGLimits lim = getECGLimits(data.params.ecg.condition);
                if (isInRange(st, lim.stShift)) {
                    ECGParameters p = data.params.ecg;
                    p.stSegmentShift = st;
                    signalGen->updateECGParameters(p);
                    Serial.printf("✓ ST: %+.2f mV\n", st);
                } else {
                    Serial.printf("✗ ST rango %+.1f a %+.1f\n", lim.stShift.minValue, lim.stShift.maxValue);
                }
            }
            break;
        }
        
        case 'f': case 'F': {  // Frecuencia (EMG)
            float f = Serial.parseFloat();
            SignalData data = signalGen->getCurrentSignalData();
            if (data.type == SignalType::EMG) {
                EMGLimits lim = getEMGLimits(data.params.emg.condition);
                if (isInRange(f, lim.frequency)) {
                    EMGParameters p = data.params.emg;
                    p.frequency = f;
                    signalGen->updateEMGParameters(p);
                    Serial.printf("✓ Freq: %.0f Hz\n", f);
                } else {
                    Serial.printf("✗ Freq rango %.0f-%.0f Hz\n", lim.frequency.minValue, lim.frequency.maxValue);
                }
            }
            break;
        }
        
        case 'd': case 'D': {  // Dicrótica (PPG)
            float d = Serial.parseFloat();
            SignalData data = signalGen->getCurrentSignalData();
            if (data.type == SignalType::PPG) {
                PPGLimits lim = getPPGLimits(data.params.ppg.condition);
                PPGParameters p = data.params.ppg;
                p.dicroticNotch = clampToLimits(d, lim.dicroticNotch);
                signalGen->updatePPGParameters(p);
                Serial.printf("✓ Dicrótica: %.2f\n", p.dicroticNotch);
            }
            break;
        }
        
        case 'w': case 'W': {  // Onda P (ECG)
            float w = Serial.parseFloat();
            SignalData data = signalGen->getCurrentSignalData();
            if (data.type == SignalType::ECG) {
                ECGLimits lim = getECGLimits(data.params.ecg.condition);
                ECGParameters p = data.params.ecg;
                p.pWaveAmplitude = clampToLimits(w, lim.pWaveAmplitude);
                signalGen->updateECGParameters(p);
                Serial.printf(" Onda P: %.2f\n", p.pWaveAmplitude);
            }
            break;
        }
        
        case '\n': case '\r':
            break;
            
        default:
            if (cmd >= 32) {
                Serial.println("Comando no reconocido. 'h' para ayuda.");
            }
            break;
    }
    
    // Limpiar buffer
    while (Serial.available()) Serial.read();
}

// ============================================================================
// CALLBACK DE EVENTOS NEXTION
// ============================================================================
void handleNextionEvent(UIEvent event, uint8_t param) {
    switch (event) {
        case UIEvent::BTN_BEGIN:
            nextion.goToPage(NextionPage::MENU_SIGNAL);
            break;
        case UIEvent::SELECT_ECG:
            nextion.goToPage(NextionPage::MENU_ECG);
            currentMode = SelectionMode::ECG;
            break;
        case UIEvent::SELECT_EMG:
            nextion.goToPage(NextionPage::MENU_EMG);
            currentMode = SelectionMode::EMG;
            break;
        case UIEvent::SELECT_PPG:
            nextion.goToPage(NextionPage::MENU_PPG);
            currentMode = SelectionMode::PPG;
            break;
        case UIEvent::SELECT_CONDITION:
            switch (currentMode) {
                case SelectionMode::ECG:
                    startSignalWithCondition(SignalType::ECG, param);
                    break;
                case SelectionMode::EMG:
                    startSignalWithCondition(SignalType::EMG, param);
                    break;
                case SelectionMode::PPG:
                    startSignalWithCondition(SignalType::PPG, param);
                    break;
            }
            nextion.goToPage(NextionPage::SIMULATION);
            break;
        case UIEvent::BTN_PLAY:
            if (signalGen->getState() == SignalState::STOPPED) {
                startSignalWithCondition(sysState.selectedSignal, sysState.selectedCondition);
            } else {
                signalGen->resumeSignal();
            }
            break;
        case UIEvent::BTN_PAUSE:
            signalGen->pauseSignal();
            break;
        case UIEvent::BTN_STOP:
            signalGen->stopSignal();
            break;
        case UIEvent::BTN_BACK:
            nextion.goToPage(NextionPage::MENU_SIGNAL);
            signalGen->stopSignal();
            break;
        case UIEvent::BTN_PARAMS:
            nextion.goToPage(NextionPage::PARAMS);
            break;
        case UIEvent::PARAM_HR_UP:
        case UIEvent::PARAM_HR_DOWN: {
            SignalData data = signalGen->getCurrentSignalData();
            float delta = (event == UIEvent::PARAM_HR_UP) ? 5.0f : -5.0f;
            if (data.type == SignalType::ECG) {
                ECGLimits lim = getECGLimits(data.params.ecg.condition);
                ECGParameters p = data.params.ecg;
                p.heartRate = clampToLimits(p.heartRate + delta, lim.heartRate);
                signalGen->updateECGParameters(p);
            } else if (data.type == SignalType::PPG) {
                PPGLimits lim = getPPGLimits(data.params.ppg.condition);
                PPGParameters p = data.params.ppg;
                p.heartRate = clampToLimits(p.heartRate + delta, lim.heartRate);
                signalGen->updatePPGParameters(p);
            }
            break;
        }
        case UIEvent::PARAM_AMP_UP:
        case UIEvent::PARAM_AMP_DOWN: {
            float delta = (event == UIEvent::PARAM_AMP_UP) ? 0.1f : -0.1f;
            SignalData data = signalGen->getCurrentSignalData();
            if (data.type == SignalType::ECG) {
                ECGParameters p = data.params.ecg;
                p.qrsAmplitude = constrain(p.qrsAmplitude + delta, 0.5f, 2.0f);
                signalGen->updateECGParameters(p);
            } else if (data.type == SignalType::EMG) {
                EMGParameters p = data.params.emg;
                p.amplitude = constrain(p.amplitude + delta, 0.0f, 1.0f);
                signalGen->updateEMGParameters(p);
            } else if (data.type == SignalType::PPG) {
                PPGParameters p = data.params.ppg;
                p.amplitude = constrain(p.amplitude + delta, 0.5f, 2.0f);
                signalGen->updatePPGParameters(p);
            }
            break;
        }
        case UIEvent::PARAM_NOISE_UP:
        case UIEvent::PARAM_NOISE_DOWN: {
            float delta = (event == UIEvent::PARAM_NOISE_UP) ? 0.02f : -0.02f;
            SignalData data = signalGen->getCurrentSignalData();
            if (data.type == SignalType::ECG) {
                ECGParameters p = data.params.ecg;
                p.noiseLevel = constrain(p.noiseLevel + delta, 0.0f, 0.5f);
                signalGen->updateECGParameters(p);
            } else if (data.type == SignalType::EMG) {
                EMGParameters p = data.params.emg;
                p.noiseLevel = constrain(p.noiseLevel + delta, 0.0f, 0.5f);
                signalGen->updateEMGParameters(p);
            } else if (data.type == SignalType::PPG) {
                PPGParameters p = data.params.ppg;
                p.noiseLevel = constrain(p.noiseLevel + delta, 0.0f, 0.5f);
                signalGen->updatePPGParameters(p);
            }
            break;
        }
        case UIEvent::PARAM_APPLY:
            nextion.goToPage(NextionPage::SIMULATION);
            break;
        default:
            break;
    }
}

// ============================================================================
// RECOLECCIÓN DE MÉTRICAS PARA NEXTION
// ============================================================================
DisplayMetrics collectDisplayMetrics() {
    DisplayMetrics metrics = {};
    SignalData data = signalGen->getCurrentSignalData();
    
    metrics.signalType = data.type;
    metrics.signalState = data.state;
    
    switch (data.type) {
        case SignalType::ECG:
            metrics.heartRate = data.params.ecg.heartRate;
            metrics.beatCount = data.sampleCount / (SAMPLE_RATE_UNIFIED * 60 / data.params.ecg.heartRate);
            metrics.rrInterval = 60000.0f / data.params.ecg.heartRate;
            break;
            
        case SignalType::EMG:
            metrics.rmsValue = data.params.emg.amplitude;
            metrics.activeMUs = (uint32_t)(data.params.emg.amplitude * 100);
            metrics.avgFiringRate = data.params.emg.frequency;
            break;
            
        case SignalType::PPG:
            metrics.heartRate = data.params.ppg.heartRate;
            // SpO2 y PI se calculan según condición
            metrics.spo2 = (data.params.ppg.condition == PPGCondition::LOW_SPO2) ? 88.0f : 98.0f;
            metrics.perfusionIndex = data.params.ppg.amplitude * 2.0f;
            break;
            
        default:
            break;
    }
    
    return metrics;
}

// ============================================================================
// SETUP
// ============================================================================
void setup() {
    // Inicializar Serial para debug
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n\n");
    Serial.println("╔════════════════════════════════════════╗");
    Serial.println("║   BioSignal Simulator Pro - ESP32     ║");
    Serial.println("║   Iniciando sistema...                 ║");
    Serial.println("╚════════════════════════════════════════╝");
    
    // LED de status (interno)
    pinMode(LED_STATUS, OUTPUT);
    digitalWrite(LED_STATUS, LOW);
    
    // LED RGB externo
    initLedRGB();
    setSystemLedState(SystemLedState::OFF);  // Rojo mientras inicializa
    
    // Obtener instancia del generador
    signalGen = SignalGenerator::getInstance();
    
    // Inicializar generador de señales
    if (!signalGen->begin()) {
        Serial.println("\n✗ ERROR: No se pudo inicializar el generador");
        while(1) { delay(1000); }
    }
    Serial.println("✓ Generador de señales inicializado");
    Serial.printf("✓ DAC salida en GPIO%d\n", DAC_SIGNAL_PIN);
    Serial.printf("✓ Frecuencia muestreo: %d Hz\n", SAMPLE_RATE_UNIFIED);
    
    // Inicializar Nextion
    Serial.println("\nInicializando pantalla Nextion...");
    Serial.printf("  - Serial2 TX: GPIO%d, RX: GPIO%d\n", NEXTION_TX_PIN, NEXTION_RX_PIN);
    Serial.printf("  - Baud rate: %d\n", NEXTION_BAUD);
    
    if (nextion.begin()) {
        Serial.println("✓ Nextion NX4024T032 inicializado");
        nextion.setEventCallback(handleNextionEvent);
        nextion.goToPage(NextionPage::SPLASH);
    } else {
        Serial.println("⚠ Nextion no respondió (continuando sin UI)");
    }
    
    Serial.printf("\n✓ Memoria libre: %d bytes\n", ESP.getFreeHeap());
    digitalWrite(LED_STATUS, HIGH);
    
    // LED RGB: Verde = Sistema listo
    setSystemLedState(SystemLedState::READY);
    
    // Mostrar ayuda Serial
    printHelp();
}

// ============================================================================
// LOOP PRINCIPAL
// ============================================================================
void loop() {
    static unsigned long lastWaveformUpdate = 0;
    static unsigned long lastMetricsUpdate = 0;
    static unsigned long lastBlink = 0;
    
    // 1. Procesar comandos Serial (debug/control alternativo)
    processSerialCommand();
    
    // 2. Procesar eventos Nextion (botones, sliders)
    nextion.processEvents();
    
    // 3. Actualizar waveform en Nextion (cada 2ms = 500 puntos/seg)
    if (millis() - lastWaveformUpdate >= 2) {
        lastWaveformUpdate = millis();
        
        if (signalGen->getState() == SignalState::RUNNING) {
            // Obtener último valor DAC y enviarlo a Nextion
            uint8_t dacValue = signalGen->getLastDACValue();
            nextion.updateWaveform(dacValue);
        }
    }
    
    // 4. Actualizar métricas en Nextion (cada 250ms)
    if (millis() - lastMetricsUpdate >= 250) {
        lastMetricsUpdate = millis();
        
        if (signalGen->getState() == SignalState::RUNNING) {
            DisplayMetrics metrics = collectDisplayMetrics();
            nextion.updateMetrics(metrics);
        }
    }
    
    // 5. LEDs de estado (interno + RGB)
    SignalState currentState = signalGen->getState();
    static SignalState lastState = SignalState::STOPPED;
    
    // Actualizar LED RGB solo cuando cambia el estado
    if (currentState != lastState) {
        switch (currentState) {
            case SignalState::RUNNING:
                setSystemLedState(SystemLedState::SIMULATING);  // Azul
                break;
            case SignalState::PAUSED:
                setSystemLedState(SystemLedState::PAUSED);      // Cyan
                break;
            case SignalState::STOPPED:
                setSystemLedState(SystemLedState::READY);       // Verde
                break;
        }
        lastState = currentState;
    }
    
    // LED interno parpadea cuando simula
    if (currentState == SignalState::RUNNING) {
        if (millis() - lastBlink > 500) {
            digitalWrite(LED_STATUS, !digitalRead(LED_STATUS));
            lastBlink = millis();
        }
    } else {
        digitalWrite(LED_STATUS, HIGH);
    }
    
    // 6. Yield para tareas del sistema
    delay(1);
}