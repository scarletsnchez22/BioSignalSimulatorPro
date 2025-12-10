/**
 * @file emg_model.h
 * @brief Modelo EMG basado en reclutamiento de unidades motoras
 * @version 1.0.0
 * 
 * REFERENCIAS CIENTÍFICAS:
 * 
 * [1] Fuglevand AJ, Winter DA, Patla AE.
 *     "Models of recruitment and rate coding organization in motor-unit pools."
 *     J Neurophysiol. 1993;70(6):2470-2488.
 * 
 * [2] Henneman E, Somjen G, Carpenter DO.
 *     "Functional significance of cell size in spinal motoneurons."
 *     J Neurophysiol. 1965;28:560-580.
 * 
 * [3] De Luca CJ, Hostage EC.
 *     "Relationship between firing rate and recruitment threshold."
 *     J Neurophysiol. 2010;104(2):1034-1046.
 */

#ifndef EMG_MODEL_H
#define EMG_MODEL_H

#include <Arduino.h>
#include "../data/signal_types.h"

// ============================================================================
// CONSTANTES DEL MODELO
// ============================================================================
#define MAX_MOTOR_UNITS     100     // Número de unidades motoras en el pool
#define RMS_BUFFER_SIZE     100     // Ventana de 100ms para cálculo RMS @ 1kHz

// Constantes de escalado para waveform
#define EMG_MODEL_MIN      -5.0f    // Mínimo teórico (mV)
#define EMG_MODEL_MAX       5.0f    // Máximo teórico (mV)
#define EMG_MODEL_RANGE    10.0f    // Rango total (±5 mV)
#define EMG_WAVEFORM_GAIN_DEFAULT  3.0f  // Ganancia default para waveform

// ============================================================================
// ESTRUCTURA DE UNIDAD MOTORA
// ============================================================================
/**
 * @brief Representa una unidad motora individual
 * 
 * Una unidad motora = 1 motoneurona + todas las fibras musculares que inerva
 */
struct MotorUnit {
    float threshold;            // Umbral de reclutamiento (0-1)
    float amplitude;            // Amplitud del MUAP (mV)
    float baseAmplitude;        // Amplitud base (para restaurar)
    float firingRate;           // Frecuencia de disparo actual (Hz)
    float lastFiringTime;       // Último tiempo de disparo (s)
    float nextFiringTime;       // Próximo tiempo de disparo (s)
    bool isActive;              // Si está reclutada actualmente
};

// ============================================================================
// ESTRUCTURA DE MÉTRICAS PARA DISPLAY
// ============================================================================
/**
 * @brief Métricas EMG para visualización en tiempo real
 */
struct EMGDisplayMetrics {
    float rmsAmplitude_mV;      // Amplitud RMS en mV
    int activeMotorUnits;       // Número de MUs activas
    float meanFiringRate_Hz;    // Frecuencia promedio de disparo
    float contractionLevel;     // Nivel de contracción (0-100%)
    const char* conditionName;  // Nombre de la condición
};

// ============================================================================
// CLASE EMGModel
// ============================================================================
class EMGModel {
private:
    // Pool de unidades motoras
    MotorUnit motorUnits[MAX_MOTOR_UNITS];
    
    // Estado de excitación
    float currentExcitation;        // Excitación actual (puede variar)
    float baseExcitation;           // Excitación base de la condición
    float accumulatedTime;          // Tiempo acumulado (s)
    
    // Parámetros del usuario
    EMGParameters params;
    
    // Parámetros pendientes (cambio diferido)
    bool hasPendingParams;
    EMGParameters pendingParams;
    
    // Variables para condiciones especiales
    float tremorPhase;              // Fase del temblor (rad)
    float fatigueLevel;             // Nivel de fatiga (0-1)
    float forceVariabilityPhase;    // Fase de variabilidad de fuerza
    
    // Variables para patologías mejoradas
    float fasciculationTimer;       // Timer para próxima fasciculación
    float fasciculationBurstEnd;    // Fin del burst de fasciculación actual
    int fasciculationActiveMU;      // MU activa durante fasciculación
    float myopathyDurations[MAX_MOTOR_UNITS];   // Duraciones individuales para polifasia
    float neuropathyDurations[MAX_MOTOR_UNITS]; // Duraciones prolongadas para neuropatía
    
    // Buffer para cálculo de RMS
    float rmsBuffer[RMS_BUFFER_SIZE];
    int rmsBufferIndex;
    float rmsSum;
    
    // Último valor generado (para visualización)
    float lastSampleValue;
    
    // Ganancia para waveform
    float waveformGain;
    
    // Estado del generador gaussiano Box-Muller (variables de instancia para reset limpio)
    bool gaussHasSpare;
    float gaussSpare;
    
    // Métodos privados
    void initializeMotorUnits();
    void resetMotorUnitsToDefault();
    void updateMotorUnitRecruitment();
    float generateMUAP(float timeSinceFiring, float amplitude);
    float generateMUAPWithDuration(float timeSinceFiring, float amplitude, float duration, float sigma);
    float generatePolyphasicMUAP(float timeSinceFiring, float amplitude, int phases);
    float gaussianRandom(float mean, float std);
    void applyConditionModifiers();
    void updateRMSBuffer(float sample);
    
    // Conversión DAC
    uint8_t voltageToDACValue(float voltage);
    
public:
    EMGModel();
    
    // Configuración
    void setParameters(const EMGParameters& newParams);
    void setPendingParameters(const EMGParameters& newParams);
    void reset();
    
    // Parámetros Tipo A (aplicación inmediata)
    void setNoiseLevel(float noise) { params.noiseLevel = noise; }
    void setAmplitude(float amp) { params.amplitude = amp; }
    
    // Generación de señal
    float generateSample(float deltaTime);
    uint8_t getDACValue(float deltaTime);
    
    // Getters para visualización
    float getCurrentExcitation() const { return currentExcitation; }
    float getCurrentValueMV() const { return lastSampleValue; }  // Valor actual en mV
    uint8_t getWaveformValue() const;  // Valor escalado 0-255 para waveform
    void setWaveformGain(float gain) { waveformGain = constrain(gain, 1.0f, 20.0f); }
    float getWaveformGain() const { return waveformGain; }
    int getActiveMotorUnits() const;
    float getRMSAmplitude() const;
    float getMeanFiringRate() const;
    float getContractionLevel() const;
    const char* getConditionName() const;
    EMGDisplayMetrics getDisplayMetrics() const;
    
    // Getters de parámetros
    EMGCondition getCondition() const { return params.condition; }
    float getNoiseLevel() const { return params.noiseLevel; }
    float getAmplitude() const { return params.amplitude; }
};

#endif // EMG_MODEL_H
