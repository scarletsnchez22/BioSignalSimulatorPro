# Análisis Espectral FFT de Modelos Matemáticos
## BioSignalSimulator Pro - Documentación Técnica

**Revisado:** 06.01.2026  
**Duración de Simulación:** 7 segundos  
**Archivos Generados:** `docs/fft_analysis/`

---

## 1. Introducción

Este documento explica los resultados del análisis FFT (Fast Fourier Transform) aplicado a los tres modelos matemáticos de señales biomédicas implementados en BioSignalSimulator Pro:

| Señal | Modelo Matemático | Referencia |
|-------|-------------------|------------|
| ECG | McSharry ECGSYN | Physionet |
| EMG | Fuglevand MUAP | J. Neurophysiol. 1993 |
| PPG | Allen Gaussiano | Physiol. Meas. 2007 |

### ¿Por qué hacer análisis FFT?

El análisis espectral nos permite:
1. **Verificar** que los modelos generan frecuencias dentro del rango clínico esperado
2. **Validar** que la frecuencia de muestreo del modelo (Fs) cumple Nyquist
3. **Diseñar** filtros RC post-DAC para reconstrucción analógica
4. **Optimizar** recursos computacionales del ESP32

---

## 2. Conceptos Fundamentales

### 2.1 Transformada de Fourier (FFT)

La FFT descompone una señal temporal en sus componentes frecuenciales:

$$X(f) = \int_{-\infty}^{\infty} x(t) \cdot e^{-j2\pi ft} \, dt$$

En forma discreta (como usamos en el análisis):

$$X[k] = \sum_{n=0}^{N-1} x[n] \cdot e^{-j2\pi kn/N}$$

Donde:
- $x[n]$ = muestra de la señal en el tiempo
- $X[k]$ = componente frecuencial en bin $k$
- $N$ = número total de muestras
- Resolución frecuencial: $\Delta f = \frac{F_s}{N}$

### 2.2 Ventana de Hanning

Antes de aplicar FFT, multiplicamos la señal por una ventana de Hanning para reducir **fuga espectral** (spectral leakage):

$$w[n] = 0.5 \left(1 - \cos\left(\frac{2\pi n}{N-1}\right)\right)$$

Esto suaviza los bordes de la señal y mejora la precisión del espectro.

### 2.3 Métricas del Espectro

| Métrica | Descripción |
|---------|-------------|
| **Frecuencia Dominante** | Frecuencia con mayor amplitud (pico del espectro) |
| **BW -3dB** | Ancho de banda donde la potencia cae a la mitad ($10^{-3/20} \approx 0.707$) |
| **BW -20dB** | Ancho de banda donde la potencia cae 100× ($10^{-20/20} = 0.01$) |
| **F 99% Energía** | Frecuencia que contiene el 99% de la energía total |
| **Energía en Banda Clínica** | Porcentaje de energía dentro del BW estándar médico |

---

## 3. Resultados por Señal

### 3.1 ECG (Electrocardiograma)

#### Parámetros del Modelo
```
Modelo: McSharry ECGSYN (Physionet)
Fs modelo: 300 Hz
Muestras analizadas: 2100 (7 segundos)
Resolución FFT: 0.1429 Hz
```

#### Resultados Espectrales

| Métrica | Valor | Interpretación |
|---------|-------|----------------|
| **Frecuencia dominante** | 1.14 Hz | Corresponde a HR ≈ 68 BPM |
| **Amplitud máxima** | 0.1170 | Normalizada |
| **BW -3dB** | 12.0 Hz | Energía principal del QRS |
| **BW -20dB** | 24.0 Hz | Incluye componentes de alta frecuencia |
| **F 99% energía** | 21.6 Hz | Prácticamente toda la información útil |
| **Energía en banda clínica** | 100% | ✓ Dentro de 0.05-150 Hz |

#### Interpretación Clínica

```
Espectro típico ECG:
                           QRS complex
         P,T waves          (edges)
             ↓                 ↓
    ┌────────┬─────────────────┬────────┐
    │████████│█████████████████│░░░░░░░░│
    └────────┴─────────────────┴────────┘
    0.1 Hz   1 Hz            15 Hz     150 Hz
```

- **0.05 - 0.5 Hz:** Ondas P y T (lentas, musculares auriculares)
- **0.5 - 10 Hz:** Componente fundamental del ritmo cardíaco
- **10 - 40 Hz:** Bordes rápidos del complejo QRS
- **> 40 Hz:** Ruido, artefactos musculares

El modelo McSharry genera correctamente el contenido hasta ~25 Hz, capturando P, QRS y T.

#### Filtro RC Recomendado

$$F_c = 2 \times F_{max(-20dB)} = 2 \times 24 = 48 \text{ Hz}$$

Con capacitor $C = 1 \mu F$:

$$R = \frac{1}{2\pi F_c C} = \frac{1}{2\pi \times 48 \times 10^{-6}} = \mathbf{3316 \, \Omega}$$

**Valor comercial sugerido:** 3.3 kΩ

---

### 3.2 EMG (Electromiograma)

