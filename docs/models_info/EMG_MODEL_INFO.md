# Simulador EMG - Implementación y Arquitectura

## BioSimulator Pro v1.1.0

> **Documento Metodológico:** Este documento describe la implementación del simulador de EMG de superficie (sEMG) desarrollado para este proyecto, incluyendo la arquitectura computacional, las adaptaciones realizadas, y las decisiones de diseño.

---

## 1. Contexto del Desarrollo

### 1.1 Problema a Resolver

Se requería un simulador de señales EMG para un dispositivo embebido (ESP32) capaz de:
- Generar señales sEMG realistas en **tiempo real** a 1 kHz
- Simular **8 condiciones** (4 niveles de contracción + 4 patologías)
- Modelar el **reclutamiento de unidades motoras** según literatura
- Producir salida analógica mediante **DAC de 8 bits**
- Calcular métricas (**RMS, MUs activas, frecuencia de disparo**)
- Ejecutarse con recursos limitados (**520 KB RAM**)

### 1.2 Modelo Teórico de Referencia

Se eligió el modelo de **Fuglevand et al. (1993)** como base teórica porque:
- Es el estándar para simulación de pools de unidades motoras
- Implementa el principio de reclutamiento de Henneman
- Modela la relación entre excitación y frecuencia de disparo
- Está validado extensamente en literatura

**Sin embargo**, el modelo original:
- Estaba diseñado para **análisis offline** (no tiempo real)
- Modelaba **needle EMG** (amplitudes mayores que sEMG)
- No incluía **patologías** (miopatía, neuropatía, etc.)
- Requería generación de trenes de disparo pre-calculados

---

## 2. Arquitectura del Sistema Desarrollado

### 2.1 Diagrama de Arquitectura General

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    BIOSIMULATOR PRO - ARQUITECTURA EMG                       │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│   ┌──────────────┐    ┌──────────────┐    ┌──────────────┐                  │
│   │   NEXTION    │◄───│   ESP32      │───►│   DAC 8-bit  │                  │
│   │   Display    │    │   MCU        │    │   MCP4725    │                  │
│   │   (HMI)      │    │              │    │              │                  │
│   └──────────────┘    └──────┬───────┘    └──────────────┘                  │
│          ▲                   │                    │                          │
│          │                   │                    ▼                          │
│   ┌──────┴───────┐    ┌──────┴───────┐    ┌──────────────┐                  │
│   │ Métricas     │    │  EMGModel    │    │ Señal        │                  │
│   │ RMS, MUs, FR │    │  (C++)       │    │ Analógica    │                  │
│   │              │    │              │    │ 0-3.3V       │                  │
│   └──────────────┘    └──────────────┘    └──────────────┘                  │
│                                                                              │
│   DESARROLLADO EN ESTE PROYECTO:                                             │
│   ├── Pool de 100 unidades motoras en tiempo real                           │
│   ├── Distribuciones exponenciales (umbrales y amplitudes)                  │
│   ├── Generación de MUAPs con Mexican Hat Wavelet                           │
│   ├── 8 condiciones clínicas implementadas                                  │
│   ├── Adaptación de needle EMG a sEMG (atenuación tisular)                  │
│   ├── Cálculo de RMS en ventana deslizante                                  │
│   └── Sistema de escalado dinámico por condición                            │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 2.2 Flujo de Generación en Tiempo Real

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         PIPELINE DE GENERACIÓN sEMG                          │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│   ENTRADA                                                                    │
│   ───────                                                                    │
│   • Condición (0-7)                                                         │
│   • Nivel de excitación (0-1)                                               │
│                                                                              │
│         │                                                                    │
│         ▼                                                                    │
│   ┌─────────────────────────────────────────────────────────────────┐       │
│   │  1. ACTUALIZAR RECLUTAMIENTO                                    │       │
│   │     Para cada MU (i = 0..99):                                   │       │
│   │     ├── Si excitación > umbral[i] → MU reclutada               │       │
│   │     ├── Calcular FR = FR_min + gain × (E - θᵢ)                 │       │
│   │     └── Actualizar tiempo de próximo disparo                    │       │
│   └─────────────────────────────────────────────────────────────────┘       │
│         │                                                                    │
│         ▼                                                                    │
│   ┌─────────────────────────────────────────────────────────────────┐       │
│   │  2. GENERAR MUAPs                                               │       │
│   │     Para cada MU activa:                                        │       │
│   │     ├── Si tiempo >= tiempo_disparo → generar MUAP             │       │
│   │     ├── MUAP = Mexican Hat Wavelet × amplitud[i]               │       │
│   │     └── Sumar a señal total                                     │       │
│   └─────────────────────────────────────────────────────────────────┘       │
│         │                                                                    │
│         ▼                                                                    │
│   ┌─────────────────────────────────────────────────────────────────┐       │
│   │  3. APLICAR MODIFICADORES DE CONDICIÓN                          │       │
│   │     ├── Normal: sin modificación                                │       │
│   │     ├── Miopatía: MUAPs cortos, polifásicos                     │       │
│   │     ├── Neuropatía: MUAPs gigantes, pérdida de MUs             │       │
│   │     ├── Temblor: modulación sinusoidal 4-6 Hz                   │       │
│   │     └── Fasciculación: disparos espontáneos aislados            │       │
│   └─────────────────────────────────────────────────────────────────┘       │
│         │                                                                    │
│         ▼                                                                    │
│   ┌─────────────────────────────────────────────────────────────────┐       │
│   │  4. CALCULAR MÉTRICAS                                           │       │
│   │     ├── RMS en ventana deslizante (100 ms)                      │       │
│   │     ├── Contar MUs activas                                      │       │
│   │     └── Frecuencia de disparo promedio                          │       │
│   └─────────────────────────────────────────────────────────────────┘       │
│         │                                                                    │
│         ▼                                                                    │
│   ┌─────────────────────────────────────────────────────────────────┐       │
│   │  5. CONVERSIÓN DAC                                              │       │
│   │     ├── Obtener rango según condición                           │       │
│   │     ├── Mapeo bipolar: 0 mV → 128, ±max → 0/255                │       │
│   │     └── Clampeo seguro                                          │       │
│   └─────────────────────────────────────────────────────────────────┘       │
│         │                                                                    │
│         ▼                                                                    │
│   SALIDA                                                                     │
│   ──────                                                                     │
│   • Valor DAC 0-255 → Señal analógica                                       │
│   • Valor mV → Display Nextion                                              │
│   • Métricas → RMS, MUs, FR, % contracción                                  │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 3. Pool de Unidades Motoras

