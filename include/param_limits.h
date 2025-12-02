/*
 * ============================================================================
 * LÍMITES DE PARAMETRIZACIÓN POR CONDICIÓN
 * ============================================================================
 * 
 * JUSTIFICACIÓN CLÍNICA:
 * Los rangos están basados en valores fisiológicos reales para cada condición.
 * Esto evita que el usuario configure valores incoherentes (ej: ECG normal con
 * HR de taquicardia).
 * 
 * REFERENCIAS:
 * - AHA Guidelines for ECG Interpretation (2018)
 * - De Luca, C.J. (1997) - EMG frequency ranges
 * - Allen, J. (2007) - PPG physiological ranges
 * 
 * ============================================================================
 */

#ifndef PARAM_LIMITS_H
#define PARAM_LIMITS_H

#include "signal_types.h"

// ============================================================================
// ESTRUCTURA DE LÍMITES
// ============================================================================
struct ParameterLimits {
    float minValue;
    float maxValue;
    float defaultValue;
    const char* unit;
};

// ============================================================================
// LÍMITES ECG POR CONDICIÓN
// ============================================================================
struct ECGLimits {
    ParameterLimits heartRate;
    ParameterLimits qrsAmplitude;
    ParameterLimits pWaveAmplitude;
    ParameterLimits tWaveAmplitude;
    ParameterLimits stShift;
    ParameterLimits noiseLevel;
};

/*
 * RANGOS HR SEGÚN CONDICIÓN ECG:
 * ══════════════════════════════════════════════════════════════════════════
 * Condición              HR Min   HR Max   Justificación
 * ──────────────────────────────────────────────────────────────────────────
 * Normal                 60       100      Ritmo sinusal normal adulto
 * Taquicardia            100      220      Por definición >100 BPM
 * Bradicardia            30       59       Por definición <60 BPM
 * Fib. Auricular         60       180      Variable, típicamente irregular
 * Fib. Ventricular       150      500      Caótico, muy rápido
 * PVC                    50       120      Normal con ectópicos
 * Bloqueo Rama           40       100      Puede causar bradicardia
 * ST Elevación           50       120      Infarto - variable
 * ST Depresión           50       150      Isquemia - puede ser por ejercicio
 * ══════════════════════════════════════════════════════════════════════════
 */
inline ECGLimits getECGLimits(ECGCondition condition) {
    ECGLimits limits;
    
    // Valores comunes para todas las condiciones
    limits.qrsAmplitude = {0.5f, 2.0f, 1.0f, "x"};
    limits.pWaveAmplitude = {0.0f, 2.0f, 1.0f, "x"};
    limits.tWaveAmplitude = {0.5f, 2.0f, 1.0f, "x"};
    limits.noiseLevel = {0.0f, 0.5f, 0.05f, ""};  // Máx 50% ruido
    
    switch (condition) {
        case ECGCondition::NORMAL:
            limits.heartRate = {60.0f, 100.0f, 75.0f, "BPM"};
            limits.stShift = {-0.1f, 0.1f, 0.0f, "mV"};  // Mínimo shift en normal
            break;
            
        case ECGCondition::TACHYCARDIA:
            limits.heartRate = {100.0f, 220.0f, 130.0f, "BPM"};
            limits.stShift = {-0.5f, 0.5f, 0.0f, "mV"};
            break;
            
        case ECGCondition::BRADYCARDIA:
            limits.heartRate = {30.0f, 59.0f, 45.0f, "BPM"};
            limits.stShift = {-0.5f, 0.5f, 0.0f, "mV"};
            break;
            
        case ECGCondition::ATRIAL_FIBRILLATION:
            limits.heartRate = {60.0f, 180.0f, 110.0f, "BPM"};
            limits.pWaveAmplitude = {0.0f, 0.3f, 0.1f, "x"};  // P ausente o mínima
            limits.stShift = {-0.5f, 0.5f, 0.0f, "mV"};
            break;
            
        case ECGCondition::VENTRICULAR_FIBRILLATION:
            limits.heartRate = {150.0f, 500.0f, 300.0f, "BPM"};
            limits.qrsAmplitude = {0.2f, 1.5f, 0.8f, "x"};  // Variable
            limits.stShift = {-2.0f, 2.0f, 0.0f, "mV"};
            break;
            
        case ECGCondition::PREMATURE_VENTRICULAR:
            limits.heartRate = {50.0f, 120.0f, 75.0f, "BPM"};
            limits.stShift = {-1.0f, 1.0f, 0.0f, "mV"};
            break;
            
        case ECGCondition::BUNDLE_BRANCH_BLOCK:
            limits.heartRate = {40.0f, 100.0f, 70.0f, "BPM"};
            limits.qrsAmplitude = {1.0f, 2.5f, 1.5f, "x"};  // QRS ancho
            limits.stShift = {-1.0f, 1.0f, 0.0f, "mV"};
            break;
            
        case ECGCondition::ST_ELEVATION:
            limits.heartRate = {50.0f, 120.0f, 80.0f, "BPM"};
            limits.stShift = {0.1f, 2.0f, 0.5f, "mV"};  // Solo elevación
            break;
            
        case ECGCondition::ST_DEPRESSION:
            limits.heartRate = {50.0f, 150.0f, 90.0f, "BPM"};
            limits.stShift = {-2.0f, -0.1f, -0.5f, "mV"};  // Solo depresión
            break;
            
        default:
            limits.heartRate = {30.0f, 200.0f, 75.0f, "BPM"};
            limits.stShift = {-2.0f, 2.0f, 0.0f, "mV"};
    }
    
    return limits;
}

