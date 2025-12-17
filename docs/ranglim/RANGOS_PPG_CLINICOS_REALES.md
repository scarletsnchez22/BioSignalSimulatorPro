# 📊 PPG - Rangos Clínicos Reales en mV

## **Valores Físicos Basados en Literatura Científica**

### **Referencias Bibliográficas**

1. **BPL Medical Technologies (2023)** - Understanding Perfusion Index in Pulse Oximeter
   - Valores típicos de PI: 0.02% (muy débil) – 20% (muy fuerte)
   - PI refleja fuerza pulsátil y cambios vasculares
   - https://www.bplmedicaltechnologies.com/blog/understanding-perfusion-index-in-pulse-oximeter-26434/

2. **Allen J., Physiol Meas. 2007;28(3):R1-R39** - Photoplethysmography and its application
   - Morfología PPG normal y alterada
   - Define PI como (AC/DC) × 100%
   - Muesca dicrótica varía según perfusión

3. **ProQuest - Sistema de Monitoreo de Pacientes**
   - AC/DC típico: **1-2%** del componente continuo
   - Reportado en múltiples trabajos instrumentales

4. **De La Peña Sanabria I., et al., Rev. Physiol. Meas., 2007**
   - Valores de PI en neonatos y adultos
   - Relevancia clínica de PI en cambios hemodinámicos

5. **Lima A, Bakker J. "Noninvasive monitoring of peripheral perfusion." Intensive Care Med, 2005**
   - PI correlaciona con flujo periférico
   - Rangos normales y patológicos

---

## **1. VALORES BASE PPG NORMAL**

### **Componentes DC y AC**

**Según literatura (ProQuest, trabajos académicos):**

| Componente | Valor típico | Justificación |
|------------|--------------|---------------|
| **DC** | **900–1200 mV** | Nivel de luz transmitida/reflejada por tejido y sangre basal. Depende del sensor y electrónica. |
| **AC** | **15–50 mV** | Pulsátil, sincronizado con latido. **PI típico 2–5% → AC = DC × PI** |
| **PI (AC/DC)** | **2–5%** | Reportado en estudios humanos (Allen 2007, Lima 2002, BPL 2023) |

### **Ejemplo Numérico**

```
DC = 1000 mV (nivel base)
PI = 3% (perfusión normal)
AC = DC × (PI/100) = 1000 × 0.03 = 30 mV

Señal PPG observada en osciloscopio:
- Baseline (diástole): 1000 mV
- Sístole (máxima absorción): 1000 - 30 = 970 mV
- Oscilación: ±30 mV alrededor de 1000 mV
```

**IMPORTANTE:** La señal PPG es **invertida** - durante sístole (más volumen de sangre) hay más absorción de luz, por lo que la señal **disminuye**.

---

## **2. TABLA DE CONDICIONES CLÍNICAS PPG**

### **Nueva Clasificación (basada en tabla del usuario)**

| AUTO_PPG_CONDITION | Condición | Rango PI (%) | DC (mV) | AC (mV) | Morfología / Notas |
|-------------------|-----------|--------------|---------|---------|---------------------|
| **0** | **Normal** | 1 – 5 | 900-1200 | 15-50 | Señal estándar, pico sistólico visible, muesca dicrótica sutil, forma de onda típica. |
| **1** | **Arritmia** | 1 – 5 | 900-1200 | 15-50 | Latidos irregulares, variabilidad RR ±15%, morfología similar a normal pero ritmo irregular. |
| **2** | **Weak Perfusion** | 0.02 – 0.4 | 900-1200 | 0.2-5 | Señal muy débil, pico casi inapreciable, muesca dicrótica desaparecida, diástole plana. |
| **3** | **Vasodilatación** | 5 – 10 | 900-1200 | 50-100 | Pico alto, muesca dicrótica marcada, diástole bien definida, flujo periférico aumentado. Morfología más "abultada". |
| **4** | **Strong Perfusion** | 10 – 20 | 900-1200 | 100-200 | Señal muy robusta, pico sistólico muy alto, muesca dicrótica prominente, diástole clara. Flujo periférico muy alto. |
| **5** | **Vasoconstricción** | 0.2 – 0.8 | 900-1200 | 2-10 | Pico pequeño, muesca apenas perceptible, diástole corta. Perfusión periférica reducida, morfología más plana. |

