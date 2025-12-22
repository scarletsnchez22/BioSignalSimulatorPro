/**
 * @file main.cpp
 * @brief BioSimulator Pro - Punto de entrada principal
 * @version 1.0.0
 * @date 18 Diciembre 2025
 * 
 * Sistema de generación de señales biomédicas (ECG, EMG, PPG).
 * 
 * ARQUITECTURA:
 * - Core 0: UI (Nextion + Serial)
 * - Core 1: Generación de señales (tiempo real)
 * 
 * HARDWARE:
 * - ESP32-WROOM-32 @ 240MHz
 * - Nextion NX8048T070 (800x480)
 * - DAC salida GPIO25 (0-3.3V, 8-bit)
 */

#include <Arduino.h>
#include "config.h"
#include "data/signal_types.h"
#include "core/signal_engine.h"
#include "core/state_machine.h"
#include "core/param_controller.h"
#include "comm/nextion_driver.h"
#include "comm/serial_handler.h"
#include "comm/wifi_server.h"

// ============================================================================
// INSTANCIAS GLOBALES
// ============================================================================
SignalEngine* signalEngine = nullptr;
NextionDriver* nextion = nullptr;
SerialHandler* serialHandler = nullptr;
StateMachine stateMachine;
ParamController paramController;

// ============================================================================
// VARIABLES TEMPORALES PARA SLIDERS ECG
// Almacenan valores mientras el popup está abierto, se aplican al cerrar
// ============================================================================
struct ECGSliderValues {
    int hr = 75;        // Frecuencia cardíaca (BPM)
    int zoom = 100;     // Zoom visual (50-200%), NO afecta modelo ni DAC
    int noise = 2;      // Ruido × 100 (0.02)
    int hrv = 5;        // Variabilidad HRV (%)
    bool modified = false;  // ¿Se modificó algún valor?
} ecgSliderValues;

// ============================================================================
// VARIABLES TEMPORALES PARA SLIDERS EMG
// Almacenan valores mientras el popup está abierto, se aplican al cerrar
// ============================================================================
struct EMGSliderValues {
    int exc = 0;        // Excitación (0-100%)
    int amp = 100;      // Amplitud × 100 (1.00 factor)
    int noise = 5;      // Ruido × 100 (0.05)
    bool modified = false;
} emgSliderValues;

// ============================================================================
// VARIABLES TEMPORALES PARA SLIDERS PPG
// Almacenan valores mientras el popup está abierto, se aplican al cerrar
// ============================================================================
struct PPGSliderValues {
    int hr = 75;        // Frecuencia cardíaca (BPM)
    int pi = 50;        // Índice perfusión × 10 (5.0%)
    int noise = 5;      // Ruido × 100 (0.05)
    int amp = 100;      // Factor amplitud/zoom (50-200%), solo visual
    bool modified = false;
} ppgSliderValues;

// ============================================================================
// PROTOTIPOS
// ============================================================================
void initializeHardware();
void initializeLED();
void setLEDState(SignalState state);
void handleUIEvent(UIEvent event, uint8_t param);
void handleSerialCommand(uint8_t cmd, uint8_t* data, uint16_t len);
void handleStateChange(SystemState oldState, SystemState newState);
void updateDisplay();

// ============================================================================
// LED RGB
// ============================================================================
void initializeLED() {
#if LED_RGB_ENABLED
    pinMode(LED_RGB_RED, OUTPUT);
    pinMode(LED_RGB_GREEN, OUTPUT);
    pinMode(LED_RGB_BLUE, OUTPUT);
    
    // Apagar todos
    digitalWrite(LED_RGB_RED, LED_RGB_COMMON_ANODE ? HIGH : LOW);
    digitalWrite(LED_RGB_GREEN, LED_RGB_COMMON_ANODE ? HIGH : LOW);
    digitalWrite(LED_RGB_BLUE, LED_RGB_COMMON_ANODE ? HIGH : LOW);
#endif
    pinMode(LED_STATUS, OUTPUT);
}

void setLEDState(SignalState state) {
#if LED_RGB_ENABLED
    bool r = false, g = false, b = false;
    
    switch (state) {
        case SignalState::STOPPED:
            r = true; g = false; b = false;  // Rojo
            break;
        case SignalState::RUNNING:
            r = false; g = false; b = true;  // Azul
            break;
        case SignalState::PAUSED:
            r = false; g = true; b = true;   // Cyan
            break;
        case SignalState::ERROR:
            r = true; g = false; b = false;  // Rojo
            break;
    }
    
    if (LED_RGB_COMMON_ANODE) {
        digitalWrite(LED_RGB_RED, !r);
        digitalWrite(LED_RGB_GREEN, !g);
        digitalWrite(LED_RGB_BLUE, !b);
    } else {
        digitalWrite(LED_RGB_RED, r);
        digitalWrite(LED_RGB_GREEN, g);
        digitalWrite(LED_RGB_BLUE, b);
    }
#endif
}

