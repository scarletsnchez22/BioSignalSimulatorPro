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
    lastSampleValue = PPG_DC_LEVEL;  // Iniciar en nivel DC
    waveformGain = PPG_WAVEFORM_GAIN_DEFAULT;  // Ganancia para waveform
    
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
    
    // =========================================================================
    // AJUSTE DE PI Y MORFOLOGÍA SEGÚN CONDICIÓN
    // Rangos clínicos validados (clinical_ranges.py):
    // - NORMAL:           PI 2.0-5.0%, SpO2 95-100%, HR 60-100
    // - ARRHYTHMIA:       PI 1.0-5.0%, SpO2 92-100%, HR 60-180 (RR variable)
    // - WEAK_PERFUSION:   PI 0.1-0.5%, SpO2 88-98%,  HR 90-140
    // - STRONG_PERFUSION: PI 5.0-20%,  SpO2 96-100%, HR 60-90
    // - VASOCONSTRICTION: PI 0.5-2.0%, SpO2 94-100%, HR 60-100
    // - LOW_SPO2:         PI 0.5-3.0%, SpO2 70-89%,  HR 90-130
    // =========================================================================
    
    switch (params.condition) {
        case PPGCondition::NORMAL:
            // Valores por defecto - pulso normal con muesca dicrótica visible
            // Allen 2007: Sujeto sano, buena perfusión periférica
            // PI: 2.0-5.0% → usar 3.5% típico
            params.perfusionIndex = 3.5f;
            params.heartRate = 75.0f;
            break;
            
        case PPGCondition::ARRHYTHMIA:
            // Fibrilación auricular: Alta variabilidad RR (>15%)
            // Se maneja en generateNextRR() con rrStd = 0.15
            // Shelley 2007: Amplitud variable latido a latido
            // PI: 1.0-5.0% → usar 3.0% (perfusión conservada)
            params.perfusionIndex = 3.0f;
            params.heartRate = 85.0f;  // Base, pero muy variable
            break;
            
        case PPGCondition::WEAK_PERFUSION:
            // Shock hipovolémico / hipoperfusión periférica
            // Reisner 2008: PI < 0.5%, amplitud reducida 60-80%
            // Awad 2001: Taquicardia compensatoria típica 100-120 BPM
            // PI: 0.1-0.5% → usar 0.3% (muy bajo)
            systolicAmplitude = 0.25f;          // Reducción ~75%
            diastolicAmplitude = 0.08f;         // Ratio preservado pero muy bajo
            dicroticDepth = 0.1f;               // Muesca casi invisible
            params.perfusionIndex = 0.3f;       // PI muy bajo (shock)
            params.heartRate = 115.0f;          // Taquicardia compensatoria
            break;
            
        case PPGCondition::STRONG_PERFUSION:
            // Vasodilatación (fiebre, sepsis temprana, ejercicio)
            // Reisner 2008: PI puede llegar a 10-20%
            // Shelley 2007: Amplitud aumentada, muesca prominente
            // PI: 5.0-20% → usar 10.0% (alto)
            systolicAmplitude = 1.6f;           // Aumento ~60%
            diastolicAmplitude = 0.7f;          // Ratio aumentado (reflexión fuerte)
            dicroticDepth = 0.35f;              // Muesca más visible
            params.perfusionIndex = 10.0f;      // PI alto (vasodilatación)
            params.heartRate = 72.0f;           // Frecuencia normal-baja
            break;
            
        case PPGCondition::VASOCONSTRICTION:
            // Vasoconstricción marcada: frío extremo, shock temprano, vasopresores
            // Allen & Murray 2002: Amplitud muy reducida, muesca atenuada
            // Reisner 2008: PI <0.5% indica perfusión periférica comprometida
            // 
            // Características morfológicas:
            // - Pico sistólico atenuado (menos pronunciado)
            // - Muesca dicrótica suavizada/eliminada
            // - Fase diastólica acortada
            // - Onda más "afilada" y menos expansiva
            // 
            // PI: 0.2-0.8% → usar 0.4% (muy bajo)
            systolicAmplitude = 0.30f;          // Reducción ~70% (onda muy débil)
            diastolicAmplitude = 0.08f;         // Casi sin componente diastólico
            dicroticDepth = 0.05f;              // Muesca casi eliminada
            systolicWidth = 0.04f;              // Pico más estrecho/afilado
            diastolicWidth = 0.06f;             // Diástole acortada
            params.perfusionIndex = 0.4f;       // PI muy bajo (0.3-0.5% típico)
            params.heartRate = 78.0f;           // HR normal-alto
            break;
            
        case PPGCondition::LOW_SPO2:
            // Hipoxemia con perfusión conservada
            // Causa pulmonar (hipoventilación, shunt), no periférica
            // Jubran 2015: SpO2 bajo puede ocurrir con buena señal PPG
            // PI: 0.5-3.0% → usar 1.5% (ligeramente reducido)
            systolicAmplitude = 0.8f;           // Reducción leve ~20%
            diastolicAmplitude = 0.3f;          // Ratio normal
            dicroticDepth = 0.2f;               // Muesca visible
            params.perfusionIndex = 1.5f;       // PI en rango
            params.heartRate = 110.0f;          // Taquicardia refleja
            break;
    }
    
    // Ajustar amplitud por perfusion index (PI correlaciona con amplitud AC)
    // Lima & Bakker 2005: PI = (AC/DC) * 100
    // Escalar respecto a PI de referencia (5.0%)
    float piScale = params.perfusionIndex / PPG_PI_REFERENCE;
    piScale = constrain(piScale, 0.02f, 4.0f);  // Limitar a rango fisiológico
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
    // PPG_AC_SCALE (0.3) representa ratio típico AC/DC en tejido sano
    float signal = PPG_DC_LEVEL + computePulseShape(phaseInCycle) * PPG_AC_SCALE;
    
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
    
    // Guardar para getter getCurrentValueNormalized()
    lastSampleValue = constrain(signal, 0.0f, 1.0f);
    
    return signal;
}

