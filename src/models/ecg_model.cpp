/**
 * @file ecg_model.cpp
 * @brief Implementación del modelo ECG McSharry con variabilidad natural
 * @version 1.0.0
 * 
 * MODELO BASE:
 * McSharry PE, Clifford GD, Tarassenko L, Smith L.
 * "A dynamical model for generating synthetic electrocardiogram signals."
 * IEEE Trans Biomed Eng. 2003;50(3):289-294.
 * 
 * EXTENSIONES IMPLEMENTADAS:
 * - Variabilidad latido-a-latido (beat-to-beat variability)
 * - Rangos fisiológicos por condición (basados en literatura clínica)
 * - Modelo alternativo para VFib (superposición espectral caótica)
 * - Implementación correcta de PVC con morfología aberrante
 * 
 * Ver docs/README_MATHEMATICAL_BASIS.md para documentación científica completa.
 */

#include "models/ecg_model.h"
#include "config.h"
#include <math.h>
#include <esp_random.h>

// ============================================================================
// RANGOS FISIOLÓGICOS POR CONDICIÓN (Basados en literatura clínica)
// ============================================================================
// Estos rangos definen los límites realistas para cada patología.
// El modelo selecciona valores aleatorios DENTRO de estos rangos.
// 
// REFERENCIAS CIENTÍFICAS:
// [1] Task Force of ESC and NASPE. "Heart Rate Variability: Standards of 
//     Measurement, Physiological Interpretation, and Clinical Use."
//     Circulation. 1996;93(5):1043-1065. doi:10.1161/01.CIR.93.5.1043
//     - Define SDNN normal = 141±39 ms, CV típico 5-8% en reposo
//     - HF power (0.15-0.4 Hz) representa modulación vagal
//
// [2] AHA/ACC Guidelines for Management of Patients with Ventricular
//     Arrhythmias. Circulation. 2017;138:e272-e391.
//     - Taquicardia sinusal: >100 BPM
//     - Bradicardia sinusal: <60 BPM (en atletas puede ser 30-50)
//
// [3] January CT, et al. "2014 AHA/ACC/HRS Guideline for Management of
//     Atrial Fibrillation." J Am Coll Cardiol. 2014;64(21):e1-e76.
//     - AFib: Ritmo irregularmente irregular, sin ondas P organizadas
//     - Respuesta ventricular típica 60-180 BPM
//
// [4] Link MS, et al. "2015 ACC/AHA/HRS Guideline for Management of
//     Adult Patients with SVT." Circulation. 2016;133:e506-e574.
//
// [5] McSharry PE, et al. IEEE Trans Biomed Eng. 2003;50(3):289-294.
//     - Parámetros originales del modelo dinámico

struct ConditionRange {
    float hrMin;        // BPM mínimo del rango
    float hrMax;        // BPM máximo del rango
    float hrStdMin;     // Variabilidad mínima (% del HR)
    float hrStdMax;     // Variabilidad máxima (% del HR)
    float stMin;        // ST shift mínimo (unidades modelo)
    float stMax;        // ST shift máximo
};

// Tabla de rangos por condición ECG
static const ConditionRange ECG_RANGES[] = {
    // 0: NORMAL: 60-100 BPM, variabilidad 3-8%, sin ST shift
    { 60.0f, 100.0f, 0.03f, 0.08f, 0.0f, 0.0f },
    
    // 1: TACHYCARDIA: 100-180 BPM, variabilidad 3-6%
    { 100.0f, 180.0f, 0.03f, 0.06f, 0.0f, 0.0f },
    
    // 2: BRADYCARDIA: 30-59 BPM, variabilidad 2-5%
    { 30.0f, 59.0f, 0.02f, 0.05f, 0.0f, 0.0f },
    
    // 3: ATRIAL_FIBRILLATION: 70-150 BPM base, alta variabilidad 15-25%
    { 70.0f, 150.0f, 0.15f, 0.25f, 0.0f, 0.0f },
    
    // 4: VENTRICULAR_FIBRILLATION: Ondas caóticas 4-10 Hz
    { 150.0f, 500.0f, 0.0f, 0.0f, 0.0f, 0.0f },
    
    // 5: PREMATURE_VENTRICULAR: 50-120 BPM base + PVCs
    { 50.0f, 120.0f, 0.04f, 0.10f, 0.0f, 0.0f },
    
    // 6: ST_ELEVATION (STEMI): 60-100 BPM, ST +0.30 a +0.55 mV
    { 60.0f, 100.0f, 0.03f, 0.06f, 0.30f, 0.55f },
    
    // 7: ST_DEPRESSION (Isquemia): 60-100 BPM, ST -0.25 a -0.50 mV
    { 60.0f, 100.0f, 0.03f, 0.06f, -0.50f, -0.25f }
};

// ============================================================================
// PARÁMETROS DE VARIABILIDAD NATURAL
// ============================================================================
// Estos factores controlan cuánto varía la morfología latido-a-latido.
//
// JUSTIFICACIÓN CIENTÍFICA:
// - La variabilidad de amplitud R por respiración es ~5-15% según:
//   Pallas-Areny R, Webster JG. "Sensors and Signal Conditioning." 2001.
//   Efecto de movimiento torácico sobre vector cardíaco.
//
// - HRV (variabilidad RR) documentada en Task Force 1996 [1]:
//   * SDNN normal = 141±39 ms sobre 24h
//   * RMSSD = 27±12 ms (variabilidad latido-a-latido)
//   * CV típico en reposo 5-8% del RR medio
//
// - Arritmia sinusal respiratoria (RSA): 0.15-0.4 Hz, modulación vagal
//   Berntson GG, et al. Psychophysiology. 1997;34(6):623-648.
//
// NOTA: Los valores de variabilidad morfológica (amplitud, ancho) son
// estimaciones empíricas conservadoras. No hay estándar publicado para
// variabilidad de bi/ti en el modelo McSharry.

// Variación de amplitud por latido (±5%)
// Basado en modulación respiratoria del vector cardíaco (5-15%)
static const float AMPLITUDE_VARIATION = 0.05f;

// Variación de ancho de onda por latido (±2%)
// Estimación conservadora - afecta duración aparente de ondas
static const float WIDTH_VARIATION = 0.02f;

// Variación de posición angular por latido (±0.01 rad ≈ ±0.6°)
// Pequeña variación en timing relativo de ondas
static const float POSITION_VARIATION = 0.01f;

// Deriva lenta del baseline (simula respiración)
// RSA típica: 0.15-0.4 Hz según Task Force 1996 [1]
static const float BASELINE_DRIFT_FREQ = 0.25f;  // Hz (centro del rango RSA)
static const float BASELINE_DRIFT_AMP = 0.015f;  // ~15 µV equivalente, sutil