### 3.1 Estructura de Datos

```cpp
// Cada unidad motora tiene estos atributos
struct MotorUnit {
    float threshold;        // Umbral de reclutamiento (0-1)
    float amplitude;        // Amplitud del MUAP (mV)
    float baseAmplitude;    // Amplitud base (para restaurar)
    float firingRate;       // Frecuencia de disparo actual (Hz)
    float lastFiringTime;   // Último disparo (segundos)
    float nextFiringTime;   // Próximo disparo programado
    bool isActive;          // ¿Está reclutada?
};

// Pool completo
MotorUnit motorUnits[100];  // 100 MUs en el pool
```

### 3.2 Distribución de Umbrales (Exponencial)

```
┌─────────────────────────────────────────────────────────────────┐
│         DISTRIBUCIÓN DE UMBRALES (Henneman + Fuglevand)          │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│   Fórmula implementada:                                          │
│   ┌──────────────────────────────────────────────────────────┐  │
│   │  θᵢ = [exp(ln(RR) × i/n) - 1] / (RR - 1) × θ_max        │  │
│   │                                                          │  │
│   │  donde:                                                  │  │
│   │  • RR = 60 (Rango de Reclutamiento)                     │  │
│   │  • n = 100 (número de MUs)                              │  │
│   │  • θ_max = 0.6 (60% excitación = todas reclutadas)      │  │
│   └──────────────────────────────────────────────────────────┘  │
│                                                                  │
│   Resultado visual:                                              │
│                                                                  │
│   Umbral                                                         │
│   0.60 │                                          ●●●●●●●●●●    │
│        │                                    ●●●●●              │
│   0.40 │                              ●●●●                      │
│        │                        ●●●                              │
│   0.20 │                  ●●●                                    │
│        │            ●●●                                          │
│   0.05 │●●●●●●●●●                                                │
│        └─────────────────────────────────────────────────→ MU   │
│          0    20    40    60    80   100                        │
│                                                                  │
│   Interpretación:                                                │
│   • MUs pequeñas (Tipo I): umbral bajo → primeras en reclutar  │
│   • MUs grandes (Tipo II): umbral alto → últimas en reclutar   │
│   • Esto implementa el Principio de Henneman                    │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 3.3 Distribución de Amplitudes (Exponencial)

```
┌─────────────────────────────────────────────────────────────────┐
│         DISTRIBUCIÓN DE AMPLITUDES (adaptada para sEMG)          │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│   Fórmula implementada:                                          │
│   ┌──────────────────────────────────────────────────────────┐  │
│   │  Pᵢ = P_min × exp(ln(RR_amp) × i/n)                     │  │
│   │                                                          │  │
│   │  donde:                                                  │  │
│   │  • P_min = 0.05 mV (MUs pequeñas)                       │  │
│   │  • RR_amp = 30 (factor de rango para sEMG)              │  │
│   │  • Rango resultante: 0.05 - 1.5 mV (30×)                │  │
│   └──────────────────────────────────────────────────────────┘  │
│                                                                  │
│   DECISIÓN DE DISEÑO IMPORTANTE:                                 │
│   ────────────────────────────────                               │
│   Fuglevand usa RR_amp = 100 para needle EMG.                   │
│   Yo uso RR_amp = 30 porque:                                     │
│   • sEMG tiene atenuación tisular significativa                 │
│   • Las señales de MUs profundas se atenúan más                 │
│   • El rango 0.05-1.5 mV es consistente con literatura sEMG     │
│                                                                  │
│   Comparación:                                                   │
│   ┌────────────────────────────────────────────────────────┐    │
│   │ Tipo EMG    │ RR_amp │ Rango amplitud │ Referencia    │    │
│   ├─────────────┼────────┼────────────────┼───────────────┤    │
│   │ Needle      │ 100    │ 0.1 - 10 mV    │ Fuglevand     │    │
│   │ sEMG (mío)  │ 30     │ 0.05 - 1.5 mV  │ De Luca 1997  │    │
│   └────────────────────────────────────────────────────────┘    │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 4. Generación de MUAPs

### 4.1 Mexican Hat Wavelet (Ricker)

