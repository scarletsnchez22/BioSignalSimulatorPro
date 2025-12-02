/*
 * ============================================================================
 * PPG Model - Generador Fotopletismográfico de Alta Fidelidad para ESP32
 * ============================================================================
 * 
 * REFERENCIAS CIENTÍFICAS:
 * 
 * [1] Elgendi, M. et al. (2019). "Optimal Signal Quality Index for 
 *     Photoplethysmogram Signals." IEEE Reviews in Biomedical Engineering,
 *     Vol. 12, pp. 27-47.
 *     - Modelo matemático gold standard (correlación r>0.95 con MIMIC-III)
 *     - Validación de parámetros morfológicos
 * 
 * [2] Allen, J. (2007). "Photoplethysmography and its application in 
 *     clinical physiological measurement." Physiological Measurement,
 *     Vol. 28, R1-R39.
 *     - Caracterización morfológica completa del pulso arterial
 *     - Relación AC/DC y perfusión tisular
 * 
 * [3] Charlton, P.H. et al. (2022). "Modeling arterial pulse waves in 
 *     healthy aging." American Journal of Physiology, Vol. 322, H942-H962.
 *     - Efectos de rigidez arterial en morfología PPG
 * 
 * [4] Task Force ESC/NASPE (1996). "Heart Rate Variability: Standards."
 *     Circulation, Vol. 93, pp. 1043-1065.
 *     - Estándares HRV (SDNN, RMSSD, LF/HF)
 * 
 * [5] Lima, A. & Bakker, J. (2005). "Noninvasive monitoring of peripheral
 *     perfusion." Intensive Care Medicine, Vol. 31, pp. 1316-1326.
 *     - Índice de Perfusión (PI) y su significado clínico
 * 
 * ============================================================================
 * MODELO MATEMÁTICO: DOBLE GAUSSIANA
 * ============================================================================
 * 
 *   PPG(t) = DC + AC(t)
 * 
 *   AC(t) = A₁·exp(-(t-μ₁)²/2σ₁²)   ← Pico sistólico (eyección ventricular)
 *         + A₂·exp(-(t-μ₂)²/2σ₂²)   ← Pico diastólico (onda reflejada)
 *         - D·exp(-(t-μd)²/2σd²)    ← Muesca dicrótica (cierre válv. aórtica)
 * 
 * PARÁMETROS NORMALIZADOS AL INTERVALO RR:
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 * | Parámetro | Valor Normal | Descripción                              |
 * |-----------|--------------|------------------------------------------|
 * | A₁        | 1.0          | Amplitud pico sistólico (referencia)     |
 * | A₂        | 0.3-0.5      | Amplitud diastólico (~40% de A₁)         |
 * | μ₁        | 0.15         | Tiempo pico sistólico (150ms en RR=1s)   |
 * | μ₂        | 0.40         | Tiempo pico diastólico                   |
 * | σ₁        | 0.055        | Ancho sistólico (dP/dt)                  |
 * | σ₂        | 0.10         | Ancho diastólico                         |
 * | D         | 0.25         | Profundidad muesca dicrótica             |
 * | μd        | 0.30         | Posición muesca dicrótica                |
 * | σd        | 0.02         | Ancho muesca dicrótica                   |
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 * 
 * ============================================================================
 * SEÑAL 100% DINÁMICA - Nunca se repite porque:
 * ============================================================================
 * 
 * 1. HRV REALISTA (Task Force 1996):
 *    - Componente LF (0.04-0.15 Hz): Mayer waves, modulación simpática
 *    - Componente HF (0.15-0.4 Hz): RSA (Respiratory Sinus Arrhythmia)
 *    - Ruido gaussiano: Variabilidad intrínseca del nodo sinusal
 * 
 * 2. MODULACIÓN RESPIRATORIA:
 *    - AM (Amplitude Modulation): 5-15% por presión intratorácica
 *    - Baseline drift: Oscilación de componente DC
 * 
 * 3. LATIDOS ECTÓPICOS ALEATORIOS:
 *    - Probabilidad dependiente de patología
 *    - Pausa compensatoria post-ectópico
 * 
 * 4. RUIDO GAUSSIANO SNR-CONTROLADO:
 *    - Normal: 45 dB, Patológico: 30-40 dB
 * 
 * ============================================================================
 */

#ifndef PPG_MODEL_H
#define PPG_MODEL_H

#include <Arduino.h>
#include "signal_types.h"

// ============================================================================
// RANGOS FISIOLÓGICOS JUSTIFICADOS
// ============================================================================