---

## **3. MÉTRICAS MEDIBLES EN OSCILOSCOPIO**

### **📏 Cómo se miden los parámetros PPG**

Estos valores permiten visualizar la señal PPG como aparecería en un osciloscopio real conectado al simulador. Son valores de **simulación didáctica** basados en referencias fisiológicas.

#### **1️⃣ RR interval (ms)**
**Cómo se mide:** Tiempo entre picos sistólicos consecutivos  
**Referencias:** Guyton & Hall, Textbook of Medical Physiology; Allen J., Physiol Meas, 2007  
**Valores típicos:**
- HR = 60 BPM → RR = 1000 ms
- HR = 75 BPM → RR = 800 ms
- HR = 100 BPM → RR = 600 ms

**Nota:** Aproximación basada en HR simulada; se usa para determinar la longitud de un ciclo PPG.

---

#### **2️⃣ Sístole y Diástole (tiempo de subida/bajada, ms)**
**Cómo se miden:**
- **Sístole:** Tiempo desde inicio de subida hasta pico sistólico
- **Diástole:** Tiempo desde pico sistólico hasta retorno a baseline

**Referencias:** Allen J., Photoplethysmography and its application, Physiol Meas, 2007  
**Valores típicos (HR ~75 BPM):**
- Sístole: 150–200 ms
- Diástole: 500–600 ms

**Nota:** Estos tiempos son rangos típicos; varían según morfología y frecuencia cardíaca.

---

#### **3️⃣ DC baseline (mV)**
**Cómo se mide:** Línea base constante (nivel diastólico)  
**Referencias:** No hay valores absolutos universales; depende del sensor y ganancia del circuito  
**Valor de simulación:** 1000 mV (didáctico)

**Nota:** Permite mantener proporciones AC/DC realistas y facilita visualización en osciloscopio virtual.

---

#### **4️⃣ AC amplitude (ΔV, mV)**
**Cómo se mide:** Diferencia pico a valle del pulso (baseline - pico sistólico)  
**Referencias:** Se deriva del PI reportado en literatura:
- BPL Medical Technologies: "Understanding Perfusion Index in Pulse Oximeter"
- Masimo Corporation: "Clinical Guide to Pulse Oximetry"
- Allen J., Physiol Meas, 2007

**Rangos por condición:**
- Weak perfusion → PI ~0.02–0.4% → AC ~0.2–5 mV
- Normal perfusion → PI 2–5% → AC ~15–50 mV
- Strong perfusion → PI 10–20% → AC ~100–200 mV

**Fórmula:** AC = DC × (PI/100)

**Nota:** Los valores en mV son aproximaciones proporcionales al DC para que la forma de onda refleje visiblemente las diferencias de perfusión.

---

#### **5️⃣ Muesca dicrótica (mV)**
**Cómo se mide:** Valle pequeño tras el pico sistólico (sobre la línea descendente)  
**Referencias:** Allen J., 2007; estudios de PPG de perfusión periférica  
**Valores típicos:**
- WEAK/VASOCONSTRICTION: Casi imperceptible (~0.5–2 mV)
- NORMAL: Visible (~2–5 mV)
- STRONG/VASODILATION: Muy marcada (~5–15 mV)

**Nota:** La amplitud absoluta se ajusta proporcionalmente según AC; sirve para enseñar diferencias en morfología entre condiciones.

---

### **✅ Resumen para Tesis**

> *"Los valores de AC, DC y tiempos de sístole/diástole se basan en referencias fisiológicas y morfológicas de la literatura de PPG (Allen, 2007; BPL Medical Technologies; Masimo Corporation) y se han adaptado a valores de simulación didácticos. La línea DC se fijó en 1000 mV, y la amplitud AC se escaló proporcionalmente para reflejar los rangos de PI reportados (0.02–20%). Los tiempos de sístole y diástole corresponden a ciclos de frecuencia cardíaca típica (HR 60–75 BPM). Estas aproximaciones permiten visualizar de manera clara los efectos de la perfusión débil, normal o fuerte en el simulador."*

