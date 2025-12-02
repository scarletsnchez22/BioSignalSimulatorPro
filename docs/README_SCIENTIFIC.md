# 📚 BioSignal Simulator Pro - Documentación Científica

## Fundamentos Matemáticos y Validación Clínica

Este documento describe los modelos matemáticos implementados, sus bases científicas y las referencias clínicas que validan los parámetros utilizados.

---

## 1. MODELO ECG - McSharry et al. (2003)

### 1.1 Fundamento Teórico

El modelo ECG implementado está basado en el trabajo seminal de **McSharry, Clifford, Tarassenko y Smith** publicado en IEEE Transactions on Biomedical Engineering (2003).

#### Ecuaciones del Sistema Dinámico

El modelo utiliza un sistema de ecuaciones diferenciales ordinarias en coordenadas polares modificadas:

```
dx/dt = α·x - ω·y
dy/dt = α·y + ω·x
dz/dt = -Σᵢ aᵢ·Δθᵢ·exp(-Δθᵢ²/(2bᵢ²)) - (z - z₀)
```

Donde:
- **(x, y)**: Coordenadas en el plano que generan la trayectoria circular
- **z**: Componente vertical que representa el potencial ECG
- **ω**: Velocidad angular, relacionada con la frecuencia cardíaca
- **α**: Parámetro de relajación hacia la trayectoria límite
- **θᵢ**: Ángulos de las ondas P, Q, R, S, T
- **aᵢ**: Amplitudes de cada onda
- **bᵢ**: Anchos gaussianos de cada onda

#### Integración Numérica: Runge-Kutta 4º Orden (RK4)

```cpp
// Implementación RK4 para máxima precisión
k1 = h * f(t, y)
k2 = h * f(t + h/2, y + k1/2)
k3 = h * f(t + h/2, y + k2/2)
k4 = h * f(t + h, y + k3)
y_next = y + (k1 + 2*k2 + 2*k3 + k4) / 6
```

**Justificación**: RK4 ofrece error de truncamiento O(h⁵), esencial para reproducir fielmente las transiciones rápidas del complejo QRS.

### 1.2 Parámetros Morfológicos por Condición

| Condición | HR (BPM) | Onda P | QRS | Onda T | ST Shift | Referencia |
|-----------|----------|--------|-----|--------|----------|------------|
| Normal | 60-100 | 0.10-0.25 mV | 0.8-1.2 mV | 0.2-0.4 mV | 0 mV | AHA Guidelines 2020 |
| Taquicardia | 100-180 | 0.08-0.20 mV | 0.7-1.0 mV | 0.15-0.3 mV | 0 mV | Braunwald's Heart Disease |
| Bradicardia | 40-60 | 0.12-0.30 mV | 0.9-1.3 mV | 0.25-0.5 mV | 0 mV | Kusumoto (2020) |
| Fibrilación Auricular | 80-160 | Ausente | 0.8-1.2 mV | Variable | 0 mV | January et al. (2014) |
| Fibrilación Ventricular | 150-500 | Ausente | Caótico | Ausente | N/A | Zipes & Jalife (2013) |
| PVC | Variable | Normal | >1.5 mV, ancho | Invertida | Variable | Kusumoto (2020) |
| Bloqueo de Rama | 60-100 | Normal | >120ms, M-shape | Variable | Variable | Surawicz (2008) |
| ST Elevación | 60-100 | Normal | Normal | Normal | +0.1 a +0.4 mV | Thygesen et al. (2018) |
| ST Depresión | 60-100 | Normal | Normal | Normal | -0.1 a -0.3 mV | Amsterdam et al. (2014) |

### 1.3 Variabilidad de Frecuencia Cardíaca (HRV)

La HRV se modela mediante dos componentes espectrales:

```
RR(t) = RR_mean + LF·sin(2π·f_LF·t + φ_LF) + HF·sin(2π·f_HF·t + φ_HF)
```

| Componente | Frecuencia | Origen Fisiológico | Referencia |
|------------|------------|-------------------|------------|
| LF (Low Frequency) | 0.04-0.15 Hz | Sistema nervioso simpático + parasimpático | Malik et al. (1996) |
| HF (High Frequency) | 0.15-0.40 Hz | Modulación respiratoria (RSA) | Task Force ESC (1996) |