// ============================================================================
// CALLBACKS
// ============================================================================
void handleUIEvent(UIEvent event, uint8_t param) {
    switch (event) {
        // Portada
        case UIEvent::BUTTON_COMENZAR:
            stateMachine.processEvent(SystemEvent::GO_TO_MENU);
            nextion->goToPage(NextionPage::MENU);
            break;
        
        // Menú - Selección de señal (solo selecciona, NO navega)
        case UIEvent::BUTTON_ECG:
            stateMachine.processEvent(SystemEvent::SELECT_ECG);
            nextion->updateMenuButtons(SignalType::ECG);
            // La navegación se hace con bt_ir
            break;
            
        case UIEvent::BUTTON_EMG:
            stateMachine.processEvent(SystemEvent::SELECT_EMG);
            nextion->updateMenuButtons(SignalType::EMG);
            // La navegación se hace con bt_ir
            break;
            
        case UIEvent::BUTTON_PPG:
            stateMachine.processEvent(SystemEvent::SELECT_PPG);
            nextion->updateMenuButtons(SignalType::PPG);
            // La navegación se hace con bt_ir
            break;
        
        case UIEvent::BUTTON_IR:
            // Comportamiento depende del estado actual
            if (stateMachine.getState() == SystemState::MENU) {
                // En menú: ir a selección de condición según señal seleccionada
                stateMachine.processEvent(SystemEvent::GO_TO_CONDITION);
                switch (stateMachine.getSelectedSignal()) {
                    case SignalType::ECG:
                        nextion->goToPage(NextionPage::ECG_SIM);
                        delay(60); // dar tiempo a que la página cargue antes de pintar botones
                        nextion->updateECGConditionButtons(stateMachine.getSelectedCondition());
                        break;
                    case SignalType::EMG:
                        nextion->goToPage(NextionPage::EMG_SIM);
                        delay(60);
                        nextion->updateEMGConditionButtons(stateMachine.getSelectedCondition());
                        break;
                    case SignalType::PPG:
                        nextion->goToPage(NextionPage::PPG_SIM);
                        delay(60);
                        nextion->updatePPGConditionButtons(stateMachine.getSelectedCondition());
                        break;
                    default:
                        break;
                }
            } else if (stateMachine.getState() == SystemState::SELECT_CONDITION) {
                // Solo permitir si hay condición seleccionada
                if (stateMachine.getSelectedCondition() != 0xFF) {
                    // En ecg_sim/emg_sim/ppg_sim: solo navegar a waveform (sin iniciar señal)
                    switch (stateMachine.getSelectedSignal()) {
                        case SignalType::ECG:
                            nextion->goToPage(NextionPage::WAVEFORM_ECG);
                            break;
                        case SignalType::EMG:
                            nextion->goToPage(NextionPage::WAVEFORM_EMG);
                            break;
                        case SignalType::PPG:
                            nextion->goToPage(NextionPage::WAVEFORM_PPG);
                            break;
                        default:
                            break;
                    }
                }
            }
            break;
            
        case UIEvent::BUTTON_ATRAS:
            stateMachine.processEvent(SystemEvent::BACK);
            // Navegar a la página correspondiente según el nuevo estado
            switch (stateMachine.getState()) {
                case SystemState::PORTADA:
                    nextion->goToPage(NextionPage::PORTADA);
                    break;
                case SystemState::MENU:
                    nextion->goToPage(NextionPage::MENU);
                    // Despintar todos los botones al volver al menú
                    nextion->updateMenuButtons(SignalType::NONE);
                    break;
                default:
                    break;
            }
            break;
            
        // Selección de condición
        case UIEvent::BUTTON_CONDITION:
            stateMachine.processEvent(SystemEvent::SELECT_CONDITION, param);
            // Actualizar botones según el tipo de señal
            if (stateMachine.getSelectedSignal() == SignalType::ECG) {
                nextion->updateECGConditionButtons(param);
            } else if (stateMachine.getSelectedSignal() == SignalType::EMG) {
                nextion->updateEMGConditionButtons(param);
            } else if (stateMachine.getSelectedSignal() == SignalType::PPG) {
                nextion->updatePPGConditionButtons(param);
            }
            break;
            
        // Controles de simulación (en páginas waveform)
        case UIEvent::BUTTON_START:
            // Serial.println("[UI] PLAY presionado");
            // Si estamos en SELECT_CONDITION, cambiar a SIMULATING
            if (stateMachine.getState() == SystemState::SELECT_CONDITION && stateMachine.getSelectedCondition() != 0xFF) {
                stateMachine.processEvent(SystemEvent::GO_TO_WAVEFORM);
            } else {
                stateMachine.processEvent(SystemEvent::START_SIMULATION);
            }
            // Habilitar streaming WiFi
            wifiServer.setStreamingEnabled(true);
            break;
            
        case UIEvent::BUTTON_PAUSE:
            // Serial.println("[UI] PAUSE presionado");
            if (signalEngine->getState() == SignalState::RUNNING) {
                stateMachine.processEvent(SystemEvent::PAUSE);
            } else {
                stateMachine.processEvent(SystemEvent::RESUME);
            }
            break;
            
        case UIEvent::BUTTON_STOP:
            // Serial.println("[UI] STOP presionado");
            stateMachine.processEvent(SystemEvent::STOP);
            // Deshabilitar streaming WiFi
            wifiServer.setStreamingEnabled(false);
            // Volver a la página de selección de condición
            switch (stateMachine.getSelectedSignal()) {
                case SignalType::ECG:
                    nextion->goToPage(NextionPage::ECG_SIM);
                    delay(60);
                    nextion->updateECGConditionButtons(stateMachine.getSelectedCondition());
                    break;
                case SignalType::EMG:
                    nextion->goToPage(NextionPage::EMG_SIM);
                    delay(60);
                    nextion->updateEMGConditionButtons(stateMachine.getSelectedCondition());
                    break;
                case SignalType::PPG:
                    nextion->goToPage(NextionPage::PPG_SIM);
                    delay(60);
                    nextion->updatePPGConditionButtons(stateMachine.getSelectedCondition());
                    break;
                default:
                    break;
            }
            break;
        
        // Popups (páginas waveform)
        // NOTA: BUTTON_VALORES eliminado - valores ahora integrados en waveform
            
        case UIEvent::BUTTON_PARAMETROS:
            // Ir a página parametros y configurar sliders
            if (stateMachine.getSelectedSignal() == SignalType::ECG) {
                nextion->goToPage(NextionPage::PARAMETROS_ECG);
                ECGModel& ecg = signalEngine->getECGModel();
                ecgSliderValues.hr = (int)ecg.getHRMean();
                // zoom ya tiene su valor actual (no se obtiene del modelo)
                ecgSliderValues.noise = (int)(ecg.getNoiseLevel() * 100);
                ecgSliderValues.hrv = (int)(ecg.getHRStd() / ecg.getHRMean() * 100);
                ecgSliderValues.modified = false;
                
                float hrMin, hrMax;
                ecg.getHRRange(hrMin, hrMax);
                nextion->setupECGParametersPage(
                    (int)hrMin, (int)hrMax,
                    ecgSliderValues.hr,
                    ecgSliderValues.zoom,  // Zoom visual (50-200%)
                    ecgSliderValues.noise,
                    ecgSliderValues.hrv
                );
            } else if (stateMachine.getSelectedSignal() == SignalType::EMG) {
                nextion->goToPage(NextionPage::PARAMETROS_EMG);
                EMGModel& emg = signalEngine->getEMGModel();
                emgSliderValues.exc = (int)(emg.getCurrentExcitation() * 100);  // 0.5 → 50
                emgSliderValues.amp = (int)(emg.getAmplitude() * 100);          // 1.5 → 150
                emgSliderValues.noise = (int)(emg.getNoiseLevel() * 100);
                emgSliderValues.modified = false;
                
                nextion->setupEMGParametersPage(
                    emgSliderValues.exc,
                    emgSliderValues.amp,
                    emgSliderValues.noise
                );
            } else if (stateMachine.getSelectedSignal() == SignalType::PPG) {
                nextion->goToPage(NextionPage::PARAMETROS_PPG);
                PPGModel& ppg = signalEngine->getPPGModel();
                ppgSliderValues.hr = (int)ppg.getCurrentHeartRate();
                ppgSliderValues.pi = (int)(ppg.getPerfusionIndex() * 10);  // 5.2% → 52
                ppgSliderValues.noise = (int)(ppg.getNoiseLevel() * 100);
                // amp ya tiene su valor actual (solo visual, no del modelo)
                ppgSliderValues.modified = false;
                
                nextion->setupPPGParametersPage(
                    ppgSliderValues.hr,
                    ppgSliderValues.pi,
                    ppgSliderValues.noise,
                    ppgSliderValues.amp
                );
            }
            break;
        
        // NOTA: BUTTON_BACK_POPUP eliminado - no hay popups de valores separados
        
        case UIEvent::BUTTON_APPLY_PARAMS:
            // bt_act: Aplicar cambios de sliders y cerrar popup
            if (stateMachine.getSelectedSignal() == SignalType::ECG) {
                if (ecgSliderValues.modified) {
                    ECGModel& ecg = signalEngine->getECGModel();
                    ECGParameters params;
                    params.condition = ecg.getCondition();
                    params.heartRate = (float)ecgSliderValues.hr;
                    // NOTA: zoom NO afecta el modelo, solo la visualización
                    params.noiseLevel = ecgSliderValues.noise / 100.0f;
                    ecg.setParameters(params);
                    
                    // Actualizar escala mV/div en waveform_ecg
                    nextion->updateECGScale(ecgSliderValues.zoom);
                    Serial.printf("[UI] Parámetros ECG aplicados, Zoom: %d%%\n", ecgSliderValues.zoom);
                }
                ecgSliderValues.modified = false;
                nextion->goToPage(NextionPage::WAVEFORM_ECG);
            } else if (stateMachine.getSelectedSignal() == SignalType::EMG) {
                if (emgSliderValues.modified) {
                    EMGModel& emg = signalEngine->getEMGModel();
                    EMGParameters params;
                    params.condition = emg.getCondition();
                    params.excitationLevel = emgSliderValues.exc / 100.0f;  // 50 → 0.5
                    params.amplitude = emgSliderValues.amp / 100.0f;        // 150 → 1.5
                    params.noiseLevel = emgSliderValues.noise / 100.0f;     // 5 → 0.05
                    emg.setParameters(params);
                    Serial.println("[UI] Parámetros EMG aplicados");
                }
                emgSliderValues.modified = false;
                nextion->goToPage(NextionPage::WAVEFORM_EMG);
            } else if (stateMachine.getSelectedSignal() == SignalType::PPG) {
                if (ppgSliderValues.modified) {
                    PPGModel& ppg = signalEngine->getPPGModel();
                    PPGParameters params;
                    params.condition = ppg.getCondition();
                    params.heartRate = (float)ppgSliderValues.hr;
                    params.perfusionIndex = ppgSliderValues.pi / 10.0f;  // 52 → 5.2
                    params.noiseLevel = ppgSliderValues.noise / 100.0f;  // 5 → 0.05
                    params.dicroticNotch = 0.4f;  // Mantener valor por defecto
                    ppg.setParameters(params);
                    Serial.println("[UI] Parámetros PPG aplicados");
                }
                ppgSliderValues.modified = false;
                nextion->goToPage(NextionPage::WAVEFORM_PPG);
            }
            break;
        
        case UIEvent::BUTTON_CANCEL_PARAMS:
            // bt_ex: Cancelar sin guardar - descartar cambios
            if (stateMachine.getSelectedSignal() == SignalType::ECG) {
                ecgSliderValues.modified = false;
                Serial.println("[UI] Cambios ECG descartados");
                nextion->goToPage(NextionPage::WAVEFORM_ECG);
            } else if (stateMachine.getSelectedSignal() == SignalType::EMG) {
                emgSliderValues.modified = false;
                Serial.println("[UI] Cambios EMG descartados");
                nextion->goToPage(NextionPage::WAVEFORM_EMG);
            } else if (stateMachine.getSelectedSignal() == SignalType::PPG) {
                ppgSliderValues.modified = false;
                Serial.println("[UI] Cambios PPG descartados");
                nextion->goToPage(NextionPage::WAVEFORM_PPG);
            }
            break;
        
        case UIEvent::BUTTON_RESET_PARAMS:
            // Resetear modelo a valores por defecto de la patología
            if (stateMachine.getSelectedSignal() == SignalType::ECG) {
                ECGModel& ecg = signalEngine->getECGModel();
                ECGCondition currentCond = ecg.getCondition();
                ecg.reset();
                ECGParameters params;
                params.condition = currentCond;
                ecg.setParameters(params);
                
                ecgSliderValues.hr = (int)ecg.getHRMean();
                ecgSliderValues.zoom = 100;  // Reset zoom a 100%
                ecgSliderValues.noise = (int)(ecg.getNoiseLevel() * 100);
                ecgSliderValues.hrv = (int)(ecg.getHRStd() / ecg.getHRMean() * 100);
                ecgSliderValues.modified = false;
                
                float hrMin, hrMax;
                ecg.getHRRange(hrMin, hrMax);
                nextion->setupECGParametersPage(
                    (int)hrMin, (int)hrMax,
                    ecgSliderValues.hr,
                    ecgSliderValues.zoom,
                    ecgSliderValues.noise,
                    ecgSliderValues.hrv
                );
                nextion->updateECGScale(ecgSliderValues.zoom);
                Serial.println("[UI] Parámetros ECG reseteados, Zoom: 100%");
            } else if (stateMachine.getSelectedSignal() == SignalType::EMG) {
                EMGModel& emg = signalEngine->getEMGModel();
                EMGCondition currentCond = emg.getCondition();
                emg.reset();
                EMGParameters params;
                params.condition = currentCond;
                emg.setParameters(params);
                
                emgSliderValues.exc = (int)(emg.getCurrentExcitation() * 100);
                emgSliderValues.amp = (int)(emg.getAmplitude() * 100);
                emgSliderValues.noise = (int)(emg.getNoiseLevel() * 100);
                emgSliderValues.modified = false;
                
                nextion->setupEMGParametersPage(
                    emgSliderValues.exc,
                    emgSliderValues.amp,
                    emgSliderValues.noise
                );
                Serial.println("[UI] Parámetros EMG reseteados");
            } else if (stateMachine.getSelectedSignal() == SignalType::PPG) {
                PPGModel& ppg = signalEngine->getPPGModel();
                PPGCondition currentCond = ppg.getCondition();
                ppg.reset();
                PPGParameters params;
                params.condition = currentCond;
                ppg.setParameters(params);
                
                ppgSliderValues.hr = (int)ppg.getCurrentHeartRate();
                ppgSliderValues.pi = (int)(ppg.getPerfusionIndex() * 10);
                ppgSliderValues.noise = (int)(ppg.getNoiseLevel() * 100);
                ppgSliderValues.amp = 100;  // Reset amp a 100%
                ppgSliderValues.modified = false;
                
                nextion->setupPPGParametersPage(
                    ppgSliderValues.hr,
                    ppgSliderValues.pi,
                    ppgSliderValues.noise,
                    ppgSliderValues.amp
                );
                Serial.println("[UI] Parámetros PPG reseteados, Amp: 100%");
            }
            break;
        
        // Sliders ECG - Solo guardan en variables temporales, NO aplican al modelo
        case UIEvent::SLIDER_ECG_HR:
            {
                int hrValue = nextion->readSliderValue("h_hr");
                if (hrValue > 0) {
                    ecgSliderValues.hr = hrValue;
                    ecgSliderValues.modified = true;
                    Serial.printf("[UI] Slider HR: %d BPM (pendiente aplicar)\n", hrValue);
                }
            }
            break;
            
        case UIEvent::SLIDER_ECG_AMP:
            {
                int zoomValue = nextion->readSliderValue("h_amp");
                if (zoomValue >= 50 && zoomValue <= 200) {
                    ecgSliderValues.zoom = zoomValue;
                    ecgSliderValues.modified = true;
                    // Actualizar etiqueta de escala en tiempo real
                    nextion->updateECGScale(zoomValue);
                    Serial.printf("[UI] Slider Zoom: %d%% (pendiente aplicar)\n", zoomValue);
                }
            }
            break;
            
        case UIEvent::SLIDER_ECG_NOISE:
            {
                int noiseValue = nextion->readSliderValue("h_noise");
                if (noiseValue >= 0) {
                    ecgSliderValues.noise = noiseValue;
                    ecgSliderValues.modified = true;
                    Serial.printf("[UI] Slider Ruido: %d (pendiente aplicar)\n", noiseValue);
                }
            }
            break;
            
        case UIEvent::SLIDER_ECG_HRV:
            {
                int hrvValue = nextion->readSliderValue("h_hrv");
                if (hrvValue >= 0) {
                    ecgSliderValues.hrv = hrvValue;
                    ecgSliderValues.modified = true;
                    Serial.printf("[UI] Slider HRV: %d%% (pendiente aplicar)\n", hrvValue);
                }
            }
            break;
        
        // Sliders EMG - Solo guardan en variables temporales, NO aplican al modelo
        case UIEvent::SLIDER_EMG_EXC:
            {
                int excValue = nextion->readSliderValue("h_exc");
                if (excValue >= 0) {
                    emgSliderValues.exc = excValue;
                    emgSliderValues.modified = true;
                    Serial.printf("[UI] Slider Excitación: %d%% (pendiente aplicar)\n", excValue);
                }
            }
            break;
            
        case UIEvent::SLIDER_EMG_AMP:
            {
                int ampValue = nextion->readSliderValue("h_amp");
                if (ampValue > 0) {
                    emgSliderValues.amp = ampValue;
                    emgSliderValues.modified = true;
                    Serial.printf("[UI] Slider Amplitud: %d (pendiente aplicar)\n", ampValue);
                }
            }
            break;
            
        case UIEvent::SLIDER_EMG_NOISE:
            {
                int noiseValue = nextion->readSliderValue("h_noise");
                if (noiseValue >= 0) {
                    emgSliderValues.noise = noiseValue;
                    emgSliderValues.modified = true;
                    Serial.printf("[UI] Slider Ruido: %d (pendiente aplicar)\n", noiseValue);
                }
            }
            break;
        
        // Sliders PPG - Solo guardan en variables temporales, NO aplican al modelo
        case UIEvent::SLIDER_PPG_HR:
            {
                int hrValue = nextion->readSliderValue("h_hr");
                if (hrValue > 0) {
                    ppgSliderValues.hr = hrValue;
                    ppgSliderValues.modified = true;
                    Serial.printf("[UI] Slider HR PPG: %d BPM (pendiente aplicar)\n", hrValue);
                }
            }
            break;
            
        case UIEvent::SLIDER_PPG_PI:
            {
                int piValue = nextion->readSliderValue("h_pi");
                if (piValue > 0) {
                    ppgSliderValues.pi = piValue;
                    ppgSliderValues.modified = true;
                    Serial.printf("[UI] Slider PI: %d (%.1f%%, pendiente aplicar)\n", piValue, piValue/10.0f);
                }
            }
            break;
            
        case UIEvent::SLIDER_PPG_NOISE:
            {
                int noiseValue = nextion->readSliderValue("h_noise");
                if (noiseValue >= 0) {
                    ppgSliderValues.noise = noiseValue;
                    ppgSliderValues.modified = true;
                    Serial.printf("[UI] Slider Ruido PPG: %d (pendiente aplicar)\n", noiseValue);
                }
            }
            break;
            
        case UIEvent::SLIDER_PPG_AMP:
            {
                int ampValue = nextion->readSliderValue("h_amp");
                if (ampValue >= 50 && ampValue <= 200) {
                    ppgSliderValues.amp = ampValue;
                    ppgSliderValues.modified = true;
                    Serial.printf("[UI] Slider Amplitud PPG: %d%% (pendiente aplicar)\n", ampValue);
                }
            }
            break;
            
        default:
            break;
    }
}