---

## **4. TABLA CONSOLIDADA - TODAS LAS CONDICIONES PPG**

### **Métricas medibles en osciloscopio por condición**

| ID | Condición | HR (BPM) | RR (ms) | Sístole (ms) | Diástole (ms) | PI (%) | DC (mV) | AC (mV) | Muesca (mV) | Morfología |
|----|-----------|----------|---------|--------------|---------------|--------|---------|---------|-------------|------------|
| **0** | **NORMAL** | 60-100 | 600-1000 | 150-200 | 500-600 | 1-5 | 1000 | 15-50 | 2-5 | Pico sistólico pronunciado, muesca visible, diástole secundario |
| **1** | **ARRHYTHMIA** | 60-180 | 333-1000 | 150-200 | Variable | 1-5 | 1000 | 15-50 | 2-5 | Similar a NORMAL pero RR muy irregular (±15%), amplitud variable |
| **2** | **WEAK_PERFUSION** | 90-140 | 428-667 | 100-150 | 300-400 | 0.02-0.4 | 1000 | 0.2-5 | 0.5-1 | Pico muy atenuado (~25%), muesca desaparecida, diástole plana |
| **3** | **VASODILATION** | 60-90 | 667-1000 | 180-220 | 550-650 | 5-10 | 1000 | 50-100 | 5-10 | Pico alto y bien definido, muesca marcada, diástole prominente |
| **4** | **STRONG_PERFUSION** | 60-90 | 667-1000 | 200-250 | 600-700 | 10-20 | 1000 | 100-200 | 10-15 | Pico muy robusto, muesca muy prominente, máxima variación pulsátil |
| **5** | **VASOCONSTRICTION** | 60-100 | 600-1000 | 120-160 | 400-500 | 0.2-0.8 | 1000 | 2-10 | 0.5-2 | Pico pequeño (~30%), muesca apenas perceptible, onda "afilada" |

**Notas:**
- **RR (ms):** Calculado como 60000/HR
- **Sístole/Diástole:** Estimados como porcentajes del ciclo RR según morfología típica
- **AC (mV):** AC = DC × (PI/100), donde DC = 1000 mV
- **Muesca (mV):** Profundidad relativa sobre la línea descendente

---

## **5. VALORES DETALLADOS POR CONDICIÓN**

### **CONDICIÓN 0: NORMAL**
- **HR:** 60-100 BPM (típico 75 BPM)
- **RR interval:** 600-1000 ms (típico 800 ms)
- **Sístole:** 150-200 ms
- **Diástole:** 500-600 ms
- **PI:** 1-5% (típico 3%)
- **DC baseline:** 1000 mV
- **AC amplitude:** 15-50 mV (típico 30 mV para PI=3%)
- **Muesca dicrótica:** 2-5 mV (visible)
- **Rango señal:** 950-1000 mV
- **Morfología:**
  - Pico sistólico pronunciado a ~150 ms
  - Muesca dicrótica visible a ~240 ms (~30% del ciclo)
  - Pico diastólico secundario a ~320 ms (~40% del ciclo)
- **Referencias:** Allen 2007, BPL 2023, Guyton & Hall

---

### **CONDICIÓN 1: ARRHYTHMIA (Fibrilación Auricular)**
- **HR:** 60-180 BPM (muy irregular)
- **RR interval:** 333-1000 ms (variabilidad >15%, SD >100 ms)
- **Sístole:** 150-200 ms (similar a NORMAL)
- **Diástole:** Variable (depende del RR actual)
- **PI:** 1-5% (variable latido a latido)
- **DC baseline:** 1000 mV
- **AC amplitude:** 15-50 mV (variable entre latidos)
- **Muesca dicrótica:** 2-5 mV (variable)
- **Rango señal:** 950-1000 mV
- **Morfología:**
  - Similar a NORMAL pero RR muy irregular
  - 15% latidos prematuros (RR cortos)
  - Amplitud variable entre latidos
  - Imposible predecir siguiente latido
- **Referencias:** Shelley 2007 (amplitud variable en FA), Allen 2007

---

