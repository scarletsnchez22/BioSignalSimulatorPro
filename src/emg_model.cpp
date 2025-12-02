/*
 * ============================================================================
 * EMG Model - Implementación de Alta Fidelidad para ESP32
 * ============================================================================
 * 
 * SEÑAL 100% DINÁMICA - La señal NUNCA se repite porque:
 * 
 * 1. VARIABILIDAD ISI (Inter-Spike Interval):
 *    - Cada MU calcula su próximo spike con: ISI = mean ± 20% variación
 *    - Usa distribución gaussiana (Box-Muller) para naturalidad
 * 
 * 2. ASINCRONÍA ENTRE MUs:
 *    - Las 100 MUs disparan independientemente
 *    - Desfase inicial aleatorio (0-100ms) para cada MU
 *    - Nunca hay dos patrones idénticos
 * 
 * 3. SUPERPOSICIÓN DINÁMICA:
 *    - Múltiples MUAPs se superponen en cada instante
 *    - La combinación cambia constantemente
 * 
 * NIVELES DE CONTRACCIÓN (% MVC - Maximum Voluntary Contraction):
 * - REPOSO:    0%   - Solo ruido de fondo
 * - LEVE:     20%   - ~20 MUs activas, tareas de precisión
 * - MODERADA: 50%   - ~50 MUs activas, sostener objetos
 * - FUERTE:   80%   - ~80 MUs activas, levantar peso
 * - MÁXIMA:  100%   - 100 MUs activas, esfuerzo máximo
 * 
 * ============================================================================
 */

#include "emg_model.h"
#include "config.h"
#include <math.h>
#include <esp_random.h>

// ============================================================================
// CONSTRUCTOR
// ============================================================================
EMGModel::EMGModel() {
    reset();
}

// ============================================================================
// RESET
// ============================================================================
void EMGModel::reset() {
    currentTime = 0.0f;
    currentExcitation = 0.0f;
    targetExcitation = 0.0f;
    rampSpeed = 0.15f;
    
    hasSpareGaussian = false;
    spareGaussian = 0.0f;
    
    activeMUCount = 0;
    avgFiringRate = 0.0f;
    rmsAccumulator = 0.0f;
    sampleCount = 0;
    
    fatigue.enabled = false;
    fatigue.level = 0.0f;
    fatigue.startTime = 0;
    fatigue.rateDecay = 1.0f;
    fatigue.amplitudeDecay = 1.0f;
    
    // Inicializar Motor Units
    initializeMotorUnits();
}

// ============================================================================
// CONFIGURACIÓN DE PARÁMETROS
// ============================================================================
/*
 * NIVELES DE CONTRACCIÓN JUSTIFICADOS:
 * 
 * | Condición      | %MVC | MUs Activas | Aplicación Clínica           |
 * |----------------|------|-------------|------------------------------|
 * | REPOSO         |  0%  |     0       | Línea base, ruido fondo      |
 * | LEVE (20%)     | 20%  |   ~20       | Escribir, teclear, precisión |
 * | MODERADA (50%) | 50%  |   ~50       | Sostener objetos, postura    |
 * | FUERTE (80%)   | 80%  |   ~80       | Levantar peso moderado       |
 * | MÁXIMA (100%)  | 100% |   100       | Esfuerzo máximo voluntario   |
 * 
 * PATOLOGÍAS:
 * - MIOPATÍA: MUAPs pequeños/cortos (daño muscular)
 * - NEUROPATÍA: MUAPs gigantes (reinervación colateral)
 * - TREMOR: Oscilación 4-6 Hz (Parkinson típico)
 * - FASCICULACIÓN: Disparos espontáneos aleatorios
 * - FATIGA: Reducción progresiva de amplitud y frecuencia
 */
