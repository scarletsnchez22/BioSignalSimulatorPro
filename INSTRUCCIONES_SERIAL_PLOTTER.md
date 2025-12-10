# 📊 Instrucciones Serial Plotter - BioSignalSimulator Pro

## ✅ Solución Implementada

El código ahora inicia **automáticamente** generando datos sin necesidad de menú interactivo. Esto permite usar el Serial Plotter visual directamente.

---

## 🎛️ Cómo Cambiar la Señal y Patología

Edita las siguientes líneas al **INICIO** del archivo `src/main_debug.cpp` (líneas 24-47):

```cpp
// ============================================================================
// CONFIGURACIÓN AUTO-START (Para Serial Plotter)
// ============================================================================
#define AUTO_START              1       // 1 = inicio automático, 0 = menú interactivo

// Tipo de señal: 0=ECG, 1=EMG, 2=PPG
#define AUTO_SIGNAL_TYPE        0       // <-- CAMBIA ESTO

// Condiciones ECG (solo si AUTO_SIGNAL_TYPE==0):
// 0=NORMAL, 1=TACHYCARDIA, 2=BRADYCARDIA, 3=ATRIAL_FIBRILLATION
// 4=VENTRICULAR_FIBRILLATION, 5=PREMATURE_VENTRICULAR, 6=BUNDLE_BRANCH_BLOCK
// 7=ST_ELEVATION, 8=ST_DEPRESSION
#define AUTO_ECG_CONDITION      0       // <-- CAMBIA ESTO

// Condiciones EMG (solo si AUTO_SIGNAL_TYPE==1):
// 0=REST, 1=MILD_CONTRACTION, 2=MODERATE_CONTRACTION, 3=STRONG_CONTRACTION
// 4=MAXIMUM_CONTRACTION, 5=TREMOR, 6=MYOPATHY, 7=NEUROPATHY
// 8=FASCICULATION, 9=FATIGUE
#define AUTO_EMG_CONDITION      0       // <-- CAMBIA ESTO

// Condiciones PPG (solo si AUTO_SIGNAL_TYPE==2):
// 0=NORMAL, 1=ARRHYTHMIA, 2=WEAK_PERFUSION, 3=STRONG_PERFUSION
// 4=VASOCONSTRICTION, 5=MOTION_ARTIFACT, 6=LOW_SPO2
#define AUTO_PPG_CONDITION      0       // <-- CAMBIA ESTO
```

---

## 📝 Ejemplos de Configuración

### Ejemplo 1: ECG Normal
```cpp
#define AUTO_SIGNAL_TYPE        0       // ECG
#define AUTO_ECG_CONDITION      0       // NORMAL
```

### Ejemplo 2: ECG Taquicardia
```cpp
#define AUTO_SIGNAL_TYPE        0       // ECG
#define AUTO_ECG_CONDITION      1       // TACHYCARDIA
```

### Ejemplo 3: ECG Fibrilación Auricular
```cpp
#define AUTO_SIGNAL_TYPE        0       // ECG
#define AUTO_ECG_CONDITION      3       // ATRIAL_FIBRILLATION
```

### Ejemplo 4: EMG Contracción Moderada
```cpp
#define AUTO_SIGNAL_TYPE        1       // EMG
#define AUTO_EMG_CONDITION      2       // MODERATE_CONTRACTION
```

### Ejemplo 5: PPG Arritmia
```cpp
#define AUTO_SIGNAL_TYPE        2       // PPG
#define AUTO_PPG_CONDITION      1       // ARRHYTHMIA
```

---

## 🚀 Procedimiento Completo

### 1️⃣ Configurar la Señal
Edita `src/main_debug.cpp` líneas 24-47 con la señal deseada

### 2️⃣ Compilar y Subir
```powershell
pio run -e esp32_wroom32 -t upload
```

### 3️⃣ Abrir Serial Plotter
- En VS Code, abre la extensión **Serial Plotter** (Mario Zechner)
- Selecciona puerto: **COM4**
- Baud rate: **115200**
- Click en **Start**

### 4️⃣ Visualizar
Los datos aparecerán automáticamente:
- **ECG**: `>ecg:`, `hr:`, `rr:`
- **EMG**: `>emg:`, `rms:`, `mus:`, `fr:`
- **PPG**: `>ppg:`, `hr:`, `rr:`, `pi:`

---

## 🔄 Volver al Menú Interactivo

Si prefieres el menú interactivo original, cambia:
```cpp
#define AUTO_START              0       // Desactivar auto-start
```

Luego compila y sube de nuevo.

---

## 📊 Formato de Datos

### ECG
```
>ecg:0.8545,hr:75.2,rr:800
```
- `ecg`: Valor en mV
- `hr`: Heart Rate en BPM
- `rr`: RR interval en ms

### EMG
```
>emg:0.234,rms:0.156,mus:45,fr:15.3
```
- `emg`: Valor en mV
- `rms`: RMS amplitude en mV
- `mus`: Motor Units activas
- `fr`: Firing Rate en Hz

### PPG
```
>ppg:0.678,hr:72.5,rr:830,pi:3.45
```
- `ppg`: Valor normalizado
- `hr`: Heart Rate en BPM
- `rr`: RR interval en ms
- `pi`: Perfusion Index en %

---

## 🎯 Ventajas de Este Método

✅ **Sin interrupciones**: Los datos se generan continuamente  
✅ **Independiente del monitor**: Puedes cerrar/abrir Serial Plotter sin perder datos  
✅ **Cambio rápido**: Solo edita 2 líneas y vuelve a subir  
✅ **Visualización profesional**: Gráficas en tiempo real con Serial Plotter  

---

## ⚠️ Notas Importantes

1. **Después de cambiar la configuración**, debes volver a compilar y subir el código
2. **El ESP32 debe estar conectado** para que Serial Plotter reciba datos
3. **Solo un programa** puede usar el puerto COM4 a la vez (Serial Plotter O Monitor, no ambos)
4. **El modo es continuo**: Los datos se generan infinitamente hasta que desconectes

---

## 🛠️ Troubleshooting

**Problema**: No veo datos en Serial Plotter  
**Solución**: Verifica que el puerto esté correcto y baud rate = 115200

**Problema**: Error "Could not open port"  
**Solución**: Cierra cualquier monitor/plotter abierto en ese puerto

**Problema**: Los datos no coinciden con lo esperado  
**Solución**: Verifica que hayas subido el código después de cambiar los #define
