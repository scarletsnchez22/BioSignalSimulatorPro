/**
 * @file param_limits.h
 * @brief Límites fisiológicos de parámetros por condición
 * @version 1.0.0
 * 
 * @details
 * Define rangos válidos para cada parámetro según la condición patológica.
 * 
 * USO PRINCIPAL:
 * - param_controller.cpp consulta estos límites para RESTRINGIR los valores
 *   que el usuario puede establecer según la condición actual.
 * - Ejemplo: Si está en NORMAL, heartRate se limita a 60-100 BPM.
 *   Si intenta poner 120, se clampea a 100.
 * 
 * BENEFICIO:
 * - Garantiza coherencia fisiológica: no se puede estar en "NORMAL" con
 *   parámetros que corresponden a otra patología.
 * - La UI (Nextion) puede consultar getCurrentXXXLimits() para mostrar
 *   el rango válido del slider según la condición actual.
 * 
 * Referencias: AHA Guidelines, Task Force 1996, De Luca 1997, Allen 2007
 */

#ifndef PARAM_LIMITS_H
#define PARAM_LIMITS_H

#include "signal_types.h"

// ============================================================================
// ESTRUCTURA DE LÍMITES
// ============================================================================
struct ParamRange {
    float min;
    float max;
    float defaultVal;
};

// ============================================================================
// LÍMITES ECG POR CONDICIÓN
// Referencias:
//   - AHA/ACC Guidelines for ECG Interpretation (2018)
//   - Task Force of ESC/NASPE. Heart rate variability (1996)
//   - Surawicz B. & Knilans T. Chou's Electrocardiography (8th ed, 2008)
//   - Goldberger AL. Clinical Electrocardiography (9th ed, 2017)
// 
// Nota: Los límites corresponden a los parámetros en ECGParameters:
// - heartRate, pWaveAmplitude, qrsAmplitude, tWaveAmplitude, stShift
// ============================================================================
struct ECGLimits {
    ParamRange heartRate;       // BPM
    ParamRange pAmplitude;      // Factor multiplicador (0.0-2.0)
    ParamRange qrsAmplitude;    // Factor multiplicador (0.5-2.0)
    ParamRange tAmplitude;      // Factor multiplicador (0.5-2.0)
    ParamRange stShift;         // mV (-0.3 a +0.3)
};

inline ECGLimits getECGLimits(ECGCondition condition) {
    ECGLimits limits;
    
    // Valores por defecto (normales) - Surawicz 2008
    limits.pAmplitude   = {0.5f, 2.0f, 1.0f};
    limits.qrsAmplitude = {0.5f, 2.0f, 1.0f};
    limits.tAmplitude   = {0.5f, 2.0f, 1.0f};
    
    switch (condition) {
        case ECGCondition::NORMAL:
            // AHA: ritmo sinusal normal 60-100 BPM
            limits.heartRate   = {60.0f, 100.0f, 75.0f};
            limits.stShift     = {-0.05f, 0.05f, 0.0f};  // <0.5mm es normal
            break;
            
        case ECGCondition::TACHYCARDIA:
            // AHA: taquicardia sinusal >100 BPM, típicamente 100-180
            limits.heartRate   = {100.0f, 180.0f, 130.0f};
            limits.stShift     = {-0.1f, 0.1f, 0.0f};
            break;
            
        case ECGCondition::BRADYCARDIA:
            // AHA: bradicardia sinusal <60 BPM
            limits.heartRate   = {30.0f, 59.0f, 45.0f};
            limits.stShift     = {-0.05f, 0.05f, 0.0f};
            break;
            
        case ECGCondition::ATRIAL_FIBRILLATION:
            // Goldberger 2017: AFib 60-180, irregular, sin onda P
            limits.heartRate   = {60.0f, 180.0f, 110.0f};
            limits.pAmplitude  = {0.0f, 0.0f, 0.0f};  // Ondas f reemplazan P
            limits.stShift     = {-0.1f, 0.1f, 0.0f};
            break;
            
        case ECGCondition::VENTRICULAR_FIBRILLATION:
            // Goldberger 2017: VFib 150-500, caótico, sin ondas definidas
            limits.heartRate   = {150.0f, 500.0f, 300.0f};
            limits.pAmplitude  = {0.0f, 0.0f, 0.0f};
            limits.qrsAmplitude = {0.0f, 0.0f, 0.0f};  // No hay QRS identificable
            limits.tAmplitude  = {0.0f, 0.0f, 0.0f};
            limits.stShift     = {-2.0f, 2.0f, 0.0f};  // Caótico
            break;
            
        case ECGCondition::PREMATURE_VENTRICULAR:
            // Surawicz 2008: PVCs tienen QRS ancho y grande, sin P precedente
            limits.heartRate   = {50.0f, 120.0f, 75.0f};
            limits.pAmplitude  = {0.0f, 0.0f, 0.0f};  // Sin P en el PVC
            limits.qrsAmplitude = {1.2f, 2.5f, 1.8f}; // QRS aumentado
            limits.tAmplitude  = {0.8f, 1.5f, 1.2f};  // T opuesta al QRS
            limits.stShift     = {-0.2f, 0.2f, 0.0f};
            break;
            
        case ECGCondition::ST_ELEVATION:
            // AHA 2018: STEMI requiere elevación ≥1mm (0.1mV)
            limits.heartRate   = {50.0f, 110.0f, 80.0f};
            limits.stShift     = {0.1f, 0.4f, 0.25f};  // ELEVACIÓN: 1-4mm
            limits.tAmplitude  = {1.0f, 2.0f, 1.5f};   // T hiperaguda
            break;
            
        case ECGCondition::ST_DEPRESSION:
            // AHA 2018: depresión ST ≥0.5mm sugiere isquemia
            limits.heartRate   = {50.0f, 150.0f, 90.0f};
            limits.stShift     = {-0.3f, -0.05f, -0.15f};  // DEPRESIÓN: 0.5-3mm
            limits.tAmplitude  = {0.3f, 0.8f, 0.5f};   // T invertida o aplanada
            break;
            
        default:
            limits.heartRate   = {60.0f, 100.0f, 75.0f};
            limits.stShift     = {-0.1f, 0.1f, 0.0f};
            break;
    }
    
    return limits;
}