void EMGModel::setParameters(const EMGParameters& newParams) {
    params = newParams;
    
    // Resetear patologías primero
    resetPathology();
    
    // Configurar según condición
    switch (params.condition) {
        // ============ CONTRACCIONES NORMALES ============
        case EMGCondition::NORMAL:
            // Reposo: 0% MVC - Solo ruido de fondo
            targetExcitation = 0.0f;
            break;
            
        case EMGCondition::MILD_CONTRACTION:
            // Contracción LEVE: 20% MVC
            // Ref: Tareas de precisión como escribir
            // ~20 MUs reclutadas (las más pequeñas, tipo I)
            targetExcitation = 0.20f;
            break;
            
        case EMGCondition::MODERATE_CONTRACTION:
            // Contracción MODERADA: 50% MVC  
            // Ref: Sostener objetos, mantener postura
            // ~50 MUs reclutadas (mezcla tipo I y II)
            targetExcitation = 0.50f;
            break;
            
        case EMGCondition::STRONG_CONTRACTION:
            // Contracción FUERTE: 80% MVC
            // Ref: Levantar peso moderado
            // ~80 MUs reclutadas
            targetExcitation = 0.80f;
            break;
            
        case EMGCondition::MAXIMUM_CONTRACTION:
            // Contracción MÁXIMA: 100% MVC
            // Ref: Esfuerzo máximo voluntario
            // 100 MUs reclutadas, todas al máximo firing rate
            targetExcitation = 1.0f;
            break;
            
        // ============ PATOLOGÍAS ============
        case EMGCondition::TREMOR:
            // Tremor Parkinsoniano: 4-6 Hz
            // Ref: Deuschl et al. (1998) Movement Disorders
            // Contracción base ~30% con oscilación superpuesta
            targetExcitation = 0.30f;
            break;
            
        case EMGCondition::MYOPATHY:
            // Miopatía: Daño muscular (distrofia, miositis)
            // Ref: Dumitru (2002) Electrodiagnostic Medicine
            // - MUAPs pequeños y cortos (pérdida de fibras)
            // - Reclutamiento temprano (compensación)
            // Demostrar al 50% MVC para comparar con normal
            targetExcitation = 0.50f;
            applyMyopathy();
            break;
            
        case EMGCondition::NEUROPATHY:
            // Neuropatía: Daño nervioso con reinervación
            // Ref: Kimura (2013) Electrodiagnosis in Diseases
            // - 40% MUs denervadas (no responden)
            // - MUs restantes más grandes (reinervación colateral)
            // - Firing rate reducido
            targetExcitation = 0.50f;
            applyNeuropathy();
            break;
            
        case EMGCondition::FASCICULATION:
            // Fasciculaciones: Disparos espontáneos
            // Ref: De Carvalho et al. (2017) Clin Neurophysiol
            // Sin contracción voluntaria, solo disparos aleatorios
            targetExcitation = 0.0f;
            break;
            
        case EMGCondition::FATIGUE:
            // Fatiga muscular progresiva
            // Ref: Cifrek et al. (2009) J Electromyogr Kinesiol
            // Contracción sostenida al 60% con deterioro
            targetExcitation = 0.60f;
            enableFatigue(true);
            break;
            
        default:
            targetExcitation = 0.0f;
            break;
    }
}

// ============================================================================
// INICIALIZACIÓN DE MOTOR UNITS (Henneman's Size Principle)
// ============================================================================
/*
 * Principio de Henneman (1957):
 * Las MUs se reclutan en orden de tamaño, de menor a mayor.
 * MUs pequeñas (tipo I, resistentes a fatiga) primero.
 * MUs grandes (tipo II, fuerza máxima) al final.
 */
void EMGModel::initializeMotorUnits() {
    for (uint8_t i = 0; i < NUM_MOTOR_UNITS; i++) {
        MotorUnit* mu = &motorUnits[i];
        float normalized_i = (float)i / (NUM_MOTOR_UNITS - 1);  // 0.0 a 1.0
        
        // ================================================================
        // 1. UMBRAL DE RECLUTAMIENTO - Función exponencial
        // ================================================================
        // Ref [1]: Distribución logarítmica de umbrales
        // Primera MU: umbral bajo (~0.017), Última MU: umbral alto (0.5)
        mu->recruitmentThreshold = LAST_MU_THRESHOLD * 
                                   (expf(logf(RR_RANGE) * normalized_i) / RR_RANGE);
        
        // ================================================================
        // 2. FIRING RATES (pps - pulsos por segundo)
        // ================================================================
        // Ref [1], [3]: MUs pequeñas tienen mayor rango de firing rate
        // MU #1:  8-35 pps (27 pps de rango)
        // MU #100: 8-25 pps (17 pps de rango)
        mu->minFiringRate = MIN_FIRING_RATE;
        mu->maxFiringRate = MAX_FIRING_FIRST - 
                           (MAX_FIRING_FIRST - MAX_FIRING_LAST) * normalized_i;
        
        // ================================================================
        // 3. AMPLITUD MUAP
        // ================================================================
        // Ref [2]: Proporcional a número de fibras y profundidad
        // MUs grandes tienen más fibras = MUAP más grande
        // Factor de profundidad: MUs grandes tienden a ser más superficiales
        float depthFactor = 0.6f + 0.4f * (1.0f - normalized_i);
        mu->muapAmplitude = MIN_MUAP_AMPLITUDE + 
                           (MAX_MUAP_AMPLITUDE - MIN_MUAP_AMPLITUDE) * 
                           powf(normalized_i, 1.2f) * depthFactor;
        
        // ================================================================
        // 4. DURACIÓN MUAP
        // ================================================================
        // Ref [2]: 6-14 ms, proporcional al tamaño de la MU
        mu->muapDurationMs = MIN_MUAP_DURATION_MS + 
                            (MAX_MUAP_DURATION_MS - MIN_MUAP_DURATION_MS) * normalized_i;
        
        // ================================================================
        // 5. GENERAR FORMA DE ONDA MUAP
        // ================================================================
        generateMUAPWaveform(mu, i);
        
        // ================================================================
        // 6. ESTADO INICIAL - CRUCIAL PARA SEÑAL DINÁMICA
        // ================================================================
        mu->active = false;
        mu->currentFiringRate = 0.0f;
        
        // DESFASE INICIAL ALEATORIO (0 a 200 ms)
        // Esto asegura que las MUs nunca estén sincronizadas
        // y la señal sea diferente cada vez que se inicia
        mu->nextSpikeTime = (float)(esp_random() % 200) / 1000.0f;
        mu->lastSpikeTime = -1.0f;
    }
}

