# Límites de Sliders por Patología - Validación Física

**Versión:** 1.0  
**Fecha:** 7 de diciembre de 2025  
**Propósito:** Validar que los sliders en la interfaz Nextion respeten los límites fisiológicos por condición

---

## 📋 Cómo Usar Este Documento

Para cada patología, se listan los **rangos válidos** de cada parámetro. Durante la validación física:

1. Selecciona una condición en el menú ECG/EMG/PPG
2. Presiona **bt_ir** → Presiona **PLAY**
3. Abre **PARAMETROS** (pop-up)
4. Mueve cada slider y verifica:
   - ✅ El slider NO permite valores fuera del rango especificado
   - ✅ El valor por defecto está dentro del rango
   - ✅ Al aplicar cambios, la señal física en el DAC refleja el cambio

---

## 🫀 ECG - Límites por Condición

### **NORMAL**
| Parámetro | Rango Válido | Valor por Defecto | Unidad |
|-----------|--------------|-------------------|--------|
| **Heart Rate (HR)** | 60 - 100 | 75 | BPM |
| **QRS Amplitude** | 50 - 200 | 100 | % (0.5x - 2.0x) |
| **Noise Level** | 0 - 100 | 5 | % |
| **HRV** | 0 - 20 | 5 | % de HR |

**Validación física:**
- DAC debería mostrar ritmo sinusal estable entre 60-100 BPM
- Amplitud QRS: ~1.0-1.5 mV en osciloscopio (100% = 1.0 mV nominal)
- Ruido: apenas perceptible al 5%

---

### **TACHYCARDIA (Taquicardia)**
| Parámetro | Rango Válido | Valor por Defecto | Unidad |
|-----------|--------------|-------------------|--------|
| **Heart Rate (HR)** | 100 - 180 | 130 | BPM |
| **QRS Amplitude** | 50 - 200 | 100 | % |
| **Noise Level** | 0 - 100 | 5 | % |
| **HRV** | 0 - 20 | 5 | % de HR |

**Validación física:**
- DAC: ritmo rápido pero regular >100 BPM
- Amplitud normal (~1.0 mV)

---

### **BRADYCARDIA (Bradicardia)**
| Parámetro | Rango Válido | Valor por Defecto | Unidad |
|-----------|--------------|-------------------|--------|
| **Heart Rate (HR)** | 30 - 59 | 45 | BPM |
| **QRS Amplitude** | 50 - 200 | 100 | % |
| **Noise Level** | 0 - 100 | 5 | % |
| **HRV** | 0 - 20 | 5 | % de HR |

**Validación física:**
- DAC: ritmo lento pero regular <60 BPM
- Intervalos RR muy largos (>1 segundo)

---

### **ATRIAL FIBRILLATION (Fibrilación Auricular)**
| Parámetro | Rango Válido | Valor por Defecto | Unidad |
|-----------|--------------|-------------------|--------|
| **Heart Rate (HR)** | 60 - 180 | 110 | BPM |
| **QRS Amplitude** | 50 - 200 | 100 | % |
| **Noise Level** | 0 - 100 | 5 | % |
| **HRV** | 20 - 50 | 30 | % de HR (irregular) |

**Validación física:**
- DAC: ritmo completamente irregular
- Sin onda P visible (reemplazada por ondas f)
- RR variable entre latidos

---

### **VENTRICULAR FIBRILLATION (Fibrilación Ventricular)**
| Parámetro | Rango Válido | Valor por Defecto | Unidad |
|-----------|--------------|-------------------|--------|
| **Heart Rate (HR)** | 150 - 500 | 300 | BPM |
| **QRS Amplitude** | 0 - 100 | 0 | % (no hay QRS) |
| **Noise Level** | 50 - 100 | 80 | % (caótico) |

**Validación física:**
- DAC: señal caótica sin morfología definida
- No hay complejos QRS identificables
- Frecuencia muy alta y desorganizada

---

### **PREMATURE VENTRICULAR (PVC - Extrasístole Ventricular)**
| Parámetro | Rango Válido | Valor por Defecto | Unidad |
|-----------|--------------|-------------------|--------|
| **Heart Rate (HR)** | 50 - 120 | 75 | BPM |
| **QRS Amplitude** | 120 - 250 | 180 | % (QRS ancho) |
| **Noise Level** | 0 - 100 | 5 | % |