// FRECUENCIA CARDÍACA - Ref: AHA Guidelines 2018
// Normal: 60-100 bpm, Atletas: 40-60 bpm
#define PPG_HR_MIN              40.0f       // bpm (bradicardia severa)
#define PPG_HR_MAX              180.0f      // bpm (taquicardia extrema)
#define PPG_HR_NORMAL           75.0f       // bpm (adulto sano reposo)

// HRV (SDNN) - Ref: Task Force ESC 1996
// Normal: 30-60 ms, Reducida: <20 ms, FA: >100 ms
#define PPG_HRV_NORMAL          50.0f       // ms (SDNN normal)
#define PPG_HRV_LOW             15.0f       // ms (riesgo cardiovascular)
#define PPG_HRV_HIGH            120.0f      // ms (fibrilación auricular)

// ÍNDICE DE PERFUSIÓN - Ref: Lima & Bakker 2005
// PI = (AC/DC) × 100%
// Normal: 1-10%, Óptimo: 4-7%, Crítico: <0.5%
#define PPG_PI_MIN              0.5f        // % (límite detectabilidad)
#define PPG_PI_MAX              15.0f       // % (vasodilatación extrema)
#define PPG_PI_NORMAL           5.0f        // % (perfusión óptima)

// SpO2 - Ref: WHO Pulse Oximetry Manual 2011
// Normal: 95-100%, Hipoxemia: <90%, Crítico: <75%
#define PPG_SPO2_NORMAL         97.0f       // %
#define PPG_SPO2_HYPOXIA        88.0f       // % (hipoxemia moderada)
#define PPG_SPO2_CRITICAL       75.0f       // % (riesgo daño orgánico)

// FRECUENCIA RESPIRATORIA - Ref: AARC Guidelines 2013
// Normal: 12-20 rpm, Modulación RSA: 5-15%
#define PPG_RESP_RATE_NORMAL    15.0f       // respiraciones/min
#define PPG_RSA_DEPTH           0.10f       // 10% modulación amplitud

// ============================================================================
// PARÁMETROS DEL MODELO DOBLE GAUSSIANA
// ============================================================================

// Gaussiana 1: Pico Sistólico (eyección ventricular)
#define G1_AMPLITUDE_NORMAL     1.0f        // Referencia normalizada
#define G1_MU_NORMAL            0.15f       // Posición temporal (15% RR)
#define G1_SIGMA_NORMAL         0.055f      // Ancho (velocidad eyección)

// Gaussiana 2: Pico Diastólico (onda reflejada)
#define G2_AMPLITUDE_NORMAL     0.40f       // 40% del pico sistólico
#define G2_MU_NORMAL            0.40f       // Posición temporal (40% RR)
#define G2_SIGMA_NORMAL         0.10f       // Ancho (decaimiento)

// Muesca Dicrótica (cierre válvula aórtica)
#define DICROTIC_DEPTH_NORMAL   0.25f       // Profundidad
#define DICROTIC_MU_NORMAL      0.30f       // Posición (30% RR)
#define DICROTIC_SIGMA_NORMAL   0.02f       // Ancho (estrecha)

// ============================================================================
// ESTRUCTURA: Parámetros Morfológicos del Pulso
// ============================================================================
struct PulseMorphology {
    // Gaussiana 1 (Sistólica)
    float a1;           // Amplitud pico sistólico
    float mu1;          // Posición temporal
    float sigma1;       // Ancho
    
    // Gaussiana 2 (Diastólica)
    float a2;           // Amplitud pico diastólico
    float mu2;          // Posición temporal
    float sigma2;       // Ancho
    
    // Muesca Dicrótica
    float dicroticDepth;    // Profundidad (0 = sin muesca)
    float dicroticMu;       // Posición
    float dicroticSigma;    // Ancho
    
    // Factores de modificación
    float arterialStiffness;    // Rigidez arterial (1.0 = normal)
    float dampingFactor;        // Amortiguamiento (1.0 = normal)
    
    // Constructor con valores normales
    PulseMorphology() :
        a1(G1_AMPLITUDE_NORMAL),
        mu1(G1_MU_NORMAL),
        sigma1(G1_SIGMA_NORMAL),
        a2(G2_AMPLITUDE_NORMAL),
        mu2(G2_MU_NORMAL),
        sigma2(G2_SIGMA_NORMAL),
        dicroticDepth(DICROTIC_DEPTH_NORMAL),
        dicroticMu(DICROTIC_MU_NORMAL),
        dicroticSigma(DICROTIC_SIGMA_NORMAL),
        arterialStiffness(1.0f),
        dampingFactor(1.0f)
    {}
};

