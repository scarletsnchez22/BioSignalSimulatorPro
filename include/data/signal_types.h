/**
 * @file signal_types.h
 * @brief Definición de tipos de señales y sus estructuras
 * @version 1.0.0
 * 
 * Enumeraciones y estructuras para ECG, EMG y PPG.
 * Los rangos exactos por condición están en param_limits.h
 */

#ifndef SIGNAL_TYPES_H
#define SIGNAL_TYPES_H

#include <Arduino.h>

// ============================================================================
// TIPOS DE SEÑALES
// ============================================================================
enum class SignalType : uint8_t {
    NONE = 0,
    ECG  = 1,
    EMG  = 2,
    PPG  = 3
};

// ============================================================================
// ESTADOS DE SEÑAL
// ============================================================================
enum class SignalState : uint8_t {
    STOPPED  = 0,
    RUNNING  = 1,
    PAUSED   = 2,
    ERROR    = 3
};

// ============================================================================
// CONDICIONES ECG (9 condiciones)
// ============================================================================
enum class ECGCondition : uint8_t {
    NORMAL = 0,                     // Ritmo sinusal normal
    TACHYCARDIA,                    // Taquicardia sinusal (>100 BPM)
    BRADYCARDIA,                    // Bradicardia sinusal (<60 BPM)
    ATRIAL_FIBRILLATION,            // Fibrilación auricular
    VENTRICULAR_FIBRILLATION,       // Fibrilación ventricular
    PREMATURE_VENTRICULAR,          // Contracciones ventriculares prematuras
    BUNDLE_BRANCH_BLOCK,            // Bloqueo de rama
    ST_ELEVATION,                   // Elevación ST (infarto)
    ST_DEPRESSION,                  // Depresión ST (isquemia)
    
    COUNT = 9                       // Total de condiciones
};

// ============================================================================
// CONDICIONES EMG (10 condiciones)
// ============================================================================
enum class EMGCondition : uint8_t {
    // Niveles de contracción (% MVC)
    REST = 0,                       // Reposo (0% MVC)
    MILD_CONTRACTION,               // Leve (20% MVC)
    MODERATE_CONTRACTION,           // Moderada (50% MVC)
    STRONG_CONTRACTION,             // Fuerte (80% MVC)
    MAXIMUM_CONTRACTION,            // Máxima (100% MVC)
    
    // Patologías
    TREMOR,                         // Temblor 4-6 Hz (Parkinson)
    MYOPATHY,                       // Miopatía (MUAPs pequeños)
    NEUROPATHY,                     // Neuropatía (MUAPs gigantes)
    FASCICULATION,                  // Fasciculaciones
    FATIGUE,                        // Fatiga muscular
    
    COUNT = 10                      // Total de condiciones
};

// ============================================================================
// CONDICIONES PPG (7 condiciones)
// ============================================================================
enum class PPGCondition : uint8_t {
    NORMAL = 0,                     // Pulso normal
    ARRHYTHMIA,                     // Arritmia
    WEAK_PERFUSION,                 // Perfusión débil
    STRONG_PERFUSION,               // Perfusión fuerte
    VASOCONSTRICTION,               // Vasoconstricción
    MOTION_ARTIFACT,                // Artefacto por movimiento
    LOW_SPO2,                       // SpO2 bajo
    
    COUNT = 7                       // Total de condiciones
};

// ============================================================================
// PARÁMETROS ECG
// Rangos varían por condición - ver param_limits.h para detalles
// ============================================================================
struct ECGParameters {
    float heartRate;                // BPM (30-500, varía por condición)
    float pWaveAmplitude;           // Factor P (0.0-2.0, 0 en AFib/VFib/PVC)
    float qrsAmplitude;             // Factor QRS (0.5-2.5, varía por condición)
    float tWaveAmplitude;           // Factor T (0.3-2.0, varía por condición)
    float stShift;                  // Desplazamiento ST en mV (-0.3 a +0.4)
    float noiseLevel;               // Nivel de ruido (0.0-1.0)
    ECGCondition condition;         // Condición actual
    
    // Constructor con valores por defecto
    ECGParameters() :
        heartRate(75.0f),
        pWaveAmplitude(1.0f),
        qrsAmplitude(1.0f),
        tWaveAmplitude(1.0f),
        stShift(0.0f),
        noiseLevel(0.05f),
        condition(ECGCondition::NORMAL)
    {}
};

// ============================================================================
// PARÁMETROS EMG
// Rangos varían por condición - ver param_limits.h para detalles
// ============================================================================
struct EMGParameters {
    float excitationLevel;          // Nivel de excitación (0.0-1.0, varía por condición)
    float amplitude;                // Amplitud relativa (0.1-3.0, varía por condición)
    float noiseLevel;               // Nivel de ruido (0.0-1.0)
    EMGCondition condition;         // Condición actual
    
