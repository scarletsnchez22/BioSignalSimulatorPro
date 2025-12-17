/**
 * @file ppg_model.cpp
 * @brief Implementación del modelo PPG con doble gaussiana
 * @version 1.0.0
 * 
 * Modelo basado en:
 * - Allen J (2007): Forma de onda PPG y componentes fisiológicos
 * - Elgendi M (2012): Posiciones temporales de picos
 * - Reisner A (2008): Significado clínico de parámetros PPG
 */

#include "models/ppg_model.h"
#include "config.h"
#include <math.h>
#include <esp_random.h>

// ============================================================================
// CONSTRUCTOR
// ============================================================================
PPGModel::PPGModel() {
    hasPendingParams = false;
    reset();
}

// ============================================================================
// RESET
// ============================================================================
void PPGModel::reset() {
    phaseInCycle = 0.0f;
    currentRR = 0.8f;  // ~75 BPM
    beatCount = 0;
    motionNoise = 0.0f;
    baselineWander = 0.0f;
    
    // Reset generador gaussiano
    gaussHasSpare = false;
    gaussSpare = 0.0f;
    
    // Parámetros de forma de onda desde constantes con respaldo científico
    // (ver ppg_model.h para referencias: Elgendi 2012, Allen 2007, Millasseau 2006)
    systolicAmplitude = 1.0f;               // Normalizado a 1.0
    systolicWidth = PPG_SYSTOLIC_WIDTH;     // 0.055 (Millasseau 2006)
    diastolicAmplitude = PPG_DIASTOLIC_RATIO; // 0.40 (Allen 2007)
    diastolicWidth = PPG_DIASTOLIC_WIDTH;   // 0.10 (más dispersa)
    dicroticDepth = PPG_NOTCH_DEPTH;        // 0.25 (Allen 2007)
    dicroticWidth = PPG_NOTCH_WIDTH;        // 0.02 (evento valvular rápido)
}

// ============================================================================
// CONFIGURACIÓN DE PARÁMETROS
// ============================================================================
void PPGModel::setParameters(const PPGParameters& newParams) {
    params = newParams;
    applyConditionModifiers();
    currentRR = generateNextRR();
}

void PPGModel::setPendingParameters(const PPGParameters& newParams) {
    pendingParams = newParams;
    hasPendingParams = true;
}

// ============================================================================
// MODIFICADORES POR CONDICIÓN PATOLÓGICA
// 
// Referencias:
// - Reisner A et al. (2008): Utility of PPG in circulatory monitoring
// - Shelley KH (2007): Photoplethysmography: Beyond the calculation of SpO2
// - Allen J & Murray A (2002): Age-related changes in PPG waveform
// - Awad AA et al. (2001): Relationship between the PI and tissue oxygenation
// ============================================================================
void PPGModel::applyConditionModifiers() {
    // Reset a valores normales (Allen 2007)
    systolicAmplitude = 1.0f;
    diastolicAmplitude = PPG_DIASTOLIC_RATIO;  // 0.4
    dicroticDepth = params.dicroticNotch;
    motionNoise = 0.0f;
    
    switch (params.condition) {
        case PPGCondition::NORMAL:
            // Valores por defecto - pulso normal con muesca dicrótica visible
            // Allen 2007: Sujeto sano, buena perfusión periférica
            dicroticDepth = 0.01f;  // Aumentar profundidad muesca para visibilidad
            break;
            
        case PPGCondition::ARRHYTHMIA:
            // Fibrilación auricular: Alta variabilidad RR (>15%)
            // Se maneja en generateNextRR() con rrStd = 0.15
            // Shelley 2007: Amplitud variable latido a latido
            dicroticDepth = 0.01f;
            break;
            
        case PPGCondition::WEAK_PERFUSION:
            // Shock hipovolémico / hipoperfusión periférica
            // Reisner 2008: PI < 0.5%, amplitud reducida 60-80%
            // Awad 2001: Taquicardia compensatoria típica 100-120 BPM
            systolicAmplitude = 0.25f;          // Reducción ~75%
            diastolicAmplitude = 0.08f;         // Ratio preservado pero muy bajo
            dicroticDepth = 0.01f;              // Estandarizado
            params.heartRate = 115.0f;          // Taquicardia compensatoria
            break;
            
        case PPGCondition::VASODILATION:
            // Vasodilatación moderada (fiebre leve, calor, ejercicio leve)
            // Reisner 2008: PI 5-10%
            // Shelley 2007: Amplitud aumentada, muesca visible
            systolicAmplitude = 1.3f;           // Aumento ~30%
            diastolicAmplitude = 0.5f;          // Ratio aumentado (reflexión)
            dicroticDepth = 0.01f;              // Estandarizado
            params.heartRate = 75.0f;           // Frecuencia normal
            break;
            
        case PPGCondition::STRONG_PERFUSION:
            // Vasodilatación (fiebre, sepsis temprana, ejercicio)
            // Reisner 2008: PI puede llegar a 10-20%
            // Shelley 2007: Amplitud aumentada, muesca prominente
            systolicAmplitude = 1.6f;           // Aumento ~60%
            diastolicAmplitude = 0.7f;          // Ratio aumentado (reflexión fuerte)
            dicroticDepth = 0.01f;              // Estandarizado
            params.heartRate = 72.0f;           // Frecuencia normal-baja
            break;
            
        case PPGCondition::VASOCONSTRICTION:
            // Frío, estrés, hipovolemia leve, vasopresores
            // Allen & Murray 2002: Muesca más prominente, amplitud reducida
            // Millasseau 2006: Aumento de índice de rigidez (stiffness index)
            systolicAmplitude = 0.7f;           // Reducción ~30%
            diastolicAmplitude = 0.35f;         // Ratio ligeramente aumentado
            dicroticDepth = 0.01f;              // Estandarizado
            // HR no se modifica (puede ser normal)
            break;
    }
    
    // Ajustar por perfusion index del usuario (PI normal ~2-5%, rango 0.1-20%)
    // Lima & Bakker 2005: PI correlaciona con flujo periférico
    float piScale = params.perfusionIndex / 3.0f;  // PI normal = 3%
    piScale = constrain(piScale, 0.1f, 2.0f);  // Limitar a rango fisiológico
    systolicAmplitude *= piScale;
    diastolicAmplitude *= piScale;
}

