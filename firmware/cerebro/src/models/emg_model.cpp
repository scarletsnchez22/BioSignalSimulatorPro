/**
 * @file emg_model.cpp
 * @brief Implementación del modelo EMG basado en reclutamiento de unidades motoras
 * @version 1.0.0
 * 
 * MODELO BASE:
 * Fuglevand AJ, Winter DA, Patla AE.
 * "Models of recruitment and rate coding organization in motor-unit pools."
 * J Neurophysiol. 1993;70(6):2470-2488.
 * 
 * PRINCIPIO DE RECLUTAMIENTO:
 * Henneman E, Somjen G, Carpenter DO.
 * "Functional significance of cell size in spinal motoneurons."
 * J Neurophysiol. 1965;28:560-580.
 * - Las unidades motoras pequeñas (bajo umbral) se reclutan primero
 * - Las unidades motoras grandes (alto umbral) se reclutan después
 * - El orden de reclutamiento es fijo y predecible
 * 
 * FRECUENCIAS DE DISPARO:
 * De Luca CJ, Hostage EC.
 * "Relationship between firing rate and recruitment threshold of motoneurons."
 * J Neurophysiol. 2010;104(2):1034-1046.
 * - Frecuencia mínima al reclutamiento: 6-8 Hz
 * - Frecuencia máxima en MVC: 30-50 Hz (hasta 60 Hz transitorio)
 * 
 * AMPLITUDES sEMG:
 * De Luca CJ.
 * "The use of surface electromyography in biomechanics."
 * J Appl Biomech. 1997;13:135-163.
 * - EMG de superficie típico: 50 µV - 5 mV
 * - MVC (contracción máxima): 1-5 mV RMS
 * 
 * VARIABILIDAD DE FUERZA:
 * Enoka RM, Christou EA, Hunter SK, et al.
 * "Mechanisms that contribute to differences in motor performance."
 * J Electromyogr Kinesiol. 2003;13(1):1-12.
 * - CV de fuerza en jóvenes: 2-5%
 * - CV de fuerza en mayores/fatiga: 5-10%
 */

#include "models/emg_model.h"
#include "config.h"
#include <math.h>
#include <esp_random.h>

// ============================================================================
// CONSTANTES DEL MODELO (basadas en literatura)
// ============================================================================

// Frecuencias de disparo (De Luca 2010)
static const float FIRING_RATE_MIN = 6.0f;      // Hz al reclutamiento
static const float FIRING_RATE_MAX = 50.0f;     // Hz en MVC
static const float FIRING_RATE_GAIN = 40.0f;    // Ganancia Hz/unidad de excitación

// Variabilidad del intervalo inter-spike (ISI)
// Típico CV = 15-25% según literatura
static const float ISI_VARIABILITY_CV = 0.20f;  // 20% coeficiente de variación

// Variabilidad de fuerza (fluctuación natural del control motor)
static const float FORCE_VARIABILITY_FREQ = 2.0f;   // Hz (1-3 Hz típico)
static const float FORCE_VARIABILITY_AMP = 0.04f;   // ±4% de fluctuación

// Escalado de salida
// Rango ±5 mV para capturar MUAPs gigantes en neuropatía
static const float EMG_MAX_MV = 5.0f;

// Parámetros del MUAP trifásico (normal)
static const float MUAP_SIGMA = 2.0f;           // ms (controla ancho)
static const float MUAP_DURATION = 12.0f;       // ms duración total

// Parámetros para patologías
static const float MYOPATHY_MUAP_DURATION = 6.0f;    // ms - MUAPs cortos en miopatía
static const float MYOPATHY_MUAP_SIGMA = 1.0f;       // ms - más estrechos
static const float NEUROPATHY_MUAP_DURATION = 20.0f; // ms - MUAPs largos en neuropatía
static const float NEUROPATHY_MUAP_SIGMA = 4.0f;     // ms - más anchos
static const float TREMOR_FREQUENCY = 5.0f;          // Hz - frecuencia del temblor
static const float TREMOR_BURST_DUTY = 0.35f;        // 35% del ciclo es burst activo

// ============================================================================
// CONSTRUCTOR
// ============================================================================
EMGModel::EMGModel() {
    hasPendingParams = false;
    forceVariabilityPhase = 0.0f;
    reset();
}

