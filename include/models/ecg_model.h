/**
 * @file ecg_model.h
 * @brief Modelo ECG basado en McSharry et al. (2003)
 * @version 1.0.0
 * 
 * Implementación del modelo dinámico de ECG con HRV.
 * 
 * REFERENCIAS CIENTÍFICAS:
 * [1] McSharry PE, et al. IEEE Trans Biomed Eng. 2003;50(3):289-294.
 *     DOI: 10.1109/TBME.2003.808805 - Modelo dinámico base
 * 
 * [2] Task Force ESC/NASPE. Circulation. 1996;93:1043-1065.
 *     DOI: 10.1161/01.CIR.93.5.1043 - Estándares HRV
 * 
 * Ver docs/README_MATHEMATICAL_BASIS.md para documentación científica completa.
 */

#ifndef ECG_MODEL_H
#define ECG_MODEL_H

#include <Arduino.h>
#include "../data/signal_types.h"

// ============================================================================
// ESTRUCTURA DE MÉTRICAS PARA DISPLAY
// ============================================================================
/**
 * @brief Métricas en tiempo real para visualización educativa
 * 
 * Todas las unidades son clínicamente relevantes:
 * - BPM: latidos por minuto
 * - RR: milisegundos
 * - Amplitudes: milivolts (mV)
 */
struct ECGDisplayMetrics {
    float bpm;              // Frecuencia cardíaca instantánea
    float rrInterval_ms;    // Intervalo RR en ms
    float rAmplitude_mV;    // Amplitud onda R en mV
    float stDeviation_mV;   // Desviación ST en mV
    uint32_t beatCount;     // Contador de latidos
    const char* conditionName;  // Nombre de la condición actual
};

// ============================================================================
// CONSTANTES DEL MODELO MCSHARRY
// ============================================================================
#define MCSHARRY_WAVES      5       // P, Q, R, S, T
#define ECG_BASELINE        0.04f   // Línea base
#define VFIB_COMPONENTS     5       // Componentes para modelo VFib alternativo

// Constantes de segmento ST (ángulos en radianes)
#define ST_SEGMENT_START    0.35f   // ~20 grados después de R
#define ST_SEGMENT_END      1.40f   // ~80 grados (antes del pico T)

// ============================================================================
// ESTADO DEL SISTEMA DINÁMICO
// ============================================================================
struct ECGDynamicState {
    float x;    // Posición X en círculo unitario
    float y;    // Posición Y en círculo unitario
    float z;    // Amplitud ECG (salida)
};

// ============================================================================
// ESTADO DEL MODELO VFIB ALTERNATIVO
// ============================================================================
struct VFibState {
    float frequencies[VFIB_COMPONENTS];  // 4-10 Hz
    float amplitudes[VFIB_COMPONENTS];   // Amplitudes variables
    float phases[VFIB_COMPONENTS];       // Fases que derivan lentamente
    float time;                          // Tiempo acumulado
    uint32_t lastUpdateMs;               // Última actualización de parámetros
};

// ============================================================================
// MORFOLOGÍA POR PATOLOGÍA
// ============================================================================
struct ECGMorphology {
    float hrMean;               // BPM medio
    float hrStd;                // Desviación estándar HR
    
    // Parámetros base de ondas PQRST (sin ajuste Bazett)
    float baseTi[MCSHARRY_WAVES];   // Ángulos base (radianes)
    float baseAi[MCSHARRY_WAVES];   // Amplitudes base
    float baseBi[MCSHARRY_WAVES];   // Anchos base
    
    // Parámetros ajustados por Bazett (los usados en cálculo)
    float ti[MCSHARRY_WAVES];   // Ángulos ajustados (radianes)
    float ai[MCSHARRY_WAVES];   // Amplitudes (no cambian con Bazett)
    float bi[MCSHARRY_WAVES];   // Anchos ajustados
    
    // Modificadores de patología
    bool pWavePresent;
    float stElevation;          // Elevación/depresión ST (+/- valores)
    float irregularityFactor;   // Factor de irregularidad RR (0-1)
    
    // PVC
    int pvcInterval;            // Cada cuántos latidos hay PVC (0 = ninguno)
    bool isPVCBeat;             // Flag: este latido es PVC
};

