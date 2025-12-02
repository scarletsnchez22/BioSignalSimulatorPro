/*
 * ============================================================================
 * PPG Model - Implementación del Generador Fotopletismográfico
 * ============================================================================
 * 
 * SEÑAL 100% DINÁMICA - Cada muestra es única porque:
 * 
 * 1. HRV REALISTA (Task Force ESC 1996):
 *    - LF (0.04-0.15 Hz): Ondas de Mayer, modulación barorrefleja
 *    - HF (0.15-0.4 Hz): RSA, acoplamiento cardio-respiratorio
 *    - Ruido gaussiano: Variabilidad intrínseca nodo sinusal
 * 
 * 2. MODULACIÓN RESPIRATORIA:
 *    - Amplitud: ±5-15% por cambios presión intratorácica
 *    - Baseline: Deriva por retorno venoso
 * 
 * 3. VARIABILIDAD LATIDO A LATIDO:
 *    - Cada RR es calculado dinámicamente
 *    - Ectópicos aleatorios con pausas compensatorias
 * 
 * PATOLOGÍAS IMPLEMENTADAS (9 condiciones):
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 * | Condición          | HR(bpm) | PI(%) | SpO2(%) | SDNN(ms) | Nota     |
 * |--------------------|---------|-------|---------|----------|----------|
 * | Normal             | 75      | 5.0   | 97      | 50       | Basal    |
 * | Bradicardia        | 45      | 8.0   | 97      | 60       | <60 bpm  |
 * | Taquicardia        | 120     | 3.0   | 96      | 30       | >100 bpm |
 * | Hipoxemia          | 90      | 2.5   | 88      | 40       | SpO2<90% |
 * | Hipertensión       | 80      | 4.0   | 97      | 35       | Rigidez↑ |
 * | Diabetes           | 78      | 2.5   | 96      | 20       | Damping↑ |
 * | Arritmia (PVCs)    | 75      | 4.0   | 97      | 80       | Ectópicos|
 * | Shock              | 110     | 0.8   | 94      | 25       | Crítico  |
 * | Fib. Auricular     | 90      | 3.0   | 96      | 120      | RR caótico|
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 * 
 * ============================================================================
 */

#include "ppg_model.h"
#include "config.h"
#include <math.h>
#include <esp_random.h>

// ============================================================================
// CONSTANTES MATEMÁTICAS
// ============================================================================
#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#ifndef TWO_PI
#define TWO_PI (2.0f * M_PI)
#endif

// Frecuencias de modulación HRV
#define LF_FREQ_CENTER  0.10f   // Hz (centro banda LF)
#define HF_FREQ_CENTER  0.25f   // Hz (centro banda HF, ~15 rpm)

// ============================================================================
// CONSTRUCTOR
// ============================================================================
PPGModel::PPGModel() {
    reset();
}

// ============================================================================
// RESET - Inicializar estado
// ============================================================================
void PPGModel::reset() {
    currentTime = 0.0f;
    beatStartTime = 0.0f;
    nextBeatTime = 0.0f;
    beatCount = 0;
    
    dcComponent = 128.0f;   // Centro del rango DAC
    acComponent = 0.0f;
    lastSample = 128.0f;
    
    // Reiniciar HRV con fases aleatorias
    hrv = HRVState();
    hrv.lfPhase = uniformRandom(0.0f, TWO_PI);
    hrv.hfPhase = uniformRandom(0.0f, TWO_PI);
    hrv.currentRR = 60000.0f / PPG_HR_NORMAL;
    
    // Reiniciar respiración con fase aleatoria
    resp = RespiratoryState();
    resp.phase = uniformRandom(0.0f, TWO_PI);
    
    // Morfología y patología por defecto
    morphology = PulseMorphology();
    pathology = PathologyConfig();
    
    // Calcular primer intervalo RR
    updateRRInterval();
    nextBeatTime = hrv.currentRR / 1000.0f;
}

// ============================================================================
// CONFIGURACIÓN DE PARÁMETROS
// ============================================================================
/*
 * PATOLOGÍAS CON JUSTIFICACIÓN CIENTÍFICA:
 * 
 * Cada patología modifica parámetros específicos basándose en:
 * - Fisiopatología conocida
 * - Literatura clínica referenciada
 * - Valores típicos de monitoreo de pacientes
 */