// ============================================================================
// CONSTRUCTOR
// ============================================================================
ECGModel::ECGModel() {
    hasPendingParams = false;
    useVFibModel = false;
    currentCondition = ECGCondition::NORMAL;
    reset();
}

// ============================================================================
// RESET - Inicializa el modelo con valores aleatorios del rango
// ============================================================================
void ECGModel::reset() {
    // Estado inicial del sistema dinámico McSharry
    // El punto (1, 0, baseline) es el inicio del ciclo cardíaco
    state.x = 1.0f;
    state.y = 0.0f;
    state.z = ECG_BASELINE;
    
    // Variables de control temporal
    lastTheta = 0.0f;
    lastBeatTime = 0;
    beatCount = 0;
    breathPhase = 0.0f;  // Inicializar fase de respiración
    
    // Reset generador gaussiano Box-Muller
    gaussHasSpare = false;
    gaussSpare = 0.0f;
    
    // Inicializar ganancia del waveform
    waveformGain = ECG_WAVEFORM_GAIN_DEFAULT;  // 5.0 por defecto
    
    // Inicializar con morfología normal
    useVFibModel = false;
    setNormalMorphology();
    
    // Generar HR inicial aleatorio dentro del rango de la condición
    initializeFromRange(ECGCondition::NORMAL);
    
    // Aplicar corrección de Bazett y generar primer RR
    applyBazettCorrection();
    currentRR = generateNextRR();
}

// ============================================================================
// INICIALIZACIÓN DESDE RANGOS CLÍNICOS
// ============================================================================
/**
 * @brief Selecciona valores aleatorios dentro del rango clínico de la condición
 * @param condition La condición ECG a inicializar
 * 
 * Esta función asegura que cada vez que se inicia una condición,
 * los parámetros sean diferentes pero siempre dentro de rangos realistas.
 */
void ECGModel::initializeFromRange(ECGCondition condition) {
    uint8_t idx = static_cast<uint8_t>(condition);
    if (idx >= 9) idx = 0;  // Fallback a normal
    
    const ConditionRange& range = ECG_RANGES[idx];
    
    // Seleccionar HR aleatorio dentro del rango
    // Usamos distribución uniforme para cubrir todo el rango
    float hrRange = range.hrMax - range.hrMin;
    morphology.hrMean = range.hrMin + randomFloat() * hrRange;
    
    // Seleccionar variabilidad aleatoria dentro del rango
    float stdRange = range.hrStdMax - range.hrStdMin;
    float stdFactor = range.hrStdMin + randomFloat() * stdRange;
    morphology.hrStd = morphology.hrMean * stdFactor;
    
    // Para ST elevation/depression, seleccionar magnitud del rango
    if (range.stMin != 0.0f || range.stMax != 0.0f) {
        float stRange = range.stMax - range.stMin;
        morphology.stElevation = range.stMin + randomFloat() * stRange;
    }
}

// ============================================================================
// CONFIGURACIÓN DE PARÁMETROS
// ============================================================================
void ECGModel::setParameters(const ECGParameters& newParams) {
    params = newParams;
    currentCondition = params.condition;
    
    // Aplicar morfología base de la condición
    applyMorphologyFromCondition(params.condition);
    
    // Si el usuario especifica un HR, usarlo (pero mantener variabilidad)
    if (params.heartRate > 0) {
        // Validar que esté dentro del rango de la condición
        uint8_t idx = static_cast<uint8_t>(params.condition);
        if (idx < 9) {
            const ConditionRange& range = ECG_RANGES[idx];
            morphology.hrMean = constrain(params.heartRate, range.hrMin, range.hrMax);
        } else {
            morphology.hrMean = params.heartRate;
        }
    }
    
    // Aplicar factores de amplitud del usuario (escalado relativo)
    // Estos modifican las amplitudes base, no las reemplazan
    if (params.qrsAmplitude > 0.1f && params.qrsAmplitude < 5.0f) {
        morphology.baseAi[2] = 30.0f * params.qrsAmplitude;
    }
    if (params.pWaveAmplitude > 0.1f && params.pWaveAmplitude < 5.0f) {
        morphology.baseAi[0] = 1.2f * params.pWaveAmplitude;
    }
    if (params.tWaveAmplitude > 0.1f && params.tWaveAmplitude < 5.0f) {
        morphology.baseAi[4] = 0.75f * params.tWaveAmplitude;
    }
    
    // Copiar valores base a los activos
    for (int i = 0; i < MCSHARRY_WAVES; i++) {
        morphology.ai[i] = morphology.baseAi[i];
    }
    
    // Aplicar stShift del usuario si lo especifica (override de la condición)
    if (fabsf(params.stShift) > 0.01f) {
        morphology.stElevation = params.stShift;
    }
    
    // Aplicar correcciones y generar nuevo RR
    applyBazettCorrection();
    currentRR = generateNextRR();
}

void ECGModel::setPendingParameters(const ECGParameters& newParams) {
    pendingParams = newParams;
    hasPendingParams = true;
}

// ============================================================================
// CORRECCIÓN DE BAZETT (hrfact del MATLAB original)
// ============================================================================
/**
 * @brief Ajusta los anchos y posiciones de las ondas según el HR
 * 
 * La fórmula de Bazett establece que el QT interval es proporcional
 * a la raíz cuadrada del RR interval:
 *   QTc = QT / sqrt(RR)
 * 
 * En el modelo McSharry, esto se implementa escalando los parámetros
 * bi (anchos) y ti (posiciones) por sqrt(RR).
 */
void ECGModel::applyBazettCorrection() {
    float rrMean = 60.0f / morphology.hrMean;  // RR en segundos
    float hrfact = sqrtf(rrMean);
    
    for (int i = 0; i < MCSHARRY_WAVES; i++) {
        morphology.bi[i] = morphology.baseBi[i] * hrfact;
        morphology.ti[i] = morphology.baseTi[i] * hrfact;
    }
    
    // DEBUG: Imprimir parámetros del modelo
    Serial.printf("[ECG] Bazett: HR=%.1f, RR=%.3f, hrfact=%.3f\n", 
                  morphology.hrMean, rrMean, hrfact);
    Serial.printf("[ECG] ai: [%.2f, %.2f, %.2f, %.2f, %.2f]\n",
                  morphology.ai[0], morphology.ai[1], morphology.ai[2], 
                  morphology.ai[3], morphology.ai[4]);
    Serial.printf("[ECG] bi: [%.3f, %.3f, %.3f, %.3f, %.3f]\n",
                  morphology.bi[0], morphology.bi[1], morphology.bi[2], 
                  morphology.bi[3], morphology.bi[4]);
    Serial.printf("[ECG] ti: [%.3f, %.3f, %.3f, %.3f, %.3f]\n",
                  morphology.ti[0], morphology.ti[1], morphology.ti[2], 
                  morphology.ti[3], morphology.ti[4]);
}