```
┌─────────────────────────────────────────────────────────────────┐
│                    FORMA DEL MUAP IMPLEMENTADA                   │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│   Fórmula:                                                       │
│   ┌──────────────────────────────────────────────────────────┐  │
│   │  MUAP(t) = -A × (1 - t²/σ²) × exp(-t²/2σ²) / 0.6065     │  │
│   │                                                          │  │
│   │  donde:                                                  │  │
│   │  • A = amplitud de la MU (mV)                           │  │
│   │  • σ = 2 ms (ancho de la wavelet)                       │  │
│   │  • t = tiempo desde el disparo (ms)                     │  │
│   │  • 0.6065 = factor de normalización analítico           │  │
│   │  • Signo negativo = convención EMG (pico negativo)      │  │
│   └──────────────────────────────────────────────────────────┘  │
│                                                                  │
│   Forma de onda (duración 12 ms):                               │
│                                                                  │
│   mV                                                             │
│   +A  │     ╱╲                 ╱╲                                │
│       │    ╱  ╲               ╱  ╲   ← fases positivas          │
│    0  │───╱────╲─────────────╱────╲───                          │
│       │  ╱      ╲           ╱      ╲                             │
│   -A  │ ╱        ╲─────────╱        ╲                            │
│       │           ╲       ╱           ← pico negativo principal │
│       │            ╲     ╱                                       │
│       │             ╲   ╱                                        │
│       │              ╲ ╱                                         │
│       │               V                                          │
│       └─────────────────────────────────────────→ t (ms)        │
│         -6    -3     0     3     6                               │
│                      ↑                                           │
│               Instante de disparo                                │
│                                                                  │
│   NORMALIZACIÓN (mi corrección v1.1):                           │
│   ─────────────────────────────────────                          │
│   El pico de la wavelet centrada es ≈ 0.6065                    │
│   Divido por este factor para que pico = amplitud MU            │
│   Esto corrige el factor arbitrario 0.5 que había antes         │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 4.2 Superposición en Tiempo Real

```
┌─────────────────────────────────────────────────────────────────┐
│              ALGORITMO DE SUPERPOSICIÓN (por muestra)            │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│   Para cada muestra (llamada a generateSample):                 │
│                                                                  │
│   signal = 0                                                     │
│   FOR i = 0 TO 99:                                              │
│       IF motorUnits[i].isActive:                                │
│           timeSinceFiring = currentTime - lastFiringTime[i]     │
│           IF timeSinceFiring < MUAP_DURATION:                   │
│               signal += generateMUAP(timeSinceFiring,           │
│                                       amplitude[i])              │
│           IF currentTime >= nextFiringTime[i]:                  │
│               // Programar próximo disparo                       │
│               isi = 1000 / firingRate[i]                        │
│               isi *= (1 + gaussianRandom(0, 0.20))  // 20% CV   │
│               nextFiringTime[i] = currentTime + isi/1000        │
│               lastFiringTime[i] = currentTime                   │
│   RETURN signal                                                  │
│                                                                  │
│   Visualización de superposición:                                │
│                                                                  │
│   MU#1    ╱╲           ╱╲           ╱╲                          │
│   MU#2        ╱╲               ╱╲                                │
│   MU#3              ╱╲                   ╱╲                      │
│   ──────────────────────────────────────────────                │
│   Suma   ╱╲╱╲╲  ╱╲ ╱╲╲  ╱╲╲╱╲  ╱╲╱╲  ← Señal sEMG              │
│          ╲  ╱╲╱   ╲╱  ╲╱    ╲╱    ╲╱                            │
│                                                                  │
│   Con más MUs activas → "patrón de interferencia"               │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 5. Implementación de Condiciones Clínicas

### 5.1 Sistema de Modificadores

```
┌─────────────────────────────────────────────────────────────────┐
│              CONDICIONES IMPLEMENTADAS (8 total)                 │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│   CONTRACCIONES (4):                                             │
│   ┌─────────────┬────────────┬─────────────┬──────────────────┐ │
│   │ Condición   │ Excitación │ MUs activas │ Rango pico (mV)  │ │
│   ├─────────────┼────────────┼─────────────┼──────────────────┤ │
│   │ REST        │ 0-5%       │ 0-20        │ ±0.02            │ │
│   │ LOW         │ 5-20%      │ 15-60       │ ±0.15            │ │
│   │ MODERATE    │ 20-50%     │ 50-100      │ ±0.50            │ │
│   │ HIGH        │ 50-100%    │ 85-100      │ ±2.00            │ │
│   └─────────────┴────────────┴─────────────┴──────────────────┘ │
│                                                                  │
│   PATOLOGÍAS (4):                                                │
│   ┌─────────────┬─────────────────────────┬──────────────────┐  │
│   │ Condición   │ Modificación            │ Rango pico (mV)  │  │
│   ├─────────────┼─────────────────────────┼──────────────────┤  │
│   │ TREMOR      │ Modulación 4-6 Hz       │ ±0.80            │  │
│   │ MYOPATHY    │ MUAPs cortos, pequeños  │ ±0.25            │  │
│   │ NEUROPATHY  │ MUAPs gigantes, pérdida │ ±2.50            │  │
│   │ FASCICUL.   │ Disparos espontáneos    │ ±1.00            │  │
│   └─────────────┴─────────────────────────┴──────────────────┘  │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 5.2 Miopatía (Implementación)

```
┌─────────────────────────────────────────────────────────────────┐
│                    MIOPATÍA - MI IMPLEMENTACIÓN                  │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│   Basado en: Kimura J. Electrodiagnosis. 4th ed. 2013           │
│                                                                  │
│   Modificaciones aplicadas:                                      │
│   ┌──────────────────────────────────────────────────────────┐  │
│   │  1. Amplitudes reducidas:                                │  │
│   │     amplitude = baseAmplitude × (0.25 + variabilidad)   │  │
│   │     Resultado: 25-45% del normal                        │  │
│   │                                                          │  │
│   │  2. Duración MUAP corta:                                │  │
│   │     MYOPATHY_DURATION = 6 ms (normal = 12 ms)           │  │
│   │                                                          │  │
│   │  3. Morfología polifásica:                              │  │
│   │     Generador especial con 3-5 fases                    │  │
│   │                                                          │  │
│   │  4. Reclutamiento precoz:                               │  │
│   │     threshold *= 0.6 (umbrales reducidos)               │  │
│   │     Más MUs para menos fuerza                           │  │
│   └──────────────────────────────────────────────────────────┘  │
│                                                                  │
│   Visual:                                                        │
│   NORMAL              MIOPATÍA                                   │
│      ╱╲                 ╱╲  ╱╲  ╱╲    ← Polifásico              │
│     ╱  ╲               ╱  ╲╱  ╲╱  ╲                             │
│   ─╱────╲─           ─╱──────────────╲─                         │
│    12 ms                  6 ms                                   │
│   Amp: 1.0              Amp: 0.25-0.45                          │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 5.3 Neuropatía (Implementación)