// ============================================================================
// GENERACIÓN DE FORMA DE ONDA MUAP (Modelo Bi-exponencial)
// ============================================================================
void EMGModel::generateMUAPWaveform(MotorUnit* mu, uint8_t index) {
    // Parámetros del modelo bi-exponencial
    float t_peak = 0.25f;       // Pico al 25% de duración
    float t_negative = 0.6f;    // Inicio fase negativa al 60%
    
    // Añadir variabilidad entre MUs
    float variation = gaussianRandom(0.0f, 0.05f);
    t_peak += variation;
    t_negative += variation * 0.5f;
    
    t_peak = constrain(t_peak, 0.15f, 0.35f);
    t_negative = constrain(t_negative, 0.5f, 0.7f);
    
    // Constantes de tiempo normalizadas
    float tau_rise = t_peak / 3.0f;
    float tau_fall = (1.0f - t_peak) / 4.0f;
    float tau_negative = (1.0f - t_negative) / 2.0f;
    
    float maxVal = 0.0f;
    float tempWaveform[MUAP_WAVEFORM_SAMPLES];
    
    for (uint8_t i = 0; i < MUAP_WAVEFORM_SAMPLES; i++) {
        float t = (float)i / (MUAP_WAVEFORM_SAMPLES - 1);  // 0 a 1 normalizado
        float value = 0.0f;
        
        if (t < t_peak) {
            // Fase ascendente
            value = 1.0f - expf(-t / tau_rise);
        }
        else if (t < t_negative) {
            // Fase descendente principal
            float t_rel = t - t_peak;
            value = expf(-t_rel / tau_fall);
        }
        else {
            // Fase negativa (componente dipolar)
            float t_rel = t - t_negative;
            value = -0.25f * (1.0f - expf(-t_rel / tau_negative));
        }
        
        // Añadir pequeñas oscilaciones secundarias (realismo)
        float secondary = 0.05f * sinf(2.0f * PI * t * 3.0f) * expf(-5.0f * t);
        value += secondary;
        
        tempWaveform[i] = value;
        if (fabsf(value) > maxVal) maxVal = fabsf(value);
    }
    
    // Normalizar a int8_t (-127 a 127)
    for (uint8_t i = 0; i < MUAP_WAVEFORM_SAMPLES; i++) {
        mu->muapWaveform[i] = (int8_t)(tempWaveform[i] / maxVal * 127.0f);
    }
}

// ============================================================================
// GENERADOR ALEATORIO GAUSSIANO (Box-Muller)
// ============================================================================
float EMGModel::gaussianRandom(float mean, float stddev) {
    if (hasSpareGaussian) {
        hasSpareGaussian = false;
        return mean + stddev * spareGaussian;
    }
    
    float u, v, s;
    do {
        u = (float)(esp_random() % 10000) / 5000.0f - 1.0f;
        v = (float)(esp_random() % 10000) / 5000.0f - 1.0f;
        s = u * u + v * v;
    } while (s >= 1.0f || s == 0.0f);
    
    s = sqrtf(-2.0f * logf(s) / s);
    spareGaussian = v * s;
    hasSpareGaussian = true;
    
    return mean + stddev * u * s;
}