// ============================================================================
// GENERACIÓN DE RR CON VARIABILIDAD
// Variabilidad RR simplificada (Task Force, 1996)
// ============================================================================
float PPGModel::generateNextRR() {
    float rrMean = 60.0f / params.heartRate;
    float rrStd = 0.02f;  // ~2% variabilidad base (dominio temporal normal)
    
    // Mayor variabilidad para arritmia (fibrilación auricular simóptica)
    if (params.condition == PPGCondition::ARRHYTHMIA) {
        rrStd = 0.15f;  // ~15% variabilidad (característico de FA)
        
        // 15% de latidos ectópicos (contracciones prematuras)
        if (esp_random() % 100 < 15) {
            rrMean *= 0.7f;  // Latido prematuro seguido de pausa compensatoria
        }
    }
    
    float rr = rrMean + gaussianRandom(0.0f, rrStd);
    rr = constrain(rr, 0.3f, 2.0f);  // Límites fisiológicos (30-200 BPM)
    
    return rr;
}

// ============================================================================
// FORMA DEL PULSO - MODELO DOBLE GAUSSIANA (Allen 2007)
// 
// El pulso PPG consiste en:
// 1. Pico sistólico: Onda de presión directa del corazón
// 2. Pico diastólico: Reflexión de onda desde bifurcación aórtica
// 3. Muesca dicrótica: Cierre de válvula aórtica (entre ambos picos)
// ============================================================================
float PPGModel::computePulseShape(float phase) {
    // Normalizar fase a 0-1
    phase = fmodf(phase, 1.0f);
    if (phase < 0) phase += 1.0f;
    
    // Pico sistólico (gaussiana principal) - ~15% del ciclo
    float systolic = systolicAmplitude * 
        expf(-powf(phase - PPG_SYSTOLIC_POS, 2) / (2.0f * powf(systolicWidth, 2)));
    
    // Pico diastólico (gaussiana secundaria) - ~40% del ciclo
    // Representa la reflexión de onda desde la bifurcación aórtica
    float diastolic = diastolicAmplitude * 
        expf(-powf(phase - PPG_DIASTOLIC_POS, 2) / (2.0f * powf(diastolicWidth, 2)));
    
    // Muesca dicrótica (resta) - ~30% del ciclo
    // Cierre de válvula aórtica, crea caída momentánea en presión
    float notch = dicroticDepth * 
        expf(-powf(phase - PPG_NOTCH_POS, 2) / (2.0f * powf(dicroticWidth, 2)));
    
    // Señal compuesta: sistólica + diastólica - muesca
    float pulse = systolic + diastolic - notch;
    
    return pulse;
}

// ============================================================================
// DETECCIÓN DE LATIDO Y APLICACIÓN DE PARÁMETROS PENDIENTES
// ============================================================================
void PPGModel::detectBeatAndApplyPending() {
    // Si fase está cerca de 0, es nuevo latido
    if (phaseInCycle < 0.01f) {
        beatCount++;
        
        // Aplicar parámetros pendientes (cambio suave entre latidos)
        if (hasPendingParams) {
            setParameters(pendingParams);
            hasPendingParams = false;
        }
        
        // Generar nuevo intervalo RR con variabilidad
        currentRR = generateNextRR();
    }
}

