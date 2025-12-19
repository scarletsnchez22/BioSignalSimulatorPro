/**
 * @file nextion_driver.cpp
 * @brief Implementación del driver Nextion NX8048T070
 * @version 1.0.0
 * @date 18 Diciembre 2025
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
                // Estructura de ecg_sim (800x480):
                // ID 1-8: botones de condición (dual-state)
                //   1=Normal(0), 2=Taquicardia(1), 3=Bradicardia(2), 4=Bloqueo(3),
                //   5=FA(4), 6=FV(5), 7=STEMI(6), 8=Isquemia(7)
                // ID 9: bt_atras (hotspot) → volver a menu
                // ID 10: bt_ir (hotspot) → ir a waveform_ecg
                // ID 11: sel_ecg (number) - variable, no genera evento
                switch (component) {
                    case 1: case 2: case 3: case 4: 
                    case 5: case 6: case 7: case 8:
                        uiEvent = UIEvent::BUTTON_CONDITION;
                        param = component - 1;  // ID 1 → condición 0, ID 8 → condición 7
                        break;
                    case 9: 
                        uiEvent = UIEvent::BUTTON_ATRAS; 
                        break;
                    case 10: 
                        uiEvent = UIEvent::BUTTON_IR; 
                        break;
                }
                break;
                
            case 3:  // EMG_SIM
                // Estructura de emg_sim (800x480):
                // ID 1-6: botones de condición (dual-state)
                //   1=Reposo(0), 2=Leve(1), 3=Moderada(2), 4=Máxima(3),
                //   5=Temblor(4), 6=Fatiga(5)
                // ID 7: bt_atras (hotspot) → volver a menu
                // ID 8: bt_ir (hotspot) → ir a waveform_emg
                // ID 9: sel_emg (number) - variable, no genera evento
                switch (component) {
                    case 1: case 2: case 3: case 4: case 5: case 6:
                        uiEvent = UIEvent::BUTTON_CONDITION;
                        param = component - 1;  // ID 1 → condición 0, ID 6 → condición 5
                        break;
                    case 7: 
                        uiEvent = UIEvent::BUTTON_ATRAS; 
                        break;
                    case 8: 
                        uiEvent = UIEvent::BUTTON_IR; 
                        break;
                }
                break;
                
            case 4:  // PPG_SIM
                // Estructura de ppg_sim (800x480):
                // ID 1-6: botones de condición (dual-state)
                //   1=Normal(0), 2=Arritmia(1), 3=Perfusión débil(2), 4=Perfusión fuerte(3),
                //   5=Vasodilatación(4), 6=Vasoconstricción(5)
                // ID 7: bt_atras (hotspot) → volver a menu
                // ID 8: bt_ir (hotspot) → ir a waveform_ppg
                // ID 9: sel_ppg (number) - variable, no genera evento
                switch (component) {
                    case 1: case 2: case 3: case 4: case 5: case 6:
                        uiEvent = UIEvent::BUTTON_CONDITION;
                        param = component - 1;  // ID 1 → condición 0, ID 6 → condición 5
                        break;
                    case 7: 
                        uiEvent = UIEvent::BUTTON_ATRAS; 
                        break;
                    case 8: 
                        uiEvent = UIEvent::BUTTON_IR; 
                        break;
                }
                break;
                
            case 5:  // WAVEFORM_ECG
                // Estructura de waveform_ecg (700x380 px):
                // ID 1: ecg (waveform) - área de dibujo
                // ID 2: play (hotspot) → iniciar simulación (si calibración OK)
                // ID 3: pause (hotspot) → pausar señal
                // ID 4: stop (hotspot) → detener y volver a ecg_sim
                // ID 5: parametros (hotspot) → ir a parametros_ecg
                // ID 6: t_patol (text) - nombre patología
                // ID 7-17: textos etiquetas (trr,tpr,tqrs,tqtc,tp,tq,tr,ts,tt,tst,thr)
                // ID 19-29: valores xfloat (nrr,npr,nqrs,nqtc,np,nq,nr,ns,nt,nst,nhr)
                // ID 31: t_mvdiv (text) - escala mV/div actual
                // ID 32: t_msdiv (text) - escala ms/div actual
                // ID 33: h_barra (progress bar) - progreso calibración
                switch (component) {
                    case 2: 
                        uiEvent = UIEvent::BUTTON_START; 
                        break;
                    case 3: 
                        uiEvent = UIEvent::BUTTON_PAUSE; 
                        break;
                    case 4: 
                        uiEvent = UIEvent::BUTTON_STOP; 
                        break;
                    case 5: 
                        uiEvent = UIEvent::BUTTON_PARAMETROS; 
                        break;
                }
                break;
                
            case 6:  // PARAMETROS_ECG (popup sobre waveform_ecg)
                // Estructura de parametros_ecg (página 6):
                // ID 2: bt_act (button) - guardar cambios y volver a waveform
                // ID 3: bt_ex (hotspot) - salir sin guardar
                // ID 4: h_hr (slider) - frecuencia cardíaca (BPM)
                // ID 5: h_amp (slider) - factor zoom visual (%)
                // ID 6: h_noise (slider) - nivel ruido
                // ID 7: h_hrv (slider) - variabilidad HRV (%)
                // ID 12: n_hr (xfloat vvs0=3,vvs1=0) - valor HR
                // ID 13: n_amp (xfloat vvs0=3,vvs1=0) - valor zoom %
                // ID 14: n_noise (xfloat vvs0=1,vvs1=2) - valor ruido
                // ID 15: n_hrv (xfloat vvs0=2,vvs1=0) - valor HRV
                // ID 16: bt_rst (button) - reset a valores iniciales
                // ID 18: t_esc (text) - escala actual "X.XX mV/div"
                switch (component) {
                    case 2:
                        uiEvent = UIEvent::BUTTON_APPLY_PARAMS;
                        break;
                    case 3:
                        uiEvent = UIEvent::BUTTON_CANCEL_PARAMS;
                        break;
                    case 4:
                        uiEvent = UIEvent::SLIDER_ECG_HR;
                        break;
                    case 5:
                        uiEvent = UIEvent::SLIDER_ECG_AMP;
                        break;
                    case 6:
                        uiEvent = UIEvent::SLIDER_ECG_NOISE;
                        break;
                    case 7:
                        uiEvent = UIEvent::SLIDER_ECG_HRV;
                        break;
                    case 16:
                        uiEvent = UIEvent::BUTTON_RESET_PARAMS;
                        break;
                }
                break;
                
            case 7:  // WAVEFORM_EMG
                // Estructura de waveform_emg (700x380 px):
                // ID 1: emg (waveform) - área de dibujo
                // ID 2: play (hotspot) → iniciar simulación
                // ID 3: pause (hotspot) → pausar señal
                // ID 4: stop (hotspot) → detener y volver a emg_sim
                // ID 5: parametros (hotspot) → ir a parametros_emg
                // ID 6: t_patol (text) - nombre condición
                // ID 13-18: nraw, nenv, nrms, nmu, nfr, nmvc (xfloat valores)
                // ID 20: mvdiv (text) - escala mV/div raw
                // ID 21: mvdiv2 (text) - escala mV/div envolvente
                // ID ??: msdiv (text) - escala ms/div (compartida)
                switch (component) {
                    case 2: 
                        uiEvent = UIEvent::BUTTON_START; 
                        break;
                    case 3: 
                        uiEvent = UIEvent::BUTTON_PAUSE; 
                        break;
                    case 4: 
                        uiEvent = UIEvent::BUTTON_STOP; 
                        break;
                    case 5: 
                        uiEvent = UIEvent::BUTTON_PARAMETROS; 
                        break;
                }
                break;
                
            case 8:  // PARAMETROS_EMG (popup sobre waveform_emg)
                // Estructura de parametros_emg (página 8):
                // ID 2: bt_act (button) - guardar cambios y volver a waveform
                // ID 3: bt_ex (hotspot) - salir sin guardar
                // ID 4: h_exc (slider) - nivel excitación (%)
                // ID 5: h_amp (slider) - factor zoom visual (%)
                // ID 6: h_noise (slider) - nivel ruido
                // ID ??: n_exc, n_amp, n_noise (xfloat valores)
                // ID 13: bt_rst (button) - reset a valores iniciales
                switch (component) {
                    case 2:
                        uiEvent = UIEvent::BUTTON_APPLY_PARAMS;
                        break;
                    case 3:
                        uiEvent = UIEvent::BUTTON_CANCEL_PARAMS;
                        break;
                    case 4:
                        uiEvent = UIEvent::SLIDER_EMG_EXC;
                        break;
                    case 5:
                        uiEvent = UIEvent::SLIDER_EMG_AMP;
                        break;
                    case 6:
                        uiEvent = UIEvent::SLIDER_EMG_NOISE;
                        break;
                    case 13:
                        uiEvent = UIEvent::BUTTON_RESET_PARAMS;
                        break;
                }
                break;
                
            case 9:  // WAVEFORM_PPG
                // Estructura de waveform_ppg (700x380 px):
                // ID 1: ppg (waveform) - área de dibujo
                // ID 2: play (hotspot) → iniciar simulación
                // ID 3: pause (hotspot) → pausar señal
                // ID 4: stop (hotspot) → detener y volver a ppg_sim
                // ID 5: parametros (hotspot) → ir a parametros_ppg
                // ID 6: t_patol (text) - nombre condición
                // ID 13-18: nac, nenv, nhr, npi, nsys, ndia (xfloat valores)
                // ID 20: mvdiv (text) - escala mV/div
                // ID 21: msdiv (text) - escala ms/div
                // ID 23: ndc (xfloat) - valor DC actual en mV
                switch (component) {
                    case 2: 
                        uiEvent = UIEvent::BUTTON_START; 
                        break;
                    case 3: 
                        uiEvent = UIEvent::BUTTON_PAUSE; 
                        break;
                    case 4: 
                        uiEvent = UIEvent::BUTTON_STOP; 
                        break;
                    case 5: 
                        uiEvent = UIEvent::BUTTON_PARAMETROS; 
                        break;
                }
                break;
                
            case 10:  // PARAMETROS_PPG (popup sobre waveform_ppg)
                // Estructura de parametros_ppg (página 10):
                // ID 2: bt_act (button) - guardar cambios y volver a waveform
                // ID 3: bt_ex (hotspot) - salir sin guardar
                // ID 4: h_hr (slider) - frecuencia cardíaca (BPM)
                // ID 5: h_pi (slider) - índice de perfusión (%)
                // ID 6: h_noise (slider) - nivel ruido
                // ID 10-12: n_hr, n_pi, n_noise (xfloat valores)
                // ID 13: bt_rst (button) - reset a valores iniciales
                // ID 14: h_amp (slider) - factor amplitud/zoom (%)
                // ID 16: n_amp (xfloat) - valor amplitud
                switch (component) {
                    case 2:
                        uiEvent = UIEvent::BUTTON_APPLY_PARAMS;
                        break;
                    case 3:
                        uiEvent = UIEvent::BUTTON_CANCEL_PARAMS;
                        break;
                    case 4:
                        uiEvent = UIEvent::SLIDER_PPG_HR;
                        break;
                    case 5:
                        uiEvent = UIEvent::SLIDER_PPG_PI;
                        break;
                    case 6:
                        uiEvent = UIEvent::SLIDER_PPG_NOISE;
                        break;
                    case 13:
                        uiEvent = UIEvent::BUTTON_RESET_PARAMS;
                        break;
                    case 14:
                        uiEvent = UIEvent::SLIDER_PPG_AMP;
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
    Serial.printf("[ECG] updateConditionButtons: cond=%d\n", selectedCondition);
    
    // Estructura ecg_sim (800x480):
    // ID 1-8: bt_norm, bt_taq, bt_bra, bt_blk, bt_fa, bt_fv, bt_stup, bt_stdn
    // ID 9: bt_atras, ID 10: bt_ir, ID 11: sel_ecg
    
    char cmd[32];
    
    // Limpiar todos los botones de condición
    sendCommand("bt_norm.val=0");
    sendCommand("bt_taq.val=0");
    sendCommand("bt_bra.val=0");
    sendCommand("bt_blk.val=0");
    sendCommand("bt_fa.val=0");
    sendCommand("bt_fv.val=0");
    sendCommand("bt_stup.val=0");
    sendCommand("bt_stdn.val=0");

    // Activar el botón seleccionado
    switch (selectedCondition) {
        case 0: sendCommand("bt_norm.val=1"); break;
        case 1: sendCommand("bt_taq.val=1"); break;
        case 2: sendCommand("bt_bra.val=1"); break;
        case 3: sendCommand("bt_blk.val=1"); break;
        case 4: sendCommand("bt_fa.val=1"); break;
        case 5: sendCommand("bt_fv.val=1"); break;
        case 6: sendCommand("bt_stup.val=1"); break;
        case 7: sendCommand("bt_stdn.val=1"); break;
        default: break;
    }

    // Actualizar variable sel_ecg
    int ecgSel = (selectedCondition >= 0 && selectedCondition <= 7) ? selectedCondition : 255;
    sprintf(cmd, "sel_ecg.val=%d", ecgSel);
    sendCommand(cmd);
}

// ============================================================================
// ACTUALIZAR BOTONES DE CONDICIONES EMG
// ============================================================================
void NextionDriver::updateEMGConditionButtons(int selectedCondition) {
    Serial.printf("[EMG] updateConditionButtons: cond=%d\n", selectedCondition);
    
    // Estructura emg_sim (800x480):
    // ID 1-6: bt_reposo, bt_leve, bt_moderada, bt_maxima, bt_temblor, bt_fatiga
    // ID 7: bt_atras, ID 8: bt_ir, ID 9: sel_emg
    
    char cmd[32];
    
    // Limpiar todos los botones de condición
    sendCommand("bt_reposo.val=0");
    sendCommand("bt_leve.val=0");
    sendCommand("bt_moderada.val=0");
    sendCommand("bt_maxima.val=0");
    sendCommand("bt_temblor.val=0");
    sendCommand("bt_fatiga.val=0");

    // Activar el botón seleccionado
    switch (selectedCondition) {
        case 0: sendCommand("bt_reposo.val=1"); break;
        case 1: sendCommand("bt_leve.val=1"); break;
        case 2: sendCommand("bt_moderada.val=1"); break;
        case 3: sendCommand("bt_maxima.val=1"); break;
        case 4: sendCommand("bt_temblor.val=1"); break;
        case 5: sendCommand("bt_fatiga.val=1"); break;
        default: break;
    }

    // Actualizar variable sel_emg
    int emgSel = (selectedCondition >= 0 && selectedCondition <= 5) ? selectedCondition : 255;
    sprintf(cmd, "sel_emg.val=%d", emgSel);
    sendCommand(cmd);
}

// ============================================================================
// ACTUALIZAR BOTONES DE CONDICIONES PPG
// ============================================================================
void NextionDriver::updatePPGConditionButtons(int selectedCondition) {
    Serial.printf("[PPG] updateConditionButtons: cond=%d\n", selectedCondition);
    
    // Estructura ppg_sim (800x480):
    // ID 1-6: bt_norm, bt_arr, bt_lowp, bt_highp, bt_vasod, bt_vascon
    // ID 7: bt_atras, ID 8: bt_ir, ID 9: sel_ppg
    
    char cmd[32];
    
    // Limpiar todos los botones de condición
    sendCommand("bt_norm.val=0");
    sendCommand("bt_arr.val=0");
    sendCommand("bt_lowp.val=0");
    sendCommand("bt_highp.val=0");
    sendCommand("bt_vasod.val=0");
    sendCommand("bt_vascon.val=0");

    // Activar el botón seleccionado
    switch (selectedCondition) {
        case 0: sendCommand("bt_norm.val=1"); break;
        case 1: sendCommand("bt_arr.val=1"); break;
        case 2: sendCommand("bt_lowp.val=1"); break;
        case 3: sendCommand("bt_highp.val=1"); break;
        case 4: sendCommand("bt_vasod.val=1"); break;
        case 5: sendCommand("bt_vascon.val=1"); break;
        default: break;
    }

    // Actualizar variable sel_ppg
    int ppgSel = (selectedCondition >= 0 && selectedCondition <= 5) ? selectedCondition : 255;
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
    // Actualizar números en waveform_ecg (página 5)
    setNumber("n_bpm", bpm);           // ID 12: Frecuencia cardíaca
    setNumber("n_rr", rr_ms);          // ID 13: Intervalo RR en ms
    setNumber("n_ramp", rAmp_x100);    // ID 14: Amplitud R × 100 (1.25mV → 125)
    setNumber("n_st", st_x100);        // ID 15: Desviación ST × 100 (-0.15mV → -15)
    setNumber("n_beats", (int)beats);  // ID 16: Contador de latidos
    
    // Actualizar texto de patología
    setText("t_patol", patologia);      // ID 6: Nombre de patología
}

// ============================================================================
// ACTUALIZAR PÁGINA VALORES EMG
// ============================================================================
void NextionDriver::updateEMGValuesPage(int rms_x100, int activeUnits, int freq_x10, int contraction,
                                         const char* condicion) {
    // Actualizar números en waveform_emg (página 7)
    setNumber("n_rms", rms_x100);         // ID 11: Amplitud RMS × 100 (0.25mV → 25, vvs0=2)
    setNumber("n_mu", activeUnits);       // ID 12: Unidades motoras activas (entero)
    setNumber("n_freq", freq_x10);        // ID 13: Frecuencia × 10 (12.5Hz → 125, vvs0=1)
    setNumber("n_cont", contraction);     // ID 14: Contracción % (entero, vvs0=0)
    
    // Actualizar texto de condición
    setText("t_patol", condicion);        // ID 6: Nombre de condición
}

// ============================================================================
// ACTUALIZAR PÁGINA VALORES PPG
// ============================================================================
void NextionDriver::updatePPGValuesPage(int hr, int rr_ms, int pi_x10, int spo2,
                                         uint32_t beats, const char* condicion) {
    // Actualizar números en waveform_ppg (página 9)
    setNumber("n_hr", hr);                // ID 12: Frecuencia cardíaca (entero, vvs0=0)
    setNumber("n_rr", rr_ms);             // ID 13: Intervalo RR en ms (entero, vvs0=0)
    setNumber("n_pi", pi_x10);            // ID 14: Índice perfusión × 10 (5.2% → 52, vvs0=1)
    setNumber("n_spo2", spo2);            // ID 15: SpO2 % (entero, vvs0=0)
    setNumber("n_beats", (int)beats);     // ID 16: Contador de latidos
    
    // Actualizar texto de condición
    setText("t_patol", condicion);        // ID 6: Nombre de condición
}

// ============================================================================
// CONFIGURAR PÁGINA PARÁMETROS ECG
// ============================================================================
void NextionDriver::setupECGParametersPage(int hrMin, int hrMax, int hrCurrent,
                                            int ampCurrent, int noiseCurrent, int hrvCurrent) {
    // Estructura parametros_ecg (página 6 - popup sobre waveform_ecg):
    // Sliders: h_hr(ID4), h_amp(ID5), h_noise(ID6), h_hrv(ID7)
    // Valores: n_hr(ID12), n_amp(ID13), n_noise(ID14), n_hrv(ID15)
    // Escala: t_esc(ID18)
    
    char cmd[48];
    
    // Slider HR (ID 4): límites dinámicos según patología
    sprintf(cmd, "h_hr.minval=%d", hrMin);
    sendCommand(cmd);
    sprintf(cmd, "h_hr.maxval=%d", hrMax);
    sendCommand(cmd);
    sprintf(cmd, "h_hr.val=%d", hrCurrent);
    sendCommand(cmd);
    sprintf(cmd, "n_hr.val=%d", hrCurrent);   // ID 12: xfloat (vvs0=3, vvs1=0)
    sendCommand(cmd);
    
    // Slider Zoom (ID 5): factor de zoom visual (50-200%)
    // 100% = 0.2 mV/div (rango base 2.0 mV / 10 div)
    // 200% = 0.1 mV/div (zoom in), 50% = 0.4 mV/div (zoom out)
    sendCommand("h_amp.minval=50");
    sendCommand("h_amp.maxval=200");
    sprintf(cmd, "h_amp.val=%d", ampCurrent);
    sendCommand(cmd);
    sprintf(cmd, "n_amp.val=%d", ampCurrent); // ID 13: xfloat (vvs0=3, vvs1=0) muestra %
    sendCommand(cmd);
    
    // Actualizar texto de escala actual
    float zoomFactor = ampCurrent / 100.0f;
    float mvDiv = 0.2f / zoomFactor;  // 0.2 mV/div es la escala base
    sprintf(cmd, "t_esc.txt=\"%.2f mV/div\"", mvDiv);
    sendCommand(cmd);
    
    // Slider Ruido (ID 6): rango 0-100 (representa 0.00-1.00)
    sendCommand("h_noise.minval=0");
    sendCommand("h_noise.maxval=100");
    sprintf(cmd, "h_noise.val=%d", noiseCurrent);
    sendCommand(cmd);
    sprintf(cmd, "n_noise.val=%d", noiseCurrent); // ID 14: xfloat (vvs0=1, vvs1=2)
    sendCommand(cmd);
    
    // Slider HRV (ID 7): rango 0-15 (representa 0-15%)
    sendCommand("h_hrv.minval=0");
    sendCommand("h_hrv.maxval=15");
    sprintf(cmd, "h_hrv.val=%d", hrvCurrent);
    sendCommand(cmd);
    sprintf(cmd, "n_hrv.val=%d", hrvCurrent); // ID 15: xfloat (vvs0=2, vvs1=0)
    sendCommand(cmd);
}

// ============================================================================
// ACTUALIZAR ESCALA ECG (mV/div)
// ============================================================================
void NextionDriver::updateECGScale(int zoomPercent) {
    // Actualiza las etiquetas de escala en waveform_ecg y parametros_ecg
    // Rango base ECG: 2.0 mV total, 10 divisiones = 0.2 mV/div
    // zoomPercent: 50-200%, donde 100% = 0.2 mV/div
    
    char cmd[48];
    float zoomFactor = zoomPercent / 100.0f;
    float mvDiv = 0.2f / zoomFactor;  // 0.2 mV/div es la escala base
    
    // Actualizar en waveform_ecg (t_mvdiv ID 31)
    sprintf(cmd, "t_mvdiv.txt=\"%.2f mV/div\"", mvDiv);
    sendCommand(cmd);
    
    // Actualizar en parametros_ecg (t_esc ID 18) si está visible
    sprintf(cmd, "t_esc.txt=\"%.2f mV/div\"", mvDiv);
    sendCommand(cmd);
    
    Serial.printf("[ECG] Zoom: %d%%, Escala: %.2f mV/div\n", zoomPercent, mvDiv);
}

// ============================================================================
// CONFIGURAR PÁGINA PARÁMETROS EMG
// ============================================================================
void NextionDriver::setupEMGParametersPage(int excCurrent, int ampCurrent, int noiseCurrent) {
    // Estructura parametros_emg (página 8 - popup sobre waveform_emg):
    // Sliders: h_exc(ID4), h_amp(ID5), h_noise(ID6)
    // Valores: n_exc, n_amp, n_noise
    
    char cmd[48];
    
    // Slider Excitación (ID 4): rango 0-100 (representa 0-100%)
    sendCommand("h_exc.minval=0");
    sendCommand("h_exc.maxval=100");
    sprintf(cmd, "h_exc.val=%d", excCurrent);
    sendCommand(cmd);
    sprintf(cmd, "n_exc.val=%d", excCurrent);
    sendCommand(cmd);
    
    // Slider Zoom/Amplitud (ID 5): rango 50-200 (factor zoom visual %)
    sendCommand("h_amp.minval=50");
    sendCommand("h_amp.maxval=200");
    sprintf(cmd, "h_amp.val=%d", ampCurrent);
    sendCommand(cmd);
    sprintf(cmd, "n_amp.val=%d", ampCurrent);
    sendCommand(cmd);
    
    // Slider Ruido (ID 6): rango 0-100 (representa 0.00-1.00)
    sendCommand("h_noise.minval=0");
    sendCommand("h_noise.maxval=100");
    sprintf(cmd, "h_noise.val=%d", noiseCurrent);
    sendCommand(cmd);
    sprintf(cmd, "n_noise.val=%d", noiseCurrent);
    sendCommand(cmd);
}

// ============================================================================
// CONFIGURAR PÁGINA PARÁMETROS PPG
// ============================================================================
void NextionDriver::setupPPGParametersPage(int hrCurrent, int piCurrent, int noiseCurrent, int ampCurrent) {
    // Estructura parametros_ppg (página 10 - popup sobre waveform_ppg):
    // Sliders: h_hr(ID4), h_pi(ID5), h_noise(ID6), h_amp(ID14)
    // Valores: n_hr(ID10), n_pi(ID11), n_noise(ID12), n_amp(ID16)
    
    char cmd[48];
    
    // Slider HR (ID 4): rango 50-150 BPM
    sendCommand("h_hr.minval=50");
    sendCommand("h_hr.maxval=150");
    sprintf(cmd, "h_hr.val=%d", hrCurrent);
    sendCommand(cmd);
    sprintf(cmd, "n_hr.val=%d", hrCurrent);
    sendCommand(cmd);
    
    // Slider PI (ID 5): rango 3-200 (representa 0.3-20.0%)
    sendCommand("h_pi.minval=3");
    sendCommand("h_pi.maxval=200");
    sprintf(cmd, "h_pi.val=%d", piCurrent);
    sendCommand(cmd);
    sprintf(cmd, "n_pi.val=%d", piCurrent);
    sendCommand(cmd);
    
    // Slider Ruido (ID 6): rango 0-100 (representa 0.00-1.00)
    sendCommand("h_noise.minval=0");
    sendCommand("h_noise.maxval=100");
    sprintf(cmd, "h_noise.val=%d", noiseCurrent);
    sendCommand(cmd);
    sprintf(cmd, "n_noise.val=%d", noiseCurrent);
    sendCommand(cmd);
    
    // Slider Amplitud/Zoom (ID 14): rango 50-200 (factor zoom visual %)
    sendCommand("h_amp.minval=50");
    sendCommand("h_amp.maxval=200");
    sprintf(cmd, "h_amp.val=%d", ampCurrent);
    sendCommand(cmd);
    sprintf(cmd, "n_amp.val=%d", ampCurrent);
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
