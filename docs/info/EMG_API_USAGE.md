# EMG Model v1.1.0 - Guía de Uso del API

## 🔧 **BUG CRÍTICO CORREGIDO: Sistema de Caché**

### Problema Anterior (v1.0.0)

```cpp
// ❌ INCORRECTO - Genera DOS muestras diferentes
emgModel.getRawSample(0.001f);      // Genera muestra 1
emgModel.getProcessedSample(0.001f); // Genera muestra 2 (DIFERENTE!)
```

**Consecuencias:**
- ❌ Avance temporal incorrecto (2ms en lugar de 1ms)
- ❌ Señales desincronizadas (ruido gaussiano diferente)
- ❌ Desperdicio de CPU (doble generación)
- ❌ Fatiga y temblor avanzan demasiado rápido

---

## ✅ **API Correcto (v1.1.0)**

### Arquitectura

```
┌─────────────┐
│   tick()    │ ← Llamar 1 vez/ciclo (genera muestra)
└──────┬──────┘
       │ cachea muestra cruda
       ├────────────┬─────────────┐
       ▼            ▼             ▼
getRawSample()  getProcessed()  getDACValue()
  (usa caché)    (usa caché)    (usa caché)
```

### Uso Correcto en Loop Principal

```cpp
EMGModel emgModel;

void loop() {
    static unsigned long lastTime = 0;
    unsigned long currentTime = millis();
    
    if (currentTime - lastTime >= 1) {  // 1ms
        float deltaTime = 0.001f;
        
        // ✅ PASO 1: Generar UNA sola muestra
        emgModel.tick(deltaTime);
        
        // ✅ PASO 2: Usar múltiples veces (sin regenerar)
        
        // Salida analógica GPIO25 (cruda bipolar)
        uint8_t rawDAC = emgModel.getRawDACValue();
        dacWrite(25, rawDAC);
        
        // Salida Nextion (procesada unipolar)
        uint8_t processedDAC = emgModel.getProcessedDACValue();
        nextion.addWavePoint(processedDAC);
        
        // Serial Plotter (ambas señales)
        Serial.print(emgModel.getRawSample());
        Serial.print(",");
        Serial.println(emgModel.getProcessedSample());
        
        lastTime = currentTime;
    }
}
```

---

## 📚 **Métodos del API**

### `void tick(float deltaTime)`

**Descripción:** Genera UNA muestra y avanza el modelo.

**Parámetros:**
- `deltaTime` - Tiempo desde última muestra (normalmente `0.001f` = 1ms)

**Uso:**
```cpp
emgModel.tick(0.001f);  // Llamar EXACTAMENTE 1 vez por ciclo
```

**IMPORTANTE:**
- ⚠️ **Llamar una sola vez por ciclo de 1ms**
- ⚠️ **Llamar ANTES de usar getRawSample() o getProcessedSample()**
- ✅ Todas las funciones siguientes usan la muestra cacheada

---

### `float getRawSample() const`

**Descripción:** Obtiene señal CRUDA cacheada (bipolar ±5 mV).

**Retorno:** Señal en mV, rango [-5.0, +5.0]

**Uso:**
```cpp
emgModel.tick(0.001f);             // Primero tick
float raw = emgModel.getRawSample(); // Después obtener
```

**Aplicaciones:**
- DAC analógico GPIO25
- Osciloscopio
- Análisis espectral (FFT)
- Serial Plotter

---

### `uint8_t getRawDACValue() const`

**Descripción:** Obtiene DAC de señal cruda (bipolar).

**Retorno:** DAC 0-255 (128 = 0mV centro)

**Mapeo:**
```
-5 mV → DAC 0
 0 mV → DAC 128
+5 mV → DAC 255
```

**Uso:**
```cpp
uint8_t dac = emgModel.getRawDACValue();
dacWrite(25, dac);  // Salida GPIO25
```

---

### `float getProcessedSample()`

**Descripción:** Obtiene señal PROCESADA cacheada (unipolar 0-5 mV).

**Pipeline:** Cruda → Filtro Butterworth 20-450 Hz → Rectificación → Envolvente RMS

**Retorno:** Envolvente RMS en mV, rango [0.0, 5.0]

**Uso:**
```cpp
emgModel.tick(0.001f);
float envelope = emgModel.getProcessedSample();
```

**Aplicaciones:**
- Nextion waveform (envolvente suave)
- Detección de contracciones
- Visualización de fatiga muscular

---

### `uint8_t getProcessedDACValue()`

**Descripción:** Obtiene DAC de señal procesada (unipolar).

**Retorno:** DAC 0-255 (señal UNIPOLAR)

**Mapeo:**
```
0 mV → DAC 0
5 mV → DAC 255
```

**Uso:**
```cpp
uint8_t dac = emgModel.getProcessedDACValue();
nextion.addWavePoint(dac);
```