void handleSerialCommand(uint8_t cmd, uint8_t* data, uint16_t len) {
    // TODO: Implementar protocolo binario
    Serial.printf("[Serial] Comando: 0x%02X, Len: %d\n", cmd, len);
}

void handleStateChange(SystemState oldState, SystemState newState) {
    // Serial.printf("[State] %s -> %s\n", 
    //               stateMachine.stateToString(oldState),
    //               stateMachine.stateToString(newState));
    
    // Verificar que signalEngine está inicializado
    if (!signalEngine) {
        // Serial.println("[State] SignalEngine no inicializado, omitiendo acciones");
        return;
    }
    
    switch (newState) {
        case SystemState::PORTADA:
            // No navegamos aquí, la navegación se maneja en handleUIEvent
            signalEngine->stopSignal();
            setLEDState(SignalState::STOPPED);
            break;
            
        case SystemState::MENU:
            // No navegamos aquí, la navegación se maneja en handleUIEvent
            signalEngine->stopSignal();
            setLEDState(SignalState::STOPPED);
            break;
            
        case SystemState::SELECT_CONDITION:
            // No navegamos aquí, la navegación se maneja en handleUIEvent
            setLEDState(SignalState::STOPPED);
            break;
            
        case SystemState::SIMULATING:
            // No navegamos aquí, la navegación se maneja en handleUIEvent
            // Solo iniciamos la generación de señal
            // Serial.printf("[State] Iniciando señal: Tipo=%d, Condición=%d\n",
            //              (int)stateMachine.getSelectedSignal(),
            //              stateMachine.getSelectedCondition());
            signalEngine->startSignal(
                stateMachine.getSelectedSignal(),
                stateMachine.getSelectedCondition()
            );
            setLEDState(SignalState::RUNNING);
            // Serial.printf("[State] Estado SignalEngine: %d (0=STOPPED, 1=RUNNING)\n", 
            //              (int)signalEngine->getState());
            break;
            
        case SystemState::PAUSED:
            signalEngine->pauseSignal();
            setLEDState(SignalState::PAUSED);
            break;
            
        default:
            break;
    }
}