#### Parámetros del Modelo
```
Modelo: Fuglevand MUAP (Motor Unit Action Potential)
Fs modelo: 1000 Hz
Muestras analizadas: 7000 (7 segundos)
Resolución FFT: 0.1429 Hz
```

#### Resultados Espectrales

| Métrica | Valor | Interpretación |
|---------|-------|----------------|
| **Frecuencia dominante** | 55.4 Hz | Pico típico de EMG superficial |
| **Amplitud máxima** | 0.0421 | Espectro más distribuido |
| **BW -3dB** | 96.6 Hz | Ancho de banda significativo |
| **BW -20dB** | 158.0 Hz | Límite práctico del contenido |
| **F 99% energía** | 146.3 Hz | Energía concentrada < 150 Hz |
| **Energía en banda clínica** | 99.7% | ✓ Dentro de 20-500 Hz |

#### Interpretación Clínica

```
Espectro típico EMG:
                    Pico típico
                        ↓
    ┌────────────────────┬──────────────────┐
    │░░░░░██████████████████████░░░░░░░░░░░░│
    └────────────────────┴──────────────────┘
    20 Hz              80 Hz              500 Hz
```

- **< 20 Hz:** Artefactos de movimiento (filtrados clínicamente)
- **20 - 150 Hz:** Contenido principal de MUAPs
- **50 - 100 Hz:** Zona de máxima energía (músculos superficiales)
- **150 - 500 Hz:** Componentes de alta frecuencia (MUAPs rápidos)

El modelo Fuglevand genera MUAPs realistas con pico en ~55 Hz, consistente con literatura.

#### Filtro RC Recomendado

$$F_c = 2 \times F_{max(-20dB)} = 2 \times 158 = 316 \text{ Hz}$$

Con capacitor $C = 1 \mu F$:

$$R = \frac{1}{2\pi \times 316 \times 10^{-6}} = \mathbf{504 \, \Omega}$$

**Valor comercial sugerido:** 510 Ω

---

### 3.3 PPG (Fotopletismografía)

#### Parámetros del Modelo
```
Modelo: Allen Gaussiano (pulso arterial)
Fs modelo: 20 Hz
Muestras analizadas: 140 (7 segundos)
Resolución FFT: 0.1429 Hz
```

#### Resultados Espectrales

| Métrica | Valor | Interpretación |
|---------|-------|----------------|
| **Frecuencia dominante** | 1.14 Hz | Corresponde a HR ≈ 68 BPM |
| **Amplitud máxima** | 0.3007 | Señal más "limpia" |
| **BW -3dB** | 1.29 Hz | Banda muy estrecha |
| **BW -20dB** | 4.86 Hz | Prácticamente toda la señal |
| **F 99% energía** | 4.86 Hz | Señal muy concentrada |
| **Energía en banda clínica** | 99.9% | ✓ Dentro de 0.5-10 Hz |

#### Interpretación Clínica

```
Espectro típico PPG:
      Fundamental (HR)   Armónicos
            ↓               ↓
    ┌───────────────────────────────────────┐
    │   ████████░░░░░░░░░░░░░░░░░░░░░░░░░░░░│
    └───────────────────────────────────────┘
    0.5 Hz   1-2 Hz     5 Hz            10 Hz
```

- **0.5 - 2 Hz:** Componente fundamental del pulso
- **2 - 5 Hz:** Armónicos del pulso (forma de onda)
- **5 - 10 Hz:** Componentes menores, ruido respiratorio

El PPG es la señal más "lenta" - por eso Fs = 20 Hz es suficiente.

#### Filtro RC Recomendado

$$F_c = 2 \times F_{max(-20dB)} = 2 \times 4.86 \approx 10 \text{ Hz}$$

Con capacitor $C = 1 \mu F$:

$$R = \frac{1}{2\pi \times 10 \times 10^{-6}} = \mathbf{16384 \, \Omega}$$

**Valor comercial sugerido:** 15 kΩ o 16 kΩ

---

## 4. Tabla Resumen de Diseño

### 4.1 Frecuencias de Muestreo del Modelo

| Señal | Fs Modelo | Fmax Clínica | Nyquist Mínimo | Margen |
|-------|-----------|--------------|----------------|--------|
| ECG | 300 Hz | 150 Hz | 300 Hz | 100% ✓ |
| EMG | 1000 Hz | 500 Hz | 1000 Hz | 100% ✓ |
| PPG | 20 Hz | 10 Hz | 20 Hz | 100% ✓ |

**Conclusión:** Las Fs están en el límite exacto de Nyquist (2× Fmax clínica). Esto es suficiente porque los modelos no generan contenido hasta el límite clínico máximo.

### 4.2 Filtros RC Post-DAC

| Señal | Fc Sugerida | R (C=1µF) | R Comercial | Atenuación @ Fs/2 |
|-------|-------------|-----------|-------------|-------------------|
| ECG | 48 Hz | 3316 Ω | 3.3 kΩ | -15 dB @ 150 Hz |
| EMG | 316 Hz | 504 Ω | 510 Ω | -10 dB @ 500 Hz |
| PPG | 10 Hz | 16384 Ω | 15 kΩ | -6 dB @ 10 Hz |