// ============================================================================
// MORFOLOGÍA POR CONDICIÓN
// ============================================================================
void ECGModel::applyMorphologyFromCondition(ECGCondition condition) {
    useVFibModel = false;  // Por defecto usar McSharry
    
    switch (condition) {
        case ECGCondition::NORMAL:
            setNormalMorphology();
            break;
        case ECGCondition::TACHYCARDIA:
            setTachycardiaMorphology();
            break;
        case ECGCondition::BRADYCARDIA:
            setBradycardiaMorphology();
            break;
        case ECGCondition::ATRIAL_FIBRILLATION:
            setAFibMorphology();
            break;
        case ECGCondition::VENTRICULAR_FIBRILLATION:
            setVFibMorphology();
            break;
        case ECGCondition::PREMATURE_VENTRICULAR:
            setPVCMorphology();
            break;
        case ECGCondition::ST_ELEVATION:
            setSTElevationMorphology();
            break;
        case ECGCondition::ST_DEPRESSION:
            setSTDepressionMorphology();
            break;
        default:
            setNormalMorphology();
            break;
    }
    
    // Inicializar con valores del rango después de establecer morfología base
    initializeFromRange(condition);
}

// ============================================================================
// DEFINICIONES DE MORFOLOGÍA
// ============================================================================

/**
 * @brief Configura morfología para ritmo sinusal normal
 * 
 * Los parámetros base provienen del MATLAB original de McSharry:
 * - ti (ángulos): [-70, -15, 0, 15, 100] grados para PQRST
 * - ai (amplitudes): [1.2, -5, 30, -7.5, 0.75]
 * - bi (anchos): [0.25, 0.1, 0.1, 0.1, 0.4]
 */
void ECGModel::setNormalMorphology() {
    // Ángulos base PQRST (radianes) - del MATLAB original
    float angles[5] = {-70.0f, -15.0f, 0.0f, 15.0f, 100.0f};
    for (int i = 0; i < MCSHARRY_WAVES; i++) {
        morphology.baseTi[i] = angles[i] * PI / 180.0f;
    }
    
    // Amplitudes PQRST - del MATLAB original, escaladas x4
    // P: pequeña positiva, Q: pequeña negativa, R: grande positiva
    // S: pequeña negativa, T: moderada positiva
    // 
    // NOTA: Los valores originales [1.2, -5, 30, -7.5, 0.75] producen
    // amplitudes pequeñas. Multiplicamos por 4 para obtener el rango
    // correcto de [-0.4, +1.2] mV.
    const float AMPLITUDE_SCALE = 4.0f;
    float amps[5] = {1.2f * AMPLITUDE_SCALE, 
                     -5.0f * AMPLITUDE_SCALE, 
                     30.0f * AMPLITUDE_SCALE, 
                     -7.5f * AMPLITUDE_SCALE, 
                     0.75f * AMPLITUDE_SCALE};
    for (int i = 0; i < MCSHARRY_WAVES; i++) {
        morphology.baseAi[i] = amps[i];
        morphology.ai[i] = amps[i];
    }
    
    // Anchos de Gaussianas base
    float widths[5] = {0.25f, 0.1f, 0.1f, 0.1f, 0.4f};
    for (int i = 0; i < MCSHARRY_WAVES; i++) {
        morphology.baseBi[i] = widths[i];
    }
    
    // Configuración adicional
    morphology.pWavePresent = true;
    morphology.stElevation = 0.0f;
    morphology.irregularityFactor = 0.0f;
    morphology.pvcInterval = 0;
    morphology.isPVCBeat = false;
}

/**
 * @brief Taquicardia sinusal: ritmo rápido pero regular
 * Rango: 100-180 BPM (típico 100-150)
 */
void ECGModel::setTachycardiaMorphology() {
    setNormalMorphology();
    // HR y variabilidad se establecen en initializeFromRange()
}

/**
 * @brief Bradicardia sinusal: ritmo lento pero regular
 * Rango: 30-59 BPM (en atletas puede ser 30-50)
 */
void ECGModel::setBradycardiaMorphology() {
    setNormalMorphology();
    // HR y variabilidad se establecen en initializeFromRange()
}

/**
 * @brief Fibrilación auricular: sin P waves, RR muy irregular
 * Rango: 60-180 BPM con alta variabilidad (15-35%)
 */
void ECGModel::setAFibMorphology() {
    setNormalMorphology();
    
    // Sin ondas P (actividad auricular caótica)
    morphology.pWavePresent = false;
    morphology.baseAi[0] = 0.0f;
    morphology.ai[0] = 0.0f;
    
    // Alta irregularidad RR (característica principal de AFib)
    morphology.irregularityFactor = 0.35f;
}

/**
 * @brief Fibrilación ventricular: ritmo caótico sin estructura PQRST
 * 
 * NOTA: McSharry NO puede simular VFib porque el modelo genera
 * ondas PQRST organizadas. VFib es caos eléctrico sin estructura.
 * 
 * Usamos un modelo alternativo basado en superposición de sinusoides
 * con frecuencias en el rango 4-10 Hz (rango espectral de VFib real).
 */
void ECGModel::setVFibMorphology() {
    useVFibModel = true;
    initVFibModel();
    
    // Valores de referencia (no usados directamente)
    morphology.hrMean = 300.0f;
    morphology.hrStd = 100.0f;
    morphology.pWavePresent = false;
}

/**
 * @brief Contracciones ventriculares prematuras (PVC/extrasístoles)
 * 
 * Cada N latidos (configurable), se genera un latido ectópico con:
 * - Sin onda P (origen ventricular)
 * - QRS ancho (>120ms)
 * - Morfología aberrante
 * - Pausa compensatoria posterior
 */
void ECGModel::setPVCMorphology() {
    setNormalMorphology();
    
    // PVC cada 4-8 latidos (aleatorio para naturalidad)
    morphology.pvcInterval = 4 + (esp_random() % 5);
}

/**
 * @brief Elevación ST (STEMI/infarto agudo)
 * 
 * El segmento ST se eleva 0.1-0.3 mV sobre la línea base.
 * También se modifica la onda T (hiperaguda en infarto agudo).
 */
