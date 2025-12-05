# BioSimulator Pro - Plan de Proyecto v1.0

**Documento de Especificación Técnica Completa**

**Versión:** 1.0  
**Estado:** Versión Inicial

---

## Tabla de Contenidos

1. [Resumen Ejecutivo](#1-resumen-ejecutivo)
2. [Objetivos del Proyecto](#2-objetivos-del-proyecto)
3. [Arquitectura del Sistema](#3-arquitectura-del-sistema)
4. [Especificación de Hardware](#4-especificación-de-hardware)
5. [Modelos de Señales](#5-modelos-de-señales)
6. [Sistema de Parametrización](#6-sistema-de-parametrización)
7. [Interfaz de Usuario (Nextion)](#7-interfaz-de-usuario-nextion)
8. [Aplicación de Escritorio](#8-aplicación-de-escritorio)
9. [Protocolo de Comunicación](#9-protocolo-de-comunicación)
10. [Validación y Métricas](#10-validación-y-métricas)
11. [Estructura de Código](#11-estructura-de-código)
12. [Plan de Implementación](#12-plan-de-implementación)
13. [Referencias Científicas](#13-referencias-científicas)
14. [Limitaciones Conocidas](#14-limitaciones-conocidas)

---

## 1. Resumen Ejecutivo

### 1.1 Descripción del Proyecto

BioSimulator Pro es un dispositivo de simulación de señales fisiológicas diseñado para el Laboratorio de Neurociencias y Bioingeniería. El sistema genera señales de ECG, EMG y PPG de forma **dinámica** (no estática/bucle) con características morfológicas y temporales clínicamente válidas.

### 1.2 Características Principales

| Característica | Descripción |
|----------------|-------------|
| **Señales** | ECG (9 condiciones), EMG (10 condiciones), PPG (7 condiciones) |
| **Generación** | Dinámica con variabilidad latido a latido (HRV real) |
| **Salida** | DAC 8-bit (0-3.3V) @ 1000 Hz para conexión con osciloscopio |
| **Visualización** | Pantalla Nextion 3.2" táctil con waveform y métricas |
| **Parametrización** | En tiempo real con límites clínicos por condición |
| **Guardado** | Exportación CSV via aplicación de escritorio |

### 1.3 Entregables

1. Firmware ESP32 completo y documentado
2. Interfaz Nextion (.HMI) funcional
3. Aplicación de escritorio (Python) para visualización y guardado
4. Diseño de PCB (KiCad)
5. Diseño mecánico (STL para impresión 3D)
6. Reporte de validación contra PhysioNet

---

## 2. Objetivos del Proyecto

### 2.1 Objetivos Principales

1. **Diseño e implementación** de simulador de señales fisiológicas (ECG, EMG, PPG)
2. **Visualización en pantalla** Nextion con waveform, unidades y métricas
3. **Parametrización en tiempo real** con rangos clínicos válidos
4. **Salida analógica** compatible con equipos de laboratorio
5. **Validación científica** contra bases de datos reales (PhysioNet)

### 2.2 Requisitos Específicos

| Requisito | Especificación |
|-----------|----------------|
| Señal dinámica | Cada latido/ciclo debe ser único (no bucle estático) |
| Límites por patología | Parámetros restringidos a rangos fisiológicamente válidos |
| Sin colisión de instrucciones | Sistema de parametrización con aplicación diferida |
| Unidades en pantalla | Waveform con escalas (mV, µV, %) |
| Salida DAC | Compatible con osciloscopio y prototipos de prueba |
| Guardado de datos | CSV con timestamp, parámetros y métricas |

---

## 3. Arquitectura del Sistema

### 3.1 Diagrama de Arquitectura

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    BIOSIMULATOR PRO v1.0                                 │
└─────────────────────────────────────────────────────────────────────────────┘

                              CAPA DE APLICACIÓN
┌─────────────────────────────────────────────────────────────────────────────┐
│  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐          │
│  │   App Desktop    │  │   Nextion HMI    │  │   Serial CLI     │          │
│  │   (Python/Qt)    │  │   (Pantalla)     │  │   (Debug)        │          │
│  └────────┬─────────┘  └────────┬─────────┘  └────────┬─────────┘          │
└───────────┼─────────────────────┼─────────────────────┼─────────────────────┘
            │                     │                     │
            └─────────────────────┼─────────────────────┘
                                  │
                        CAPA DE COMUNICACIÓN
┌─────────────────────────────────────────────────────────────────────────────┐
│  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐          │
│  │  USB Serial      │  │  Serial2         │  │  Protocolo       │          │
│  │  (PC ↔ ESP32)    │  │  (Nextion)       │  │  Binario         │          │
│  │  115200 baud     │  │  115200 baud     │  │  Estructurado    │          │
│  └──────────────────┘  └──────────────────┘  └──────────────────┘          │
└─────────────────────────────────────────────────────────────────────────────┘
                                  │
                    CAPA DE CONTROL (ESP32 FIRMWARE)
┌─────────────────────────────────────────────────────────────────────────────┐
│                                                                             │
│  ┌───────────────────────────────┐  ┌───────────────────────────────┐      │
│  │           CORE 0              │  │           CORE 1              │      │
│  │      (UI + Comunicación)      │  │    (Generación Tiempo Real)   │      │
│  │                               │  │                               │      │
│  │  • CommandHandler             │  │  • SignalEngine               │      │
│  │  • UIManager                  │  │  • BufferManager              │      │
│  │  • ParamController            │  │  • TimerISR (IRAM)            │      │
│  │  • TelemetryStreamer          │  │  • DACDriver                  │      │
│  └───────────────────────────────┘  └───────────────────────────────┘      │
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                       MODELOS DE SEÑAL                              │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐                 │   │
│  │  │  ECGModel   │  │  EMGModel   │  │  PPGModel   │                 │   │
│  │  │  McSharry   │  │  MotorUnit  │  │  Gaussian   │                 │   │
│  │  │  RK4 + HRV  │  │  Henneman   │  │  HRV + RSA  │                 │   │
│  │  └─────────────┘  └─────────────┘  └─────────────┘                 │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
                                  │
                           CAPA DE HARDWARE
┌─────────────────────────────────────────────────────────────────────────────┐
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐           │
│  │ ESP32       │ │ Nextion     │ │ DAC Buffer  │ │ LED RGB     │           │
│  │ WROOM-32    │ │ NX4024T032  │ │ GPIO25      │ │ Status      │           │
│  │ 240MHz      │ │ 320x240     │ │ 0-3.3V      │ │             │           │
│  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘           │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 3.2 Distribución de Tareas por Core

| Core | Tareas | Prioridad | Justificación |
|------|--------|-----------|---------------|
| **Core 0** | UI, Serial, Nextion, Parámetros | Media | No crítico en tiempo |
| **Core 1** | Generación señal, Buffer, ISR, DAC | Alta | Tiempo real crítico |

### 3.3 Flujo de Datos

```
[Modelo] → [Buffer Circular 2KB] → [Timer ISR 1kHz] → [DAC GPIO25] → [Salida BNC]
              ↑                           ↓
        [ParamController]          [TelemetryStreamer]
              ↑                           ↓
        [Nextion/Serial]           [App Desktop/CSV]
```

---

## 4. Especificación de Hardware

### 4.1 Componentes Principales

| Componente | Especificación | Función |
|------------|----------------|---------|
| **MCU** | ESP32-WROOM-32 | Procesamiento dual-core |
| **Pantalla** | Nextion NX4024T032 (3.2", 320x240) | Interfaz táctil |
| **DAC** | Interno ESP32, GPIO25 | Salida analógica |
| **LED RGB** | Cátodo/Ánodo común | Indicador de estado |

### 4.2 Pinout ESP32

| Pin | Función | Descripción |
|-----|---------|-------------|
| GPIO25 | DAC1 | Salida de señal principal |
| GPIO26 | DAC2 | Salida secundaria (opcional) |
| GPIO16 | RX2 | Recepción Nextion |
| GPIO17 | TX2 | Transmisión Nextion |
| GPIO4 | LED_R | LED RGB - Rojo |
| GPIO5 | LED_G | LED RGB - Verde |
| GPIO18 | LED_B | LED RGB - Azul |
| GPIO2 | LED_STATUS | LED interno |

### 4.3 Especificaciones DAC

| Parámetro | Valor | Nota |
|-----------|-------|------|
| Resolución | 8 bits | 256 niveles (0-255) |
| Rango de voltaje | 0 - 3.3V | Sin amplificación |
| Resolución de voltaje | 12.9 mV/paso | 3.3V / 256 |
| Frecuencia de muestreo | 1000 Hz | Configurable |
| Tiempo entre muestras | 1 ms | 1/1000 Hz |

### 4.4 Estados del LED RGB

| Estado | Color | Descripción |
|--------|-------|-------------|
| OFF | Rojo | Sistema apagado o error |
| READY | Verde | Listo para usar |
| SIMULATING | Azul | Generando señal |
| PAUSED | Cyan | Señal pausada |
| ERROR | Rojo parpadeante | Error del sistema |

---

## 5. Modelos de Señales

### 5.1 Modelo ECG - McSharry

#### 5.1.1 Fundamento Matemático

Basado en: McSharry, P.E., et al. (2003). *"A Dynamical Model for Generating Synthetic Electrocardiogram Signals"*. IEEE TBME.

**Sistema de ecuaciones diferenciales:**

```
dx/dt = α·x - ω·y
dy/dt = α·y + ω·x
dz/dt = -Σ(aᵢ·Δθᵢ·exp(-Δθᵢ²/2bᵢ²)) - (z - z₀)

donde:
- (x, y) = punto en círculo unitario
- z = señal ECG de salida
- ω = 2π/RR = velocidad angular
- aᵢ, bᵢ, θᵢ = parámetros de ondas P, Q, R, S, T
```

**Integración numérica:** Runge-Kutta 4to orden (RK4)

#### 5.1.2 Condiciones ECG

| Condición | HR Default | Características | Modificaciones al Modelo |
|-----------|------------|-----------------|--------------------------|
| Normal | 75 BPM | Ritmo sinusal regular | Parámetros estándar |
| Taquicardia | 130 BPM | HR > 100 BPM | ω aumentado |
| Bradicardia | 45 BPM | HR < 60 BPM | ω reducido |
| Fib. Auricular | 110 BPM | Sin onda P, RR irregular | aₚ ≈ 0, alta variabilidad RR |
| Fib. Ventricular | ~300 BPM* | Ritmo caótico sin latidos coordinados | Ondas caóticas, sin estructura PQRST |
| PVC | 75 BPM | Latidos prematuros | Cada N latidos: RR corto + pausa |
| Bloqueo Rama | 70 BPM | QRS ancho > 120ms | bQRS aumentado |
| ST Elevación | 80 BPM | Segmento ST elevado | Offset positivo post-S |
| ST Depresión | 90 BPM | Segmento ST deprimido | Offset negativo post-S |

#### 5.1.3 Variabilidad (HRV)

```
RR(n) = RR_mean + σ·N(0,1) + irregularidad

donde:
- RR_mean = 60000/HR (ms)
- σ = desviación estándar (típico 4-8 ms para normal)
- N(0,1) = distribución normal estándar (Box-Muller)
- irregularidad = factor adicional para patologías
```

### 5.2 Modelo EMG - Motor Units

#### 5.2.1 Fundamento

Basado en:
- Henneman, E. (1957). Principio de reclutamiento por tamaño
- De Luca, C.J. (1997). EMG de superficie

**Componentes del modelo:**
- 100 Motor Units (MUs) con características individuales
- Reclutamiento ordenado por umbral (Henneman)
- MUAPs bi-exponenciales con variabilidad ISI

#### 5.2.2 Ecuaciones

```
Umbral de reclutamiento:
  threshold(i) = RR_RANGE · exp(ln(RR_RANGE)·i/N) / RR_RANGE

Firing rate:
  FR(i) = FR_min + slope·(excitation - threshold(i))·100

ISI con variabilidad:
  ISI = mean_ISI ± 20% (gaussiano)

Señal EMG:
  EMG(t) = Σ MU_activas [ MUAP(t - t_spike) · amplitud ]
```

#### 5.2.3 Condiciones EMG

| Condición | Excitación | MUs Activas | Características |
|-----------|------------|-------------|-----------------|
| Reposo (0%) | 0.00 | 0 | Solo ruido de fondo |
| Leve (20%) | 0.20 | ~20 | Tareas de precisión |
| Moderada (50%) | 0.50 | ~50 | Sostener objetos |
| Fuerte (80%) | 0.80 | ~80 | Levantar peso |
| Máxima (100%) | 1.00 | 100 | Esfuerzo máximo |
| Temblor | 0.30 | ~30 | Oscilación 4-6 Hz (Parkinson) |
| Miopatía | 0.50 | ~70 | MUAPs pequeños y cortos |
| Neuropatía | 0.50 | ~30 | MUAPs gigantes, 40% denervación |
| Fasciculación | 0.00 | Aleatorio | Disparos espontáneos |
| Fatiga | 0.60→0.40 | Decreciente | Reducción progresiva |

### 5.3 Modelo PPG - Doble Gaussiana

#### 5.3.1 Fundamento

Basado en:
- Allen, J. (2007). PPG en medición fisiológica
- Task Force ESC (1996). Estándares HRV

**Ecuación del pulso:**

```
PPG(t) = DC + AC(t)

AC(t) = A₁·exp(-(t-μ₁)²/2σ₁²)    ← Pico sistólico
      + A₂·exp(-(t-μ₂)²/2σ₂²)    ← Pico diastólico  
      - D·exp(-(t-μd)²/2σd²)     ← Muesca dicrótica

donde t está normalizado al intervalo RR (0 a 1)
```

#### 5.3.2 Parámetros Normales

| Parámetro | Valor | Descripción |
|-----------|-------|-------------|
| A₁ | 1.0 | Amplitud sistólica (referencia) |
| μ₁ | 0.15 | Posición pico sistólico (15% RR) |
| σ₁ | 0.055 | Ancho sistólico |
| A₂ | 0.40 | Amplitud diastólica (40% de A₁) |
| μ₂ | 0.40 | Posición pico diastólico |
| σ₂ | 0.10 | Ancho diastólico |
| D | 0.25 | Profundidad muesca dicrótica |
| μd | 0.30 | Posición muesca |
| σd | 0.02 | Ancho muesca |

#### 5.3.3 Condiciones PPG

| Condición | HR | PI | Modificaciones |
|-----------|----|----|----------------|
| Normal | 75 | 5% | Parámetros estándar |
| Arritmia | 75 | 4% | Alta variabilidad RR (15% ectópicos) |
| Perfusión Débil | 110 | 0.8% | Amplitud muy reducida |
| Perfusión Fuerte | 70 | 12% | Amplitud aumentada |
| Vasoconstricción | 85 | 4% | Muesca dicrótica prominente |
| Artefacto Mov. | 75 | 5% | Ruido alto, spikes aleatorios |
| SpO2 Bajo | 100 | 2.5% | Taquicardia compensatoria |

---

## 6. Sistema de Parametrización

### 6.1 Problema de Sincronización

**El problema:** Los modelos generan señales dinámicas basadas en estado interno. Si el usuario cambia un parámetro (ej: HR) a mitad de un ciclo, el modelo puede generar una señal distorsionada.

**La solución:** Clasificar parámetros en dos tipos y aplicarlos de forma diferente.

### 6.2 Clasificación de Parámetros

#### Tipo A: Aplicación Inmediata

Parámetros que NO afectan el timing del ciclo actual.

| Señal | Parámetros Inmediatos |
|-------|----------------------|
| ECG | Ruido, Amplitud QRS, Amplitud P, Amplitud T, ST Shift |
| EMG | Ruido |
| PPG | Ruido, Muesca Dicrótica, Amplitud |

#### Tipo B: Aplicación en Próximo Ciclo

Parámetros que SÍ afectan el timing y deben esperar.

| Señal | Parámetros Diferidos |
|-------|---------------------|
| ECG | Heart Rate, Condición, Intervalos PR/QT |
| EMG | Nivel de Excitación, Condición |
| PPG | Heart Rate, Condición |

### 6.3 Implementación

```cpp
// Estructura para parámetros pendientes
struct PendingParams {
    bool hasPending;           // ¿Hay cambio pendiente?
    ECGParameters params;      // Nuevos parámetros
    unsigned long requestTime; // Cuándo se solicitó
};

// En el modelo, al detectar fin de ciclo:
void onCycleComplete() {
    if (pendingParams.hasPending) {
        applyParameters(pendingParams.params);
        pendingParams.hasPending = false;
    }
}
```

### 6.4 Rangos Fisiológicos por Condición

Los parámetros de cada condición están definidos como **RANGOS** basados en literatura clínica validada (ver Sección 13).

> **NOTA IMPORTANTE:** El comportamiento específico de cada señal (inicio, variabilidad, deriva temporal) se definirá al revisar cada modelo individualmente. Cada tipo de señal (ECG, EMG, PPG) tiene características dinámicas diferentes que requieren análisis particular.

#### ECG - Rangos Fisiológicos por Condición (Validados con Literatura)

| Condición | Rango HR (BPM) | Referencia Clínica | Notas Modelo |
|-----------|----------------|--------------------| -------------|
| Normal | **60-100** | AHA Guidelines | Ritmo sinusal regular |
| Taquicardia Sinusal | **100-180** | StatPearls, ECGpedia | Típico 100-150 BPM |
| Bradicardia Sinusal | **30-59** | ACC/AHA 2018 | En atletas puede ser 30-50 |
| Fib. Auricular | **60-180** | BMJ, ACC/AHA 2023 | Resp. ventricular 100-180 típica sin tx |
| Fib. Ventricular* | **150-500** | LITFL, Mayo Clinic | Caótico, sin latidos coordinados |
| PVC | **50-120** | Lógica clínica | Ritmo base normal + extrasístoles |
| Bloqueo Rama | **40-100** | Fisiología estándar | QRS ancho >120ms |
| ST Elevación | **50-110** | AHA | Posible IAM, ST +0.1 a +0.3 mV |
| ST Depresión | **50-150** | AHA | Isquemia, ST -0.1 a -0.3 mV |

> **\*Nota sobre Fibrilación Ventricular:** La FV NO tiene "latidos" regulares medibles en BPM tradicional. Es un ritmo caótico sin estructura PQRST. El valor ~300 representa la "frecuencia dominante" del ventrículo fibrilando. El modelo simula la **apariencia visual** de FV para fines educativos, no la fisiología real (que causa paro cardíaco inmediato).

#### EMG - Rangos por Nivel de Contracción

**Nota:** Los "niveles de excitación" son parámetros del modelo que representan el % de Contracción Voluntaria Máxima (MVC). La señal inicia en un punto aleatorio dentro del rango y varía naturalmente.

| Condición | Rango Excitación | Rango Amplitud | Freq Disparo MU | Referencia |
|-----------|------------------|----------------|-----------------|------------|
| Reposo | **0.0 - 0.1** | <50 μV | - | Sin contracción voluntaria |
| Leve | **0.1 - 0.3** | 100-200 μV | 8-12 Hz | ~20% MVC |
| Moderada | **0.3 - 0.6** | 300-500 μV | 12-16 Hz | ~50% MVC |
| Fuerte | **0.6 - 0.9** | 600-1000 μV | 16-20 Hz | ~80% MVC |
| Máxima | **0.8 - 1.0** | >1000 μV | 20-25 Hz | 100% MVC |
| Temblor | **0.1 - 0.5** | Variable | 4-6 Hz | Parkinson típico |
| Miopatía | **0.1 - 0.4** | Reducida | MUAPs pequeños | Enfermedad muscular |
| Neuropatía | **0.3 - 1.0** | Aumentada | MUAPs gigantes | Denervación parcial |
| Fasciculación | **0.0 - 0.3** | Esporádica | Aleatorio | Disparos espontáneos |
| Fatiga | **0.2 - 0.8** (decrece) | Decreciente | Decreciente | Simulación temporal |

#### PPG - Rangos Fisiológicos por Condición (Validados con Literatura)

| Condición | Rango HR (BPM) | Rango PI (%) | Referencia PI | Notas |
|-----------|----------------|--------------|---------------|-------|
| Normal | **60-100** | **2-10%** | Lima 2002 | Perfusión normal en dedos |
| Arritmia | **50-150** | **2-8%** | - | Alta variabilidad RR |
| Perfusión Débil | **90-140** | **0.3-1%** | Lima 2002: <1% bajo | Taquicardia compensatoria |
| Perfusión Fuerte | **50-90** | **10-20%** | >10% fuerte | Vasodilatación |
| Vasoconstricción | **70-110** | **1-5%** | - | Muesca dicrótica prominente |
| Artefacto Mov. | **60-100** | **2-10%** | - | Ruido alto, spikes |
| SpO2 Bajo | **80-130** | **0.5-3%** | - | Taquicardia + PI reducido |

### 6.5 Flujo de Usuario para Parametrización

```
1. Usuario está en pantalla SIMULACIÓN
   → Señal corriendo con valores actuales
   → Métricas mostrando valores REALES (cambian cada latido)
   → Config mostrando valores CONFIGURADOS (fijos)

2. Usuario presiona [⚙ Parámetros]
   → Se abre pantalla PARÁMETROS
   → Sliders muestran valores CONFIGURADOS actuales
   → Señal SIGUE corriendo en background

3. Usuario ajusta sliders
   → Los rangos están limitados por la condición
   → Valores fuera de rango no son posibles

4. Usuario presiona [✓ Aplicar]
   → Parámetros Tipo A: Se aplican inmediatamente
   → Parámetros Tipo B: Se marcan como "pendientes"
   → Sistema muestra: "Cambios aplicados (HR efectivo en próximo latido)"
   → Vuelve a pantalla SIMULACIÓN

5. En el próximo ciclo/latido:
   → Parámetros pendientes se aplican
   → Señal refleja los nuevos valores

ALTERNATIVAS:
- [✗ Cancelar]: Descarta cambios, vuelve a SIMULACIÓN
- [↺ Restablecer]: Pone sliders en valores DEFAULT de la condición
```

---

## 7. Interfaz de Usuario (Nextion)

### 7.1 Mapa de Páginas

```
Página 0: SPLASH
    ↓
Página 1: SELECT_SIGNAL
    ↓
Página 2: SELECT_CONDITION
    ↓
Página 3: SIMULATION ←→ Página 4: PARAMETERS
```

### 7.2 Diseño de Páginas

#### Página 0: Splash (2 segundos)

```
┌─────────────────────────────────────────┐
│                                         │
│      ╔═══════════════════════════╗     │
│      ║  BioSimulator Pro  ║     │
│      ║          v3.0             ║     │
│      ╚═══════════════════════════╝     │
│                                         │
│           [  INICIAR  ]                │
│                                         │
│     Lab. Neurociencias y Bioingeniería │
│                                         │
└─────────────────────────────────────────┘

Elementos Nextion:
- t0: Título (texto)
- t1: Versión (texto)
- t2: Subtítulo (texto)
- b0: Botón "INICIAR" → page 1
```

#### Página 1: Selección de Señal

```
┌─────────────────────────────────────────┐
│  Seleccione tipo de señal:              │
├─────────────────────────────────────────┤
│                                         │
│   ┌───────────┐                         │
│   │    ECG    │  Electrocardiograma     │
│   │    ♥      │  9 condiciones          │
│   └───────────┘                         │
│                                         │
│   ┌───────────┐                         │
│   │    EMG    │  Electromiografía       │
│   │    💪     │  10 condiciones         │
│   └───────────┘                         │
│                                         │
│   ┌───────────┐                         │
│   │    PPG    │  Fotopletismografía     │
│   │    🩸     │  7 condiciones          │
│   └───────────┘                         │
│                                         │
└─────────────────────────────────────────┘

Elementos Nextion:
- t0: Título
- b0: Botón ECG → envía "ECG", page 2
- b1: Botón EMG → envía "EMG", page 2
- b2: Botón PPG → envía "PPG", page 2
```

#### Página 2: Selección de Condición

```
┌─────────────────────────────────────────┐
│  ECG - Condición:               [←]    │
├─────────────────────────────────────────┤
│                                         │
│  ┌─────────────┐  ┌─────────────┐      │
│  │   Normal    │  │ Taquicardia │      │
│  └─────────────┘  └─────────────┘      │
│                                         │
│  ┌─────────────┐  ┌─────────────┐      │
│  │ Bradicardia │  │ Fib. Auric. │      │
│  └─────────────┘  └─────────────┘      │
│                                         │
│  ┌─────────────┐  ┌─────────────┐      │
│  │    PVC      │  │ Bloq. Rama  │      │
│  └─────────────┘  └─────────────┘      │
│                                         │
│  ┌─────────────┐  ┌─────────────┐      │
│  │ ST Elevado  │  │ ST Deprimido│      │
│  └─────────────┘  └─────────────┘      │
│                                         │
└─────────────────────────────────────────┘

Elementos Nextion:
- t0: Título dinámico ("ECG - Condición:", "EMG - Condición:", etc.)
- b0-b8: Botones de condición (visibilidad según señal)
- bBack: Botón atrás → page 1
```

#### Página 3: Simulación (Principal)

```
┌─────────────────────────────────────────┐
│ ECG Normal         ● RUN   [⚙][⏸][⏹][←]│
├─────────────────────────────────────────┤
│ +1.0mV ┬────────────────────────────┐  │
│        │    ╭╮          ╭╮          │  │
│  0.0mV │───╯╰──────────╯╰──────────│  │
│        │       ╭─╮          ╭─╮     │  │
│ -0.5mV │───────╯ ╰──────────╯ ╰────│  │
│        └────────────────────────────┘  │
│         0s      1s      2s      3s     │
├─────────────────────────────────────────┤
│ ┌── MEDIDO ───────┐ ┌── CONFIG ──────┐ │
│ │ HR:  73 BPM     │ │ HR:   75 BPM   │ │
│ │ RR:  822 ms     │ │ Ruido: 0.05    │ │
│ │ QRS: 98 ms      │ │ Amp:   1.0     │ │
│ └─────────────────┘ └────────────────┘ │
└─────────────────────────────────────────┘

Elementos Nextion:
- t0: Título (señal + condición)
- tStatus: Estado (RUN/PAUSED/STOPPED)
- s0: Waveform component (320 width)
- tHR, tRR, tQRS: Métricas medidas (actualizan cada latido)
- tHRcfg, tNoise, tAmp: Config actual (fijos hasta cambio)
- bParams: Botón parámetros → page 4
- bPause: Botón pausa/play
- bStop: Botón stop
- bBack: Botón atrás → page 1
```

#### Página 4: Parámetros

```
┌─────────────────────────────────────────┐
│ Parámetros - ECG Normal           [←]  │
├─────────────────────────────────────────┤
│                                         │
│ Heart Rate (60-100 BPM):               │
│ [60]════════════●════════════[100]     │
│                 75 BPM                  │
│                                         │
│ Amplitud QRS (0.5-2.0):                │
│ [0.5]════●══════════════════[2.0]      │
│          1.0 x                          │
│                                         │
│ Nivel Ruido (0.0-0.5):                 │
│ [0.0]●══════════════════════[0.5]      │
│      0.05                               │
│                                         │
│ ST Shift (mV):                         │
│ [-0.1]═════════●════════════[0.1]      │
│                0.0 mV                   │
│                                         │
├─────────────────────────────────────────┤
│ [✓ Aplicar] [✗ Cancelar] [↺ Defaults] │
└─────────────────────────────────────────┘

Elementos Nextion:
- t0: Título
- h0: Slider HR (minval, maxval dinámicos según condición)
- h1: Slider Amplitud
- h2: Slider Ruido
- h3: Slider ST (solo ECG)
- tVal0-3: Valores actuales de sliders
- bApply: Aplicar → envía valores, page 3
- bCancel: Cancelar → page 3 (sin cambios)
- bDefaults: Restablecer → carga defaults
```

### 7.3 Comunicación ESP32 ↔ Nextion

#### Comandos ESP32 → Nextion

```cpp
// Cambiar página
nextion.sendCommand("page 3");

// Actualizar texto
nextion.sendCommand("t0.txt=\"ECG Normal\"");

// Actualizar número
nextion.sendCommand("n0.val=75");

// Actualizar waveform (añadir punto)
nextion.sendCommand("add 1,0,128");  // canal 1, componente 0, valor 128

// Configurar slider
nextion.sendCommand("h0.minval=60");
nextion.sendCommand("h0.maxval=100");
nextion.sendCommand("h0.val=75");
```

#### Eventos Nextion → ESP32

```
Formato: 0x65 [page] [component] [event] 0xFF 0xFF 0xFF

Ejemplos:
- Botón presionado: 65 01 02 01 FF FF FF (page 1, comp 2, touch)
- Slider cambió: 65 04 00 [valor] FF FF FF
```

---

## 8. Aplicación de Escritorio

### 8.1 Funcionalidades

| Función | Descripción |
|---------|-------------|
| **Conexión Serial** | Auto-detección de puerto, 115200 baud |
| **Visualización** | Gráfica en tiempo real (PyQtGraph) |
| **Control** | Panel de parámetros sincronizado con Nextion |
| **Guardado** | Exportación CSV con metadatos |
| **Métricas** | Cálculo de HR, RR, SDNN en tiempo real |

### 8.2 Formato CSV

```csv
# BioSimulator Pro - Data Export
# Signal Type: ECG
# Condition: NORMAL
# Date: 2025-12-04 18:00:00
# Sample Rate: 1000 Hz
# Duration: 60.0 s
# Parameters: HR=75, QRS_Amp=1.0, P_Amp=1.0, Noise=0.05, ST_Shift=0.0
#
timestamp_ms,sample_value,dac_value,heart_rate,rr_interval_ms
0,0.125,145,75.2,798
1,0.132,147,75.2,798
2,0.141,149,75.1,799
...
```

### 8.3 Estructura de la Aplicación

```
desktop_app/
├── src/
│   ├── main.py              # Punto de entrada
│   ├── serial_handler.py    # Comunicación serial
│   ├── signal_viewer.py     # Visualización gráfica
│   ├── data_logger.py       # Guardado CSV
│   ├── param_panel.py       # Panel de parámetros
│   └── ui/
│       └── main_window.py   # Interfaz principal
├── requirements.txt
│   # pyserial
│   # pyqt5
│   # pyqtgraph
│   # numpy
└── README.md
```

---

## 9. Protocolo de Comunicación

### 9.1 Protocolo Binario (ESP32 ↔ PC)

```cpp
// Estructura de paquete
typedef struct __attribute__((packed)) {
    uint8_t  header;        // 0xAA
    uint8_t  cmd;           // Comando
    uint8_t  signalType;    // 1=ECG, 2=EMG, 3=PPG
    uint16_t dataLen;       // Longitud de datos
    uint8_t  data[256];     // Payload
    uint8_t  checksum;      // XOR de todos los bytes
} Packet;

// Comandos
#define CMD_START_SIGNAL     0x01
#define CMD_STOP_SIGNAL      0x02
#define CMD_PAUSE_SIGNAL     0x03
#define CMD_RESUME_SIGNAL    0x04
#define CMD_SET_PARAMS       0x10
#define CMD_GET_PARAMS       0x11
#define CMD_GET_DEFAULTS     0x12
#define CMD_STREAM_DATA      0x20  // ESP32 → PC (streaming)
#define CMD_GET_METRICS      0x21
#define CMD_ACK              0xF0
#define CMD_ERROR            0xFF
```

### 9.2 Formato de Streaming

```
Para minimizar overhead, streaming usa formato compacto:

[0xBB] [sample_high] [sample_low] [flags]

donde:
- 0xBB = header de muestra
- sample = valor DAC 0-255 (8-bit, pero empaquetado en 16 por si se expande)
- flags = bit 0: nuevo latido detectado
```

---

## 10. Validación y Métricas

### 10.1 Fuentes de Datos de Referencia

| Señal | Base de Datos | Descripción |
|-------|---------------|-------------|
| ECG | MIT-BIH Arrhythmia DB | 48 registros con arritmias anotadas |
| ECG | MIT-BIH Normal Sinus | 18 registros de sujetos sanos |
| ECG | PTB Diagnostic ECG | 549 registros con diagnósticos |
| EMG | Ninapro Database | EMG de mano, múltiples gestos |
| PPG | MIMIC-III Waveform | Miles de registros de UCI |

### 10.2 Métricas de Validación

#### Métricas Morfológicas

| Métrica | Descripción | Umbral Aceptable |
|---------|-------------|------------------|
| Error RR | Diferencia en intervalo RR | < 10% |
| Error PR | Diferencia en intervalo PR | < 10% |
| Error QRS | Diferencia en duración QRS | < 10% |
| Error QT | Diferencia en intervalo QT | < 10% |
| Ratio P/R | Relación amplitudes | Dentro de rango fisiológico |
| Ratio T/R | Relación amplitudes | Dentro de rango fisiológico |

#### Métricas Estadísticas

| Métrica | Descripción | Umbral Aceptable |
|---------|-------------|------------------|
| Correlación (r) | Pearson entre señales | > 0.80 |
| RMSE | Error cuadrático medio | < 0.15 (normalizado) |
| DTW | Dynamic Time Warping | < 100 |

#### Métricas Frecuenciales

| Métrica | Descripción | Umbral Aceptable |
|---------|-------------|------------------|
| Coherencia espectral | Similitud en frecuencia | > 0.70 |
| Potencia LF/HF | Ratio de bandas HRV | Dentro de rango |

### 10.3 Proceso de Validación

```
1. RECOLECCIÓN
   ├── Descargar datasets de PhysioNet
   └── Generar señales equivalentes con simulador

2. PREPROCESAMIENTO
   ├── Filtrar (0.5-40 Hz para ECG)
   ├── Normalizar (0-1)
   └── Segmentar latidos

3. CÁLCULO DE MÉTRICAS
   ├── Correlación
   ├── RMSE
   ├── DTW
   ├── Intervalos
   └── Espectro

4. ANÁLISIS
   ├── Media ± SD por métrica
   ├── Test estadísticos
   └── Gráficas comparativas

5. REPORTE
   ├── Tablas de resultados
   ├── Gráficas Bland-Altman
   └── Conclusiones y limitaciones
```

### 10.4 Criterios de Aceptación

| Resultado | Criterio |
|-----------|----------|
| **VALIDADO** | Todas las métricas en "Aceptable" o mejor |
| **PARCIAL** | Algunas métricas fallan pero con justificación documentada |
| **FALLIDO** | Métricas críticas (r, RMSE) inaceptables |

---

## 11. Estructura de Código

### 11.1 Estructura de Carpetas

```
BioSimulator_Pro/
│
├── docs/
│   ├── PROJECT_PLAN.md          ← Este documento
│   ├── ARCHITECTURE.md
│   ├── USER_MANUAL.md
│   └── VALIDATION_REPORT.md
│
├── firmware/
│   ├── include/
│   │   ├── config.h              # Configuración global
│   │   ├── pins.h                # Definición de pines
│   │   │
│   │   ├── models/
│   │   │   ├── ecg_model.h
│   │   │   ├── emg_model.h
│   │   │   └── ppg_model.h
│   │   │
│   │   ├── core/
│   │   │   ├── signal_engine.h
│   │   │   ├── buffer_manager.h
│   │   │   ├── param_controller.h
│   │   │   └── state_machine.h
│   │   │
│   │   ├── comm/
│   │   │   ├── protocol.h
│   │   │   ├── nextion_driver.h
│   │   │   └── serial_handler.h
│   │   │
│   │   └── data/
│   │       ├── param_defaults.h
│   │       ├── param_limits.h
│   │       └── signal_types.h
│   │
│   ├── src/
│   │   ├── main.cpp
│   │   ├── models/
│   │   ├── core/
│   │   ├── comm/
│   │   └── data/
│   │
│   ├── test/
│   │   └── ...
│   │
│   └── platformio.ini
│
├── nextion_hmi/
│   ├── BioSimulator.HMI
│   ├── fonts/
│   └── images/
│
├── desktop_app/
│   ├── src/
│   ├── requirements.txt
│   └── README.md
│
├── hardware/
│   ├── pcb/
│   ├── enclosure/
│   └── datasheets/
│
├── validation/
│   ├── scripts/
│   ├── datasets/
│   └── reports/
│
└── samples/
    ├── ecg/
    ├── emg/
    └── ppg/
```

### 11.2 Archivos Clave a Modificar/Crear

| Archivo | Acción | Prioridad |
|---------|--------|-----------|
| `core/param_controller.h/cpp` | CREAR | Alta |
| `core/state_machine.h/cpp` | CREAR | Alta |
| `comm/nextion_driver.h/cpp` | MODIFICAR | Alta |
| `data/param_defaults.h` | CREAR | Alta |
| `models/ecg_model.cpp` | MODIFICAR (pending params) | Alta |
| `models/emg_model.cpp` | MODIFICAR (pending params) | Alta |
| `models/ppg_model.cpp` | MODIFICAR (pending params) | Alta |
| `main.cpp` | MODIFICAR (flujo estados) | Alta |

---

## 12. Plan de Implementación

### 12.1 Prioridades (para tiempo limitado)

**CRÍTICO (Día 1):**
1. Hacer funcionar waveform básico en Nextion
2. Mostrar señal ECG/EMG/PPG en pantalla

**IMPORTANTE (Día 2-3):**
3. Implementar navegación de páginas
4. Añadir métricas básicas en pantalla
5. Implementar sliders de parámetros

**DESEABLE (Después):**
6. Sistema completo de parametrización diferida
7. Aplicación de escritorio
8. Validación formal

### 12.2 Tareas Día 1

```
□ 1. Verificar comunicación Serial2 con Nextion
     - Test envío de comandos básicos
     - Confirmar baud rate (115200)

□ 2. Crear página básica con waveform en Nextion Editor
     - Añadir componente Waveform (s0)
     - Configurar tamaño y colores

□ 3. Implementar envío de datos al waveform
     - En el loop/tarea, enviar valores DAC al waveform
     - Comando: add s0.id,0,valor

□ 4. Probar con las 3 señales (ECG, EMG, PPG)
     - Verificar que se visualizan correctamente
```

### 12.3 Código Mínimo para Waveform

```cpp
// En nextion_driver.cpp

void NextionDriver::sendWaveformPoint(uint8_t componentId, uint8_t channel, uint8_t value) {
    // Comando: add [component_id],[channel],[value]
    char cmd[20];
    sprintf(cmd, "add %d,%d,%d", componentId, channel, value);
    sendCommand(cmd);
}

// En el loop principal o tarea de UI
void updateDisplay() {
    static unsigned long lastUpdate = 0;
    
    // Actualizar waveform cada 10ms (100 Hz de refresh)
    if (millis() - lastUpdate >= 10) {
        uint8_t dacValue = signalGen->getLastDACValue();
        
        // Escalar de 0-255 a 0-altura_waveform (ej: 0-100)
        uint8_t waveValue = map(dacValue, 0, 255, 0, 100);
        
        nextion.sendWaveformPoint(1, 0, waveValue);  // s0 tiene id=1
        
        lastUpdate = millis();
    }
}
```

---

## 13. Referencias Científicas

### 13.1 Modelos Matemáticos de Señales

| Referencia | Aplicación | DOI/URL |
|------------|------------|---------|
| McSharry, P.E. et al. (2003) | Modelo ECG dinámico | DOI: 10.1109/TBME.2003.808805 |
| Henneman, E. (1957) | Principio de reclutamiento por tamaño (MU) | Science |
| De Luca, C.J. (1997) | Surface EMG: Detection and Recording | J. Applied Biomechanics |
| Allen, J. (2007) | PPG in clinical physiological measurement | DOI: 10.1088/0967-3334/28/3/R01 |
| Task Force ESC/NASPE (1996) | Estándares HRV | Circulation 93:1043-1065 |

### 13.2 Guías Clínicas para Parámetros ECG (Validadas)

| Referencia | Aplicación | DOI/URL |
|------------|------------|---------|
| **American Heart Association (2023)** | HR normal: 60-100 BPM | https://www.heart.org/ |
| **ACC/AHA/HRS Guideline on Bradycardia (2018)** | Bradicardia: <60 BPM | DOI: 10.1161/CIR.0000000000000628 |
| **Sinus Tachycardia Expert Review (2021)** | Taquicardia: >100 BPM, típico 100-180 BPM | DOI: 10.1161/CIRCEP.121.007960 |
| **ACC/AHA/ACCP/HRS AFib Guideline (2023)** | Fib. Auricular: resp. ventricular 100-180 BPM | DOI: 10.1161/CIR.0000000000001193 |
| **LITFL - Ventricular Fibrillation** | FV: ritmo caótico hasta 500 BPM | https://litfl.com/ventricular-fibrillation-vf-ecg-library/ |
| **Mayo Clinic - Ventricular Fibrillation** | FV: causa paro cardíaco inmediato | https://www.mayoclinic.org/diseases-conditions/ventricular-fibrillation/ |
| **StatPearls - Sinus Tachycardia** | Taquicardia sinusal 100-150 BPM típico | NCBI/StatPearls |
| **ECGpedia / ECG Learning Center** | Rangos y morfologías ECG | https://ecgwaves.com/ |

### 13.3 Referencias EMG

| Referencia | Aplicación | Valores |
|------------|------------|---------|
| **De Luca, C.J. (1997)** | Frecuencias de disparo MU | 15-20 Hz activación media |
| **Henneman, E. (1957)** | Principio de tamaño | Reclutamiento ordenado de MUs |
| **Merletti & Parker (2004)** | Electromyography: Physiology | Rangos de amplitud por %MVC |

### 13.4 Referencias PPG

| Referencia | Aplicación | DOI/URL |
|------------|------------|---------|
| **Lima, A.P. et al. (2002)** | Perfusion Index: 2-10% normal, <0.3% crítico | DOI: 10.1007/s00134-002-1387-9 |
| **Elgendi, M. et al. (2019)** | PPG Signal Quality | IEEE Reviews in Biomedical Engineering |
| **WHO Pulse Oximetry Manual (2011)** | SpO2: normal 95-100%, hipoxia <90% | WHO |

### 13.5 Bases de Datos para Validación

| Base de Datos | Señal | Descripción | URL |
|---------------|-------|-------------|-----|
| **MIT-BIH Arrhythmia DB** | ECG | 48 registros con arritmias anotadas | physionet.org |
| **MIT-BIH Normal Sinus** | ECG | 18 registros de sujetos sanos | physionet.org |
| **PTB Diagnostic ECG** | ECG | 549 registros con diagnósticos | physionet.org |
| **Ninapro Database** | EMG | EMG de mano, múltiples gestos | ninapro.hevs.ch |
| **MIMIC-III Waveform** | PPG | Miles de registros de UCI | physionet.org |

### 13.6 Métricas de Validación

| Referencia | Aplicación |
|------------|------------|
| **Pan-Tompkins (1985)** | Algoritmo detección QRS |
| **Bland-Altman (1986)** | Análisis de concordancia entre métodos |
| **DTW - Dynamic Time Warping** | Comparación de señales temporales |

---

## 14. Limitaciones Conocidas

### 14.1 Limitaciones de Hardware

| Limitación | Impacto | Mitigación |
|------------|---------|------------|
| DAC 8-bit (256 niveles) | Resolución 12.9 mV/paso | Documentar en reporte |
| Sin PSRAM | Buffer limitado a 2KB | Suficiente para 2s de señal |
| Nextion 320x240 | Waveform de ~300 puntos visible | Scrolling automático |

### 14.2 Limitaciones de Modelo

| Limitación | Impacto | Mitigación |
|------------|---------|------------|
| ECG solo Lead II | No multi-derivación | Futuro: DAC externo |
| EMG 1kHz | Nyquist para 500Hz max | Documentar como superficie-EMG |
| SpO2 simulado | No es cálculo real | Documentar como aproximación |
| Sin ruido de línea 50/60Hz | Menos realista | Podría añadirse |

### 14.3 Limitaciones de Validación

| Limitación | Impacto | Mitigación |
|------------|---------|------------|
| Comparación morfológica | No prueba uso clínico | Clarificar propósito educativo |
| Sin validación con pacientes | No es dispositivo médico | Documentar claramente |

---

## Historial de Cambios

| Versión | Fecha | Cambios |
|---------|-------|---------|
| 1.0 | 2025-12 | Documento inicial |
| 2.0 | 2025-12 | Añadido sistema de parametrización |
| 3.0 | 2025-12 | Versión completa con validación |

---

**Fin del documento**