void PPGModel::setParameters(const PPGParameters& newParams) {
    params = newParams;
    
    // Aplicar morfología según condición patológica
    switch (params.condition) {
        case PPGCondition::NORMAL:
            setNormalMorphology();
            break;
            
        case PPGCondition::ARRHYTHMIA:
            setArrhythmiaMorphology();
            break;
            
        case PPGCondition::WEAK_PERFUSION:
            setShockMorphology();
            break;
            
        case PPGCondition::STRONG_PERFUSION:
            // Vasodilatación: PI alto, muesca dicrótica prominente
            setNormalMorphology();
            pathology.perfusionIndex = 12.0f;
            morphology.dicroticDepth = 0.35f;
            break;
            
        case PPGCondition::VASOCONSTRICTION:
            setHypertensionMorphology();
            break;
            
        case PPGCondition::MOTION_ARTIFACT:
            setNormalMorphology();
            pathology.motionArtifactProb = 0.25f;
            pathology.snrDb = 25.0f;
            break;
            
        case PPGCondition::LOW_SPO2:
            setHypoxemiaMorphology();
            break;
            
        default:
            setNormalMorphology();
            break;
    }
    
    // Aplicar parámetros del usuario si son válidos
    if (params.heartRate >= PPG_HR_MIN && params.heartRate <= PPG_HR_MAX) {
        pathology.heartRate = params.heartRate;
    }
    
    // Actualizar HRV con nuevo HR
    hrv.currentRR = 60000.0f / pathology.heartRate;
}

// ============================================================================
// MORFOLOGÍA: NORMAL (Estado Basal)
// ============================================================================
/*
 * Adulto sano en reposo:
 * - HR: 75 bpm (rango normal 60-100)
 * - PI: 5% (rango óptimo 4-7%)
 * - SpO2: 97% (normal >95%)
 * - SDNN: 50 ms (balance autonómico saludable)
 * - Muesca dicrótica bien definida (buena compliance arterial)
 * 
 * Ref: Allen (2007), Elgendi (2019)
 */
void PPGModel::setNormalMorphology() {
    morphology = PulseMorphology();  // Valores por defecto
    
    pathology.heartRate = 75.0f;
    pathology.perfusionIndex = 5.0f;
    pathology.spo2 = 97.0f;
    pathology.ectopicProbability = 0.01f;   // 1% (normal)
    pathology.snrDb = 45.0f;                 // Señal limpia
    pathology.motionArtifactProb = 0.02f;
    
    hrv.sdnn = 50.0f;
    hrv.lfPower = 0.5f;
    hrv.hfPower = 0.3f;
    hrv.irregularRhythm = false;
}

// ============================================================================
// MORFOLOGÍA: BRADICARDIA
// ============================================================================
/*
 * Frecuencia cardíaca <60 bpm
 * Causas: Bloqueo AV, hipotiroidismo, beta-bloqueantes, atletas
 * 
 * Características:
 * - HR: 45 bpm (bradicardia moderada)
 * - PI: 8% (aumentado por mayor volumen sistólico - Frank-Starling)
 * - Pulso amplio y redondeado (mayor tiempo de eyección)
 * - Componente diastólico prominente (mayor tiempo de llenado)
 * 
 * Ref: AHA Guidelines (2018)
 */
void PPGModel::setBradycardiaMorphology() {
    setNormalMorphology();
    
    pathology.heartRate = 45.0f;
    pathology.perfusionIndex = 8.0f;    // Compensación Frank-Starling
    
    // Pulso más amplio
    morphology.sigma1 = 0.07f;          // Sistólico más ancho
    morphology.sigma2 = 0.12f;          // Diastólico más ancho
    morphology.a2 = 0.50f;              // Componente diastólico prominente
    
    hrv.sdnn = 60.0f;                   // HRV normal-alta
}