---

## 🚫 **Métodos DEPRECATED**

### ❌ `float generateSample(float deltaTime)`

**DEPRECATED:** Usar `tick()` en su lugar.

**Razón:** No debería llamarse directamente desde código de usuario.

---

### ❌ `uint8_t getDACValue(float deltaTime)`

**DEPRECATED:** Usar `getRawDACValue()` en su lugar.

**Razón:** Versión antigua que regeneraba muestra.

---

## 📊 **Ejemplos Completos**

### Ejemplo 1: Salida Dual (DAC + Nextion)

```cpp
void loop() {
    static unsigned long lastTime = 0;
    
    if (millis() - lastTime >= 1) {
        // Tick del modelo (1 sola vez)
        emgModel.tick(0.001f);
        
        // Salida 1: GPIO25 (señal cruda para osciloscopio)
        dacWrite(25, emgModel.getRawDACValue());
        
        // Salida 2: Nextion (envolvente suave)
        nextion.addWavePoint(emgModel.getProcessedDACValue());
        
        lastTime = millis();
    }
}
```

---

### Ejemplo 2: Serial Plotter con Múltiples Métricas

```cpp
void loop() {
    static unsigned long lastTime = 0;
    
    if (millis() - lastTime >= 10) {  // 100 Hz
        emgModel.tick(0.01f);
        
        // Plotear múltiples señales
        Serial.print("Raw:");
        Serial.print(emgModel.getRawSample());
        Serial.print(",Envelope:");
        Serial.print(emgModel.getProcessedSample());
        Serial.print(",RMS:");
        Serial.print(emgModel.getRMSAmplitude());
        Serial.print(",MUs:");
        Serial.println(emgModel.getActiveMotorUnits());
        
        lastTime = millis();
    }
}
```

---

### Ejemplo 3: Validación de Rangos Clínicos

```cpp
void validateEMGRange(EMGCondition condition) {
    emgModel.setCondition(condition);
    emgModel.reset();
    
    float sumRMS = 0.0f;
    const int samples = 1000;
    
    for (int i = 0; i < samples; i++) {
        emgModel.tick(0.001f);  // 1 tick por muestra
        sumRMS += emgModel.getRMSAmplitude();
        delay(1);
    }
    
    float avgRMS = sumRMS / samples;
    Serial.printf("Condición: %s, RMS promedio: %.3f mV\n", 
                  emgModel.getConditionName(), avgRMS);
}
```

---

## ⚠️ **Errores Comunes**

### Error 1: Llamar getters sin tick()

```cpp
// ❌ INCORRECTO
float raw = emgModel.getRawSample();  // Retorna 0.0f (no hay caché)
```

**Solución:**
```cpp
// ✅ CORRECTO
emgModel.tick(0.001f);
float raw = emgModel.getRawSample();
```

---

### Error 2: Llamar tick() múltiples veces

```cpp
// ❌ INCORRECTO - avanza el tiempo demasiado rápido
emgModel.tick(0.001f);
emgModel.tick(0.001f);  // 2ms en lugar de 1ms
```

**Solución:**
```cpp
// ✅ CORRECTO - 1 tick por ciclo
emgModel.tick(0.001f);
```

---

### Error 3: Usar métodos deprecated con deltaTime

```cpp
// ❌ DEPRECATED
float raw = emgModel.getRawSample(0.001f);  // API antigua
```

**Solución:**
```cpp
// ✅ NUEVO API
emgModel.tick(0.001f);
float raw = emgModel.getRawSample();
```

---

## 🎯 **Verificación de Corrección**

### Test: Señales Sincronizadas

```cpp
void testSynchronization() {
    emgModel.reset();
    
    for (int i = 0; i < 10; i++) {
        emgModel.tick(0.001f);
        
        float raw = emgModel.getRawSample();
        float processed = emgModel.getProcessedSample();
        
        // Las señales deben estar sincronizadas
        // (processed es versión filtrada de raw)
        Serial.printf("Tick %d: Raw=%.3f, Proc=%.3f\n", i, raw, processed);
    }
}
```

**Salida esperada:**
```
Tick 0: Raw=0.023, Proc=0.000
Tick 1: Raw=-0.018, Proc=0.012
Tick 2: Raw=0.041, Proc=0.025
...
```

✅ `processed` es envolvente suavizada de `raw` (misma base temporal)

---

## 📖 **Referencias**

- **Filtro Butterworth:** `applyBandpassFilter()` - 20-450 Hz @ 1kHz
- **Envolvente RMS:** `applyRMSEnvelope()` - Ventana 25ms
- **Rangos clínicos:** `/docs/ranglim/RANGOS_CLINICOS.md`
- **Modelo científico:** `/docs/models_info/EMG_MODEL_INFO.md`

---

*BioSimulator Pro v1.1.0 - Bug Crítico Corregido*