// ============================================================================
// ESTRUCTURA: Estado de Variabilidad Cardíaca (HRV)
// ============================================================================
struct HRVState {
    float sdnn;             // SDNN en ms (desviación estándar RR)
    float lfPower;          // Potencia banda LF (0.04-0.15 Hz)
    float hfPower;          // Potencia banda HF (0.15-0.4 Hz)
    float lfPhase;          // Fase actual LF
    float hfPhase;          // Fase actual HF
    float currentRR;        // Intervalo RR actual (ms)
    float lastRR;           // Intervalo RR anterior
    
    // Para detección de irregularidad (FA)
    bool irregularRhythm;   // true = distribución uniforme (FA)
    
    HRVState() :
        sdnn(PPG_HRV_NORMAL),
        lfPower(0.5f),
        hfPower(0.3f),
        lfPhase(0.0f),
        hfPhase(0.0f),
        currentRR(800.0f),
        lastRR(800.0f),
        irregularRhythm(false)
    {}
};

// ============================================================================
// ESTRUCTURA: Estado Respiratorio
// ============================================================================
struct RespiratoryState {
    float rate;             // Frecuencia respiratoria (rpm)
    float phase;            // Fase actual (0 a 2π)
    float amDepth;          // Profundidad modulación amplitud
    float baselineDrift;    // Deriva de línea base
    
    RespiratoryState() :
        rate(PPG_RESP_RATE_NORMAL),
        phase(0.0f),
        amDepth(PPG_RSA_DEPTH),
        baselineDrift(0.02f)
    {}
};

// ============================================================================
// ESTRUCTURA: Configuración de Patología
// ============================================================================
struct PathologyConfig {
    float heartRate;            // HR objetivo
    float perfusionIndex;       // PI objetivo
    float spo2;                 // SpO2 simulado
    float ectopicProbability;   // Prob. latido ectópico (0-1)
    float snrDb;                // Relación señal/ruido en dB
    float motionArtifactProb;   // Prob. artefacto movimiento
    
    PathologyConfig() :
        heartRate(PPG_HR_NORMAL),
        perfusionIndex(PPG_PI_NORMAL),
        spo2(PPG_SPO2_NORMAL),
        ectopicProbability(0.01f),
        snrDb(45.0f),
        motionArtifactProb(0.02f)
    {}
};

// ============================================================================
// CLASE: PPGModel
// ============================================================================
class PPGModel {
private:
    // Parámetros de entrada
    PPGParameters params;
    
    // Estado del modelo
    PulseMorphology morphology;
    HRVState hrv;
    RespiratoryState resp;
    PathologyConfig pathology;
    
    // Variables de tiempo
    float currentTime;          // Tiempo actual en segundos
    float beatStartTime;        // Inicio del latido actual
    float nextBeatTime;         // Tiempo del próximo latido
    uint32_t beatCount;         // Contador de latidos
    
    // Componentes de señal
    float dcComponent;          // Componente DC actual
    float acComponent;          // Componente AC actual
    float lastSample;           // Última muestra generada
    
    // Generador de números aleatorios
    float gaussianRandom(float mean, float stddev);
    float uniformRandom(float min, float max);
    
    // Generación de señal
    float generatePulseWaveform(float t_normalized);
    float calculateHRVModulation();
    float calculateRespiratoryModulation();
    float generateNoise();
    float generateMotionArtifact();
    
    // Configuración de patologías
    void setNormalMorphology();
    void setBradycardiaMorphology();
    void setTachycardiaMorphology();
    void setHypoxemiaMorphology();
    void setHypertensionMorphology();
    void setDiabeticMorphology();
    void setArrhythmiaMorphology();
    void setShockMorphology();
    void setAtrialFibrillationMorphology();
    
    // Utilidades
    float gaussian(float t, float mu, float sigma, float amplitude);
    void updateRRInterval();
    bool checkEctopicBeat();
    
public:
    PPGModel();
    
    // Configuración
    void setParameters(const PPGParameters& params);
    void reset();
    
    // Generación de señal
    float generateSample(float deltaTime);
    uint8_t getDACValue(float deltaTime);
    
    // Getters para métricas
    float getHeartRate() const { return 60000.0f / hrv.currentRR; }
    float getPerfusionIndex() const { return pathology.perfusionIndex; }
    float getSpO2() const { return pathology.spo2; }
    float getSDNN() const { return hrv.sdnn; }
    float getDCComponent() const { return dcComponent; }
    float getACAmplitude() const { return acComponent; }
    uint32_t getBeatCount() const { return beatCount; }
    
    // Control de respiración (para RSA)
    void setRespiratoryRate(float rpm);
};

#endif // PPG_MODEL_H
