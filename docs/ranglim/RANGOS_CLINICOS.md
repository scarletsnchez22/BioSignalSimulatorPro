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

### 📺 Señales por Salida (Arquitectura del Simulador)

| Salida | Señal Digital | Voltaje Físico | Frecuencia | Propósito |
|--------|---------------|----------------|------------|-----------|
| **Nextion Waveform** | Envolvente RMS procesada (0-3.5 mV) | — (UART serial) | 1 kHz | Didáctica visual principal |
| **DAC GPIO25** | Señal cruda mapeada (0-255) | 0-3.3V | 1 kHz | Osciloscopio, trigger, debug |
| **Serial Plotter** | Ambas señales + métricas | — (USB serial) | 500 Hz | Validación, captura para tesis |

-
#### **✅ Utilidades Reales del DAC (sin exagerar)**

##### **1. Osciloscopio básico - Visualización de morfología (cualitativa)**
```
DAC GPIO25 → Cable BNC → Osciloscopio
```
**SÍ sirve para:**
- Observar morfología PQRST en ECG (forma general del latido)
- Ver envolvente EMG (patrón de contracción)
- Medir intervalos temporales (RR, PR, QT) con cursores del osciloscopio
- Demostración didáctica de formas de onda

**NO sirve para:**
- Mediciones de amplitud precisas (ruido ±10 mV + resolución 12.9 mV/paso invalidan mediciones clínicas en el rango de mV)
- Simulación clínica certificada (no cumple estándares IEC 60601)

---

##### **2. Trigger de sincronización - Detección de eventos**
```
DAC GPIO25 → Pin de trigger → Sistema externo
```
**SÍ sirve para:**
- Detectar pico R en ECG (umbral simple)
- Sincronizar cámara de video con latidos cardíacos
- Trigger básico para adquisición multimodal (EMG + acelerómetro)

**Ejemplo de implementación:**
```cpp
if (ecgDACValue > 200) {  // Umbral para detectar pico R
    digitalWrite(TRIGGER_PIN, HIGH);
    delayMicroseconds(100);
    digitalWrite(TRIGGER_PIN, LOW);
}
```

---

##### **3. Prueba de algoritmos de procesamiento (estudiantes)**
```
DAC GPIO25 → ADC de Arduino/ESP32 → Algoritmo de detección
```
**SÍ sirve para:**
- Validar algoritmos de detección de QRS (ej. Pan-Tompkins)
- Probar filtros digitales con señal conocida
- Proyectos educativos de procesamiento de señales

**NO sirve para:**
- Entrenamiento de modelos ML con datos clínicos reales (la resolución y ruido no son representativos)

---

##### **4. ❌ NO sirve para control de prótesis mioeléctricas profesionales**

**Alternativa realista:**
- El DAC puede generar señal de prueba para validar **lógica de control** (ej. umbral de activación ON/OFF)
- **NO** para simular señal EMG clínica real que alimentaría una prótesis comercial

---

##### **5. ❌ NO sirve directamente para tarjetas de adquisición profesionales**

**Razones técnicas:**
- ADCs clínicos esperan señales en rango ±5 mV o ±10 mV (NO 0-3.3V)
- Requieren impedancia de fuente alta (1-10 kΩ vs 100 Ω del DAC)
- Esperan ruido <1 mV (vs ±10-20 mV del DAC)

**Posible solución (fuera del alcance del proyecto):**
- Circuito acondicionador externo:
  - Divisor resistivo: 3.3V → ±5 mV
  - Filtro RC pasa-bajas (reducir ruido)
  - Buffer de alta impedancia (op-amp)
- Esto requiere diseño PCB adicional (NO parte del simulador base)

---

#### **🎯 Resumen: ¿Para qué SÍ sirve el DAC?**

| Aplicación | ¿Funciona? | Limitación |
|------------|------------|------------|
| Osciloscopio (morfología) | ✅ SÍ | Solo cualitativo, no amplitudes precisas |
| Trigger sincronización | ✅ SÍ | Detección de eventos simple |
| Debug algoritmos (estudiantes) | ✅ SÍ | Señal de prueba conocida |
| Prótesis profesionales | ❌ NO | Requiere resolución/aislamiento |
| ADC clínicos directos | ❌ NO | Rango de voltaje incompatible |
| Mediciones clínicas certificadas | ❌ NO | No cumple IEC 60601 |

**Conclusión técnica:**
El DAC GPIO25 es una **salida auxiliar para debug y demostración**, NO un generador biomédico certificado. Su utilidad principal es permitir verificación visual de morfología en osciloscopio y trigger básico para sincronización.

> **Refs:** ESP32 Technical Reference Manual v5.4 (DAC specifications) | IEC 60601-2-27 (ECG equipment safety)

---

## PPG - Fotopletismografía

### Rangos por Condición

| # | Condición | Rango (norm) | PI típico (%) |
|---|-----------|--------------|---------------|
| 0 | Normal | 0.92–1.00 | 2–5 |
| 1 | Arritmia | 0.90–1.00 | 1–5 |
| 2 | Perfusión Débil | 0.995–1.00 | 0.1–0.5 |
| 3 | Perfusión Fuerte | 0.80–1.00 | 5–20 |
| 4 | Vasoconstricción | 0.992–1.00 | 0.2–0.8 |
| 5 | SpO2 Bajo | 0.93–1.00 | 0.5–3.5 |

> **Ref:** Allen J. Physiol Meas. 2007;28(3):R1-R39.

---

### Índice de Perfusión (PI)

| Condición | PI (%) |
|-----------|--------|
| Normal | 2–5 |
| Perfusión débil | < 0.5 |
| Perfusión fuerte | > 5 |
| Vasoconstricción | 0.2–0.8 |

> **Ref:** Lima AP, et al. Intensive Care Med. 2002;28(4):445-449.

---

### Saturación de Oxígeno (SpO2)

| Condición | SpO2 (%) |
|-----------|----------|
| Normal | 95–100 |
| Hipoxemia leve | 90–94 |
| Hipoxemia moderada | 85–89 |
| Hipoxemia severa | < 85 |

> **Ref:** WHO Pulse Oximetry Training Manual. 2011.

---

### Frecuencia Cardíaca (HR)

| Condición | HR (BPM) |
|-----------|----------|
| Normal | 60–100 |
| Bradicardia | < 60 |
| Taquicardia | > 100 |

> **Ref:** AHA/ACC Guidelines. Circulation. 2017.

---

## Referencias Generales

| Señal | Referencias Principales |
|-------|------------------------|
| ECG | Goldberger AL 2017, Surawicz 2008, Task Force ESC/NASPE 1996 |
| EMG | Fuglevand 1993, De Luca 1997/2010, Kimura 2013, Henneman 1965 |
| PPG | Allen J 2007, Lima 2002, WHO 2011 |

---

*BioSimulator Pro v1.1.0*
