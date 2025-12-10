/**
 * @file param_controller.cpp
 * @brief Implementación del controlador de parámetros
 * @version 1.0.0
 */

#include "core/param_controller.h"
#include "data/param_limits.h"

// ============================================================================
// CONSTRUCTOR
// ============================================================================
ParamController::ParamController() : 
    activeSignalType(SignalType::NONE)
{
    resetToDefaults();
}

// ============================================================================
// SET ACTIVE SIGNAL
// ============================================================================
void ParamController::setActiveSignal(SignalType type, uint8_t condition) {
    activeSignalType = type;
    
    switch (type) {
        case SignalType::ECG:
            currentECG.condition = static_cast<ECGCondition>(condition);
            break;
        case SignalType::EMG:
            currentEMG.condition = static_cast<EMGCondition>(condition);
            break;
        case SignalType::PPG:
            currentPPG.condition = static_cast<PPGCondition>(condition);
            break;
        default:
            break;
    }
}

// ============================================================================
// PARÁMETROS TIPO A (INMEDIATOS)
// ============================================================================
void ParamController::setNoiseLevel(float noise) {
    noise = clamp(noise, 0.0f, 1.0f);
    
    switch (activeSignalType) {
        case SignalType::ECG:
            currentECG.noiseLevel = noise;
            break;
        case SignalType::EMG:
            currentEMG.noiseLevel = noise;
            break;
        case SignalType::PPG:
            currentPPG.noiseLevel = noise;
            break;
        default:
            break;
    }
}

void ParamController::setAmplitude(float amplitude) {
    switch (activeSignalType) {
        case SignalType::ECG: {
            ECGLimits limits = getECGLimits(currentECG.condition);
            currentECG.qrsAmplitude = clamp(amplitude, limits.qrsAmplitude.min, limits.qrsAmplitude.max);
            break;
        }
        case SignalType::EMG: {
            EMGLimits limits = getEMGLimits(currentEMG.condition);
            currentEMG.amplitude = clamp(amplitude, limits.amplitude.min, limits.amplitude.max);
            break;
        }
        case SignalType::PPG: {
            // PPG usa perfusionIndex como proxy de amplitud
            PPGLimits limits = getPPGLimits(currentPPG.condition);
            float pi = amplitude * 5.0f;
            currentPPG.perfusionIndex = clamp(pi, limits.perfusionIndex.min, limits.perfusionIndex.max);
            break;
        }
        default:
            break;
    }
}

void ParamController::setSTShift(float shift) {
    if (activeSignalType == SignalType::ECG) {
        ECGLimits limits = getECGLimits(currentECG.condition);
        currentECG.stShift = clamp(shift, limits.stShift.min, limits.stShift.max);
    }
}

void ParamController::setDicroticNotch(float notch) {
    if (activeSignalType == SignalType::PPG) {
        PPGLimits limits = getPPGLimits(currentPPG.condition);
        currentPPG.dicroticNotch = clamp(notch, limits.dicroticNotch.min, limits.dicroticNotch.max);
    }
}

// ============================================================================
// PARÁMETROS TIPO B (DIFERIDOS)
// ============================================================================
void ParamController::setHeartRate(float hr) {
    switch (activeSignalType) {
        case SignalType::ECG: {
            ECGLimits limits = getECGLimits(currentECG.condition);
            ECGParameters pending = currentECG;
            pending.heartRate = clamp(hr, limits.heartRate.min, limits.heartRate.max);
            pendingECG.hasPending = true;
            pendingECG.pendingValue = pending;
            pendingECG.requestTime = millis();
            break;
        }
        case SignalType::PPG: {
            PPGLimits limits = getPPGLimits(currentPPG.condition);
            PPGParameters pending = currentPPG;
            pending.heartRate = clamp(hr, limits.heartRate.min, limits.heartRate.max);
            pendingPPG.hasPending = true;
            pendingPPG.pendingValue = pending;
            pendingPPG.requestTime = millis();
            break;
        }
        default:
            break;
    }
}

void ParamController::setCondition(uint8_t condition) {
    switch (activeSignalType) {
        case SignalType::ECG: {
            ECGParameters pending = currentECG;
            pending.condition = static_cast<ECGCondition>(condition);
            
            // Re-clampear parámetros a los nuevos límites de la condición
            ECGLimits newLimits = getECGLimits(pending.condition);
            pending.heartRate = clamp(pending.heartRate, newLimits.heartRate.min, newLimits.heartRate.max);
            pending.qrsAmplitude = clamp(pending.qrsAmplitude, newLimits.qrsAmplitude.min, newLimits.qrsAmplitude.max);
            pending.stShift = clamp(pending.stShift, newLimits.stShift.min, newLimits.stShift.max);
            pending.pWaveAmplitude = clamp(pending.pWaveAmplitude, newLimits.pAmplitude.min, newLimits.pAmplitude.max);
            pending.tWaveAmplitude = clamp(pending.tWaveAmplitude, newLimits.tAmplitude.min, newLimits.tAmplitude.max);
            
            pendingECG.hasPending = true;
            pendingECG.pendingValue = pending;
            pendingECG.requestTime = millis();
            break;
        }
        case SignalType::EMG: {
            EMGParameters pending = currentEMG;
            pending.condition = static_cast<EMGCondition>(condition);
            
            // Re-clampear parámetros a los nuevos límites
            EMGLimits newLimits = getEMGLimits(pending.condition);
            pending.excitationLevel = clamp(pending.excitationLevel, newLimits.excitationLevel.min, newLimits.excitationLevel.max);
            pending.amplitude = clamp(pending.amplitude, newLimits.amplitude.min, newLimits.amplitude.max);
            
            pendingEMG.hasPending = true;
            pendingEMG.pendingValue = pending;
            pendingEMG.requestTime = millis();
            break;
        }
        case SignalType::PPG: {
            PPGParameters pending = currentPPG;
            pending.condition = static_cast<PPGCondition>(condition);
            
            // Re-clampear parámetros a los nuevos límites
            PPGLimits newLimits = getPPGLimits(pending.condition);
            pending.heartRate = clamp(pending.heartRate, newLimits.heartRate.min, newLimits.heartRate.max);
            pending.perfusionIndex = clamp(pending.perfusionIndex, newLimits.perfusionIndex.min, newLimits.perfusionIndex.max);
            pending.dicroticNotch = clamp(pending.dicroticNotch, newLimits.dicroticNotch.min, newLimits.dicroticNotch.max);
            
            pendingPPG.hasPending = true;
            pendingPPG.pendingValue = pending;
            pendingPPG.requestTime = millis();
            break;
        }
        default:
            break;
    }
}

