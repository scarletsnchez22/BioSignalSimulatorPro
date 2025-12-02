/*
 * ============================================================================
 * NEXTION UI - Implementación de Interfaz de Usuario
 * ============================================================================
 */

#include "nextion_ui.h"
#include <stdarg.h>

// Instancia global
NextionUI nextion;

// Terminador de comando Nextion
static const uint8_t CMD_END[] = {0xFF, 0xFF, 0xFF};

// ============================================================================
// CONSTRUCTOR
// ============================================================================
NextionUI::NextionUI() {
    serial = nullptr;
    currentPage = NextionPage::SPLASH;
    selectedSignal = SignalType::NONE;
    selectedConditionIndex = 0;
    isSimulating = false;
    eventCallback = nullptr;
}

// ============================================================================
// INICIALIZACIÓN
// ============================================================================
bool NextionUI::begin() {
    serial = &NEXTION_SERIAL;
    serial->begin(NEXTION_BAUD, SERIAL_8N1, NEXTION_RX_PIN, NEXTION_TX_PIN);
    
    delay(100);  // Esperar inicialización de Nextion
    
    // Limpiar buffer
    while (serial->available()) {
        serial->read();
    }
    
    // Enviar reset
    sendCommand("rest");
    delay(500);
    
    // Configurar brillo al 100%
    sendCommand("dim=100");
    
    // Ir a página splash
    goToPage(NextionPage::SPLASH);
    
    DEBUG_PRINTLN("[NextionUI] Inicializado");
    return true;
}

// ============================================================================
// COMUNICACIÓN CON NEXTION
// ============================================================================
void NextionUI::sendCommand(const char* cmd) {
    if (serial == nullptr) return;
    serial->print(cmd);
    endCommand();
}

void NextionUI::sendCommandF(const char* format, ...) {
    if (serial == nullptr) return;
    
    va_list args;
    va_start(args, format);
    vsnprintf(cmdBuffer, CMD_BUFFER_SIZE, format, args);
    va_end(args);
    
    serial->print(cmdBuffer);
    endCommand();
}

void NextionUI::endCommand() {
    if (serial == nullptr) return;
    serial->write(CMD_END, 3);
}

bool NextionUI::readResponse(uint8_t* buffer, uint8_t maxLen, uint16_t timeout) {
    if (serial == nullptr) return false;
    
    unsigned long start = millis();
    uint8_t index = 0;
    uint8_t endCount = 0;
    
    while (millis() - start < timeout && index < maxLen) {
        if (serial->available()) {
            uint8_t b = serial->read();
            buffer[index++] = b;
            
            if (b == 0xFF) {
                endCount++;
                if (endCount >= 3) return true;
            } else {
                endCount = 0;
            }
        }
    }
    return false;
}

// ============================================================================
// NAVEGACIÓN DE PÁGINAS
// ============================================================================
void NextionUI::goToPage(NextionPage page) {
    currentPage = page;
    sendCommandF("page %d", (int)page);
    
    // Inicializar página según tipo
    switch (page) {
        case NextionPage::SPLASH:
            showSplashPage();
            break;
        case NextionPage::MENU_SIGNAL:
            showSignalMenuPage();
            break;
        case NextionPage::MENU_ECG:
            showConditionMenuPage(SignalType::ECG);
            break;
        case NextionPage::MENU_EMG:
            showConditionMenuPage(SignalType::EMG);
            break;
        case NextionPage::MENU_PPG:
            showConditionMenuPage(SignalType::PPG);
            break;
        case NextionPage::SIMULATION:
            showSimulationPage();
            break;
        case NextionPage::PARAMS:
            showParamsPage();
            break;
    }
}

// ============================================================================
// PÁGINAS INDIVIDUALES
// ============================================================================
void NextionUI::showSplashPage() {
    // Mostrar información del dispositivo
    sendCommandF("tTitle.txt=\"%s\"", DEVICE_NAME);
    sendCommandF("tVersion.txt=\"v%s\"", FIRMWARE_VERSION);
}

void NextionUI::showSignalMenuPage() {
    // Los botones ya están definidos en el HMI de Nextion
    // Solo actualizamos textos si es necesario
    sendCommand("tHeader.txt=\"Seleccione Tipo de Señal\"");
}