// ============================================================================
// ACTUALIZACIÓN DE ESTADO DE MOTOR UNITS
// ============================================================================
void EMGModel::updateMotorUnitStates() {
    float effectiveExcitation = currentExcitation;
    
    // Aplicar fatiga si está activa
    if (fatigue.enabled && fatigue.level > 0.0f) {
        effectiveExcitation *= (1.0f - fatigue.level * 0.4f);
    }
    
    activeMUCount = 0;
    float totalRate = 0.0f;
    
    for (uint8_t i = 0; i < NUM_MOTOR_UNITS; i++) {
        MotorUnit* mu = &motorUnits[i];
        
        // Verificar reclutamiento
        if (effectiveExcitation >= mu->recruitmentThreshold) {
            mu->active = true;
            activeMUCount++;
            
            // Rate Coding: +3 pps por cada 10% de excitación sobre umbral
            float excitationAbove = (effectiveExcitation - mu->recruitmentThreshold) * 100.0f;
            mu->currentFiringRate = mu->minFiringRate + RATE_CODING_SLOPE * excitationAbove;
            
            // Limitar al máximo
            mu->currentFiringRate = fminf(mu->currentFiringRate, mu->maxFiringRate);
            
            // Aplicar efecto de fatiga
            if (fatigue.enabled) {
                mu->currentFiringRate *= fatigue.rateDecay;
            }
            
            totalRate += mu->currentFiringRate;
        } else {
            mu->active = false;
            mu->currentFiringRate = 0.0f;
        }
    }
    
    avgFiringRate = (activeMUCount > 0) ? totalRate / activeMUCount : 0.0f;
}

// ============================================================================
// GENERACIÓN DE MUESTRA EMG
// ============================================================================
float EMGModel::generateRawSample(float deltaTime) {
    float sample = 0.0f;
    
    // Sumar contribución de todas las MUs activas
    for (uint8_t i = 0; i < NUM_MOTOR_UNITS; i++) {
        MotorUnit* mu = &motorUnits[i];
        
        if (!mu->active || mu->currentFiringRate < 0.1f) continue;
        
        // Verificar si debe generar un spike
        if (currentTime >= mu->nextSpikeTime) {
            // Calcular ISI con variabilidad gaussiana (CoV = 20%)
            float meanISI = 1.0f / mu->currentFiringRate;
            float stdISI = meanISI * ISI_COV;
            float nextISI = gaussianRandom(meanISI, stdISI);
            
            // Limitar ISI (5-200 ms)
            nextISI = constrain(nextISI, 0.005f, 0.2f);
            
            mu->nextSpikeTime = currentTime + nextISI;
            mu->lastSpikeTime = currentTime;
        }
        
        // Contribución del MUAP si está activo
        float timeSinceSpike = currentTime - mu->lastSpikeTime;
        float muapDurationSec = mu->muapDurationMs / 1000.0f;
        
        if (timeSinceSpike >= 0.0f && timeSinceSpike < muapDurationSec) {
            // Calcular índice con interpolación lineal
            float normalizedTime = timeSinceSpike / muapDurationSec;
            float floatIdx = normalizedTime * (MUAP_WAVEFORM_SAMPLES - 1);
            uint8_t idx0 = (uint8_t)floatIdx;
            uint8_t idx1 = (idx0 < MUAP_WAVEFORM_SAMPLES - 1) ? idx0 + 1 : idx0;
            float frac = floatIdx - idx0;
            
            // Interpolación lineal entre samples
            float waveVal = mu->muapWaveform[idx0] * (1.0f - frac) + 
                           mu->muapWaveform[idx1] * frac;
            
            // Escalar por amplitud (int8 -> mV)
            float amplitude = mu->muapAmplitude;
            if (fatigue.enabled) {
                amplitude *= fatigue.amplitudeDecay;
            }
            
            sample += (waveVal / 127.0f) * amplitude;
        }
    }
    
    return sample;
}