**Validación física:**
- DAC: ritmo sinusal con QRS anchos y grandes intercalados
- PVCs tienen mayor amplitud que latidos normales
- Sin onda P precediendo el PVC

---

### **BUNDLE BRANCH BLOCK (Bloqueo de Rama)**
| Parámetro | Rango Válido | Valor por Defecto | Unidad |
|-----------|--------------|-------------------|--------|
| **Heart Rate (HR)** | 40 - 100 | 70 | BPM |
| **QRS Amplitude** | 80 - 150 | 120 | % |
| **Noise Level** | 0 - 100 | 5 | % |

**Validación física:**
- DAC: QRS ensanchado (>120 ms visualmente)
- Morfología alterada (mellado o bimodal)

---

### **ST ELEVATION (Elevación ST - STEMI)**
| Parámetro | Rango Válido | Valor por Defecto | Unidad |
|-----------|--------------|-------------------|--------|
| **Heart Rate (HR)** | 50 - 110 | 80 | BPM |
| **QRS Amplitude** | 50 - 200 | 100 | % |
| **ST Shift** | +10 - +40 | +25 | % (+0.1 a +0.4 mV) |
| **T Amplitude** | 100 - 200 | 150 | % (hiperaguda) |

**Validación física:**
- DAC: segmento ST elevado por encima de la línea isoeléctrica
- Onda T grande y puntiaguda
- Elevación de ~0.25 mV (2.5 mm a escala estándar)

---

### **ST DEPRESSION (Depresión ST - Isquemia)**
| Parámetro | Rango Válido | Valor por Defecto | Unidad |
|-----------|--------------|-------------------|--------|
| **Heart Rate (HR)** | 50 - 150 | 90 | BPM |
| **QRS Amplitude** | 50 - 200 | 100 | % |
| **ST Shift** | -30 - -5 | -15 | % (-0.3 a -0.05 mV) |
| **T Amplitude** | 30 - 80 | 50 | % (invertida/aplanada) |

**Validación física:**
- DAC: segmento ST deprimido por debajo de la línea isoeléctrica
- Onda T pequeña o invertida
- Depresión de ~0.15 mV (1.5 mm a escala estándar)

---

## 💪 EMG - Límites por Condición

### **REST (Reposo)**
| Parámetro | Rango Válido | Valor por Defecto | Unidad |
|-----------|--------------|-------------------|--------|
| **Excitation Level** | 0 - 10 | 0 | % MVC |
| **Amplitude** | 10 - 50 | 20 | % |
| **Noise Level** | 0 - 100 | 5 | % |

**Validación física:**
- DAC: señal de muy baja amplitud, casi plana con ruido de fondo
- Amplitud RMS: ~0.02-0.05 mV

---

### **MILD CONTRACTION (Contracción Leve)**
| Parámetro | Rango Válido | Valor por Defecto | Unidad |
|-----------|--------------|-------------------|--------|
| **Excitation Level** | 10 - 30 | 20 | % MVC |
| **Amplitude** | 50 - 100 | 70 | % |
| **Noise Level** | 0 - 100 | 5 | % |

**Validación física:**
- DAC: bursts intermitentes de ~0.5-0.7 mV RMS
- Densidad moderada de MUAPs

---

### **MODERATE CONTRACTION (Contracción Moderada)**
| Parámetro | Rango Válido | Valor por Defecto | Unidad |
|-----------|--------------|-------------------|--------|
| **Excitation Level** | 30 - 60 | 50 | % MVC |
| **Amplitude** | 80 - 150 | 100 | % |
| **Noise Level** | 0 - 100 | 5 | % |

**Validación física:**
- DAC: señal densa y continua, ~1.0 mV RMS
- Interferencia moderada de MUAPs

---

### **STRONG CONTRACTION (Contracción Fuerte)**
| Parámetro | Rango Válido | Valor por Defecto | Unidad |
|-----------|--------------|-------------------|--------|
| **Excitation Level** | 60 - 90 | 80 | % MVC |
| **Amplitude** | 120 - 200 | 150 | % |
| **Noise Level** | 0 - 100 | 5 | % |

**Validación física:**
- DAC: señal muy densa, ~1.5 mV RMS
- Patrón de interferencia completo

---

### **MAXIMUM CONTRACTION (Contracción Máxima)**
| Parámetro | Rango Válido | Valor por Defecto | Unidad |
|-----------|--------------|-------------------|--------|
| **Excitation Level** | 80 - 100 | 100 | % MVC |
| **Amplitude** | 150 - 250 | 200 | % |
| **Noise Level** | 0 - 100 | 5 | % |