// ============================================================================
// ACTUALIZACIÓN DE DISPLAY
// ============================================================================
void updateDisplay() {
    static unsigned long lastUpdate = 0;
    static uint32_t lastSampleCount = 0;
    
    unsigned long now = millis();
    
    // =========================================================================
    // WAVEFORM: Arquitectura unificada con signal_engine (contador de ticks)
    // Downsampling respecto a Fs_timer (4kHz) usando NEXTION_DOWNSAMPLE_*
    // ECG: 4000/200 = 20:1 → 200 Hz efectivo
    // EMG: 4000/100 = 40:1 → 100 Hz efectivo
    // PPG: 4000/100 = 40:1 → 100 Hz efectivo
    // =========================================================================
    if (signalEngine->getState() == SignalState::RUNNING) {
        uint32_t currentSampleCount = signalEngine->getSignalData().sampleCount;
        SignalType type = signalEngine->getCurrentType();
        
        // Determinar ratio de downsampling según tipo de señal
        uint8_t downsampleRatio;
        switch (type) {
            case SignalType::ECG: downsampleRatio = NEXTION_DOWNSAMPLE_ECG; break;
            case SignalType::EMG: downsampleRatio = NEXTION_DOWNSAMPLE_EMG; break;
            case SignalType::PPG: downsampleRatio = NEXTION_DOWNSAMPLE_PPG; break;
            default: downsampleRatio = 20; break;
        }
        
        // Enviar punto solo cuando corresponde según el ratio
        if (currentSampleCount != lastSampleCount && 
            currentSampleCount % downsampleRatio == 0) {
            
            uint8_t dacValue;
            
            // Para PPG usamos getWaveformValue() que escala solo la AC al rango 0-255
            // Para ECG/EMG usamos getLastDACValue() que ya tiene el escalado correcto
            if (type == SignalType::PPG) {
                dacValue = signalEngine->getPPGModel().getWaveformValue();
            } else {
                dacValue = signalEngine->getLastDACValue();
            }
            
            // Aplicar zoom visual (solo afecta display, no DAC)
            float zoomFactor = 1.0f;
            if (type == SignalType::ECG) {
                zoomFactor = ecgSliderValues.zoom / 100.0f;
            } else if (type == SignalType::EMG) {
                zoomFactor = emgSliderValues.amp / 100.0f;
            } else if (type == SignalType::PPG) {
                zoomFactor = ppgSliderValues.amp / 100.0f;
            }
            
            // Zoom visual: expandir/comprimir respecto al centro (127)
            int centered = dacValue - 127;
            int zoomed = (int)(centered * zoomFactor) + 127;
            zoomed = constrain(zoomed, 0, 255);
            int waveValue = map(zoomed, 0, 255, 10, NEXTION_WAVEFORM_HEIGHT - 10);
            
            // Enviar al waveform (ID=1, Canal=0)
            nextion->addWaveformPoint(WAVEFORM_COMPONENT_ID, WAVEFORM_CHANNEL, (uint8_t)waveValue);
        }
        lastSampleCount = currentSampleCount;
    }
    
    // Actualizar métricas y valores en pantalla a 4 Hz
    if (now - lastUpdate >= METRICS_UPDATE_MS) {
        if (signalEngine->getState() == SignalState::RUNNING) {
            SignalType type = signalEngine->getCurrentType();
            
            // Actualizar valores integrados en waveforms
            switch (type) {
                case SignalType::ECG: {
                    ECGModel& ecg = signalEngine->getECGModel();
                    nextion->updateECGValuesPage(
                        (int)ecg.getCurrentBPM(),
                        (int)ecg.getCurrentRR_ms(),
                        (int)(ecg.getRWaveAmplitude_mV() * 100),
                        (int)(ecg.getSTDeviation_mV() * 100),
                        ecg.getBeatCount(),
                        ecg.getConditionName()
                    );
                    break;
                }
                case SignalType::EMG: {
                    EMGModel& emg = signalEngine->getEMGModel();
                    nextion->updateEMGValuesPage(
                        (int)(emg.getRMSAmplitude() * 100),      // 0.25mV → 25
                        emg.getActiveMotorUnits(),               // Entero
                        (int)(emg.getMeanFiringRate() * 10),     // 12.5Hz → 125
                        (int)emg.getContractionLevel(),          // % entero
                        emg.getConditionName()
                    );
                    break;
                }
                case SignalType::PPG: {
                    PPGModel& ppg = signalEngine->getPPGModel();
                    nextion->updatePPGValuesPage(
                        (int)ppg.getCurrentHeartRate(),          // BPM
                        (int)ppg.getCurrentRRInterval(),         // RR en ms
                        (int)(ppg.getPerfusionIndex() * 10),     // PI × 10 (5.2% → 52)
                        ppg.getBeatCount(),
                        ppg.getConditionName()
                    );
                    break;
                }
                default:
                    break;
            }
        }
        lastUpdate = now;
    }
}

