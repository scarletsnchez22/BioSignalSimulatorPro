/**
 * @file nextion_driver.cpp
 * @brief Implementación del driver Nextion
 * @version 1.0.0
 */

#include "comm/nextion_driver.h"

// ============================================================================
// CONSTRUCTOR
// ============================================================================
NextionDriver::NextionDriver(HardwareSerial& serialPort) : serial(serialPort) {
    eventCallback = nullptr;
    rxIndex = 0;
    currentPage = NextionPage::SPLASH;
    displayedSignal = SignalType::NONE;
}

// ============================================================================
// INICIALIZACIÓN
// ============================================================================
bool NextionDriver::begin() {
    serial.begin(NEXTION_BAUD, SERIAL_8N1, NEXTION_RX_PIN, NEXTION_TX_PIN);
    delay(100);
    
    // Reset Nextion
    sendCommand("rest");
    delay(500);
    
    // Ir a página splash
    goToPage(NextionPage::SPLASH);
    
    return true;
}

// ============================================================================
// COMANDOS BÁSICOS
// ============================================================================
void NextionDriver::sendCommand(const char* cmd) {
    serial.print(cmd);
    sendEndSequence();
}

void NextionDriver::sendEndSequence() {
    serial.write(0xFF);
    serial.write(0xFF);
    serial.write(0xFF);
}

// ============================================================================
// PROCESAR EVENTOS
// ============================================================================
void NextionDriver::process() {
    while (serial.available()) {
        uint8_t byte = serial.read();
        
        if (rxIndex < sizeof(rxBuffer)) {
            rxBuffer[rxIndex++] = byte;
        }
        
        // Verificar fin de mensaje (3 x 0xFF)
        if (rxIndex >= 3 && 
            rxBuffer[rxIndex-1] == 0xFF &&
            rxBuffer[rxIndex-2] == 0xFF &&
            rxBuffer[rxIndex-3] == 0xFF) {
            parseEvent();
            rxIndex = 0;
        }
    }
}

void NextionDriver::parseEvent() {
    if (rxIndex < 4) return;
    
    // Evento de touch: 0x65 [page] [component] [event] 0xFF 0xFF 0xFF
    if (rxBuffer[0] == 0x65) {
        uint8_t page = rxBuffer[1];
        uint8_t component = rxBuffer[2];
        uint8_t touchEvent = rxBuffer[3];
        
        if (touchEvent != 1) return;  // Solo eventos de touch
        
        UIEvent uiEvent = UIEvent::NONE;
        uint8_t param = 0;
        
        switch (page) {
            case 0:  // SPLASH
                uiEvent = UIEvent::BUTTON_START;
                break;
                
            case 1:  // SELECT_SIGNAL
                switch (component) {
                    case 1: uiEvent = UIEvent::BUTTON_ECG; break;
                    case 2: uiEvent = UIEvent::BUTTON_EMG; break;
                    case 3: uiEvent = UIEvent::BUTTON_PPG; break;
                }
                break;
                
            case 2:  // SELECT_CONDITION
                if (component >= 1 && component <= 10) {
                    uiEvent = UIEvent::BUTTON_CONDITION;
                    param = component - 1;
                } else if (component == 20) {
                    uiEvent = UIEvent::BUTTON_BACK;
                }
                break;
                
            case 3:  // SIMULATION
                switch (component) {
                    case 1: uiEvent = UIEvent::BUTTON_PAUSE; break;
                    case 2: uiEvent = UIEvent::BUTTON_STOP; break;
                    case 3: uiEvent = UIEvent::BUTTON_PARAMS; break;
                    case 4: uiEvent = UIEvent::BUTTON_BACK; break;
                }
                break;
                
            case 4:  // PARAMETERS
                switch (component) {
                    case 10: uiEvent = UIEvent::BUTTON_APPLY; break;
                    case 11: uiEvent = UIEvent::BUTTON_CANCEL; break;
                    case 12: uiEvent = UIEvent::BUTTON_DEFAULTS; break;
                }
                break;
        }
        
        if (eventCallback != nullptr && uiEvent != UIEvent::NONE) {
            eventCallback(uiEvent, param);
        }
    }
}

// ============================================================================
// NAVEGACIÓN
// ============================================================================
void NextionDriver::goToPage(NextionPage page) {
    char cmd[16];
    sprintf(cmd, "page %d", (int)page);
    sendCommand(cmd);
    currentPage = page;
}

// ============================================================================
// ACTUALIZAR COMPONENTES
// ============================================================================
void NextionDriver::setText(const char* component, const char* text) {
    char cmd[64];
    sprintf(cmd, "%s.txt=\"%s\"", component, text);
    sendCommand(cmd);
}