void ECGModel::setSTElevationMorphology() {
    setNormalMorphology();
    
    // La magnitud del ST se establece en initializeFromRange()
    
    // T wave HIPERAGUDA (característica de STEMI hiperagudo)
    // En fase aguda, la T es alta, puntiaguda y simétrica
    morphology.baseAi[4] = 1.5f;   // T wave más alta (normal ~0.75)
    morphology.ai[4] = 1.5f;
    morphology.baseBi[4] = 0.25f;  // T wave más ancha
    morphology.bi[4] = 0.25f;
    
    // Onda Q patológica puede aparecer (necrosis)
    morphology.baseAi[1] = -0.15f; // Q wave más profunda
    morphology.ai[1] = -0.15f;
}

/**
 * @brief Depresión ST (isquemia subendocárdica)
 * 
 * El segmento ST se deprime 0.1-0.3 mV bajo la línea base.
 * Patrón típico: "downsloping" o "horizontal" después del punto J
 * 
 * Características en Lead II:
 * - ST claramente por debajo de la línea PR (isoeléctrica)
 * - Puede ser horizontal, descendente o en "silla de montar"
 * - Onda T puede ser aplanada o invertida
 */
void ECGModel::setSTDepressionMorphology() {
    setNormalMorphology();
    
    // La magnitud del ST (negativa) se establece en initializeFromRange()
    
    // Onda T puede estar aplanada o ligeramente invertida en isquemia
    morphology.baseAi[4] = 0.4f;   // T wave más pequeña (normal ~0.75)
    morphology.ai[4] = 0.4f;
    
    // S wave puede ser más profunda
    morphology.baseAi[3] = -0.35f;  // S más profunda
    morphology.ai[3] = -0.35f;
}

// ============================================================================
// MODELO VFIB ALTERNATIVO (Superposición espectral caótica)
// ============================================================================
/**
 * @brief Inicializa el modelo alternativo para fibrilación ventricular
 * 
 * VFib se caracteriza por actividad eléctrica desorganizada con
 * frecuencia dominante de 4-10 Hz. Simulamos esto con múltiples
 * osciladores con parámetros que varían en el tiempo.
 */
void ECGModel::initVFibModel() {
    vfibState.time = 0.0f;
    vfibState.lastUpdateMs = millis();
    
    // Inicializar componentes frecuenciales
    for (int k = 0; k < VFIB_COMPONENTS; k++) {
        // Frecuencias en rango VFib real: 4-10 Hz
        // Coarse VFib: 4-6 Hz (mejor pronóstico)
        // Fine VFib: 6-10 Hz (peor pronóstico)
        vfibState.frequencies[k] = 4.0f + (float)k * 1.2f + 
            randomFloat() * 0.8f;
        
        // Amplitudes variables
        vfibState.amplitudes[k] = 0.12f + randomFloat() * 0.15f;
        
        // Fases aleatorias
        vfibState.phases[k] = randomFloat() * 2.0f * PI;
    }
}

/**
 * @brief Genera una muestra de VFib usando el modelo espectral
 * 
 * VFib real: ondas caóticas 4-10 Hz, amplitud 0.1-1.0 mV, 300-400 "latidos"/min
 * Sin estructura PQRST identificable
 */
float ECGModel::generateVFibSample(float deltaTime) {
    vfibState.time += deltaTime;
    
    // Actualizar parámetros cada ~50ms para efecto más caótico
    uint32_t now = millis();
    if (now - vfibState.lastUpdateMs > 50) {
        updateVFibParameters();
        vfibState.lastUpdateMs = now;
    }
    
    // Suma de componentes sinusoidales (ondas caóticas)
    float signal = 0.0f;
    for (int k = 0; k < VFIB_COMPONENTS; k++) {
        signal += vfibState.amplitudes[k] * 
                  sinf(2.0f * PI * vfibState.frequencies[k] * vfibState.time + 
                       vfibState.phases[k]);
    }
    
    // Añadir ruido de alta frecuencia para más caos
    signal += gaussianRandom(0.0f, 0.08f);
    
    // Escalar para obtener amplitud típica de VFib (0.1-1.0 mV pico)
    // Las ondas de VFib son más pequeñas que QRS normal
    signal = signal * 0.8f + ECG_BASELINE;
    
    // IMPORTANTE: Actualizar state.z para que getCurrentValueMV() funcione
    state.z = signal;
    
    // Calcular pseudo-HR basado en frecuencia dominante (~300-400 BPM)
    // Esto es solo para referencia, VFib no tiene HR real
    currentRR = 60.0f / (vfibState.frequencies[0] * 60.0f);  // ~0.15-0.2s
    
    return signal;
}

/**
 * @brief Actualiza parámetros de VFib para mantener caos
 */
void ECGModel::updateVFibParameters() {
    for (int k = 0; k < VFIB_COMPONENTS; k++) {
        // Modular amplitudes (simula variación coarse/fine)
        vfibState.amplitudes[k] += gaussianRandom(0.0f, 0.012f);
        vfibState.amplitudes[k] = constrain(vfibState.amplitudes[k], 0.04f, 0.30f);
        
        // Derivar fases (crea irregularidad)
        vfibState.phases[k] += gaussianRandom(0.0f, 0.06f);
        
        // Variar frecuencias ligeramente
        vfibState.frequencies[k] += gaussianRandom(0.0f, 0.12f);
        vfibState.frequencies[k] = constrain(vfibState.frequencies[k], 3.5f, 11.0f);
    }
}

// ============================================================================
// GENERACIÓN DE RR CON HRV NATURAL
// ============================================================================
/**
 * @brief Genera el próximo intervalo RR con variabilidad natural
 * 
 * La variabilidad del ritmo cardíaco (HRV) es una característica
 * fundamental del ECG real. Incluye:
 * - Variabilidad respiratoria (RSA)
 * - Fluctuaciones de baja frecuencia (barorreflejo)
 * - Ruido aleatorio
 */
float ECGModel::generateNextRR() {
    // RR base en segundos
    float rrMean = 60.0f / morphology.hrMean;
    
    // Desviación estándar proporcional al HR
    float rrStd = (morphology.hrStd / morphology.hrMean) * rrMean;
    
    // Componente aleatorio Gaussiano
    float rr = rrMean + gaussianRandom(0.0f, rrStd);
    
    // Añadir irregularidad adicional para AFib
    if (morphology.irregularityFactor > 0) {
        float irregularity = gaussianRandom(0.0f, morphology.irregularityFactor * 0.25f);
        rr *= (1.0f + irregularity);
    }
    
    // Obtener límites de la condición actual
    uint8_t idx = static_cast<uint8_t>(currentCondition);
    if (idx < 9) {
        const ConditionRange& range = ECG_RANGES[idx];
        // Convertir HR a RR: RR_min = 60/HR_max, RR_max = 60/HR_min
        float rrMin = 60.0f / (range.hrMax * 1.05f);  // 5% margen
        float rrMax = 60.0f / (range.hrMin * 0.95f);  // 5% margen
        rr = constrain(rr, rrMin, rrMax);
    } else {
        // Limitar a rangos fisiológicos extremos como fallback
        rr = constrain(rr, 0.2f, 2.5f);
    }
    
    return rr;
}