**Validación física:**
- DAC: señal de máxima densidad, ~2.0 mV RMS
- Patrón de interferencia completo y saturado

---

### **TREMOR (Temblor)**
| Parámetro | Rango Válido | Valor por Defecto | Unidad |
|-----------|--------------|-------------------|--------|
| **Excitation Level** | 10 - 50 | 30 | % MVC |
| **Amplitude** | 50 - 150 | 100 | % |
| **Noise Level** | 10 - 50 | 20 | % |

**Validación física:**
- DAC: bursts rítmicos a 4-8 Hz (temblor)
- Amplitud variable, ~1.0 mV RMS

---

### **MYOPATHY (Miopatía)**
| Parámetro | Rango Válido | Valor por Defecto | Unidad |
|-----------|--------------|-------------------|--------|
| **Excitation Level** | 10 - 40 | 30 | % MVC |
| **Amplitude** | 20 - 60 | 40 | % (reducida) |
| **Noise Level** | 0 - 100 | 5 | % |

**Validación física:**
- DAC: MUAPs pequeños y cortos, ~0.4 mV RMS
- Densidad aumentada para compensar debilidad

---

### **NEUROPATHY (Neuropatía)**
| Parámetro | Rango Válido | Valor por Defecto | Unidad |
|-----------|--------------|-------------------|--------|
| **Excitation Level** | 30 - 100 | 50 | % MVC |
| **Amplitude** | 150 - 300 | 200 | % (aumentada) |
| **Noise Level** | 0 - 100 | 5 | % |

**Validación física:**
- DAC: MUAPs gigantes y polifásicos, ~2.0 mV RMS
- Reinervación colateral (MUs más grandes)

---

### **FASCICULATION (Fasciculación)**
| Parámetro | Rango Válido | Valor por Defecto | Unidad |
|-----------|--------------|-------------------|--------|
| **Excitation Level** | 0 - 30 | 10 | % MVC |
| **Amplitude** | 50 - 150 | 100 | % |
| **Noise Level** | 0 - 100 | 5 | % |

**Validación física:**
- DAC: MUAPs espontáneos e irregulares sin contracción voluntaria
- Baja frecuencia (1-3 Hz)

---

### **FATIGUE (Fatiga)**
| Parámetro | Rango Válido | Valor por Defecto | Unidad |
|-----------|--------------|-------------------|--------|
| **Excitation Level** | 20 - 80 | 60 | % MVC |
| **Amplitude** | 50 - 150 | 100 | % |
| **Noise Level** | 10 - 50 | 20 | % |

**Validación física:**
- DAC: amplitud decrece con el tiempo
- Frecuencia mediana de potencia disminuye

---

## 🩸 PPG - Límites por Condición

### **NORMAL**
| Parámetro | Rango Válido | Valor por Defecto | Unidad |
|-----------|--------------|-------------------|--------|
| **Heart Rate (HR)** | 60 - 100 | 75 | BPM |
| **Perfusion Index (PI)** | 20 - 100 | 50 | % (2.0% - 10.0%) |
| **Noise Level** | 0 - 100 | 5 | % |

**Validación física:**
- DAC: onda pulsátil regular con muesca dicrótica clara
- PI ~5% (50 en slider = 5.0%)
- Amplitud pico-a-pico ~0.5-1.0 V

---

### **ARRHYTHMIA (Arritmia)**
| Parámetro | Rango Válido | Valor por Defecto | Unidad |
|-----------|--------------|-------------------|--------|
| **Heart Rate (HR)** | 50 - 150 | 75 | BPM |
| **Perfusion Index (PI)** | 20 - 80 | 40 | % |
| **Noise Level** | 0 - 100 | 10 | % |

**Validación física:**
- DAC: intervalos RR irregulares
- Amplitud variable entre latidos

---

### **WEAK PERFUSION (SpO2 Bajo)**
| Parámetro | Rango Válido | Valor por Defecto | Unidad |
|-----------|--------------|-------------------|--------|
| **Heart Rate (HR)** | 90 - 140 | 110 | BPM |
| **Perfusion Index (PI)** | 3 - 20 | 8 | % (0.3% - 2.0%) |
| **Noise Level** | 0 - 100 | 5 | % |