### **CONDICIÓN 2: WEAK_PERFUSION (Shock/Hipoperfusión)**
- **HR:** 90-140 BPM (taquicardia compensatoria, típico 115 BPM)
- **RR interval:** 428-667 ms (típico 520 ms)
- **Sístole:** 100-150 ms (acortada)
- **Diástole:** 300-400 ms (acortada por taquicardia)
- **PI:** 0.02-0.4% (muy bajo, típico 0.2%)
- **DC baseline:** 1000 mV
- **AC amplitude:** 0.2-5 mV (típico 2 mV, casi imperceptible)
- **Muesca dicrótica:** 0.5-1 mV (desaparecida, apenas perceptible)
- **Rango señal:** 995-1000 mV (casi plano)
- **Morfología:**
  - Pico sistólico muy atenuado (~25% amplitud normal)
  - Muesca dicrótica desaparecida
  - Diástole plana, sin componente secundario
  - Onda muy débil, apenas visible en osciloscopio
- **Causas:** Shock hipovolémico, hipotermia, hipoperfusión periférica
- **Referencias:** Reisner 2008 (PI <0.5% en shock), Lima 2005, BPL 2023

---

### **CONDICIÓN 3: VASODILATION**
- **HR:** 60-90 BPM (típico 75 BPM)
- **RR interval:** 667-1000 ms (típico 800 ms)
- **Sístole:** 180-220 ms (prolongada, subida más lenta)
- **Diástole:** 550-650 ms (bien definida)
- **PI:** 5-10% (alto, típico 7.5%)
- **DC baseline:** 1000 mV
- **AC amplitude:** 50-100 mV (típico 75 mV)
- **Muesca dicrótica:** 5-10 mV (marcada y profunda)
- **Rango señal:** 900-1000 mV
- **Morfología:**
  - Pico sistólico alto y bien definido a ~200 ms
  - Muesca dicrótica marcada y profunda a ~300 ms
  - Pico diastólico secundario prominente a ~400 ms
  - Onda más "abultada" y expansiva
  - Diástole bien diferenciada con descenso suave
- **Causas:** Ejercicio, calor, fiebre, sepsis temprana
- **Referencias:** Allen 2007, BPL 2023 (PI aumentado en vasodilatación)

---

### **CONDICIÓN 4: STRONG_PERFUSION (Flujo muy alto)**
- **HR:** 60-90 BPM (típico 70 BPM)
- **RR interval:** 667-1000 ms (típico 857 ms)
- **Sístole:** 200-250 ms (muy prolongada, subida lenta)
- **Diástole:** 600-700 ms (muy definida)
- **PI:** 10-20% (muy alto, típico 15%)
- **DC baseline:** 1000 mV
- **AC amplitude:** 100-200 mV (típico 150 mV)
- **Muesca dicrótica:** 10-15 mV (muy prominente)
- **Rango señal:** 800-1000 mV (máxima excursión)
- **Morfología:**
  - Pico sistólico muy robusto a ~220 ms
  - Muesca dicrótica muy prominente a ~340 ms (~40% ciclo)
  - Pico diastólico muy claro a ~450 ms
  - Máxima variación pulsátil (200 mV)
  - Señal muy visible y estable en osciloscopio
  - Descenso diastólico muy pronunciado
- **Causas:** Vasodilatación extrema, flujo hiperdinámico
- **Referencias:** BPL 2023 (PI puede llegar a 20%), Allen 2007

---

### **CONDICIÓN 5: VASOCONSTRICTION**
- **HR:** 60-100 BPM (típico 78 BPM)
- **RR interval:** 600-1000 ms (típico 769 ms)
- **Sístole:** 120-160 ms (acortada, subida rápida)
- **Diástole:** 400-500 ms (corta y plana)
- **PI:** 0.2-0.8% (muy bajo, típico 0.5%)
- **DC baseline:** 1000 mV
- **AC amplitude:** 2-10 mV (típico 5 mV)
- **Muesca dicrótica:** 0.5-2 mV (apenas perceptible)
- **Rango señal:** 990-1000 mV
- **Morfología:**
  - Pico sistólico pequeño (~30% amplitud normal) a ~140 ms
  - Muesca dicrótica apenas perceptible a ~230 ms
  - Pico diastólico casi ausente
  - Diástole corta y plana
  - Onda más "afilada" y estrecha
  - Ancho sistólico reducido (pico más angosto)
  - Descenso rápido sin componente secundario