uint8_t PPGModel::getDACValue(float deltaTime) {
    float voltage = generateSample(deltaTime);
    return voltageToDACValue(voltage);
}

uint8_t PPGModel::voltageToDACValue(float voltage) {
    // Escalar PPG (típico 0-1) a 0-255
    float normalized = constrain(voltage, 0.0f, 1.0f);
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
 * @brief Obtiene el índice de perfusión actual con variabilidad natural
 * 
 * El índice de perfusión (PI) representa la relación AC/DC del PPG
 * y es indicador de perfusión periférica. En la realidad, PI fluctúa
 * latido a latido debido a cambios en tono vascular y respiración.
 * 
 * Rangos clínicos (clinical_ranges.py):
 * - NORMAL:           2.0-5.0%
 * - ARRHYTHMIA:       1.0-5.0%
 * - WEAK_PERFUSION:   0.1-0.5%
 * - STRONG_PERFUSION: 5.0-20.0%
 * - VASOCONSTRICTION: 0.5-2.0%
 * - LOW_SPO2:         0.5-3.0%
 * 
 * @return Índice de perfusión con variabilidad gaussiana
 */
float PPGModel::getPerfusionIndex() const {
    float basePI = params.perfusionIndex;
    float piStd = 0.0f;
    float piMin = 0.1f;
    float piMax = 20.0f;
    
    // Variabilidad y clampeo según condición
    switch (params.condition) {
        case PPGCondition::NORMAL:
            // PI: 2.0-5.0%, base 3.5%, std ~0.5
            piStd = 0.5f;
            piMin = 2.0f;
            piMax = 5.0f;
            break;
            
        case PPGCondition::ARRHYTHMIA:
            // PI: 1.0-5.0%, base 3.0%, std ~0.8 (más variable por arritmia)
            piStd = 0.8f;
            piMin = 1.0f;
            piMax = 5.0f;
            break;
            
        case PPGCondition::WEAK_PERFUSION:
            // PI: 0.1-0.5%, base 0.3%, std ~0.08 (rango estrecho)
            piStd = 0.08f;
            piMin = 0.1f;
            piMax = 0.5f;
            break;
            
        case PPGCondition::STRONG_PERFUSION:
            // PI: 5.0-20%, base 10.0%, std ~2.5
            piStd = 2.5f;
            piMin = 5.0f;
            piMax = 20.0f;
            break;
            
        case PPGCondition::VASOCONSTRICTION:
            // PI: 0.2-0.8%, base 0.4%, std ~0.12 (rango estrecho, muy bajo)
            piStd = 0.12f;
            piMin = 0.2f;
            piMax = 0.8f;
            break;
            
        case PPGCondition::LOW_SPO2:
            // PI: 0.5-3.0%, base 1.5%, std ~0.5
            piStd = 0.5f;
            piMin = 0.5f;
            piMax = 3.0f;
            break;
    }
    
    // Añadir variabilidad gaussiana
    float dynamicPI = basePI + gaussianRandomConst(0.0f, piStd);
    
    // Clampear al rango válido para la condición
    return constrain(dynamicPI, piMin, piMax);
}

/**
 * @brief Estima SpO2 basado en la condición actual
 * 
 * En un oxímetro real, SpO2 se calcula del ratio R/IR.
 * Aquí simulamos valores típicos según la condición:
 * - Normal: 95-100%
 * - Hipoxemia leve: 90-94%
 * - Hipoxemia moderada: 85-89%
 * - Hipoxemia severa: <85%
 * 
 * Referencias:
 * - Jubran A. Pulse oximetry. Crit Care. 2015;19:272
 * - Chan ED et al. Pulse oximetry. Respir Med. 2013;107(6):789-799
 * 
 * @return SpO2 estimado en porcentaje (70-100%)
 */
float PPGModel::getSpO2() const {
    float baseSpO2 = 98.0f;  // Normal
    
    // =========================================================================
    // SpO2 SEGÚN CONDICIÓN - Rangos clínicos (clinical_ranges.py):
    // - NORMAL:           95-100%
    // - ARRHYTHMIA:       92-100%
    // - WEAK_PERFUSION:   88-98%
    // - STRONG_PERFUSION: 96-100%
    // - VASOCONSTRICTION: 91-100% (variable por señal débil)
    // - LOW_SPO2:         70-89%
    // =========================================================================
    
    switch (params.condition) {
        case PPGCondition::NORMAL:
            // SpO2 normal: 95-100%, típico 97-99%
            // Rango: 95-100 → base 97.5 ± 1.0
            baseSpO2 = 97.5f + gaussianRandomConst(0.0f, 0.8f);
            break;
            
        case PPGCondition::ARRHYTHMIA:
            // Arritmia: 92-100%, puede afectar ligeramente la lectura
            // Rango: 92-100 → base 96.0 ± 1.5
            baseSpO2 = 96.0f + gaussianRandomConst(0.0f, 1.5f);
            break;
            
        case PPGCondition::WEAK_PERFUSION:
            // Hipoperfusión: 88-98%, lecturas menos confiables
            // Rango: 88-98 → base 93.0 ± 2.0
            baseSpO2 = 93.0f + gaussianRandomConst(0.0f, 2.0f);
            break;
            
        case PPGCondition::STRONG_PERFUSION:
            // Buena perfusión: 96-100%, lecturas estables y altas
            // Rango: 96-100 → base 98.0 ± 0.8
            baseSpO2 = 98.0f + gaussianRandomConst(0.0f, 0.8f);
            break;
            
        case PPGCondition::VASOCONSTRICTION:
            // Vasoconstricción marcada: 91-100%, SpO2 variable por señal débil
            // Rango: 91-100 → base 95.0 ± 2.0 (más inestable)
            baseSpO2 = 95.0f + gaussianRandomConst(0.0f, 2.0f);
            break;
            
        case PPGCondition::LOW_SPO2:
            // Hipoxemia: 70-89%, SpO2 bajo por causa pulmonar
            // Rango: 70-89 → base 80.0 ± 4.0 (centrado en rango)
            baseSpO2 = 80.0f + gaussianRandomConst(0.0f, 4.0f);
            break;
    }
    
    // Clampeo final según condición para garantizar rangos
    switch (params.condition) {
        case PPGCondition::NORMAL:
            return constrain(baseSpO2, 95.0f, 100.0f);
        case PPGCondition::ARRHYTHMIA:
            return constrain(baseSpO2, 92.0f, 100.0f);
        case PPGCondition::WEAK_PERFUSION:
            return constrain(baseSpO2, 88.0f, 98.0f);
        case PPGCondition::STRONG_PERFUSION:
            return constrain(baseSpO2, 96.0f, 100.0f);
        case PPGCondition::VASOCONSTRICTION:
            return constrain(baseSpO2, 91.0f, 100.0f);  // Tolerancia: puede bajar a 91%
        case PPGCondition::LOW_SPO2:
            return constrain(baseSpO2, 70.0f, 90.0f);   // Tolerancia: hasta 90%
        default:
            return constrain(baseSpO2, 70.0f, 100.0f);
    }
}

// Versión const del generador gaussiano para getters
float PPGModel::gaussianRandomConst(float mean, float std) const {
    // Versión simplificada para uso en getters const
    // Usa esp_random() que es thread-safe
    float u = (esp_random() / (float)UINT32_MAX);
    float v = (esp_random() / (float)UINT32_MAX);
    
    // Aproximación Box-Muller simplificada
    float z = sqrtf(-2.0f * logf(u + 0.0001f)) * cosf(2.0f * PI * v);
    return mean + std * z;
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
        case PPGCondition::LOW_SPO2: return "SpO2 Bajo";
        default: return "Desconocido";
    }
}

// ============================================================================
// ESCALADO CON GANANCIA FIJA PARA WAVEFORM
// ============================================================================
/**
 * @brief Obtiene valor escalado 0-255 para el waveform Nextion
 * @return Valor amplificado con ganancia fija
 * 
 * PPG es una señal normalizada 0-1, centrada en ~0.5 (DC level)
 * Amplificamos respecto al centro para ocupar más del display
 */
uint8_t PPGModel::getWaveformValue() const {
    float sample = lastSampleValue;
    
    // Centro del rango PPG (nivel DC)
    const float CENTER = 0.5f;
    
    // Amplificar respecto al centro
    float amplified = CENTER + waveformGain * (sample - CENTER);
    
    // Clampeo al rango 0-1
    amplified = constrain(amplified, 0.0f, 1.0f);
    
    // Invertir Y para Nextion (Y=0 arriba)
    // En PPG, valores altos = más absorción = menos luz = pico sistólico
    float normalized = 1.0f - amplified;
    
    return (uint8_t)(normalized * 255.0f);
}