// ============================================================================
// RESET
// ============================================================================
void EMGModel::reset() {
    currentExcitation = 0.0f;
    baseExcitation = 0.0f;
    accumulatedTime = 0.0f;
    tremorPhase = 0.0f;
    fatigueLevel = 0.0f;
    forceVariabilityPhase = 0.0f;
    lastSampleValue = 0.0f;
    waveformGain = EMG_WAVEFORM_GAIN_DEFAULT;  // Ganancia para waveform
    
    // Variables para patologías
    fasciculationTimer = 0.0f;
    fasciculationBurstEnd = 0.0f;
    fasciculationActiveMU = -1;
    
    // Inicializar duraciones para patologías
    for (int i = 0; i < MAX_MOTOR_UNITS; i++) {
        // Miopatía: duraciones cortas con variabilidad (polifasia)
        myopathyDurations[i] = MYOPATHY_MUAP_DURATION * (0.7f + 0.6f * ((float)(esp_random() % 100) / 100.0f));
        // Neuropatía: duraciones largas
        neuropathyDurations[i] = NEUROPATHY_MUAP_DURATION * (0.8f + 0.4f * ((float)(esp_random() % 100) / 100.0f));
    }
    
    // Inicializar buffer RMS
    rmsBufferIndex = 0;
    rmsSum = 0.0f;
    for (int i = 0; i < RMS_BUFFER_SIZE; i++) {
        rmsBuffer[i] = 0.0f;
    }
    
    // Reset generador gaussiano Box-Muller
    gaussHasSpare = false;
    gaussSpare = 0.0f;
    
    initializeMotorUnits();
}

// ============================================================================
// INICIALIZACIÓN DE UNIDADES MOTORAS
// ============================================================================
/**
 * @brief Inicializa el pool de unidades motoras según el principio de Henneman
 * 
 * El principio de Henneman establece que las MUs se reclutan en orden de tamaño:
 * - MUs pequeñas (tipo I, lentas, resistentes a fatiga) → umbral bajo
 * - MUs grandes (tipo II, rápidas, fatigables) → umbral alto
 * 
 * La distribución de umbrales es exponencial según Fuglevand 1993:
 *   threshold_i = exp(ln(RR) × i/n) / RR
 * donde RR es el rango de reclutamiento (típico 30-60)
 */
void EMGModel::initializeMotorUnits() {
    // Rango de reclutamiento: las MUs se reclutan entre 0% y ~60% de excitación
    // (el resto del rango es para aumentar frecuencia de disparo)
    const float RECRUITMENT_RANGE = 60.0f;
    
    for (int i = 0; i < MAX_MOTOR_UNITS; i++) {
        float normalizedIndex = (float)i / (float)MAX_MOTOR_UNITS;
        
        // Umbral exponencial (Fuglevand 1993)
        // Las primeras MUs tienen umbral muy bajo, las últimas cercano a 0.6
        motorUnits[i].threshold = (expf(logf(RECRUITMENT_RANGE) * normalizedIndex) - 1.0f) 
                                   / (RECRUITMENT_RANGE - 1.0f) * 0.6f;
        
        // Amplitud proporcional al tamaño (principio del tamaño)
        // MUs grandes producen MUAPs más grandes
        // Rango: 0.3 mV (pequeñas) a 1.5 mV (grandes)
        motorUnits[i].amplitude = 0.3f + normalizedIndex * 1.2f;
        
        // Guardar amplitud base para restaurar después de patologías
        motorUnits[i].baseAmplitude = motorUnits[i].amplitude;
        
        // Frecuencia de disparo inicial
        motorUnits[i].firingRate = FIRING_RATE_MIN;
        
        // Tiempos de disparo
        motorUnits[i].lastFiringTime = -1.0f;
        motorUnits[i].nextFiringTime = gaussianRandom(0.0f, 0.1f);
        motorUnits[i].isActive = false;
    }
}

// ============================================================================
// RESTAURAR UNIDADES MOTORAS A VALORES BASE
// ============================================================================
/**
 * @brief Restaura todas las MUs a sus valores iniciales
 * 
 * IMPORTANTE: Debe llamarse ANTES de aplicar modificadores de condición
 * para evitar que cambios de condiciones anteriores persistan.
 */
void EMGModel::resetMotorUnitsToDefault() {
    for (int i = 0; i < MAX_MOTOR_UNITS; i++) {
        float normalizedIndex = (float)i / (float)MAX_MOTOR_UNITS;
        
        // Restaurar umbral
        const float RECRUITMENT_RANGE = 60.0f;
        motorUnits[i].threshold = (expf(logf(RECRUITMENT_RANGE) * normalizedIndex) - 1.0f) 
                                   / (RECRUITMENT_RANGE - 1.0f) * 0.6f;
        
        // Restaurar amplitud desde base guardada
        motorUnits[i].amplitude = motorUnits[i].baseAmplitude;
        
        // Resetear estado
        motorUnits[i].isActive = false;
    }
}

