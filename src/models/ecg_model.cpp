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
    // NORMAL: 60-100 BPM, variabilidad 3-8%, sin ST shift
    { 60.0f, 100.0f, 0.03f, 0.08f, 0.0f, 0.0f },
    
    // TACHYCARDIA: 100-180 BPM, variabilidad 3-6%
    { 100.0f, 180.0f, 0.03f, 0.06f, 0.0f, 0.0f },
    
    // BRADYCARDIA: 30-59 BPM, variabilidad 2-5%
    { 30.0f, 59.0f, 0.02f, 0.05f, 0.0f, 0.0f },
    
    // ATRIAL_FIBRILLATION: 60-180 BPM, alta variabilidad 15-35%
    { 60.0f, 180.0f, 0.15f, 0.35f, 0.0f, 0.0f },
    
    // VENTRICULAR_FIBRILLATION: No aplica rangos tradicionales (caótico)
    { 150.0f, 500.0f, 0.0f, 0.0f, 0.0f, 0.0f },
    
    // PREMATURE_VENTRICULAR: 50-120 BPM base + PVCs
    { 50.0f, 120.0f, 0.04f, 0.10f, 0.0f, 0.0f },
    
    // BUNDLE_BRANCH_BLOCK: 40-100 BPM
    { 40.0f, 100.0f, 0.03f, 0.07f, 0.0f, 0.0f },
    
    // ST_ELEVATION: 50-110 BPM, ST +0.1 a +0.3 (en unidades modelo: 0.15-0.35)
    { 50.0f, 110.0f, 0.03f, 0.08f, 0.15f, 0.35f },
    
    // ST_DEPRESSION: 50-150 BPM, ST -0.1 a -0.3 (en unidades modelo: -0.35 a -0.15)
    { 50.0f, 150.0f, 0.03f, 0.08f, -0.35f, -0.15f }
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
        case ECGCondition::BUNDLE_BRANCH_BLOCK:
            setBBBMorphology();
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
    
    // Amplitudes PQRST - del MATLAB original
    // P: pequeña positiva, Q: pequeña negativa, R: grande positiva
    // S: pequeña negativa, T: moderada positiva
    float amps[5] = {1.2f, -5.0f, 30.0f, -7.5f, 0.75f};
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
 * @brief Bloqueo de rama: QRS ancho (>120ms)
 * 
 * Se ensanchan las ondas Q, R, S aumentando sus parámetros bi.
 * El QRS resultante simula la conducción retardada por una rama.
 */
void ECGModel::setBBBMorphology() {
    setNormalMorphology();
    
    // QRS ancho: aumentar bi para Q, R, S (índices 1, 2, 3)
    // Esto simula la duración >120ms del QRS
    morphology.baseBi[1] = 0.15f;  // Q más ancho
    morphology.baseBi[2] = 0.15f;  // R más ancho
    morphology.baseBi[3] = 0.15f;  // S más ancho
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
    // T wave hiperaguda (típico de STEMI agudo)
    morphology.baseAi[4] = 1.2f;
    morphology.ai[4] = 1.2f;
}

/**
 * @brief Depresión ST (isquemia subendocárdica)
 * 
 * El segmento ST se deprime 0.1-0.3 mV bajo la línea base.
 */
void ECGModel::setSTDepressionMorphology() {
    setNormalMorphology();
    // La magnitud del ST (negativa) se establece en initializeFromRange()
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
 */
float ECGModel::generateVFibSample(float deltaTime) {
    vfibState.time += deltaTime;
    
    // Actualizar parámetros cada ~80ms para efecto caótico
    uint32_t now = millis();
    if (now - vfibState.lastUpdateMs > 80) {
        updateVFibParameters();
        vfibState.lastUpdateMs = now;
    }
    
    // Suma de componentes sinusoidales
    float signal = 0.0f;
    for (int k = 0; k < VFIB_COMPONENTS; k++) {
        signal += vfibState.amplitudes[k] * 
                  sinf(2.0f * PI * vfibState.frequencies[k] * vfibState.time + 
                       vfibState.phases[k]);
    }
    
    // Añadir ruido de alta frecuencia
    signal += gaussianRandom(0.0f, 0.06f);
    
    // Escalar al rango del modelo
    signal = signal * 0.6f + ECG_BASELINE;
    
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
    
    // Limitar a rangos fisiológicos extremos
    // RR mínimo ~0.2s = 300 BPM, RR máximo ~2.5s = 24 BPM
    rr = constrain(rr, 0.2f, 2.5f);
    
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
    float zDot = 0.0f;
    for (int i = 0; i < MCSHARRY_WAVES; i++) {
        // Saltar P wave si no está presente (AFib, PVC)
        if (!morphology.pWavePresent && i == 0) continue;
        
        // Distancia angular a esta onda (wrap a [-π, π])
        float dTheta = fmodf(theta - morphology.ti[i] + PI, 2.0f * PI) - PI;
        
        // Contribución Gaussiana de esta onda
        float biSq = morphology.bi[i] * morphology.bi[i];
        zDot -= morphology.ai[i] * dTheta * expf(-0.5f * dTheta * dTheta / biSq);
    }
    
    // Restauración a línea base (constante de tiempo ~1)
    zDot -= (s.z - ECG_BASELINE);
    
    // Elevación/depresión ST en el segmento correcto
    // ST segment: entre S wave (~20°) y T wave (~80°)
    if (fabsf(morphology.stElevation) > 0.001f) {
        if (theta > ST_SEGMENT_START && theta < ST_SEGMENT_END) {
            // Posición normalizada en segmento ST
            float stPosition = (theta - ST_SEGMENT_START) / (ST_SEGMENT_END - ST_SEGMENT_START);
            
            // Envolvente suave sin²(x*π) para transiciones naturales
            float envelope = sinf(stPosition * PI);
            envelope = envelope * envelope;
            
            zDot += morphology.stElevation * envelope * 0.5f;
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
 */
float ECGModel::getCurrentBPM() const {
    if (useVFibModel) {
        return 0.0f;  // VFib no tiene BPM definible
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
        case ECGCondition::BUNDLE_BRANCH_BLOCK:   return "BRD/BRI";
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