// ============================================================================
// GENERACIÓN DE MUESTRA
// ============================================================================
float PPGModel::generateSample(float deltaTime) {
    // Avanzar fase dentro del ciclo cardíaco
    phaseInCycle += deltaTime / currentRR;
    
    // Reset al completar ciclo
    if (phaseInCycle >= 1.0f) {
        phaseInCycle = fmodf(phaseInCycle, 1.0f);
        detectBeatAndApplyPending();
    }
    
    // Calcular forma del pulso: DC + AC escalado
    // 0.3 representa ratio típico AC/DC en tejido sano
    float signal = 1.0f + computePulseShape(phaseInCycle) * 0.3f;
    
    // Añadir baseline wander lento (~0.05 Hz respiratorio)
    // Mantener fase cíclica para evitar overflow
    baselineWander = fmodf(baselineWander + deltaTime * 0.5f, 2.0f * PI);
    signal += 0.02f * sinf(baselineWander);
    
    // Añadir ruido de movimiento si aplica
    if (motionNoise > 0) {
        signal += gaussianRandom(0.0f, motionNoise * 0.1f);
        
        // Spikes aleatorios
        if (esp_random() % 500 < 1) {
            signal += gaussianRandom(0.0f, 0.3f);
        }
    }
    
    // Añadir ruido base
    signal += gaussianRandom(0.0f, params.noiseLevel * 0.05f);
    
    return signal;
}

uint8_t PPGModel::getDACValue(float deltaTime) {
    float voltage = generateSample(deltaTime);
    return voltageToDACValue(voltage);
}

uint8_t PPGModel::voltageToDACValue(float voltage) {
    // QUITAR ESTA LÍNEA: voltage = 1.0f - voltage;
    
    float normalized = constrain(voltage, 0.0f, 1.5f);
    normalized = normalized / 1.5f;  // Escalar 0-1.5 a 0-1
    return (uint8_t)(normalized * 255.0f);
}

// ============================================================================
// HELPERS
// ============================================================================

/**
 * @brief Generador de números aleatorios con distribución gaussiana
 * 
 * Implementa el algoritmo Box-Muller para transformar números uniformes
 * en distribución normal. Usa variables de instancia en lugar de static
 * para permitir reset limpio del modelo.
 * 
 * @param mean Media de la distribución
 * @param std Desviación estándar
 * @return Valor aleatorio con distribución N(mean, std)
 */
float PPGModel::gaussianRandom(float mean, float std) {
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
// GETTERS
// ============================================================================

/**
 * @brief Obtiene la frecuencia cardíaca actual basada en RR
 * @return Frecuencia cardíaca en BPM
 */
float PPGModel::getCurrentHeartRate() const {
    return 60.0f / currentRR;
}

/**
 * @brief Obtiene el índice de perfusión actual
 * 
 * El índice de perfusión (PI) representa la relación AC/DC del PPG
 * y es indicador de perfusión periférica. Valores típicos: 2-5%
 * 
 * @return Índice de perfusión (0.1-20.0)
 */
float PPGModel::getPerfusionIndex() const {
    return params.perfusionIndex;
}

/**
 * @brief Determina si estamos en fase sistólica
 * 
 * La sístole ocupa aproximadamente el primer 25% del ciclo cardíaco,
 * correspondiendo a la eyección ventricular.
 * 
 * @return true si fase < 25% del ciclo
 */
bool PPGModel::isInSystole() const {
    return (phaseInCycle < 0.3f);
}

const char* PPGModel::getConditionName() const {
    switch (params.condition) {
        case PPGCondition::NORMAL: return "Normal";
        case PPGCondition::ARRHYTHMIA: return "Arritmia";
        case PPGCondition::WEAK_PERFUSION: return "Perfusión Débil";
        case PPGCondition::STRONG_PERFUSION: return "Perfusión Fuerte";
        case PPGCondition::VASOCONSTRICTION: return "Vasoconstricción";
        default: return "Desconocido";
    }
}

// ============================================================================
// MÉTRICAS MEDIBLES (según tabla de rangos clínicos PPG)
// ============================================================================

/**
 * @brief Obtiene la amplitud AC en mV
 * 
 * AC amplitude = DC × (PI/100)
 * Usa DC teórico de 1000 mV hasta resolver escalado
 * 
 * @return AC en mV (teórico)
 */
float PPGModel::getACAmplitude() const {
    const float DC_BASELINE_MV = 1000.0f;  // Valor teórico de referencia
    return DC_BASELINE_MV * (params.perfusionIndex / 100.0f);
}

/**
 * @brief Obtiene el DC baseline en mV
 * 
 * Baseline constante según tabla de rangos clínicos
 * 
 * @return DC en mV (teórico, siempre 1000 mV)
 */
float PPGModel::getDCBaseline() const {
    return 1000.0f;  // Valor teórico de referencia
}

/**
 * @brief Obtiene la duración de la sístole en ms
 * 
 * Sístole: desde inicio del latido hasta final del pico sistólico
 * Aproximadamente 25-30% del ciclo RR según literatura
 * 
 * @return Duración de sístole en ms
 */
float PPGModel::getSystoleTime() const {
    // Sístole: ~30% del ciclo RR (Elgendi 2012, Allen 2007)
    return currentRR * 1000.0f * 0.30f;
}

/**
 * @brief Obtiene la duración de la diástole en ms
 * 
 * Diástole: desde final de sístole hasta inicio del próximo latido
 * Aproximadamente 70% del ciclo RR
 * 
 * @return Duración de diástole en ms
 */
float PPGModel::getDiastoleTime() const {
    // Diástole: ~70% del ciclo RR (resto del ciclo)
    return currentRR * 1000.0f * 0.70f;
}