// ============================================================================
// CONFIGURACIÓN DE PARÁMETROS
// ============================================================================
void EMGModel::setParameters(const EMGParameters& newParams) {
    params = newParams;
    
    // IMPORTANTE: Restaurar MUs a valores base antes de aplicar nueva condición
    resetMotorUnitsToDefault();
    
    // Aplicar modificadores de la condición seleccionada
    applyConditionModifiers();
    
    // Guardar excitación base (será modulada por variabilidad)
    baseExcitation = currentExcitation;
}

void EMGModel::setPendingParameters(const EMGParameters& newParams) {
    pendingParams = newParams;
    hasPendingParams = true;
}

// ============================================================================
// MODIFICADORES POR CONDICIÓN
// ============================================================================
/**
 * @brief Aplica modificadores específicos según la condición EMG
 * 
 * Cada condición establece:
 * - Nivel de excitación (determina cuántas MUs se reclutan)
 * - Modificaciones morfológicas (miopatía, neuropatía)
 * - Comportamientos especiales (temblor, fasciculación)
 */
void EMGModel::applyConditionModifiers() {
    // Resetear variables de condiciones especiales
    tremorPhase = 0.0f;
    fatigueLevel = 0.0f;
    
    switch (params.condition) {
        case EMGCondition::REST:
            // Reposo: 0-5% MVC - mínima actividad muscular
            // RMS <0.05 mV, muy pocas MUs activas
            currentExcitation = constrain(params.excitationLevel, 0.0f, 0.05f);
            if (params.excitationLevel == 0.0f) currentExcitation = 0.02f;
            break;
            
        case EMGCondition::LOW_CONTRACTION:
            // Contracción baja: 5-20% MVC
            // RMS 0.1-0.4 mV, picos <1.0 mV
            currentExcitation = constrain(params.excitationLevel, 0.05f, 0.20f);
            if (params.excitationLevel == 0.0f) currentExcitation = 0.12f;
            break;
            
        case EMGCondition::MODERATE_CONTRACTION:
            // Contracción moderada: 20-50% MVC
            // RMS 0.5-1.2 mV, picos hasta ~2.0 mV
            currentExcitation = constrain(params.excitationLevel, 0.20f, 0.50f);
            if (params.excitationLevel == 0.0f) currentExcitation = 0.35f;
            break;
            
        case EMGCondition::HIGH_CONTRACTION:
            // Contracción alta: 50-100% MVC
            // RMS 1.5-5.0 mV, picos hasta 4-5 mV
            currentExcitation = constrain(params.excitationLevel, 0.50f, 1.0f);
            if (params.excitationLevel == 0.0f) currentExcitation = 0.75f;
            break;
            
        case EMGCondition::TREMOR:
            // Temblor tipo Parkinson (4-6 Hz)
            // Ref: Deuschl G. Mov Disord. 1998 - Frecuencia típica 4-6 Hz
            // La excitación se modula en generateSample() para crear bursts
            currentExcitation = 0.0f;  // Empieza en 0, se modula cíclicamente
            break;
            
        case EMGCondition::MYOPATHY:
            // Miopatía: daño muscular primario
            // Ref: Kimura J. Electrodiagnosis 4th ed. 2013
            // MUAPs pequeños (<500µV), polifásicos, duración corta
            // Reclutamiento precoz: muchas MUs para poca fuerza
            currentExcitation = 0.50f;  // Alta excitación para reclutamiento agresivo
            for (int i = 0; i < MAX_MOTOR_UNITS; i++) {
                // Amplitudes muy reducidas (30-50% del normal)
                motorUnits[i].amplitude = motorUnits[i].baseAmplitude * (0.25f + 0.2f * ((float)(i % 10) / 10.0f));
                // Umbrales más bajos = reclutamiento precoz
                motorUnits[i].threshold *= 0.6f;
            }
            break;
            
        case EMGCondition::NEUROPATHY:
            // Neuropatía: pérdida de MUs con reinervación colateral
            // Ref: Kimura J. Electrodiagnosis 4th ed. 2013
            // MUAPs gigantes (>2mV), duración larga, pocas MUs activas
            currentExcitation = 0.40f;
            for (int i = 0; i < MAX_MOTOR_UNITS; i++) {
                if (i % 4 != 0) {
                    // 75% de MUs perdidas (umbral inalcanzable)
                    motorUnits[i].threshold = 2.0f;
                    motorUnits[i].amplitude = 0.0f;
                } else {
                    // 25% sobreviven con reinervación: MUAPs gigantes (3-5x normal)
                    motorUnits[i].amplitude = motorUnits[i].baseAmplitude * (3.0f + 2.0f * ((float)(i % 5) / 5.0f));
                    motorUnits[i].threshold *= 0.8f;  // Umbral ligeramente más bajo
                }
            }
            break;
            
        case EMGCondition::FASCICULATION:
            // Fasciculaciones: disparos espontáneos aislados en reposo
            // Ref: Mills KR. J Neurol Neurosurg Psychiatry. 2010
            // Eventos discretos, 1-3 por segundo, sin patrón periódico
            currentExcitation = 0.0f;  // Base silenciosa
            // Programar primera fasciculación
            fasciculationTimer = accumulatedTime + 0.3f + ((float)(esp_random() % 1000) / 1000.0f);
            fasciculationActiveMU = -1;
            break;
            
        default:
            currentExcitation = 0.0f;
            break;
    }
}