    // Constructor con valores por defecto
    EMGParameters() :
        excitationLevel(0.0f),
        amplitude(1.0f),
        noiseLevel(0.05f),
        condition(EMGCondition::REST)
    {}
};

// ============================================================================
// PARÁMETROS PPG
// Rangos varían por condición - ver param_limits.h para detalles
// ============================================================================
struct PPGParameters {
    float heartRate;                // BPM (50-150, varía por condición)
    float perfusionIndex;           // Índice de perfusión % (0.3-20.0, varía por condición)
    float dicroticNotch;            // Muesca dicrótica (0.0-0.7, varía por condición)
    float noiseLevel;               // Nivel de ruido (0.0-1.0)
    PPGCondition condition;         // Condición actual
    
    // Constructor con valores por defecto
    PPGParameters() :
        heartRate(75.0f),
        perfusionIndex(5.0f),
        dicroticNotch(0.3f),
        noiseLevel(0.05f),
        condition(PPGCondition::NORMAL)
    {}
};

// ============================================================================
// ESTRUCTURA DE DATOS DE SEÑAL
// ============================================================================
struct SignalData {
    SignalType type;
    SignalState state;
    uint32_t sampleCount;
    unsigned long lastUpdateTime;
    
    // Parámetros por tipo (sin union para evitar problemas con constructores)
    ECGParameters ecg;
    EMGParameters emg;
    PPGParameters ppg;
    
    SignalData() : 
        type(SignalType::NONE),
        state(SignalState::STOPPED),
        sampleCount(0),
        lastUpdateTime(0),
        ecg(),
        emg(),
        ppg()
    {}
};

// ============================================================================
// FUNCIONES DE CONVERSIÓN A STRING
// ============================================================================
inline const char* signalTypeToString(SignalType type) {
    switch (type) {
        case SignalType::ECG: return "ECG";
        case SignalType::EMG: return "EMG";
        case SignalType::PPG: return "PPG";
        default: return "NONE";
    }
}

inline const char* signalStateToString(SignalState state) {
    switch (state) {
        case SignalState::RUNNING: return "RUNNING";
        case SignalState::PAUSED:  return "PAUSED";
        case SignalState::STOPPED: return "STOPPED";
        case SignalState::ERROR:   return "ERROR";
        default: return "UNKNOWN";
    }
}

inline const char* ecgConditionToString(ECGCondition cond) {
    switch (cond) {
        case ECGCondition::NORMAL:                  return "Normal";
        case ECGCondition::TACHYCARDIA:             return "Taquicardia";
        case ECGCondition::BRADYCARDIA:             return "Bradicardia";
        case ECGCondition::ATRIAL_FIBRILLATION:     return "Fib. Auricular";
        case ECGCondition::VENTRICULAR_FIBRILLATION:return "Fib. Ventricular";
        case ECGCondition::PREMATURE_VENTRICULAR:   return "PVC";
        case ECGCondition::BUNDLE_BRANCH_BLOCK:     return "Bloq. Rama";
        case ECGCondition::ST_ELEVATION:            return "ST Elevado";
        case ECGCondition::ST_DEPRESSION:           return "ST Deprimido";
        default: return "Desconocido";
    }
}

inline const char* emgConditionToString(EMGCondition cond) {
    switch (cond) {
        case EMGCondition::REST:                 return "Reposo";
        case EMGCondition::MILD_CONTRACTION:     return "Leve";
        case EMGCondition::MODERATE_CONTRACTION: return "Moderada";
        case EMGCondition::STRONG_CONTRACTION:   return "Fuerte";
        case EMGCondition::MAXIMUM_CONTRACTION:  return "Maxima";
        case EMGCondition::TREMOR:               return "Temblor";
        case EMGCondition::MYOPATHY:             return "Miopatia";
        case EMGCondition::NEUROPATHY:           return "Neuropatia";
        case EMGCondition::FASCICULATION:        return "Fasciculacion";
        case EMGCondition::FATIGUE:              return "Fatiga";
        default: return "Desconocido";
    }
}

inline const char* ppgConditionToString(PPGCondition cond) {
    switch (cond) {
        case PPGCondition::NORMAL:           return "Normal";
        case PPGCondition::ARRHYTHMIA:       return "Arritmia";
        case PPGCondition::WEAK_PERFUSION:   return "Perfusion Baja";
        case PPGCondition::STRONG_PERFUSION: return "Perfusion Alta";
        case PPGCondition::VASOCONSTRICTION: return "Vasoconstriccion";
        case PPGCondition::MOTION_ARTIFACT:  return "Artefacto Mov.";
        case PPGCondition::LOW_SPO2:         return "SpO2 Bajo";
        default: return "Desconocido";
    }
}

#endif // SIGNAL_TYPES_H