```
┌─────────────────────────────────────────────────────────────────┐
│                   NEUROPATÍA - MI IMPLEMENTACIÓN                 │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│   Basado en: Kimura J. Electrodiagnosis. 4th ed. 2013           │
│                                                                  │
│   CORRECCIÓN v1.1: Pérdida ALEATORIA (no determinística)        │
│                                                                  │
│   Modificaciones aplicadas:                                      │
│   ┌──────────────────────────────────────────────────────────┐  │
│   │  1. Pérdida aleatoria de MUs:                           │  │
│   │     FOR cada MU:                                         │  │
│   │       IF random() < 0.70:  // 70% pérdida               │  │
│   │         threshold = 2.0    // inalcanzable              │  │
│   │         amplitude = 0                                    │  │
│   │                                                          │  │
│   │  2. Reinervación de MUs sobrevivientes:                 │  │
│   │     amplitude = baseAmplitude × (3.0 + random × 2.0)    │  │
│   │     Resultado: 3-5× amplitud normal (MUAPs gigantes)    │  │
│   │                                                          │  │
│   │  3. Duración MUAP larga:                                │  │
│   │     NEUROPATHY_DURATION = 20 ms (normal = 12 ms)        │  │
│   └──────────────────────────────────────────────────────────┘  │
│                                                                  │
│   Pool de MUs:                                                   │
│   NORMAL (100 MUs)         NEUROPATÍA (~30 MUs)                 │
│   ●●●●●●●●●●●●●●●●●●●●     ○○○●○○○○●○○○●○○○○○○●                 │
│   ●●●●●●●●●●●●●●●●●●●●     ○○○○○○○○●○○○○○○●○○○○                 │
│   ●●●●●●●●●●●●●●●●●●●●     ○○○●○○○○○○○○○○○○●○○○                 │
│                            ↓                                     │
│                            MUs gigantes (reinervación)          │
│                                                                  │
│   ANTES (v1.0): pérdida i % 4 != 0 (determinística, artificial) │
│   AHORA (v1.1): pérdida aleatoria 70% (más realista)            │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 5.4 Temblor (Implementación)

```
┌─────────────────────────────────────────────────────────────────┐
│                    TEMBLOR - MI IMPLEMENTACIÓN                   │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│   Basado en: Deuschl G, et al. Mov Disord. 1998                 │
│                                                                  │
│   Implementación:                                                │
│   ┌──────────────────────────────────────────────────────────┐  │
│   │  // Modulación sinusoidal de la excitación               │  │
│   │  tremorPhase += 2π × TREMOR_FREQ × deltaTime            │  │
│   │                                                          │  │
│   │  float modulation = 0.5 + 0.5 × sin(tremorPhase)        │  │
│   │  currentExcitation = baseExcitation × modulation        │  │
│   │                                                          │  │
│   │  donde TREMOR_FREQ = 5.0 Hz (rango típico 4-6 Hz)       │  │
│   └──────────────────────────────────────────────────────────┘  │
│                                                                  │
│   Efecto visual:                                                 │
│                                                                  │
│   Excitación                                                     │
│        │      ╱╲      ╱╲      ╱╲      ╱╲                        │
│   100% │     ╱  ╲    ╱  ╲    ╱  ╲    ╱  ╲                       │
│        │    ╱    ╲  ╱    ╲  ╱    ╲  ╱    ╲                      │
│    50% │───╱──────╲╱──────╲╱──────╲╱──────╲───                  │
│        │                                                         │
│     0% │─────────────────────────────────────                   │
│        └──────────────────────────────────────→ tiempo          │
│          ←── 200 ms ──→ (5 Hz)                                  │
│                                                                  │
│   La señal EMG sube y baja rítmicamente a 4-6 Hz                │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 6. Escalado Dinámico y Salida DAC

### 6.1 Sistema de Rangos por Condición

