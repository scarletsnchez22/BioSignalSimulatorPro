# Rangos Clínicos - BioSimulator Pro

> Tablas de referencia para validación de señales biológicas simuladas.  
> Para información técnica del modelo ver: `docs/models_info/`

---

## ECG - Electrocardiograma (Lead II)

### Complejo PQRST Normal

| Componente | Duración | Amplitud (mV) |
|------------|----------|---------------|
| Onda P | < 110 ms | 0.15 – 0.25 |
| Intervalo PR | 120 – 200 ms | — |
| Complejo QRS | 70 – 100 ms | — |
| Onda Q | < 40 ms | < 25% de R |
| Onda R | — | 0.8 – 1.2 |
| Onda S | — | −0.05 a −0.50 |
| Segmento ST | — | ≈ 0 (isoeléctrico) |
| Onda T | — | 0.20 – 0.40 |
| Intervalo QT | 320 – 440 ms | — |
| QTc (Bazett) | 320 – 460 ms | — |
| HR | 60 – 100 BPM | — |

> **Refs:** Goldberger AL. Clinical Electrocardiography. 9th ed. Elsevier, 2017. | Surawicz B. Chou's Electrocardiography. 6th ed. 2008.

---

### Rangos QTc

| Clasificación | QTc (ms) | Riesgo |
|---------------|----------|--------|
| QTc corto | < 320 | Arritmias ventriculares |
| Normal | 320 – 460 | Sin riesgo |
| QTc prolongado | > 460 | Torsades de Pointes |

> **Ref:** Bazett HC. Heart. 1920;7:353-370.

---

### Condiciones ECG (8 Patologías)

| # | Condición | HR (BPM) | RR (ms) | PR (ms) | QRS (ms) | ST/T |
|---|-----------|----------|---------|---------|----------|------|
| 0 | Ritmo Normal | 60–100 | 600–1000 | 120–200 | 80–120 | ST=0, T 0.2–0.6 mV |
| 1 | Taquicardia Sinusal | >100 | <600 | 120–200 | 80–120 | Normales |
| 2 | Bradicardia Sinusal | <60 | >1000 | 170–200 | 80–120 | Normales |
| 3 | Fibrilación Auricular | Variable | Irregular | — | 80–120 | Secundarios |
| 4 | Fibrilación Ventricular | — | — | — | — | Caótico 4–10 Hz |
| 5 | Bloqueo AV 1º | 60–100 | 600–1000 | >200 | 80–120 | Normales |
| 6 | Elevación ST (STEMI) | Variable | Variable | 120–200 | 80–120 | ST↑ ≥0.2 mV |
| 7 | Depresión ST | Variable | Variable | 120–200 | 80–120 | ST↓ 0.05–0.2 mV |

> **Refs:** Task Force ESC/NASPE 1996 (HRV) | AHA/ACC/HRS 2014 (AFib) | ACC/AHA 2004 (STEMI) | AHA/ACC/HRS 2018 (Bradycardia)

---

### Variabilidad RR

| Condición | CV% RR | Interpretación |
|-----------|--------|----------------|
| Normal/Sinus | < 10% | Regular |
| Fibrilación Auricular | 15–35% | Irregularmente irregular |
| Fibrilación Ventricular | N/A | Caótico |

> **Ref:** Task Force ESC/NASPE. Circulation. 1996;93(5):1043-1065.

---

## EMG - Electromiografía de Superficie (sEMG)

### 📊 Tabla de condiciones validadas con Fuglevand 1993

**IMPORTANTE:** Las condiciones LOW, MODERATE y HIGH usan **secuencias dinámicas** que alternan entre REST y CONTRACCIÓN para mostrar claramente el envelope. Los valores RMS son los **picos durante la fase de contracción**.

| Condición | Excitación (MVC) | MUs Activas | FR Media (Hz) | RMS Pico (mV) | Secuencia | Estado | Fuente |
|----------|------------------|-------------|---------------|---------------|-----------|---------|--------|
| **Reposo** | 0.5% | 0 | 0 | 0.001 | Estático | Solo ruido térmico | Fuglevand 1993 ✅ |
| **Leve** | 12% | 68-70 | 8-10 | 0.52 | REST 1s → LOW 3s (ciclo 4s) | Durante contracción | Fuglevand 1993 ✅ |
| **Moderada** | 35% | 100 | 15-17 | 1.7 | REST 1s → MOD 3s (ciclo 4s) | Durante contracción | Fuglevand 1993 ✅ |
| **Alta** | 80% | 100 | 31-37 | 2.8 | REST 1s → HIGH 3s (ciclo 4s) | Durante contracción | Fuglevand 1993 ✅ |
| **Temblor Parkinson** | Variable | Variable | 4-6 Hz modulación | 0.1-0.5 | Estático con modulación interna | Temblor continuo | Gulati & Pandey 2024 |
| **Fatiga** | 50% sostenido | 100 | Decay progresivo | 1.5 → 0.4 | Estático con decay | MDF 120→80 Hz | Cifrek 2009, Sun 2022 |

