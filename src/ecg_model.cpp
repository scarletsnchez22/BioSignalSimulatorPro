/*
 * ============================================================================
 * ECG Model - Implementación completa del modelo McSharry
 * ============================================================================
 * Basado en: "A Dynamical Model for Generating Synthetic Electrocardiogram 
 *             Signals" - McSharry et al., IEEE Trans. Biomed. Eng., 2003
 * 
 * Características:
 * - Sistema de ecuaciones diferenciales acopladas
 * - Integración numérica Runge-Kutta 4to orden
 * - HRV (Heart Rate Variability) realista
 * - Múltiples patologías configurables
 * - Generación continua sin repetición
 * ============================================================================
 */

#include "ecg_model.h"
#include "config.h"
#include <math.h>
#include <esp_random.h>

// ============================================================================
// CONSTRUCTOR
// ============================================================================
ECGModel::ECGModel() {
    reset();
}

// ============================================================================
// RESET - Inicializar estado del modelo
// ============================================================================
void ECGModel::reset() {
    // Estado inicial del sistema dinámico
    state.x = 1.0f;
    state.y = 0.0f;
    state.z = ECG_BASELINE;
    
    // Variables de control
    lastTheta = 0.0f;
    lastBeatTime = 0;
    beatCount = 0;
    
    // RR inicial
    currentRR = 0.8f;  // ~75 BPM
    
    // Morfología por defecto
    setNormalSinusMorphology();
}

// ============================================================================
// CONFIGURACIÓN DE PARÁMETROS
// ============================================================================
void ECGModel::setParameters(const ECGParameters& newParams) {
    params = newParams;
    
    // Aplicar morfología según condición
    setMorphologyFromCondition(params.condition);
    
    // Override del HR si se especifica
    if (params.heartRate > 0) {
        morphology.hrMean = params.heartRate;
    }
    
    // Aplicar modificadores de amplitud
    for (int i = 0; i < MCSHARRY_WAVES; i++) {
        if (i == 0) morphology.ai[i] *= params.pWaveAmplitude;
        if (i == 2) morphology.ai[i] *= params.qrsAmplitude;  // R
        if (i == 4) morphology.ai[i] *= params.tWaveAmplitude; // T
    }
    
    // ST elevation/depression desde parámetros
    if (params.stSegmentShift != 0.0f) {
        morphology.stElevation = params.stSegmentShift;
    }
    
    // Generar primer RR
    currentRR = generateNextRR();
}

// ============================================================================
// SELECTOR DE MORFOLOGÍA POR CONDICIÓN
// ============================================================================
void ECGModel::setMorphologyFromCondition(ECGCondition condition) {
    switch (condition) {
        case ECGCondition::NORMAL:
            setNormalSinusMorphology();
            break;
        case ECGCondition::BRADYCARDIA:
            setBradycardiaMorphology();
            break;
        case ECGCondition::TACHYCARDIA:
            setTachycardiaMorphology();
            break;
        case ECGCondition::ATRIAL_FIBRILLATION:
            setAtrialFibrillationMorphology();
            break;
        case ECGCondition::VENTRICULAR_FIBRILLATION:
            setVentricularFibrillationMorphology();
            break;
        case ECGCondition::PREMATURE_VENTRICULAR:
            setPVCMorphology();
            break;
        case ECGCondition::BUNDLE_BRANCH_BLOCK:
            setBundleBranchBlockMorphology();
            break;
        case ECGCondition::ST_ELEVATION:
            setSTElevationMorphology();
            break;
        case ECGCondition::ST_DEPRESSION:
            setSTDepressionMorphology();
            break;
        default:
            setNormalSinusMorphology();
            break;
    }
}

// ============================================================================
// DEFINICIONES DE PATOLOGÍAS
// ============================================================================