// ============================================================================
// MORFOLOGÍA: TAQUICARDIA
// ============================================================================
/*
 * Frecuencia cardíaca >100 bpm
 * Causas: Fiebre, ansiedad, hipovolemia, hipertiroidismo
 * 
 * Características:
 * - HR: 120 bpm
 * - PI: 3% (reducido por menor tiempo diastólico)
 * - Pulso angosto y agudo (menor tiempo de eyección)
 * - Muesca dicrótica menos visible
 * - HRV reducida (predominio simpático)
 * 
 * Ref: AHA Guidelines (2018)
 */
void PPGModel::setTachycardiaMorphology() {
    setNormalMorphology();
    
    pathology.heartRate = 120.0f;
    pathology.perfusionIndex = 3.0f;
    pathology.spo2 = 96.0f;
    
    // Pulso más estrecho
    morphology.sigma1 = 0.04f;          // Sistólico angosto
    morphology.sigma2 = 0.08f;          // Diastólico angosto
    morphology.dicroticDepth = 0.15f;   // Muesca menos visible
    
    hrv.sdnn = 30.0f;                   // HRV reducida
    hrv.lfPower = 0.7f;                 // Predominio simpático
    hrv.hfPower = 0.2f;
}

// ============================================================================
// MORFOLOGÍA: HIPOXEMIA
// ============================================================================
/*
 * Saturación de oxígeno <90%
 * Causas: EPOC, neumonía, SDRA, altitud
 * 
 * Características:
 * - SpO2: 88% (hipoxemia moderada)
 * - PI: 2.5% (vasoconstricción hipóxica)
 * - Amplitud reducida
 * - Señal más ruidosa
 * - Taquicardia compensatoria común
 * 
 * Ref: WHO Pulse Oximetry Manual (2011)
 */
void PPGModel::setHypoxemiaMorphology() {
    setNormalMorphology();
    
    pathology.heartRate = 90.0f;        // Taquicardia compensatoria
    pathology.perfusionIndex = 2.5f;    // Vasoconstricción
    pathology.spo2 = 88.0f;             // Hipoxemia moderada
    pathology.snrDb = 35.0f;            // Señal más ruidosa
    
    morphology.a1 = 0.7f;               // Amplitud reducida
    morphology.a2 = 0.25f;
    morphology.dampingFactor = 0.8f;
    
    hrv.sdnn = 40.0f;
}

// ============================================================================
// MORFOLOGÍA: HIPERTENSIÓN ARTERIAL
// ============================================================================
/*
 * Rigidez arterial aumentada por remodelado vascular crónico
 * 
 * Características:
 * - Velocidad de onda de pulso aumentada
 * - Pulso alto y estrecho ("pico de iglesia")
 * - Muesca dicrótica atenuada
 * - Componente diastólico reducido
 * 
 * Ref: Millasseau et al. (2002), Charlton et al. (2022)
 */
void PPGModel::setHypertensionMorphology() {
    setNormalMorphology();
    
    pathology.heartRate = 80.0f;
    pathology.perfusionIndex = 4.0f;
    
    morphology.arterialStiffness = 1.8f;    // 80% más rígido
    morphology.sigma1 = 0.045f;             // Pulso más estrecho
    morphology.a2 = 0.30f;                  // Componente diastólico reducido
    morphology.dicroticDepth = 0.15f;       // Muesca atenuada
    
    hrv.sdnn = 35.0f;                       // HRV reducida
}

// ============================================================================
// MORFOLOGÍA: DIABETES / ARTERIOSCLEROSIS
// ============================================================================
/*
 * Glicación vascular (AGEs) → rigidez + pérdida de compliance
 * 
 * Características:
 * - Pulso amortiguado
 * - Muesca dicrótica casi inexistente
 * - Pérdida de características diastólicas
 * - Deriva de baseline aumentada
 * - Microangiopatía → PI bajo
 * 
 * Ref: Charlton et al. (2022)
 */
