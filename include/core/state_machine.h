/**
 * @file state_machine.h
 * @brief Máquina de estados del sistema
 * @version 1.0.0
 * 
 * Gestiona las transiciones de estado de la aplicación.
 */

#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <Arduino.h>
#include "../data/signal_types.h"

// ============================================================================
// ESTADOS DEL SISTEMA
// ============================================================================
enum class SystemState : uint8_t {
    INIT,               // Inicializando
    IDLE,               // Esperando selección
    SIGNAL_SELECTED,    // Señal seleccionada, esperando condición
    SIMULATING,         // Generando señal
    PAUSED,             // Señal pausada
    PARAMETERS,         // En pantalla de parámetros
    ERROR               // Error del sistema
};

// ============================================================================
// EVENTOS DEL SISTEMA
// ============================================================================
enum class SystemEvent : uint8_t {
    INIT_COMPLETE,
    SELECT_ECG,
    SELECT_EMG,
    SELECT_PPG,
    SELECT_CONDITION,
    START_SIMULATION,
    PAUSE,
    RESUME,
    STOP,
    OPEN_PARAMS,
    APPLY_PARAMS,
    CANCEL_PARAMS,
    ERROR_OCCURRED,
    BACK
};

// ============================================================================
// CLASE StateMachine
// ============================================================================
class StateMachine {
private:
    SystemState currentState;
    SignalType selectedSignal;
    uint8_t selectedCondition;
    
    // Callback para notificar cambios
    void (*onStateChange)(SystemState oldState, SystemState newState);
    
public:
    StateMachine();
    
    // Procesar eventos
    void processEvent(SystemEvent event, uint8_t param = 0);
    
    // Getters
    SystemState getState() const { return currentState; }
    SignalType getSelectedSignal() const { return selectedSignal; }
    uint8_t getSelectedCondition() const { return selectedCondition; }
    
    // Callback
    void setStateChangeCallback(void (*callback)(SystemState, SystemState));
    
    // Debug
    const char* stateToString(SystemState state);
    const char* eventToString(SystemEvent event);
};

#endif // STATE_MACHINE_H