**Validación física:**
- DAC: amplitud muy reducida (~0.1-0.3 V)
- Muesca dicrótica casi imperceptible
- Posible taquicardia compensatoria

---

### **STRONG PERFUSION (Perfusión Fuerte)**
| Parámetro | Rango Válido | Valor por Defecto | Unidad |
|-----------|--------------|-------------------|--------|
| **Heart Rate (HR)** | 50 - 90 | 70 | BPM |
| **Perfusion Index (PI)** | 100 - 200 | 120 | % (10.0% - 20.0%) |
| **Noise Level** | 0 - 100 | 5 | % |

**Validación física:**
- DAC: amplitud grande (~1.5-2.0 V)
- Muesca dicrótica muy prominente
- Bradicardia relativa

---

### **VASOCONSTRICTION (Vasoconstricción)**
| Parámetro | Rango Válido | Valor por Defecto | Unidad |
|-----------|--------------|-------------------|--------|
| **Heart Rate (HR)** | 70 - 110 | 85 | BPM |
| **Perfusion Index (PI)** | 10 - 50 | 30 | % (1.0% - 5.0%) |
| **Noise Level** | 0 - 100 | 5 | % |

**Validación física:**
- DAC: amplitud reducida (~0.3-0.5 V)
- Muesca dicrótica menos pronunciada
- Onda más estrecha (menor compliance arterial)

---

### **MOTION ARTIFACT (Artefactos de Movimiento)**
| Parámetro | Rango Válido | Valor por Defecto | Unidad |
|-----------|--------------|-------------------|--------|
| **Heart Rate (HR)** | 60 - 100 | 75 | BPM |
| **Perfusion Index (PI)** | 20 - 100 | 50 | % |
| **Noise Level** | 30 - 100 | 50 | % (alto) |

**Validación física:**
- DAC: señal con distorsiones superpuestas
- Línea base inestable
- Picos espurios entre latidos

---

### **LOW SPO2 (Hipoxia - SpO2 Bajo)**
| Parámetro | Rango Válido | Valor por Defecto | Unidad |
|-----------|--------------|-------------------|--------|
| **Heart Rate (HR)** | 80 - 130 | 100 | BPM |
| **Perfusion Index (PI)** | 5 - 30 | 15 | % (0.5% - 3.0%) |
| **Noise Level** | 0 - 100 | 10 | % |

**Validación física:**
- DAC: amplitud reducida (~0.2-0.4 V)
- Taquicardia compensatoria
- Muesca dicrótica atenuada

---

## 🔧 Validación Práctica - Checklist

Para cada condición, verificar:

- [ ] **Slider HR/Excitation:** Se mueve dentro del rango, no excede límites
- [ ] **Slider Amplitude:** Cambios se reflejan en la amplitud del DAC (osciloscopio)
- [ ] **Slider Noise:** Ruido visible aumenta/disminuye proporcionalmente
- [ ] **Valores por defecto:** Al entrar a parámetros, los sliders están en el default
- [ ] **Aplicar cambios:** Al presionar `bt_act`, la señal física cambia inmediatamente
- [ ] **Reset:** Al presionar `bt_rst`, los sliders vuelven al default de la condición

---

## 📊 Conversión de Valores

### Sliders en Nextion (0-100 o rangos específicos)

| Parámetro UI | Conversión a Física | Ejemplo |
|--------------|---------------------|---------|
| **HR** | Slider value = BPM directo | 75 → 75 BPM |
| **Amplitude ECG/EMG** | `value / 100.0f` | 150 → 1.5x factor |
| **Perfusion Index PPG** | `value / 10.0f` | 50 → 5.0% PI |
| **Noise** | `value / 100.0f` | 20 → 0.2 (20% de amplitud) |
| **ST Shift** | `value / 100.0f` en mV | 25 → 0.25 mV |

---

## ✅ Criterios de Éxito

La validación es exitosa si:

1. **Límites respetados:** Ningún slider permite valores fuera del rango por condición
2. **Coherencia física:** Los cambios en sliders se reflejan correctamente en el DAC
3. **Defaults correctos:** Valores por defecto están dentro del rango y son clínicamente razonables
4. **Aplicación inmediata:** Al cerrar pop-up de parámetros, la señal cambia sin necesidad de reiniciar

---

**Última actualización:** 7 de diciembre de 2025  
**Responsable:** Equipo BioSignalSimulator Pro