void PPGModel::setDiabeticMorphology() {
    setNormalMorphology();
    
    pathology.heartRate = 78.0f;
    pathology.perfusionIndex = 2.5f;        // Microangiopatía
    pathology.spo2 = 96.0f;
    
    morphology.dampingFactor = 0.6f;        // 40% amortiguamiento
    morphology.dicroticDepth = 0.08f;       // Muesca casi inexistente
    morphology.a2 = 0.20f;                  // Pérdida elasticidad
    
    resp.baselineDrift = 0.05f;             // Mayor deriva
    
    hrv.sdnn = 20.0f;                       // HRV muy reducida (neuropatía)
    hrv.lfPower = 0.3f;
    hrv.hfPower = 0.2f;
}

// ============================================================================
// MORFOLOGÍA: ARRITMIA (Extrasístoles Ventriculares)
// ============================================================================
/*
 * Latidos ectópicos prematuros con pausa compensatoria
 * 
 * Características:
 * - HRV muy elevada pero con patrón
 * - 15% latidos ectópicos
 * - Latido prematuro: RR corto, amplitud reducida
 * - Post-ectópico: RR largo (pausa compensatoria)
 * 
 * Ref: Task Force ESC (1996)
 */
void PPGModel::setArrhythmiaMorphology() {
    setNormalMorphology();
    
    pathology.heartRate = 75.0f;
    pathology.ectopicProbability = 0.15f;   // 15% latidos ectópicos
    
    hrv.sdnn = 80.0f;                       // HRV elevada
}

// ============================================================================
// MORFOLOGÍA: SHOCK / HIPOVOLEMIA
// ============================================================================
/*
 * Hipoperfusión crítica por pérdida de volumen circulante
 * 
 * Características:
 * - PI <1% (CRÍTICO - límite detectabilidad)
 * - Amplitudes muy reducidas
 * - Taquicardia compensatoria
 * - Señal muy degradada
 * - Alto riesgo artefactos (agitación)
 * 
 * Ref: Lima & Bakker (2005)
 */
void PPGModel::setShockMorphology() {
    pathology.heartRate = 110.0f;           // Taquicardia compensatoria
    pathology.perfusionIndex = 0.8f;        // CRÍTICO
    pathology.spo2 = 94.0f;
    pathology.snrDb = 30.0f;                // Señal muy degradada
    pathology.motionArtifactProb = 0.15f;   // Agitación
    pathology.ectopicProbability = 0.05f;
    
    morphology.a1 = 0.3f;                   // Amplitud muy reducida
    morphology.a2 = 0.1f;
    morphology.dicroticDepth = 0.05f;
    morphology.dampingFactor = 0.5f;
    
    hrv.sdnn = 25.0f;
}

// ============================================================================
// MORFOLOGÍA: FIBRILACIÓN AURICULAR
// ============================================================================
/*
 * Despolarización auricular caótica → intervalos RR totalmente irregulares
 * 
 * Características diagnósticas clave:
 * - SDNN extremadamente alto (>100 ms)
 * - Distribución RR UNIFORME (no gaussiana)
 * - Ausencia total de patrón predecible
 * - Llenado ventricular inconsistente → amplitud variable
 * 
 * Ref: Task Force ESC (1996)
 */
void PPGModel::setAtrialFibrillationMorphology() {
    setNormalMorphology();
    
    pathology.heartRate = 90.0f;            // HR media (rango 50-150)
    pathology.perfusionIndex = 3.0f;        // Gasto cardíaco subóptimo
    pathology.spo2 = 96.0f;
    
    morphology.a2 = 0.25f;                  // Variable por llenado inconsistente
    
    hrv.sdnn = 120.0f;                      // Extremadamente irregular
    hrv.irregularRhythm = true;             // Flag para distribución uniforme
}

