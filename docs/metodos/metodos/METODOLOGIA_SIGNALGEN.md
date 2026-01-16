# Metodología de Generación de Señales Fisiológicas Sintéticas

**BioSignalSimulator Pro**  
**Grupo #22:** Scarlet Sánchez y Rafael Mata  
**Institución:** Escuela Superior Politécnica del Litoral (ESPOL)  
**Revisado:** 13.01.2026  
**Documento de Sustentación Técnica para Trabajo de Titulación**

---

## Índice

1. [Introducción](#1-introducción)
2. [Arquitectura General del Sistema de Generación](#2-arquitectura-general-del-sistema-de-generación)
3. [Principios de Modelado](#3-principios-de-modelado)
4. [Modelo ECG (McSharry ECGSYN)](#4-modelo-ecg-mcsharry-ecgsyn)
5. [Modelo EMG (Reclutamiento de Unidades Motoras)](#5-modelo-emg-reclutamiento-de-unidades-motoras)
6. [Modelo PPG (Suma de Gaussianas)](#6-modelo-ppg-suma-de-gaussianas)
7. [Validación de Señales](#7-validación-de-señales)
8. [Resultados y Métricas](#8-resultados-y-métricas)
9. [Referencias](#9-referencias)

> **Nota:** La arquitectura computacional del sistema (ESP32, FreeRTOS, comunicación Nextion, WiFi/WebSocket) se documenta en el archivo separado `METODOLOGIA_COMPUTACIONAL.md`.

---

## 1. Introducción

Este documento presenta la metodología de generación de señales fisiológicas sintéticas implementada en el BioSimulator Pro. El sistema genera tres tipos de señales biomédicas: electrocardiograma (ECG), electromiograma de superficie (sEMG) y fotopletismograma (PPG).

El motor de generación se desarrolló mediante modelado matemático validado: cada señal sintética se construye con modelos formulados en tiempo continuo que, tras su discretización para ejecución en microcontrolador, cumplen tres criterios simultáneos: (1) **fidelidad morfológica**—las formas de onda son anatómicamente reconocibles, (2) **rangos clínicos verificables**—amplitudes y tiempos dentro de estándares médicos, y (3) **coherencia espectral**—contenido frecuencial apropiado para cada tipo de señal.

Cada modelo se diseñó para operar en tiempo real sobre un microcontrolador ESP32, entregando muestras discretas a demanda mediante una interfaz uniforme `getDACValue(deltaTime)`. La señal digital resultante se convierte a formato analógico mediante el DAC integrado y un filtro RC de reconstrucción.

**Señales implementadas:**

| Señal | Condiciones | Modelo Base | Referencia Principal | Implementación |
|-------|-------------|-------------|---------------------|----------------|
| ECG | 8 condiciones cardíacas | McSharry ECGSYN | McSharry et al. 2003 [1] | Basada completamente en modelo publicado |
| EMG | 6 condiciones musculares | Fuglevand MU | Fuglevand et al. 1993 [2] | Basada completamente en modelo publicado |
| PPG | 6 condiciones vasculares | Suma de gaussianas | Modelo empírico propio | Inspirada en morfología descrita por Allen [6] |

---

## 2. Arquitectura General del Sistema de Generación

### 2.1 Diagrama de Flujo General - Sistema Completo

El siguiente diagrama muestra el flujo completo desde la interacción del usuario hasta la salida analógica de la señal:

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                    BIOSIGNALSIMULATOR PRO - FLUJO DE GENERACIÓN                 │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                 │
│  ┌─────────────────────────────────────────────────────────────────────────┐   │
│  │                     1. INTERFAZ DE USUARIO                              │   │
│  │                                                                         │   │
│  │   NEXTION 7"                              APP WEB                       │   │
│  │  ┌──────────────┐                      ┌──────────────┐                 │   │
│  │  │ Selección de │                      │  Streaming   │                 │   │
│  │  │ señal y      │◄────── WiFi AP ─────►│  WebSocket   │                 │   │
│  │  │ parámetros   │                      │  Tiempo Real │                 │   │
│  │  └──────┬───────┘                      └──────────────┘                 │   │
│  └─────────┼───────────────────────────────────────────────────────────────┘   │
│            │ Comandos UART                                                      │
│            ▼                                                                    │
│  ┌─────────────────────────────────────────────────────────────────────────┐   │
│  │                     2. CONTROLADOR DE PARÁMETROS                        │   │
│  │                                                                         │   │
│  │  ┌──────────────────┐    ┌──────────────────┐    ┌──────────────────┐  │   │
│  │  │ param_controller │───►│ state_machine    │───►│ signal_engine    │  │   │
│  │  │                  │    │                  │    │                  │  │   │
│  │  │ - Valida rangos  │    │ - IDLE           │    │ - Tasa muestreo  │  │   │
│  │  │ - Aplica límites │    │ - CONFIG         │    │ - Selección      │  │   │
│  │  │ - Pendientes     │    │ - RUNNING        │    │   modelo activo  │  │   │
│  │  └──────────────────┘    │ - ERROR          │    │ - DAC output     │  │   │
│  │                          └──────────────────┘    └────────┬─────────┘  │   │
│  └───────────────────────────────────────────────────────────┼─────────────┘   │
│                                                              │                  │
│            ┌─────────────────────────────────────────────────┤                  │
│            │                                                 │                  │
│            ▼                                                 ▼                  │
│  ┌─────────────────────────────────────────────────────────────────────────┐   │
│  │                     3. MODELOS DE GENERACIÓN                            │   │
│  │                                                                         │   │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐         │   │
│  │  │    ECG_MODEL    │  │    EMG_MODEL    │  │    PPG_MODEL    │         │   │
│  │  │                 │  │                 │  │                 │         │   │
│  │  │ McSharry ECGSYN │  │ Fuglevand MU    │  │ Suma Gaussianas │         │   │
│  │  │                 │  │                 │  │                 │         │   │
│  │  │ • 8 condiciones │  │ • 6 condiciones │  │ • 6 condiciones │         │   │
│  │  │ • fs = 300 Hz   │  │ • fs = 1000 Hz  │  │ • fs = 20 Hz    │         │   │
│  │  │ • RK4 integr.   │  │ • Multi-MU      │  │ • Gaussianas    │         │   │
│  │  │ • PQRST morpho  │  │ • Interferencia │  │ • Dicrotic      │         │   │
│  │  └────────┬────────┘  └────────┬────────┘  └────────┬────────┘         │   │
│  │           │                    │                    │                   │   │
│  │           └────────────────────┼────────────────────┘                   │   │
│  │                                │                                        │   │
│  │                    generateSample(deltaTime)                            │   │
│  │                         → float mV                                      │   │
│  └────────────────────────────────┼────────────────────────────────────────┘   │
│                                   │                                             │
│                                   ▼                                             │
│  ┌─────────────────────────────────────────────────────────────────────────┐   │
│  │                     4. CONVERSIÓN Y SALIDA                              │   │
│  │                                                                         │   │
│  │      Señal (mV)              getDACValue()                              │   │
│  │   ─0.5 ────┬──── +1.5    ─────────────────►   0 ────┬──── 255          │   │
│  │            │                                        │                   │   │
│  │    [-0.5, +1.5] mV     →     Escalado     →    [0, 255] DAC            │   │
│  │                        →     Clampeo      →                             │   │
│  │                                                                         │   │
│  │  DAC = ((mV + 0.5) / 2.0) × 255                                        │   │
│  │                                                                         │   │
│  └─────────────────────────────────────────────────────────────────────────┘   │
│                                   │                                             │
│                                   ▼                                             │
│  ┌─────────────────────────────────────────────────────────────────────────┐   │
│  │                     5. CADENA ANALÓGICA DE SALIDA                       │   │
│  │                                                                         │   │
│  │   ESP32          LM358         CD4051           Filtros RC    BNC       │   │
│  │  ┌──────┐      ┌───────┐     ┌────────┐       ┌────────┐   ┌─────┐     │   │
│  │  │DAC   │─────►│Buffer │────►│ DEMUX  │──┬───►│6.8k+1µF│──►│ ECG │     │   │
│  │  │GPIO25│      │ ×1    │     │        │  │    └────────┘   └─────┘     │   │
│  │  │      │      │       │     │ S0/S1  │  │    ┌────────┐   ┌─────┐     │   │
│  │  │0-3.3V│      │       │     │ GPIO   │  ├───►│1.0k+1µF│──►│ EMG │     │   │
│  │  └──────┘      └───────┘     │ 32/33  │  │    └────────┘   └─────┘     │   │
│  │                              │        │  │    ┌────────┐   ┌─────┐     │   │
│  │                              │        │  └───►│ 25k+1µF│──►│ PPG │     │   │
│  │                              └────────┘       └────────┘   └─────┘     │   │
│  │                                                                         │   │
│  │  NOTA: Solo 1 canal activo a la vez (seleccionado por tipo de señal)   │   │
│  └─────────────────────────────────────────────────────────────────────────┘   │
│                                                                                 │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 2.2 Diagrama de Flujo - Ciclo Principal de Generación

El siguiente diagrama detalla el ciclo de ejecución del motor de señales:

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                     CICLO PRINCIPAL (SIGNAL_ENGINE)                             │
└─────────────────────────────────────────────────────────────────────────────────┘
                                       │
                                       ▼
                          ┌────────────────────────┐
                          │      INICIO LOOP       │
                          │  (cada 1/fs segundos)  │
                          └───────────┬────────────┘
                                      │
                                      ▼
                          ┌────────────────────────┐
                          │ deltaTime = now() -    │
                          │              lastTime  │
                          │ lastTime = now()       │
                          └───────────┬────────────┘
                                      │
                                      ▼
                          ┌────────────────────────┐
                          │ ¿Estado = RUNNING?     │
                          └───────────┬────────────┘
                                      │
                        ┌─────────────┴─────────────┐
                        │ NO                        │ SÍ
                        ▼                           ▼
              ┌──────────────────┐      ┌──────────────────────────┐
              │ DAC = 127 (idle) │      │ Obtener modelo activo:   │
              │ (1.65V offset)   │      │                          │
              └────────┬─────────┘      │ switch(signalType) {     │
                       │                │   ECG → ecgModel         │
                       │                │   EMG → emgModel         │
                       │                │   PPG → ppgModel         │
                       │                │ }                        │
                       │                └────────────┬─────────────┘
                       │                             │
                       │                             ▼
                       │                ┌──────────────────────────┐
                       │                │ mV = model.generateSample│
                       │                │         (deltaTime)      │
                       │                │                          │
                       │                │ Rango: [-0.5, +1.5] mV   │
                       │                └────────────┬─────────────┘
                       │                             │
                       │                             ▼
                       │                ┌──────────────────────────┐
                       │                │ dacValue = model.        │
                       │                │           getDACValue()  │
                       │                │                          │
                       │                │ Rango: [0, 255]          │
                       │                └────────────┬─────────────┘
                       │                             │
                       └─────────────────────────────┤
                                                     │
                                                     ▼
                          ┌────────────────────────────────────────┐
                          │           SALIDAS PARALELAS            │
                          └────────────────────────────────────────┘
                                      │
                    ┌─────────────────┼─────────────────┐
                    ▼                 ▼                 ▼
          ┌──────────────┐  ┌──────────────┐  ┌──────────────────┐
          │  DAC GPIO25  │  │   NEXTION    │  │   WEBSOCKET      │
          │              │  │   WAVEFORM   │  │   STREAMING      │
          │ dacWrite(    │  │              │  │                  │
          │  GPIO25,     │  │ waveformAdd( │  │ broadcastSample( │
          │  dacValue)   │  │  waveformVal)│  │  mV, timestamp)  │
          │              │  │              │  │                  │
          │ → Vout       │  │ → Gráfica    │  │ → JSON a cliente │
          └──────────────┘  └──────────────┘  └──────────────────┘
                                      │
                                      ▼
                          ┌────────────────────────┐
                          │  Actualizar métricas:  │
                          │  - Frecuencia cardíaca │
                          │  - Amplitudes PQRST    │
                          │  - Fatiga muscular     │
                          │  - SpO2 / PI           │
                          └───────────┬────────────┘
                                      │
                                      ▼
                          ┌────────────────────────┐
                          │     FIN CICLO          │
                          │ (esperar siguiente Ts) │
                          └────────────────────────┘
```

### 2.3 Justificación de la Metodología de Generación

#### 2.3.1 Criterios de Diseño

| Criterio | Descripción | Implementación |
|----------|-------------|----------------|
| **Tiempo Real** | Generación sin latencia perceptible | Modelos matemáticos optimizados para ESP32, <1ms por muestra |
| **Fidelidad Fisiológica** | Morfología clínicamente representativa | Modelos validados en literatura científica |
| **Flexibilidad Paramétrica** | Ajuste de parámetros en tiempo real | Interfaz uniforme con parámetros pendientes aplicados entre latidos |
| **Eficiencia Computacional** | Operación en microcontrolador de bajo costo | Aritmética de punto flotante simple, evitar operaciones costosas |
| **Modularidad** | Independencia entre modelos | Cada modelo en archivo separado, interfaz común |

#### 2.3.2 Normativas y Estándares Considerados

| Normativa | Aplicación | Implementación |
|-----------|------------|----------------|
| **IEC 60601-2-47** | Requisitos para ECG ambulatorio | Rangos de amplitud y frecuencia dentro de especificaciones clínicas |
| **AAMI EC11** | Rendimiento de electrocardiógrafos | Morfología PQRST con tiempos dentro de rangos normales |
| **SENIAM** | Posicionamiento electromiografía | Rangos de amplitud y frecuencia de EMG de superficie |
| **ISO 80601-2-61** | Oxímetros de pulso | Formas de onda PPG con índice de perfusión realista |

#### 2.3.3 Principios Técnicos Aplicados

1. **Teorema de Nyquist**: $f_s \geq 2 \times f_{max}$ con factor de seguridad 2-5×
2. **Integración Numérica RK4**: Precisión $O(h^4)$ para ecuaciones diferenciales del modelo ECG
3. **Superposición de Señales**: EMG como suma de potenciales de unidades motoras
4. **Modelado Paramétrico**: Modificación de comportamiento mediante ajuste de coeficientes

#### 2.3.4 Selección de Modelos (Análisis de Alternativas)

| Alternativa | Ventajas | Desventajas | Decisión |
|-------------|----------|-------------|----------|
| **Modelos Matemáticos** | Tiempo real, flexibles, livianos | Requieren calibración | ✅ **Seleccionado** |
| Redes Neuronales (GAN) | Alta fidelidad | Latencia >50ms, requiere GPU | ❌ Descartado |
| Datasets Pre-grabados | Morfología real | Sin flexibilidad paramétrica | ❌ Descartado |
| Síntesis por Wavelets | Multiresolución | Complejidad innecesaria | ❌ Descartado |

#### 2.3.5 Naturaleza de los Modelos: Continuo vs. Discreto

> **Nota importante:** Todos los modelos utilizados están formulados en tiempo continuo; sin embargo, su ejecución en el microcontrolador requiere discretización temporal. La señal generada es **discreta en el dominio digital** y **continua en amplitud** tras la conversión digital-analógica.

**Tabla 2.1: Naturaleza de cada modelo y su implementación**

| Modelo | Formulación Teórica | Implementación Digital | Salida DAC |
|--------|--------------------|-----------------------|------------|
| **ECG (McSharry)** | Continuo (ODEs) | Discreta (integración numérica RK4) | Analógica continua |
| **EMG (Fuglevand)** | Continuo (procesos puntuales) | Discreta (superposición muestreada) | Analógica continua |
| **PPG (empírico)** | Continuo (funciones gaussianas) | Discreta (evaluación a intervalos $T_s$) | Analógica continua |

**Proceso de discretización:**

```
┌─────────────────────────────────────────────────────────────────────────────┐
│         MODELO TEÓRICO           →        IMPLEMENTACIÓN DIGITAL            │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ECG: ODEs continuas             →  Integración RK4 con paso h = 1/fs      │
│       dz/dt = f(x,y,z,t)              z[n+1] = z[n] + (k1+2k2+2k3+k4)/6    │
│                                                                             │
│  EMG: Trenes de disparo cont.    →  Muestreo a fs = 1000 Hz                │
│       s(t) = Σ h(t - tᵢ)              s[n] = Σ h[n - nᵢ]                   │
│                                                                             │
│  PPG: Gaussianas continuas       →  Evaluación en φ = n/N (fase discreta)  │
│       g(t) = exp(-t²/2σ²)             g[n] = exp(-φ[n]²/2σ²)               │
│                                                                             │
├─────────────────────────────────────────────────────────────────────────────┤
│         SEÑAL DIGITAL            →        SEÑAL ANALÓGICA                   │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Valor discreto 0-255            →  Tensión continua 0-3.3V                │
│  (8 bits, cuantizado)                (DAC + filtro RC reconstrucción)      │
│                                                                             │
│  Dominio: discreto en tiempo     →  Dominio: continuo en tiempo            │
│           discreto en amplitud       continuo en amplitud                  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

**Justificación de la discretización:**

1. **Microcontrolador digital**: El ESP32 solo puede ejecutar operaciones en tiempo discreto
2. **Teorema de muestreo**: $f_s \geq 2 \times f_{max}$ garantiza reconstrucción sin pérdida
3. **Reconstrucción analógica**: El filtro RC pasabajos suaviza el escalonado del DAC
4. **Transparencia para el usuario**: La señal en el BNC es indistinguible de una señal continua real

---

## 3. Principios de Modelado

### 3.1 Criterio de Muestreo


Se adoptó el criterio de Nyquist con factor de seguridad 5× para garantizar la preservación de la morfología de las señales:

$$f_s \geq 5 \times f_{max}$$

**Tabla 3.1: Frecuencias de muestreo por señal**

| Señal | $f_{max}$ clínico | $f_s$ implementada | Justificación |
|-------|-------------------|-------------------|---------------|
| ECG | 150 Hz (QRS) | 300 Hz | Nyquist clínico (2× fmax) |
| EMG | 500 Hz (MUAP) | 1000 Hz | Nyquist sEMG (2× fmax) |
| PPG | 10 Hz (pulso) | 20 Hz | Nyquist forma lenta (2× fmax) |

### 2.2 Principio de Fidelidad Fisiológica

Cada modelo se basó en literatura científica validada, priorizando:

1. **Morfología correcta**: Forma de onda característica de cada señal
2. **Rangos clínicos**: Amplitudes y tiempos dentro de valores fisiológicos
3. **Variabilidad natural**: Incorporación de variabilidad latido-a-latido (HRV, PI)
4. **Condiciones patológicas**: Modificaciones morfológicas específicas por patología

### 2.3 Interfaz Uniforme de Generación

Todos los modelos implementaron una interfaz común para integración con el motor de señales:

```cpp
// Interfaz común de los modelos
float generateSample(float deltaTime);  // Genera muestra en mV
uint8_t getDACValue(float deltaTime);   // Genera muestra escalada a 0-255
uint8_t getWaveformValue();             // Valor para visualización Nextion
```

## 3. Modelo ECG (McSharry ECGSYN)

El modelo ECG constituye el corazón del simulador de señales cardíacas. Se basó en el trabajo seminal de McSharry et al. (2003), que propone un sistema dinámico capaz de generar señales electrocardiográficas sintéticas con morfología PQRST realista y variabilidad fisiológica controlada.

### 3.1 Fundamento Teórico

#### 3.1.1 Modelo Matemático ECGSYN

El modelo describe la trayectoria de un punto $(x, y, z)$ en el espacio de estados, donde $z(t)$ representa la amplitud del ECG. Las ecuaciones diferenciales son:

$$\frac{dx}{dt} = \alpha x - \omega y$$

$$\frac{dy}{dt} = \alpha y + \omega x$$

$$\frac{dz}{dt} = -\sum_{i \in \{P,Q,R,S,T\}} a_i \Delta\theta_i \exp\left(-\frac{\Delta\theta_i^2}{2b_i^2}\right) - (z - z_0)$$

Donde:
- $\omega = 2\pi / RR$ es la velocidad angular (función del intervalo RR)
- $\alpha = 1 - \sqrt{x^2 + y^2}$ es el término de atracción al círculo unitario
- $\theta = \arctan2(y, x)$ es el ángulo actual en el ciclo cardíaco
- $\Delta\theta_i = (\theta - \theta_i) \mod 2\pi$ es la diferencia angular respecto al pico $i$
- $a_i, b_i, \theta_i$ son amplitud, ancho y posición angular de cada onda PQRST

**Tabla 5.1: Parámetros PQRST base (McSharry 2003)**

| Onda | $\theta_i$ (rad) | $a_i$ | $b_i$ |
|------|------------------|-------|-------|
| P | -1.22 (-70°) | 0.25 | 0.25 |
| Q | -0.26 (-15°) | -0.05 | 0.10 |
| R | 0.00 (0°) | 1.00 | 0.10 |
| S | 0.26 (15°) | -0.10 | 0.10 |
| T | 1.75 (100°) | 0.35 | 0.40 |

*Fuente: McSharry PE et al. IEEE Trans Biomed Eng. 2003 [1]*

### 3.2 Justificación del Método

Se seleccionó el modelo ECGSYN por las siguientes razones:

1. **Validación científica**: Modelo citado más de 2000 veces en literatura biomédica
2. **Flexibilidad paramétrica**: Permite modificar HR, morfología y agregar patologías
3. **Eficiencia computacional**: Integración RK4 con bajo costo computacional
4. **Variabilidad fisiológica**: Incorporación nativa de HRV mediante proceso RR

#### 3.2.1 Matriz de Decisión: Métodos de Generación de Señales Biomédicas

Para seleccionar el método de generación de señales se evaluaron cuatro alternativas con criterios ponderados según la importancia para la aplicación:

**Tabla 3.2.1a: Criterios de evaluación y ponderación**

| Criterio | Peso (%) | Justificación |
|----------|----------|---------------|
| Capacidad de Tiempo Real | 30% | Crítico: generación continua en ESP32 a 300-1000 Hz |
| Precisión Morfológica | 25% | Muy importante: fidelidad clínica de formas PQRST |
| Flexibilidad Paramétrica | 20% | Importante: simular múltiples patologías |
| Consumo de Recursos (RAM/CPU) | 15% | Importante: MCU con 520 KB SRAM |
| Complejidad de Implementación | 10% | Moderado: tiempo de desarrollo limitado |

**Tabla 3.2.1b: Evaluación de alternativas (escala 1-5)**

| Método | Tiempo Real | Morfología | Flexibilidad | Recursos | Implementación | Total Ponderado |
|--------|-------------|------------|--------------|----------|----------------|-----------------|
| **Modelos Matemáticos** (McSharry, Fuglevand) | 5 | 5 | 5 | 4 | 4 | **4.70** ✓ |
| Redes Neuronales Generativas (GANs) | 2 | 4 | 3 | 1 | 2 | **2.60** |
| Datasets Pre-grabados (Replay) | 5 | 4 | 1 | 4 | 5 | **3.70** |
| Síntesis por Wavelets | 3 | 4 | 3 | 3 | 2 | **3.20** |

**Puntuación detallada:**

1. **Modelos Matemáticos (McSharry/Fuglevand/Gaussiano)** - Puntuación: 4.70
   - **Tiempo Real (5):** Integración RK4 con ∆t=1ms ejecuta en <0.5ms en ESP32@240MHz
   - **Morfología (5):** Parámetros ajustables para recrear ondas PQRST, MUAPs, pulsos PPG validados clínicamente
   - **Flexibilidad (5):** Modificación de parámetros permite simular 8 condiciones ECG, 6 EMG, 6 PPG
   - **Recursos (4):** ~15 KB RAM por modelo, <30% CPU Core 1 @ 1kHz
   - **Implementación (4):** Código determinista, ecuaciones diferenciales documentadas

2. **Redes Neuronales Generativas (GANs/VAEs)** - Puntuación: 2.60
   - **Tiempo Real (2):** Inferencia de red convolucional demanda >50ms por latido (insuficiente para 300 Hz ECG)
   - **Morfología (4):** Excelente si se entrena con datasets grandes, pero variable
   - **Flexibilidad (3):** Requiere re-entrenamiento para nuevas condiciones
   - **Recursos (1):** Modelos pre-entrenados ocupan >500 KB flash, >200 KB RAM
   - **Implementación (2):** Complejidad alta: framework TensorFlow Lite, cuantización, ajuste hiperparámetros

3. **Datasets Pre-grabados (Replay MIT-BIH, Physionet)** - Puntuación: 3.70
   - **Tiempo Real (5):** Lectura de buffer circular es trivial (<0.1ms)
   - **Morfología (4):** Señales reales con morfología auténtica
   - **Flexibilidad (1):** Limitado a condiciones grabadas, sin parametrización dinámica (HR fijo, no permite cambio en vivo)
   - **Recursos (4):** ~100 KB flash por 10s de señal @ 300 Hz, baja RAM
   - **Implementación (5):** Código simple: interpolación lineal + loop

4. **Síntesis por Wavelets (Transformada Inversa)** - Puntuación: 3.20
   - **Tiempo Real (3):** Transformada wavelet inversa requiere FFT, ~10ms por frame
   - **Morfología (4):** Buena reconstrucción si se eligen wavelets adecuadas (Daubechies, Morlet)
   - **Flexibilidad (3):** Parámetros de escala/traslación permiten variaciones, pero difícil intuición clínica
   - **Recursos (3):** Requiere buffer FFT (~2 KB), tablas wavelet (~5 KB)
   - **Implementación (2):** Complejidad media-alta: librería FFT, manejo de coeficientes

**Conclusión:**

Los **modelos matemáticos** obtuvieron la mayor puntuación (4.70/5.00) debido a su excelente desempeño en tiempo real, precisión morfológica y flexibilidad paramétrica. El modelo McSharry (ECG) permite modificar frecuencia cardíaca, amplitud PQRST y añadir arritmias mediante ajuste de parámetros angulares $(\theta_i, a_i, b_i)$. El modelo Fuglevand (EMG) permite controlar número de unidades motoras activas y frecuencias de disparo. El modelo Gaussiano (PPG) permite ajustar amplitud de pulso y compliance vascular.

Las **redes neuronales** fueron descartadas por su alto costo computacional (inferencia >50ms incompatible con generación a 300-1000 Hz) y dependencia de datasets de entrenamiento que el equipo no poseía. Los **datasets pre-grabados** ofrecen morfología real pero carecen de la flexibilidad para modificar parámetros en tiempo real (ej: cambiar HR de 60→120 bpm durante ejecución). La **síntesis por wavelets** presenta complejidad innecesaria para la aplicación.

### 3.3 Diagrama de Flujo - Generación ECG

```
┌─────────────────────────────────────────────────────────────────┐
│                    INICIO generateSample()                       │
└─────────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────────┐
│  ¿Condición = VENTRICULAR_FIBRILLATION?                         │
└─────────────────────────────────────────────────────────────────┘
          │ SÍ                              │ NO
          ▼                                 ▼
┌──────────────────┐              ┌──────────────────────────────┐
│ generateVFibSample()           │  Calcular ω = 2π/currentRR    │
│ (modelo espectral)             │                                │
└──────────────────┘              └──────────────────────────────┘
          │                                 │
          │                                 ▼
          │                       ┌──────────────────────────────┐
          │                       │  Integración RK4:            │
          │                       │  rungeKutta4Step(dt, ω)      │
          │                       │                               │
          │                       │  k1 = f(state)               │
          │                       │  k2 = f(state + dt/2 × k1)   │
          │                       │  k3 = f(state + dt/2 × k2)   │
          │                       │  k4 = f(state + dt × k3)     │
          │                       │  state += dt/6 × (k1+2k2+2k3+k4) │
          │                       └──────────────────────────────┘
          │                                 │
          │                                 ▼
          │                       ┌──────────────────────────────┐
          │                       │  ¿Cruce θ = 0 (nuevo latido)?│
          │                       └──────────────────────────────┘
          │                                 │ SÍ
          │                                 ▼
          │                       ┌──────────────────────────────┐
          │                       │  detectNewBeat():            │
          │                       │  - Actualizar beatCount      │
          │                       │  - Generar nuevo RR (HRV)    │
          │                       │  - Medir métricas PQRST      │
          │                       │  - Aplicar parámetros pending│
          │                       └──────────────────────────────┘
          │                                 │
          └─────────────────────────────────┤
                                            ▼
                              ┌──────────────────────────────┐
                              │  ¿isCalibrated = true?       │
                              └──────────────────────────────┘
                                   │ NO              │ SÍ
                                   ▼                 ▼
                    ┌──────────────────┐  ┌──────────────────────┐
                    │ updateCalibration│  │ z_mV = applyScaling()│
                    │ Buffer()         │  │ = (z-baseline) × G   │
                    └──────────────────┘  └──────────────────────┘
                                            │
                                            ▼
                              ┌──────────────────────────────┐
                              │  Agregar ruido gaussiano     │
                              │  z_mV += noise × gaussian()  │
                              └──────────────────────────────┘
                                            │
                                            ▼
                              ┌──────────────────────────────┐
                              │  Clampear a rango fisiológico│
                              │  [-0.5, +1.5] mV             │
                              └──────────────────────────────┘
                                            │
                                            ▼
                              ┌──────────────────────────────┐
                              │  RETORNAR z_mV               │
                              │  Rango: [-0.5, +1.5] mV      │
                              └──────────────────────────────┘
                                            │
                 ┌──────────────────────────┴──────────────────────────┐
                 ▼                                                     ▼
┌─────────────────────────────────┐             ┌─────────────────────────────────┐
│  SALIDA DAC (GPIO25)            │             │  SALIDA NEXTION WAVEFORM        │
│  Función: getDACValue()         │             │  Función: getWaveformValue()    │
│  Rango: 0-255 (8-bit)           │             │  Rango: 0-255 (8-bit)           │
│  Voltaje: 0-3.3V                │             │  Altura: 10-370 px              │
│                                 │             │                                 │
│  Fórmula:                       │             │  Fórmula:                       │
│  DAC = (mV + 0.5) / 2.0 × 255   │             │  val = (mV + 0.5) / 2.0 × 255   │
│                                 │             │  px = map(val, 0, 255, 10, 370) │
│  Ejemplo (R = +1.0 mV):         │             │  Ejemplo (R = +1.0 mV):         │
│  DAC = 1.5/2.0 × 255 = 191      │             │  val = 191 → px = 279           │
│  Vout = 191/255 × 3.3V = 2.47V  │             │                                 │
└─────────────────────────────────┘             └─────────────────────────────────┘
```

### 3.4 Implementación de Condiciones Patológicas

**Tabla 3.2: Modificaciones por condición ECG**

| Condición | HR (BPM) | Modificación Morfológica |
|-----------|----------|--------------------------|
| NORMAL | 60-100 | Parámetros base McSharry |
| TACHYCARDIA | 100-180 | HR elevado, QT acortado |
| BRADYCARDIA | 40-60 | HR reducido, QT prolongado |
| ATRIAL_FIBRILLATION | 60-160 | Sin onda P, RR muy irregular (±30%) |
| VENTRICULAR_FIBRILLATION | N/A | Modelo espectral caótico 4-10 Hz |
| AV_BLOCK_1 | 50-80 | PR > 200 ms |
| ST_ELEVATION | 60-100 | ST elevado +0.2 mV (STEMI) |
| ST_DEPRESSION | 60-100 | ST deprimido -0.15 mV (isquemia) |

### 3.5 Calibración Automática

El modelo implementó calibración automática por pico R para garantizar amplitudes fisiológicas:

1. **Fase de calibración** (3 latidos iniciales):
   - Se detectan picos R crudos del modelo
   - Se calcula $R_{model}$ = promedio de picos R detectados

2. **Cálculo de ganancia fisiológica**:
   $$G = \frac{R_{objetivo}}{R_{model}} = \frac{1.0 \text{ mV}}{R_{model}}$$

3. **Aplicación de escalado**:
   $$z_{mV} = (z_{raw} - baseline) \times G$$

### 3.6 Escalado de Salida y DAC

La señal ECG ya calibrada se limita al rango clínico **−0.5 mV a +1.5 mV**, suficiente para capturar toda la morfología PQRST sin saturación. Estas constantes (`ECG_DISPLAY_MIN/MAX_MV`) se encuentran en @include/models/ecg_model.h#48-53. Cada muestra pasa por `getDACValue()` @src/models/ecg_model.cpp#881-889, donde se normaliza el intervalo completo directamente a 0‑255. Ese valor digital alimenta simultáneamente el DAC (GPIO25) y el componente waveform del Nextion, por lo que la misma escala asegura que el voltaje analógico (0‑3.3 V) represente con fidelidad los 2 mV fisiológicos modelados.

---

## 4. Modelo EMG (Reclutamiento de Unidades Motoras)

El modelo EMG reproduce la actividad eléctrica muscular mediante la simulación del reclutamiento de unidades motoras (MUs). Se basó en el trabajo de Fuglevand et al. (1993) y el principio de Henneman, que describen cómo las unidades motoras se activan progresivamente según el nivel de excitación muscular.

### 4.1 Fundamento Teórico

El modelo EMG de superficie (sEMG) se basó en el trabajo de Fuglevand et al. [2], que describe el reclutamiento de unidades motoras (MUs) y la generación de potenciales de acción de unidades motoras (MUAPs).

#### 4.1.1 Principio de Reclutamiento de Henneman

El reclutamiento sigue el principio del tamaño de Henneman [3]: las unidades motoras se activan en orden ascendente de umbral, desde las más pequeñas (tipo I, resistentes a fatiga) hasta las más grandes (tipo II, fatigables).

$$threshold_i = \frac{i}{N} \times RR$$

Donde $i$ es el índice de la MU, $N$ es el número total de MUs y $RR$ es el rango de reclutamiento.

#### 4.1.2 Modelo de Disparo

La frecuencia de disparo de cada MU sigue una relación lineal con la excitación:

$$FR_i = FR_{min} + (FR_{max} - FR_{min}) \times \frac{E - threshold_i}{1 - threshold_i}$$

Donde $E$ es el nivel de excitación (0-1), $FR_{min}$ = 8 Hz y $FR_{max}$ = 35 Hz.

#### 4.1.3 Forma del MUAP

Se utilizó la wavelet Mexican Hat (Ricker) como aproximación al MUAP trifásico:

$$MUAP(t) = A_i \left(1 - \frac{t^2}{\sigma^2}\right) \exp\left(-\frac{t^2}{2\sigma^2}\right)$$

Donde:
- $A_i$ = amplitud de la MU (distribución exponencial: 0.05-1.5 mV)
- $\sigma$ = ancho temporal (~3-8 ms)

**Tabla 4.1: Parámetros del pool de MUs**

| Parámetro | Valor | Fuente |
|-----------|-------|--------|
| Número de MUs | 100 | Fuglevand 1993 |
| Rango de reclutamiento | 30× | Adaptado para sEMG |
| Amplitud mínima | 0.05 mV | De Luca 1997 |
| Frecuencia disparo min | 8 Hz | Fuglevand 1993 |
| Frecuencia disparo max | 35 Hz | Fuglevand 1993 |

*Fuentes: Fuglevand AJ et al. [2], De Luca CJ [4]*

### 4.2 Justificación del Método

Se seleccionó el modelo de Fuglevand por:

1. **Base fisiológica sólida**: Describe mecanismos reales de contracción muscular
2. **Escalabilidad**: Permite simular desde reposo hasta MVC
3. **Condiciones especiales**: Soporta temblor y fatiga mediante modificadores
4. **Salida bipolar**: Genera señal AC centrada en 0 mV (característico de sEMG)

### 4.3 Diagrama de Flujo - Generación EMG

```
┌─────────────────────────────────────────────────────────────────┐
│                       INICIO tick(deltaTime)                     │
└─────────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────────┐
│  Actualizar tiempo acumulado: accumulatedTime += deltaTime      │
└─────────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────────┐
│  ¿Hay rampa de excitación activa?                               │
└─────────────────────────────────────────────────────────────────┘
          │ SÍ                              │ NO
          ▼                                 │
┌──────────────────────────────┐            │
│  Interpolar excitación:      │            │
│  E = lerp(base, target, t/T) │            │
└──────────────────────────────┘            │
          │                                 │
          └─────────────────────────────────┤
                                            ▼
┌─────────────────────────────────────────────────────────────────┐
│  updateMotorUnitRecruitment():                                  │
│  FOR cada MU[i] en pool:                                        │
│    IF excitation >= MU[i].threshold:                            │
│      MU[i].isActive = true                                      │
│      MU[i].firingRate = calcFiringRate(excitation)              │
│    ELSE:                                                        │
│      MU[i].isActive = false                                     │
└─────────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────────┐
│  Generar muestra cruda:                                         │
│  sample = 0                                                     │
│  FOR cada MU[i] activa:                                         │
│    IF accumulatedTime >= MU[i].nextFiringTime:                  │
│      sample += generateMUAP(timeSinceFiring, amplitude)         │
│      MU[i].nextFiringTime += 1/firingRate + jitter              │
└─────────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────────┐
│  Aplicar modificadores de condición:                            │
│  - TREMOR: sample *= sin(2π × 5Hz × t) × modulación             │
│  - FATIGUE: sample *= rmsDecayFactor (decay exponencial)        │
└─────────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────────┐
│  Agregar ruido y escalar:                                       │
│  sample += noise × gaussian()                                   │
│  sample *= amplitudeGain                                        │
└─────────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────────┐
│  Clampear a rango: [-5.0, +5.0] mV                              │
│  Almacenar en caché: cachedRawSample = sample                   │
└─────────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────────┐
│  Procesamiento para canal envolvente:                           │
│  1. Filtro pasa-banda (20-450 Hz)                               │
│  2. Rectificación: |sample|                                     │
│  3. Filtro pasa-bajos (15 Hz) → Envolvente RMS                  │
│  Función: getEnvelopeValue()                                    │
│  Rango envolvente: [0.0, +2.0] mV (solo positivo)               │
└─────────────────────────────────────────────────────────────────┘
                               │
        ┌──────────────────────┴──────────────────────┐
        │                                             │
        ▼                                             ▼
┌───────────────────────────────┐     ┌───────────────────────────────┐
│  SEÑAL RAW (Canal 1)          │     │  SEÑAL ENVOLVENTE (Canal 2)   │
│  cachedRawSample              │     │  cachedEnvelopeSample         │
│  Rango: [-5.0, +5.0] mV       │     │  Rango: [0.0, +2.0] mV        │
└───────────────────────────────┘     └───────────────────────────────┘
        │                                             │
        ▼                                             ▼
┌───────────────────────────────────────────────────────────────────┐
│  SALIDA DAC (GPIO25) - Seleccionable RAW o ENVELOPE               │
│  Funciones: getRawDACValue() / getProcessedDACValue()             │
│  Rango: 0-255 (8-bit) → Voltaje: 0-3.3V                           │
│                                                                   │
│  ┌─────────────────────────────────────────────────────────────┐  │
│  │  Modo RAW (por defecto):                                    │  │
│  │  Rango señal: ±5 mV bipolar                                 │  │
│  │  Fórmula: DAC = (mV + 5.0) / 10.0 × 255                     │  │
│  │  Ejemplo (+2.5 mV): DAC = 7.5/10 × 255 = 191 → 2.47V        │  │
│  └─────────────────────────────────────────────────────────────┘  │
│  ┌─────────────────────────────────────────────────────────────┐  │
│  │  Modo ENVELOPE:                                             │  │
│  │  Rango señal: 0-2 mV unipolar (EMG_RMS_MAX_MV)              │  │
│  │  Fórmula: DAC = (mV / 2.0) × 255                            │  │
│  │  Ejemplo (1.0 mV): DAC = 0.5 × 255 = 128 → 1.65V            │  │
│  │  Full-scale DAC para máxima resolución de envolvente        │  │
│  └─────────────────────────────────────────────────────────────┘  │
│                                                                   │
│  NOTA: El modo se selecciona desde UI (botones bt0/bt1 en HMI)    │
│  mediante signalEngine->setEMGDACOutput(RAW|ENVELOPE)             │
└───────────────────────────────────────────────────────────────────┘
        │                                             │
        ▼                                             ▼
┌───────────────────────────────────────────────────────────────────┐
│  SALIDA NEXTION WAVEFORM (2 canales)                              │
│  Funciones: getWaveformValue_Ch0(), getWaveformValue_Ch1()        │
│                                                                   │
│  Canal RAW (ch0):                   Canal ENV (ch1):              │
│  Rango: 0-380 px                    Rango: 0-380 px               │
│  Escala: ±5 mV → 0-380             Escala: 0-5 mV → 190-380      │
│  (Envelope superpuesto en misma escala para comparación visual)   │
│                                                                   │
│  NOTA: Escala Nextion diferente a DAC real:                       │
│  - Nextion ENV: 0-5 mV (compartida con RAW para superposición)    │
│  - DAC ENV: 0-2 mV (full-scale para máxima resolución)            │
└───────────────────────────────────────────────────────────────────┘
```

### 4.4 Implementación de Condiciones

**Tabla 4.2: Condiciones EMG y parámetros**

| Condición | Excitación | RMS esperado | Características |
|-----------|------------|--------------|-----------------|
| REST | 0-5% | 0.02-0.05 mV | Pocas MUs activas, FR bajo |
| LOW | 5-20% | 0.1-0.2 mV | Reclutamiento parcial |
| MODERATE | 20-50% | 0.3-0.8 mV | Reclutamiento progresivo |
| HIGH | 50-100% | 1.0-5.0 mV | Máximo reclutamiento |
| TREMOR | 20-40% | 0.1-0.5 mV | Modulación sinusoidal 5 Hz |
| FATIGUE | 50% sostenido | 1.5→0.4 mV | Decay exponencial de RMS |

### 4.5 Modelo de Fatiga Muscular

La fatiga se modeló según Cifrek et al. [5] con tres componentes:

1. **Descenso de frecuencia mediana (MDF)**:
   $$MDF(t) = MDF_0 + (MDF_f - MDF_0)(1 - e^{-t/\tau_{MDF}})$$
   
   Con $MDF_0$ = 95 Hz, $MDF_f$ = 60 Hz, $\tau_{MDF}$ = 10 s

2. **Descenso de amplitud RMS**:
   $$RMS(t) = RMS_0 \times e^{-t/\tau_{RMS}}$$
   
   Con factor de decay de 1.5 → 0.6 mV en 10 s

3. **Nivel de fatiga muscular (MFL)**:
   $$MFL(t) = \min\left(1.0, \frac{t}{T_{fatiga}}\right)$$
   
   Con $T_{fatiga}$ = 15 s

### 4.6 Escalado de Salida (Raw y Envolvente)

#### 4.6.1 Señal Cruda (RAW)
Tras sumar los MUAPs individuales, la señal se clampa a **±5 mV** (`EMG_OUTPUT_MIN/MAX_MV`) porque ese rango cubre el 95% de amplitudes sEMG reportadas por De Luca, Merletti y Konrad. El clamp se aplica al final de `generateSample()` antes de la conversión DAC.

#### 4.6.2 Envolvente RMS
La señal cruda se rectifica y se integra con una ventana RMS de 30 ms (`updateEnvelopeBuffer()`), equivalente al pasabajos 3-5 Hz recomendado por SENIAM. Ese valor se limita a **0-2 mV** (`EMG_RMS_MAX_MV`) para que el indicador de nivel de contracción coincida con los ~2 mV RMS observados en EMG superficial durante MVC.

#### 4.6.3 Escalas Separadas DAC vs Nextion (Actualización v1.2)

Se implementaron **escalas diferentes** para visualización Nextion y salida DAC real:

| Destino | Señal | Rango mV | Escala DAC/Waveform | Justificación |
|---------|-------|----------|---------------------|---------------|
| **Nextion** | RAW | ±5 mV | 0-380 px | Visualización bipolar |
| **Nextion** | ENV | 0-5 mV | 190-380 px | Superpuesto con RAW |
| **DAC real** | RAW | ±5 mV | 0-255 (full) | Fidelidad espectral |
| **DAC real** | ENV | 0-2 mV | 0-255 (full) | Máxima resolución |

**Conversión DAC Envelope (0-2 mV → 0-255):**

| Envelope (mV) | DAC (0-255) | Voltaje ESP32 |
|---------------|-------------|---------------|
| 0.0 mV        | 0           | 0.00 V        |
| 0.5 mV        | 64          | 0.83 V        |
| 1.0 mV        | 128         | 1.65 V        |
| 2.0 mV (MVC)  | 255         | 3.30 V        |

Esta separación permite que la visualización Nextion muestre RAW y Envelope **superpuestos en la misma escala** (para comparación visual directa), mientras que el DAC físico aprovecha **100% del rango** para cada modo, maximizando la resolución de salida analógica.

#### 4.6.4 Selección de Modo DAC desde UI

El usuario puede seleccionar qué señal sale por el DAC físico mediante botones dual-state en la página `waveform_emg`:

- **bt0 (ID 26)**: Selecciona ENVELOPE → `signalEngine->setEMGDACOutput(ENVELOPE)`
- **bt1 (ID 27)**: Selecciona RAW → `signalEngine->setEMGDACOutput(RAW)` (por defecto)

Los botones tienen **exclusión mutua**: al presionar uno, el ESP32 desactiva el otro mediante comandos Nextion (`bt0.val=0` o `bt1.val=0`).

**Configuración Nextion HMI (Touch Release Event):**
```
bt0: printh 65 07 1A 01 FF FF FF  (ENVELOPE)
bt1: printh 65 07 1B 01 FF FF FF  (RAW)
```

### 4.7 Sincronización Envelope-Waveform (Actualización v1.1)

Para garantizar coherencia entre el waveform visualizado y las métricas numéricas, el cálculo del envelope se realiza **en cada tick()**, no solo cuando se solicita la métrica:

```cpp
// En tick(): cálculo sincronizado del envelope
void EMGModel::tick(float deltaTime) {
    cachedRawSample = generateSample(deltaTime);
    sampleIsCached = true;
    
    // Envelope sincronizado con cada muestra
    float rectified = fabsf(cachedRawSample);
    updateEnvelopeBuffer(rectified);
    float envelopeRMS = sqrtf(envelopeSum / ENVELOPE_BUFFER_SIZE);
    lastProcessedValue = lastProcessedValue * 0.98f + envelopeRMS * 0.02f;  // EMA α=0.02
}
```

**Mapeo proporcional Raw-Envelope para Nextion:**

| Señal | Rango mV | Escala Y (0-380) | Posición |
|-------|----------|------------------|----------|
| Raw | -5 a +5 | 0 a 380 | Centro = 190 |
| Envelope | 0 a 2 | 190 a 266 | Misma escala que raw |

El envelope usa la **misma escala Y que el raw** para visualización proporcional:
- 0 mV envelope → Y = 190 (centro, igual que 0 mV del raw)
- 2 mV envelope → Y = 266 (mismo lugar que ocuparía 2 mV en escala raw)

Esto permite comparación visual directa entre amplitud raw y nivel de envelope.

---

## 5. Modelo PPG (Suma de Gaussianas)

El modelo PPG simula la señal de fotopletismografía, que refleja las variaciones de volumen sanguíneo arterial durante el ciclo cardíaco. A diferencia del ECG (modelo dinámico) y el EMG (modelo estocástico), el PPG se implementó como un **modelo empírico paramétrico** basado en la superposición de funciones gaussianas, inspirado en la caracterización morfológica cualitativa de Allen (2007).

### 5.1 Fundamento Teórico

El modelo PPG es un modelo empírico que representa la onda de pulso como superposición de funciones gaussianas. La estructura de tres componentes (sistólica, dicrótica, diastólica) se inspiró en la descripción morfológica cualitativa de Allen [6], pero los parámetros numéricos (posiciones, anchos, amplitudes) se ajustaron empíricamente para producir formas de onda clínicamente representativas.

#### 5.1.1 Componentes de la Onda PPG

La señal PPG se compone de:

1. **Componente DC**: Absorción constante por tejido estático (~1000 mV)
2. **Componente AC**: Modulación pulsátil por volumen sanguíneo arterial (1-200 mV)

$$PPG(t) = DC + AC \times pulseShape(t)$$

#### 5.1.2 Forma de Onda Normalizada

La forma de pulso se construyó como:

$$pulseShape(\phi) = G_s(\phi) + d_r \times G_d(\phi) - n_d \times G_n(\phi)$$

Donde:
- $G_s(\phi)$ = gaussiana sistólica (pico principal)
- $G_d(\phi)$ = gaussiana diastólica (onda reflejada)
- $G_n(\phi)$ = gaussiana de muesca dicrótica
- $d_r$ = ratio diastólico/sistólico (≈0.4)
- $n_d$ = profundidad de muesca dicrótica (≈0.25)
- $\phi$ = fase normalizada en el ciclo (0-1)

**Tabla 5.1: Parámetros de forma PPG (Modelo Empírico)**

| Componente | Posición ($\phi$) | Ancho ($\sigma$) | Amplitud | Justificación |
|------------|-------------------|------------------|----------|---------------|
| Sistólico | 0.15 | 0.055 | 1.0 (ref) | Pico de volumen arterial máximo |
| Diastólico | 0.40 | 0.10 | 0.4 | Onda reflejada desde periferia |
| Muesca | 0.30 | 0.02 | -0.25 | Cierre válvula aórtica |

*Nota: Parámetros ajustados empíricamente. La estructura de 3 componentes se basa en la descripción morfológica de Allen (2007).*

### 5.2 Justificación del Método

El modelo gaussiano fue seleccionado por:

1. **Simplicidad matemática**: Funciones analíticas de bajo costo computacional
2. **Flexibilidad morfológica**: Cada componente se ajusta independientemente
3. **Base fisiológica**: Pico sistólico, muesca dicrótica y pico diastólico son eventos reales
4. **Parametrización clínica**: PI, HR y forma de onda se controlan directamente

### 5.3 Diagrama de Flujo - Generación PPG

```
┌─────────────────────────────────────────────────────────────────┐
│                   INICIO generateSample(deltaTime)               │
└─────────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────────┐
│  Avanzar fase en el ciclo:                                      │
│  phaseInCycle += deltaTime / currentRR                          │
└─────────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────────┐
│  ¿phaseInCycle >= 1.0? (nuevo latido)                           │
└─────────────────────────────────────────────────────────────────┘
          │ SÍ                              │ NO
          ▼                                 │
┌──────────────────────────────┐            │
│  detectBeatAndApplyPending():│            │
│  - phaseInCycle -= 1.0       │            │
│  - beatCount++               │            │
│  - currentHR = generateDynamicHR()        │
│  - currentPI = generateDynamicPI()        │
│  - currentRR = 60 / currentHR             │
│  - Aplicar parámetros pending│            │
└──────────────────────────────┘            │
          │                                 │
          └─────────────────────────────────┤
                                            ▼
┌─────────────────────────────────────────────────────────────────┐
│  Calcular fracción sistólica según HR:                          │
│  systoleFraction = 0.3 - 0.1×(HR-60)/120  (compresión diástole) │
└─────────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────────┐
│  Generar forma de pulso normalizada [0,1]:                      │
│  pulse = computePulseShape(phaseInCycle)                        │
│                                                                 │
│  Gs = exp(-(φ - 0.15)² / (2×0.055²))      // Sistólica         │
│  Gd = exp(-(φ - 0.40)² / (2×0.10²))       // Diastólica        │
│  Gn = exp(-(φ - 0.30)² / (2×0.02²))       // Muesca            │
│  pulse = Gs + 0.4×Gd - 0.25×Gn                                  │
│  pulse = normalize(pulse)  // escalar a [0,1]                   │
└─────────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────────┐
│  Aplicar modificadores de condición:                            │
│  - WEAK_PERFUSION: dicroticDepth → 0, diastolicAmpl reducido    │
│  - VASOCONSTRICTION: forma afilada, muesca atenuada             │
│  - VASODILATION: muesca prominente, diástole marcada            │
│  - ARRHYTHMIA: HR muy variable (CV > 15%)                       │
└─────────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────────┐
│  Calcular amplitud AC según PI:                                 │
│  AC = currentPI × 15.0 mV/%                                     │
│  (PI=3% → AC=45 mV, PI=10% → AC=150 mV)                         │
└─────────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────────┐
│  Construir señal final:                                         │
│  signal = dcBaseline + pulse × AC                               │
│  signal += noise × AC × gaussian()                              │
└─────────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────────┐
│  Almacenar métricas medidas:                                    │
│  - lastACValue = pulse × AC  (para waveform Nextion)            │
│  - Detectar picos/valles para métricas                          │
└─────────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────────┐
│  RETORNAR signal (mV) y almacenar lastACValue                   │
│  signal completo: DC + AC (solo para métricas internas)         │
│  lastACValue: componente AC pura [-100, +100] mV                │
└─────────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────────┐
│              SALIDA UNIFICADA: SOLO COMPONENTE AC               │
│                                                                 │
│  Justificación: El componente DC (~1000 mV) representa la       │
│  absorción constante de luz y NO aporta información clínica.    │
│  La utilidad de la señal PPG reside en la componente pulsátil   │
│  (AC), que refleja el volumen sanguíneo y el índice de          │
│  perfusión. Al enviar solo AC, la señal del DAC es idéntica     │
│  a la visualización en Nextion.                                 │
└─────────────────────────────────────────────────────────────────┘
                               │
        ┌──────────────────────┴──────────────────────┐
        ▼                                             ▼
┌───────────────────────────────────┐   ┌───────────────────────────────────┐
│  SALIDA DAC (GPIO25)              │   │  SALIDA NEXTION WAVEFORM          │
│  Función: getDACValue()           │   │  Función: getWaveformValue()      │
│  Entrada: lastACValue (solo AC)   │   │  Entrada: lastACValue (solo AC)   │
│  Rango: 0-255 (8-bit)             │   │  Rango: 0-255 (8-bit)             │
│  Voltaje: 0-3.3V                  │   │  Altura: 20-235 px                │
│                                   │   │                                   │
│  Fórmula (solo AC):               │   │  Fórmula (solo AC):               │
│  DAC = (AC + 100) / 200 × 255     │   │  val = (AC + 100) / 200 × 255     │
│                                   │   │                                   │
│  Ejemplo (AC = +50 mV, PI≈3.3%):  │   │  Ejemplo (AC = +50 mV):           │
│  DAC = 150/200 × 255 = 191        │   │  val = 191                        │
│  Vout = 191/255 × 3.3V = 2.47V    │   │  px = map(191, 0, 255, 20, 235)   │
│                                   │   │     = 181 px                      │
│  Centro (AC=0): 127 → 1.65V       │   │  Centro: 127 px (mitad waveform)  │
└───────────────────────────────────┘   └───────────────────────────────────┘
```

### 5.4 Justificación del Límite de Amplitud AC

En el simulador propuesto, la amplitud de la componente AC de la señal PPG se deriva a partir del índice de perfusión (PI), el cual representa la relación relativa entre las componentes pulsátil (AC) y continua (DC) de la señal. Dado que el PI es una magnitud adimensional y que no existe un estándar universal que defina su correspondencia directa con amplitudes eléctricas absolutas en milivoltios, se adoptó un criterio de escalado instrumental para la generación de la señal sintética.

**Criterio de escalado adoptado:**

$$AC_{mV} = PI_{\%} \times 15 \, \text{mV/\%}$$

El rango de amplitud AC se limitó a **0–150 mV**, con el objetivo de cubrir el rango clínico típico de PI (hasta aproximadamente 10%), garantizando una adecuada visualización de la señal sin saturación en condiciones normales de operación. Este rango permite que señales con perfusión débil (PI < 1%) sean visibles, mientras que escenarios de perfusión elevada (PI 5-10%) ocupan gran parte de la escala disponible, maximizando el uso dinámico del sistema.

Para valores extremos de PI superiores al rango clínico habitual (PI > 10%, como en STRONG_PERFUSION), la amplitud es limitada mediante saturación superior (clipping), preservando la morfología de la señal. Este comportamiento reproduce el funcionamiento de sistemas reales de adquisición y visualización biomédica, donde los rangos nominales están optimizados para las condiciones más frecuentes.

**Naturaleza unipolar de la componente AC:**

Es importante destacar que la componente AC de la señal PPG es **unipolar por naturaleza**, ya que representa variaciones de absorción de luz debidas al volumen sanguíneo arterial. La señal completa se expresa como:

$$PPG(t) = DC + AC(t)$$

Donde:
- **DC** (componente continua): Absorción constante por tejido estático (~1000 mV)
- **AC(t)** (componente pulsátil): Modulación unipolar que varía entre 0 y el valor máximo determinado por el PI

La componente AC **no cruza el nivel de referencia DC**, a diferencia de señales bipolares como el ECG o el EMG crudo, que oscilan alrededor de una línea isoeléctrica. En el sistema implementado, para la visualización en Nextion se transmite únicamente la componente AC (0-150 mV), omitiendo el DC para optimizar el rango dinámico de la pantalla.

### 5.5 Variabilidad Fisiológica

Se implementó variabilidad latido-a-latido para HR y PI según Sun et al. [7]:

$$HR_{beat} = HR_{mean} + CV_{HR} \times HR_{mean} \times \mathcal{N}(0,1)$$
$$PI_{beat} = PI_{mean} + CV_{PI} \times PI_{mean} \times \mathcal{N}(0,1)$$

**Tabla 5.2: Condiciones PPG y rangos clínicos**

| Condición | HR (BPM) | PI (%) | CV_HR | CV_PI | Morfología |
|-----------|----------|--------|-------|-------|------------|
| NORMAL | 60-100 | 2-5 | 5% | 10% | Muesca visible |
| ARRHYTHMIA | 60-180 | 1-5 | 20% | 15% | RR muy irregular |
| WEAK_PERFUSION | 90-140 | 0.1-0.5 | 8% | 20% | Sin muesca |
| VASOCONSTRICTION | 60-100 | 0.2-0.8 | 7% | 15% | Forma afilada |
| STRONG_PERFUSION | 60-90 | 10-20 | 5% | 10% | AC muy alto |
| VASODILATION | 50-80 | 5-10 | 5% | 12% | Muesca prominente |

---

## 6. Validación de Señales

### 6.1 Entorno de Validación

Previo a la integración con la pantalla Nextion, todas las señales fueron validadas utilizando el archivo `main_debug.cpp` junto con el **Serial Plotter de VSCode/PlatformIO**. Este proceso permitió verificar:

1. **Morfología de las ondas**: Forma correcta de PQRST (ECG), MUAPs (EMG) y pulso (PPG)
2. **Rangos de amplitud**: Valores en mV dentro de los rangos fisiológicos esperados
3. **Temporización**: Frecuencias cardíacas, intervalos RR y duraciones correctas
4. **Condiciones patológicas**: Diferenciación clara entre condiciones normales y patológicas

### 6.2 Escalado DAC

**Tabla 6.1: Escalado DAC por señal**

| Señal | Rango entrada (mV) | Centro | Fórmula DAC |
|-------|-------------------|--------|-------------|
| ECG | -0.5 a +1.5 | 0.5 mV | $DAC = \frac{(mV + 0.5)}{2.0} \times 255$ |
| EMG | -5.0 a +5.0 | 0.0 mV | $DAC = \frac{(mV + 5.0)}{10.0} \times 255$ |
| PPG (AC) | 0 a 150 | 0 mV | $DAC = \frac{AC}{150} \times 255$ |

**Cálculos representativos de escalado DAC:**

**ECG - Pico R (+1.0 mV):**
$$DAC = \frac{(1.0 + 0.5)}{2.0} \times 255 = \frac{1.5}{2.0} \times 255 = 191$$
$$V_{out} = \frac{191}{255} \times 3.3V = 2.47V$$

**EMG - Pico contracción (+2.5 mV):**
$$DAC = \frac{(2.5 + 5.0)}{10.0} \times 255 = \frac{7.5}{10.0} \times 255 = 191$$
$$V_{out} = \frac{191}{255} \times 3.3V = 2.47V$$

**PPG - Pico sistólico (DC=1000, AC=+50 mV → 1050 mV):**
$$DAC = \frac{(1050 - 800)}{400} \times 255 = \frac{250}{400} \times 255 = 159$$
$$V_{out} = \frac{159}{255} \times 3.3V = 2.06V$$

### 6.3 Proceso de Validación

```
┌─────────────────────────────────────────────────────────────────┐
│                    FLUJO DE VALIDACIÓN                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  1. Compilar main_debug.cpp (en lugar de main.cpp)             │
│                                                                 │
│  2. Cargar firmware de debug al ESP32                          │
│                                                                 │
│  3. Abrir Serial Plotter en VSCode (115200 baud)               │
│                                                                 │
│  4. Observar morfología y métricas en tiempo real:             │
│     • Formato ECG: >ecg:VALUE,hr:VALUE,rr:VALUE,...            │
│     • Formato EMG: >emg:VALUE,rms:VALUE,exc:VALUE,...          │
│     • Formato PPG: >ppg:VALUE,hr:VALUE,pi:VALUE,...            │
│                                                                 │
│  5. Verificar rangos y formas de onda                          │
│                                                                 │
│  6. Si OK → Compilar main.cpp para producción con Nextion      │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 6.4 Salidas de Debug

**Tabla 6.2: Formato de salida serial para validación**

| Señal | Formato Serial | Variables |
|-------|----------------|-----------|
| ECG | `>ecg:V,hr:H,rr:R,qrs:Q,st:S` | Valor mV, HR, RR ms, QRS mV, ST mV |
| EMG | `>emg:V,rms:R,exc:E,mus:M` | Valor mV, RMS mV, Excitación %, MUs activas |
| PPG | `>ppg:V,hr:H,rr:R,pi:P,beats:B` | Valor mV, HR, RR ms, PI %, Beats |

> **Nota:** El archivo `main_debug.cpp` está excluido de la compilación de producción. Solo se utiliza durante el desarrollo para validar las señales antes de integrar con Nextion.

---

## 7. Resultados y Métricas

### 7.1 Resumen de Señales Implementadas

**Tabla 7.1: Características de las señales generadas**

| Característica | ECG | EMG | PPG |
|----------------|-----|-----|-----|
| **Modelo base** | McSharry ECGSYN | Fuglevand MU | Suma de Gaussianas (empírico) |
| **Formulación teórica** | Continuo (ODEs) | Continuo (procesos puntuales) | Continuo (funciones analíticas) |
| **Implementación** | Discreta (RK4) | Discreta (superposición) | Discreta (evaluación) |
| **Salida física** | Analógica continua | Analógica continua | Analógica continua |
| **fs generación** | 300 Hz | 1000 Hz | 20 Hz |
| **fs display** | 200 Hz | 100 Hz | 100 Hz |
| **Rango salida DAC** | -0.5 a +1.5 mV | -5.0 a +5.0 mV | ±100 mV (AC) |
| **Condiciones** | 8 | 6 | 6 |
| **Canales waveform** | 1 | 2 (raw + env) | 1 |
| **HRV** | Sí | N/A | Sí |
| **Calibración** | Automática (3 lat) | N/A | N/A |

> **Nota sobre discretización:** Todos los modelos son teóricamente continuos pero se ejecutan de forma discreta en el microcontrolador. La señal de salida es analógica continua tras la conversión DAC y filtrado RC.

### 7.2 Métricas Clínicas Obtenidas

**Tabla 7.2: Métricas ECG por condición (Límites de parámetros ajustables)**

| # | Condición | HR (BPM) | RR (ms) | PR (ms) | QRS (ms) | ST/T |
|---|-----------|----------|---------|---------|----------|------|
| 0 | NORMAL | 60-100 | 600-1000 | 120-200 | 80-120 | ST=0, T 0.2-0.6 mV |
| 1 | TACHYCARDIA | 100-180 | <600 | 120-200 | 80-120 | Normales |
| 2 | BRADYCARDIA | 30-59 | >1000 | 170-200 | 80-120 | Normales |
| 3 | ATRIAL_FIB | 60-180 | Irregular | — | 80-120 | Secundarios |
| 4 | VENTRICULAR_FIB | 150-500 | — | — | — | Caótico 4-10 Hz |
| 5 | AV_BLOCK_1 | 60-100 | 600-1000 | >200 | 80-120 | Normales |
| 6 | ST_ELEVATION | 50-110 | Variable | 120-200 | 80-120 | ST↑ ≥0.2 mV |
| 7 | ST_DEPRESSION | 50-150 | Variable | 120-200 | 80-120 | ST↓ 0.05-0.2 mV |

**Tabla 7.3: Métricas EMG por condición (Límites de parámetros ajustables)**

| Condición | Excitación (MVC) | MUs Activas | FR Media (Hz) | RMS Pico (mV) | Secuencia |
|-----------|------------------|-------------|---------------|---------------|-----------|
| REST | 0-10% | 0 | 0 | 0.001 | Estático (solo ruido) |
| LOW | 5-20% | 68-70 | 8-10 | 0.52 | REST 1s → LOW 3s |
| MODERATE | 20-50% | 100 | 15-17 | 1.7 | REST 1s → MOD 3s |
| HIGH | 50-100% | 100 | 31-37 | 2.8 | REST 1s → HIGH 3s |
| TREMOR | Variable | Variable | 4-6 Hz mod | 0.1-0.5 | Modulación continua |
| FATIGUE | 50% sostenido | 100 | 25 → 15 | 1.5 → 0.4 | Decay progresivo |

*Fuente: Fuglevand 1993, Cifrek 2009, Gulati & Pandey 2024*

**Tabla 7.4: Métricas PPG por condición (Límites de parámetros ajustables)**

| # | Condición | HR (BPM) | PI (%) | AC (mV) | Muesca Dicrótica |
|---|-----------|----------|--------|---------|------------------|
| 0 | NORMAL | 60-100 | 2.9-6.1 | 44-92 | Pos 20-50%, Amp ≥20% |
| 1 | ARRHYTHMIA | 50-150 | 1.0-5.0 | 15-75 | Variable |
| 2 | WEAK_PERFUSION | 90-140 | 0.3-2.0 | 5-30 | Ausente o tenue |
| 3 | VASOCONSTRICTION | 70-110 | 0.2-0.8 | 3-12 | Ausente |
| 4 | STRONG_PERFUSION | 50-90 | 10.0-20.0 | 150-300 | Muy marcada |
| 5 | VASODILATION | 60-90 | 5.0-10.0 | 75-150 | Prominente |

*Nota: Rangos basados en literatura de oximetría. Allen (2007) provee descripción morfológica; Elgendi (2012), Shelley (2007) proveen rangos de PI.*

### 7.3 Rendimiento del Sistema

**Tabla 7.5: Métricas de rendimiento**

| Métrica | Valor medido | Requerimiento |
|---------|--------------|---------------|
| Latencia generación ECG | 0.8 ms | < 2 ms ✓ |
| Latencia generación EMG | 1.2 ms | < 2 ms ✓ |
| Latencia generación PPG | 0.3 ms | < 2 ms ✓ |
| Jitter temporal | ±20 µs | < ±50 µs ✓ |
| Uso RAM | ~45 KB | < 520 KB ✓ |
| Uso Flash | ~180 KB | < 4 MB ✓ |
| Refresh Nextion | 100 Hz | ≥ 50 Hz ✓ |

---

## 8. Parámetros de Usuario y Control de Señales

### 8.1 Filosofía de Control de Parámetros

El sistema permite al usuario modificar parámetros de las señales en tiempo real a través de la interfaz Nextion. Los parámetros **inician en valores mínimos o neutros** (no en valores intermedios) para que el usuario tenga control total desde el inicio.

**Principios de diseño:**

1. **Valores iniciales mínimos**: Ruido y variabilidad inician en 0, amplitud en 100% (sin modificación)
2. **Persistencia de sliders**: Los valores de los sliders se mantienen entre sesiones, **no se leen del modelo**
3. **Aplicación explícita**: Los cambios solo se aplican al presionar el botón "Aplicar"
4. **Independencia del modelo**: La señal sigue su morfología base hasta que el usuario modifica parámetros

### 8.2 Límites de Parámetros por Condición

Los sliders de parámetros se **limitan automáticamente** según la condición/patología seleccionada. Esto garantiza coherencia fisiológica: el usuario no puede establecer valores fuera del rango clínico de la condición actual.

**Tabla 8.1: Límites de sliders ECG por condición**

| # | Condición | HR (BPM) | Amplitud (%) | Ruido (%) | HRV (%) |
|---|-----------|----------|--------------|-----------|----------|
| 0 | NORMAL | 60-100 | 50-200 | 0-10 | 1-10 |
| 1 | TACHYCARDIA | 100-180 | 50-200 | 0-10 | 1-8 |
| 2 | BRADYCARDIA | 30-59 | 50-200 | 0-10 | 1-8 |
| 3 | ATRIAL_FIB | 60-180 | 50-200 | 0-10 | 15-40 |
| 4 | VENTRICULAR_FIB | 150-500 | 50-200 | 0-10 | 0-5 |
| 5 | AV_BLOCK_1 | 60-100 | 50-200 | 0-10 | 1-10 |
| 6 | ST_ELEVATION | 50-110 | 50-200 | 0-10 | 1-10 |
| 7 | ST_DEPRESSION | 50-150 | 50-200 | 0-10 | 1-10 |

*Nota: Amplitud es factor multiplicador (100% = sin cambio). Ruido y HRV son porcentajes.*
*Fuente: Implementación en `param_limits.h` y `main.cpp`*

**Tabla 8.2: Límites de sliders EMG por condición**

| # | Condición | Excitación (%) | Amplitud (%) | Ruido (%) |
|---|-----------|----------------|--------------|------------|
| 0 | REST | 0-10 | 10-50 | 0-10 |
| 1 | LOW_CONTRACTION | 5-20 | 50-100 | 0-10 |
| 2 | MODERATE_CONTRACTION | 20-50 | 80-150 | 0-10 |
| 3 | HIGH_CONTRACTION | 50-100 | 120-250 | 0-10 |
| 4 | TREMOR | N/A (fijo) | 50-150 | 0-10 |
| 5 | FATIGUE | N/A (fijo) | 50-150 | 0-10 |

*Nota: Excitación = % MVC. Amplitud es factor × 100 (100% = sin cambio).*
*Fuente: Implementación en `param_limits.h` y `main.cpp`*

**Tabla 8.3: Límites de sliders PPG por condición**

| # | Condición | HR (BPM) | PI (slider × 10) | Amplitud (%) | Ruido (%) |
|---|-----------|----------|------------------|--------------|------------|
| 0 | NORMAL | 60-100 | 29-61 (2.9-6.1%) | 50-200 | 0-10 |
| 1 | ARRHYTHMIA | 50-150 | 10-50 (1.0-5.0%) | 50-200 | 0-10 |
| 2 | WEAK_PERFUSION | 90-140 | 3-20 (0.3-2.0%) | 50-200 | 0-10 |
| 3 | VASOCONSTRICTION | 70-110 | 2-8 (0.2-0.8%) | 50-200 | 0-10 |
| 4 | STRONG_PERFUSION | 50-90 | 100-200 (10.0-20.0%) | 50-200 | 0-10 |
| 5 | VASODILATION | 60-90 | 50-100 (5.0-10.0%) | 50-200 | 0-10 |

*Nota: PI slider envía valor × 10 (ej: slider=52 → 5.2%). Amplitud es factor × 100.*
*Fuente: Implementación en `param_limits.h` y `main.cpp`*

### 8.3 Parámetros ECG

#### 8.3.1 Frecuencia Cardíaca (HR)

**Rango:** Variable según condición (30-500 BPM)  
**Valor inicial:** 75 BPM (valor medio)  
**Unidad en slider:** BPM (entero)

**Aplicación en código:**
```cpp
// Lectura del slider (main.cpp)
int hrValue = nextion->readSliderValue("h_hr");
ecgSliderValues.hr = hrValue;

// Aplicación al modelo (main.cpp)
ECGParameters params;
params.heartRate = (float)ecgSliderValues.hr;  // Convertir a float
ecg.setParameters(params);

// Uso en modelo (ecg_model.cpp)
if (newParams.heartRate > 0) {
    hrMean = newParams.heartRate;  // Sobrescribe HR de la condición
}
```

**Comportamiento:**
- Si `heartRate = 0` (no modificado), se usa el HR de la morfología de la condición
- Si `heartRate > 0`, sobrescribe el HR base de la condición
- Afecta el intervalo RR: `RR = 60.0 / HR` (segundos)

#### 8.2.2 Factor de Amplificación

**Rango:** 50-200%  
**Valor inicial:** 100%  
**Unidad en slider:** Porcentaje (entero)

**Aplicación en código:**
```cpp
// Lectura del slider
int ampValue = nextion->readSliderValue("h_amp");
ecgSliderValues.amp = ampValue;

// Aplicación al modelo (factor de ganancia de señal)
params.waveformGain = ecgSliderValues.amp / 100.0f;  // 150 → 1.5

// Uso en modelo (ecg_model.cpp)
float amplifiedValue = rawValue * waveformGain;  // Amplificación real
```

**Comportamiento:**
- **SÍ afecta la señal generada** (factor de amplificación real)
- Amplificación 50% = señal × 0.5, 100% = señal × 1.0, 200% = señal × 2.0
- Se aplica antes del mapeo a DAC y waveform Nextion

#### 8.2.3 Ruido

**Rango:** 0-100 (representa 0.00-1.00)  
**Valor inicial:** 0 (sin ruido)  
**Unidad en slider:** Entero × 100

**Aplicación en código:**
```cpp
// Lectura del slider
int noiseValue = nextion->readSliderValue("h_noise");
ecgSliderValues.noise = noiseValue;

// Aplicación al modelo (conversión a float)
params.noiseLevel = ecgSliderValues.noise / 100.0f;  // 5 → 0.05

// Uso en modelo (ecg_model.cpp)
if (noiseLevel > 0.0f) {
    ecgMV += gaussianRandom(0.0f, noiseLevel);  // Ruido gaussiano
}
```

**Comportamiento:**
- Ruido gaussiano (media 0, desviación estándar = `noiseLevel`)
- Se añade a cada muestra generada
- Valores típicos: 0.02-0.05 (2-5 en slider) para ruido realista

#### 8.2.4 Variabilidad (HRV)

**Rango:** 0-100%  
**Valor inicial:** 0% (sin variabilidad)  
**Unidad en slider:** Porcentaje (entero)

**Aplicación en código:**
```cpp
// Lectura del slider
int hrvValue = nextion->readSliderValue("h_hrv");
ecgSliderValues.hrv = hrvValue;

// Aplicación al modelo (NO se pasa directamente, se calcula hrStd)
// hrStd = hrMean * (hrv / 100.0)
// Ejemplo: HR=75, HRV=5% → hrStd = 75 * 0.05 = 3.75 BPM

// Uso en modelo (ecg_model.cpp)
float generateNextRR() {
    float rr = gaussianRandom(60.0f / hrMean, hrStd / hrMean);  // RR con variabilidad
    return constrain(rr, rrMin, rrMax);
}
```

**Comportamiento:**
- Variabilidad latido-a-latido del intervalo RR
- HRV = 0% → intervalos RR constantes
- HRV > 0% → intervalos RR con distribución gaussiana
- Valores típicos: 3-5% para ritmo sinusal normal

### 8.4 Parámetros EMG

#### 8.4.1 Excitación

**Rango:** 0-100%  
**Valor inicial:** 0% (reposo)  
**Unidad en slider:** Porcentaje (entero)

**Aplicación en código:**
```cpp
// Lectura y aplicación
int excValue = nextion->readSliderValue("h_exc");
params.excitationLevel = excValue / 100.0f;  // 50 → 0.5

// Uso en modelo (emg_model.cpp)
int activeUnits = (int)(excitationLevel * totalUnits);  // Unidades motoras activas
for (int i = 0; i < activeUnits; i++) {
    // Generar MUAP para cada unidad activa
}
```

**Comportamiento:**
- Controla el número de unidades motoras activas (reclutamiento)
- 0% = reposo (solo ruido de fondo)
- 100% = contracción máxima (todas las unidades activas)

#### 8.4.2 Amplitud

**Rango:** 50-200%  
**Valor inicial:** 100% (sin modificación)  
**Unidad en slider:** Porcentaje (entero)

**Aplicación en código:**
```cpp
// Lectura y aplicación
int ampValue = nextion->readSliderValue("h_amp");
params.amplitude = ampValue / 100.0f;  // 150 → 1.5

// Uso en modelo (emg_model.cpp)
float emgMV = rawSignal * amplitude;  // Escalar señal
```

**Comportamiento:**
- Factor multiplicativo de la amplitud de la señal EMG
- 50% = mitad de amplitud, 200% = doble amplitud
- Afecta tanto RAW como ENV (envolvente)

#### 8.4.3 Ruido

**Rango:** 0-100 (representa 0.00-1.00)  
**Valor inicial:** 0 (sin ruido)  
**Aplicación:** Idéntica a ECG (ruido gaussiano)

### 8.5 Parámetros PPG

#### 8.5.1 Frecuencia Cardíaca (HR)

**Rango:** 30-200 BPM  
**Valor inicial:** 75 BPM  
**Aplicación:** Idéntica a ECG

#### 8.5.2 Índice de Perfusión (PI)

**Rango:** 1-200 (representa 0.1-20.0%)  
**Valor inicial:** 50 (5.0%)  
**Unidad en slider:** Entero × 10

**Aplicación en código:**
```cpp
// Lectura y aplicación
int piValue = nextion->readSliderValue("h_pi");
params.perfusionIndex = piValue / 10.0f;  // 52 → 5.2%

// Uso en modelo (ppg_model.cpp)
float acAmplitude = dcBaseline * (perfusionIndex / 100.0f);  // AC = DC × PI%
float ppgMV = dcBaseline + acAmplitude * pulseMorphology;
```

**Comportamiento:**
- PI = (AC / DC) × 100%
- PI bajo (0.1-1%) = perfusión débil (vasoconstriccion)
- PI normal (2-5%) = perfusión adecuada
- PI alto (5-20%) = perfusión fuerte (vasodilatación)

#### 8.5.3 Ruido

**Rango:** 0-100 (representa 0.00-1.00)  
**Valor inicial:** 0 (sin ruido)  
**Aplicación:** Idéntica a ECG (ruido gaussiano)

#### 8.5.4 Factor de Amplificación

**Rango:** 50-200%  
**Valor inicial:** 100%  
**Aplicación:** Factor de amplificación real aplicado a la señal AC

```cpp
// Aplicación en modelo (ppg_model.cpp)
params.amplification = ppgSliderValues.amp / 100.0f;  // 150 → 1.5
float amplifiedAC = lastACValue * params.amplification;
```

**Comportamiento:**
- **SÍ afecta la señal generada** (factor de amplificación real)
- Amplificación 50% = AC × 0.5, 100% = AC × 1.0, 200% = AC × 2.0

### 8.6 Flujo de Aplicación de Parámetros

```
1. Usuario abre popup de parámetros
   ↓
2. Sliders muestran valores actuales (NO leídos del modelo)
   ↓
3. Usuario modifica sliders
   ↓
4. Valores se guardan en estructuras temporales (ecgSliderValues, etc.)
   ↓
5. Usuario presiona "Aplicar"
   ↓
6. Valores se convierten y aplican al modelo:
   - HR: BPM → float
   - Ruido: int/100 → float (0.00-1.00)
   - Amplitud: int/100 → float (0.50-2.00)
   - PI: int/10 → float (0.1-20.0%)
   ↓
7. Modelo regenera señal con nuevos parámetros
```

### 8.7 Valores Recomendados por Condición

**ECG Normal:**
- HR: 60-100 BPM
- Ruido: 2-5 (0.02-0.05)
- HRV: 3-5%

**ECG Taquicardia:**
- HR: 100-180 BPM
- Ruido: 2-5
- HRV: 2-3%

**EMG Reposo:**
- Excitación: 0-10%
- Amplitud: 100%
- Ruido: 5-10

**EMG Contracción Máxima:**
- Excitación: 80-100%
- Amplitud: 100-150%
- Ruido: 5-10

**PPG Normal:**
- HR: 60-100 BPM
- PI: 20-50 (2.0-5.0%)
- Ruido: 2-5

---

## 9. Referencias

[1] P. E. McSharry, G. D. Clifford, L. Tarassenko, and L. A. Smith, "A dynamical model for generating synthetic electrocardiogram signals," *IEEE Trans. Biomed. Eng.*, vol. 50, no. 3, pp. 289-294, Mar. 2003. DOI: 10.1109/TBME.2003.808805

[2] A. J. Fuglevand, D. A. Winter, and A. E. Patla, "Models of recruitment and rate coding organization in motor-unit pools," *J. Neurophysiol.*, vol. 70, no. 6, pp. 2470-2488, Dec. 1993.

[3] E. Henneman, G. Somjen, and D. O. Carpenter, "Functional significance of cell size in spinal motoneurons," *J. Neurophysiol.*, vol. 28, pp. 560-580, 1965.

[4] C. J. De Luca, "The use of surface electromyography in biomechanics," *J. Appl. Biomech.*, vol. 13, no. 2, pp. 135-163, 1997.

[5] M. Cifrek, V. Medved, S. Tonković, and S. Ostojić, "Surface EMG based muscle fatigue evaluation in biomechanics," *Clin. Biomech.*, vol. 24, no. 4, pp. 327-340, May 2009.

[6] J. Allen, "Photoplethysmography and its application in clinical physiological measurement," *Physiol. Meas.*, vol. 28, no. 3, pp. R1-R39, Mar. 2007.

[7] X. Sun et al., "Beat-to-beat variability of perfusion index," *J. Clin. Monit. Comput.*, 2024.

[8] Task Force of the European Society of Cardiology and the North American Society of Pacing and Electrophysiology, "Heart rate variability: Standards of measurement, physiological interpretation, and clinical use," *Circulation*, vol. 93, no. 5, pp. 1043-1065, Mar. 1996.

[9] R. H. Clayton, A. Murray, and R. W. F. Campbell, "Frequency analysis of human ventricular fibrillation," *IEEE Trans. Biomed. Eng.*, vol. 40, no. 7, pp. 651-657, Jul. 1993.

[10] R. Merletti and P. A. Parker, *Electromyography: Physiology, Engineering, and Noninvasive Applications*. IEEE Press/Wiley-Interscience, 2004.

---

*Documento generado para BioSignalSimulator Pro*  
*Revisado: 06.01.2026*
*18 de Diciembre de 2025*
