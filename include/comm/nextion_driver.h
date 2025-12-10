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
    WAVEFORM_ECG = 5,   // Waveform ECG
    VALORES_ECG = 6,    // Popup valores ECG
    PARAMETROS_ECG = 7, // Popup parámetros ECG
    WAVEFORM_EMG = 8,   // Waveform EMG
    VALORES_EMG = 9,    // Popup valores EMG
    PARAMETROS_EMG = 10,// Popup parámetros EMG
    WAVEFORM_PPG = 11,  // Waveform PPG
    VALORES_PPG = 12,   // Popup valores PPG
    PARAMETROS_PPG = 13 // Popup parámetros PPG
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
    BUTTON_BACK_POPUP,      // Volver desde popup valores (solo cierra)
    BUTTON_APPLY_PARAMS,    // bt_act: Aplicar parámetros y cerrar
    BUTTON_CANCEL_PARAMS,   // bt_ex: Cancelar sin guardar
    BUTTON_RESET_PARAMS,    // bt_reset: Resetear a valores por defecto
    
    // Sliders ECG (parametros_ecg página 7)
    SLIDER_ECG_HR,          // Slider frecuencia cardíaca (ID 10)
    SLIDER_ECG_AMP,         // Slider amplitud QRS (ID 11)
    SLIDER_ECG_NOISE,       // Slider nivel de ruido (ID 12)
    SLIDER_ECG_HRV,         // Slider variabilidad HRV (ID 13)
    
    // Sliders EMG (parametros_emg página 10)
    SLIDER_EMG_EXC,         // Slider excitación (ID 8)
    SLIDER_EMG_AMP,         // Slider amplitud (ID 9)
    SLIDER_EMG_NOISE,       // Slider ruido (ID 10)
    
    // Sliders PPG (parametros_ppg página 13)
    SLIDER_PPG_HR,          // Slider frecuencia cardíaca (ID 8)
    SLIDER_PPG_PI,          // Slider índice de perfusión (ID 9)
    SLIDER_PPG_NOISE        // Slider ruido (ID 10)
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
    
    // Actualizar botones del menú (selección de señal)
    void updateMenuButtons(SignalType selected);
    
    // Actualizar botones de condiciones ECG (página ECG_SIM)
    void updateECGConditionButtons(int selectedCondition);

    // Actualizar botones de condiciones EMG (página EMG_SIM)
    void updateEMGConditionButtons(int selectedCondition);

    // Actualizar botones de condiciones PPG (página PPG_SIM)
    void updatePPGConditionButtons(int selectedCondition);
    
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
    
    // Actualizar valores en página valores_ecg
    void updateECGValuesPage(int bpm, int rr_ms, int rAmp_x100, int st_x100, 
                              uint32_t beats, const char* patologia);
    
    // Actualizar valores en página valores_emg
    void updateEMGValuesPage(int rms_x100, int activeUnits, int freq_x10, int contraction,
                              const char* condicion);
    
    // Actualizar valores en página valores_ppg
    void updatePPGValuesPage(int hr, int rr_ms, int pi_x10, int spo2,
                              uint32_t beats, const char* condicion);
    
    // Configurar página parametros_ecg con límites según patología
    void setupECGParametersPage(int hrMin, int hrMax, int hrCurrent,
                                 int ampCurrent, int noiseCurrent, int hrvCurrent);
    
    // Configurar página parametros_emg
    void setupEMGParametersPage(int excCurrent, int ampCurrent, int noiseCurrent);
    
    // Configurar página parametros_ppg
    void setupPPGParametersPage(int hrCurrent, int piCurrent, int noiseCurrent);
    
    // Leer valor de un slider (retorna -1 si error)
    int readSliderValue(const char* sliderName);
    
    // Callback
    void setEventCallback(UIEventCallback callback);
};

#endif // NEXTION_DRIVER_H
