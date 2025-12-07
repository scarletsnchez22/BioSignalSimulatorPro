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
        
        // DEBUG: Mostrar cada byte recibido (deshabilitado para rendimiento)
        // Serial.printf("[RX] 0x%02X\n", byte);
        
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
                // ID 1: bt_atras (hotspot) → volver a menu
                // ID 2: bt_ir (hotspot) → ir a ecg_wave
                // ID 3: sel_ecg (number) - variable, no genera evento
                // ID 4-12: botones de condición (dual-state)
                //   4=Normal(0), 5=Taquicardia(1), 6=Bradicardia(2),
                //   7=FA(3), 8=FV(4), 9=PVC(5), 10=BRB(6), 11=STup(7), 12=STdn(8)
                switch (component) {
                    case 1: 
                        uiEvent = UIEvent::BUTTON_ATRAS; 
                        break;
                    case 2: 
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
                // Estructura real de ppg_sim (según capturas):
                // ID 1-7: botones de condición en orden
                //   1=Normal(0), 2=Arritmia(1), 3=SPO2 bajo(2), 4=Perfusión débil(3),
                //   5=Perfusión fuerte(4), 6=Vasoconstricción(5), 7=Ruido/Movimiento(6)
                // ID 8: bt_atras (hotspot) → volver a menu
                // ID 9: bt_ir (hotspot) → ir a ppg_wave
                // sel_ppg es un number independiente (no genera evento)
                switch (component) {
                    case 8: 
                        uiEvent = UIEvent::BUTTON_ATRAS; 
                        break;
                    case 9: 
                        uiEvent = UIEvent::BUTTON_IR; 
                        break;
                    case 1: case 2: case 3: case 4: case 5: case 6: case 7:
                        uiEvent = UIEvent::BUTTON_CONDITION;
                        param = component - 1;  // ID 1 → condición 0, ID 7 → condición 6
                        break;
                }
                break;
                
            case 5:  // WAVEFORM_ECG
                // Estructura de waveform_ecg:
                // ID 1: ecg (waveform) 399x211 - no genera evento de touch
                // ID 2: v_actual (hotspot) → ir a valores_ecg (página 6)
                // ID 3: parametros (hotspot) → ir a parametros_ecg (página 7)
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
                
            case 6:  // VALORES_ECG
                // Estructura de valores_ecg:
                // ID 0: p0 (picture) - background
                // ID 1: valores (picture) - popup frame
                // ID 2: bt_act (button) → volver a waveform_ecg (página 5)
                // ID 3-6: t_bpm, t_rr, t_ramp, t_st (text) - etiquetas
                // ID 7-10: n_bpm, n_rr, n_ramp, n_st (xfloat) - valores
                // ID 11: n_beats (number) - contador
                // ID 12: t_beats (text) - etiqueta
                // ID 13: t_patol (text) - patología actual
                switch (component) {
                    case 2:
                        uiEvent = UIEvent::BUTTON_BACK_POPUP;
                        break;
                }
                break;
                
            case 7:  // PARAMETROS_ECG
                // Estructura de parametros_ecg:
                // ID 0: p0 (picture) - background
                // ID 1: parametros (picture) - popup frame
                // ID 2-5: t_hr, t_amp, t_noise, t_hrv (text) - etiquetas
                // ID 6-9: n_hr, n_amp, n_noise, n_hrv (xfloat) - valores display
                // ID 10: h_hr (slider) - frecuencia cardíaca
                // ID 11: h_amp (slider) - amplitud QRS
                // ID 12: h_noise (slider) - nivel de ruido
                // ID 13: h_hrv (slider) - variabilidad HRV
                // ID 14: p_reset (picture) - imagen botón reset
                // ID 15: bt_reset (hotspot) - resetear parámetros
                // ID 16: bt_act (button) - aplicar y cerrar
                // ID 17: bt_ex (hotspot) - cancelar sin guardar
                switch (component) {
                    case 10:
                        uiEvent = UIEvent::SLIDER_ECG_HR;
                        break;
                    case 11:
                        uiEvent = UIEvent::SLIDER_ECG_AMP;
                        break;
                    case 12:
                        uiEvent = UIEvent::SLIDER_ECG_NOISE;
                        break;
                    case 13:
                        uiEvent = UIEvent::SLIDER_ECG_HRV;
                        break;
                    case 15:
                        uiEvent = UIEvent::BUTTON_RESET_PARAMS;
                        break;
                    case 16:
                        uiEvent = UIEvent::BUTTON_APPLY_PARAMS;
                        break;
                    case 17:
                        uiEvent = UIEvent::BUTTON_CANCEL_PARAMS;
                        break;
                }
                break;
                
            case 8:  // WAVEFORM_EMG
                // Estructura de waveform_emg:
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
                
            case 9:  // VALORES_EMG
                // Estructura de valores_emg:
                // ID 0: p0 (picture) - background
                // ID 1: valores (picture) - popup frame
                // ID 2: bt_act (button) → volver a waveform_emg (página 8)
                // ID 3-6: t_rms, t_mu, t_freq, t_cont (text) - etiquetas
                // ID 7-10: n_rms, n_mu, n_freq, n_cont (xfloat/number) - valores
                // ID 11: t_patol (text) - nombre condición
                switch (component) {
                    case 2:
                        uiEvent = UIEvent::BUTTON_BACK_POPUP;
                        break;
                }
                break;
                
            case 10:  // PARAMETROS_EMG
                // Estructura de parametros_emg:
                // ID 0: p0 (picture) - background
                // ID 1: parametros (picture) - popup frame
                // ID 2-4: t_exc, t_amp, t_noise (text) - etiquetas
                // ID 5-7: n_exc, n_amp, n_noise (xfloat) - valores display
                // ID 8: h_exc (slider) - excitación
                // ID 9: h_amp (slider) - amplitud
                // ID 10: h_noise (slider) - ruido
                // ID 11: p_reset (picture) - imagen botón reset
                // ID 12: bt_reset (hotspot) - resetear parámetros
                // ID 13: bt_act (button) - aplicar y cerrar
                // ID 14: bt_ex (hotspot) - cancelar sin guardar
                switch (component) {
                    case 8:
                        uiEvent = UIEvent::SLIDER_EMG_EXC;
                        break;
                    case 9:
                        uiEvent = UIEvent::SLIDER_EMG_AMP;
                        break;
                    case 10:
                        uiEvent = UIEvent::SLIDER_EMG_NOISE;
                        break;
                    case 12:
                        uiEvent = UIEvent::BUTTON_RESET_PARAMS;
                        break;
                    case 13:
                        uiEvent = UIEvent::BUTTON_APPLY_PARAMS;
                        break;
                    case 14:
                        uiEvent = UIEvent::BUTTON_CANCEL_PARAMS;
                        break;
                }
                break;
                
            case 11:  // WAVEFORM_PPG
                // Estructura de waveform_ppg:
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
                
            case 12:  // VALORES_PPG
                // Estructura de valores_ppg:
                // ID 0: p0 (picture) - background
                // ID 1: valores (picture) - popup frame
                // ID 2: bt_act (button) → volver a waveform_ppg (página 11)
                // ID 3-6: t_hr, t_rr, t_pi, t_spo2 (text) - etiquetas
                // ID 7-10: n_hr, n_rr, n_pi, n_spo2 (xfloat) - valores
                // ID 11: t_beats (text) - etiqueta
                // ID 12: n_beats (number) - contador
                // ID 13: t_patol (text) - nombre condición
                switch (component) {
                    case 2:
                        uiEvent = UIEvent::BUTTON_BACK_POPUP;
                        break;
                }
                break;
                
            case 13:  // PARAMETROS_PPG
                // Estructura de parametros_ppg:
                // ID 0: p0 (picture) - background
                // ID 1: parametros (picture) - popup frame
                // ID 2-4: t_hr, t_pi, t_noise (text) - etiquetas
                // ID 5-7: n_hr, n_pi, n_noise (xfloat) - valores display
                // ID 8: h_hr (slider) - frecuencia cardíaca
                // ID 9: h_pi (slider) - índice de perfusión
                // ID 10: h_noise (slider) - ruido
                // ID 11: p_reset (picture) - imagen botón reset
                // ID 12: bt_reset (hotspot) - resetear parámetros
                // ID 13: bt_act (button) - aplicar y cerrar
                // ID 14: bt_ex (hotspot) - cancelar sin guardar
                switch (component) {
                    case 8:
                        uiEvent = UIEvent::SLIDER_PPG_HR;
                        break;
                    case 9:
                        uiEvent = UIEvent::SLIDER_PPG_PI;
                        break;
                    case 10:
                        uiEvent = UIEvent::SLIDER_PPG_NOISE;
                        break;
                    case 12:
                        uiEvent = UIEvent::BUTTON_RESET_PARAMS;
                        break;
                    case 13:
                        uiEvent = UIEvent::BUTTON_APPLY_PARAMS;
                        break;
                    case 14:
                        uiEvent = UIEvent::BUTTON_CANCEL_PARAMS;
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
// ACTUALIZAR BOTONES DEL MENÚ
// ============================================================================
void NextionDriver::updateMenuButtons(SignalType selected) {
    // IDs de botones en página MENU (página 1):
    // bt_ecg = 1, bt_emg = 2, bt_ppg = 3
    
    // Despintar todos (pic y pic2 = 0, val = 0 para dual state)
    sendCommand("bt_ecg.val=0");
    sendCommand("bt_emg.val=0");
    sendCommand("bt_ppg.val=0");
    
    // Pintar el seleccionado (val = 1 para dual state)
    switch (selected) {
        case SignalType::ECG:
            sendCommand("bt_ecg.val=1");
            break;
        case SignalType::EMG:
            sendCommand("bt_emg.val=1");
            break;
        case SignalType::PPG:
            sendCommand("bt_ppg.val=1");
            break;
        default:
            break;
    }
}

// ============================================================================
// ACTUALIZAR BOTONES DE CONDICIONES ECG
// ============================================================================
void NextionDriver::updateECGConditionButtons(int selectedCondition) {
    // Forzar limpieza total: val=0, pic=0, pic2=0 para dual-state
    sendCommand("bt_nom.val=0");
    sendCommand("bt_nom.pic=0");
    sendCommand("bt_nom.pic2=0");
    sendCommand("bt_taq.val=0");
    sendCommand("bt_bra.val=0");
    sendCommand("bt_fa.val=0");
    sendCommand("bt_fv.val=0");
    sendCommand("bt_pvc.val=0");
    sendCommand("bt_brb.val=0");
    sendCommand("bt_stup.val=0");
    sendCommand("bt_stdn.val=0");

    char cmd[32];
    for (int id = 4; id <= 12; id++) {
        sprintf(cmd, "b%d.val=0", id);
        sendCommand(cmd);
    }

    switch (selectedCondition) {
        case 0: 
            sendCommand("bt_nom.val=1"); 
            sendCommand("b4.val=1"); 
            break;
        case 1: sendCommand("bt_taq.val=1"); sendCommand("b5.val=1"); break;
        case 2: sendCommand("bt_bra.val=1"); sendCommand("b6.val=1"); break;
        case 3: sendCommand("bt_fa.val=1");  sendCommand("b7.val=1"); break;
        case 4: sendCommand("bt_fv.val=1");  sendCommand("b8.val=1"); break;
        case 5: sendCommand("bt_pvc.val=1"); sendCommand("b9.val=1"); break;
        case 6: sendCommand("bt_brb.val=1"); sendCommand("b10.val=1"); break;
        case 7: sendCommand("bt_stup.val=1");sendCommand("b11.val=1"); break;
        case 8: sendCommand("bt_stdn.val=1");sendCommand("b12.val=1"); break;
        default: break;
    }

    int ecgSel = (selectedCondition >= 0 && selectedCondition <= 8) ? selectedCondition : -1;
    sprintf(cmd, "sel_ecg.val=%d", ecgSel);
    sendCommand(cmd);
}

// ============================================================================
// ACTUALIZAR BOTONES DE CONDICIONES EMG
// ============================================================================
void NextionDriver::updateEMGConditionButtons(int selectedCondition) {
    // bt_reposo (0), bt_leve (1), bt_moderada (2), bt_fuerte (3), bt_maxima (4),
    // bt_temblor (5), bt_miopatia (6), bt_neuropatia (7), bt_fasc (8), bt_fatiga (9)

    sendCommand("bt_reposo.val=0");
    sendCommand("bt_leve.val=0");
    sendCommand("bt_moderada.val=0");
    sendCommand("bt_fuerte.val=0");
    sendCommand("bt_maxima.val=0");
    sendCommand("bt_temblor.val=0");
    sendCommand("bt_miopatia.val=0");
    sendCommand("bt_neuropatia.val=0");
    sendCommand("bt_fasc.val=0");
    sendCommand("bt_fatiga.val=0");

    switch (selectedCondition) {
        case 0: sendCommand("bt_reposo.val=1"); break;
        case 1: sendCommand("bt_leve.val=1"); break;
        case 2: sendCommand("bt_moderada.val=1"); break;
        case 3: sendCommand("bt_fuerte.val=1"); break;
        case 4: sendCommand("bt_maxima.val=1"); break;
        case 5: sendCommand("bt_temblor.val=1"); break;
        case 6: sendCommand("bt_miopatia.val=1"); break;
        case 7: sendCommand("bt_neuropatia.val=1"); break;
        case 8: sendCommand("bt_fasc.val=1"); break;
        case 9: sendCommand("bt_fatiga.val=1"); break;
        default: break;
    }

    char cmd[24];
    int emgSel = (selectedCondition >= 0 && selectedCondition <= 9) ? selectedCondition : -1;
    sprintf(cmd, "sel_emg.val=%d", emgSel);
    sendCommand(cmd);
}

// ============================================================================
// ACTUALIZAR BOTONES DE CONDICIONES PPG
// ============================================================================
void NextionDriver::updatePPGConditionButtons(int selectedCondition) {
    // Forzar limpieza total: val=0, pic=0, pic2=0 para dual-state
    sendCommand("bt_nom.val=0");
    sendCommand("bt_nom.pic=0");
    sendCommand("bt_nom.pic2=0");
    sendCommand("bt_arr.val=0");
    sendCommand("bt_spo2.val=0");
    sendCommand("bt_lowp.val=0");
    sendCommand("bt_highp.val=0");
    sendCommand("bt_vasc.val=0");
    sendCommand("bt_art.val=0");

    char cmd[32];
    for (int id = 1; id <= 7; id++) {
        sprintf(cmd, "b%d.val=0", id);
        sendCommand(cmd);
    }

    switch (selectedCondition) {
        case 0: 
            sendCommand("bt_nom.val=1"); 
            sendCommand("b1.val=1"); 
            break;
        case 1: sendCommand("bt_arr.val=1"); sendCommand("b2.val=1"); break;
        case 2: sendCommand("bt_spo2.val=1"); sendCommand("b3.val=1"); break;
        case 3: sendCommand("bt_lowp.val=1"); sendCommand("b4.val=1"); break;
        case 4: sendCommand("bt_highp.val=1");sendCommand("b5.val=1"); break;
        case 5: sendCommand("bt_vasc.val=1"); sendCommand("b6.val=1"); break;
        case 6: sendCommand("bt_art.val=1");  sendCommand("b7.val=1"); break;
        default: break;
    }

    int ppgSel = (selectedCondition >= 0 && selectedCondition <= 6) ? selectedCondition : -1;
    sprintf(cmd, "sel_ppg.val=%d", ppgSel);
    sendCommand(cmd);
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

// ============================================================================
// ACTUALIZAR PÁGINA VALORES ECG
// ============================================================================
void NextionDriver::updateECGValuesPage(int bpm, int rr_ms, int rAmp_x100, int st_x100,
                                         uint32_t beats, const char* patologia) {
    // Actualizar números (xfloat en Nextion)
    setNumber("n_bpm", bpm);           // ID 3: Frecuencia cardíaca
    setNumber("n_rr", rr_ms);          // ID 4: Intervalo RR en ms
    setNumber("n_ramp", rAmp_x100);    // ID 5: Amplitud R × 100 (1.25mV → 125)
    setNumber("n_st", st_x100);        // ID 6: Desviación ST × 100 (-0.15mV → -15)
    setNumber("n_beats", (int)beats);  // ID 11: Contador de latidos
    
    // Actualizar texto de patología
    setText("t_patol", patologia);      // ID 13: Nombre de patología
}

// ============================================================================
// ACTUALIZAR PÁGINA VALORES EMG
// ============================================================================
void NextionDriver::updateEMGValuesPage(int rms_x100, int activeUnits, int freq_x10, int contraction,
                                         const char* condicion) {
    // Actualizar números (xfloat/number en Nextion)
    setNumber("n_rms", rms_x100);         // ID 7: Amplitud RMS × 100 (0.25mV → 25, vvs0=2)
    setNumber("n_mu", activeUnits);       // ID 8: Unidades motoras activas (entero)
    setNumber("n_freq", freq_x10);        // ID 9: Frecuencia × 10 (12.5Hz → 125, vvs0=1)
    setNumber("n_cont", contraction);     // ID 10: Contracción % (entero, vvs0=0)
    
    // Actualizar texto de condición
    setText("t_patol", condicion);        // ID 11: Nombre de condición
}

// ============================================================================
// ACTUALIZAR PÁGINA VALORES PPG
// ============================================================================
void NextionDriver::updatePPGValuesPage(int hr, int rr_ms, int pi_x10, int spo2,
                                         uint32_t beats, const char* condicion) {
    // Actualizar números (xfloat/number en Nextion)
    setNumber("n_hr", hr);                // ID 7: Frecuencia cardíaca (entero, vvs0=0)
    setNumber("n_rr", rr_ms);             // ID 8: Intervalo RR en ms (entero, vvs0=0)
    setNumber("n_pi", pi_x10);            // ID 9: Índice perfusión × 10 (5.2% → 52, vvs0=1)
    setNumber("n_spo2", spo2);            // ID 10: SpO2 % (entero, vvs0=0)
    setNumber("n_beats", (int)beats);     // ID 12: Contador de latidos
    
    // Actualizar texto de condición
    setText("t_patol", condicion);        // ID 13: Nombre de condición
}

// ============================================================================
// CONFIGURAR PÁGINA PARÁMETROS ECG
// ============================================================================
void NextionDriver::setupECGParametersPage(int hrMin, int hrMax, int hrCurrent,
                                            int ampCurrent, int noiseCurrent, int hrvCurrent) {
    // Configurar límites del slider HR según la patología
    char cmd[32];
    
    // Slider HR (ID 11): límites dinámicos según patología
    sprintf(cmd, "h_hr.minval=%d", hrMin);
    sendCommand(cmd);
    sprintf(cmd, "h_hr.maxval=%d", hrMax);
    sendCommand(cmd);
    sprintf(cmd, "h_hr.val=%d", hrCurrent);
    sendCommand(cmd);
    sprintf(cmd, "n_hr.val=%d", hrCurrent);   // ID 7: xfloat display
    sendCommand(cmd);
    
    // Slider Amplitud (ID 12): rango fijo 50-200 (representa 0.50-2.00 mV)
    sendCommand("h_amp.minval=50");
    sendCommand("h_amp.maxval=200");
    sprintf(cmd, "h_amp.val=%d", ampCurrent);
    sendCommand(cmd);
    sprintf(cmd, "n_amp.val=%d", ampCurrent); // ID 8: xfloat display
    sendCommand(cmd);
    
    // Slider Ruido (ID 13): rango fijo 0-10 (representa 0.00-0.10)
    sendCommand("h_noise.minval=0");
    sendCommand("h_noise.maxval=10");
    sprintf(cmd, "h_noise.val=%d", noiseCurrent);
    sendCommand(cmd);
    sprintf(cmd, "n_noise.val=%d", noiseCurrent); // ID 9: xfloat display
    sendCommand(cmd);
    
    // Slider HRV (ID 14): rango fijo 0-15 (representa 0-15%)
    sendCommand("h_hrv.minval=0");
    sendCommand("h_hrv.maxval=15");
    sprintf(cmd, "h_hrv.val=%d", hrvCurrent);
    sendCommand(cmd);
    sprintf(cmd, "n_hrv.val=%d", hrvCurrent); // ID 10: xfloat display
    sendCommand(cmd);
}

// ============================================================================
// CONFIGURAR PÁGINA PARÁMETROS EMG
// ============================================================================
void NextionDriver::setupEMGParametersPage(int excCurrent, int ampCurrent, int noiseCurrent) {
    char cmd[32];
    
    // Slider Excitación (ID 8): rango fijo 0-100 (representa 0-100%)
    sendCommand("h_exc.minval=0");
    sendCommand("h_exc.maxval=100");
    sprintf(cmd, "h_exc.val=%d", excCurrent);
    sendCommand(cmd);
    sprintf(cmd, "n_exc.val=%d", excCurrent); // ID 5: xfloat display (vvs0=0)
    sendCommand(cmd);
    
    // Slider Amplitud (ID 9): rango fijo 10-300 (representa 0.10-3.00)
    sendCommand("h_amp.minval=10");
    sendCommand("h_amp.maxval=300");
    sprintf(cmd, "h_amp.val=%d", ampCurrent);
    sendCommand(cmd);
    sprintf(cmd, "n_amp.val=%d", ampCurrent); // ID 6: xfloat display (vvs0=2)
    sendCommand(cmd);
    
    // Slider Ruido (ID 10): rango fijo 0-100 (representa 0.00-1.00)
    sendCommand("h_noise.minval=0");
    sendCommand("h_noise.maxval=100");
    sprintf(cmd, "h_noise.val=%d", noiseCurrent);
    sendCommand(cmd);
    sprintf(cmd, "n_noise.val=%d", noiseCurrent); // ID 7: xfloat display (vvs0=2)
    sendCommand(cmd);
}

// ============================================================================
// CONFIGURAR PÁGINA PARÁMETROS PPG
// ============================================================================
void NextionDriver::setupPPGParametersPage(int hrCurrent, int piCurrent, int noiseCurrent) {
    char cmd[32];
    
    // Slider HR (ID 8): rango fijo 50-150 BPM
    sendCommand("h_hr.minval=50");
    sendCommand("h_hr.maxval=150");
    sprintf(cmd, "h_hr.val=%d", hrCurrent);
    sendCommand(cmd);
    sprintf(cmd, "n_hr.val=%d", hrCurrent); // ID 5: xfloat display (vvs0=0)
    sendCommand(cmd);
    
    // Slider PI (ID 9): rango fijo 3-200 (representa 0.3-20.0%)
    sendCommand("h_pi.minval=3");
    sendCommand("h_pi.maxval=200");
    sprintf(cmd, "h_pi.val=%d", piCurrent);
    sendCommand(cmd);
    sprintf(cmd, "n_pi.val=%d", piCurrent); // ID 6: xfloat display (vvs0=1)
    sendCommand(cmd);
    
    // Slider Ruido (ID 10): rango fijo 0-100 (representa 0.00-1.00)
    sendCommand("h_noise.minval=0");
    sendCommand("h_noise.maxval=100");
    sprintf(cmd, "h_noise.val=%d", noiseCurrent);
    sendCommand(cmd);
    sprintf(cmd, "n_noise.val=%d", noiseCurrent); // ID 7: xfloat display (vvs0=2)
    sendCommand(cmd);
}

// ============================================================================
// LEER VALOR DE SLIDER
// ============================================================================
int NextionDriver::readSliderValue(const char* sliderName) {
    // Enviar comando get para solicitar el valor
    char cmd[32];
    sprintf(cmd, "get %s.val", sliderName);
    sendCommand(cmd);
    
    // Esperar respuesta (timeout 100ms)
    unsigned long startTime = millis();
    while (millis() - startTime < 100) {
        if (serial.available() >= 8) {
            // Respuesta numérica: 0x71 [4 bytes little-endian] 0xFF 0xFF 0xFF
            uint8_t response[8];
            for (int i = 0; i < 8; i++) {
                response[i] = serial.read();
            }
            
            if (response[0] == 0x71 && 
                response[5] == 0xFF && 
                response[6] == 0xFF && 
                response[7] == 0xFF) {
                // Decodificar valor little-endian
                int32_t value = response[1] | 
                               (response[2] << 8) | 
                               (response[3] << 16) | 
                               (response[4] << 24);
                return value;
            }
        }
        delay(1);
    }
    
    return -1;  // Error o timeout
}