// ============================================================================
// LÍMITES EMG POR CONDICIÓN
// ============================================================================
struct EMGLimits {
    ParameterLimits amplitude;      // % de activación
    ParameterLimits frequency;      // Hz dominante
    ParameterLimits noiseLevel;
};

/*
 * RANGOS EMG SEGÚN CONDICIÓN:
 * ══════════════════════════════════════════════════════════════════════════
 * Condición              Amp Min  Amp Max  Freq Min  Freq Max
 * ──────────────────────────────────────────────────────────────────────────
 * Reposo                 0.0      0.1      20        80       Solo ruido
 * Leve 20%               0.1      0.3      30        100      Tareas finas
 * Moderada 50%           0.3      0.6      40        120      Sostener
 * Fuerte 80%             0.6      0.9      50        150      Levantar
 * Máxima 100%            0.8      1.0      60        150      Esfuerzo max
 * Temblor                0.1      0.5      4         8        Parkinson 4-6Hz
 * Miopatía               0.1      0.4      60        150      MUAPs cortos
 * Neuropatía             0.3      1.0      20        80       MUAPs gigantes
 * Fasciculación          0.0      0.3      -         -        Espontáneo
 * Fatiga                 0.2      0.8      30        100      Decae
 * ══════════════════════════════════════════════════════════════════════════
 */
inline EMGLimits getEMGLimits(EMGCondition condition) {
    EMGLimits limits;
    limits.noiseLevel = {0.0f, 0.3f, 0.1f, ""};
    
    switch (condition) {
        case EMGCondition::NORMAL:  // Reposo
            limits.amplitude = {0.0f, 0.1f, 0.05f, ""};
            limits.frequency = {20.0f, 80.0f, 50.0f, "Hz"};
            break;
            
        case EMGCondition::MILD_CONTRACTION:
            limits.amplitude = {0.1f, 0.3f, 0.2f, ""};
            limits.frequency = {30.0f, 100.0f, 60.0f, "Hz"};
            break;
            
        case EMGCondition::MODERATE_CONTRACTION:
            limits.amplitude = {0.3f, 0.6f, 0.5f, ""};
            limits.frequency = {40.0f, 120.0f, 80.0f, "Hz"};
            break;
            
        case EMGCondition::STRONG_CONTRACTION:
            limits.amplitude = {0.6f, 0.9f, 0.8f, ""};
            limits.frequency = {50.0f, 150.0f, 100.0f, "Hz"};
            break;
            
        case EMGCondition::MAXIMUM_CONTRACTION:
            limits.amplitude = {0.8f, 1.0f, 1.0f, ""};
            limits.frequency = {60.0f, 150.0f, 120.0f, "Hz"};
            break;
            
        case EMGCondition::TREMOR:
            limits.amplitude = {0.1f, 0.5f, 0.3f, ""};
            limits.frequency = {4.0f, 8.0f, 5.0f, "Hz"};  // Parkinson
            break;
            
        case EMGCondition::MYOPATHY:
            limits.amplitude = {0.1f, 0.4f, 0.2f, ""};
            limits.frequency = {60.0f, 150.0f, 100.0f, "Hz"};
            break;
            
        case EMGCondition::NEUROPATHY:
            limits.amplitude = {0.3f, 1.0f, 0.6f, ""};
            limits.frequency = {20.0f, 80.0f, 40.0f, "Hz"};
            break;
            
        case EMGCondition::FASCICULATION:
            limits.amplitude = {0.0f, 0.3f, 0.1f, ""};
            limits.frequency = {20.0f, 80.0f, 50.0f, "Hz"};
            break;
            
        case EMGCondition::FATIGUE:
            limits.amplitude = {0.2f, 0.8f, 0.5f, ""};
            limits.frequency = {30.0f, 100.0f, 60.0f, "Hz"};
            break;
            
        default:
            limits.amplitude = {0.0f, 1.0f, 0.5f, ""};
            limits.frequency = {10.0f, 150.0f, 50.0f, "Hz"};
    }
    
    return limits;
}