// ============================================================================
// ACTUALIZACIÓN DE RECLUTAMIENTO
// ============================================================================
/**
 * @brief Actualiza el estado de reclutamiento de todas las MUs
 * 
 * Según De Luca 2010, la relación entre excitación y frecuencia de disparo es:
 *   FR = FR_min + gain × (excitation - threshold)
 * 
 * donde:
 *   - FR_min = 6-8 Hz (frecuencia al reclutamiento)
 *   - gain = ~40 Hz por unidad de excitación
 *   - FR_max = 50 Hz (puede llegar a 60 transitoriamente)
 */
void EMGModel::updateMotorUnitRecruitment() {
    for (int i = 0; i < MAX_MOTOR_UNITS; i++) {
        // Verificar si la MU debe activarse según su umbral
        if (currentExcitation >= motorUnits[i].threshold) {
            if (!motorUnits[i].isActive) {
                // Recién reclutada: programar primer disparo
                motorUnits[i].isActive = true;
                motorUnits[i].nextFiringTime = accumulatedTime + gaussianRandom(0.05f, 0.02f);
            }
            
            // Calcular frecuencia de disparo (De Luca 2010)
            float excitationAboveThreshold = currentExcitation - motorUnits[i].threshold;
            motorUnits[i].firingRate = FIRING_RATE_MIN + excitationAboveThreshold * FIRING_RATE_GAIN;
            
            // Limitar a rango fisiológico (6-50 Hz, hasta 60 en picos)
            motorUnits[i].firingRate = constrain(motorUnits[i].firingRate, 
                                                  FIRING_RATE_MIN, 
                                                  FIRING_RATE_MAX);
            
            // Nota: La condición FATIGUE fue eliminada del modelo
        } else {
            motorUnits[i].isActive = false;
        }
    }
}

// ============================================================================
// GENERACIÓN DE MUAP TRIFÁSICO
// ============================================================================
/**
 * @brief Genera un potencial de acción de unidad motora (MUAP) trifásico
 * 
 * Usa la segunda derivada de una Gaussiana para producir la forma
 * característica: pequeño positivo → grande negativo → pequeño positivo
 * 
 * La forma matemática es:
 *   MUAP(t) = A × (1 - ((t-t0)/σ)²) × exp(-((t-t0)²)/(2σ²))
 * 
 * Esta es la "Mexican Hat Wavelet" o wavelet de Ricker, usada
 * frecuentemente para modelar MUAPs en literatura.
 * 
 * @param timeSinceFiring Tiempo desde el último disparo (segundos)
 * @param amplitude Amplitud del MUAP (mV)
 * @return Valor del MUAP en este instante (mV)
 */
float EMGModel::generateMUAP(float timeSinceFiring, float amplitude) {
    // Convertir a milisegundos para trabajar con parámetros en ms
    float t_ms = timeSinceFiring * 1000.0f;
    
    // Fuera de la ventana de duración, no hay contribución
    if (t_ms < 0 || t_ms > MUAP_DURATION) {
        return 0.0f;
    }
    
    // Centrar el MUAP en la mitad de la duración
    float t0 = MUAP_DURATION / 2.0f;
    float t_centered = t_ms - t0;
    
    // Segunda derivada de Gaussiana (Mexican Hat Wavelet)
    // Normalizada para que el pico negativo tenga amplitud = amplitude
    float sigma_sq = MUAP_SIGMA * MUAP_SIGMA;
    float t_sq = t_centered * t_centered;
    
    float gaussian = expf(-t_sq / (2.0f * sigma_sq));
    float wavelet = (1.0f - t_sq / sigma_sq) * gaussian;
    
    // El factor -1 invierte para que el pico principal sea negativo
    // (convención en EMG: el pico más grande suele ser negativo)
    // Escalamos por 0.5 para normalizar la amplitud del pico
    return -amplitude * wavelet * 0.5f;
}

// ============================================================================
// GENERACIÓN DE MUAP CON DURACIÓN VARIABLE
// ============================================================================
/**
 * @brief Genera un MUAP con duración y sigma personalizados
 * 
 * Usado para neuropatía (MUAPs largos) y miopatía (MUAPs cortos)
 */