void NextionDriver::setNumber(const char* component, int value) {
    char cmd[32];
    sprintf(cmd, "%s.val=%d", component, value);
    sendCommand(cmd);
}

void NextionDriver::setFloat(const char* component, float value, uint8_t decimals) {
    char cmd[32];
    char format[8];
    sprintf(format, "%%.%df", decimals);
    char valStr[16];
    sprintf(valStr, format, value);
    sprintf(cmd, "%s.txt=\"%s\"", component, valStr);
    sendCommand(cmd);
}

// ============================================================================
// WAVEFORM
// ============================================================================
void NextionDriver::addWaveformPoint(uint8_t componentId, uint8_t channel, uint8_t value) {
    char cmd[20];
    sprintf(cmd, "add %d,%d,%d", componentId, channel, value);
    sendCommand(cmd);
}

void NextionDriver::clearWaveform(uint8_t componentId, uint8_t channel) {
    char cmd[16];
    sprintf(cmd, "cle %d,%d", componentId, channel);
    sendCommand(cmd);
}

// ============================================================================
// SLIDERS
// ============================================================================
void NextionDriver::setSliderValue(const char* component, int value) {
    setNumber(component, value);
}

void NextionDriver::setSliderLimits(const char* component, int minVal, int maxVal) {
    char cmd[32];
    sprintf(cmd, "%s.minval=%d", component, minVal);
    sendCommand(cmd);
    sprintf(cmd, "%s.maxval=%d", component, maxVal);
    sendCommand(cmd);
}

int NextionDriver::getSliderValue(const char* component) {
    // TODO: Implementar lectura de slider
    return 0;
}

// ============================================================================
// VISIBILIDAD
// ============================================================================
void NextionDriver::setVisible(const char* component, bool visible) {
    char cmd[32];
    sprintf(cmd, "vis %s,%d", component, visible ? 1 : 0);
    sendCommand(cmd);
}

// ============================================================================
// CONFIGURAR PÁGINA DE CONDICIONES
// ============================================================================
void NextionDriver::setupConditionPage(SignalType signalType) {
    displayedSignal = signalType;
    
    // Configurar título
    switch (signalType) {
        case SignalType::ECG:
            setText("t0", "ECG - Condicion:");
            // Mostrar botones ECG (9 condiciones)
            for (int i = 0; i < 9; i++) setVisible(("b" + String(i)).c_str(), true);
            for (int i = 9; i < 10; i++) setVisible(("b" + String(i)).c_str(), false);
            break;
            
        case SignalType::EMG:
            setText("t0", "EMG - Condicion:");
            // Mostrar botones EMG (10 condiciones)
            for (int i = 0; i < 10; i++) setVisible(("b" + String(i)).c_str(), true);
            break;
            
        case SignalType::PPG:
            setText("t0", "PPG - Condicion:");
            // Mostrar botones PPG (7 condiciones)
            for (int i = 0; i < 7; i++) setVisible(("b" + String(i)).c_str(), true);
            for (int i = 7; i < 10; i++) setVisible(("b" + String(i)).c_str(), false);
            break;
            
        default:
            break;
    }
}

// ============================================================================
// ACTUALIZAR MÉTRICAS
// ============================================================================
void NextionDriver::updateMetrics(const DisplayMetrics& metrics, SignalType type) {
    switch (type) {
        case SignalType::ECG:
            setFloat("tHR", metrics.heartRate, 0);
            setFloat("tRR", metrics.rrInterval, 0);
            break;
            
        case SignalType::EMG:
            setFloat("tExc", metrics.excitationLevel * 100.0f, 0);
            setNumber("tMU", metrics.activeMotorUnits);
            break;
            
        case SignalType::PPG:
            setFloat("tHR", metrics.heartRate, 0);
            setFloat("tPI", metrics.perfusionIndex, 1);
            break;
            
        default:
            break;
    }
}

// ============================================================================
// ESTADO DE SIMULACIÓN
// ============================================================================
void NextionDriver::setSimulationState(SignalState state) {
    switch (state) {
        case SignalState::RUNNING:
            setText("tStatus", "CORRIENDO");
            break;
        case SignalState::PAUSED:
            setText("tStatus", "PAUSADO");
            break;
        case SignalState::STOPPED:
            setText("tStatus", "DETENIDO");
            break;
        case SignalState::ERROR:
            setText("tStatus", "ERROR");
            break;
    }
}

// ============================================================================
// CALLBACK
// ============================================================================
void NextionDriver::setEventCallback(UIEventCallback callback) {
    eventCallback = callback;
}
