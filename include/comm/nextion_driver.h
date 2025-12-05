/**
 * @file nextion_driver.h
 * @brief Driver para pantalla Nextion NX4024T032
 * @version 1.0.0
 * 
 * Comunicación serial con la pantalla Nextion.
 */

#ifndef NEXTION_DRIVER_H
#define NEXTION_DRIVER_H

#include <Arduino.h>
#include "../config.h"
#include "../data/signal_types.h"

// ============================================================================
// PÁGINAS NEXTION
// ============================================================================
enum class NextionPage : uint8_t {
    SPLASH = 0,
    SELECT_SIGNAL = 1,
    SELECT_CONDITION = 2,
    SIMULATION = 3,
    PARAMETERS = 4
};

// ============================================================================
// EVENTOS DE UI
// ============================================================================
enum class UIEvent : uint8_t {
    NONE = 0,
    BUTTON_ECG,
    BUTTON_EMG,
    BUTTON_PPG,
    BUTTON_CONDITION,       // param = índice condición
    BUTTON_START,
    BUTTON_PAUSE,
    BUTTON_STOP,
    BUTTON_PARAMS,
    BUTTON_APPLY,
    BUTTON_CANCEL,
    BUTTON_DEFAULTS,
    BUTTON_BACK,
    SLIDER_CHANGED          // param = slider ID
};

// ============================================================================
// ESTRUCTURA PARA MÉTRICAS EN DISPLAY
// ============================================================================
struct DisplayMetrics {
    // ECG
    float heartRate;
    float rrInterval;
    float qrsAmplitude;
    float stLevel;
    
    // EMG
    float excitationLevel;
    int activeMotorUnits;
    float rmsAmplitude;
    
    // PPG
    float perfusionIndex;
    
    // Común
    float noiseLevel;
    float amplitude;
};

// ============================================================================
// CALLBACK PARA EVENTOS
// ============================================================================
typedef void (*UIEventCallback)(UIEvent event, uint8_t param);

// ============================================================================
// CLASE NextionDriver
// ============================================================================
class NextionDriver {
private:
    HardwareSerial& serial;
    UIEventCallback eventCallback;
    
    // Buffer de recepción
    uint8_t rxBuffer[32];
    uint8_t rxIndex;
    
    // Estado
    NextionPage currentPage;
    SignalType displayedSignal;
    
    // Métodos privados
    void sendCommand(const char* cmd);
    void sendEndSequence();
    void parseEvent();
    
public:
    NextionDriver(HardwareSerial& serialPort);
    
    // Inicialización
    bool begin();
    
    // Procesar eventos entrantes (llamar en loop)
    void process();
    
    // Navegación
    void goToPage(NextionPage page);
    NextionPage getCurrentPage() const { return currentPage; }
    
    // Actualizar textos
    void setText(const char* component, const char* text);
    void setNumber(const char* component, int value);
    void setFloat(const char* component, float value, uint8_t decimals = 1);
    
    // Waveform
    void addWaveformPoint(uint8_t componentId, uint8_t channel, uint8_t value);
    void clearWaveform(uint8_t componentId, uint8_t channel);
    
    // Sliders
    void setSliderValue(const char* component, int value);
    void setSliderLimits(const char* component, int minVal, int maxVal);
    int getSliderValue(const char* component);
    
    // Visibilidad
    void setVisible(const char* component, bool visible);
    
    // Configurar condiciones dinámicas (página 2)
    void setupConditionPage(SignalType signalType);
    
    // Actualizar métricas en pantalla de simulación
    void updateMetrics(const DisplayMetrics& metrics, SignalType type);
    
    // Estado de simulación
    void setSimulationState(SignalState state);
    
    // Callback
    void setEventCallback(UIEventCallback callback);
};

#endif // NEXTION_DRIVER_H
