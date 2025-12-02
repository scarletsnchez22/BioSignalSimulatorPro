/*
 * ============================================================================
 * EMG Model - Modelo de Alta Fidelidad para ESP32
 * ============================================================================
 * 
 * REFERENCIAS CIENTÍFICAS:
 * [1] Fuglevand, A. J., Winter, D. A., & Patla, A. E. (1993). 
 *     "Models of recruitment and rate coding organization in motor-unit pools."
 *     Journal of Neurophysiology, 70(6), 2470-2488.
 * 
 * [2] Ma, S., et al. (2024). "NeuroMotion: Open-source simulator for 
 *     biophysically detailed motor unit activity." 
 *     PLOS Computational Biology, 20(7), e1012257.
 * 
 * [3] De Luca, C. J., & Hostage, E. C. (2010). "Relationship between 
 *     firing rate and recruitment threshold of motoneurons."
 *     Journal of Neurophysiology, 104(2), 1034-1046.
 * 
 * [4] Henneman, E. (1957). "Relation between size of neurons and their 
 *     susceptibility to discharge." Science, 126(3287), 1345-1347.
 *     (Size Principle - reclutamiento ordenado por tamaño)
 * 
 * SEÑAL DINÁMICA: Cada muestra es ÚNICA debido a:
 * - ISI (Inter-Spike Interval) con variabilidad gaussiana (CoV=20%)
 * - Cada MU dispara de forma asíncrona e independiente
 * - Desfase inicial aleatorio entre MUs
 * - La señal NUNCA se repite mientras esté corriendo
 * 
 * ============================================================================
 */

#ifndef EMG_MODEL_H
#define EMG_MODEL_H

#include <Arduino.h>
#include "signal_types.h"

// ============================================================================
// CONFIGURACIÓN DEL MODELO - VALORES JUSTIFICADOS
// ============================================================================

// Número de Motor Units - Ref [1]: Típico 100-300 para músculos pequeños
// 100 MUs es estándar en literatura para First Dorsal Interosseous
#define NUM_MOTOR_UNITS         100

// Samples por forma de onda MUAP (interpolación lineal para suavidad)
#define MUAP_WAVEFORM_SAMPLES   32

// ============================================================================
// PARÁMETROS DE RECLUTAMIENTO - Ref [1], [2], [3]
// ============================================================================

// Rango de reclutamiento (RR) - Ref [1]: 30 para músculos de mano
// RR = ratio entre umbral de última MU / primera MU
#define RR_RANGE                30.0f

// Última MU reclutada al 50% MVC - Ref [3]: típico para músculos pequeños
// En músculos grandes (cuádriceps) puede ser hasta 95% MVC
#define LAST_MU_THRESHOLD       0.5f

// Firing rates - Ref [1], [2]:
// - Mínimo: 8 pps (debajo de esto la fusión de twitches es incompleta)
// - Primera MU alcanza ~35 pps a 100% MVC
// - Última MU alcanza ~25 pps (reclutada tarde, menos tiempo para aumentar)
#define MIN_FIRING_RATE         8.0f        // pps (pulsos por segundo)
#define MAX_FIRING_FIRST        35.0f       // pps para MU #1
#define MAX_FIRING_LAST         25.0f       // pps para MU #100

// Rate Coding - Ref [1]: Aumento de ~3 pps por cada 10% de excitación
#define RATE_CODING_SLOPE       0.3f        // 0.3 pps por 1% excitación

// Variabilidad ISI - Ref [2]: CoV típico = 15-25%
// Esto es CRUCIAL para señal dinámica no repetitiva
#define ISI_COV                 0.20f       // 20% variabilidad

// ============================================================================
// PARÁMETROS MUAP (Motor Unit Action Potential) - Ref [1], [2]
// ============================================================================

// Duración MUAP - Ref [2]: 6-14 ms típico
// MUs pequeñas = duración corta, MUs grandes = duración larga
#define MIN_MUAP_DURATION_MS    6.0f        // ms (MU pequeña, tipo I)
#define MAX_MUAP_DURATION_MS    14.0f       // ms (MU grande, tipo II)

// Amplitud MUAP - Ref [2]: 0.05 - 3.0 mV según tamaño y profundidad
#define MIN_MUAP_AMPLITUDE      0.05f       // mV (MU pequeña/profunda)
#define MAX_MUAP_AMPLITUDE      2.5f        // mV (MU grande/superficial)

// ============================================================================
// ESTRUCTURA: Motor Unit
// ============================================================================
struct MotorUnit {
    // Propiedades de reclutamiento
    float recruitmentThreshold;     // 0-1 (normalizado a MVC)
    float minFiringRate;            // Hz
    float maxFiringRate;            // Hz
    
    // Propiedades MUAP
    float muapAmplitude;            // mV
    float muapDurationMs;           // ms
    int8_t muapWaveform[MUAP_WAVEFORM_SAMPLES];  // Forma normalizada (-127 a 127)
    
    // Estado de disparo
    bool active;
    float currentFiringRate;        // Hz actual
    float nextSpikeTime;            // Tiempo del próximo spike (s)
    float lastSpikeTime;            // Tiempo del último spike (s)
};

// ============================================================================
// ESTRUCTURA: Estado de fatiga
// ============================================================================
struct FatigueState {
    bool enabled;
    float level;                    // 0-1
    unsigned long startTime;
    float rateDecay;                // Factor de reducción de firing rate
    float amplitudeDecay;           // Factor de reducción de amplitud
};

// ============================================================================
// CLASE: EMGModel
// ============================================================================
class EMGModel {
private:
    // Array de Motor Units (memoria estática)
    MotorUnit motorUnits[NUM_MOTOR_UNITS];
    
    // Parámetros actuales
    EMGParameters params;
    
    // Estado del modelo
    float currentTime;
    float currentExcitation;        // Nivel de excitación actual (0-1)
    float targetExcitation;         // Objetivo de excitación
    float rampSpeed;                // Velocidad de transición
    
    // Estado de fatiga
    FatigueState fatigue;
    
    // Estadísticas
    uint32_t activeMUCount;
    float avgFiringRate;
    float rmsAccumulator;
    uint32_t sampleCount;
    
    // Generador aleatorio gaussiano
    bool hasSpareGaussian;
    float spareGaussian;
    
    // Métodos privados - Inicialización
    void initializeMotorUnits();
    void generateMUAPWaveform(MotorUnit* mu, uint8_t index);
    
    // Métodos privados - Generación
    float gaussianRandom(float mean, float stddev);
    void updateMotorUnitStates();
    float generateRawSample(float deltaTime);
    
    // Métodos privados - Patologías
    void applyMyopathy();
    void applyNeuropathy();
    void resetPathology();
    
    // Conversión
    uint8_t voltageToDACValue(float voltage);
    
public:
    EMGModel();
    
    // Configuración
    void setParameters(const EMGParameters& params);
    void reset();
    
    // Control de excitación
    void setExcitationLevel(float level);   // 0.0 - 1.0
    float getExcitationLevel() const { return currentExcitation; }
    
    // Generación de señal
    float generateSample(float deltaTime);
    uint8_t getDACValue(float deltaTime);
    
    // Fatiga
    void enableFatigue(bool enable);
    float getFatigueLevel() const { return fatigue.level; }
    
    // Estadísticas
    uint32_t getActiveMUCount() const { return activeMUCount; }
    float getAvgFiringRate() const { return avgFiringRate; }
    float getRMS();
};

#endif // EMG_MODEL_H