void ParamController::setExcitationLevel(float level) {
    if (activeSignalType == SignalType::EMG) {
        EMGLimits limits = getEMGLimits(currentEMG.condition);
        EMGParameters pending = currentEMG;
        pending.excitationLevel = clamp(level, limits.excitationLevel.min, limits.excitationLevel.max);
        pendingEMG.hasPending = true;
        pendingEMG.pendingValue = pending;
        pendingEMG.requestTime = millis();
    }
}

// ============================================================================
// APLICAR PARÁMETROS PENDIENTES
// ============================================================================
bool ParamController::applyPendingParams() {
    bool applied = false;
    
    if (pendingECG.hasPending) {
        currentECG = pendingECG.pendingValue;
        pendingECG.hasPending = false;
        applied = true;
    }
    
    if (pendingEMG.hasPending) {
        currentEMG = pendingEMG.pendingValue;
        pendingEMG.hasPending = false;
        applied = true;
    }
    
    if (pendingPPG.hasPending) {
        currentPPG = pendingPPG.pendingValue;
        pendingPPG.hasPending = false;
        applied = true;
    }
    
    return applied;
}

bool ParamController::hasPendingParams() const {
    return pendingECG.hasPending || pendingEMG.hasPending || pendingPPG.hasPending;
}

// ============================================================================
// GETTERS DE LÍMITES
// ============================================================================
ECGLimits ParamController::getCurrentECGLimits() const {
    return getECGLimits(currentECG.condition);
}

EMGLimits ParamController::getCurrentEMGLimits() const {
    return getEMGLimits(currentEMG.condition);
}

PPGLimits ParamController::getCurrentPPGLimits() const {
    return getPPGLimits(currentPPG.condition);
}

// ============================================================================
// RESET TO DEFAULTS
// ============================================================================
void ParamController::resetToDefaults() {
    currentECG = ECGParameters();
    currentEMG = EMGParameters();
    currentPPG = PPGParameters();
    
    pendingECG.hasPending = false;
    pendingEMG.hasPending = false;
    pendingPPG.hasPending = false;
}

// ============================================================================
// HELPERS
// ============================================================================
float ParamController::clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

bool ParamController::validateECGParams(const ECGParameters& params) {
    ECGLimits limits = getECGLimits(params.condition);
    
    if (params.heartRate < limits.heartRate.min || params.heartRate > limits.heartRate.max) return false;
    if (params.noiseLevel < 0.0f || params.noiseLevel > 1.0f) return false;
    if (params.qrsAmplitude < limits.qrsAmplitude.min || params.qrsAmplitude > limits.qrsAmplitude.max) return false;
    if (params.stShift < limits.stShift.min || params.stShift > limits.stShift.max) return false;
    if (params.pWaveAmplitude < limits.pAmplitude.min || params.pWaveAmplitude > limits.pAmplitude.max) return false;
    if (params.tWaveAmplitude < limits.tAmplitude.min || params.tWaveAmplitude > limits.tAmplitude.max) return false;
    return true;
}

bool ParamController::validateEMGParams(const EMGParameters& params) {
    EMGLimits limits = getEMGLimits(params.condition);
    
    if (params.excitationLevel < limits.excitationLevel.min || params.excitationLevel > limits.excitationLevel.max) return false;
    if (params.noiseLevel < 0.0f || params.noiseLevel > 1.0f) return false;
    if (params.amplitude < limits.amplitude.min || params.amplitude > limits.amplitude.max) return false;
    return true;
}

bool ParamController::validatePPGParams(const PPGParameters& params) {
    PPGLimits limits = getPPGLimits(params.condition);
    
    if (params.heartRate < limits.heartRate.min || params.heartRate > limits.heartRate.max) return false;
    if (params.noiseLevel < 0.0f || params.noiseLevel > 1.0f) return false;
    if (params.perfusionIndex < limits.perfusionIndex.min || params.perfusionIndex > limits.perfusionIndex.max) return false;
    if (params.dicroticNotch < limits.dicroticNotch.min || params.dicroticNotch > limits.dicroticNotch.max) return false;
    return true;
}
