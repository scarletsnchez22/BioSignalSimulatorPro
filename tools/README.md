# 🔬 Herramientas de Validación - BioSimulator Pro

## Descripción

Este directorio contiene herramientas Python para validar las señales biológicas generadas por el ESP32 contra rangos clínicos establecidos científicamente.

## Archivos

| Archivo | Descripción |
|---------|-------------|
| `clinical_ranges.py` | Tablas de rangos clínicos para ECG, EMG y PPG |
| `signal_validator.py` | Script comprobador que lee datos del serial |

---

## 📊 Tablas de Rangos Clínicos

### Ver todas las tablas
```bash
python signal_validator.py --show-ranges
```

### ECG - Parámetros por Patología (8 condiciones)

| # | Condición | HR (BPM) | RR (ms) | R (mV) | ST (mV) | QRS (ms) |
|---|-----------|----------|---------|--------|---------|----------|
| 0 | Normal | 60-100 | 600-1000 | 0.5-1.5 | ±0.05 | 80-100 |
| 1 | Taquicardia | 100-180 | 333-600 | 0.5-1.5 | ±0.05 | 80-100 |
| 2 | Bradicardia | 30-59 | 1017-2000 | 0.5-1.5 | ±0.05 | 80-100 |
| 3 | AFib | 57-188 | 319-1053 | 0.4-1.5 | ±0.1 | 80-120 |
| 4 | VFib | 240-600* | 100-250 | 0.1-1.0 | N/A | N/A |
| 5 | PVC | 50-120 | 500-1200 | 0.5-2.5 | ±0.3 | 120-200 |
| 6 | STEMI | 60-100 | 600-1000 | 0.5-1.8 | +0.25 a +0.60 | 80-120 |
| 7 | Isquemia | 60-100 | 600-1000 | 0.5-1.5 | -0.55 a -0.20 | 80-100 |

*VFib: pseudo-HR basado en frecuencia de ondas caóticas (4-10 Hz)

### EMG - Parámetros por Condición (8 condiciones)

| # | Condición | RMS (mV) | MUs | FR (Hz) | Contracción (%) |
|---|-----------|----------|-----|---------|-----------------|
| 0 | Reposo (0-10% MVC) | 0-0.08 | 0-10 | 0-8 | 0-10 |
| 1 | Baja (10-30% MVC) | 0.08-0.4 | 10-35 | 8-18 | 10-30 |
| 2 | Moderada (30-60% MVC) | 0.3-1.2 | 30-65 | 12-28 | 30-60 |
| 3 | Alta (60-100% MVC) | 0.8-5.0 | 60-100 | 20-50 | 60-100 |
| 4 | Temblor Parkinson | 0.1-1.0 | 10-50 | 4-8 | 10-50 |
| 5 | Miopatía | 0.03-0.25 | 35-75 | 15-40 | 25-55 |
| 6 | Neuropatía | 0.5-4.0 | 8-35 | 6-18 | 35-65 |
| 7 | Fasciculación | 0-0.3 | 0-5 | 0.3-3 | 0-8 |

### PPG - Parámetros por Condición (6 condiciones)

| # | Condición | HR (BPM) | RR (ms) | PI (%) | SpO2 (%) | Notas |
|---|-----------|----------|---------|--------|----------|-------|
| 0 | Normal | 60-100 | 600-1000 | 2-5 | 95-100 | Muesca dicrótica visible |
| 1 | Arritmia | 60-180 | 333-1500 | 1-5 | 92-100 | RR muy variable (>15%) |
| 2 | Perf. Débil | 90-140 | 428-667 | 0.1-0.5 | 88-98 | Taquicardia compensatoria |
| 3 | Perf. Fuerte | 60-90 | 667-1000 | 5-20 | 96-100 | Vasodilatación, fiebre |
| 4 | Vasoconstricción | 60-100 | 600-1000 | **0.2-0.8** | 91-100 | PI muy bajo, muesca atenuada |
| 5 | SpO2 Bajo | 90-130 | 462-667 | 0.5-3.5 | 70-90 | Hipoxemia, causa pulmonar |

**Nota:** PI y SpO2 son valores dinámicos con variabilidad gaussiana natural.

---

## 🔧 Uso del Validador

### Requisitos
```bash
pip install pyserial
```

### Listar puertos disponibles
```bash
python signal_validator.py --list-ports
```

### Validar ECG
```bash
# ECG Normal
python signal_validator.py --port COM4 --signal ecg --condition NORMAL

# Taquicardia
python signal_validator.py --port COM4 --signal ecg --condition TACHY

# Fibrilación Ventricular
python signal_validator.py --port COM4 --signal ecg --condition VFIB

# STEMI (Infarto)
python signal_validator.py --port COM4 --signal ecg --condition STE
```

### Validar EMG
```bash
# Reposo (0-10% MVC)
python signal_validator.py --port COM4 --signal emg --condition REST

# Contracción Baja (10-30% MVC)
python signal_validator.py --port COM4 --signal emg --condition LOW

# Contracción Moderada (30-60% MVC)
python signal_validator.py --port COM4 --signal emg --condition MODERATE

# Contracción Alta (60-100% MVC)
python signal_validator.py --port COM4 --signal emg --condition HIGH

# Temblor Parkinson (4-6 Hz)
python signal_validator.py --port COM4 --signal emg --condition TREMOR

# Miopatía (MUAPs pequeños)
python signal_validator.py --port COM4 --signal emg --condition MYOPATHY

# Neuropatía (MUAPs gigantes)
python signal_validator.py --port COM4 --signal emg --condition NEUROPATHY

# Fasciculación (disparos espontáneos)
python signal_validator.py --port COM4 --signal emg --condition FASCICULATION
```

