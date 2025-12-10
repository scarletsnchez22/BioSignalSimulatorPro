/**
 * @file ppg_model.h
 * @brief Modelo PPG con doble gaussiana y 6 condiciones clínicas
 * @version 1.1.0
 * 
 * Modelo de fotopletismografía con HRV, muesca dicrótica y simulación
 * de condiciones patológicas con parámetros dinámicos (PI, SpO2).
 * 
 * Condiciones soportadas:
 * - NORMAL:           PI 2-5%, SpO2 95-100%, muesca dicrótica visible
 * - ARRHYTHMIA:       PI 1-5%, SpO2 92-100%, RR muy variable (>15%)
 * - WEAK_PERFUSION:   PI 0.1-0.5%, SpO2 88-98%, taquicardia compensatoria
 * - STRONG_PERFUSION: PI 5-20%, SpO2 96-100%, vasodilatación
 * - VASOCONSTRICTION: PI 0.2-0.8%, SpO2 91-100%, onda aplanada, muesca atenuada
 * - LOW_SPO2:         PI 0.5-3.5%, SpO2 70-90%, hipoxemia con perfusión conservada
 * 
 * Referencias científicas:
 * - Allen J. "Photoplethysmography and its application in clinical
 *   physiological measurement." Physiological Measurement, 2007.
 * - Elgendi M. "On the Analysis of Fingertip Photoplethysmogram Signals."
 *   Current Cardiology Reviews, 2012.
 * - Reisner A et al. "Utility of the photoplethysmogram in circulatory
 *   monitoring." Anesthesiology, 2008.
 * - Lima A, Bakker J. "Noninvasive monitoring of peripheral perfusion."
 *   Intensive Care Med, 2005.
 * - Jubran A. "Pulse oximetry." Crit Care, 2015.
 */

#ifndef PPG_MODEL_H
#define PPG_MODEL_H

#include <Arduino.h>
#include "../data/signal_types.h"

// ============================================================================
// CONSTANTES TEMPORALES DEL MODELO PPG
// Basado en: Elgendi M. "On the Analysis of Fingertip PPG Signals" (2012)
//            Allen J. "Photoplethysmography and its application" (2007)
//            Millasseau SC et al. "Contour analysis of the PPG pulse" (2006)
// ============================================================================

// --- Posiciones temporales (fracción del ciclo RR) ---
// Elgendi 2012, Fig. 3: Análisis de puntos fiduciales
#define PPG_SYSTOLIC_POS    0.15f   // Pico sistólico: 10-20% del ciclo (media ~15%)
#define PPG_NOTCH_POS       0.30f   // Muesca dicrótica: 25-35% (cierre válvula aórtica)
#define PPG_DIASTOLIC_POS   0.40f   // Pico diastólico: 35-50% (onda reflejada)

// --- Anchos gaussianos (desviación estándar normalizada) ---
// Derivados de FWHM típicos en Millasseau 2006
#define PPG_SYSTOLIC_WIDTH  0.055f  // σ sistólico: ~55ms FWHM a 75 BPM
#define PPG_DIASTOLIC_WIDTH 0.10f   // σ diastólico: más ancho (onda dispersada)
#define PPG_NOTCH_WIDTH     0.02f   // σ muesca: muy estrecha (evento valvular rápido)

// --- Amplitudes relativas ---
// Allen 2007, Tabla 1: Ratios de amplitud típicos
#define PPG_DIASTOLIC_RATIO 0.40f   // Ratio diastólico/sistólico: 0.3-0.6 (media 0.4)
#define PPG_NOTCH_DEPTH     0.25f   // Profundidad muesca: 0.1-0.4 (variable con edad)

// --- Componentes DC/AC ---
// Reisner 2008: El componente pulsátil es típicamente 1-5% del DC
#define PPG_DC_LEVEL        0.50f   // Nivel DC base (absorción tisular constante)
#define PPG_AC_SCALE        0.30f   // Escala AC: componente pulsátil visible

// --- Índice de Perfusión ---
// Lima & Bakker 2005: "Noninvasive monitoring of peripheral perfusion"
// PI normal en adultos: 0.5-5%, puede llegar a 20% en vasodilatación
#define PPG_PI_REFERENCE    5.0f    // PI de referencia para escala 1.0
#define PPG_PI_MIN          0.1f    // PI mínimo (shock severo)
#define PPG_PI_MAX          20.0f   // PI máximo (vasodilatación extrema)

// Constantes de escalado para waveform
#define PPG_MODEL_MIN       0.0f    // Mínimo teórico (normalizado)
#define PPG_MODEL_MAX       1.0f    // Máximo teórico (normalizado)
#define PPG_MODEL_RANGE     1.0f    // Rango total
#define PPG_WAVEFORM_GAIN_DEFAULT  1.5f  // Ganancia default (PPG ya usa buen rango)

// ============================================================================
// CLASE PPGModel
// ============================================================================
class PPGModel {
private:
    // Estado temporal
    float phaseInCycle;         // Fase actual (0-1)
    float currentRR;            // Intervalo RR actual (segundos)
    uint32_t beatCount;
    
    // Estado para generador gaussiano (Box-Muller)
    bool gaussHasSpare;
    float gaussSpare;
    
    // Parámetros del modelo
    float systolicAmplitude;    // A1
    float systolicWidth;        // σ1
    float diastolicAmplitude;   // A2
    float diastolicWidth;       // σ2
    float dicroticDepth;        // D
    float dicroticWidth;        // σd
    
    // Parámetros de entrada
    PPGParameters params;
    
    // Parámetros pendientes (Tipo B)
    bool hasPendingParams;
    PPGParameters pendingParams;
    
    // Variables para artefactos
    float motionNoise;
    float baselineWander;
    
    // Último valor generado (para visualización)
    float lastSampleValue;
    
    // Ganancia para waveform
    float waveformGain;
    
    // Métodos privados
    float generateNextRR();
    float gaussianRandom(float mean, float std);
    float computePulseShape(float phase);
    void applyConditionModifiers();
    void detectBeatAndApplyPending();
    
    // Conversión
    uint8_t voltageToDACValue(float voltage);
    
public:
    PPGModel();
    
    // Configuración
    void setParameters(const PPGParameters& newParams);
    void setPendingParameters(const PPGParameters& newParams);
    void reset();
    
    // Parámetros Tipo A (aplicación inmediata)
    void setNoiseLevel(float noise) { params.noiseLevel = noise; }
    void setAmplitude(float amp) { params.perfusionIndex = amp; systolicAmplitude = amp * 0.1f; }
    
    // Generación
    float generateSample(float deltaTime);
    uint8_t getDACValue(float deltaTime);
    
    // Getters
    float getCurrentHeartRate() const;
    float getCurrentRRInterval() const { return currentRR * 1000.0f; } // ms
    uint32_t getBeatCount() const { return beatCount; }
    float getPerfusionIndex() const;  // PI dinámico con variabilidad gaussiana
    float getSpO2() const;             // SpO2 dinámico según condición (70-100%)
    bool isInSystole() const;
    PPGCondition getCondition() const { return params.condition; }
    const char* getConditionName() const;
    float getNoiseLevel() const { return params.noiseLevel; }
    float getCurrentValueNormalized() const { return lastSampleValue; }  // Valor 0-1
    uint8_t getWaveformValue() const;  // Valor escalado 0-255 para waveform
    void setWaveformGain(float gain) { waveformGain = constrain(gain, 1.0f, 10.0f); }
    float getWaveformGain() const { return waveformGain; }
    
private:
    // Helper para getters const
    float gaussianRandomConst(float mean, float std) const;
};

#endif // PPG_MODEL_H
