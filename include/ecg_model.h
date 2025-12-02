#ifndef ECG_MODEL_H
#define ECG_MODEL_H

#include <Arduino.h>
#include "signal_types.h"

// ============================================================================
// CONSTANTES DEL MODELO MCSHARRY
// ============================================================================
#define MCSHARRY_WAVES      5       // P, Q, R, S, T
#define ECG_BASELINE        0.04f   // Línea base del sistema dinámico

// ============================================================================
// ESTRUCTURA: Estado del sistema dinámico
// ============================================================================
struct DynamicState {
    float x;    // Posición X en círculo unitario
    float y;    // Posición Y en círculo unitario  
    float z;    // Amplitud ECG (salida del modelo)
};

// ============================================================================
// ESTRUCTURA: Parámetros morfológicos de patología
// ============================================================================
struct PathologyMorphology {
    float hrMean;           // BPM medio
    float hrStd;            // Desviación estándar del HR
    float lfhfRatio;        // Ratio LF/HF para HRV
    
    // Parámetros de las 5 ondas PQRST
    float ti[MCSHARRY_WAVES];   // Ángulos de extremos (radianes)
    float ai[MCSHARRY_WAVES];   // Amplitudes relativas
    float bi[MCSHARRY_WAVES];   // Anchos gaussianos
    
    // Modificadores específicos de patología
    bool pWavePresent;      // false para fibrilación auricular
    float stElevation;      // Elevación/depresión ST (mV)
    float prInterval;       // Intervalo PR (ms)
    float qrsWidth;         // Ancho QRS (ms)
    
    // Extras para patologías específicas
    float irregularityFactor;   // Factor de irregularidad RR
    int pvcInterval;            // Cada cuántos latidos hay PVC (0=ninguno)
};

// ============================================================================
// CLASE: ECGModel - Modelo McSharry completo
// ============================================================================
class ECGModel {
private:
    // Estado del sistema dinámico
    DynamicState state;
    DynamicState k1, k2, k3, k4, temp;  // Para RK4
    
    // Variables de control temporal
    float currentRR;            // Intervalo RR actual (segundos)
    float lastTheta;            // Para detección de latido
    unsigned long lastBeatTime; // Timestamp último latido
    uint32_t beatCount;         // Contador de latidos
    
    // Parámetros morfológicos actuales
    PathologyMorphology morphology;
    ECGParameters params;       // Parámetros de interfaz
    
    // Métodos privados - Sistema dinámico
    void computeDerivatives(const DynamicState& s, DynamicState& ds, float rr);
    void rungeKutta4Step(float dt);
    
    // Métodos privados - HRV y variabilidad
    float generateNextRR();
    float gaussianRandom(float mean, float std);
    void detectBeatAndUpdateRR();
    
    // Métodos privados - Configuración de patologías
    void setMorphologyFromCondition(ECGCondition condition);
    void setNormalSinusMorphology();
    void setBradycardiaMorphology();
    void setTachycardiaMorphology();
    void setAtrialFibrillationMorphology();
    void setVentricularFibrillationMorphology();
    void setPVCMorphology();
    void setBundleBranchBlockMorphology();
    void setSTElevationMorphology();
    void setSTDepressionMorphology();
    
    // Conversión de salida
    uint8_t voltageToDACValue(float voltage);
    
public:
    ECGModel();
    
    // Configuración
    void setParameters(const ECGParameters& params);
    void reset();
    
    // Generación de señal - método principal
    float generateSample(float deltaTime);
    uint8_t getDACValue(float deltaTime);
    
    // Getters para debug/monitoreo
    float getCurrentPhase() const;
    float getRRInterval() const { return currentRR; }
    unsigned long getLastBeatTime() const { return lastBeatTime; }
    uint32_t getBeatCount() const { return beatCount; }
    float getCurrentZ() const { return state.z; }
};

#endif // ECG_MODEL_H