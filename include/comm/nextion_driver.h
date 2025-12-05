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
// PÁGINAS NEXTION (deben coincidir con el .HMI)
// ============================================================================
enum class NextionPage : uint8_t {
    PORTADA = 0,        // Splash/bienvenida
    MENU = 1,           // Selección de señal (ECG/EMG/PPG)
    ECG_SIM = 2,        // Selección condición ECG
    EMG_SIM = 3,        // Selección condición EMG
    PPG_SIM = 4,        // Selección condición PPG
    ECG_WAVE = 5,       // Waveform ECG + sliders + métricas
    EMG_WAVE = 6,       // Waveform EMG + sliders + métricas
    PPG_WAVE = 7        // Waveform PPG + sliders + métricas
};

// ============================================================================
// EVENTOS DE UI
// ============================================================================
enum class UIEvent : uint8_t {
    NONE = 0,
    
    // Portada
    BUTTON_COMENZAR,        // Botón "Comenzar" en portada
    
    // Menú selección de señal
    BUTTON_ECG,             // Selección ECG
    BUTTON_EMG,             // Selección EMG
    BUTTON_PPG,             // Selección PPG
    BUTTON_IR,              // Botón "Ir" (navegar a simulación)
    
    // Simulación (común a ECG/EMG/PPG)
    BUTTON_CONDITION,       // param = índice condición (0-9)
    BUTTON_START,           // Iniciar/Play simulación
    BUTTON_PAUSE,           // Pausar simulación
    BUTTON_STOP,            // Detener y volver a menú
    BUTTON_ATRAS,           // Regresar
    
    // Popups waveform
    BUTTON_VALORES,         // Mostrar popup valores actuales
    BUTTON_PARAMETROS,      // Mostrar popup parámetros
    
    // Sliders
    SLIDER_PARAM1,          // Slider 1 (HR/Excitation/HR)
    SLIDER_PARAM2,          // Slider 2 (Amplitude)
    SLIDER_PARAM3,          // Slider 3 (ST/Noise/Dicrotic)
    SLIDER_PARAM4           // Slider 4 (Noise)
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