float EMGModel::generateMUAPWithDuration(float timeSinceFiring, float amplitude, 
                                          float duration, float sigma) {
    float t_ms = timeSinceFiring * 1000.0f;
    
    if (t_ms < 0 || t_ms > duration) {
        return 0.0f;
    }
    
    float t0 = duration / 2.0f;
    float t_centered = t_ms - t0;
    
    float sigma_sq = sigma * sigma;
    float t_sq = t_centered * t_centered;
    
    float gaussian = expf(-t_sq / (2.0f * sigma_sq));
    float wavelet = (1.0f - t_sq / sigma_sq) * gaussian;
    
    return -amplitude * wavelet * 0.5f;
}

// ============================================================================
// GENERACIÓN DE MUAP POLIFÁSICO (MIOPATÍA)
// ============================================================================
/**
 * @brief Genera un MUAP polifásico para miopatía
 * 
 * Los MUAPs miopáticos tienen múltiples fases pequeñas debido a:
 * - Pérdida de fibras musculares
 * - Regeneración con fibras inmaduras
 * - Desincronización de la activación
 */
float EMGModel::generatePolyphasicMUAP(float timeSinceFiring, float amplitude, int phases) {
    float t_ms = timeSinceFiring * 1000.0f;
    float duration = MYOPATHY_MUAP_DURATION;
    
    if (t_ms < 0 || t_ms > duration) {
        return 0.0f;
    }
    
    float signal = 0.0f;
    float phaseSpacing = duration / (float)(phases + 1);
    float sigma = MYOPATHY_MUAP_SIGMA * 0.7f;  // Más estrecho para cada fase
    
    // Generar múltiples picos pequeños
    for (int p = 0; p < phases; p++) {
        float t0 = phaseSpacing * (p + 1);
        float t_centered = t_ms - t0;
        float sigma_sq = sigma * sigma;
        float t_sq = t_centered * t_centered;
        
        float gaussian = expf(-t_sq / (2.0f * sigma_sq));
        float wavelet = (1.0f - t_sq / sigma_sq) * gaussian;
        
        // Alternar polaridad para crear patrón polifásico
        float polarity = (p % 2 == 0) ? 1.0f : -1.0f;
        signal += polarity * amplitude * wavelet * 0.3f;
    }
    
    return signal;
}

// ============================================================================
// GENERACIÓN DE MUESTRA
// ============================================================================
/**
 * @brief Genera una muestra de EMG
 * 
 * Proceso:
 * 1. Aplicar variabilidad natural de fuerza (fluctuación del control motor)
 * 2. Manejar condiciones especiales (temblor, fatiga, fasciculación)
 * 3. Actualizar reclutamiento de MUs
 * 4. Sumar MUAPs de todas las MUs activas que disparan
 * 5. Añadir ruido de fondo
 * 
 * @param deltaTime Tiempo desde última muestra (típicamente 1 ms)
 * @return Valor EMG en mV
 */