- **Causas:** Frío extremo, estrés, vasopresores, hipovolemia temprana
- **Referencias:** Allen & Murray 2002 (morfología alterada), Reisner 2008

---

## **4. MAPEO A DAC 8-BIT (0-255)**

### **Estrategia de Escalado**

**Opción recomendada:** Escalado fijo para mantener proporciones reales

```
Rango físico PPG: 700-1200 mV (cubre todas las condiciones)
Mapeo lineal: 700 mV → 0, 1200 mV → 255

Para cada muestra:
DAC_value = (voltage_mV - 700) / (1200 - 700) × 255
DAC_value = (voltage_mV - 700) / 500 × 255
```

**Ventajas:**
- ✅ Preserva proporciones reales entre condiciones
- ✅ WEAK_PERFUSION se ve débil vs STRONG_PERFUSION robusta
- ✅ Valores medibles con cursor en osciloscopio
- ✅ Didácticamente claro (no distorsiona morfología)

**Ejemplo:**
```
NORMAL (1000 mV baseline, AC=30 mV):
- Diástole: 1000 mV → DAC = 153
- Sístole: 970 mV → DAC = 138
- Variación: 15 niveles DAC

WEAK_PERFUSION (1000 mV baseline, AC=2 mV):
- Diástole: 1000 mV → DAC = 153
- Sístole: 998 mV → DAC = 152
- Variación: 1 nivel DAC (casi plano)

STRONG_PERFUSION (1000 mV baseline, AC=150 mV):
- Diástole: 1000 mV → DAC = 153
- Sístole: 850 mV → DAC = 76
- Variación: 77 niveles DAC (muy visible)
```

---

## **5. ESCALADO A NEXTION WAVEFORM**

### **Para pantalla 7" (700×380 px)**

**Configuración sugerida:**
- **Eje Y:** 700-1200 mV (rango completo PPG)
- **Grid vertical:** 100 mV/div (5 divisiones = 500 mV)
- **Eje X:** Tiempo en ms (depende de HR)
- **Grid horizontal:** 200 ms/div (para HR=75 BPM, ciclo=800 ms)

**Mapeo Y:**
```cpp
uint16_t yPixel = (uint16_t)((1200.0f - voltage_mV) / 500.0f * 380.0f);
// 1200 mV → Y=0 (arriba)
// 700 mV → Y=380 (abajo)
```

---

## **6. VALIDACIÓN EXPERIMENTAL**

### **Pruebas Recomendadas**

1. **Serial Plotter Arduino IDE:**
   ```
   >ppg:VOLTAGE_MV,pi:PI_PERCENT,hr:HR_BPM,beats:COUNT
   ```

2. **Osciloscopio en GPIO25:**
   - Medir voltajes con cursor
   - Verificar AC/DC ratio
   - Confirmar morfología

3. **Comparación entre condiciones:**
   - WEAK vs NORMAL vs STRONG: amplitud claramente diferente
   - VASOCONSTRICTION: onda afilada vs VASODILATION: onda abultada

---

## **7. RESUMEN EJECUTIVO**

| Aspecto | Valor |
|---------|-------|
| **DC nominal** | 1000 mV |
| **Rango DC** | 900-1200 mV (±10%) |
| **AC típico (PI=3%)** | 30 mV |
| **Rango AC total** | 0.2-200 mV (PI=0.02-20%) |
| **PI normal** | 2-5% |
| **PI mínimo** | 0.02% (shock severo) |
| **PI máximo** | 20% (vasodilatación extrema) |
| **Rango señal total** | 700-1200 mV |
| **Escalado DAC** | Lineal fijo 700-1200 mV → 0-255 |

---

**Fecha:** Diciembre 2024  
**Versión:** 2.0 - Valores reales en mV basados en literatura científica  
**Autor:** Basado en referencias BPL 2023, Allen 2007, ProQuest, Lima 2005
