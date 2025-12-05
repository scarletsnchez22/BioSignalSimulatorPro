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
    currentPage = NextionPage::PORTADA;
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
    
    // Ir a página portada
    goToPage(NextionPage::PORTADA);
    
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
        
        if (touchEvent != 1) return;  // Solo eventos de release (1)
        
        UIEvent uiEvent = UIEvent::NONE;
        uint8_t param = 0;
        
        switch (page) {
            case 0:  // PORTADA
                if (component == 1) {  // bt_comenzar
                    uiEvent = UIEvent::BUTTON_COMENZAR;
                }
                break;
                
            case 1:  // MENU
                switch (component) {
                    case 1: uiEvent = UIEvent::BUTTON_ECG; break;   // bt_ecg
                    case 2: uiEvent = UIEvent::BUTTON_EMG; break;   // bt_emg
                    case 3: uiEvent = UIEvent::BUTTON_PPG; break;   // bt_ppg
                    case 4: uiEvent = UIEvent::BUTTON_ATRAS; break; // bt_atras
                    case 5: uiEvent = UIEvent::BUTTON_IR; break;    // bt_ir
                }
                break;
                
            case 2:  // ECG_SIM
                // Estructura de ecg_sim:
                // ID 0: bt_atras (hotspot) → volver a menu
                // ID 1: bt_ir (hotspot) → ir a ecg_wave (si sel_ecg >= 0)
                // ID 3: sel_ecg (number) - variable, no genera evento
                // ID 4-12: botones de condición (dual-state)
                //   4=Normal(0), 5=Taquicardia(1), 6=Bradicardia(2),
                //   7=FA(3), 8=FV(4), 9=PVC(5), 10=BRB(6), 11=STup(7), 12=STdn(8)
                switch (component) {
                    case 0: 
                        uiEvent = UIEvent::BUTTON_ATRAS; 
                        break;
                    case 1: 
                        uiEvent = UIEvent::BUTTON_IR; 
                        break;
                    case 4: case 5: case 6: case 7: case 8: 
                    case 9: case 10: case 11: case 12:
                        uiEvent = UIEvent::BUTTON_CONDITION;
                        param = component - 4;  // ID 4 → condición 0, ID 12 → condición 8
                        break;
                }
                break;
                
            case 3:  // EMG_SIM
                // Estructura de emg_sim:
                // ID 1-10: botones de condición (dual-state)
                //   1=Reposo(0), 2=Leve(1), 3=Moderada(2), 4=Fuerte(3), 5=Máxima(4),
                //   6=Temblor(5), 7=Miopatía(6), 8=Neuropatía(7), 9=Fasciculación(8), 10=Fatiga(9)
                // ID 11: sel_emg (number) - variable, no genera evento
                // ID 12: bt_atras (hotspot) → volver a menu
                // ID 13: bt_ir (hotspot) → ir a emg_wave (si sel_emg >= 0)
                switch (component) {
                    case 12: 
                        uiEvent = UIEvent::BUTTON_ATRAS; 
                        break;
                    case 13: 
                        uiEvent = UIEvent::BUTTON_IR; 
                        break;
                    case 1: case 2: case 3: case 4: case 5:
                    case 6: case 7: case 8: case 9: case 10:
                        uiEvent = UIEvent::BUTTON_CONDITION;
                        param = component - 1;  // ID 1 → condición 0, ID 10 → condición 9
                        break;
                }
                break;
                
            case 4:  // PPG_SIM
                // Estructura de ppg_sim:
                // ID 1: bt_norm (dual-state) → sel_ppg=0 (Normal)
                // ID 2: sel_ppg (number) - variable, no genera evento
                // ID 3: bt_arr (dual-state) → sel_ppg=1 (Arritmia)
                // ID 4: bt_spo2 (dual-state) → sel_ppg=2 (SPO2 Bajo)
                // ID 5: bt_lowp (dual-state) → sel_ppg=3 (Perfusión Débil)
                // ID 6: bt_highp (dual-state) → sel_ppg=4 (Perfusión Fuerte)
                // ID 7: bt_vasc (dual-state) → sel_ppg=5 (Vasoconstricción)
                // ID 8: bt_art (dual-state) → sel_ppg=6 (Ruido Movimiento)
                // ID 9: bt_atras (hotspot) → volver a menu
                // ID 10: bt_ir (hotspot) → ir a ppg_wave (si sel_ppg >= 0)
                switch (component) {
                    case 9: 
                        uiEvent = UIEvent::BUTTON_ATRAS; 
                        break;
                    case 10: 
                        uiEvent = UIEvent::BUTTON_IR; 
                        break;
                    case 1:
                        uiEvent = UIEvent::BUTTON_CONDITION;
                        param = 0;  // Normal
                        break;
                    case 3: case 4: case 5: case 6: case 7: case 8:
                        uiEvent = UIEvent::BUTTON_CONDITION;
                        param = component - 2;  // ID 3 → condición 1, ID 8 → condición 6
                        break;
                }
                break;
                
            case 5:  // ECG_WAVE
                // Estructura de ecg_wave:
                // ID 1: ecg (waveform) 399x211 - no genera evento de touch
                // ID 2: v_actual (hotspot) → mostrar popup valores actuales
                // ID 3: parametros (hotspot) → mostrar popup parámetros
                // ID 4: play (hotspot) → iniciar/reanudar señal
                // ID 5: pause (hotspot) → pausar señal
                // ID 6: stop (hotspot) → detener y volver a menú
                switch (component) {
                    case 2: 
                        uiEvent = UIEvent::BUTTON_VALORES; 
                        break;
                    case 3: 
                        uiEvent = UIEvent::BUTTON_PARAMETROS; 
                        break;
                    case 4: 
                        uiEvent = UIEvent::BUTTON_START; 
                        break;
                    case 5: 
                        uiEvent = UIEvent::BUTTON_PAUSE; 
                        break;
                    case 6: 
                        uiEvent = UIEvent::BUTTON_STOP; 
                        break;
                }
                break;
                
            case 6:  // EMG_WAVE
                // Estructura de emg_wave:
                // ID 1: emg (waveform) 399x211 - no genera evento de touch
                // ID 2: v_actual (hotspot) → mostrar popup valores actuales
                // ID 3: parametros (hotspot) → mostrar popup parámetros
                // ID 4: play (hotspot) → iniciar/reanudar señal
                // ID 5: pause (hotspot) → pausar señal
                // ID 6: stop (hotspot) → detener y volver a menú
                switch (component) {
                    case 2: 
                        uiEvent = UIEvent::BUTTON_VALORES; 
                        break;
                    case 3: 
                        uiEvent = UIEvent::BUTTON_PARAMETROS; 
                        break;
                    case 4: 
                        uiEvent = UIEvent::BUTTON_START; 
                        break;
                    case 5: 
                        uiEvent = UIEvent::BUTTON_PAUSE; 
                        break;
                    case 6: 
                        uiEvent = UIEvent::BUTTON_STOP; 
                        break;
                }
                break;
                
            case 7:  // PPG_WAVE
                // Estructura de ppg_wave:
                // ID 1: ppg (waveform) 399x211 - no genera evento de touch
                // ID 2: v_actual (hotspot) → mostrar popup valores actuales
                // ID 3: parametros (hotspot) → mostrar popup parámetros
                // ID 4: play (hotspot) → iniciar/reanudar señal
                // ID 5: pause (hotspot) → pausar señal
                // ID 6: stop (hotspot) → detener y volver a menú
                switch (component) {
                    case 2: 
                        uiEvent = UIEvent::BUTTON_VALORES; 
                        break;
                    case 3: 
                        uiEvent = UIEvent::BUTTON_PARAMETROS; 
                        break;
                    case 4: 
                        uiEvent = UIEvent::BUTTON_START; 
                        break;
                    case 5: 
                        uiEvent = UIEvent::BUTTON_PAUSE; 
                        break;
                    case 6: 
                        uiEvent = UIEvent::BUTTON_STOP; 
                        break;
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