float EMGModel::generateSample(float deltaTime) {
    accumulatedTime += deltaTime;
    
    // Aplicar parámetros pendientes si hay
    if (hasPendingParams) {
        setParameters(pendingParams);
        hasPendingParams = false;
    }
    
    // =========================================================================
    // VARIABILIDAD NATURAL DE FUERZA
    // =========================================================================
    // El control motor humano no es perfecto - hay fluctuaciones naturales
    // de ~2-5% a frecuencia de 1-3 Hz (Enoka 2003)
    if (baseExcitation > 0.0f && params.condition != EMGCondition::TREMOR) {
        forceVariabilityPhase += deltaTime * 2.0f * PI * FORCE_VARIABILITY_FREQ;
        if (forceVariabilityPhase > 2.0f * PI) forceVariabilityPhase -= 2.0f * PI;
        
        float variability = sinf(forceVariabilityPhase) * FORCE_VARIABILITY_AMP;
        // Añadir componente aleatorio de alta frecuencia
        variability += gaussianRandom(0.0f, 0.02f);
        
        currentExcitation = baseExcitation * (1.0f + variability);
        currentExcitation = constrain(currentExcitation, 0.0f, 1.0f);
    }
    
    // =========================================================================
    // MODIFICACIONES ESPECÍFICAS POR CONDICIÓN
    // =========================================================================
    if (params.condition == EMGCondition::TREMOR) {
        // Temblor Parkinsoniano: 4-6 Hz (típico 5 Hz)
        // Modulación sinusoidal suave de la envolvente
        // Excitación(t) = base * (0.5 + 0.5 * sin(2π * f * t))
        tremorPhase += deltaTime * 2.0f * PI * TREMOR_FREQUENCY;
        if (tremorPhase > 2.0f * PI) tremorPhase -= 2.0f * PI;
        
        // Modulación sinusoidal: oscila entre 0 y base (nunca completamente silencioso)
        float tremorModulation = 0.5f + 0.5f * sinf(tremorPhase);
        // Base ~0.25 para tener 10-40 MUs activas oscilando
        currentExcitation = 0.25f * tremorModulation;
        
    } else if (params.condition == EMGCondition::FASCICULATION) {
        // Fasciculaciones: eventos aislados espontáneos
        // Características: 1-3 eventos/segundo, duración 50-150ms, aleatorios
        
        // Desactivar todas las MUs por defecto
        currentExcitation = 0.0f;
        
        // ¿Es tiempo de una nueva fasciculación?
        if (accumulatedTime >= fasciculationTimer && fasciculationActiveMU < 0) {
            // Iniciar nuevo evento de fasciculación
            fasciculationActiveMU = esp_random() % MAX_MOTOR_UNITS;
            // Burst corto: 50-150 ms
            float burstDuration = 0.05f + ((float)(esp_random() % 100) / 1000.0f);
            fasciculationBurstEnd = accumulatedTime + burstDuration;
            
            // Activar la MU seleccionada
            motorUnits[fasciculationActiveMU].isActive = true;
            motorUnits[fasciculationActiveMU].nextFiringTime = accumulatedTime;
            motorUnits[fasciculationActiveMU].firingRate = 25.0f + (float)(esp_random() % 15);  // 25-40 Hz durante burst
        }
        
        // ¿Estamos en un burst activo?
        if (fasciculationActiveMU >= 0) {
            if (accumulatedTime < fasciculationBurstEnd) {
                // Mantener la MU activa durante el burst
                motorUnits[fasciculationActiveMU].isActive = true;
            } else {
                // Fin del burst
                motorUnits[fasciculationActiveMU].isActive = false;
                fasciculationActiveMU = -1;
                // Programar próxima fasciculación (intervalo aleatorio 0.3-1.5s)
                fasciculationTimer = accumulatedTime + 0.3f + ((float)(esp_random() % 1200) / 1000.0f);
            }
        }
    }
    
    // =========================================================================
    // ACTUALIZAR RECLUTAMIENTO
    // =========================================================================
    updateMotorUnitRecruitment();
    
    // =========================================================================
    // GENERAR SEÑAL: SUMAR MUAPs DE TODAS LAS MUs ACTIVAS
    // =========================================================================
    float signal = 0.0f;
    
    // Determinar duración de MUAP según condición
    float muapDuration = MUAP_DURATION;
    if (params.condition == EMGCondition::MYOPATHY) {
        muapDuration = MYOPATHY_MUAP_DURATION;
    } else if (params.condition == EMGCondition::NEUROPATHY) {
        muapDuration = NEUROPATHY_MUAP_DURATION;
    }
    
    for (int i = 0; i < MAX_MOTOR_UNITS; i++) {
        // Usar duración apropiada para verificar si hay MUAP en curso
        float thisMuapDuration = muapDuration;
        if (params.condition == EMGCondition::MYOPATHY) {
            thisMuapDuration = myopathyDurations[i];
        } else if (params.condition == EMGCondition::NEUROPATHY) {
            thisMuapDuration = neuropathyDurations[i];
        }
        
        if (motorUnits[i].isActive || 
            (accumulatedTime - motorUnits[i].lastFiringTime) < (thisMuapDuration / 1000.0f)) {
            
            // Verificar si es tiempo de disparar
            if (motorUnits[i].isActive && accumulatedTime >= motorUnits[i].nextFiringTime) {
                motorUnits[i].lastFiringTime = accumulatedTime;
                
                // Calcular próximo disparo con variabilidad ISI
                float isi = 1.0f / motorUnits[i].firingRate;
                isi *= (1.0f + gaussianRandom(0.0f, ISI_VARIABILITY_CV));
                isi = constrain(isi, 0.015f, 0.2f);
                motorUnits[i].nextFiringTime = accumulatedTime + isi;
            }
            
            // Añadir contribución del MUAP según condición
            float timeSinceFiring = accumulatedTime - motorUnits[i].lastFiringTime;
            
            if (params.condition == EMGCondition::MYOPATHY) {
                // Miopatía: MUAPs pequeños y polifásicos
                if (timeSinceFiring >= 0 && timeSinceFiring < (myopathyDurations[i] / 1000.0f)) {
                    // Usar MUAPs polifásicos (3-5 fases)
                    int phases = 3 + (i % 3);  // 3, 4 o 5 fases
                    signal += generatePolyphasicMUAP(timeSinceFiring, motorUnits[i].amplitude, phases);
                }
            } else if (params.condition == EMGCondition::NEUROPATHY) {
                // Neuropatía: MUAPs gigantes y largos
                if (timeSinceFiring >= 0 && timeSinceFiring < (neuropathyDurations[i] / 1000.0f)) {
                    float sigma = NEUROPATHY_MUAP_SIGMA * (0.8f + 0.4f * ((float)(i % 5) / 5.0f));
                    signal += generateMUAPWithDuration(timeSinceFiring, motorUnits[i].amplitude, 
                                                        neuropathyDurations[i], sigma);
                }
            } else {
                // Normal y otras condiciones
                if (timeSinceFiring >= 0 && timeSinceFiring < (MUAP_DURATION / 1000.0f)) {
                    signal += generateMUAP(timeSinceFiring, motorUnits[i].amplitude);
                }
            }
        }
    }
    
    // =========================================================================
    // APLICAR GANANCIA Y RUIDO
    // =========================================================================
    signal *= params.amplitude;
    
    // Ruido de fondo (interferencia, ruido de electrodo)
    // Típico: 5-50 µV RMS en buenas condiciones
    signal += gaussianRandom(0.0f, params.noiseLevel * 0.1f);
    
    // =========================================================================
    // ACTUALIZAR BUFFER RMS Y GUARDAR VALOR
    // =========================================================================
    updateRMSBuffer(signal);
    lastSampleValue = signal;  // Guardar para getter getCurrentValueMV()
    
    return signal;
}