// ============================================================================
// SETUP
// ============================================================================
void setup() {
    // Iniciar Serial para debug
    Serial.begin(115200);
    delay(100);
    
    Serial.println("\n╔═══════════════════════════════════════════════╗");
    Serial.println("║     " DEVICE_NAME " v" FIRMWARE_VERSION "     ║");
    Serial.println("╚═══════════════════════════════════════════════╝");
    Serial.printf("Hardware: %s\n", HARDWARE_MODEL);
    Serial.printf("Free Heap: %d KB\n", ESP.getFreeHeap() / 1024);
    
    // Inicializar LED
    initializeLED();
    setLEDState(SignalState::STOPPED);
    
    // Inicializar Nextion
    nextion = new NextionDriver(NEXTION_SERIAL);
    if (!nextion->begin()) {
        Serial.println("[ERROR] No se pudo inicializar Nextion");
    }
    nextion->setEventCallback(handleUIEvent);
    
    // Inicializar SerialHandler
    serialHandler = new SerialHandler(Serial);
    serialHandler->setCommandCallback(handleSerialCommand);
    
    // Inicializar motor de señales
    signalEngine = SignalEngine::getInstance();
    if (!signalEngine->begin()) {
        Serial.println("[ERROR] No se pudo inicializar SignalEngine");
        setLEDState(SignalState::ERROR);
        return;
    }
    
    // Configurar máquina de estados
    stateMachine.setStateChangeCallback(handleStateChange);
    stateMachine.processEvent(SystemEvent::INIT_COMPLETE);
    
    // Navegar a página inicial
    nextion->goToPage(NextionPage::PORTADA);
    
    // Inicializar WiFi Server
    Serial.println("\n[WiFi] Iniciando servidor web...");
    if (wifiServer.begin()) {
        Serial.println("[WiFi] Servidor iniciado correctamente");
        Serial.println("[WiFi] SSID: BioSimulator_Pro");
        Serial.println("[WiFi] Pass: biosignal123");
        Serial.println("[WiFi] URL: http://192.168.4.1");
    } else {
        Serial.println("[WiFi] ERROR: No se pudo iniciar servidor");
    }
    
    // LED verde: listo
    digitalWrite(LED_RGB_RED, LED_RGB_COMMON_ANODE ? HIGH : LOW);
    digitalWrite(LED_RGB_GREEN, LED_RGB_COMMON_ANODE ? LOW : HIGH);
    digitalWrite(LED_RGB_BLUE, LED_RGB_COMMON_ANODE ? HIGH : LOW);
    
    Serial.println("\n[OK] Sistema inicializado correctamente");
    Serial.println("Presione 'h' para ayuda\n");
}