// ============================================================================
// LÍMITES PPG POR CONDICIÓN
// ============================================================================
struct PPGLimits {
    ParameterLimits heartRate;
    ParameterLimits amplitude;
    ParameterLimits dicroticNotch;
    ParameterLimits noiseLevel;
};

/*
 * RANGOS PPG SEGÚN CONDICIÓN:
 * ══════════════════════════════════════════════════════════════════════════
 * Condición              HR Min   HR Max   Amp Min  Amp Max  Dicrótica
 * ──────────────────────────────────────────────────────────────────────────
 * Normal                 60       100      0.8      1.2      0.2-0.4
 * Arritmia               50       150      0.5      1.0      0.1-0.5
 * Perfusión Débil        80       140      0.1      0.5      0.0-0.2
 * Perfusión Fuerte       50       90       1.0      2.0      0.3-0.6
 * Vasoconstricción       70       110      0.5      1.0      0.4-0.8
 * Artefacto Mov.         60       100      0.5      1.5      0.1-0.5
 * SpO2 Bajo              80       130      0.3      0.8      0.1-0.3
 * ══════════════════════════════════════════════════════════════════════════
 */
inline PPGLimits getPPGLimits(PPGCondition condition) {
    PPGLimits limits;
    limits.noiseLevel = {0.0f, 0.3f, 0.05f, ""};
    
    switch (condition) {
        case PPGCondition::NORMAL:
            limits.heartRate = {60.0f, 100.0f, 75.0f, "BPM"};
            limits.amplitude = {0.8f, 1.2f, 1.0f, ""};
            limits.dicroticNotch = {0.2f, 0.4f, 0.3f, ""};
            break;
            
        case PPGCondition::ARRHYTHMIA:
            limits.heartRate = {50.0f, 150.0f, 75.0f, "BPM"};
            limits.amplitude = {0.5f, 1.0f, 0.8f, ""};
            limits.dicroticNotch = {0.1f, 0.5f, 0.3f, ""};
            break;
            
        case PPGCondition::WEAK_PERFUSION:
            limits.heartRate = {80.0f, 140.0f, 110.0f, "BPM"};
            limits.amplitude = {0.1f, 0.5f, 0.3f, ""};
            limits.dicroticNotch = {0.0f, 0.2f, 0.1f, ""};
            break;
            
        case PPGCondition::STRONG_PERFUSION:
            limits.heartRate = {50.0f, 90.0f, 70.0f, "BPM"};
            limits.amplitude = {1.0f, 2.0f, 1.5f, ""};
            limits.dicroticNotch = {0.3f, 0.6f, 0.4f, ""};
            break;
            
        case PPGCondition::VASOCONSTRICTION:
            limits.heartRate = {70.0f, 110.0f, 85.0f, "BPM"};
            limits.amplitude = {0.5f, 1.0f, 0.7f, ""};
            limits.dicroticNotch = {0.4f, 0.8f, 0.6f, ""};
            break;
            
        case PPGCondition::MOTION_ARTIFACT:
            limits.heartRate = {60.0f, 100.0f, 75.0f, "BPM"};
            limits.amplitude = {0.5f, 1.5f, 1.0f, ""};
            limits.dicroticNotch = {0.1f, 0.5f, 0.3f, ""};
            limits.noiseLevel = {0.2f, 0.8f, 0.5f, ""};  // Más ruido
            break;
            
        case PPGCondition::LOW_SPO2:
            limits.heartRate = {80.0f, 130.0f, 100.0f, "BPM"};
            limits.amplitude = {0.3f, 0.8f, 0.5f, ""};
            limits.dicroticNotch = {0.1f, 0.3f, 0.2f, ""};
            break;
            
        default:
            limits.heartRate = {40.0f, 180.0f, 75.0f, "BPM"};
            limits.amplitude = {0.1f, 2.0f, 1.0f, ""};
            limits.dicroticNotch = {0.0f, 1.0f, 0.3f, ""};
    }
    
    return limits;
}

// ============================================================================
// FUNCIONES DE VALIDACIÓN
// ============================================================================

// Validar y limitar valor dentro de rango
inline float clampToLimits(float value, const ParameterLimits& limits) {
    if (value < limits.minValue) return limits.minValue;
    if (value > limits.maxValue) return limits.maxValue;
    return value;
}

// Verificar si valor está en rango
inline bool isInRange(float value, const ParameterLimits& limits) {
    return (value >= limits.minValue && value <= limits.maxValue);
}

// Obtener string descriptivo del rango
inline void getRangeString(char* buffer, size_t bufSize, const ParameterLimits& limits) {
    snprintf(buffer, bufSize, "%.1f-%.1f %s", 
             limits.minValue, limits.maxValue, limits.unit);
}

#endif // PARAM_LIMITS_H