/**
 * @brief Genera número aleatorio Gaussiano usando Box-Muller
 * 
 * Usa variables de instancia en lugar de static para permitir
 * reset limpio del modelo y evitar problemas de thread-safety.
 */
float ECGModel::gaussianRandom(float mean, float std) {
    if (gaussHasSpare) {
        gaussHasSpare = false;
        return mean + std * gaussSpare;
    }
    
    float u, v, s;
    do {
        u = randomFloat() * 2.0f - 1.0f;
        v = randomFloat() * 2.0f - 1.0f;
        s = u * u + v * v;
    } while (s >= 1.0f || s == 0.0f);
    
    s = sqrtf(-2.0f * logf(s) / s);
    gaussSpare = v * s;
    gaussHasSpare = true;
    
    return mean + std * u * s;
}

/**
 * @brief Genera número aleatorio uniforme [0, 1)
 */
float ECGModel::randomFloat() {
    return (float)(esp_random()) / (float)UINT32_MAX;
}

// ============================================================================
// VARIABILIDAD LATIDO-A-LATIDO
// ============================================================================
/**
 * @brief Aplica variaciones sutiles a la morfología del latido actual
 * 
 * En un ECG real, cada latido es ligeramente diferente debido a:
 * - Variaciones en la conducción
 * - Cambios en la posición del corazón (respiración)
 * - Actividad del sistema nervioso autónomo
 * 
 * Esta función modifica sutilmente ai, bi, ti para cada latido.
 */
void ECGModel::applyBeatToBeatVariation() {
    for (int i = 0; i < MCSHARRY_WAVES; i++) {
        // Variación de amplitud (±5%)
        float ampVar = 1.0f + gaussianRandom(0.0f, AMPLITUDE_VARIATION);
        morphology.ai[i] = morphology.baseAi[i] * ampVar;
        
        // Variación de ancho (±3%)
        float widthVar = 1.0f + gaussianRandom(0.0f, WIDTH_VARIATION);
        morphology.bi[i] = morphology.baseBi[i] * widthVar;
        
        // Variación de posición angular (±0.02 rad ≈ ±1°)
        float posVar = gaussianRandom(0.0f, POSITION_VARIATION);
        morphology.ti[i] = morphology.baseTi[i] + posVar;
    }
    
    // Re-aplicar Bazett después de variaciones
    float rrMean = 60.0f / morphology.hrMean;
    float hrfact = sqrtf(rrMean);
    for (int i = 0; i < MCSHARRY_WAVES; i++) {
        morphology.bi[i] *= hrfact;
        morphology.ti[i] *= hrfact;
    }
}

// ============================================================================
// PVC - PREPARACIÓN DE LATIDO ECTÓPICO
// ============================================================================
/**
 * @brief Configura morfología para un latido PVC
 * 
 * Características del PVC:
 * - Sin onda P (origen ventricular, no auricular)
 * - QRS ancho y aberrante
 * - Onda T generalmente invertida (discordante con QRS)
 */
void ECGModel::preparePVCBeat() {
    morphology.isPVCBeat = true;
    
    // Sin onda P
    morphology.ai[0] = 0.0f;
    
    // QRS ancho y diferente
    morphology.bi[1] = 0.18f;
    morphology.bi[2] = 0.18f;
    morphology.bi[3] = 0.18f;
    
    // Morfología aberrante: puede ser positivo o negativo
    // Aquí simulamos un PVC con R prominente pero diferente
    morphology.ai[2] = morphology.baseAi[2] * (0.7f + randomFloat() * 0.4f);
    if (randomFloat() > 0.5f) {
        morphology.ai[2] = -morphology.ai[2];  // 50% chance de ser negativo
    }
    
    // S más profunda
    morphology.ai[3] = morphology.baseAi[3] * 1.4f;
    
    // T invertida (discordante)
    morphology.ai[4] = -fabsf(morphology.baseAi[4]) * 0.7f;
}

/**
 * @brief Restaura morfología normal después de un PVC
 */
void ECGModel::prepareNormalBeat() {
    morphology.isPVCBeat = false;
    
    // Restaurar valores base
    for (int i = 0; i < MCSHARRY_WAVES; i++) {
        morphology.ai[i] = morphology.baseAi[i];
    }
    
    // Re-aplicar corrección de Bazett
    applyBazettCorrection();
}

// ============================================================================
// SISTEMA DINÁMICO MCSHARRY - ECUACIONES DIFERENCIALES
// ============================================================================
/**
 * @brief Calcula las derivadas del sistema dinámico McSharry
 * 
 * El modelo usa 3 ODEs acopladas:
 * dx/dt = αx - ωy
 * dy/dt = αy + ωx
 * dz/dt = -Σ(ai * Δθi * exp(-Δθi²/2bi²)) - (z - z0)
 * 
 * Donde:
 * - α = 1 - sqrt(x² + y²) : atracción al círculo unitario
 * - ω = 2π/RR : velocidad angular
 * - θ = atan2(y, x) : ángulo actual
 * - Δθi = θ - θi : distancia angular a cada onda
 */