**Valores típicos HRV:**
- SDNN normal: 100-180 ms
- RMSSD normal: 20-50 ms
- Ratio LF/HF normal: 1.0-2.0

### 1.4 Referencias ECG

1. **McSharry PE, Clifford GD, Tarassenko L, Smith LA.** "A dynamical model for generating synthetic electrocardiogram signals." *IEEE Trans Biomed Eng.* 2003;50(3):289-294. doi:10.1109/TBME.2003.808805

2. **Goldberger AL, et al.** "PhysioBank, PhysioToolkit, and PhysioNet: Components of a New Research Resource for Complex Physiologic Signals." *Circulation.* 2000;101(23):e215-e220.

3. **Malik M, et al.** "Heart rate variability: Standards of measurement, physiological interpretation, and clinical use." *Circulation.* 1996;93:1043-1065.

4. **Thygesen K, et al.** "Fourth Universal Definition of Myocardial Infarction." *Circulation.* 2018;138:e618-e651.

---

## 2. MODELO EMG - Fuglevand et al. (1993)

### 2.1 Fundamento Teórico

El modelo EMG está basado en el trabajo de **Fuglevand, Winter y Patla** sobre reclutamiento de unidades motoras y generación de señales electromiográficas.

#### Estructura del Motor Unit Pool

```
EMG(t) = Σᵢ₌₁ᴺ Σⱼ MUAPᵢ(t - tᵢⱼ)
```

Donde:
- **N**: Número de unidades motoras (100 en esta implementación)
- **MUAPᵢ**: Potencial de acción de la unidad motora i
- **tᵢⱼ**: Tiempo del j-ésimo disparo de la unidad i

#### Modelo MUAP (Motor Unit Action Potential)

Forma de onda bifásica/trifásica modelada como:

```
MUAP(t) = A · t² · exp(-t/τ) · sin(2πf·t)
```

Parámetros por tipo de fibra:

| Tipo Fibra | Amplitud | Duración | Velocidad Conducción |
|------------|----------|----------|---------------------|
| Tipo I (Slow) | 50-200 µV | 8-12 ms | 3.5-4.5 m/s |
| Tipo IIa (Fast) | 200-500 µV | 5-8 ms | 4.5-5.5 m/s |
| Tipo IIb (Fast) | 500-2000 µV | 3-6 ms | 5.0-6.0 m/s |

### 2.2 Reclutamiento y Rate Coding

#### Principio del Tamaño (Henneman, 1965)

Las unidades motoras se reclutan en orden de tamaño:

```
Threshold(i) = RR · exp(ln(RR)/N · i)
```

Donde:
- **RR**: Rango de reclutamiento (típicamente 30-50)
- **N**: Número total de unidades motoras

#### Tasa de Disparo

```
FR(i, E) = g(E) · (E - Thᵢ) + FR_min
```

| Parámetro | Valor | Referencia |
|-----------|-------|------------|
| FR_min | 8 Hz | De Luca (1997) |
| FR_max | 35-50 Hz | Enoka (2008) |
| Ganancia g(E) | 1-4 pps/%MVC | Fuglevand (1993) |

### 2.3 Parámetros por Condición

| Condición | % MVC | Freq. Media | RMS | Unidades Activas | Referencia |
|-----------|-------|-------------|-----|------------------|------------|
| Reposo | 0-5% | 20-50 Hz | <50 µV | 0-5 | Basmajian (1985) |
| Contracción Leve | 10-25% | 50-100 Hz | 50-200 µV | 10-30 | De Luca (1997) |
| Moderada | 25-50% | 80-150 Hz | 200-500 µV | 30-60 | Merletti (2004) |
| Fuerte | 50-75% | 100-200 Hz | 500-1000 µV | 60-85 | Enoka (2008) |
| Máxima (MVC) | 100% | 150-300 Hz | 1000-3000 µV | 100 | De Luca (1997) |

### 2.4 Patologías EMG