```
┌─────────────────────────────────────────────────────────────────┐
│              ESCALADO DINÁMICO (desarrollado)                    │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│   PROBLEMA: Cada condición tiene amplitudes muy diferentes.     │
│   Si uso rango fijo, señales pequeñas serían invisibles.        │
│                                                                  │
│   SOLUCIÓN: getOutputRange() retorna rango según condición      │
│                                                                  │
│   ┌──────────────────────────────────────────────────────────┐  │
│   │  void getOutputRange(float* minMV, float* maxMV) {       │  │
│   │      switch (condition) {                                │  │
│   │          case REST:        *min=-0.02; *max=0.02; break;│  │
│   │          case LOW:         *min=-0.15; *max=0.15; break;│  │
│   │          case MODERATE:    *min=-0.50; *max=0.50; break;│  │
│   │          case HIGH:        *min=-2.00; *max=2.00; break;│  │
│   │          case TREMOR:      *min=-0.80; *max=0.80; break;│  │
│   │          case MYOPATHY:    *min=-0.25; *max=0.25; break;│  │
│   │          case NEUROPATHY:  *min=-2.50; *max=2.50; break;│  │
│   │          case FASCICUL:    *min=-1.00; *max=1.00; break;│  │
│   │      }                                                   │  │
│   │  }                                                       │  │
│   └──────────────────────────────────────────────────────────┘  │
│                                                                  │
│   Conversión DAC (señal BIPOLAR):                               │
│   ┌──────────────────────────────────────────────────────────┐  │
│   │  normalized = voltage / maxAbs   // -1 a +1              │  │
│   │  DAC = 128 + normalized × 127    // 0-255                │  │
│   │                                                          │  │
│   │  Centro: 0 mV → DAC 128                                  │  │
│   │  Máximo positivo → DAC 255                               │  │
│   │  Máximo negativo → DAC 0                                 │  │
│   └──────────────────────────────────────────────────────────┘  │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 7. Cálculo de Métricas

### 7.1 RMS en Ventana Deslizante

```cpp
// Buffer circular para RMS
float rmsBuffer[100];  // 100 ms @ 1 kHz
int rmsIndex = 0;

// Por cada muestra
rmsBuffer[rmsIndex] = sample * sample;
rmsIndex = (rmsIndex + 1) % 100;

// Calcular RMS
float sumSquares = 0;
for (int i = 0; i < 100; i++) {
    sumSquares += rmsBuffer[i];
}
float rms = sqrt(sumSquares / 100);
```

### 7.2 Conteo de MUs Activas

```cpp
int getActiveMotorUnits() {
    int count = 0;
    for (int i = 0; i < MAX_MOTOR_UNITS; i++) {
        if (motorUnits[i].isActive) count++;
    }
    return count;
}
```

---

## 8. Estructura del Código

### 8.1 Organización de Archivos

```
include/models/
└── emg_model.h          ← Constantes, estructuras, clase EMGModel

src/models/
└── emg_model.cpp        ← Implementación completa

Constantes clave (emg_model.h):
├── MAX_MOTOR_UNITS = 100
├── MUAP_AMP_MIN = 0.05f      (mV)
├── MUAP_AMP_RANGE = 30.0f    (factor exponencial)
├── FIRING_RATE_MIN = 6.0f    (Hz)
├── FIRING_RATE_MAX = 50.0f   (Hz)
├── ISI_VARIABILITY_CV = 0.20f
├── MUAP_PEAK_NORM = 0.6065f
├── NEUROPATHY_LOSS_RATE = 0.70f
└── TREMOR_FREQUENCY = 5.0f   (Hz)
```

### 8.2 Métodos Principales

```cpp
// Generación
float generateSample(float deltaTime);
uint8_t getDACValue(float deltaTime);

// Configuración
void setParameters(const EMGParameters& params);
void setCondition(EMGCondition condition);
void reset();

// Métricas
float getRMS_mV() const;
int getActiveMotorUnits() const;
float getAverageFiringRate() const;
float getContractionLevel() const;