// ============================================================================
// LÍMITES EMG POR CONDICIÓN
// Referencia: De Luca 1997, Henneman 1965, Fuglevand 1993
// ============================================================================
struct EMGLimits {
    ParamRange excitationLevel;  // 0.0-1.0 (% MVC)
    ParamRange amplitude;        // Factor multiplicador
};

inline EMGLimits getEMGLimits(EMGCondition condition) {
    EMGLimits limits;
    
    switch (condition) {
        case EMGCondition::REST:
            // 0-5% MVC - RMS <0.05 mV
            limits.excitationLevel = {0.0f, 0.05f, 0.02f};
            limits.amplitude       = {0.1f, 0.3f, 0.15f};
            break;
            
        case EMGCondition::LOW_CONTRACTION:
            // 5-20% MVC - RMS 0.1-0.4 mV
            limits.excitationLevel = {0.05f, 0.20f, 0.12f};
            limits.amplitude       = {0.3f, 0.8f, 0.5f};
            break;
            
        case EMGCondition::MODERATE_CONTRACTION:
            // 20-50% MVC - RMS 0.5-1.2 mV
            limits.excitationLevel = {0.20f, 0.50f, 0.35f};
            limits.amplitude       = {0.8f, 1.5f, 1.0f};
            break;
            
        case EMGCondition::HIGH_CONTRACTION:
            // 50-100% MVC - RMS 1.5-5.0 mV
            limits.excitationLevel = {0.50f, 1.0f, 0.75f};
            limits.amplitude       = {1.5f, 3.0f, 2.0f};
            break;
            
        case EMGCondition::TREMOR:
            limits.excitationLevel = {0.1f, 0.5f, 0.3f};
            limits.amplitude       = {0.5f, 1.5f, 1.0f};
            break;
            
        case EMGCondition::MYOPATHY:
            limits.excitationLevel = {0.1f, 0.4f, 0.3f};
            limits.amplitude       = {0.2f, 0.6f, 0.4f};  // Reducida
            break;
            
        case EMGCondition::NEUROPATHY:
            limits.excitationLevel = {0.3f, 1.0f, 0.5f};
            limits.amplitude       = {1.5f, 3.0f, 2.0f};  // Aumentada (MUAPs gigantes)
            break;
            
        case EMGCondition::FASCICULATION:
            limits.excitationLevel = {0.0f, 0.3f, 0.1f};
            limits.amplitude       = {0.5f, 1.5f, 1.0f};
            break;
            
        default:
            limits.excitationLevel = {0.0f, 1.0f, 0.5f};
            limits.amplitude       = {0.5f, 2.0f, 1.0f};
            break;
    }
    
    return limits;
}