/**
 * @brief Actualiza el buffer circular para cálculo de RMS
 */
void EMGModel::updateRMSBuffer(float sample) {
    // Restar el valor antiguo de la suma
    rmsSum -= rmsBuffer[rmsBufferIndex] * rmsBuffer[rmsBufferIndex];
    
    // Añadir nuevo valor
    rmsBuffer[rmsBufferIndex] = sample;
    rmsSum += sample * sample;
    
    // Avanzar índice circular
    rmsBufferIndex = (rmsBufferIndex + 1) % RMS_BUFFER_SIZE;
}

uint8_t EMGModel::getDACValue(float deltaTime) {
    float voltage = generateSample(deltaTime);
    return voltageToDACValue(voltage);
}

// ============================================================================
// HELPERS
// ============================================================================

/**
 * @brief Genera número aleatorio con distribución Gaussiana
 * 
 * Implementa el algoritmo Box-Muller para transformar números uniformes
 * en distribución normal. Usa variables de instancia en lugar de static
 * para permitir reset limpio del modelo y evitar problemas de thread-safety.
 * 
 * @param mean Media de la distribución
 * @param std Desviación estándar
 * @return Valor aleatorio con distribución N(mean, std)
 */
float EMGModel::gaussianRandom(float mean, float std) {
    if (gaussHasSpare) {
        gaussHasSpare = false;
        return mean + std * gaussSpare;
    }
    
    float u, v, s;
    do {
        u = (esp_random() / (float)UINT32_MAX) * 2.0f - 1.0f;
        v = (esp_random() / (float)UINT32_MAX) * 2.0f - 1.0f;
        s = u * u + v * v;
    } while (s >= 1.0f || s == 0.0f);
    
    s = sqrtf(-2.0f * logf(s) / s);
    gaussSpare = v * s;
    gaussHasSpare = true;
    
    return mean + std * u * s;
}

// ============================================================================
// GETTERS PARA VISUALIZACIÓN
// ============================================================================

/**
 * @brief Obtiene número de unidades motoras activas
 * @return Cantidad de MUs actualmente reclutadas (0-100)
 */
int EMGModel::getActiveMotorUnits() const {
    int count = 0;
    for (int i = 0; i < MAX_MOTOR_UNITS; i++) {
        if (motorUnits[i].isActive) count++;
    }
    return count;
}

/**
 * @brief Obtiene amplitud RMS de la señal EMG
 * @return RMS en mV (calculado sobre ventana de ~100ms)
 * 
 * El RMS es la métrica más común para cuantificar actividad EMG.
 * Valores típicos:
 *   - Reposo: < 0.05 mV
 *   - Contracción leve: 0.1-0.5 mV
 *   - Contracción moderada: 0.5-1.5 mV
 *   - Contracción máxima: 1.5-4.0 mV
 */
float EMGModel::getRMSAmplitude() const {
    if (rmsSum <= 0.0f) return 0.0f;
    return sqrtf(rmsSum / (float)RMS_BUFFER_SIZE);
}

/**
 * @brief Obtiene frecuencia promedio de disparo de MUs activas
 * @return Frecuencia en Hz (6-50 Hz típico)
 */