void NextionUI::showConditionMenuPage(SignalType type) {
    selectedSignal = type;
    
    // Título según tipo
    const char* title = "";
    switch (type) {
        case SignalType::ECG: title = "Condiciones ECG"; break;
        case SignalType::EMG: title = "Condiciones EMG"; break;
        case SignalType::PPG: title = "Condiciones PPG"; break;
        default: break;
    }
    sendCommandF("tHeader.txt=\"%s\"", title);
    
    // Popular lista de condiciones
    populateConditionMenu(type);
}

void NextionUI::showSimulationPage() {
    // Limpiar waveform
    sendCommand("cle 1,0");  // Limpiar canal 1 del waveform
    
    // Actualizar labels
    const char* typeName = signalTypeToString(selectedSignal);
    sendCommandF("tSignalType.txt=\"%s\"", typeName);
    
    // Mostrar nombre de condición
    const char* condName = getConditionName(selectedSignal, selectedConditionIndex);
    sendCommandF("tCondition.txt=\"%s\"", condName);
    
    // Estado inicial: detenido
    updateButtonStates(SignalState::STOPPED);
}

void NextionUI::showParamsPage() {
    // Mostrar parámetros actuales según tipo de señal
    sendCommandF("tParamTitle.txt=\"Parametros %s\"", 
                 signalTypeToString(selectedSignal));
}

// ============================================================================
// ACTUALIZACIÓN DE WAVEFORM
// ============================================================================
void NextionUI::updateWaveform(uint8_t dacValue) {
    if (currentPage != NextionPage::SIMULATION) return;
    
    // Escalar valor DAC (0-255) a altura del waveform (típico 0-200)
    uint8_t scaledValue = (dacValue * 200) / 255;
    
    // Añadir punto al waveform (componente s0, canal 0)
    sendCommandF("add 1,0,%d", scaledValue);
}

void NextionUI::updateMetrics(const DisplayMetrics& metrics) {
    if (currentPage != NextionPage::SIMULATION) return;
    
    // Actualizar métricas según tipo de señal
    switch (metrics.signalType) {
        case SignalType::ECG:
            sendCommandF("tHR.txt=\"HR: %.0f\"", metrics.heartRate);
            sendCommandF("tBeats.txt=\"Latidos: %lu\"", metrics.beatCount);
            sendCommandF("tRR.txt=\"RR: %.0f ms\"", metrics.rrInterval);
            break;
            
        case SignalType::EMG:
            sendCommandF("tRMS.txt=\"RMS: %.2f mV\"", metrics.rmsValue);
            sendCommandF("tMUs.txt=\"MUs: %lu\"", metrics.activeMUs);
            sendCommandF("tFR.txt=\"FR: %.1f Hz\"", metrics.avgFiringRate);
            break;
            
        case SignalType::PPG:
            sendCommandF("tHR.txt=\"HR: %.0f\"", metrics.heartRate);
            sendCommandF("tSpO2.txt=\"SpO2: %.0f%%\"", metrics.spo2);
            sendCommandF("tPI.txt=\"PI: %.1f%%\"", metrics.perfusionIndex);
            break;
            
        default:
            break;
    }
    
    // Estado de señal
    const char* stateStr = signalStateToString(metrics.signalState);
    sendCommandF("tState.txt=\"%s\"", stateStr);
}

void NextionUI::updateButtonStates(SignalState state) {
    // Habilitar/deshabilitar botones según estado
    switch (state) {
        case SignalState::STOPPED:
            sendCommand("bPlay.pic=1");     // Play habilitado
            sendCommand("bPause.pic=4");    // Pause deshabilitado
            sendCommand("bStop.pic=6");     // Stop deshabilitado
            break;
            
        case SignalState::RUNNING:
            sendCommand("bPlay.pic=2");     // Play deshabilitado
            sendCommand("bPause.pic=3");    // Pause habilitado
            sendCommand("bStop.pic=5");     // Stop habilitado
            break;
            
        case SignalState::PAUSED:
            sendCommand("bPlay.pic=1");     // Play habilitado (resume)
            sendCommand("bPause.pic=4");    // Pause deshabilitado
            sendCommand("bStop.pic=5");     // Stop habilitado
            break;
            
        default:
            break;
    }
}