// ============================================================================
// LOOP
// ============================================================================
void loop() {
    // Procesar eventos de Nextion
    nextion->process();
    
    // Procesar comandos serial
    serialHandler->process();
    
    // Actualizar display
    updateDisplay();
    
    // Procesar WiFi Server
    wifiServer.loop();
    
    // Enviar datos a clientes WebSocket si hay señal activa
    if (wifiServer.getClientCount() > 0 && stateMachine.getState() == SystemState::SIMULATING) {
        WSSignalData wsData;
        WSSignalMetrics wsMetrics;
        memset(&wsData, 0, sizeof(wsData));
        memset(&wsMetrics, 0, sizeof(wsMetrics));
        
        SignalType type = signalEngine->getCurrentType();
        
        switch (type) {
            case SignalType::ECG: {
                ECGModel& ecg = signalEngine->getECGModel();
                wsData.signalType = "ECG";
                wsData.condition = ecg.getConditionName();
                wsData.state = "RUNNING";
                wsData.value = ecg.getCurrentValueMV();
                wsData.dacValue = signalEngine->getLastDACValue();
                wsData.timestamp = millis();
                
                wsMetrics.hr = (int)ecg.getCurrentHeartRate();
                wsMetrics.rr = (int)ecg.getCurrentRRInterval();
                wsMetrics.qrs = ecg.getQRSAmplitude();
                wsMetrics.st = ecg.getSTDeviation_mV();
                break;
            }
            case SignalType::EMG: {
                EMGModel& emg = signalEngine->getEMGModel();
                wsData.signalType = "EMG";
                wsData.condition = emg.getConditionName();
                wsData.state = "RUNNING";
                wsData.value = emg.getCurrentValueMV();
                wsData.dacValue = signalEngine->getLastDACValue();
                wsData.timestamp = millis();
                
                wsMetrics.rms = emg.getRMSAmplitude();
                wsMetrics.excitation = (int)(emg.getExcitation() * 100);
                wsMetrics.activeUnits = emg.getActiveMotorUnits();
                break;
            }
            case SignalType::PPG: {
                PPGModel& ppg = signalEngine->getPPGModel();
                wsData.signalType = "PPG";
                wsData.condition = ppg.getConditionName();
                wsData.state = "RUNNING";
                wsData.value = ppg.getLastACValue();
                wsData.dacValue = signalEngine->getLastDACValue();
                wsData.timestamp = millis();
                
                wsMetrics.hr = (int)ppg.getCurrentHeartRate();
                wsMetrics.rr = (int)ppg.getCurrentRRInterval();
                wsMetrics.pi = ppg.getPerfusionIndex();
                break;
            }
            default:
                break;
        }
        
        wifiServer.sendSignalData(wsData);
        wifiServer.sendMetrics(wsMetrics);
    }
    
    // Pequeño delay para no saturar
    delay(1);
}
