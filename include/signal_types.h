#ifndef SIGNAL_TYPES_H
#define SIGNAL_TYPES_H

#include <Arduino.h>

// ============= ENUMERACIONES =============

// Tipos de señales fisiológicas
enum class SignalType {
    NONE = 0,
    ECG  = 1,
    EMG  = 2,
    PPG  = 3
};

// Estados de las señales
enum class SignalState {
    STOPPED = 0,
    RUNNING = 1,
    PAUSED  = 2,
    ERROR   = 3
};

// ============= PATOLOGÍAS ECG =============
enum class ECGCondition {
    NORMAL = 0,              // Ritmo sinusal normal
    TACHYCARDIA,             // >100 BPM en reposo
    BRADYCARDIA,             // <60 BPM
    ATRIAL_FIBRILLATION,     // Fibrilación auricular (irregular, sin onda P)
    VENTRICULAR_FIBRILLATION,// Fibrilación ventricular (caótico)
    PREMATURE_VENTRICULAR,   // Contracciones ventriculares prematuras (PVC)
    BUNDLE_BRANCH_BLOCK,     // Bloqueo de rama (QRS ancho)
    ST_ELEVATION,            // Elevación del segmento ST (infarto)
    ST_DEPRESSION            // Depresión del segmento ST (isquemia)
};

// ============= PATOLOGÍAS EMG =============
enum class EMGCondition {
    // CONTRACCIONES NORMALES (% MVC - Maximum Voluntary Contraction)
    NORMAL = 0,              // Reposo (0% MVC) - Solo ruido de fondo
    MILD_CONTRACTION,        // Leve (20% MVC) - Tareas de precisión
    MODERATE_CONTRACTION,    // Moderada (50% MVC) - Sostener objetos
    STRONG_CONTRACTION,      // Fuerte (80% MVC) - Levantar peso
    MAXIMUM_CONTRACTION,     // Máxima (100% MVC) - Esfuerzo máximo
    
    // PATOLOGÍAS
    TREMOR,                  // Temblor 4-6 Hz (Parkinson)
    MYOPATHY,                // Miopatía (MUAPs pequeños/cortos)
    NEUROPATHY,              // Neuropatía (MUAPs gigantes)
    FASCICULATION,           // Fasciculaciones (disparos espontáneos)
    FATIGUE                  // Fatiga muscular progresiva
};

// ============= PATOLOGÍAS PPG =============
enum class PPGCondition {
    NORMAL = 0,              // Pulso normal
    ARRHYTHMIA,              // Arritmia (irregular)
    WEAK_PERFUSION,          // Mala perfusión (amplitud baja)
    STRONG_PERFUSION,        // Buena perfusión (amplitud alta)
    VASOCONSTRICTION,        // Vasoconstricción (onda dicrótica prominente)
    MOTION_ARTIFACT,         // Artefacto por movimiento
    LOW_SPO2                 // Simulación de baja saturación
};

// ============= ESTRUCTURA DE PARÁMETROS ECG =============
struct ECGParameters {
    float heartRate;         // BPM (30-200)
    float pWaveAmplitude;    // Factor multiplicador onda P (0.5-2.0)
    float qrsAmplitude;      // Factor multiplicador QRS (0.5-2.0)
    float tWaveAmplitude;    // Factor multiplicador onda T (0.5-2.0)
    float prInterval;        // Intervalo PR en segundos (0.12-0.20)
    float qtInterval;        // Intervalo QT en segundos (0.35-0.45)
    float stSegmentShift;    // Desplazamiento ST en mV (-2.0 a +2.0)
    float noiseLevel;        // Nivel de ruido (0.0-1.0)
    ECGCondition condition;  // Condición patológica
    bool leadII;             // true = Lead II, false = Lead I
    
    // Constructor con valores por defecto (ritmo sinusal normal)
    ECGParameters() :
        heartRate(75.0f),
        pWaveAmplitude(1.0f),
        qrsAmplitude(1.0f),
        tWaveAmplitude(1.0f),
        prInterval(0.16f),
        qtInterval(0.40f),
        stSegmentShift(0.0f),
        noiseLevel(0.05f),
        condition(ECGCondition::NORMAL),
        leadII(true)
    {}
};

// ============= ESTRUCTURA DE PARÁMETROS EMG =============
struct EMGParameters {
    float frequency;         // Frecuencia dominante en Hz (10-150)
    float amplitude;         // Amplitud relativa (0.0-1.0)
    float noiseLevel;        // Nivel de ruido (0.0-1.0)
    float burstFrequency;    // Frecuencia de ráfagas en Hz (para contracciones)
    float burstDuration;     // Duración de ráfaga en segundos
    EMGCondition condition;  // Condición patológica
    