// ============================================================================
// LÍMITES PPG POR CONDICIÓN
// Referencias:
//   - Allen J. (2007). Photoplethysmography and its application in clinical
//     physiological measurement. Physiological Measurement, 28(3), R1-R39.
//   - Elgendi M. (2012). On the analysis of fingertip photoplethysmogram signals.
//     Current Cardiology Reviews, 8(1), 14-25.
//   - Millasseau SC. (2002). Contour analysis of the photoplethysmographic pulse.
//     Journal of Hypertension, 20(12), 2407-2414.
//   - Reisner A. (2008). Utility of the photoplethysmogram in circulatory monitoring.
//     Anesthesiology, 108(5), 950-958.
//   - Shelley KH. (2007). Photoplethysmography: Beyond the calculation of arterial
//     oxygen saturation and heart rate. Anesthesia & Analgesia, 105(6), S31-S36.
// ============================================================================
struct PPGLimits {
    ParamRange heartRate;
    ParamRange perfusionIndex;   // PI en % (0.1-20%)
    ParamRange dicroticNotch;    // Prominencia de muesca dicrótica (0-1)
};

inline PPGLimits getPPGLimits(PPGCondition condition) {
    PPGLimits limits;
    
    switch (condition) {
        case PPGCondition::NORMAL:
            // Allen 2007: ritmo sinusal 60-100, PI 2-10%, notch 0.2-0.5
            limits.heartRate       = {60.0f, 100.0f, 75.0f};
            limits.perfusionIndex  = {2.0f, 10.0f, 5.0f};
            limits.dicroticNotch   = {0.2f, 0.5f, 0.35f};
            break;
            
        case PPGCondition::ARRHYTHMIA:
            // Elgendi 2012: variabilidad RR aumentada, notch variable
            limits.heartRate       = {50.0f, 150.0f, 75.0f};
            limits.perfusionIndex  = {2.0f, 8.0f, 4.0f};
            limits.dicroticNotch   = {0.1f, 0.4f, 0.25f};
            break;
            
        case PPGCondition::WEAK_PERFUSION:
            // Reisner 2008: hipovolemia reduce notch significativamente
            limits.heartRate       = {90.0f, 140.0f, 110.0f};
            limits.perfusionIndex  = {0.3f, 2.0f, 0.8f};
            limits.dicroticNotch   = {0.05f, 0.15f, 0.1f};  // Muy reducido
            break;
            
        case PPGCondition::STRONG_PERFUSION:
            // Millasseau 2002: vasodilatación aumenta notch
            limits.heartRate       = {50.0f, 90.0f, 70.0f};
            limits.perfusionIndex  = {10.0f, 20.0f, 12.0f};
            limits.dicroticNotch   = {0.4f, 0.7f, 0.55f};  // Prominente
            break;
            
        case PPGCondition::VASOCONSTRICTION:
            // Shelley 2007: vasoconstricción reduce amplitud y notch
            limits.heartRate       = {70.0f, 110.0f, 85.0f};
            limits.perfusionIndex  = {1.0f, 5.0f, 3.0f};
            limits.dicroticNotch   = {0.1f, 0.25f, 0.15f};  // Reducido
            break;
            
        case PPGCondition::LOW_SPO2:
            // Reisner 2008: hipoxia reduce perfusión periférica
            limits.heartRate       = {80.0f, 130.0f, 100.0f};
            limits.perfusionIndex  = {0.5f, 3.0f, 1.5f};
            limits.dicroticNotch   = {0.1f, 0.3f, 0.2f};   // Reducido
            break;
            
        default:
            limits.heartRate       = {60.0f, 100.0f, 75.0f};
            limits.perfusionIndex  = {2.0f, 10.0f, 5.0f};
            limits.dicroticNotch   = {0.2f, 0.5f, 0.35f};
            break;
    }
    
    return limits;
}

// ============================================================================
// LÍMITES GLOBALES [RESERVED - Para validación de entrada serial/UI]
// Constantes absolutas para validación independiente de condición
// ============================================================================
struct GlobalLimits {
    static constexpr float NOISE_MIN = 0.0f;
    static constexpr float NOISE_MAX = 1.0f;
    static constexpr float NOISE_DEFAULT = 0.05f;
    
    static constexpr float AMPLITUDE_MIN = 0.1f;
    static constexpr float AMPLITUDE_MAX = 2.0f;
    static constexpr float AMPLITUDE_DEFAULT = 1.0f;
    
    static constexpr float HR_ABSOLUTE_MIN = 30.0f;
    static constexpr float HR_ABSOLUTE_MAX = 200.0f;
};

#endif // PARAM_LIMITS_H