float EMGModel::generateSample(float deltaTime) {
    // Rampa suave hacia excitación objetivo
    if (fabsf(currentExcitation - targetExcitation) > 0.001f) {
        currentExcitation += (targetExcitation - currentExcitation) * rampSpeed * deltaTime * 10.0f;
    }
    
    // Actualizar fatiga si está activa
    if (fatigue.enabled) {
        float elapsedSec = (millis() - fatigue.startTime) / 1000.0f;
        fatigue.level = fminf(0.6f, elapsedSec / 50.0f);  // Máximo 60% en 50 segundos
        fatigue.rateDecay = 1.0f - fatigue.level * 0.3f;
        fatigue.amplitudeDecay = 1.0f - fatigue.level * 0.2f;
    }
    
    // Actualizar estado de MUs
    updateMotorUnitStates();
    
    // Generar muestra base
    float sample = generateRawSample(deltaTime);
    
    // Manejar condiciones especiales
    if (params.condition == EMGCondition::TREMOR) {
        // Tremor 4-6 Hz típico de Parkinson
        float tremorFreq = 5.0f + gaussianRandom(0.0f, 0.5f);
        float tremorMod = 0.3f * sinf(2.0f * PI * tremorFreq * currentTime);
        targetExcitation = 0.3f * (1.0f + tremorMod);
    }
    else if (params.condition == EMGCondition::FASCICULATION) {
        // Fasciculaciones: disparos espontáneos aleatorios
        static float nextFascTime = 0.0f;
        if (currentTime >= nextFascTime) {
            // Activar una MU aleatoria brevemente
            int randomMU = esp_random() % NUM_MOTOR_UNITS;
            motorUnits[randomMU].lastSpikeTime = currentTime;
            // Próxima fasciculación en 0.5-3 segundos
            nextFascTime = currentTime + 0.5f + (esp_random() % 2500) / 1000.0f;
        }
    }
    
    // Ruido de fondo (muy pequeño)
    sample += gaussianRandom(0.0f, 0.003f * params.noiseLevel);
    
    // Acumular para RMS
    rmsAccumulator += sample * sample;
    sampleCount++;
    
    currentTime += deltaTime;
    
    return sample;
}

// ============================================================================
// CONVERSIÓN A DAC
// ============================================================================
uint8_t EMGModel::voltageToDACValue(float voltage) {
    // EMG típico: -3mV a +3mV
    // Mapear a 0-255
    float normalized = (voltage + 3.0f) / 6.0f;
    normalized = constrain(normalized, 0.0f, 1.0f);
    
    int dacValue = (int)(normalized * 255.0f);
    return (uint8_t)constrain(dacValue, 0, 255);
}

uint8_t EMGModel::getDACValue(float deltaTime) {
    float voltage = generateSample(deltaTime);
    return voltageToDACValue(voltage);
}

// ============================================================================
// CONTROL DE EXCITACIÓN
// ============================================================================
void EMGModel::setExcitationLevel(float level) {
    targetExcitation = constrain(level, 0.0f, 1.0f);
}

// ============================================================================
// CONTROL DE FATIGA
// ============================================================================
void EMGModel::enableFatigue(bool enable) {
    fatigue.enabled = enable;
    if (enable) {
        fatigue.startTime = millis();
        fatigue.level = 0.0f;
    } else {
        fatigue.level = 0.0f;
        fatigue.rateDecay = 1.0f;
        fatigue.amplitudeDecay = 1.0f;
    }
}

// ============================================================================
// PATOLOGÍAS
// ============================================================================
void EMGModel::resetPathology() {
    // Re-inicializar MUs a valores normales
    initializeMotorUnits();
}

void EMGModel::applyMyopathy() {
    // Miopatía: MUAPs pequeños y cortos, más MUs reclutadas
    for (uint8_t i = 0; i < NUM_MOTOR_UNITS; i++) {
        motorUnits[i].muapAmplitude *= 0.4f;
        motorUnits[i].muapDurationMs *= 0.7f;
        motorUnits[i].recruitmentThreshold *= 0.7f;  // Reclutamiento más fácil
        generateMUAPWaveform(&motorUnits[i], i);
    }
}

void EMGModel::applyNeuropathy() {
    // Neuropatía: Denervación + Reinervación
    // 40% de MUs inactivas (denervación)
    uint8_t denervatedCount = NUM_MOTOR_UNITS * 0.4f;
    for (uint8_t i = 0; i < denervatedCount; i++) {
        motorUnits[i].recruitmentThreshold = 999.0f;  // Nunca se recluta
    }
    
    // MUs restantes más grandes (reinervación colateral)
    for (uint8_t i = denervatedCount; i < NUM_MOTOR_UNITS; i++) {
        motorUnits[i].muapAmplitude *= 2.0f;
        motorUnits[i].muapDurationMs *= 1.5f;
        motorUnits[i].maxFiringRate *= 0.75f;  // Firing rate reducido
        generateMUAPWaveform(&motorUnits[i], i);
    }
}

// ============================================================================
// ESTADÍSTICAS
// ============================================================================
float EMGModel::getRMS() {
    if (sampleCount == 0) return 0.0f;
    
    float rms = sqrtf(rmsAccumulator / sampleCount) * 1000.0f;  // Convertir a μV
    
    // Reset acumuladores
    rmsAccumulator = 0.0f;
    sampleCount = 0;
    
    return rms;
}