// ============================================================================
// CLASE ECGModel
// ============================================================================
class ECGModel {
private:
    // Estado del sistema dinámico
    ECGDynamicState state;
    ECGDynamicState k1, k2, k3, k4, temp;  // Para RK4
    
    // Estado alternativo para VFib
    VFibState vfibState;
    bool useVFibModel;  // Flag para usar modelo alternativo
    
    // Variables de control temporal
    float currentRR;
    float lastTheta;
    float breathPhase;          // Fase de respiración para drift del baseline
    unsigned long lastBeatTime;
    uint32_t beatCount;
    
    // Configuración
    ECGMorphology morphology;
    ECGParameters params;
    ECGCondition currentCondition;
    
    // Parámetros pendientes (Tipo B - aplicación diferida)
    bool hasPendingParams;
    ECGParameters pendingParams;
    
    // Estado del generador gaussiano Box-Muller (variables de instancia para reset limpio)
    bool gaussHasSpare;
    float gaussSpare;
    
    // Métodos privados - Sistema dinámico McSharry
    void computeDerivatives(const ECGDynamicState& s, ECGDynamicState& ds, float rr);
    void rungeKutta4Step(float dt);
    void applyBazettCorrection();
    
    // Métodos privados - Modelo VFib alternativo
    void initVFibModel();
    float generateVFibSample(float deltaTime);
    void updateVFibParameters();
    
    // Métodos privados - HRV y variabilidad
    float generateNextRR();
    float gaussianRandom(float mean, float std);
    float randomFloat();
    void detectBeatAndApplyPending();
    void preparePVCBeat();
    void prepareNormalBeat();
    void applyBeatToBeatVariation();
    void initializeFromRange(ECGCondition condition);
    
    // Métodos privados - Morfología por condición
    void applyMorphologyFromCondition(ECGCondition condition);
    void setNormalMorphology();
    void setTachycardiaMorphology();
    void setBradycardiaMorphology();
    void setAFibMorphology();
    void setVFibMorphology();
    void setPVCMorphology();
    void setBBBMorphology();
    void setSTElevationMorphology();
    void setSTDepressionMorphology();
    
    // Conversión DAC
    uint8_t voltageToDACValue(float mV);
    
public:
    ECGModel();
    
    // Configuración
    void setParameters(const ECGParameters& newParams);
    void setPendingParameters(const ECGParameters& newParams);  // Tipo B
    void reset();
    
    // Parámetros Tipo A (aplicación inmediata)
    void setNoiseLevel(float noise) { params.noiseLevel = noise; }
    void setAmplitude(float amp) { params.qrsAmplitude = amp; morphology.baseAi[2] = -5.0f * amp; }
    
    // Generación
    float generateSample(float deltaTime);
    uint8_t getDACValue(float deltaTime);
    
    // =========================================================================
    // GETTERS PARA VISUALIZACIÓN EDUCATIVA
    // =========================================================================
    // Métricas en tiempo real con unidades clínicas (mV, ms, BPM)
    
    /** @brief BPM instantáneo basado en RR actual */
    float getCurrentBPM() const;
    
    /** @brief Intervalo RR en milisegundos */
    float getCurrentRR_ms() const;
    
    /** @brief Amplitud onda R en mV (típico 0.5-1.5 mV) */
    float getRWaveAmplitude_mV() const;
    
    /** @brief Desviación ST en mV (normal ~0, STEMI +0.1-0.3) */
    float getSTDeviation_mV() const;
    
    /** @brief Valor ECG actual en mV equivalentes */
    float getCurrentValueMV() const;
    
    /** @brief Nombre de la condición actual (para display) */
    const char* getConditionName() const;
    
    /** @brief Contador de latidos desde reset */
    uint32_t getBeatCount() const;
    
    /** @brief Indica si estamos en fase QRS */
    bool isInBeat() const;
    
    /** @brief Obtiene todas las métricas en una estructura */
    ECGDisplayMetrics getDisplayMetrics() const;
    
    // Getters legacy (mantener compatibilidad)
    float getCurrentHeartRate() const { return morphology.hrMean; }
    float getCurrentRRInterval() const { return currentRR * 1000.0f; }
    ECGCondition getCondition() const { return currentCondition; }
};

#endif // ECG_MODEL_H