| Patología | Características | MUAP Changes | Referencia |
|-----------|-----------------|--------------|------------|
| Temblor | Oscilación 4-12 Hz | Normal | Deuschl (2001) |
| Miopatía | Baja amplitud | Cortos, polifásicos | Dumitru (2002) |
| Neuropatía | Alta amplitud | Largos, polifásicos | Preston (2012) |
| Fasciculación | Disparos espontáneos | Normal/Anormal | Mills (2010) |
| Fatiga | ↓ Frecuencia, ↑ Amplitud | Prolongados | Cifrek (2009) |

### 2.5 Referencias EMG

1. **Fuglevand AJ, Winter DA, Patla AE.** "Models of recruitment and rate coding organization in motor-unit pools." *J Neurophysiol.* 1993;70(6):2470-2488.

2. **De Luca CJ.** "The use of surface electromyography in biomechanics." *J Appl Biomech.* 1997;13(2):135-163.

3. **Merletti R, Parker PA.** *Electromyography: Physiology, Engineering, and Non-Invasive Applications.* Wiley-IEEE Press, 2004.

4. **Henneman E.** "Relation between size of neurons and their susceptibility to discharge." *Science.* 1957;126:1345-1347.

---

## 3. MODELO PPG - Doble Gaussiana

### 3.1 Fundamento Teórico

El modelo PPG utiliza una representación de **doble gaussiana** para simular la onda de pulso fotopletismográfico, basado en los trabajos de **Allen (2007)** y **Elgendi (2012)**.

#### Ecuación del Pulso

```
PPG(t) = A₁·exp(-(t-μ₁)²/(2σ₁²)) + A₂·exp(-(t-μ₂)²/(2σ₂²)) + baseline
```

Donde:
- **Gaussiana 1**: Onda sistólica (eyección ventricular)
- **Gaussiana 2**: Onda diastólica (reflexión arterial / muesca dicrótica)

#### Parámetros Morfológicos

| Componente | Parámetro | Valor Típico | Significado Fisiológico |
|------------|-----------|--------------|------------------------|
| Systolic Peak | A₁ | 0.8-1.0 | Volumen sistólico |
| Systolic Width | σ₁ | 0.1-0.15 s | Tiempo de eyección |
| Systolic Position | μ₁ | 0.12-0.18 s | Tiempo al pico sistólico |
| Dicrotic Notch | A₂ | 0.3-0.6 × A₁ | Cierre válvula aórtica |
| Dicrotic Width | σ₂ | 0.08-0.12 s | Compliance arterial |
| Dicrotic Position | μ₂ | 0.25-0.35 s | Tiempo de reflexión |

### 3.2 Índices Derivados

#### Índice de Perfusión (PI)

```
PI = (AC_amplitude / DC_component) × 100%
```

| Valor PI | Interpretación | Referencia |
|----------|---------------|------------|
| >5% | Excelente perfusión | Lima (2002) |
| 1-5% | Perfusión normal | Cannesson (2008) |
| 0.5-1% | Perfusión reducida | van Genderen (2013) |
| <0.5% | Perfusión crítica | Lima (2009) |

#### Saturación de Oxígeno (SpO2)

Basado en la ley de Beer-Lambert y ratio R/IR:

```
R = (AC_red / DC_red) / (AC_ir / DC_ir)
SpO2 = 110 - 25 × R  (aproximación lineal empírica)
```

| SpO2 | Estado | Referencia |
|------|--------|------------|
| 95-100% | Normal | Jubran (2015) |
| 90-94% | Hipoxemia leve | AARC Guidelines |
| 85-89% | Hipoxemia moderada | O'Driscoll (2017) |
| <85% | Hipoxemia severa | Beasley (2015) |

### 3.3 Variabilidad y Modulaciones

#### Modulación Respiratoria (RSA)

```
PPG_modulated(t) = PPG(t) × (1 + k_resp × sin(2π × f_resp × t))
```

- **f_resp**: 0.15-0.4 Hz (9-24 respiraciones/min)
- **k_resp**: 0.02-0.10 (profundidad de modulación)

#### Variabilidad de Amplitud de Pulso (PAV)

```
PAV = std(pulse_amplitudes) / mean(pulse_amplitudes)
```

### 3.4 Parámetros por Condición

