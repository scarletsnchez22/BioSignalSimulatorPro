/**
 * @file ppg_model.h
 * @brief Modelo PPG con doble gaussiana y 6 condiciones clínicas
 * @version 1.1.0
 * 
 * Modelo de fotopletismografía con HRV, muesca dicrótica y simulación
 * de condiciones patológicas con parámetros dinámicos (PI, SpO2).
 * 
 * Condiciones soportadas (valores en mV físicos reales):
 * - NORMAL:           PI 1-5%, DC=1000mV, AC=15-50mV, morfología estándar
 * - ARRHYTHMIA:       PI 1-5%, DC=1000mV, AC=15-50mV, RR irregular ±15%
 * - WEAK_PERFUSION:   PI 0.02-0.4%, DC=1000mV, AC=0.2-5mV, señal muy débil
 * - VASODILATION:     PI 5-10%, DC=1000mV, AC=50-100mV, muesca marcada
 * - STRONG_PERFUSION: PI 10-20%, DC=1000mV, AC=100-200mV, señal muy robusta
 * - VASOCONSTRICTION: PI 0.2-0.8%, DC=1000mV, AC=2-10mV, onda afilada
 * 
 * Referencias científicas:
 * - BPL Medical Technologies (2023): Understanding Perfusion Index
 * - Allen J. Physiol Meas. 2007;28(3):R1-R39: Morfología PPG y PI
 * - ProQuest: AC/DC típico 1-2% del componente continuo
 * - De La Peña Sanabria I., et al. Rev. Physiol. Meas., 2007: PI clínico
 * - Lima A, Bakker J. Intensive Care Med, 2005: Monitoreo perfusión periférica
 * - Reisner A et al. Anesthesiology, 2008: Utilidad clínica PPG
/**
 * @file ppg_model.h
 * @brief Modelo PPG con doble gaussiana
 * @version 1.0.0
 * 
 * Modelo de fotopletismografía con HRV y muesca dicrótica.
 * 
 * Referencias científicas:
 * - Allen J. "Photoplethysmography and its application in clinical
 *   physiological measurement." Physiological Measurement, 2007.
 * - Elgendi M. "On the Analysis of Fingertip Photoplethysmogram Signals."
 *   Current Cardiology Reviews, 2012.
 * - Reisner A et al. "Utility of the photoplethysmogram in circulatory
 *   monitoring." Anesthesiology, 2008.
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
    float getPerfusionIndex() const;
    bool isInSystole() const;
    PPGCondition getCondition() const { return params.condition; }
    const char* getConditionName() const;
    float getNoiseLevel() const { return params.noiseLevel; }
    
    // Métricas medibles (según tabla de rangos clínicos)
    float getACAmplitude() const;      // AC en mV
    float getDCBaseline() const;       // DC en mV (siempre 1000)
    float getSystoleTime() const;      // Duración sístole en ms
    float getDiastoleTime() const;     // Duración diástole en ms
};

#endif // PPG_MODEL_H