void ECGModel::computeDerivatives(const ECGDynamicState& s, ECGDynamicState& ds, float rr) {
    // Velocidad angular (determina la frecuencia cardíaca)
    float omega = 2.0f * PI / rr;
    
    // Ángulo actual en el ciclo cardíaco
    float theta = atan2f(s.y, s.x);
    
    // Factor de atracción al círculo unitario
    float alpha = 1.0f - sqrtf(s.x * s.x + s.y * s.y);
    
    // Derivadas de posición (movimiento circular)
    ds.x = alpha * s.x - omega * s.y;
    ds.y = alpha * s.y + omega * s.x;
    
    // Derivada de z (forma de onda ECG)
    // Suma de contribuciones Gaussianas de cada onda PQRST
    // 
    // Ecuación McSharry: dz/dt = -Σ(ai * Δθi * exp(-Δθi²/2bi²)) - (z - z0)
    // 
    // NOTA: Los parámetros ai del paper están normalizados para producir
    // amplitudes en el rango [-0.4, +1.2]. El factor omega (velocidad angular)
    // afecta la escala temporal, por lo que multiplicamos por omega para
    // mantener la amplitud correcta independiente del HR.
    float zDot = 0.0f;
    for (int i = 0; i < MCSHARRY_WAVES; i++) {
        // Saltar P wave si no está presente (AFib, PVC)
        if (!morphology.pWavePresent && i == 0) continue;
        
        // Distancia angular a esta onda (wrap a [-π, π])
        float dTheta = fmodf(theta - morphology.ti[i] + PI, 2.0f * PI) - PI;
        
        // Contribución Gaussiana de esta onda
        // Multiplicamos por omega para escalar correctamente con el HR
        float biSq = morphology.bi[i] * morphology.bi[i];
        zDot -= omega * morphology.ai[i] * dTheta * expf(-0.5f * dTheta * dTheta / biSq);
    }
    
    // Restauración a línea base (constante de tiempo ~1)
    zDot -= (s.z - ECG_BASELINE);
    
    // =========================================================================
    // ELEVACIÓN/DEPRESIÓN ST - Modelos diferenciados
    // =========================================================================
    if (fabsf(morphology.stElevation) > 0.001f) {
        // Rango del segmento ST
        const float ST_START = 0.25f;      // Punto J (fin QRS)
        const float ST_END = 1.3f;         // Inicio onda T
        
        if (theta > ST_START && theta < ST_END) {
            float envelope = 0.0f;
            float stPosition = (theta - ST_START) / (ST_END - ST_START);
            
            if (morphology.stElevation > 0) {
                // =====================================================
                // ELEVACIÓN ST (STEMI) - Patrón "meseta/lomo de delfín"
                // =====================================================
                const float PLATEAU_START = 0.15f;  // 15% del segmento
                const float PLATEAU_END = 0.75f;    // 75% del segmento
                
                if (stPosition < PLATEAU_START) {
                    // Subida rápida desde punto J
                    envelope = stPosition / PLATEAU_START;
                    envelope = envelope * envelope;
                } else if (stPosition < PLATEAU_END) {
                    // MESETA SOSTENIDA
                    envelope = 1.0f;
                } else {
                    // Transición suave a onda T
                    float pos = (stPosition - PLATEAU_END) / (1.0f - PLATEAU_END);
                    envelope = 1.0f - (pos * pos);
                }
                
                // Factor 3.0 para elevación muy visible
                zDot += morphology.stElevation * envelope * 3.0f;
                
            } else {
                // =====================================================
                // DEPRESIÓN ST (Isquemia) - Patrón "downsloping"
                // =====================================================
                // La depresión ST típica es descendente: empieza en punto J
                // y baja progresivamente hasta la onda T
                //
                // Patrón: máxima depresión al inicio, recuperación gradual
                
                // Downsloping: máximo al inicio, decrece linealmente
                // envelope va de 1.0 (inicio) a 0.3 (fin) - no llega a 0
                envelope = 1.0f - (stPosition * 0.7f);
                
                // Factor 3.5 para depresión muy visible (didáctico)
                // stElevation es negativo, así que esto resta de zDot
                zDot += morphology.stElevation * envelope * 3.5f;
            }
        }
    }
    
    // Deriva del baseline (simula respiración)
    // Nota: breathPhase se actualiza en generateSample para usar deltaTime correcto
    zDot += BASELINE_DRIFT_AMP * cosf(breathPhase) * 0.1f;
    
    ds.z = zDot;
}

// ============================================================================
// INTEGRACIÓN RK4
// ============================================================================
/**
 * @brief Integración Runge-Kutta de 4to orden
 * 
 * RK4 es más preciso que Euler y suficientemente eficiente para ESP32.
 * Error de truncamiento O(h⁵) vs O(h²) de Euler.
 */
void ECGModel::rungeKutta4Step(float dt) {
    // DEBUG: Imprimir estado cada 1000 pasos (~1 segundo)
    static uint32_t stepCount = 0;
    stepCount++;
    if (stepCount % 1000 == 0) {
        float theta = atan2f(state.y, state.x);
        float radius = sqrtf(state.x * state.x + state.y * state.y);
        Serial.printf("[RK4] x=%.4f, y=%.4f, z=%.4f, theta=%.2f, r=%.4f, dt=%.6f, RR=%.3f\n",
                      state.x, state.y, state.z, theta, radius, dt, currentRR);
    }
    
    // k1
    computeDerivatives(state, k1, currentRR);
    
    // k2
    temp.x = state.x + 0.5f * dt * k1.x;
    temp.y = state.y + 0.5f * dt * k1.y;
    temp.z = state.z + 0.5f * dt * k1.z;
    computeDerivatives(temp, k2, currentRR);
    
    // k3
    temp.x = state.x + 0.5f * dt * k2.x;
    temp.y = state.y + 0.5f * dt * k2.y;
    temp.z = state.z + 0.5f * dt * k2.z;
    computeDerivatives(temp, k3, currentRR);
    
    // k4
    temp.x = state.x + dt * k3.x;
    temp.y = state.y + dt * k3.y;
    temp.z = state.z + dt * k3.z;
    computeDerivatives(temp, k4, currentRR);
    
    // Actualización final (promedio ponderado)
    state.x += dt * (k1.x + 2.0f * k2.x + 2.0f * k3.x + k4.x) / 6.0f;
    state.y += dt * (k1.y + 2.0f * k2.y + 2.0f * k3.y + k4.y) / 6.0f;
    state.z += dt * (k1.z + 2.0f * k2.z + 2.0f * k3.z + k4.z) / 6.0f;
}

// ============================================================================
// DETECCIÓN DE LATIDO Y MANEJO DE CICLO
// ============================================================================
/**
 * @brief Detecta inicio de nuevo latido y aplica cambios pendientes
 * 
 * Un nuevo latido ocurre cuando θ cruza de negativo a positivo
 * (el punto pasa por x=1, y=0 en el círculo unitario).
 */
void ECGModel::detectBeatAndApplyPending() {
    float theta = atan2f(state.y, state.x);
    
    // Detectar cruce por cero (nuevo latido)
    if (lastTheta < 0 && theta >= 0) {
        beatCount++;
        lastBeatTime = millis();
        
        // Aplicar variabilidad latido-a-latido
        applyBeatToBeatVariation();
        
        // Aplicar parámetros pendientes si hay
        if (hasPendingParams) {
            setParameters(pendingParams);
            hasPendingParams = false;
        }
        
        // Manejar PVCs
        if (morphology.pvcInterval > 0 && (beatCount % morphology.pvcInterval) == 0) {
            preparePVCBeat();
            // PVC es prematuro: RR más corto
            currentRR = currentRR * (0.65f + randomFloat() * 0.15f);
        } else {
            if (morphology.isPVCBeat) {
                // Pausa compensatoria después del PVC
                prepareNormalBeat();
                currentRR = generateNextRR() * (1.2f + randomFloat() * 0.2f);
            } else {
                // Latido normal: generar nuevo RR
                currentRR = generateNextRR();
            }
        }
    }
    
    lastTheta = theta;
}