    // Constructor con valores por defecto (músculo relajado)
    EMGParameters() :
        frequency(50.0f),
        amplitude(0.1f),
        noiseLevel(0.8f),
        burstFrequency(0.0f),
        burstDuration(0.0f),
        condition(EMGCondition::NORMAL)
    {}
};

// ============= ESTRUCTURA DE PARÁMETROS PPG =============
struct PPGParameters {
    float heartRate;         // BPM (40-180)
    float amplitude;         // Amplitud del pulso (0.5-2.0)
    float dicroticNotch;     // Prominencia de muesca dicrótica (0.0-1.0)
    float dcComponent;       // Componente DC (offset) (50-200)
    float noiseLevel;        // Nivel de ruido (0.0-1.0)
    float irregularity;      // Irregularidad del ritmo (0.0-1.0, 0=regular)
    PPGCondition condition;  // Condición patológica
    
    // Constructor con valores por defecto (pulso normal)
    PPGParameters() :
        heartRate(75.0f),
        amplitude(1.0f),
        dicroticNotch(0.3f),
        dcComponent(128.0f),
        noiseLevel(0.05f),
        irregularity(0.0f),
        condition(PPGCondition::NORMAL)
    {}
};

// ============= ESTRUCTURA DE PARÁMETROS UNIFICADA =============
// Nota: No usamos union porque C++ no permite constructores no triviales en unions
struct SignalParams {
    ECGParameters ecg;
    EMGParameters emg;
    PPGParameters ppg;
    
    SignalParams() : ecg(), emg(), ppg() {}
};

// ============= ESTRUCTURA UNIFICADA DE SEÑAL =============
struct SignalData {
    SignalType type;
    SignalState state;
    SignalParams params;
    
    unsigned long lastUpdateTime;
    uint32_t sampleCount;
    
    SignalData() : 
        type(SignalType::NONE),
        state(SignalState::STOPPED),
        params(),
        lastUpdateTime(0),
        sampleCount(0)
    {}
};

// ============= FUNCIONES AUXILIARES =============

// Convertir enum a string para debug
inline const char* signalTypeToString(SignalType type) {
    switch(type) {
        case SignalType::ECG: return "ECG";
        case SignalType::EMG: return "EMG";
        case SignalType::PPG: return "PPG";
        default: return "NONE";
    }
}

inline const char* signalStateToString(SignalState state) {
    switch(state) {
        case SignalState::RUNNING: return "RUNNING";
        case SignalState::PAUSED: return "PAUSED";
        case SignalState::STOPPED: return "STOPPED";
        case SignalState::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

inline const char* ecgConditionToString(ECGCondition condition) {
    switch(condition) {
        case ECGCondition::NORMAL: return "Normal";
        case ECGCondition::TACHYCARDIA: return "Taquicardia";
        case ECGCondition::BRADYCARDIA: return "Bradicardia";
        case ECGCondition::ATRIAL_FIBRILLATION: return "Fib. Auricular";
        case ECGCondition::VENTRICULAR_FIBRILLATION: return "Fib. Ventricular";
        case ECGCondition::PREMATURE_VENTRICULAR: return "PVC";
        case ECGCondition::BUNDLE_BRANCH_BLOCK: return "Bloqueo Rama";
        case ECGCondition::ST_ELEVATION: return "Elevacion ST";
        case ECGCondition::ST_DEPRESSION: return "Depresion ST";
        default: return "Desconocido";
    }
}

inline const char* emgConditionToString(EMGCondition condition) {
    switch(condition) {
        case EMGCondition::NORMAL: return "Reposo";
        case EMGCondition::MILD_CONTRACTION: return "Leve 20%";
        case EMGCondition::MODERATE_CONTRACTION: return "Moderada 50%";
        case EMGCondition::STRONG_CONTRACTION: return "Fuerte 80%";
        case EMGCondition::MAXIMUM_CONTRACTION: return "Maxima 100%";
        case EMGCondition::TREMOR: return "Temblor";
        case EMGCondition::MYOPATHY: return "Miopatia";
        case EMGCondition::NEUROPATHY: return "Neuropatia";
        case EMGCondition::FASCICULATION: return "Fasciculacion";
        case EMGCondition::FATIGUE: return "Fatiga";
        default: return "Desconocido";
    }
}

inline const char* ppgConditionToString(PPGCondition condition) {
    switch(condition) {
        case PPGCondition::NORMAL: return "Normal";
        case PPGCondition::ARRHYTHMIA: return "Arritmia";
        case PPGCondition::WEAK_PERFUSION: return "Perfusion Debil";
        case PPGCondition::STRONG_PERFUSION: return "Perfusion Fuerte";
        case PPGCondition::VASOCONSTRICTION: return "Vasoconstriccion";
        case PPGCondition::MOTION_ARTIFACT: return "Artefacto Mov.";
        case PPGCondition::LOW_SPO2: return "SpO2 Bajo";
        default: return "Desconocido";
    }
}

#endif // SIGNAL_TYPES_H