float EMGModel::getMeanFiringRate() const {
    float sumRate = 0.0f;
    int activeCount = 0;
    
    for (int i = 0; i < MAX_MOTOR_UNITS; i++) {
        if (motorUnits[i].isActive) {
            sumRate += motorUnits[i].firingRate;
            activeCount++;
        }
    }
    
    if (activeCount == 0) return 0.0f;
    return sumRate / (float)activeCount;
}

/**
 * @brief Obtiene nivel de contracción actual
 * @return Porcentaje de contracción máxima (0-100%)
 */
float EMGModel::getContractionLevel() const {
    return currentExcitation * 100.0f;
}

/**
 * @brief Obtiene nombre de la condición actual
 * @return String descriptivo de la condición
 */
const char* EMGModel::getConditionName() const {
    switch (params.condition) {
        case EMGCondition::REST:                 return "Reposo (0-10% MVC)";
        case EMGCondition::LOW_CONTRACTION:      return "Baja (10-30% MVC)";
        case EMGCondition::MODERATE_CONTRACTION: return "Moderada (30-60% MVC)";
        case EMGCondition::HIGH_CONTRACTION:     return "Alta (60-100% MVC)";
        case EMGCondition::TREMOR:               return "Temblor Parkinson";
        case EMGCondition::MYOPATHY:             return "Miopatia";
        case EMGCondition::NEUROPATHY:           return "Neuropatia";
        case EMGCondition::FASCICULATION:        return "Fasciculacion";
        default:                                 return "Desconocido";
    }
}

/**
 * @brief Obtiene todas las métricas para display en un struct
 * @return EMGDisplayMetrics con todos los valores
 */
EMGDisplayMetrics EMGModel::getDisplayMetrics() const {
    EMGDisplayMetrics metrics;
    metrics.rmsAmplitude_mV = getRMSAmplitude();
    metrics.activeMotorUnits = getActiveMotorUnits();
    metrics.meanFiringRate_Hz = getMeanFiringRate();
    metrics.contractionLevel = getContractionLevel();
    metrics.conditionName = getConditionName();
    return metrics;
}

// ============================================================================
// CONVERSIÓN DAC
// ============================================================================

/**
 * @brief Convierte voltaje a valor DAC de 8 bits
 * @param voltage Voltaje en mV (rango ±5mV)
 * @return Valor DAC 0-255 (128 = 0mV)
 * 
 * Escalado diseñado para capturar rango completo de sEMG:
 *   - Normal: 0.05-2mV
 *   - Neuropatía (MUAPs gigantes): hasta 4-5mV
 * 
 * Fórmula: DAC = 128 + (voltage / 5.0) * 127
 */
uint8_t EMGModel::voltageToDACValue(float voltage) {
    // Rango de entrada: ±5mV (permite ver MUAPs gigantes de neuropatía)
    // Usa constante global EMG_MAX_MV definida arriba
    
    // Limitar al rango
    if (voltage > EMG_MAX_MV) voltage = EMG_MAX_MV;
    if (voltage < -EMG_MAX_MV) voltage = -EMG_MAX_MV;
    
    // Convertir a DAC: 0mV -> 128, ±5mV -> 0/255
    float normalized = voltage / EMG_MAX_MV;  // -1 a +1
    int dacValue = (int)(128.0f + normalized * 127.0f);
    
    // Clamp por seguridad
    if (dacValue < 0) dacValue = 0;
    if (dacValue > 255) dacValue = 255;
    
    return (uint8_t)dacValue;
}

// ============================================================================
// ESCALADO CON GANANCIA FIJA PARA WAVEFORM
// ============================================================================
/**
 * @brief Obtiene valor escalado 0-255 para el waveform Nextion
 * @return Valor amplificado con ganancia fija
 * 
 * EMG es una señal centrada en 0, rango teórico ±5mV
 * Amplificamos para ocupar más del display
 */
uint8_t EMGModel::getWaveformValue() const {
    float sample = lastSampleValue;
    
    // Amplificar la señal
    float amplified = sample * waveformGain;
    
    // Rango amplificado
    const float AMP_MIN = EMG_MODEL_MIN * waveformGain;  // -5 * 3 = -15
    const float AMP_MAX = EMG_MODEL_MAX * waveformGain;  // 5 * 3 = 15
    const float AMP_RANGE = AMP_MAX - AMP_MIN;           // 30
    
    // Mapear al rango 0-1
    float normalized = (amplified - AMP_MIN) / AMP_RANGE;
    
    // Clampeo seguro
    normalized = constrain(normalized, 0.0f, 1.0f);
    
    // Invertir Y para Nextion
    normalized = 1.0f - normalized;
    
    return (uint8_t)(normalized * 255.0f);
}
