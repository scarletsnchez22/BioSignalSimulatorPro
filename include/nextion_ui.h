/*
 * ============================================================================
 * NEXTION UI - Interfaz de Usuario para Pantalla Nextion NX4024T032
 * ============================================================================
 * 
 * FLUJO DE NAVEGACIÓN:
 * ════════════════════════════════════════════════════════════════════════════
 *                                                                              
 *   ┌─────────────────┐      ┌─────────────────┐      ┌─────────────────┐     
 *   │    SPLASH       │      │   MENÚ SEÑAL    │      │  MENÚ CONDICIÓN │     
 *   │                 │      │                 │      │                 │     
 *   │  BioSignal      │─────>│  ○ ECG          │─────>│  ○ Normal       │     
 *   │  Simulator Pro  │      │  ○ EMG          │      │  ○ Patología 1  │     
 *   │                 │      │  ○ PPG          │      │  ○ Patología 2  │     
 *   │  [COMENZAR]     │      │                 │      │  ...            │     
 *   └─────────────────┘      └─────────────────┘      └────────┬────────┘     
 *                                                              │              
 *                                                              v              
 *   ┌─────────────────────────────────────────────────────────────────────┐   
 *   │                      PANTALLA DE SIMULACIÓN                         │   
 *   │  ┌───────────────────────────────────────────────────────────────┐  │   
 *   │  │                                                               │  │   
 *   │  │                      GRÁFICA DE SEÑAL                         │  │   
 *   │  │                    (Waveform en tiempo real)                  │  │   
 *   │  │                                                               │  │   
 *   │  └───────────────────────────────────────────────────────────────┘  │   
 *   │  ┌────────┐                                                         │   
 *   │  │ HR: 75 │  [PAUSE]  [STOP]  [PARAMS]           Tipo: ECG Normal  │   
 *   │  │SpO2:97%│                                                         │   
 *   │  └────────┘                                                         │   
 *   └─────────────────────────────────────────────────────────────────────┘   
 *                                                                              
 * ════════════════════════════════════════════════════════════════════════════
 * 
 * PROTOCOLO NEXTION:
 * - Comunicación Serial @ 9600 baud (configurable)
 * - Comandos terminan con 0xFF 0xFF 0xFF
 * - Respuestas comienzan con código de evento
 * 
 * ============================================================================
 */

#ifndef NEXTION_UI_H
#define NEXTION_UI_H

#include <Arduino.h>
#include "config.h"
#include "signal_types.h"

// ============================================================================
// PÁGINAS DE LA INTERFAZ NEXTION
// ============================================================================
enum class NextionPage : uint8_t {
    SPLASH      = 0,    // Pantalla de bienvenida
    MENU_SIGNAL = 1,    // Selección tipo de señal
    MENU_ECG    = 2,    // Condiciones ECG
    MENU_EMG    = 3,    // Condiciones EMG
    MENU_PPG    = 4,    // Condiciones PPG
    SIMULATION  = 5,    // Pantalla de simulación con gráfica
    PARAMS      = 6     // Ajuste de parámetros
};

// ============================================================================
// EVENTOS DE UI
// ============================================================================
enum class UIEvent : uint8_t {
    NONE = 0,
    
    // Navegación
    BTN_BEGIN,          // Botón comenzar (splash)
    BTN_BACK,           // Botón volver
    
    // Selección de señal
    SELECT_ECG,
    SELECT_EMG,
    SELECT_PPG,
    
    // Control de simulación
    BTN_PLAY,           // Iniciar/reanudar
    BTN_PAUSE,          // Pausar
    BTN_STOP,           // Detener
    BTN_PARAMS,         // Abrir parámetros
    
    // Cambio de parámetros
    PARAM_HR_UP,        // Aumentar HR
    PARAM_HR_DOWN,      // Disminuir HR
    PARAM_AMP_UP,       // Aumentar amplitud
    PARAM_AMP_DOWN,     // Disminuir amplitud
    PARAM_NOISE_UP,     // Aumentar ruido
    PARAM_NOISE_DOWN,   // Disminuir ruido
    PARAM_APPLY,        // Aplicar cambios
    
    // Selección de condición (código + índice)
    SELECT_CONDITION    // + conditionIndex
};

// ============================================================================
// ESTRUCTURA: Métricas en tiempo real para mostrar
// ============================================================================
struct DisplayMetrics {
    // ECG
    float heartRate;        // BPM
    uint32_t beatCount;     // Contador de latidos
    float rrInterval;       // Intervalo RR (ms)
    
    // EMG
    float rmsValue;         // RMS en mV
    uint32_t activeMUs;     // Motor Units activas
    float avgFiringRate;    // Tasa de disparo promedio
    
    // PPG
    float spo2;             // Saturación O2 (%)
    float perfusionIndex;   // Índice de perfusión (%)
    float sdnn;             // HRV SDNN (ms)
    
    // General
    SignalType signalType;
    SignalState signalState;
    const char* conditionName;
};

// ============================================================================
// CLASE: NextionUI
// ============================================================================
class NextionUI {
private:
    HardwareSerial* serial;
    NextionPage currentPage;
    
    // Buffer para comandos
    static const uint8_t CMD_BUFFER_SIZE = 64;
    char cmdBuffer[CMD_BUFFER_SIZE];
    
    // Estado de UI
    SignalType selectedSignal;
    uint8_t selectedConditionIndex;
    bool isSimulating;
    
    // Callback para eventos
    void (*eventCallback)(UIEvent event, uint8_t param);
    
    // Métodos privados - Comunicación
    void sendCommand(const char* cmd);
    void sendCommandF(const char* format, ...);
    void endCommand();
    bool readResponse(uint8_t* buffer, uint8_t maxLen, uint16_t timeout);
    
    // Métodos privados - Páginas
    void showSplashPage();
    void showSignalMenuPage();
    void showConditionMenuPage(SignalType type);
    void showSimulationPage();
    void showParamsPage();
    
    // Métodos privados - Actualización de UI
    void updateWaveformPoint(uint8_t value);
    void updateMetricsDisplay(const DisplayMetrics& metrics);
    void updateButtonStates(SignalState state);
    
public:
    NextionUI();
    
    // Inicialización
    bool begin();
    
    // Navegación
    void goToPage(NextionPage page);
    NextionPage getCurrentPage() const { return currentPage; }
    
    // Configurar callback de eventos
    void setEventCallback(void (*callback)(UIEvent, uint8_t));
    
    // Procesar eventos entrantes (llamar en loop)
    void processEvents();
    
    // Actualizar visualización
    void updateWaveform(uint8_t dacValue);
    void updateMetrics(const DisplayMetrics& metrics);
    void showMessage(const char* title, const char* message);
    
    // Setters para estado
    void setSimulating(bool sim) { isSimulating = sim; }
    void setSelectedSignal(SignalType type) { selectedSignal = type; }
    
    // Construcción de menús dinámicos
    void populateConditionMenu(SignalType type);
    
    // Utilidades
    static const char* getConditionName(SignalType type, uint8_t index);
    static uint8_t getConditionCount(SignalType type);
};

// ============================================================================
// INSTANCIA GLOBAL
// ============================================================================
extern NextionUI nextion;

#endif // NEXTION_UI_H