### 4.3 Diagrama de Circuito RC

```
                 R
    DAC ────/\/\/\/────┬──── Vout
                       │
                       │
                      ─┴─
                      ─┬─ C = 1µF
                       │
                      ─┴─ GND
```

Función de transferencia:

$$H(f) = \frac{1}{1 + j\frac{f}{F_c}}$$

$$|H(f)| = \frac{1}{\sqrt{1 + \left(\frac{f}{F_c}\right)^2}}$$

---

## 5. Validación de Criterio de Nyquist

### Teorema de Nyquist-Shannon

> "Para reconstruir una señal sin aliasing, la frecuencia de muestreo debe ser al menos el doble de la frecuencia máxima de la señal."

$$F_s \geq 2 \times F_{max}$$

### Verificación por Señal

| Señal | Fmax Real (99% energía) | Fs Modelo | ¿Cumple Nyquist? |
|-------|-------------------------|-----------|------------------|
| ECG | 21.6 Hz | 300 Hz | ✓ Fs > 2×21.6 = 43.2 Hz |
| EMG | 146.3 Hz | 1000 Hz | ✓ Fs > 2×146.3 = 292.6 Hz |
| PPG | 4.86 Hz | 20 Hz | ✓ Fs > 2×4.86 = 9.7 Hz |

**Conclusión:** Todas las señales cumplen holgadamente el criterio de Nyquist.

---

## 6. Interpretación de Gráficos Generados

Los gráficos en `docs/fft_analysis/` contienen 4 subplots cada uno:

### Panel Superior Izquierdo: Señal Temporal
- Eje X: Tiempo (segundos)
- Eje Y: Amplitud normalizada
- Muestra los primeros 2 segundos de la señal generada

### Panel Superior Derecho: Espectro de Amplitud
- Eje X: Frecuencia (Hz)
- Eje Y: Amplitud normalizada
- Línea vertical verde: Frecuencia dominante
- Área sombreada: Banda clínica estándar

### Panel Inferior Izquierdo: Espectro en dB
- Eje X: Frecuencia (Hz)
- Eje Y: Potencia (dB)
- Líneas horizontales: -3dB y -20dB
- Permite ver componentes de baja amplitud

### Panel Inferior Derecho: Energía Acumulada
- Eje X: Frecuencia (Hz)
- Eje Y: Energía acumulada (%)
- Línea horizontal: 99%
- Muestra cómo se distribuye la energía frecuencial

---

## 7. Implicaciones para el Firmware

### 7.1 Arquitectura de Generación

```
┌─────────────────────────────────────────────────────────────┐
│                    ESP32 - BioSignalSimulator               │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌──────────┐    ┌──────────┐    ┌──────────┐    ┌──────┐  │
│  │  Modelo  │───>│Interpol. │───>│   DAC    │───>│ RC   │──>│ Vout
│  │ @ Fs_mod │    │ @ Fs_out │    │  8-bit   │    │Filter│  │
│  └──────────┘    └──────────┘    └──────────┘    └──────┘  │
│                                                             │
│  ECG: 300 Hz ──> 6000 Hz ──> 0-3.3V ──> Fc=48Hz            │
│  EMG: 1000 Hz ─> 6000 Hz ──> 0-3.3V ──> Fc=316Hz           │
│  PPG: 20 Hz ──> 6000 Hz ──> 0-3.3V ──> Fc=10Hz             │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### 7.2 Beneficios de la Arquitectura

1. **Modelo a Fs_modelo baja:**
   - Menor carga computacional
   - Buffers más pequeños
   - Más tiempo para cálculos complejos

2. **Interpolación a Fs_out alta:**
   - DAC opera a frecuencia fija
   - Simplifica timing de interrupciones
   - Mejor reconstrucción analógica

3. **Filtro RC post-DAC:**
   - Elimina "escalones" del DAC
   - Suaviza la señal
   - Atenúa frecuencias de aliasing

---

## 8. Referencias

1. **McSharry PE, et al.** "A dynamical model for generating synthetic electrocardiogram signals." IEEE Trans Biomed Eng. 2003;50(3):289-294.

2. **Fuglevand AJ, et al.** "Models of recruitment and rate coding organization in motor-unit pools." J Neurophysiol. 1993;70(6):2470-2488.

3. **Allen J.** "Photoplethysmography and its application in clinical physiological measurement." Physiol Meas. 2007;28(3):R1-R39.

4. **IEC 60601-2-27:2011** - Medical electrical equipment - Particular requirements for electrocardiographic monitoring equipment.

5. **SENIAM recommendations** - Surface ElectroMyoGraphy for the Non-Invasive Assessment of Muscles.

---

## 9. Archivos Generados

```
docs/fft_analysis/
├── fft_modelo_ECG.png          # Gráfico espectral ECG
├── fft_modelo_EMG.png          # Gráfico espectral EMG
├── fft_modelo_PPG.png          # Gráfico espectral PPG
└── fft_modelos_reporte.txt     # Reporte de texto plano
```

---

*Generado automáticamente por `tools/model_fft_analysis.py`*  
*BioSignalSimulator Pro - 2026*