// Rangos
void getOutputRange(float* minMV, float* maxMV) const;
```

---

## 9. Tablas Comparativas: Código vs Literatura

### 9.1 Amplitudes sEMG

| Parámetro | Mi Código | Literatura | Referencia |
|-----------|-----------|------------|------------|
| Amplitud mínima MU | 0.05 mV | 50 µV – 0.2 mV | De Luca 1997 |
| Amplitud máxima MU | 1.5 mV | 1 – 2 mV | De Luca 1997 |
| Factor de rango (RR_amp) | 30× | 100× (needle) | Fuglevand 1993 |
| sEMG típico | 0.05 – 2 mV | 50 µV – 5 mV | De Luca 1997 |
| MVC RMS | 0.5 – 1.5 mV | 1 – 5 mV | De Luca 1997 |
| Neuropatía máximo | **±2.5 mV** | 5-10 mV p-p (needle) | Kimura 2013 |

> **Justificación ±2.5 mV para neuropatía:**
> Kimura 2013 reporta MUAPs gigantes de 5-10 mV pico-a-pico en **needle EMG**.
> En sEMG, la atenuación tisular reduce esto ~50%, resultando en ±2.5 mV pico.
> La constante EMG_MAX_MV = 5.0f está **obsoleta** - usar getOutputRange().

### 9.2 Frecuencias de Disparo

| Parámetro | Mi Código | Literatura | Referencia |
|-----------|-----------|------------|------------|
| FR al reclutamiento | 6 Hz | 6-8 Hz | De Luca 2010 |
| FR máxima sostenida | 50 Hz | 30-50 Hz | De Luca 2010 |
| FR pico transitorio | - | hasta 60 Hz | De Luca 2010 |
| **Ganancia FR** | **40 Hz/unidad** | ~35-45 Hz/unidad | De Luca 2010 |
| CV del ISI | 20% | 15-25% | Literatura estándar |

> **Derivación de la ganancia FR (40 Hz/unidad):**
> 
> De Luca 2010 (Fig. 5) muestra la relación FR vs excitación:
> - FR al reclutamiento (θ): ~6-8 Hz
> - FR máxima: ~50 Hz en MVC
> - Rango de excitación útil: ~0-100% (1.0 unidad)
> 
> Cálculo: ganancia ≈ (FR_max - FR_min) / rango_excitación
>          ganancia ≈ (50 - 6) / 1.0 = 44 Hz/unidad
> 
> Uso 40 Hz/unidad como valor conservador dentro del rango 35-45 reportado.

### 9.3 Parámetros de Patologías

| Parámetro | Mi Código | Literatura | Referencia |
|-----------|-----------|------------|------------|
| **MIOPATÍA** | | | |
| Amplitud MUAP | 25-45% base | <500 µV | Kimura 2013, Cap. 13 |
| Duración MUAP | 6 ms | 4-8 ms | Kimura 2013 |
| Morfología | Polifásica 3-5 fases | Polifásica | Kimura 2013 |
| Reclutamiento | Umbral ×0.6 | Precoz | Kimura 2013 |
| **NEUROPATÍA** | | | |
| Pérdida de MUs | 70% aleatorio | 50-90% | Kimura 2013, Cap. 14 |
| **Factor reinervación** | **3-5×** | **3-10×** | **Stålberg 1982** |
| Duración MUAP | 20 ms | 15-25 ms | Kimura 2013 |
| Amplitud gigante | 3-5× base | >5 mV (needle) | Kimura 2013 |
| **TEMBLOR** | | | |
| Frecuencia | 5 Hz | 4-6 Hz | Deuschl 1998 |
| Modulación | Sinusoidal | Oscilatoria | Deuschl 1998 |
| **FASCICULACIÓN** | | | |
| Frecuencia eventos | 0.5-3 Hz | 0.1-10 Hz | Mills 2010 |
| Duración burst | 50-150 ms | Variable | Mills 2010 |

> **Referencia explícita para reinervación 3-5×:**
> 
> Stålberg E, Trontelj JV. "Single Fiber Electromyography." 
> Raven Press, New York, 1982. ISBN: 0-89004-773-5
> 
> "During reinnervation, surviving motor units may increase their 
> territory 3-10 fold, resulting in MUAP amplitudes 3-10× normal."
> 
> Uso factor 3-5× como rango conservador para sEMG (atenuación tisular).

### 9.4 Rangos de Salida por Condición

| Condición | Rango Pico (mV) | RMS Esperado (mV) | Justificación |
|-----------|-----------------|-------------------|---------------|
| REST | ±0.02 | <0.02 | Ruido fondo <50 µV (De Luca 1997) |
| LOW | ±0.15 | 0.02-0.10 | 5-20% MVC (De Luca 1997) |
| MODERATE | ±0.50 | 0.10-0.40 | 20-50% MVC (De Luca 1997) |
| HIGH | ±2.00 | 0.50-1.50 | 50-100% MVC (De Luca 1997) |
| TREMOR | ±0.80 | 0.10-0.50 | Modulación 4-6 Hz (Deuschl 1998) |
| MYOPATHY | ±0.25 | 0.05-0.15 | MUAPs <500 µV (Kimura 2013) |
| NEUROPATHY | ±2.50 | 0.30-1.50 | Gigantes atenuados sEMG (Kimura 2013) |
| FASCICULATION | ±1.00 | 0.10-0.50 | Descargas aisladas (Mills 2010) |

---

## 10. Resumen de Contribuciones

| Componente | Base Teórica | Mi Contribución |
|------------|--------------|-----------------|
| Pool de MUs | Fuglevand 1993 | Implementación tiempo real C++ |
| Umbrales exponenciales | Fuglevand 1993 | Código optimizado para ESP32 |
| Amplitudes exponenciales | Fuglevand 1993 | **Adaptación sEMG (RR=30)** |
| Frecuencias de disparo | De Luca 2010 | Variabilidad ISI gaussiana |
| Mexican Hat MUAP | Literatura estándar | **Normalización correcta (0.6065)** |
| Miopatía | Kimura 2013 | Implementación con MUAPs polifásicos |
| Neuropatía | Kimura 2013 | **Pérdida aleatoria + reinervación** |
| Temblor | Deuschl 1998 | Modulación sinusoidal 4-6 Hz |
| Fasciculación | Mills 2010 | Disparos espontáneos aislados |
| Escalado dinámico | **Nuevo** | Rangos por condición |
| RMS en tiempo real | **Nuevo** | Ventana deslizante 100 ms |

---

## 11. Flujo Completo del Modelo EMG

> **Guía Didáctica:** Este es el viaje de una señal EMG desde el encendido hasta la salida del DAC. Cada paso incluye las funciones del código, la matemática involucrada, y el proceso lógico.

### PASO 1: INICIO - Constructor y Reset

**Cuándo ocurre:** Al encender el sistema, se crea el modelo desde cero.

**Funciones involucradas:**
```cpp
EMGModel::EMGModel()           // Constructor
void EMGModel::reset()         // Limpia todo
void EMGModel::initializeMotorUnits()  // Crea las 100 MUs
```

**Proceso:**
1. `EMGModel()` inicializa variables a 0
2. `reset()` limpia todo (tiempo, excitación, buffers)
3. `initializeMotorUnits()` crea las 100 unidades motoras

**Matemática - Distribución de Umbrales (Henneman + Fuglevand):**
```
θᵢ = [exp(ln(RR) × i/n) - 1] / (RR - 1) × θ_max

donde:
  RR = 60 (Rango de Reclutamiento)
  n = 100 (número de MUs)
  θ_max = 0.6 (60% excitación = todas reclutadas)
```

**Matemática - Distribución de Amplitudes:**
```
Pᵢ = P_min × exp(ln(RR_amp) × i/n)

donde:
  P_min = 0.05 mV
  RR_amp = 30 (adaptado para sEMG, no 100 como needle)
