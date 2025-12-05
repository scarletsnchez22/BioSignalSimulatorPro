/**
 * @file state_machine.cpp
 * @brief Implementación de la máquina de estados
 * @version 1.0.0
 */

#include "core/state_machine.h"

// ============================================================================
// CONSTRUCTOR
// ============================================================================
StateMachine::StateMachine() {
    currentState = SystemState::INIT;
    selectedSignal = SignalType::NONE;
    selectedCondition = 0;
    onStateChange = nullptr;
}

// ============================================================================
// PROCESAR EVENTOS
// ============================================================================
void StateMachine::processEvent(SystemEvent event, uint8_t param) {
    SystemState oldState = currentState;
    SystemState newState = currentState;
    
    switch (currentState) {
        case SystemState::INIT:
            if (event == SystemEvent::INIT_COMPLETE) {
                newState = SystemState::IDLE;
            }
            break;
            
        case SystemState::IDLE:
            switch (event) {
                case SystemEvent::SELECT_ECG:
                    selectedSignal = SignalType::ECG;
                    newState = SystemState::SIGNAL_SELECTED;
                    break;
                case SystemEvent::SELECT_EMG:
                    selectedSignal = SignalType::EMG;
                    newState = SystemState::SIGNAL_SELECTED;
                    break;
                case SystemEvent::SELECT_PPG:
                    selectedSignal = SignalType::PPG;
                    newState = SystemState::SIGNAL_SELECTED;
                    break;
                default:
                    break;
            }
            break;
            
        case SystemState::SIGNAL_SELECTED:
            switch (event) {
                case SystemEvent::SELECT_CONDITION:
                    selectedCondition = param;
                    newState = SystemState::SIMULATING;
                    break;
                case SystemEvent::BACK:
                    selectedSignal = SignalType::NONE;
                    newState = SystemState::IDLE;
                    break;
                default:
                    break;
            }
            break;
            
        case SystemState::SIMULATING:
            switch (event) {
                case SystemEvent::PAUSE:
                    newState = SystemState::PAUSED;
                    break;
                case SystemEvent::STOP:
                    newState = SystemState::IDLE;
                    break;
                case SystemEvent::OPEN_PARAMS:
                    newState = SystemState::PARAMETERS;
                    break;
                case SystemEvent::BACK:
                    newState = SystemState::SIGNAL_SELECTED;
                    break;
                default:
                    break;
            }
            break;
            
        case SystemState::PAUSED:
            switch (event) {
                case SystemEvent::RESUME:
                    newState = SystemState::SIMULATING;
                    break;
                case SystemEvent::STOP:
                    newState = SystemState::IDLE;
                    break;
                case SystemEvent::OPEN_PARAMS:
                    newState = SystemState::PARAMETERS;
                    break;
                default:
                    break;
            }
            break;
            
        case SystemState::PARAMETERS:
            switch (event) {
                case SystemEvent::APPLY_PARAMS:
                case SystemEvent::CANCEL_PARAMS:
                    newState = SystemState::SIMULATING;
                    break;
                default:
                    break;
            }
            break;
            
        case SystemState::ERROR:
            if (event == SystemEvent::INIT_COMPLETE) {
                newState = SystemState::IDLE;
            }
            break;
    }
    
    // Notificar cambio de estado
    if (newState != oldState) {
        currentState = newState;
        if (onStateChange != nullptr) {
            onStateChange(oldState, newState);
        }
    }
}

// ============================================================================
// CALLBACK
// ============================================================================
void StateMachine::setStateChangeCallback(void (*callback)(SystemState, SystemState)) {
    onStateChange = callback;
}

// ============================================================================
// CONVERSIÓN A STRING
// ============================================================================
const char* StateMachine::stateToString(SystemState state) {
    switch (state) {
        case SystemState::INIT:             return "INIT";
        case SystemState::IDLE:             return "IDLE";
        case SystemState::SIGNAL_SELECTED:  return "SIGNAL_SELECTED";
        case SystemState::SIMULATING:       return "SIMULATING";
        case SystemState::PAUSED:           return "PAUSED";
        case SystemState::PARAMETERS:       return "PARAMETERS";
        case SystemState::ERROR:            return "ERROR";
        default:                            return "UNKNOWN";
    }
}

const char* StateMachine::eventToString(SystemEvent event) {
    switch (event) {
        case SystemEvent::INIT_COMPLETE:        return "INIT_COMPLETE";
        case SystemEvent::SELECT_ECG:           return "SELECT_ECG";
        case SystemEvent::SELECT_EMG:           return "SELECT_EMG";
        case SystemEvent::SELECT_PPG:           return "SELECT_PPG";
        case SystemEvent::SELECT_CONDITION:     return "SELECT_CONDITION";
        case SystemEvent::START_SIMULATION:     return "START_SIMULATION";
        case SystemEvent::PAUSE:                return "PAUSE";
        case SystemEvent::RESUME:               return "RESUME";
        case SystemEvent::STOP:                 return "STOP";
        case SystemEvent::OPEN_PARAMS:          return "OPEN_PARAMS";
        case SystemEvent::APPLY_PARAMS:         return "APPLY_PARAMS";
        case SystemEvent::CANCEL_PARAMS:        return "CANCEL_PARAMS";
        case SystemEvent::ERROR_OCCURRED:       return "ERROR_OCCURRED";
        case SystemEvent::BACK:                 return "BACK";
        default:                                return "UNKNOWN";
    }
}
