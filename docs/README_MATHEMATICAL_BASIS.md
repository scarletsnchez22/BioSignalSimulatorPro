# BioSimulator Pro - Fundamentos Matemáticos y Científicos

## Información del Documento
| Campo | Valor |
|-------|-------|
| **Versión** | 1.0.0 |
| **Estado** | Referencia Técnica Completa |

---

## Índice

### Modelo ECG
1. [Modelo de Generación ECG](#1-modelo-de-generación-ecg)
2. [Variabilidad del Ritmo Cardíaco (HRV)](#2-variabilidad-del-ritmo-cardíaco-hrv)
3. [Variabilidad Morfológica Latido-a-Latido](#3-variabilidad-morfológica-latido-a-latido)
4. [Implementación de Patologías ECG](#4-implementación-de-patologías-ecg)
5. [Sistema de Escalado ECG y Conversión DAC](#5-sistema-de-escalado-ecg-y-conversión-dac)

### Modelo EMG
6. [Modelo de Generación EMG](#6-modelo-de-generación-emg)
7. [Reclutamiento de Unidades Motoras](#7-reclutamiento-de-unidades-motoras)
8. [Forma del MUAP: Mexican Hat Wavelet](#8-forma-del-muap-mexican-hat-wavelet)
9. [Condiciones Neuromusculares](#9-condiciones-neuromusculares)
10. [Sistema de Escalado EMG](#10-sistema-de-escalado-emg)

### Modelo PPG
11. [Modelo de Generación PPG](#11-modelo-de-generación-ppg)
12. [Parámetros Temporales y Morfológicos](#12-parámetros-temporales-y-morfológicos)
13. [Condiciones Clínicas PPG](#13-condiciones-clínicas-ppg)
14. [Sistema de Escalado PPG](#14-sistema-de-escalado-ppg)

### Referencias
15. [Referencias Científicas](#15-referencias-científicas)

---

## 1. Modelo de Generación ECG

### 1.1 Fundamento: Modelo Dinámico de McSharry

El simulador ECG está basado en el modelo dinámico desarrollado por **McSharry, Clifford, Tarassenko y Smith (2003)** [1]. Este modelo es ampliamente validado y utilizado en investigación de procesamiento de señales biomédicas.

> **Referencia Principal:**  
> McSharry PE, Clifford GD, Tarassenko L, Smith L.  
> *"A dynamical model for generating synthetic electrocardiogram signals."*  
> IEEE Trans Biomed Eng. 2003;50(3):289-294.  
> DOI: [10.1109/TBME.2003.808805](https://doi.org/10.1109/TBME.2003.808805)

### 1.2 Ecuaciones del Sistema Dinámico

El modelo McSharry utiliza **tres ecuaciones diferenciales ordinarias (ODEs) acopladas** que describen una trayectoria en el espacio 3D. El punto de estado $(x, y)$ rota alrededor del círculo unitario mientras $z$ genera la forma de onda ECG.

#### Variables de Estado

| Variable | Descripción |
|----------|-------------|
| $x(t)$ | Componente X en el círculo unitario |
| $y(t)$ | Componente Y en el círculo unitario |
| $z(t)$ | **Salida ECG** (en mV equivalentes) |
| $\theta(t)$ | Ángulo actual: $\theta = \arctan2(y, x)$ |

#### Ecuaciones Diferenciales

**Ecuación 1 - Dinámica en X:**
$$\frac{dx}{dt} = \alpha \cdot x - \omega \cdot y$$

**Ecuación 2 - Dinámica en Y:**
$$\frac{dy}{dt} = \alpha \cdot y + \omega \cdot x$$

**Ecuación 3 - Forma de onda ECG (Z):**
$$\frac{dz}{dt} = -\sum_{i=1}^{5} a_i \cdot \Delta\theta_i \cdot \exp\left(-\frac{\Delta\theta_i^2}{2b_i^2}\right) - (z - z_0)$$

#### Definiciones de Variables

| Símbolo | Fórmula | Descripción |
|---------|---------|-------------|
| $\alpha$ | $1 - \sqrt{x^2 + y^2}$ | Factor de atracción al círculo unitario |
| $\omega$ | $\displaystyle\frac{2\pi}{RR}$ | Velocidad angular (rad/s) |
| $\Delta\theta_i$ | $(\theta - \theta_i) \mod [-\pi, \pi]$ | Distancia angular a cada onda |
| $z_0$ | $0.04$ | Línea base (offset isoeléctrico) |

### 1.3 Parámetros de las Ondas PQRST

Las cinco ondas del ciclo cardíaco están controladas por tres conjuntos de parámetros. Valores del código MATLAB original de PhysioNet [2]:

| Onda | Índice | $\theta_i$ (grados) | $\theta_i$ (radianes) | $a_i$ (amplitud) | $b_i$ (ancho) |
|------|--------|---------------------|----------------------|------------------|---------------|
| **P** | 0 | −70° | −1.222 | 1.2 | 0.25 |
| **Q** | 1 | −15° | −0.262 | −5.0 | 0.10 |
| **R** | 2 | 0° | 0 | 30.0 | 0.10 |
| **S** | 3 | +15° | +0.262 | −7.5 | 0.10 |
| **T** | 4 | +100° | +1.745 | 0.75 | 0.40 |

**Interpretación de parámetros:**
- $\theta_i$: Posición angular de cada onda en el ciclo cardíaco
- $a_i$: Amplitud relativa (signo indica dirección: + hacia arriba, − hacia abajo)
- $b_i$: Ancho de la Gaussiana que forma cada onda

### 1.4 Corrección de Bazett

La fórmula de Bazett [3] ajusta el intervalo QT según la frecuencia cardíaca:

$$QT_c = \frac{QT}{\sqrt{RR}}$$

En el modelo, esto se implementa escalando los parámetros de ancho ($b_i$) y posición ($\theta_i$):

$$b_i^{ajustado} = b_i^{base} \cdot \sqrt{RR}$$
$$\theta_i^{ajustado} = \theta_i^{base} \cdot \sqrt{RR}$$

Donde $RR$ está en segundos. Esto preserva las relaciones temporales del ECG a diferentes frecuencias cardíacas.

### 1.5 Integración Numérica: Runge-Kutta 4º Orden

Las ODEs se resuelven usando el método **RK4**, que ofrece error de truncamiento $O(h^5)$:

$$k_1 = f(t_n, y_n)$$
$$k_2 = f\left(t_n + \frac{h}{2}, y_n + \frac{h}{2}k_1\right)$$
$$k_3 = f\left(t_n + \frac{h}{2}, y_n + \frac{h}{2}k_2\right)$$
$$k_4 = f(t_n + h, y_n + h \cdot k_3)$$
$$y_{n+1} = y_n + \frac{h}{6}(k_1 + 2k_2 + 2k_3 + k_4)$$

**Parámetros de implementación:**
- Paso de tiempo: $h = 0.001$ s (1 ms)
- Frecuencia de muestreo: $f_s = 1000$ Hz

---

## 2. Variabilidad del Ritmo Cardíaco (HRV)

### 2.1 Fundamento Científico

La variabilidad del intervalo RR es una característica fundamental del ritmo cardíaco normal, documentada extensivamente en el estándar del Task Force 1996 [4]:

> **Referencia:**  
> Task Force of the European Society of Cardiology and the North American Society of Pacing and Electrophysiology.  
> *"Heart Rate Variability: Standards of Measurement, Physiological Interpretation, and Clinical Use."*  
> Circulation. 1996;93(5):1043-1065.  
> DOI: [10.1161/01.CIR.93.5.1043](https://doi.org/10.1161/01.CIR.93.5.1043)

### 2.2 Valores Normales de HRV

Del Task Force 1996, Tabla 5 (valores normales en adultos sanos):

| Medida | Valor Normal | Descripción |
|--------|--------------|-------------|
| **SDNN** | 141 ± 39 ms | Desviación estándar de RR en 24h |
| **RMSSD** | 27 ± 12 ms | Variabilidad latido-a-latido |
| **CV** | 5-8% | Coeficiente de variación en reposo |

### 2.3 Implementación de HRV

El intervalo RR se genera con variabilidad Gaussiana:

$$RR_n = RR_{medio} + \mathcal{N}(0, \sigma_{RR})$$

Donde:
- $RR_{medio} = \displaystyle\frac{60}{HR_{BPM}}$ segundos
- $\sigma_{RR} = CV \times RR_{medio}$
- $CV$ = Coeficiente de variación (5-8% para ritmo normal)

### 2.4 Rangos Fisiológicos por Condición

Basados en guías clínicas [5][6][7]:

| Condición | HR (BPM) | CV (%) | Fuente |
|-----------|----------|--------|--------|
| Normal | 60-100 | 5-8% | Task Force 1996 |
| Taquicardia Sinusal | 100-180 | 3-6% | AHA Guidelines |
| Bradicardia Sinusal | 30-59 | 2-5% | ACC/AHA 2017 |
| Fibrilación Auricular | 60-180 | 15-35% | January 2014 |
| PVC | 50-120 | 4-10% | Literatura clínica |
| Bloqueo de Rama | 40-100 | 3-7% | ECG textbooks |

---

## 3. Variabilidad Morfológica Latido-a-Latido

### 3.1 Justificación

En un ECG real, cada latido presenta variaciones sutiles debido a:
- Movimiento torácico por respiración
- Modulación del sistema nervioso autónomo
- Variaciones en la conducción cardíaca

### 3.2 Parámetros de Variabilidad

| Parámetro | Valor | Justificación |
|-----------|-------|---------------|
| Variación de amplitud | ±5% | Modulación respiratoria del vector cardíaco [8] |
| Variación de ancho | ±2% | Estimación conservadora empírica |
| Variación de posición | ±0.01 rad | Timing sutil (~0.6°) |
| Frecuencia de deriva baseline | 0.25 Hz | Centro del rango RSA (0.15-0.4 Hz) [4] |

### 3.3 Implementación

Cada latido aplica variaciones aleatorias Gaussianas:

$$a_i^{latido} = a_i^{base} \times (1 + \mathcal{N}(0, 0.05))$$
$$b_i^{latido} = b_i^{base} \times (1 + \mathcal{N}(0, 0.02))$$
$$\theta_i^{latido} = \theta_i^{base} + \mathcal{N}(0, 0.01)$$

**Nota:** La variabilidad morfológica es una estimación empírica. No existe un estándar publicado específico para variabilidad de parámetros McSharry latido-a-latido.

---

## 4. Implementación de Patologías ECG

### 4.1 Ritmo Sinusal Normal

Morfología base con todos los parámetros estándar de McSharry.

### 4.2 Taquicardia y Bradicardia Sinusal

Solo cambia la frecuencia cardíaca:
- **Taquicardia:** HR seleccionado aleatoriamente de [100, 180] BPM
- **Bradicardia:** HR seleccionado aleatoriamente de [30, 59] BPM

La corrección de Bazett ajusta automáticamente la morfología.

### 4.3 Fibrilación Auricular (AFib)

| Característica | Implementación |
|----------------|----------------|
| Sin ondas P | $a_0 = 0$ |
| RR muy irregular | CV = 15-35% |
| Respuesta ventricular variable | HR ∈ [60, 180] BPM |

### 4.4 Fibrilación Ventricular (VFib)

> ⚠️ **IMPORTANTE:** El modelo McSharry **NO puede simular VFib** porque genera ondas PQRST organizadas. VFib es caos eléctrico sin estructura reconocible.

**Modelo Alternativo: Superposición Espectral**

$$VFib(t) = \sum_{k=1}^{5} A_k(t) \cdot \sin(2\pi f_k t + \phi_k(t)) + \mathcal{N}(0, 0.06)$$

| Parámetro | Rango | Descripción |
|-----------|-------|-------------|
| $f_k$ | 4-10 Hz | Frecuencia dominante de VFib [9] |
| $A_k(t)$ | 0.04-0.30 | Amplitud modulada lentamente |
| $\phi_k(t)$ | Deriva lenta | Fase con variación aleatoria |

**Justificación:** El rango 4-10 Hz corresponde al espectro real de VFib. VFib "gruesa" (coarse): 4-6 Hz. VFib "fina" (fine): 6-10 Hz.

### 4.5 Contracciones Ventriculares Prematuras (PVC)

| Característica | Implementación |
|----------------|----------------|
| Sin onda P | $a_0 = 0$ (origen ventricular) |
| QRS ancho | $b_{1,2,3} = 0.18$ (vs 0.10 normal) |
| Morfología aberrante | $a_2$ modificado aleatoriamente |
| T discordante | $a_4$ invertido |
| Timing prematuro | $RR_{PVC} = RR_{normal} \times 0.7$ |
| Pausa compensatoria | $RR_{siguiente} = RR_{normal} \times 1.3$ |

### 4.6 Bloqueo de Rama (BBB)

QRS ensanchado (>120 ms) implementado aumentando el ancho de las Gaussianas:

$$b_1 = b_2 = b_3 = 0.15 \text{ rad}$$

### 4.7 Elevación y Depresión ST

El segmento ST ocurre entre las ondas S y T:
- **Rango angular:** $\theta \in [0.35, 1.40]$ radianes (~20° a ~80°)

**Implementación:**
$$z_{ST} = z_{base} + ST_{nivel} \cdot \sin^2\left(\frac{\theta - 0.35}{1.05} \cdot \pi\right)$$

| Condición | $ST_{nivel}$ | Significado Clínico |
|-----------|--------------|---------------------|
| Normal | 0 | Isoeléctrico |
| STEMI | +0.15 a +0.35 | Infarto agudo con elevación ST |
| Isquemia | −0.35 a −0.15 | Depresión ST por isquemia subendocárdica |

---

## 5. Sistema de Escalado ECG y Conversión DAC

### 5.1 Definición de Escala

El modelo McSharry produce $z(t)$ en **unidades adimensionales**. Para uso educativo y clínico, definimos:

$$\boxed{1.0 \text{ unidad del modelo} \equiv 1.0 \text{ mV}}$$

Esta equivalencia es consistente con ECG Lead II típico:
- Onda R: 0.5-1.5 mV
- Onda P: 0.1-0.25 mV
- Onda T: 0.1-0.5 mV
- Baseline: 0 mV

### 5.2 Rango del Modelo

Con los parámetros McSharry estándar:

| Punto | Valor (mV eq.) |
|-------|----------------|
| Pico R | ~1.0 a 1.2 |
| Baseline | ~0.04 |
| Valle Q/S | ~−0.3 a −0.4 |

**Rango total definido:** $[-0.5, +1.5]$ mV

### 5.3 Conversión a DAC

El ESP32 tiene DAC de 8 bits (0-255) con salida 0-3.3V.

**Mapeo lineal:**

$$DAC = \frac{z_{mV} - (-0.5)}{2.0} \times 255$$

| Valor ECG | Valor DAC | Voltaje Salida |
|-----------|-----------|----------------|
| −0.5 mV | 0 | 0.00 V |
| 0.0 mV | 64 | 0.83 V |
| +1.0 mV | 191 | 2.47 V |
| +1.5 mV | 255 | 3.30 V |

**Factor de conversión para osciloscopio:**
$$1 \text{ mV}_{modelo} = 1.65 \text{ V}_{DAC}$$

---

## 6. Modelo de Generación EMG

### 6.1 Fundamento: Diferencia con ECG

A diferencia del ECG (señal determinista y periódica), el EMG de superficie (sEMG) es una señal **estocástica** (aleatoria controlada) que resulta de la superposición de potenciales de acción de múltiples unidades motoras.

> **Referencia Principal:**  
> Fuglevand AJ, Winter DA, Patla AE.  
> *"Models of recruitment and rate coding organization in motor-unit pools."*  
> J Neurophysiol. 1993;70(6):2470-2488.  
> DOI: [10.1152/jn.1993.70.6.2470](https://doi.org/10.1152/jn.1993.70.6.2470)

### 6.2 Concepto de Unidad Motora

Una **unidad motora (MU)** es la unidad funcional básica del control motor:

$$\text{Unidad Motora} = 1 \text{ motoneurona} + n \text{ fibras musculares inervadas}$$

El número de fibras por MU varía según el músculo:
- Músculos oculares: 3-10 fibras/MU (control fino)
- Músculos de extremidades: 100-2000 fibras/MU (fuerza)

### 6.3 Señal EMG como Suma de MUAPs

La señal EMG en cualquier instante es la superposición de todos los potenciales de acción de unidades motoras (MUAPs) activos:

$$EMG(t) = \sum_{i=1}^{N} \sum_{j} h_i(t - t_{ij}) + n(t)$$

Donde:
- $N$ = número de unidades motoras en el pool (100 en nuestro modelo)
- $h_i(t)$ = forma del MUAP de la i-ésima unidad motora
- $t_{ij}$ = tiempo del j-ésimo disparo de la i-ésima MU
- $n(t)$ = ruido de fondo (interferencia, electrodo)

---

## 7. Reclutamiento de Unidades Motoras

### 7.1 Principio de Henneman (Principio del Tamaño)

Las unidades motoras se reclutan en orden de tamaño, desde las más pequeñas hasta las más grandes:

> **Referencia:**  
> Henneman E, Somjen G, Carpenter DO.  
> *"Functional significance of cell size in spinal motoneurons."*  
> J Neurophysiol. 1965;28:560-580.

**Implicaciones del principio del tamaño:**

| MUs Pequeñas (reclutadas primero) | MUs Grandes (reclutadas después) |
|-----------------------------------|----------------------------------|
| Tipo I (lentas, oxidativas) | Tipo II (rápidas, glucolíticas) |
| Resistentes a fatiga | Fatigables |
| Bajo umbral de reclutamiento | Alto umbral de reclutamiento |
| MUAPs pequeños (0.3-0.5 mV) | MUAPs grandes (1-2 mV) |
| Control fino de fuerza | Generación de fuerza máxima |

### 7.2 Distribución de Umbrales de Reclutamiento

Según Fuglevand (1993), los umbrales de reclutamiento siguen una distribución exponencial:

$$\theta_i = \frac{\exp\left(\ln(RR) \cdot \frac{i}{n}\right) - 1}{RR - 1} \cdot \theta_{max}$$

Donde:
- $\theta_i$ = umbral de la i-ésima MU (normalizado 0-1)
- $RR$ = rango de reclutamiento (típico 30-60)
- $n$ = número total de MUs
- $\theta_{max}$ = excitación máxima para reclutar todas las MUs (~0.6)

**Interpretación:** Las primeras MUs se reclutan a excitaciones muy bajas. El 60% de la excitación ya recluta todas las MUs; el 40% restante se usa para aumentar la frecuencia de disparo.

### 7.3 Frecuencias de Disparo

Una vez reclutada, la frecuencia de disparo de una MU aumenta linealmente con la excitación:

> **Referencia:**  
> De Luca CJ, Hostage EC.  
> *"Relationship between firing rate and recruitment threshold of motoneurons."*  
> J Neurophysiol. 2010;104(2):1034-1046.

$$FR_i = FR_{min} + g \cdot (E - \theta_i)$$

Donde:
- $FR_i$ = frecuencia de disparo de la MU i (Hz)
- $FR_{min}$ = frecuencia mínima al reclutamiento (6-8 Hz)
- $g$ = ganancia (~40 Hz por unidad de excitación)
- $E$ = nivel de excitación actual (0-1)
- $\theta_i$ = umbral de la MU i

**Rangos fisiológicos de frecuencia:**

| Condición | Rango FR | Fuente |
|-----------|----------|--------|
| Al reclutamiento | 6-8 Hz | De Luca 2010 |
| Contracción sostenida | 10-30 Hz | De Luca 1997 |
| MVC (contracción máxima) | 30-50 Hz | De Luca 2010 |
| Picos transitorios | hasta 60 Hz | Literatura |

### 7.4 Variabilidad del Intervalo de Disparo (ISI)

El intervalo entre disparos (Inter-Spike Interval) no es constante sino que varía:

$$ISI_j = \frac{1}{FR} \cdot (1 + \mathcal{N}(0, CV_{ISI}))$$

Donde $CV_{ISI}$ = 0.15-0.25 (15-25% de coeficiente de variación).

Esta variabilidad es fisiológica y produce el carácter "ruidoso" del EMG.

---

## 8. Forma del MUAP: Mexican Hat Wavelet

### 8.1 Justificación de la Forma Trifásica

Un MUAP real tiene forma **trifásica** (positivo-negativo-positivo) debido a la propagación del potencial de acción a lo largo de las fibras musculares. La fase negativa central es la más prominente.

### 8.2 Modelo Matemático: Segunda Derivada de Gaussiana

Usamos la **Mexican Hat Wavelet** (segunda derivada de una Gaussiana) para modelar el MUAP:

$$MUAP(t) = A \cdot \left(1 - \frac{(t-t_0)^2}{\sigma^2}\right) \cdot \exp\left(-\frac{(t-t_0)^2}{2\sigma^2}\right)$$

Donde:
- $A$ = amplitud del MUAP (mV)
- $t_0$ = centro del MUAP (típicamente DURACIÓN/2)
- $\sigma$ = parámetro de ancho (2 ms típico)

### 8.3 Parámetros del MUAP

| Parámetro | Valor | Justificación |
|-----------|-------|---------------|
| Duración total | 12 ms | Rango típico 8-15 ms [De Luca 1997] |
| Sigma ($\sigma$) | 2 ms | Controla ancho de fases |
| Amplitud MUs pequeñas | 0.3 mV | Primeras en reclutar |
| Amplitud MUs grandes | 1.5 mV | Últimas en reclutar |

### 8.4 Forma Visual del MUAP

```
      ╭──╮                    ╭──╮
     ╱    ╲                  ╱    ╲     Fases positivas pequeñas
────╯      ╲              ╱      ╰────
            ╲            ╱
             ╲          ╱
              ╲        ╱
               ╲──────╱                 Fase negativa dominante
               
    0ms    4ms    6ms    8ms    12ms
```

---

## 9. Condiciones Neuromusculares

### 9.1 Tabla de Condiciones

| Condición | Excitación | Modificación MUs | Característica |
|-----------|------------|------------------|----------------|
| REST | 0% | Normal | Solo ruido de fondo |
| MILD | 20% | Normal | Pocas MUs, baja frecuencia |
| MODERATE | 50% | Normal | Patrón de interferencia parcial |
| STRONG | 80% | Normal | Interferencia densa |
| MAXIMUM | 100% | Normal | Interferencia completa |
| TREMOR | 30% oscilatoria | Normal | Modulación 4-6 Hz |
| MYOPATHY | 40% | Amplitud × 0.4 | MUAPs pequeños |
| NEUROPATHY | 50% | 2/3 perdidas, resto × 2.5 | MUAPs gigantes |
| FASCICULATION | 0% | Disparos aleatorios | Espontáneos |
| FATIGUE | 60%→90% | FR reducida | Progresivo |

### 9.2 Miopatía

En miopatías (distrofias, miositis), hay **pérdida de fibras musculares** dentro de cada unidad motora. Esto produce:

$$A_{miopatía} = A_{normal} \times 0.4$$

- MUAPs de menor amplitud
- MUAPs polifásicos (no simulado en detalle)
- Mayor reclutamiento para compensar debilidad

### 9.3 Neuropatía

En neuropatías, hay **pérdida de motoneuronas** con **reinervación colateral**. Las MUs supervivientes adoptan fibras huérfanas:

$$A_{neuropatía} = A_{normal} \times 2.5$$

- Pérdida de 2/3 de las MUs (umbral → inalcanzable)
- MUAPs gigantes en MUs supervivientes
- Patrón de reclutamiento reducido

### 9.4 Temblor Parkinsoniano

El temblor de reposo en Parkinson tiene frecuencia característica de 4-6 Hz:

$$E(t) = 0.3 \times \left(0.5 + 0.5 \cdot \sin(2\pi \cdot 5 \cdot t)\right)$$

La excitación oscila sinusoidalmente, produciendo activación y relajación rítmica.

### 9.5 Variabilidad Natural de Fuerza

El control motor humano no es perfecto. Hay fluctuaciones naturales en la fuerza producida:

> **Referencia:**  
> Enoka RM, Christou EA, Hunter SK, et al.  
> *"Mechanisms that contribute to differences in motor performance."*  
> J Electromyogr Kinesiol. 2003;13(1):1-12.

$$E(t) = E_{base} \cdot \left(1 + \Delta E(t)\right)$$

Donde:
- $\Delta E(t) = 0.04 \cdot \sin(2\pi \cdot 2 \cdot t) + \mathcal{N}(0, 0.02)$
- Fluctuación del ±4% a ~2 Hz más componente aleatorio

---

## 10. Sistema de Escalado EMG

### 10.1 Amplitudes sEMG Típicas

> **Referencia:**  
> De Luca CJ.  
> *"The use of surface electromyography in biomechanics."*  
> J Appl Biomech. 1997;13:135-163.

| Nivel de Contracción | Amplitud RMS | Picos |
|----------------------|--------------|-------|
| Reposo | < 50 µV | — |
| Leve (20% MVC) | 0.1-0.5 mV | ±1 mV |
| Moderada (50% MVC) | 0.5-1.5 mV | ±2 mV |
| Máxima (100% MVC) | 1.5-4.0 mV | ±4 mV |
| Neuropatía (MUAPs gigantes) | Variable | ±5 mV |

### 10.2 Rango del Modelo

Para capturar tanto EMG normal como patológico (especialmente MUAPs gigantes de neuropatía):

$$\boxed{\text{Rango EMG} = \pm 5 \text{ mV}}$$

### 10.3 Conversión a DAC

Mapeo lineal a DAC de 8 bits:

$$DAC = 128 + \frac{V_{mV}}{5.0} \times 127$$

| Valor EMG | Valor DAC | Voltaje Salida |
|-----------|-----------|----------------|
| −5.0 mV | 0 | 0.00 V |
| 0.0 mV | 128 | 1.65 V |
| +5.0 mV | 255 | 3.30 V |

**Factor de conversión para osciloscopio:**
$$1 \text{ mV}_{modelo} = 0.33 \text{ V}_{DAC}$$

---

## 11. Modelo de Generación PPG

### 11.1 Fundamento: Fotopletismografía

El fotopletismograma (PPG) mide cambios de volumen sanguíneo en tejido periférico mediante absorción de luz. A diferencia del ECG (señal eléctrica) y EMG (superposición de MUAPs), el PPG es una señal **óptica** que refleja la dinámica cardiovascular.

> **Referencias Principales:**
> 
> Allen J.  
> *"Photoplethysmography and its application in clinical physiological measurement."*  
> Physiological Measurement. 2007;28:R1-R39.  
> DOI: [10.1088/0967-3334/28/3/R01](https://doi.org/10.1088/0967-3334/28/3/R01)
>
> Elgendi M.  
> *"On the Analysis of Fingertip Photoplethysmogram Signals."*  
> Current Cardiology Reviews. 2012;8(1):14-25.  
> DOI: [10.2174/157340312801215782](https://doi.org/10.2174/157340312801215782)

### 11.2 Modelo Matemático: Doble Gaussiana

El pulso PPG se modela como la suma de tres componentes gaussianas:

$$PPG(t) = DC + AC \cdot \left[ G_{sist}(t) + G_{diast}(t) - G_{notch}(t) \right]$$

Donde cada componente gaussiana tiene la forma:

$$G_i(t) = A_i \cdot \exp\left(-\frac{(\phi - \phi_i)^2}{2\sigma_i^2}\right)$$

#### Variables del Modelo

| Variable | Descripción |
|----------|-------------|
| $\phi$ | Fase en el ciclo cardíaco (0-1) |
| $DC$ | Componente DC (absorción tisular constante) |
| $AC$ | Escala del componente pulsátil |
| $G_{sist}$ | Gaussiana del pico sistólico |
| $G_{diast}$ | Gaussiana del pico diastólico |
| $G_{notch}$ | Gaussiana de la muesca dicrótica (resta) |

### 11.3 Origen Fisiológico de los Componentes

| Componente | Origen Fisiológico | Referencia |
|------------|-------------------|------------|
| **Pico Sistólico** | Onda de presión directa del ventrículo izquierdo | Allen 2007 |
| **Muesca Dicrótica** | Cierre de válvula aórtica (incisura) | Elgendi 2012 |
| **Pico Diastólico** | Reflexión de onda desde bifurcación aórtica | Millasseau 2006 |

> **Referencia Adicional:**
> 
> Millasseau SC, Kelly RP, Ritter JM, Chowienczyk PJ.  
> *"Determination of age-related increases in large artery stiffness by digital pulse contour analysis."*  
> Clinical Science. 2002;103:371-377.

---

## 12. Parámetros Temporales y Morfológicos

### 12.1 Posiciones Temporales (Elgendi 2012)

Las posiciones de los puntos fiduciales del PPG están expresadas como fracción del ciclo RR:

| Punto Fiducial | Posición ($\phi$) | Tiempo a 75 BPM | Justificación |
|----------------|-------------------|-----------------|---------------|
| **Pico Sistólico** | 0.15 (15%) | ~120 ms | Elgendi 2012, Fig. 3 |
| **Muesca Dicrótica** | 0.30 (30%) | ~240 ms | Cierre válvula aórtica |
| **Pico Diastólico** | 0.40 (40%) | ~320 ms | Reflexión de onda |

### 12.2 Anchos Gaussianos (Millasseau 2006)

Los anchos ($\sigma$) determinan la duración de cada fase:

| Componente | $\sigma$ | FWHM aproximado | Justificación |
|------------|----------|-----------------|---------------|
| Sistólico | 0.055 | ~90 ms | Eyección ventricular rápida |
| Diastólico | 0.10 | ~160 ms | Onda reflejada dispersada |
| Muesca | 0.02 | ~32 ms | Evento valvular rápido |

**Nota:** FWHM = Full Width at Half Maximum ≈ $2.355 \times \sigma$

### 12.3 Amplitudes Relativas (Allen 2007)

| Parámetro | Valor | Rango Fisiológico | Fuente |
|-----------|-------|-------------------|--------|
| Ratio Diastólico/Sistólico | 0.40 | 0.30 - 0.60 | Allen 2007, Tabla 1 |
| Profundidad Muesca | 0.25 | 0.10 - 0.40 | Variable con edad |
| Ratio AC/DC | ~0.01-0.05 | Variable | Define el Perfusion Index |

### 12.4 Índice de Perfusión (PI)

El Índice de Perfusión cuantifica la relación entre componentes pulsátil y estático:

$$PI = \frac{AC}{DC} \times 100\%$$

> **Referencia:**
> 
> Lima AP, Bakker J.  
> *"Noninvasive monitoring of peripheral perfusion."*  
> Intensive Care Medicine. 2005;31:1316-1326.  
> DOI: [10.1007/s00134-005-2790-2](https://doi.org/10.1007/s00134-005-2790-2)

| Estado Clínico | PI (%) | Interpretación |
|----------------|--------|----------------|
| Normal | 2-5% | Perfusión periférica adecuada |
| Vasodilatación | 5-20% | Fiebre, sepsis temprana |
| Vasoconstricción marcada | 0.2-0.8% | Frío extremo, vasopresores |
| Shock / Perfusión débil | 0.1-0.5% | Hipoperfusión severa |

**Nota:** PI y SpO2 son valores dinámicos con variabilidad gaussiana natural en el modelo.

---

## 13. Condiciones Clínicas PPG

### 13.1 Tabla de Condiciones y Parámetros

| Condición | Sistólica | Diastólica | Muesca | HR | PI (%) | SpO2 (%) |
|-----------|-----------|------------|--------|-----|--------|----------|
| **NORMAL** | 1.0 | 0.4 | user | 75 | 2-5 | 95-100 |
| **ARRHYTHMIA** | 1.0 | 0.4 | user | 85* | 1-5 | 92-100 |
| **WEAK_PERFUSION** | 0.25 | 0.08 | 0.1 | 115 | 0.1-0.5 | 88-98 |
| **STRONG_PERFUSION** | 1.6 | 0.7 | 0.35 | 72 | 5-20 | 96-100 |
| **VASOCONSTRICTION** | 0.30 | 0.08 | 0.05 | 78 | 0.2-0.8 | 91-100 |
| **LOW_SPO2** | 0.8 | 0.3 | 0.2 | 110 | 0.5-3.5 | 70-90 |

*ARRHYTHMIA: HR muy variable (CV_RR=15%), base 85 BPM

**Nota:** PI y SpO2 son valores dinámicos con variabilidad gaussiana.

### 13.2 Perfusión Débil (WEAK_PERFUSION)

Simula shock hipovolémico o hipoperfusión periférica severa.

> **Referencia:**
> 
> Reisner A, Shaltis PA, McCombie D, Asada HH.  
> *"Utility of the photoplethysmogram in circulatory monitoring."*  
> Anesthesiology. 2008;108(5):950-958.

**Cambios morfológicos:**
- Amplitud sistólica reducida 75% ($A_{sist} = 0.25$)
- Muesca dicrótica casi invisible ($d = 0.1$)
- Taquicardia compensatoria (HR = 115 BPM)
- PI < 0.5%

### 13.3 Perfusión Fuerte (STRONG_PERFUSION)

Simula vasodilatación por fiebre, sepsis temprana, o ejercicio.

**Cambios morfológicos:**
- Amplitud sistólica aumentada 60% ($A_{sist} = 1.6$)
- Ratio diastólico aumentado (reflexión fuerte)
- Muesca más prominente ($d = 0.35$)
- PI = 10-20%

### 13.4 Vasoconstricción Marcada (VASOCONSTRICTION)

Simula vasoconstricción severa por frío extremo, shock temprano, o vasopresores.

> **Referencias:**
> 
> Allen J, Murray A.  
> *"Age-related changes in the characteristics of the photoplethysmographic pulse shape at various body sites."*  
> Physiological Measurement. 2003;24:297-307.
>
> Reisner A, et al.  
> *"Utility of the photoplethysmogram in circulatory monitoring."*  
> Anesthesiology. 2008;108(5):950-958.

**Cambios morfológicos:**
- Amplitud reducida 70% ($A_{sist} = 0.30$) — onda muy débil
- Componente diastólico casi ausente ($A_{diast} = 0.08$)
- Muesca dicrótica casi eliminada ($d = 0.05$) — atenuada, no prominente
- Pico sistólico más estrecho/afilado ($\sigma_{sist} = 0.04$)
- Diástole acortada ($\sigma_{diast} = 0.06$)
- PI muy bajo: 0.2-0.8% (típico 0.4%)
- SpO2 variable: 91-100% (inestable por señal débil)
- Frecuencia cardíaca normal (78 BPM)

### 13.5 Arritmia (ARRHYTHMIA)

Simula fibrilación auricular u otra arritmia irregular.

**Cambios morfológicos:**
- Morfología normal pero variabilidad RR extrema (CV = 15%)
- 15% de latidos prematuros ($RR_{prematuro} = 0.7 \times RR_{normal}$)
- Amplitud variable latido a latido

### 13.6 Hipoxemia con Perfusión Conservada (LOW_SPO2)

Simula SpO2 bajo (70-90%) por causa pulmonar (hipoventilación, shunt), no periférica.
La señal PPG mantiene buena forma porque la perfusión está conservada.

> **Referencias:**
> 
> Jubran A.  
> *"Pulse oximetry."*  
> Crit Care. 2015;19:272.
>
> Awad AA, et al.  
> *"The relationship between the photoplethysmographic waveform and systemic vascular resistance."*  
> J Clin Monit Comput. 2007;21:365-372.

**Cambios morfológicos:**
- Amplitud reducida 20% ($A_{sist} = 0.8$) — leve, perfusión conservada
- Muesca visible ($d = 0.2$)
- Taquicardia refleja (HR = 110 BPM)
- PI: 0.5-3.5% (ligeramente reducido)
- SpO2: 70-90% (hipoxemia, clampeado al rango)

**Nota clínica:** Es posible tener buena perfusión periférica y SpO2 bajo
por causas centrales (pulmonares), no técnicas.

### 13.7 Artefacto de Movimiento (MOTION_ARTIFACT)

Simula movimiento del paciente durante la medición.

> **Referencia:**
> 
> Shelley KH.  
> *"Photoplethysmography: Beyond the calculation of arterial oxygen saturation and heart rate."*  
> Anesthesia & Analgesia. 2007;105(6):S31-S36.

**Cambios:**
- Morfología subyacente normal
- Ruido gaussiano superpuesto (40% de amplitud)
- Spikes aleatorios (P = 0.2%)

---

## 14. Sistema de Escalado PPG

### 14.1 Componentes DC y AC

A diferencia de ECG y EMG que miden voltajes directamente, PPG mide **absorción de luz** que se normaliza a un rango 0-1:

| Componente | Valor | Descripción |
|------------|-------|-------------|
| DC | 0.50 | Nivel base (absorción tisular constante) |
| AC máximo | ±0.15 | Componente pulsátil (30% del DC) |

### 14.2 Rango Total

$$\boxed{\text{Rango PPG} = [0.0, 1.0]}$$

Donde:
- 0.0 = Mínima absorción de luz
- 1.0 = Máxima absorción de luz
- ~0.5 = Nivel DC típico

### 14.3 Conversión a DAC

Mapeo lineal directo a DAC de 8 bits:

$$DAC = PPG_{normalizado} \times 255$$

| Valor PPG | Valor DAC | Voltaje Salida |
|-----------|-----------|----------------|
| 0.0 | 0 | 0.00 V |
| 0.5 (DC) | 128 | 1.65 V |
| 1.0 | 255 | 3.30 V |

### 14.4 Baseline Wander Respiratorio

Se añade modulación sinusoidal lenta para simular efecto respiratorio:

$$PPG_{final}(t) = PPG(t) + 0.02 \cdot \sin(2\pi f_{resp} \cdot t)$$

Donde $f_{resp} \approx 0.25$ Hz (15 respiraciones/minuto).

---

## 15. Referencias Científicas

### Referencias Primarias ECG

1. **McSharry PE, Clifford GD, Tarassenko L, Smith L.**  
   *"A dynamical model for generating synthetic electrocardiogram signals."*  
   IEEE Trans Biomed Eng. 2003;50(3):289-294.  
   DOI: 10.1109/TBME.2003.808805

2. **PhysioNet ECGSYN.**  
   Implementación MATLAB original del modelo McSharry.  
   https://physionet.org/content/ecgsyn/1.0.0/

3. **Bazett HC.**  
   *"An analysis of the time-relations of electrocardiograms."*  
   Heart. 1920;7:353-370.

4. **Task Force of ESC and NASPE.**  
   *"Heart Rate Variability: Standards of Measurement, Physiological Interpretation, and Clinical Use."*  
   Circulation. 1996;93(5):1043-1065.  
   DOI: 10.1161/01.CIR.93.5.1043

### Referencias Primarias EMG

11. **Fuglevand AJ, Winter DA, Patla AE.**  
    *"Models of recruitment and rate coding organization in motor-unit pools."*  
    J Neurophysiol. 1993;70(6):2470-2488.  
    DOI: 10.1152/jn.1993.70.6.2470

12. **Henneman E, Somjen G, Carpenter DO.**  
    *"Functional significance of cell size in spinal motoneurons."*  
    J Neurophysiol. 1965;28:560-580.  
    DOI: 10.1152/jn.1965.28.3.560

13. **De Luca CJ, Hostage EC.**  
    *"Relationship between firing rate and recruitment threshold of motoneurons."*  
    J Neurophysiol. 2010;104(2):1034-1046.  
    DOI: 10.1152/jn.00258.2010

14. **De Luca CJ.**  
    *"The use of surface electromyography in biomechanics."*  
    J Appl Biomech. 1997;13:135-163.

15. **Enoka RM, Christou EA, Hunter SK, et al.**  
    *"Mechanisms that contribute to differences in motor performance."*  
    J Electromyogr Kinesiol. 2003;13(1):1-12.  
    DOI: 10.1016/S1050-6411(02)00094-X

### Referencias Primarias PPG

16. **Allen J.**  
    *"Photoplethysmography and its application in clinical physiological measurement."*  
    Physiological Measurement. 2007;28:R1-R39.  
    DOI: 10.1088/0967-3334/28/3/R01

17. **Elgendi M.**  
    *"On the Analysis of Fingertip Photoplethysmogram Signals."*  
    Current Cardiology Reviews. 2012;8(1):14-25.  
    DOI: 10.2174/157340312801215782

18. **Millasseau SC, Kelly RP, Ritter JM, Chowienczyk PJ.**  
    *"Determination of age-related increases in large artery stiffness."*  
    Clinical Science. 2002;103:371-377.  
    DOI: 10.1042/cs1030371

19. **Reisner A, Shaltis PA, McCombie D, Asada HH.**  
    *"Utility of the photoplethysmogram in circulatory monitoring."*  
    Anesthesiology. 2008;108(5):950-958.  
    DOI: 10.1097/ALN.0b013e31816c89e1

20. **Lima AP, Bakker J.**  
    *"Noninvasive monitoring of peripheral perfusion."*  
    Intensive Care Medicine. 2005;31:1316-1326.  
    DOI: 10.1007/s00134-005-2790-2

21. **Shelley KH.**  
    *"Photoplethysmography: Beyond the calculation of SpO2 and heart rate."*  
    Anesthesia & Analgesia. 2007;105(6):S31-S36.  
    DOI: 10.1213/01.ane.0000269512.82836.c9

22. **Allen J, Murray A.**  
    *"Age-related changes in PPG pulse shape at various body sites."*  
    Physiological Measurement. 2003;24:297-307.  
    DOI: 10.1088/0967-3334/24/2/306

23. **Awad AA, Haddadin AS, Tantawy H, et al.**  
    *"The relationship between PPG waveform and systemic vascular resistance."*  
    J Clin Monit Comput. 2007;21:365-372.  
    DOI: 10.1007/s10877-007-9097-5

### Guías Clínicas ECG

5. **Al-Khatib SM, et al.**  
   *"2017 AHA/ACC/HRS Guideline for Management of Patients With Ventricular Arrhythmias."*  
   Circulation. 2018;138:e272-e391.

6. **January CT, et al.**  
   *"2014 AHA/ACC/HRS Guideline for the Management of Patients With Atrial Fibrillation."*  
   J Am Coll Cardiol. 2014;64(21):e1-e76.

7. **Link MS, et al.**  
   *"2015 ACC/AHA/HRS Guideline for the Management of Adult Patients With Supraventricular Tachycardia."*  
   Circulation. 2016;133:e506-e574.

### Referencias de Soporte

8. **Pallas-Areny R, Webster JG.**  
   *"Sensors and Signal Conditioning."*  
   Wiley, 2001.  
   (Modulación respiratoria del vector cardíaco)

9. **Weaver WD, et al.**  
   *"Amplitude of ventricular fibrillation waveform and outcome after cardiac arrest."*  
   Ann Intern Med. 1985;102(1):53-55.

10. **Goldberger AL, et al.**  
    *"PhysioBank, PhysioToolkit, and PhysioNet: Components of a new research resource for complex physiologic signals."*  
    Circulation. 2000;101(23):e215-e220.

---