// ============================================================================
// GENERACIÓN DE MUESTRA
// ============================================================================
/**
 * @brief Genera una muestra de ECG
 * @param deltaTime Tiempo desde última muestra (típicamente 1ms)
 * @return Valor de ECG en unidades del modelo
 */
float ECGModel::generateSample(float deltaTime) {
    // VFib usa modelo alternativo
    if (useVFibModel) {
        return generateVFibSample(deltaTime);
    }
    
    // Actualizar fase de respiración para drift del baseline
    breathPhase += BASELINE_DRIFT_FREQ * 2.0f * PI * deltaTime;
    if (breathPhase > 2.0f * PI) breathPhase -= 2.0f * PI;
    
    // Modelo McSharry estándar
    rungeKutta4Step(deltaTime);
    detectBeatAndApplyPending();
    
    // Añadir ruido según parámetro del usuario
    float noise = gaussianRandom(0.0f, params.noiseLevel * 0.04f);
    
    return state.z + noise;
}

/**
 * @brief Genera valor DAC para salida analógica
 */
uint8_t ECGModel::getDACValue(float deltaTime) {
    float voltage = generateSample(deltaTime);
    return voltageToDACValue(voltage);
}

// ============================================================================
// SISTEMA DE ESCALADO Y CONVERSIÓN
// ============================================================================
// 
// DEFINICIÓN DE ESCALA:
// El modelo McSharry produce z(t) en unidades adimensionales. Para uso
// educativo y clínico, definimos una escala equivalente a milivolts (mV):
//
//   1.0 unidad del modelo ≈ 1.0 mV
//
// Esto es consistente con ECG Lead II típico donde:
//   - Onda R: 0.5-1.5 mV (típico ~1.0 mV)
//   - Onda P: 0.1-0.25 mV
//   - Onda T: 0.1-0.5 mV
//   - Ondas Q, S: -0.1 a -0.5 mV
//   - Baseline (isoeléctrica): 0 mV
//
// Con parámetros McSharry estándar (ai=[1.2,-5,30,-7.5,0.75]) el rango
// típico de z(t) es aproximadamente [-0.4, 1.2], lo cual es razonable
// para un ECG en mV.
//
// CONVERSIÓN DAC:
// ESP32 DAC tiene 8 bits (0-255) con salida 0-3.3V.
// Definimos el mapeo:
//   - z = -0.5 mV → DAC = 0   → 0.0 V
//   - z = +1.5 mV → DAC = 255 → 3.3 V
//   - z = 0.0 mV  → DAC = 64  → 0.83 V (baseline)
//   - z = 1.0 mV  → DAC = 191 → 2.47 V (R wave típica)
//
// Factor de escala para osciloscopio:
//   1 mV modelo = (3.3V / 2.0mV) = 1.65 V/mV en salida DAC
//
// ============================================================================

// Rango del modelo en mV equivalentes
static const float ECG_MV_MIN = -0.5f;   // mV mínimo (margen para Q/S profundas)
static const float ECG_MV_MAX = 1.5f;    // mV máximo (margen para R alta)
static const float ECG_MV_RANGE = ECG_MV_MAX - ECG_MV_MIN;  // 2.0 mV total

/**
 * @brief Convierte valor del modelo (mV equivalente) a valor DAC 8-bit
 * @param mV Valor de ECG en milivolts equivalentes
 * @return Valor DAC 0-255 para salida analógica
 * 
 * Mapeo lineal:
 *   mV = -0.5 → DAC = 0
 *   mV = +1.5 → DAC = 255
 */
uint8_t ECGModel::voltageToDACValue(float mV) {
    float normalized = (mV - ECG_MV_MIN) / ECG_MV_RANGE;
    normalized = constrain(normalized, 0.0f, 1.0f);
    return (uint8_t)(normalized * 255.0f);
}

/**
 * @brief Obtiene el valor actual en mV equivalentes
 * @return Valor ECG actual en milivolts
 */
float ECGModel::getCurrentValueMV() const {
    return state.z;  // El modelo ya está en escala mV
}

// ============================================================================
// ESCALADO CON GANANCIA FIJA PARA WAVEFORM
// ============================================================================
/**
 * @brief Obtiene valor escalado 0-255 para el waveform Nextion
 * @return Valor amplificado con ganancia fija para ocupar ~90% del display
 * 
 * ESTRATEGIA DE AMPLIFICACIÓN (preserva morfología):
 * 
 * 1. El modelo McSharry produce z(t) en rango teórico [-0.4, +1.2]
 * 2. En la práctica, la señal puede usar solo parte de ese rango
 * 3. Aplicamos ganancia fija para amplificar ANTES del mapeo
 * 4. Mapeamos al rango 0-255 usando el rango teórico completo
 * 
 * Fórmula:
 *   v' = G * (v - centro) + centro   // Amplificar respecto al centro
 *   v_255 = round((v' - ECG_MODEL_MIN) / ECG_MODEL_RANGE * 255)
 *   v_255 = clamp(v_255, 0, 255)
 * 
 * Con G=5.0 (default), una señal que usa 20% del rango pasará a usar ~100%
 */
uint8_t ECGModel::getWaveformValue() const {
    float sample = state.z;
    
    // =========================================================================
    // ESTRATEGIA DE AMPLIFICACIÓN SIMPLE Y DIRECTA
    // =========================================================================
    // 
    // El modelo McSharry produce z(t) en rango teórico [-0.4, +1.2]
    // Rango total = 1.6 unidades
    // 
    // Para el waveform queremos:
    // 1. Multiplicar la señal por la ganancia (amplificar)
    // 2. Mapear al rango 0-255 para ocupar todo el display
    // 
    // Fórmula simplificada:
    //   v' = sample * waveformGain
    //   v_255 = (v' - min_amplificado) / rango_amplificado * 255
    // =========================================================================
    
    // Amplificar la señal directamente
    float amplified = sample * waveformGain;
    
    // Rango amplificado (el rango original multiplicado por ganancia)
    const float AMP_MIN = ECG_MODEL_MIN * waveformGain;  // -0.4 * 5 = -2.0
    const float AMP_MAX = ECG_MODEL_MAX * waveformGain;  // 1.2 * 5 = 6.0
    const float AMP_RANGE = AMP_MAX - AMP_MIN;           // 8.0
    
    // Mapear al rango 0-1
    float normalized = (amplified - AMP_MIN) / AMP_RANGE;
    
    // Clampeo seguro
    normalized = constrain(normalized, 0.0f, 1.0f);
    
    return (uint8_t)(normalized * 255.0f);
}

// ============================================================================
// GETTERS PARA VISUALIZACIÓN EDUCATIVA
// ============================================================================
// Estas funciones proveen métricas en tiempo real para mostrar en la
// pantalla Nextion. Los valores están en unidades clínicamente relevantes.