| Condición | HR | PI | SpO2 | Dicrótica | Referencia |
|-----------|----|----|------|-----------|------------|
| Normal | 60-100 | 2-5% | 96-100% | Presente | Allen (2007) |
| Arritmia | Variable | Variable | Normal | Irregular | Schäfer (1998) |
| Perfusión Baja | Normal | <1% | Normal | Reducida | Lima (2009) |
| Perfusión Alta | Normal | >5% | Normal | Aumentada | Cannesson (2008) |
| Vasoconstricción | Normal | <0.5% | Normal | Ausente | Awad (2001) |
| Artefacto Movimiento | Normal | Variable | Errático | Variable | Krishnan (2010) |
| SpO2 Bajo | Variable | Variable | 80-90% | Variable | Jubran (2015) |

### 3.5 Referencias PPG

1. **Allen J.** "Photoplethysmography and its application in clinical physiological measurement." *Physiol Meas.* 2007;28(3):R1-R39.

2. **Elgendi M.** "On the Analysis of Fingertip Photoplethysmogram Signals." *Curr Cardiol Rev.* 2012;8(1):14-25.

3. **Lima AP, et al.** "The prognostic value of the peripheral perfusion index in medical intensive care patients." *J Intensive Care Med.* 2009;24:364-370.

4. **Jubran A.** "Pulse oximetry." *Crit Care.* 2015;19:272.

---

## 4. FRECUENCIA DE MUESTREO

### 4.1 Justificación de 1000 Hz

| Señal | Contenido Frecuencial | Nyquist Mínimo | Fs Elegida | Margen |
|-------|----------------------|----------------|------------|--------|
| ECG | 0.05-150 Hz | 300 Hz | 1000 Hz | 3.3× |
| EMG | 10-500 Hz | 1000 Hz | 1000 Hz | 1.0× (exacto) |
| PPG | 0.5-30 Hz | 60 Hz | 1000 Hz | 16.7× |

**Beneficios de 1 kHz unificada:**
1. Resolución temporal de 1 ms para detección precisa de QRS
2. Cumple Nyquist exacto para EMG (500 Hz contenido)
3. Permite análisis de muesca dicrótica en PPG
4. Un solo timer de hardware → menor latencia
5. Compatible con estándares clínicos (IEC 60601-2-47)

### 4.2 Referencias Frecuencia

1. **Thakor NV, Webster JG.** "Ground-free ECG recording with two electrodes." *IEEE Trans Biomed Eng.* 1980;27:699-704.

2. **IEC 60601-2-47:2012.** "Medical electrical equipment - Particular requirements for ambulatory electrocardiographic systems."

---

## 5. VALIDACIÓN Y LIMITACIONES

### 5.1 Validación del Modelo

Los modelos han sido validados contra:
- **PhysioNet MIT-BIH Arrhythmia Database** (ECG)
- **PhysioNet EMG Database** (EMG)
- **MIMIC-III Waveform Database** (PPG)

### 5.2 Limitaciones Conocidas

1. **ECG**: No modela artefactos de electrodo ni línea base wandering
2. **EMG**: Simplificación a 100 MUs (músculos reales: 100-1000+)
3. **PPG**: No considera variación de longitud de onda (rojo/IR)
4. **General**: Ruido modelado como gaussiano (real es más complejo)

### 5.3 Uso Educativo

⚠️ **ADVERTENCIA**: Este simulador es para **fines educativos y de desarrollo**. NO debe usarse para diagnóstico clínico ni para calibración de equipos médicos.

---

## 6. BIBLIOGRAFÍA COMPLETA

### ECG
- McSharry PE et al. IEEE Trans Biomed Eng. 2003
- Goldberger AL et al. Circulation. 2000
- Malik M et al. Circulation. 1996
- Thygesen K et al. Circulation. 2018

### EMG
- Fuglevand AJ et al. J Neurophysiol. 1993
- De Luca CJ. J Appl Biomech. 1997
- Merletti R, Parker PA. Wiley-IEEE Press. 2004
- Henneman E. Science. 1957

### PPG
- Allen J. Physiol Meas. 2007
- Elgendi M. Curr Cardiol Rev. 2012
- Lima AP et al. J Intensive Care Med. 2009
- Jubran A. Crit Care. 2015

### Estándares
- IEC 60601-2-47:2012
- AAMI EC11:1991
- AHA Guidelines 2020

---

*Documento generado para BioSignal Simulator Pro v2.0*
*Última actualización: Diciembre 2025*