// ============================================================================
// PROCESAMIENTO DE EVENTOS
// ============================================================================
void NextionUI::processEvents() {
    if (serial == nullptr || !serial->available()) return;
    
    uint8_t buffer[16];
    if (!readResponse(buffer, 16, 50)) return;
    
    // Procesar evento según código
    uint8_t eventCode = buffer[0];
    
    // 0x65 = Touch Event
    if (eventCode == 0x65) {
        uint8_t pageId = buffer[1];
        uint8_t componentId = buffer[2];
        uint8_t event = buffer[3];  // 0=release, 1=press
        
        if (event != 0) return;  // Solo procesar release
        
        UIEvent uiEvent = UIEvent::NONE;
        uint8_t param = 0;
        
        // Mapear componentes a eventos según página
        switch ((NextionPage)pageId) {
            case NextionPage::SPLASH:
                if (componentId == 1) uiEvent = UIEvent::BTN_BEGIN;
                break;
                
            case NextionPage::MENU_SIGNAL:
                if (componentId == 1) uiEvent = UIEvent::SELECT_ECG;
                else if (componentId == 2) uiEvent = UIEvent::SELECT_EMG;
                else if (componentId == 3) uiEvent = UIEvent::SELECT_PPG;
                else if (componentId == 4) uiEvent = UIEvent::BTN_BACK;
                break;
                
            case NextionPage::MENU_ECG:
            case NextionPage::MENU_EMG:
            case NextionPage::MENU_PPG:
                if (componentId >= 1 && componentId <= 10) {
                    uiEvent = UIEvent::SELECT_CONDITION;
                    param = componentId - 1;  // Índice 0-based
                } else if (componentId == 11) {
                    uiEvent = UIEvent::BTN_BACK;
                }
                break;
                
            case NextionPage::SIMULATION:
                if (componentId == 1) uiEvent = UIEvent::BTN_PLAY;
                else if (componentId == 2) uiEvent = UIEvent::BTN_PAUSE;
                else if (componentId == 3) uiEvent = UIEvent::BTN_STOP;
                else if (componentId == 4) uiEvent = UIEvent::BTN_PARAMS;
                break;
                
            case NextionPage::PARAMS:
                if (componentId == 1) uiEvent = UIEvent::PARAM_HR_DOWN;
                else if (componentId == 2) uiEvent = UIEvent::PARAM_HR_UP;
                else if (componentId == 3) uiEvent = UIEvent::PARAM_AMP_DOWN;
                else if (componentId == 4) uiEvent = UIEvent::PARAM_AMP_UP;
                else if (componentId == 5) uiEvent = UIEvent::PARAM_NOISE_DOWN;
                else if (componentId == 6) uiEvent = UIEvent::PARAM_NOISE_UP;
                else if (componentId == 7) uiEvent = UIEvent::PARAM_APPLY;
                else if (componentId == 8) uiEvent = UIEvent::BTN_BACK;
                break;
                
            default:
                break;
        }
        
        // Llamar callback si hay evento válido
        if (uiEvent != UIEvent::NONE && eventCallback != nullptr) {
            eventCallback(uiEvent, param);
        }
    }
}

void NextionUI::setEventCallback(void (*callback)(UIEvent, uint8_t)) {
    eventCallback = callback;
}

// ============================================================================
// UTILIDADES
// ============================================================================
void NextionUI::showMessage(const char* title, const char* message) {
    // Mostrar mensaje en popup (si el HMI lo soporta)
    sendCommandF("tMsgTitle.txt=\"%s\"", title);
    sendCommandF("tMsgBody.txt=\"%s\"", message);
    sendCommand("vis pMessage,1");  // Hacer visible el popup
}

void NextionUI::populateConditionMenu(SignalType type) {
    uint8_t count = getConditionCount(type);
    
    for (uint8_t i = 0; i < count && i < 10; i++) {
        const char* name = getConditionName(type, i);
        sendCommandF("b%d.txt=\"%s\"", i, name);
        sendCommandF("vis b%d,1", i);  // Hacer visible
    }
    
    // Ocultar botones no usados
    for (uint8_t i = count; i < 10; i++) {
        sendCommandF("vis b%d,0", i);
    }
}

uint8_t NextionUI::getConditionCount(SignalType type) {
    switch (type) {
        case SignalType::ECG: return 9;  // Normal + 8 patologías
        case SignalType::EMG: return 10; // 5 contracciones + 5 patologías
        case SignalType::PPG: return 7;  // Normal + 6 condiciones
        default: return 0;
    }
}

const char* NextionUI::getConditionName(SignalType type, uint8_t index) {
    switch (type) {
        case SignalType::ECG:
            return ecgConditionToString((ECGCondition)index);
        case SignalType::EMG:
            return emgConditionToString((EMGCondition)index);
        case SignalType::PPG:
            return ppgConditionToString((PPGCondition)index);
        default:
            return "Desconocido";
    }
}
