/**
 * @file main.cpp
 * @brief BioSimulator Pro - Punto de entrada principal
 * @version 1.0.0
 * 
 * Sistema de generación de señales biomédicas (ECG, EMG, PPG).
 * 
 * ARQUITECTURA:
 * - Core 0: UI (Nextion + Serial)
 * - Core 1: Generación de señales (tiempo real)
 * 
 * HARDWARE:
 * - ESP32-WROOM-32 @ 240MHz
 * - Nextion NX4024T032 (320x240)
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

// ============================================================================
// INSTANCIAS GLOBALES
// ============================================================================
SignalEngine* signalEngine = nullptr;
NextionDriver* nextion = nullptr;
SerialHandler* serialHandler = nullptr;
StateMachine stateMachine;
ParamController paramController;

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
        case UIEvent::BUTTON_ECG:
            stateMachine.processEvent(SystemEvent::SELECT_ECG);
            break;
            
        case UIEvent::BUTTON_EMG:
            stateMachine.processEvent(SystemEvent::SELECT_EMG);
            break;
            
        case UIEvent::BUTTON_PPG:
            stateMachine.processEvent(SystemEvent::SELECT_PPG);
            break;
            
        case UIEvent::BUTTON_CONDITION:
            stateMachine.processEvent(SystemEvent::SELECT_CONDITION, param);
            break;
            
        case UIEvent::BUTTON_START:
            stateMachine.processEvent(SystemEvent::START_SIMULATION);
            break;
            
        case UIEvent::BUTTON_PAUSE:
            if (signalEngine->getState() == SignalState::RUNNING) {
                stateMachine.processEvent(SystemEvent::PAUSE);
            } else {
                stateMachine.processEvent(SystemEvent::RESUME);
            }
            break;
            
        case UIEvent::BUTTON_STOP:
            stateMachine.processEvent(SystemEvent::STOP);
            break;
            
        case UIEvent::BUTTON_PARAMS:
            stateMachine.processEvent(SystemEvent::OPEN_PARAMS);
            break;
            
        case UIEvent::BUTTON_APPLY:
            stateMachine.processEvent(SystemEvent::APPLY_PARAMS);
            break;
            
        case UIEvent::BUTTON_CANCEL:
            stateMachine.processEvent(SystemEvent::CANCEL_PARAMS);
            break;
            
        case UIEvent::BUTTON_BACK:
            stateMachine.processEvent(SystemEvent::BACK);
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
    Serial.printf("[State] %s -> %s\n", 
                  stateMachine.stateToString(oldState),
                  stateMachine.stateToString(newState));
    
    switch (newState) {
        case SystemState::IDLE:
            nextion->goToPage(NextionPage::SELECT_SIGNAL);
            setLEDState(SignalState::STOPPED);
            break;
            
        case SystemState::SIGNAL_SELECTED:
            nextion->setupConditionPage(stateMachine.getSelectedSignal());
            nextion->goToPage(NextionPage::SELECT_CONDITION);
            break;
            
        case SystemState::SIMULATING:
            nextion->goToPage(NextionPage::SIMULATION);
            signalEngine->startSignal(
                stateMachine.getSelectedSignal(),
                stateMachine.getSelectedCondition()
            );
            setLEDState(SignalState::RUNNING);
            break;
            
        case SystemState::PAUSED:
            signalEngine->pauseSignal();
            setLEDState(SignalState::PAUSED);
            break;
            
        case SystemState::PARAMETERS:
            nextion->goToPage(NextionPage::PARAMETERS);
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
    static unsigned long lastWaveform = 0;
    
    unsigned long now = millis();
    
    // Actualizar waveform a 100 Hz
    if (now - lastWaveform >= WAVEFORM_UPDATE_MS) {
        if (signalEngine->getState() == SignalState::RUNNING) {
            uint8_t dacValue = signalEngine->getLastDACValue();
            // Escalar de 0-255 a 0-100 (altura del waveform)
            uint8_t waveValue = map(dacValue, 0, 255, 0, 100);
            nextion->addWaveformPoint(1, 0, waveValue);
        }
        lastWaveform = now;
    }
    
    // Actualizar métricas a 4 Hz
    if (now - lastUpdate >= METRICS_UPDATE_MS) {
        if (signalEngine->getState() == SignalState::RUNNING) {
            DisplayMetrics metrics;
            SignalType type = signalEngine->getCurrentType();
            
            switch (type) {
                case SignalType::ECG: {
                    ECGModel& ecg = signalEngine->getECGModel();
                    metrics.heartRate = ecg.getCurrentHeartRate();
                    metrics.rrInterval = ecg.getCurrentRRInterval();
                    break;
                }
                case SignalType::EMG: {
                    EMGModel& emg = signalEngine->getEMGModel();
                    metrics.excitationLevel = emg.getCurrentExcitation();
                    metrics.activeMotorUnits = emg.getActiveMotorUnits();
                    break;
                }
                case SignalType::PPG: {
                    PPGModel& ppg = signalEngine->getPPGModel();
                    metrics.heartRate = ppg.getCurrentHeartRate();
                    metrics.perfusionIndex = ppg.getPerfusionIndex();
                    break;
                }
                default:
                    break;
            }
            
            nextion->updateMetrics(metrics, type);
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
    
    // Pequeño delay para no saturar
    delay(1);
}