/**
 * @brief Indica si estamos en la fase QRS del ciclo
 */
bool ECGModel::isInBeat() const {
    if (useVFibModel) return false;
    float theta = atan2f(state.y, state.x);
    return (theta > -0.15f && theta < 0.15f);
}

/**
 * @brief Obtiene BPM instantáneo basado en el RR actual
 * @return Frecuencia cardíaca en latidos por minuto
 * 
 * Para VFib: retorna pseudo-HR basado en frecuencia dominante (300-400 BPM)
 * Nota: VFib no tiene latidos reales, pero las ondas caóticas tienen
 * una frecuencia de 4-10 Hz que equivale a 240-600 "ciclos"/min
 */
float ECGModel::getCurrentBPM() const {
    if (useVFibModel) {
        // Pseudo-HR basado en frecuencia dominante de ondas caóticas
        // Frecuencia típica VFib: 4-7 Hz = 240-420 BPM equivalente
        float dominantFreq = vfibState.frequencies[0];
        return dominantFreq * 60.0f;  // Hz a BPM
    }
    if (currentRR <= 0.0f) return 0.0f;
    return 60.0f / currentRR;
}

/**
 * @brief Obtiene intervalo RR actual
 * @return Intervalo RR en milisegundos
 */
float ECGModel::getCurrentRR_ms() const {
    return currentRR * 1000.0f;
}

/**
 * @brief Obtiene amplitud de onda R actual
 * @return Amplitud R en mV (escala del modelo)
 * 
 * Valor típico normal: 0.5-1.5 mV en Lead II
 * Valores altos pueden indicar hipertrofia ventricular
 */
float ECGModel::getRWaveAmplitude_mV() const {
    // ai[2] es el parámetro de amplitud R en el modelo
    // Convertir de unidades internas a mV aproximados
    // Con ai=30 (valor base), el pico R es ~1.0 mV
    return fabsf(morphology.ai[2]) / 30.0f;  // Normalizado a ~1.0 mV base
}

/**
 * @brief Obtiene nivel de elevación/depresión ST
 * @return Desviación ST en mV
 * 
 * Normal: ±0.1 mV (≈isoeléctrico)
 * STEMI: +0.1 a +0.3 mV
 * Isquemia: -0.1 a -0.3 mV
 */
float ECGModel::getSTDeviation_mV() const {
    return morphology.stElevation;
}

/**
 * @brief Obtiene duración estimada del complejo QRS
 * @return Duración QRS en milisegundos
 * 
 * Referencias:
 * - Surawicz B, Knilans T. Chou's Electrocardiography (8th ed, 2008)
 * - Goldberger AL. Clinical Electrocardiography (9th ed, 2017)
 * 
 * Valores típicos:
 * - Normal: 80-100 ms
 * - PVC: 120-200 ms
 * - VFib: No definido (0 ms)
 */
float ECGModel::getQRSDuration_ms() const {
    if (useVFibModel) {
        return 0.0f;  // VFib no tiene QRS identificable
    }
    
    // La duración QRS depende del ancho (bi) de las ondas Q, R, S
    // bi[1]=Q, bi[2]=R, bi[3]=S en el modelo McSharry
    // El ancho en radianes se convierte a tiempo usando el período RR
    
    // Ancho combinado QRS en radianes (aproximación)
    float qrsWidthRad = morphology.bi[1] + morphology.bi[2] + morphology.bi[3];
    
    // Convertir de radianes a fracción del ciclo (2π = ciclo completo)
    float qrsFraction = qrsWidthRad / (2.0f * PI);
    
    // Duración base según condición (valores clínicos típicos)
    float baseDuration_ms;
    
    switch (currentCondition) {
        case ECGCondition::PREMATURE_VENTRICULAR:
            // PVC: QRS ancho 120-200 ms
            baseDuration_ms = 160.0f;
            break;
            
        case ECGCondition::VENTRICULAR_FIBRILLATION:
            // VFib: sin QRS definido
            return 0.0f;
            
        case ECGCondition::TACHYCARDIA:
            // Taquicardia: QRS puede ser ligeramente más corto
            baseDuration_ms = 85.0f;
            break;
            
        case ECGCondition::BRADYCARDIA:
            // Bradicardia: QRS normal
            baseDuration_ms = 90.0f;
            break;
            
        default:
            // Normal y otras: 80-100 ms
            baseDuration_ms = 90.0f;
            break;
    }
    
    // Ajustar ligeramente por el ancho del modelo (±10%)
    float adjustment = 1.0f + (qrsFraction - 0.1f) * 0.5f;
    adjustment = constrain(adjustment, 0.9f, 1.1f);
    
    return baseDuration_ms * adjustment;
}

/**
 * @brief Obtiene nombre de la condición actual
 * @return String con nombre de la condición
 */
const char* ECGModel::getConditionName() const {
    switch (currentCondition) {
        case ECGCondition::NORMAL:                return "Normal";
        case ECGCondition::TACHYCARDIA:           return "Taquicardia";
        case ECGCondition::BRADYCARDIA:           return "Bradicardia";
        case ECGCondition::ATRIAL_FIBRILLATION:   return "FA";
        case ECGCondition::VENTRICULAR_FIBRILLATION: return "FV";
        case ECGCondition::PREMATURE_VENTRICULAR: return "PVC";
        case ECGCondition::ST_ELEVATION:          return "STEMI";
        case ECGCondition::ST_DEPRESSION:         return "Isquemia";
        default:                                  return "Desconocido";
    }
}

/**
 * @brief Obtiene contador de latidos
 * @return Número de latidos desde reset
 */
uint32_t ECGModel::getBeatCount() const {
    return beatCount;
}

/**
 * @brief Estructura con todas las métricas para display
 */
ECGDisplayMetrics ECGModel::getDisplayMetrics() const {
    ECGDisplayMetrics metrics;
    metrics.bpm = getCurrentBPM();
    metrics.rrInterval_ms = getCurrentRR_ms();
    metrics.rAmplitude_mV = getRWaveAmplitude_mV();
    metrics.stDeviation_mV = getSTDeviation_mV();
    metrics.beatCount = beatCount;
    metrics.conditionName = getConditionName();
    return metrics;
}

/**
 * @brief Obtiene los rangos de HR para la patología actual
 * @param minHR Valor mínimo de HR para la patología
 * @param maxHR Valor máximo de HR para la patología
 */
void ECGModel::getHRRange(float& minHR, float& maxHR) const {
    uint8_t idx = static_cast<uint8_t>(currentCondition);
    if (idx >= 9) idx = 0;
    
    minHR = ECG_RANGES[idx].hrMin;
    maxHR = ECG_RANGES[idx].hrMax;
}