void ECGModel::setNormalSinusMorphology() {
    morphology.hrMean = 72.0f + gaussianRandom(0, 8);  // 64-80 BPM
    morphology.hrStd = 4.0f + gaussianRandom(0, 1);
    morphology.lfhfRatio = 0.5f;
    
    // Ángulos PQRST (grados -> radianes)
    float angles[5] = {-70.0f, -15.0f, 0.0f, 15.0f, 100.0f};
    for (int i = 0; i < 5; i++) {
        morphology.ti[i] = angles[i] * PI / 180.0f;
    }
    
    // Amplitudes PQRST (unidades normalizadas)
    float amps[5] = {1.2f, -5.0f, 30.0f, -7.5f, 0.75f};
    for (int i = 0; i < 5; i++) {
        morphology.ai[i] = amps[i] * (1.0f + gaussianRandom(0, 0.05f));
    }
    
    // Anchos gaussianos
    float widths[5] = {0.25f, 0.1f, 0.1f, 0.1f, 0.4f};
    for (int i = 0; i < 5; i++) {
        morphology.bi[i] = widths[i];
    }
    
    morphology.pWavePresent = true;
    morphology.stElevation = 0.0f;
    morphology.prInterval = 160.0f;
    morphology.qrsWidth = 100.0f;
    morphology.irregularityFactor = 0.0f;
    morphology.pvcInterval = 0;
}

void ECGModel::setBradycardiaMorphology() {
    setNormalSinusMorphology();
    morphology.hrMean = 45.0f + gaussianRandom(0, 5);   // 40-50 BPM
    morphology.hrStd = 3.0f;
    // T wave puede ser más prominente
    morphology.ai[4] = 1.0f + gaussianRandom(0, 0.1f);
}

void ECGModel::setTachycardiaMorphology() {
    setNormalSinusMorphology();
    morphology.hrMean = 130.0f + gaussianRandom(0, 15);  // 115-145 BPM
    morphology.hrStd = 6.0f;
    morphology.qrsWidth = 105.0f;
    // Intervalos más cortos, P puede fusionarse con T anterior
    morphology.bi[0] = 0.20f;  // P más estrecha
}

void ECGModel::setAtrialFibrillationMorphology() {
    setNormalSinusMorphology();
    morphology.hrMean = 100.0f + gaussianRandom(0, 20);  // 80-120 BPM variable
    morphology.hrStd = 25.0f;  // Alta variabilidad
    morphology.lfhfRatio = 3.0f;
    
    // Onda P ausente o muy reducida
    morphology.pWavePresent = false;
    morphology.ai[0] = 0.2f;  // P prácticamente ausente
    
    // Irregularidad alta
    morphology.irregularityFactor = 0.35f;
}

void ECGModel::setVentricularFibrillationMorphology() {
    morphology.hrMean = 300.0f + gaussianRandom(0, 50);  // Muy rápido
    morphology.hrStd = 80.0f;
    morphology.lfhfRatio = 5.0f;
    
    // Morfología caótica
    float angles[5] = {-60.0f, -10.0f, 0.0f, 10.0f, 60.0f};
    for (int i = 0; i < 5; i++) {
        morphology.ti[i] = angles[i] * PI / 180.0f;
    }
    
    // Amplitudes reducidas y variables
    float amps[5] = {0.3f, -2.0f, 8.0f, -3.0f, 0.4f};
    for (int i = 0; i < 5; i++) {
        morphology.ai[i] = amps[i] * (0.5f + gaussianRandom(0, 0.3f));
    }
    
    // Anchos muy variables
    for (int i = 0; i < 5; i++) {
        morphology.bi[i] = 0.15f + gaussianRandom(0, 0.05f);
    }
    
    morphology.pWavePresent = false;
    morphology.irregularityFactor = 1.0f;
}

void ECGModel::setPVCMorphology() {
    setNormalSinusMorphology();
    morphology.hrMean = 75.0f + gaussianRandom(0, 5);
    // PVC cada 4-8 latidos
    morphology.pvcInterval = 4 + (esp_random() % 5);
}

void ECGModel::setBundleBranchBlockMorphology() {
    setNormalSinusMorphology();
    morphology.qrsWidth = 140.0f + gaussianRandom(0, 10);  // >120ms
    
    // QRS ensanchado y bifásico
    morphology.bi[1] = 0.15f;  // Q más ancho
    morphology.bi[2] = 0.18f;  // R más ancho (notch)
    morphology.bi[3] = 0.15f;  // S más ancho
    
    // R' secundario (forma M en V1)
    morphology.ai[2] = 25.0f;  // R principal menor
}

void ECGModel::setSTElevationMorphology() {
    setNormalSinusMorphology();
    // Elevación ST (infarto agudo)
    morphology.stElevation = 0.25f + gaussianRandom(0, 0.08f);  // 2-3mm
    // Onda Q patológica más profunda
    morphology.ai[1] = -8.0f - gaussianRandom(0, 2);
    // T puede ser hiperaguda inicialmente
    morphology.ai[4] = 1.2f;
}