```

**Resultado:** 100 MUs con threshold (umbral), amplitude (fuerza), isActive = false

---

### PASO 2: CONFIGURACIÓN - setParameters()

**Cuándo ocurre:** El usuario elige la condición (reposo, contracción, patología).

**Funciones involucradas:**
```cpp
void EMGModel::setParameters(const EMGParameters& params)
void EMGModel::resetMotorUnitsToDefault()
void EMGModel::applyConditionModifiers()
```

**Proceso:**
1. `params.condition` determina QUÉ simular (ej: `EMGCondition::MYOPATHY`)
2. `resetMotorUnitsToDefault()` restaura las 100 MUs a valores normales
3. `applyConditionModifiers()` MODIFICA las MUs según la condición:

**Modificadores por Condición:**

| Condición | Modificación | Código |
|-----------|--------------|--------|
| REST | excitación = 3% | `currentExcitation = 0.03f` |
| LOW | excitación = 15% | `currentExcitation = 0.15f` |
| MODERATE | excitación = 35% | `currentExcitation = 0.35f` |
| HIGH | excitación = 80% | `currentExcitation = 0.80f` |
| MYOPATHY | amplitudes ×0.25-0.45, umbral ×0.6 | Reclutamiento precoz |
| NEUROPATHY | 70% MUs muertas, sobrevivientes ×3-5 | MUAPs gigantes |
| TREMOR | prepara fase para modulación 5 Hz | `tremorPhase = 0` |
| FASCICULATION | prepara timer aleatorio | `nextFasciculationTime` |

**Matemática - Neuropatía (pérdida aleatoria):**
```cpp
for (int i = 0; i < MAX_MOTOR_UNITS; i++) {
    if (randomFloat() < 0.70f) {  // 70% pérdida ALEATORIA
        motorUnits[i].threshold = 2.0f;  // Inalcanzable
        motorUnits[i].amplitude = 0.0f;
    } else {
        // Reinervación: amplitud ×3-5
        float reinervationFactor = 3.0f + randomFloat() * 2.0f;
        motorUnits[i].amplitude *= reinervationFactor;
    }
}
```

---

### PASO 3: BUCLE - generateSample() cada 1ms

**Cuándo ocurre:** Se llama 1000 veces por segundo (1 kHz).

**Función principal:**
```cpp
float EMGModel::generateSample(float deltaTime)
```

**Proceso:**
1. `accumulatedTime += deltaTime` (reloj interno avanza)
2. Aplicar variabilidad de fuerza: excitación oscila ±4% (control motor natural)
3. Si TREMOR: modular excitación sinusoidalmente a 5 Hz
4. Si FASCICULATION: disparar MUs al azar cada 0.3-1.5 segundos
5. `updateMotorUnitRecruitment()` decide quién se activa
6. Sumar MUAPs de todas las MUs activas
7. Retornar señal en mV

**Matemática - Variabilidad de Fuerza:**
```
excitación_efectiva = currentExcitation × (1 + 0.04 × sin(2π × 2Hz × t))
```

**Matemática - Modulación Temblor:**
```
modulación = 0.5 + 0.5 × sin(2π × 5Hz × t)
excitación_modulada = excitación × modulación
```

---

### PASO 4: RECLUTAMIENTO - ¿Quién dispara?

**Cuándo ocurre:** Dentro de `generateSample()`, para cada muestra.

**Función:**
```cpp
void EMGModel::updateMotorUnitRecruitment()
```

**Proceso - Principio de Henneman:**
```
Para cada MU (i = 0 a 99):
  SI currentExcitation >= motorUnits[i].threshold:
    → MU se activa (isActive = true)
    → Calcula firing rate
    → Programa próximo disparo
  SI NO:
    → MU se desactiva (isActive = false)
```

**Matemática - Frecuencia de Disparo (De Luca 2010):**
```
FR = FR_min + gain × (E - θᵢ)

donde:
  FR_min = 6 Hz (frecuencia al reclutamiento)
  gain = 40 Hz/unidad (derivado de De Luca 2010)
  E = excitación actual (0-1)
  θᵢ = umbral de la MU i
  
Ejemplo: E = 0.5, θᵢ = 0.2
  FR = 6 + 40 × (0.5 - 0.2) = 6 + 12 = 18 Hz
```

**Ejemplo de Reclutamiento:**
| Excitación | MUs Activas | Descripción |
|------------|-------------|-------------|
| 5% | 0-10 | Reposo, solo MUs pequeñas |
| 20% | 0-40 | Contracción baja |
| 50% | 0-80 | Contracción moderada |
| 80% | 0-95 | Contracción alta |

---

### PASO 5: SUMA DE MUAPs - Construir la señal

**Cuándo ocurre:** Después del reclutamiento, en cada muestra.

**Proceso:**
```cpp
float signal = 0.0f;

for (int i = 0; i < MAX_MOTOR_UNITS; i++) {
    if (!motorUnits[i].isActive) continue;
    
    // ¿Es tiempo de disparar?
    if (accumulatedTime >= motorUnits[i].nextFiringTime) {
        motorUnits[i].lastFiringTime = accumulatedTime;
        
        // Calcular ISI con variabilidad
        float isi = 1000.0f / motorUnits[i].firingRate;  // ms
        isi *= (1.0f + gaussianRandom(0, ISI_VARIABILITY_CV));
        motorUnits[i].nextFiringTime = accumulatedTime + isi/1000.0f;
    }
    
    // Calcular contribución del MUAP
    float timeSinceFiring = accumulatedTime - motorUnits[i].lastFiringTime;
    if (timeSinceFiring < MUAP_DURATION/1000.0f) {
        signal += generateMUAP(timeSinceFiring * 1000.0f, 
                               motorUnits[i].amplitude);
    }
}
```

**Matemática - Variabilidad del ISI:**
```
ISI_real = ISI_base × (1 + ruido)

donde:
  ISI_base = 1000/FR ms
  ruido ~ N(0, 0.20)  // CV = 20%