#### ✅ Valores Validados por Hardware

Todos los valores de REST, LOW, MODERATE y HIGH fueron **validados experimentalmente** en ESP32 comparando con los parámetros del modelo Fuglevand 1993:

- **Pool de MUs:** 100 unidades motoras
- **Distribución de umbrales:** Exponencial con RTE=0.35 y RR=30
- **Primera MU:** Umbral ~1.2% MVC
- **Última MU:** Umbral 35% MVC
- **Reclutamiento completo:** ≥35% MVC (todas las 100 MUs activas)
- **Rate coding:** >35% MVC (aumento de FR, no de MUs)

#### 🔄 Secuencias Dinámicas (Visualización de Envelope)

Las secuencias dinámicas permiten observar la **modulación del envelope RMS** en tiempo real. Ciclos de 4 segundos optimizados para Nextion (50 Hz → 200 muestras/ciclo), permitiendo visualizar 3-4 bursts en ventana de 15s:

**LOW_CONTRACTION:**
- Ciclo: 4 segundos (REST 1s + LOW 3s)
- RMS: 0.001 mV → 0.52 mV → 0.001 mV
- Muestra reclutamiento parcial (70 MUs)

**MODERATE_CONTRACTION:**
- Ciclo: 4 segundos (REST 1s + MODERATE 3s)
- RMS: 0.001 mV → 1.7 mV → 0.001 mV
- Muestra reclutamiento completo (100 MUs, FR moderado)

**HIGH_CONTRACTION:**
- Ciclo: 4 segundos (REST 1s + HIGH 3s)
- RMS: 0.001 mV → 2.8 mV → 0.001 mV
- Muestra rate coding dominante (100 MUs, FR alto 31-37 Hz)

> **Refs:** De Luca CJ. J Appl Biomech. 1997;13(2):135-163. | De Luca CJ, Hostage EC. J Neurophysiol. 2010;104(2):1034-1046. | Gulati D, Pandey S. Neurol India. 2024;72(1):41-48. | Cifrek M et al. Coll Antropol. 2009;33(2):439-449. | Sun Y et al. Comput Intell Neurosci. 2022;2022:4950936. | Wang L et al. Biomed Signal Process Control. 2021;68:102694.

---

### Frecuencias de Disparo

| Parámetro | Valor | Unidad |
|-----------|-------|--------|
| FR mínima (reclutamiento) | 6–8 | Hz |
| FR máxima (MVC) | 30–50 | Hz |
| Ganancia FR | ~40 | Hz/unidad |
| CV ISI | 15–25 | % |

> **Ref:** De Luca CJ, Hostage EC. J Neurophysiol. 2010;104(2):1034-1046.

---


### Tipos de Unidades Motoras

| Tipo | Umbral | Amplitud | Fatigabilidad |
|------|--------|----------|---------------|
| I (S) | Bajo (0–20%) | Pequeña | Resistente |
| IIa (FR) | Medio (20–50%) | Media | Moderada |
| IIb (FF) | Alto (50–100%) | Grande | Fatigable |

> **Ref:** Henneman E, et al. J Neurophysiol. 1965;28:560-580.

---

## PPG - Fotopletismografía

### Tabla de Condiciones Clínicas PPG

| # | Condición | PI (%) | Morfología / Notas | Muesca Dicrótica |
|---|-----------|--------|-------------------|------------------|
| 0 | **Normal** | 2.9–6.1 | Pico sistólico claro; upstroke rápido; muesca sutil; d/s 0.1–0.4 | Posición: 20–50%; Amplitud: ≥20%; Anchura: 20–60 ms |
| 1 | **Arritmia** | 1.0–5.0 | Latidos irregulares; amplitud variable; plantilla promedio dispersa | Posición: variable; Amplitud: 10–30%; Anchura: 20–70 ms |
| 2 | **Weak Perfusion** | 0.5–2.1 | AC muy reducido; pico atenuado; muesca ausente o tenue | Posición: <20% o ausente; Amplitud: <10%; no detectable |
| 3 | **Vasodilatación** | 5.0–10.0 | Pico más alto y ancho; muesca más marcada; mejor relleno diastólico | Posición: 25–55%; Amplitud: 20–40%; Anchura: 30–60 ms |
| 4 | **Strong Perfusion** | 7.0–20.0 | Señal robusta; muesca y reflejo vascular prominentes; alta AC | Posición: 30–60%; Amplitud: ≥30%; Anchura: 30–80 ms |
| 5 | **Vasoconstricción** | 0.7–0.8 | Pulso pequeño y aplanado; upstroke menos pronunciado; muesca tenue | Posición: <20% o ausente; Amplitud: <10%; no medible |