void ECGModel::setSTDepressionMorphology() {
    setNormalSinusMorphology();
    // Depresión ST (isquemia)
    morphology.stElevation = -0.15f - gaussianRandom(0, 0.05f);  // 1-2mm
    // T puede invertirse
    morphology.ai[4] = -0.5f;
}

// ============================================================================
// GENERACIÓN DE HRV (Heart Rate Variability)
// ============================================================================
float ECGModel::generateNextRR() {
    float hrRandom = morphology.hrMean;
    
    // Añadir variabilidad gaussiana base
    hrRandom += gaussianRandom(0, morphology.hrStd);
    
    // Irregularidad adicional para ciertas patologías
    if (morphology.irregularityFactor > 0.0f) {
        float irregular = gaussianRandom(0, morphology.hrStd * morphology.irregularityFactor * 2.0f);
        hrRandom += irregular;
    }
    
    // Para PVC: latido prematuro cada N latidos
    if (morphology.pvcInterval > 0 && beatCount > 0) {
        if ((beatCount % morphology.pvcInterval) == 0) {
            hrRandom *= 1.4f;  // RR más corto (latido prematuro)
        } else if ((beatCount % morphology.pvcInterval) == 1) {
            hrRandom *= 0.7f;  // Pausa compensatoria
        }
    }
    
    // Limitar a rangos fisiológicos
    hrRandom = constrain(hrRandom, 25.0f, 350.0f);
    
    // Convertir BPM a intervalo RR en segundos
    return 60.0f / hrRandom;
}

float ECGModel::gaussianRandom(float mean, float std) {
    // Box-Muller transform para distribución gaussiana
    static bool hasSpare = false;
    static float spare;
    
    if (hasSpare) {
        hasSpare = false;
        return mean + std * spare;
    }
    
    float u, v, s;
    do {
        u = (float)(esp_random() % 10000) / 5000.0f - 1.0f;
        v = (float)(esp_random() % 10000) / 5000.0f - 1.0f;
        s = u * u + v * v;
    } while (s >= 1.0f || s == 0.0f);
    
    s = sqrtf(-2.0f * logf(s) / s);
    spare = v * s;
    hasSpare = true;
    
    return mean + std * u * s;
}

// ============================================================================
// SISTEMA DINÁMICO - ECUACIONES DE MCSHARRY
// ============================================================================
void ECGModel::computeDerivatives(const DynamicState& s, DynamicState& ds, float rr) {
    // Ángulo actual en el círculo unitario
    float theta = atan2f(s.y, s.x);
    
    // Velocidad angular basada en RR
    float omega = 2.0f * PI / rr;
    
    // Factor de ajuste por frecuencia cardíaca
    float hrFactor = sqrtf(morphology.hrMean / 60.0f);
    float hrFactor2 = sqrtf(hrFactor);
    
    // Calcular parámetros ajustados
    float ti[MCSHARRY_WAVES], ai[MCSHARRY_WAVES], bi[MCSHARRY_WAVES];
    
    for (int i = 0; i < MCSHARRY_WAVES; i++) {
        ai[i] = morphology.ai[i];
        bi[i] = morphology.bi[i] * hrFactor;
    }
    
    // Ajustar ángulos según frecuencia
    ti[0] = morphology.ti[0] * hrFactor2;  // P
    ti[1] = morphology.ti[1] * hrFactor;   // Q
    ti[2] = morphology.ti[2];              // R (fijo)
    ti[3] = morphology.ti[3] * hrFactor;   // S
    ti[4] = morphology.ti[4] * hrFactor2;  // T
    
    // Calcular suma de atractores gaussianos (dz/dt)
    float dz = 0.0f;
    
    for (int i = 0; i < MCSHARRY_WAVES; i++) {
        // Saltar onda P si no está presente
        if (i == 0 && !morphology.pWavePresent) continue;
        
        // Diferencia angular (wrapping)
        float dtheta = fmodf(theta - ti[i] + PI, 2.0f * PI) - PI;
        
        float amp = ai[i];
        
        // Modificar amplitud para elevación/depresión ST
        // Afecta principalmente entre S y T
        if (i == 3 && morphology.stElevation != 0.0f) {
            amp += morphology.stElevation * 12.0f;
        }
        
        // Ecuación del atractor gaussiano
        float width = bi[i];
        dz += -amp * dtheta * expf(-0.5f * (dtheta / width) * (dtheta / width));
    }
    
    // Factor de relajación hacia línea base
    dz -= (s.z - ECG_BASELINE);
    
    // Ecuaciones del oscilador en círculo unitario
    float r = sqrtf(s.x * s.x + s.y * s.y);
    float alpha = 1.0f - r;  // Atracción hacia círculo unitario
    
    ds.x = alpha * s.x - omega * s.y;
    ds.y = alpha * s.y + omega * s.x;
    ds.z = dz;
}