```

---

### PASO 6: FORMA DEL MUAP - Mexican Hat Wavelet

**Cuándo ocurre:** Cada vez que una MU está en su ventana de disparo.

**Función:**
```cpp
float EMGModel::generateMUAP(float timeMs, float amplitude)
```

**Matemática - Segunda Derivada de Gaussiana (Ricker Wavelet):**
```
MUAP(t) = -A × (1 - t²/σ²) × exp(-t²/2σ²) / 0.6065

donde:
  A = amplitud de la MU (mV)
  σ = 2 ms (controla ancho del MUAP)
  t = tiempo desde disparo, centrado en 0
  0.6065 = factor de normalización analítico
```

**Parámetros por Condición:**
| Condición | Duración | σ (ms) | Morfología |
|-----------|----------|--------|------------|
| Normal | 12 ms | 2.0 | Trifásica |
| Miopatía | 6 ms | 1.0 | Polifásica (3-5 fases) |
| Neuropatía | 20 ms | 4.0 | Ancha, gigante |

**Forma Visual:**
```
      Voltaje
        +    ╱╲                 ╱╲
             ╱  ╲               ╱  ╲    ← fases positivas
        0  ─╱────╲─────────────╱────╲─
            ╱      ╲           ╱
        -   ╱        ╲─────────╱       ← pico negativo principal
                      ╲       ╱
                       ╲     ╱
                        ╲   ╱
                         ╲ ╱
                          V
            └──────────────────────────→ tiempo
              -6 ms    0    +6 ms
```

---

### PASO 7: SALIDA - Convertir a DAC

**Cuándo ocurre:** Al final de `generateSample()`.

**Funciones:**
```cpp
uint8_t EMGModel::getDACValue(float deltaTime)
uint8_t EMGModel::voltageToDACValue(float voltage)
void EMGModel::getOutputRange(float* minMV, float* maxMV)
```

**Proceso:**
1. `signal *= params.amplitude` (ganancia del usuario)
2. `signal += gaussianRandom(0, noiseLevel)` (~0.01 mV ruido)
3. `getOutputRange()` determina escala según condición
4. `voltageToDACValue()` mapea mV a 0-255

**Matemática - Mapeo Bipolar a DAC:**
```
normalized = voltage / maxAbs      // Resultado: -1 a +1
DAC = 128 + round(normalized × 127) // Resultado: 0-255

Mapeo:
  0 mV   → DAC 128 (centro)
  +max mV → DAC 255
  -max mV → DAC 0
```

**Rangos por Condición:**
| Condición | Rango (mV) | 0.5 mV → DAC |
|-----------|------------|--------------|
| REST | ±0.02 | Saturado (255) |
| LOW | ±0.15 | ~212 |
| MODERATE | ±0.50 | 191 |
| HIGH | ±2.00 | 160 |
| NEUROPATHY | ±2.50 | 153 |

---

### 🔁 RESUMEN DEL CICLO

```
┌─────────────────────────────────────────────────────────────────┐
│                    CICLO DE VIDA EMG                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  1️⃣ UNA VEZ AL INICIO:                                          │
│     Constructor → Reset → Inicializar 100 MUs                   │
│                                                                  │
│  2️⃣ CUANDO CAMBIAS CONDICIÓN:                                   │
│     setParameters() → Reset MUs → Aplicar Modificadores         │
│                                                                  │
│  3️⃣ CADA 1 ms (BUCLE INFINITO):                                 │
│     generateSample() → Reclutamiento → Suma MUAPs → DAC         │
│                                                                  │
│  🎯 SALIDA:                                                      │
│     1000 valores/segundo que simulan EMG real                   │
│     Rango: 0-255 para DAC, mV para display                      │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 12. Referencias

| Ref | Cita Completa | Uso en este Trabajo |
|-----|---------------|---------------------|
| [1] | Fuglevand AJ, Winter DA, Patla AE. "Models of recruitment and rate coding organization in motor-unit pools." J Neurophysiol. 1993;70(6):2470-2488. DOI: 10.1152/jn.1993.70.6.2470 | Modelo base, umbrales exponenciales |
| [2] | Henneman E, Somjen G, Carpenter DO. "Functional significance of cell size in spinal motoneurons." J Neurophysiol. 1965;28:560-580. DOI: 10.1152/jn.1965.28.3.560 | Principio de reclutamiento |
| [3] | De Luca CJ, Hostage EC. "Relationship between firing rate and recruitment threshold." J Neurophysiol. 2010;104(2):1034-1046. DOI: 10.1152/jn.01018.2009 | Frecuencias de disparo, ganancia FR |
| [4] | De Luca CJ. "The use of surface electromyography in biomechanics." J Appl Biomech. 1997;13(2):135-163. DOI: 10.1123/jab.13.2.135 | Amplitudes sEMG típicas |
| [5] | Kimura J. "Electrodiagnosis in Diseases of Nerve and Muscle." 4th ed. Oxford University Press, 2013. ISBN: 978-0199738687 | Miopatía, neuropatía (Cap. 13-14) |
| [6] | Stålberg E, Trontelj JV. "Single Fiber Electromyography." Raven Press, 1982. ISBN: 0-89004-773-5 | **Factor reinervación 3-10×** |
| [7] | Deuschl G, Bain P, Brin M. "Consensus statement on Tremor." Mov Disord. 1998;13(S3):2-23. DOI: 10.1002/mds.870131303 | Frecuencia temblor 4-6 Hz |
| [8] | Mills KR. "Detecting fasciculations in amyotrophic lateral sclerosis." J Neurol Neurosurg Psychiatry. 2010;81(5):550-555. | Fasciculaciones |

---

*BioSimulator Pro - Documento Metodológico EMG v1.1.0*