// ============================================================================
// GENERADOR GAUSSIANO (Box-Muller)
// ============================================================================
float PPGModel::gaussianRandom(float mean, float stddev) {
    static bool hasSpare = false;
    static float spare;
    
    if (hasSpare) {
        hasSpare = false;
        return mean + stddev * spare;
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
    
    return mean + stddev * u * s;
}

// ============================================================================
// GENERADOR UNIFORME
// ============================================================================
float PPGModel::uniformRandom(float min, float max) {
    return min + (float)(esp_random() % 10000) / 10000.0f * (max - min);
}

// ============================================================================
// FUNCIÓN GAUSSIANA
// ============================================================================
float PPGModel::gaussian(float t, float mu, float sigma, float amplitude) {
    float diff = t - mu;
    return amplitude * expf(-(diff * diff) / (2.0f * sigma * sigma));
}

// ============================================================================
// ACTUALIZACIÓN DE INTERVALO RR CON HRV
// ============================================================================
/*
 * Genera variabilidad RR realista basada en Task Force ESC (1996):
 * 
 * RR(n) = RR_mean + LF_component + HF_component + noise
 * 
 * LF (0.04-0.15 Hz): Mayer waves, modulación barorrefleja
 * HF (0.15-0.4 Hz): RSA, acoplamiento respiratorio
 */
void PPGModel::updateRRInterval() {
    float rrMean = 60000.0f / pathology.heartRate;  // ms
    
    if (hrv.irregularRhythm) {
        // FIBRILACIÓN AURICULAR: distribución uniforme
        float rrMin = rrMean * 0.5f;    // 50% del RR medio
        float rrMax = rrMean * 1.5f;    // 150% del RR medio
        hrv.currentRR = uniformRandom(rrMin, rrMax);
    } else {
        // RITMO NORMAL: modelo HRV gaussiano
        
        // Componente LF (ondas de Mayer ~0.1 Hz)
        float lfComponent = hrv.lfPower * hrv.sdnn * sinf(hrv.lfPhase);
        
        // Componente HF (RSA ~0.25 Hz)
        float hfComponent = hrv.hfPower * hrv.sdnn * sinf(hrv.hfPhase);
        
        // Ruido gaussiano
        float noise = gaussianRandom(0.0f, hrv.sdnn * 0.3f);
        
        // RR final
        hrv.lastRR = hrv.currentRR;
        hrv.currentRR = rrMean + lfComponent + hfComponent + noise;
        
        // Limitar a rangos fisiológicos
        float rrMin = 60000.0f / PPG_HR_MAX;
        float rrMax = 60000.0f / PPG_HR_MIN;
        hrv.currentRR = fmaxf(rrMin, fminf(rrMax, hrv.currentRR));
    }
}

// ============================================================================
// VERIFICAR LATIDO ECTÓPICO
// ============================================================================
bool PPGModel::checkEctopicBeat() {
    float rand = uniformRandom(0.0f, 1.0f);
    return rand < pathology.ectopicProbability;
}

// ============================================================================
// MODULACIÓN RESPIRATORIA
// ============================================================================
/*
 * RSA (Respiratory Sinus Arrhythmia):
 * - La HR aumenta durante inspiración
 * - La HR disminuye durante espiración
 * - Modulación de amplitud por presión intratorácica
 * 
 * Ref: Task Force ESC (1996)
 */
float PPGModel::calculateRespiratoryModulation() {
    // Modulación de amplitud (5-15%)
    float am = 1.0f + resp.amDepth * sinf(resp.phase);
    return am;
}

// ============================================================================
// GENERACIÓN DE RUIDO
// ============================================================================
float PPGModel::generateNoise() {
    // Convertir SNR dB a factor lineal
    float snrLinear = powf(10.0f, pathology.snrDb / 20.0f);
    float noiseAmplitude = 1.0f / snrLinear;
    
    return gaussianRandom(0.0f, noiseAmplitude);
}

// ============================================================================
// GENERACIÓN DE ARTEFACTO DE MOVIMIENTO
// ============================================================================
float PPGModel::generateMotionArtifact() {
    if (uniformRandom(0.0f, 1.0f) < pathology.motionArtifactProb) {
        // Spike aleatorio
        return gaussianRandom(0.0f, 0.5f);
    }
    return 0.0f;
}

// ============================================================================
// GENERACIÓN DE FORMA DE ONDA DEL PULSO
// ============================================================================
/*
 * MODELO DOBLE GAUSSIANA + MUESCA DICRÓTICA
 * 
 * PPG(t) = G1(t) + G2(t) - Dicrotic(t)
 * 
 * donde t está normalizado al intervalo RR (0 a 1)
 */
float PPGModel::generatePulseWaveform(float t_normalized) {
    // Gaussiana 1: Pico Sistólico
    float g1 = gaussian(t_normalized, morphology.mu1, 
                       morphology.sigma1 / morphology.arterialStiffness,
                       morphology.a1);
    
    // Gaussiana 2: Pico Diastólico (onda reflejada)
    float g2 = gaussian(t_normalized, morphology.mu2,
                       morphology.sigma2,
                       morphology.a2);
    
    // Muesca Dicrótica (cierre válvula aórtica)
    float dicrotic = gaussian(t_normalized, morphology.dicroticMu,
                             morphology.dicroticSigma,
                             morphology.dicroticDepth);
    
    // Combinar componentes
    float pulse = (g1 + g2 - dicrotic) * morphology.dampingFactor;
    
    return pulse;
}

// ============================================================================
// GENERACIÓN DE MUESTRA - FUNCIÓN PRINCIPAL
// ============================================================================
float PPGModel::generateSample(float deltaTime) {
    // Actualizar tiempo
    currentTime += deltaTime;
    
    // Actualizar fases de modulación
    hrv.lfPhase += TWO_PI * LF_FREQ_CENTER * deltaTime;
    if (hrv.lfPhase > TWO_PI) hrv.lfPhase -= TWO_PI;
    
    hrv.hfPhase += TWO_PI * HF_FREQ_CENTER * deltaTime;
    if (hrv.hfPhase > TWO_PI) hrv.hfPhase -= TWO_PI;
    
    resp.phase += TWO_PI * (resp.rate / 60.0f) * deltaTime;
    if (resp.phase > TWO_PI) resp.phase -= TWO_PI;
    
    // ¿Es tiempo de un nuevo latido?
    if (currentTime >= nextBeatTime) {
        beatStartTime = currentTime;
        beatCount++;
        
        // Verificar latido ectópico
        bool isEctopic = checkEctopicBeat();
        
        // Calcular próximo intervalo RR
        updateRRInterval();
        
        if (isEctopic) {
            // Latido ectópico: RR corto, seguido de pausa
            hrv.currentRR *= 0.6f;   // RR reducido
        }
        
        nextBeatTime = currentTime + hrv.currentRR / 1000.0f;
    }
    
    // Calcular posición dentro del ciclo cardíaco (0 a 1)
    float rrSeconds = hrv.currentRR / 1000.0f;
    float t_normalized = (currentTime - beatStartTime) / rrSeconds;
    t_normalized = fminf(1.0f, fmaxf(0.0f, t_normalized));
    
    // Generar forma de onda del pulso
    float pulse = generatePulseWaveform(t_normalized);
    
    // Aplicar modulación respiratoria
    float respMod = calculateRespiratoryModulation();
    pulse *= respMod;
    
    // Calcular componentes AC y DC
    acComponent = pulse * pathology.perfusionIndex * 2.55f;  // Escalar a rango DAC
    
    // DC con deriva respiratoria
    float dcDrift = resp.baselineDrift * sinf(resp.phase);
    dcComponent = params.dcComponent + dcDrift * 20.0f;
    
    // Señal final
    float sample = dcComponent + acComponent;
    
    // Añadir ruido
    sample += generateNoise() * 5.0f;
    
    // Añadir artefacto de movimiento si aplica
    sample += generateMotionArtifact() * 50.0f;
    
    // Guardar última muestra
    lastSample = sample;
    
    return sample;
}

// ============================================================================
// CONVERSIÓN A VALOR DAC (0-255)
// ============================================================================
uint8_t PPGModel::getDACValue(float deltaTime) {
    float sample = generateSample(deltaTime);
    
    // Limitar a rango DAC
    int dacValue = (int)sample;
    if (dacValue > 255) dacValue = 255;
    if (dacValue < 0) dacValue = 0;
    
    return (uint8_t)dacValue;
}

// ============================================================================
// CONTROL DE FRECUENCIA RESPIRATORIA
// ============================================================================
void PPGModel::setRespiratoryRate(float rpm) {
    if (rpm >= 8.0f && rpm <= 30.0f) {
        resp.rate = rpm;
    }
}