// ============================================================================
// INTEGRACIÓN RUNGE-KUTTA 4TO ORDEN
// ============================================================================
void ECGModel::rungeKutta4Step(float dt) {
    // k1 = f(t, y)
    computeDerivatives(state, k1, currentRR);
    
    // k2 = f(t + dt/2, y + dt/2 * k1)
    temp.x = state.x + 0.5f * dt * k1.x;
    temp.y = state.y + 0.5f * dt * k1.y;
    temp.z = state.z + 0.5f * dt * k1.z;
    computeDerivatives(temp, k2, currentRR);
    
    // k3 = f(t + dt/2, y + dt/2 * k2)
    temp.x = state.x + 0.5f * dt * k2.x;
    temp.y = state.y + 0.5f * dt * k2.y;
    temp.z = state.z + 0.5f * dt * k2.z;
    computeDerivatives(temp, k3, currentRR);
    
    // k4 = f(t + dt, y + dt * k3)
    temp.x = state.x + dt * k3.x;
    temp.y = state.y + dt * k3.y;
    temp.z = state.z + dt * k3.z;
    computeDerivatives(temp, k4, currentRR);
    
    // Actualizar estado: y(t+dt) = y(t) + dt/6 * (k1 + 2*k2 + 2*k3 + k4)
    state.x += (dt / 6.0f) * (k1.x + 2.0f*k2.x + 2.0f*k3.x + k4.x);
    state.y += (dt / 6.0f) * (k1.y + 2.0f*k2.y + 2.0f*k3.y + k4.y);
    state.z += (dt / 6.0f) * (k1.z + 2.0f*k2.z + 2.0f*k3.z + k4.z);
}

// ============================================================================
// DETECCIÓN DE LATIDO Y ACTUALIZACIÓN DE RR
// ============================================================================
void ECGModel::detectBeatAndUpdateRR() {
    float theta = atan2f(state.y, state.x);
    
    // Detectar cruce por R (theta cruza de negativo a positivo por 0)
    if (lastTheta < 0.0f && theta >= 0.0f) {
        beatCount++;
        lastBeatTime = millis();
        
        // Generar nuevo RR para el próximo latido
        currentRR = generateNextRR();
    }
    
    lastTheta = theta;
}

// ============================================================================
// GENERACIÓN DE MUESTRA - MÉTODO PRINCIPAL
// ============================================================================
float ECGModel::generateSample(float deltaTime) {
    // Integrar sistema dinámico un paso
    rungeKutta4Step(deltaTime);
    
    // Detectar latido y actualizar RR dinámicamente
    detectBeatAndUpdateRR();
    
    // La salida es state.z (señal ECG)
    float sample = state.z;
    
    // Añadir ruido de medición si está configurado
    if (params.noiseLevel > 0.0f) {
        float noise = gaussianRandom(0.0f, 0.01f * params.noiseLevel);
        sample += noise;
    }
    
    return sample;
}

// ============================================================================
// CONVERSIÓN A DAC
// ============================================================================
uint8_t ECGModel::voltageToDACValue(float voltage) {
    // El sistema produce valores típicamente entre -0.4 y 1.2
    // Normalizar a rango 0-1
    float normalized = (voltage + 0.5f) / 1.8f;
    
    // Escalar a DAC (0-255) con margen
    int dacValue = (int)(normalized * 220.0f) + 17;
    
    // Limitar a rango válido
    return (uint8_t)constrain(dacValue, 0, 255);
}

uint8_t ECGModel::getDACValue(float deltaTime) {
    float voltage = generateSample(deltaTime);
    return voltageToDACValue(voltage);
}

// ============================================================================
// GETTERS
// ============================================================================
float ECGModel::getCurrentPhase() const {
    return atan2f(state.y, state.x);
}