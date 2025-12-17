# 🎛️ EMG - Parametrización Segura para Usuario

## **Filosofía de Diseño**

Los usuarios pueden ajustar **parámetros cosméticos** sin destruir la **integridad fisiológica** de cada patología.

### **Parámetros Ajustables** ✅
1. **Excitación (0-100%)**: Modifica intensidad de contracción dentro del rango patológico
2. **Amplitud (0.5-2.0x)**: Escala global de voltaje (simula impedancia de piel)
3. **Ruido (0-10%)**: Añade ruido gaussiano aditivo

### **Parámetros Bloqueados** 🔒
- **Firing Rate (FR)**: Específico de cada patología
- **Motor Units (MUs)**: Reclutamiento automático Henneman
- **Frecuencia tremor**: Fija 4.5 Hz (Parkinson)
- **Parámetros fatiga**: MDF, tau, duración fija

---

## **1. SLIDER: EXCITACIÓN (0-100%)**

### **Rangos Seguros por Condición**

| Condición | Min % | Default % | Max % | Notas |
|-----------|-------|-----------|-------|-------|
| **REST** | 0 | 3 | 10 | Mínima activación, noise floor |
| **LOW** | 5 | 15 | 30 | Contracción suave |
| **MODERATE** | 20 | 40 | 60 | Contracción moderada |
| **HIGH** | 50 | 80 | 100 | Máxima contracción voluntaria |
| **TREMOR** | 3 | 8 | 15 | Activación muy baja, tremor visible |
| **FATIGUE** | 40 | 50 | 60 | Contracción sostenida (protocolo fijo) |

**Implementación:**
```cpp
float EMGModel::clampExcitationForCondition(float excitation) const {
    switch (params.condition) {
        case EMGCondition::REST:
            return constrain(excitation, 0.0f, 0.10f);  // 0-10%
        case EMGCondition::LOW_CONTRACTION:
            return constrain(excitation, 0.05f, 0.30f); // 5-30%
        case EMGCondition::MODERATE_CONTRACTION:
            return constrain(excitation, 0.20f, 0.60f); // 20-60%
        case EMGCondition::HIGH_CONTRACTION:
            return constrain(excitation, 0.50f, 1.00f); // 50-100%
        case EMGCondition::TREMOR:
            return constrain(excitation, 0.03f, 0.15f); // 3-15%
        case EMGCondition::FATIGUE:
            return constrain(excitation, 0.40f, 0.60f); // 40-60% (protocolo)
        default:
            return constrain(excitation, 0.0f, 1.0f);
    }
}
```

---

## **2. SLIDER: AMPLITUD (0.5-2.0x)**

### **Factor Multiplicador Global**

**Rango seguro:** 0.5x - 2.0x (±100%)

**Propósito:** Simula cambios en impedancia de piel/electrodos sin alterar morfología.

**Implementación:**
```cpp
// En generateSample(), multiplicar señal final
signal *= params.amplitude;  // 0.5-2.0x

// Límites en setter
void EMGModel::setAmplitude(float amp) {
    params.amplitude = constrain(amp, 0.5f, 2.0f);
}
```

**Efectos:**
- ✅ Mantiene proporciones entre condiciones
- ✅ No altera FR, MUs, ni características espectrales
- ✅ Simula variabilidad inter-sujeto (contacto electrodo)

---

## **3. SLIDER: RUIDO (0-10%)**

### **Nivel de Ruido Gaussiano**

**Rango seguro:** 0.0% - 10.0% (0.0 - 0.5 mV RMS)

**Implementación:**
```cpp
void EMGModel::setNoiseLevel(float noise) {
    params.noiseLevel = constrain(noise, 0.0f, 0.10f); // 0-10%
}

// En generateSample()
float noiseSample = gaussianRandom(0.0f, params.noiseLevel * EMG_OUTPUT_MAX_MV);
signal += noiseSample;
```

**Niveles sugeridos:**
- 0%: Señal limpia (ideal laboratorio)
- 2-3%: Clínico típico
- 5-7%: Ambiente ruidoso
- 10%: Máximo permitido (señal aún interpretable)

---

## **4. PARÁMETROS NO AJUSTABLES**

### **¿Por qué están bloqueados?**

| Parámetro | Razón |
|-----------|-------|
| **Firing Rate** | Define la patología (8 Hz tremor ≠ 22 Hz fatiga) |
| **MUs activas** | Henneman automático, fisiología respetada |
| **Frecuencia tremor** | 4.5 Hz Parkinson (fijo, no variable) |
| **MDF fatiga** | Progresión 95→60 Hz temporal, no ajustable |
| **Tau fatiga** | Constante tiempo 10s para ciclo visible |

---

## **5. LÍMITES EN NEXTION**

### **Configuración Sliders .HMI**

```javascript
// PÁGINA: parametros_emg (ID 8)

// Slider Excitación (h_exc, ID 8)
minval: 0
maxval: 100
value: [depende de condición, ver tabla arriba]

// Slider Amplitud (h_amp, ID 9)
minval: 50   // 0.5x
maxval: 200  // 2.0x
value: 100   // 1.0x default

// Slider Ruido (h_noise, ID 10)
minval: 0
maxval: 10   // 10% máximo
value: 3     // 3% típico
```

---

## **6. VALIDACIÓN AL APLICAR PARÁMETROS**

```cpp
void EMGModel::setParameters(const EMGParameters& newParams) {
    // Copiar condición
    params.condition = newParams.condition;
    
    // VALIDAR excitación según condición
    params.excitationLevel = clampExcitationForCondition(newParams.excitationLevel);
    
    // VALIDAR amplitud (0.5-2.0x)
    params.amplitude = constrain(newParams.amplitude, 0.5f, 2.0f);
    
    // VALIDAR ruido (0-10%)
    params.noiseLevel = constrain(newParams.noiseLevel, 0.0f, 0.10f);
    
    // Aplicar modificadores de condición (FR, MUs, etc. automáticos)
    applyConditionModifiers();
}
```

---

## **7. FEEDBACK AL USUARIO**

### **Indicadores en Pantalla**

```
┌─────────────────────────────────────────────┐
│  Parámetros EMG - MODERATE                  │
├─────────────────────────────────────────────┤
│                                             │
│  Excitación:  [████████░░] 40%  (20-60%)   │
│  Amplitud:    [█████████░] 1.0x (0.5-2.0)  │
│  Ruido:       [██░░░░░░░░] 3%   (0-10%)    │
│                                             │
│  ⚠️ Límites automáticos por condición       │
│                                             │
│  [Aplicar]  [Cancelar]  [Reset]             │
└─────────────────────────────────────────────┘
```

**Mostrar rangos válidos:** Usuario ve que MODERATE permite 20-60% excitación, no 0-100%.

---

## **8. RESUMEN TÉCNICO**

| Aspecto | Decisión |
|---------|----------|
| **Parámetros ajustables** | Excitación, Amplitud, Ruido |
| **Parámetros bloqueados** | FR, MUs, Tremor freq, Fatiga params |
| **Validación** | Clamp automático en setParameters() |
| **Rangos dinámicos** | Dependen de condición actual |
| **Preserva patología** | ✅ Sí, características core intactas |
| **Grid/escala afectada** | ❌ No, grid fijo ±5mV siempre |