### Clasificación de Muesca Dicrótica (Aguilar et al. 2022)

| Clase | Profundidad | Interpretación |
|-------|-------------|----------------|
| I | < 20% | Vasodilatación / Tono bajo |
| II | 20–35% | Normal bajo |
| **III** | **20–50%** | **Tono vascular normal** |
| IV | > 50% | Vasoconstricción / Rigidez arterial |

### Modelo de Duración Sístole/Diástole (Fisiología Cardiovascular)

La literatura fisiológica describe que la **duración de la sístole varía poco** con la frecuencia cardíaca, mientras que la **diástole absorbe el cambio**. El modelo implementa:

- **Sístole ~constante**: ~300ms base (rango 250-350ms)
- **Diástole variable**: RR - sístole (se comprime a HR alto)

| HR (BPM) | RR (ms) | Sístole (ms) | Diástole (ms) | Fracción Sístole |
|----------|---------|--------------|---------------|------------------|
| 60 | 1000 | ~320 | ~680 | 32% |
| 75 | 800 | ~300 | ~500 | 37% |
| 90 | 667 | ~285 | ~382 | 43% |
| 120 | 500 | ~270 | ~230 | 54% |

> El acortamiento del ciclo cardíaco a frecuencias elevadas se produce predominantemente a expensas de la diástole.


**Flujo del modelo:**
```
Patología → HR,PI (aleatorios dentro del rango) → RR = 60/HR
→ systole_time = f(HR), diastole_time = RR - systole
→ pulseShape normalizado [0,1] (base Allen: systolic=1.0, diastolic=0.4)
→ AC = PI × 15 mV/% → signal = DC + pulse × AC
```

**Variabilidad (sigma = mean × CV):**
| Condición | HR CV | PI CV | Notas |
|-----------|-------|-------|-------|
| Normal | 2% | 10% | Variabilidad fisiológica |
| Arritmia | 15% | 20% | Alta variabilidad RR |
| Otras | 2% | 10-15% | Según condición |

**Forma de onda (Allen 2007):**
- `systolicAmplitude = 1.0` (base, siempre)
- `diastolicAmplitude = 0.4` (ratio d/s, siempre)
- `dicroticDepth` = según tabla clínica (0.05-0.35)
- **PI controla la amplitud AC** (único escalado de amplitud)

---

### Referencias PPG

1. **Sun, X., He, H., Xu, M., & Long, Y.** (2024). *Peripheral perfusion index of pulse oximetry in adult patients: a narrative review.* European Journal of Medical Research, 29, 457. https://link.springer.com/article/10.1186/s40001-024-02048-3

2. **De la Peña Sanabria, I., Ochoa Martelo, M., Baquero Latorre, H., & Acosta‑Reyes, J.** (2017). *Peripheral perfusion index in the neonatal ICU: A response to non‑invasive monitoring of the critical newborn.* doi:10.1016/j.rprh.2017.10.015

3. **University of California San Diego.** (2017). *Cardiac Cycle* (teaching notes / PDF). https://cvil.ucsd.edu/wp-content/uploads/2017/02/cardiac-cycle.pdf

4. **Aguilar, F. G., Monares Z., E., et al.** (2022). *Algoritmo de Emergencias Médicas de Chiapas para pacientes en estado de choque.* Medicina Crítica (Colegio Mexicano de Medicina Crítica). — Clasificación de muesca dicrótica Clase III = 20–50% como tono vascular normal.

5. **Allen J.** (2007). *Photoplethysmography and its application in clinical physiological measurement.* Physiological Measurement, 28(3):R1-R39.

---

## Referencias Generales

| Señal | Referencias Principales |
|-------|------------------------|
| ECG | Goldberger AL 2017, Surawicz 2008, Task Force ESC/NASPE 1996 |
| EMG | Fuglevand 1993, De Luca 1997/2010, Kimura 2013, Henneman 1965 |
| PPG | Sun 2024, De la Peña 2017, UCSD 2017, Aguilar 2022, Allen 2007 |

---

*BioSimulator Pro v2.0.0*
