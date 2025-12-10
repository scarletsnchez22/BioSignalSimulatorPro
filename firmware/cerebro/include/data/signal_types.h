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
// CONDICIONES ECG (8 condiciones)
// ============================================================================
enum class ECGCondition : uint8_t {
    NORMAL = 0,                     // Ritmo sinusal normal
    TACHYCARDIA,                    // Taquicardia sinusal (>100 BPM)
    BRADYCARDIA,                    // Bradicardia sinusal (<60 BPM)
    ATRIAL_FIBRILLATION,            // Fibrilación auricular
    VENTRICULAR_FIBRILLATION,       // Fibrilación ventricular
    PREMATURE_VENTRICULAR,          // Contracciones ventriculares prematuras
    ST_ELEVATION,                   // Elevación ST (infarto STEMI)
    ST_DEPRESSION,                  // Depresión ST (isquemia)
    
    COUNT = 8                       // Total de condiciones
};

// ============================================================================
// CONDICIONES EMG (8 condiciones)
// Basado en De Luca CJ (2010), Fuglevand AJ (1993)
// ============================================================================
enum class EMGCondition : uint8_t {
    // Niveles de contracción (% MVC - Maximum Voluntary Contraction)
    REST = 0,                       // 0-10% MVC - Reposo/mínima actividad
    LOW_CONTRACTION,                // 10-30% MVC - Contracción baja
    MODERATE_CONTRACTION,           // 30-60% MVC - Contracción moderada
    HIGH_CONTRACTION,               // 60-100% MVC - Contracción alta/máxima
    
    // Patologías - Kimura J. "Electrodiagnosis" 4th ed. 2013
    TREMOR,                         // Temblor 4-6 Hz (Parkinson)
    MYOPATHY,                       // Miopatía (MUAPs pequeños, polifasicos)
    NEUROPATHY,                     // Neuropatía (MUAPs gigantes, reinervación)
    FASCICULATION,                  // Fasciculaciones espontáneas
    
    COUNT = 8                       // Total de condiciones
};

// ============================================================================
// CONDICIONES PPG (6 condiciones)
// Basado en Allen J. Physiol Meas. 2007;28(3):R1-R39
//          Lima A, Bakker J. Intensive Care Med. 2005;31(10):1316-1326
//          Jubran A. Crit Care. 2015;19:272
// 
// PI y SpO2 son valores dinámicos con variabilidad gaussiana natural.
// ============================================================================
enum class PPGCondition : uint8_t {
    NORMAL = 0,                     // PI 2-5%, SpO2 95-100%, muesca dicrótica visible
    ARRHYTHMIA,                     // PI 1-5%, SpO2 92-100%, RR muy variable (>15%)
    WEAK_PERFUSION,                 // PI 0.1-0.5%, SpO2 88-98%, taquicardia compensatoria
    STRONG_PERFUSION,               // PI 5-20%, SpO2 96-100%, vasodilatación
    VASOCONSTRICTION,               // PI 0.2-0.8%, SpO2 91-100%, onda aplanada, muesca atenuada
    LOW_SPO2,                       // PI 0.5-3.5%, SpO2 70-90%, hipoxemia causa pulmonar
    
    COUNT = 6                       // Total de condiciones
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
        case ECGCondition::ST_ELEVATION:            return "ST Elevado";
        case ECGCondition::ST_DEPRESSION:           return "ST Deprimido";
        default: return "Desconocido";
    }
}

inline const char* emgConditionToString(EMGCondition cond) {
    switch (cond) {
        case EMGCondition::REST:                 return "Reposo";
        case EMGCondition::LOW_CONTRACTION:      return "Baja";
        case EMGCondition::MODERATE_CONTRACTION: return "Moderada";
        case EMGCondition::HIGH_CONTRACTION:     return "Alta";
        case EMGCondition::TREMOR:               return "Temblor";
        case EMGCondition::MYOPATHY:             return "Miopatia";
        case EMGCondition::NEUROPATHY:           return "Neuropatia";
        case EMGCondition::FASCICULATION:        return "Fasciculacion";
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
        case PPGCondition::LOW_SPO2:         return "SpO2 Bajo";
        default: return "Desconocido";
    }
}

#endif // SIGNAL_TYPES_H