### Validar PPG
```bash
# Normal (PI 2-5%, SpO2 95-100%)
python signal_validator.py --port COM4 --signal ppg --condition NORMAL

# Arritmia (RR muy variable)
python signal_validator.py --port COM4 --signal ppg --condition ARRHYTHMIA

# Perfusión Débil (PI <0.5%, taquicardia)
python signal_validator.py --port COM4 --signal ppg --condition WEAK_PERFUSION

# Perfusión Fuerte (PI >5%, vasodilatación)
python signal_validator.py --port COM4 --signal ppg --condition STRONG_PERFUSION

# Vasoconstricción Marcada (PI 0.2-0.8%, onda aplanada)
python signal_validator.py --port COM4 --signal ppg --condition VASOCONSTRICTION

# SpO2 Bajo / Hipoxemia (SpO2 70-90%)
python signal_validator.py --port COM4 --signal ppg --condition LOW_SPO2
```

### Opciones adicionales
```bash
# Duración limitada (30 segundos)
python signal_validator.py --port COM4 --signal ecg --condition NORMAL --duration 30

# Modo silencioso (solo resumen)
python signal_validator.py --port COM4 --signal ecg --condition NORMAL --quiet
```

---

## 📋 Formato de Datos Serial

El ESP32 envía datos en el siguiente formato:

### ECG
```
>ecg:VALUE,hr:VALUE,rr:VALUE,ramp:VALUE,st:VALUE,qrs:VALUE,beats:VALUE
```
- `ecg`: Valor de señal en mV
- `hr`: Frecuencia cardíaca en BPM
- `rr`: Intervalo RR en ms
- `ramp`: Amplitud onda R en mV
- `st`: Desviación ST en mV
- `qrs`: Duración QRS en ms
- `beats`: Contador de latidos

### EMG
```
>emg:VALUE,rms:VALUE,mus:VALUE,fr:VALUE,cont:VALUE
```
- `emg`: Valor de señal en mV
- `rms`: Amplitud RMS en mV
- `mus`: Unidades motoras activas
- `fr`: Frecuencia de disparo en Hz
- `cont`: Porcentaje de contracción

### PPG
```
>ppg:VALUE,hr:VALUE,rr:VALUE,pi:VALUE,spo2:VALUE,beats:VALUE
```
- `ppg`: Valor de señal normalizado (0-1)
- `hr`: Frecuencia cardíaca en BPM
- `rr`: Intervalo RR en ms
- `pi`: Índice de perfusión en %
- `spo2`: Saturación de oxígeno en %
- `beats`: Contador de latidos

---

## 📚 Referencias Científicas

### ECG
- McSharry PE, et al. IEEE Trans Biomed Eng. 2003;50(3):289-294
- Task Force ESC/NASPE. Circulation. 1996;93:1043-1065
- Surawicz B, Knilans T. Chou's Electrocardiography. 8th ed. 2008
- Goldberger AL. Clinical Electrocardiography. 9th ed. 2017

### EMG
- Fuglevand AJ, et al. J Neurophysiol. 1993;70(6):2470-2488
- De Luca CJ. J Appl Biomech. 1997;13(2):135-163
- Henneman E, et al. J Neurophysiol. 1965;28:560-580
- Merletti R, Parker P. Electromyography: Physiology, Engineering. 2004

### PPG
- Allen J. Physiol Meas. 2007;28(3):R1-R39
- Elgendi M. Curr Cardiol Rev. 2012;8(1):14-25
- Reisner A, et al. Anesthesiology. 2008;108(5):950-958
- Jubran A. Pulse oximetry. Crit Care. 2015;19:272
- Lima A, Bakker J. Intensive Care Med. 2005;31(10):1316-1326

---

## 🎯 Ejemplo de Salida

```
================================================================================
BIOSIMULATOR PRO - VALIDADOR DE SEÑALES
================================================================================
Señal: ECG
Condición: NORMAL
Puerto: COM4 @ 115200 baud
Inicio: 2024-01-15 14:30:00
================================================================================

RANGOS CLÍNICOS ESPERADOS:
----------------------------------------
  HR:     60 - 100 BPM
  RR:     600 - 1000 ms
  R amp:  0.50 - 1.50 mV
  ST:     -0.05 - +0.05 mV
  QRS:    80 - 100 ms

  Notas: HR 60-100 BPM, QRS <100ms, ST isoeléctrico

Presione Ctrl+C para detener y ver resumen.

--------------------------------------------------------------------------------
✓ ECG: +0.8234mV | HR: 72.5 | RR: 828ms | R: 1.02mV | ST: +0.012mV | QRS: 92ms
✓ ECG: +0.1523mV | HR: 72.5 | RR: 828ms | R: 1.02mV | ST: +0.012mV | QRS: 92ms
...

================================================================================
RESUMEN DE VALIDACIÓN
================================================================================
Duración: 30.2 segundos
Muestras totales: 3020
Muestras válidas: 2985 (98.8%)
Muestras inválidas: 35 (1.2%)

----------------------------------------
VALIDACIÓN POR PARÁMETRO:
----------------------------------------
  hr             : 3020/3020 válidos (100.0%) ✓
  rr             : 2998/3020 válidos (99.3%) ✓
  r_amplitude    : 3020/3020 válidos (100.0%) ✓
  st_deviation   : 3015/3020 válidos (99.8%) ✓
  qrs_duration   : 3020/3020 válidos (100.0%) ✓

================================================================================
VEREDICTO: ✓ SEÑAL CORRECTA - Cumple rangos clínicos
================================================================================
```
