# Metodología de Arquitectura Computacional del Sistema

**BioSimulator Pro v1.0.0**  
**Fecha:** 18 de Diciembre de 2025  
**Documento de Sustentación Técnica**

---

## Índice

1. [Introducción](#1-introducción)
2. [Arquitectura de Hardware](#2-arquitectura-de-hardware)
3. [Arquitectura de Software Embebido](#3-arquitectura-de-software-embebido)
4. [Motor de Generación de Señales](#4-motor-de-generación-de-señales)
5. [Subsistema de Visualización Nextion](#5-subsistema-de-visualización-nextion)
6. [Subsistema de Comunicación WiFi](#6-subsistema-de-comunicación-wifi)
7. [Flujo de Datos Integrado](#7-flujo-de-datos-integrado)
8. [Cálculos Computacionales Representativos](#8-cálculos-computacionales-representativos)
9. [Referencias](#9-referencias)

---

## 1. Introducción

Este documento describe la arquitectura computacional del sistema BioSimulator Pro, abarcando desde la distribución de tareas en el microcontrolador ESP32 hasta la comunicación con interfaces externas (pantalla Nextion táctil y aplicación web vía WiFi). El objetivo fue diseñar un sistema capaz de generar señales biológicas sintéticas en tiempo real con determinismo temporal, mientras se mantenía una interfaz de usuario responsiva y se habilitaba el streaming de datos hacia dispositivos externos.

La arquitectura se fundamentó en tres principios:

1. **Separación de dominios temporales**: La generación de señales operó en tiempo real estricto (ISR + Core 1), mientras que la interfaz de usuario y comunicaciones operaron con requisitos de tiempo más relajados (Core 0).

2. **Buffering para desacoplamiento**: Un buffer circular en RAM rápida permitió que la generación de muestras y la salida DAC operaran de forma asíncrona, absorbiendo variaciones temporales.

3. **Downsampling selectivo**: Las interfaces de visualización recibieron datos a tasas reducidas respecto a la generación interna, optimizando el ancho de banda sin comprometer la fidelidad perceptual.

---

## 2. Arquitectura de Hardware

### 2.1 Plataforma de Procesamiento

Se seleccionó el **ESP32-WROOM-32** como unidad de procesamiento central por las siguientes características:

| Característica | Valor | Justificación |
|----------------|-------|---------------|
| CPU | Dual-core Xtensa LX6 @ 240 MHz | Procesamiento paralelo: UI en Core 0, señales en Core 1 |
| SRAM | 520 KB | Buffers de señal (2048 × 3 bytes) y estados de modelos |
| Flash | 4 MB | Firmware, tablas precalculadas y configuración |
| DAC | 2 canales × 8-bit | Salida analógica 0–3.3 V en GPIO25 |
| UART | 3 puertos | UART0 (debug), UART2 (Nextion) |
| WiFi | 802.11 b/g/n | Access Point para streaming WebSocket |
| FreeRTOS | Integrado | Gestión de tareas con prioridades |

### 2.2 Periféricos del Sistema

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         PERIFÉRICOS DEL SISTEMA                              │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────┐     ┌─────────────────┐     ┌─────────────────────┐   │
│  │   ESP32-WROOM   │     │  Nextion 7"     │     │   Cliente WiFi      │   │
│  │   (MCU Central) │     │  NX8048T070     │     │   (Navegador Web)   │   │
│  └────────┬────────┘     └────────┬────────┘     └──────────┬──────────┘   │
│           │                       │                         │              │
│           │ GPIO25 (DAC)          │ UART2                   │ WiFi AP      │
│           │ 0-3.3V, 8-bit         │ 115200 baud             │ WebSocket    │
│           ▼                       ▼                         ▼              │
│  ┌─────────────────┐     ┌─────────────────┐     ┌─────────────────────┐   │
│  │  TL072 Buffer   │     │  Waveform       │     │   App Web           │   │
│  │  + Conector BNC │     │  700×380 px     │     │   (React/HTML5)     │   │
│  │  Salida 0-3.3V  │     │  Táctil         │     │   Tiempo Real       │   │
│  └─────────────────┘     └─────────────────┘     └─────────────────────┘   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 3. Arquitectura de Software Embebido

### 3.1 Diagrama de Arquitectura Completa

El siguiente diagrama muestra la arquitectura computacional completa del sistema, incluyendo la distribución de tareas entre núcleos, el flujo de datos hacia el DAC, la pantalla Nextion y los clientes WiFi:

```
┌─────────────────────────────────────────────────────────────────────────────────────────────┐
│                           BioSimulator Pro v1.0.0 - ARQUITECTURA COMPLETA                    │
│                              ESP32 Dual-Core + WiFi AP + Nextion                             │
├─────────────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                             │
│  ┌─────────────────────────────────────┐    ┌─────────────────────────────────────────────┐ │
│  │           CORE 0 (PRO)              │    │              CORE 1 (APP)                   │ │
│  │     Interfaz y Comunicación         │    │        Generación Tiempo Real              │ │
│  │         Prioridad: 1-2              │    │            Prioridad: 5                    │ │
│  ├─────────────────────────────────────┤    ├─────────────────────────────────────────────┤ │
│  │                                     │    │                                             │ │
│  │  ┌───────────────────────────────┐  │    │  ┌─────────────────────────────────────┐    │ │
│  │  │   loop() - Main Task          │  │    │  │   generationTask (FreeRTOS)         │    │ │
│  │  │   Prioridad: 1                │  │    │  │   Prioridad: 5 (máxima)             │    │ │
│  │  │                               │  │    │  │   Stack: 4096 bytes                 │    │ │
│  │  │   Cada 10 ms:                 │  │    │  │   Bucle continuo:                   │    │ │
│  │  │   • nextion.process()         │  │    │  │   • Verificar estado RUNNING        │    │ │
│  │  │   • Parsear eventos táctiles  │  │    │  │   • Calcular espacio en buffer      │    │ │
│  │  │   • stateMachine.update()     │  │    │  │   • Generar muestras + interpolar   │    │ │
│  │  │                               │  │    │  │   • Escribir en buffer circular     │    │ │
│  │  │   Waveform (contador ticks):  │  │    │  │   • vTaskDelay(1) yield             │    │ │
│  │  │   • sampleCount % ratio == 0  │  │    │  └─────────────────────────────────────┘    │ │
│  │  │   • addWaveformPoint()        │  │    │                    │                        │ │
│  │  │                               │  │    │                    ▼                        │ │
│  │  │   Cada METRICS_UPDATE_MS:     │  │    │  ┌─────────────────────────────────────┐    │ │
│  │  │   • updateMetrics()           │  │    │  │   Modelos de Señal                  │    │ │
│  │  └───────────────────────────────┘  │    │  │                                     │    │ │
│  │              │                      │    │  │   ECGModel   EMGModel   PPGModel    │    │ │
│  │              ▼                      │    │  │   ┌──────┐   ┌──────┐   ┌──────┐    │    │ │
│  │  ┌───────────────────────────────┐  │    │  │   │RK4   │   │MU    │   │Gauss │    │    │ │
│  │  │   WiFiServer_BioSim           │  │    │  │   │McSharry│ │Fugle-│   │Allen │    │    │ │
│  │  │   (WiFi AP + WebSocket)       │  │    │  │   │      │   │vand  │   │      │    │    │ │
│  │  │                               │  │    │  │   └──────┘   └──────┘   └──────┘    │    │ │
│  │  │   AP: BioSimulator_Pro        │  │    │  └─────────────────────────────────────┘    │ │
│  │  │   Pass: biosignal123          │  │    │                    │                        │ │
│  │  │   IP: 192.168.4.1             │  │    │                    ▼                        │ │
│  │  │   Puerto WS: 81               │  │    │  ┌─────────────────────────────────────┐    │ │
│  │  │                               │  │    │  │   Buffer Circular (DRAM_ATTR)       │    │ │
│  │  │   Cada WS_SEND_INTERVAL_MS:   │  │    │  │   Tamaño: 2048 muestras             │    │ │
│  │  │   • sendSignalData() (100 Hz) │  │    │  │   Tipo: uint8_t (0-255)             │    │ │
│  │  └───────────────────────────────┘  │    │  │                                     │    │ │
│  │                                     │    │  │   writeIndex ──► [████████░░░░]     │    │ │
│  │                                     │    │  │                   readIndex ──┘     │    │ │
│  │                                     │    │  │   Acceso ISR-safe (volátil)         │    │ │
│  │                                     │    │  └─────────────────────────────────────┘    │ │
│  └─────────────────────────────────────┘    │                    │                        │ │
│                                             │                    ▼                        │ │
│                                             │  ┌─────────────────────────────────────┐    │ │
│                                             │  │   Timer ISR (hw_timer_t)            │    │ │
│                                             │  │   Período: 0.25 ms (4 kHz)          │    │ │
│                                             │  │   IRAM_ATTR (ejecución rápida)      │    │ │
│                                             │  │                                     │    │ │
│                                             │  │   timerISR():                       │    │ │
│                                             │  │   • Leer buffer[readIndex]          │    │ │
│                                             │  │   • dacWrite(GPIO25, value)         │    │ │
│                                             │  │   • readIndex = (readIndex+1) % N   │    │ │
│                                             │  │   • Detectar underruns              │    │ │
│                                             │  └─────────────────────────────────────┘    │ │
│                                             └─────────────────────────────────────────────┘ │
│                                                                                             │
├─────────────────────────────────────────────────────────────────────────────────────────────┤
│                                    SALIDAS DEL SISTEMA                                      │
├─────────────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                             │
│  ┌───────────────────────┐   ┌───────────────────────┐   ┌─────────────────────────────┐   │
│  │   DAC (GPIO25)        │   │   NEXTION UART2       │   │   WiFi WebSocket            │   │
│  │   Salida Analógica    │   │   Display Táctil      │   │   Streaming Remoto          │   │
│  ├───────────────────────┤   ├───────────────────────┤   ├─────────────────────────────┤   │
│  │                       │   │                       │   │                             │   │
│  │  Actualización: 4 kHz │   │  Waveform: 100-200 Hz │   │  Señal: 100 Hz (10 ms)      │   │
│  │  Resolución: 8-bit    │   │  Métricas: 4 Hz       │   │  Métricas: 4 Hz (250 ms)    │   │
│  │  Rango: 0-3.3 V       │   │  Baudios: 115200      │   │  Formato: JSON              │   │
│  │  Jitter: < ±50 µs     │   │  Área: 700×380 px     │   │  Max clientes: 4            │   │
│  │                       │   │                       │   │                             │   │
│  │  Función:             │   │  Funciones:           │   │  Funciones:                 │   │
│  │  dacWrite(pin, val)   │   │  addWaveformPoint()   │   │  sendSignalData()           │   │
│  │                       │   │  updateMetrics()      │   │  sendMetrics()              │   │
│  │                       │   │  goToPage()           │   │  broadcastState()           │   │
│  └───────────────────────┘   └───────────────────────┘   └─────────────────────────────┘   │
│           │                           │                             │                      │
│           ▼                           ▼                             ▼                      │
│  ┌───────────────────────┐   ┌───────────────────────┐   ┌─────────────────────────────┐   │
│  │   Conector BNC        │   │   Pantalla 7"         │   │   Navegador Web             │   │
│  │   → Osciloscopio      │   │   800×480 px          │   │   http://192.168.4.1        │   │
│  │   → Equipo médico     │   │   Táctil capacitivo   │   │   Waveform Canvas           │   │
│  │   → ADC externo       │   │   Controles UI        │   │   Métricas tiempo real      │   │
│  └───────────────────────┘   └───────────────────────┘   └─────────────────────────────┘   │
│                                                                                             │
└─────────────────────────────────────────────────────────────────────────────────────────────┘
```

### 3.2 Distribución de Tareas por Núcleo

La distribución de tareas entre los dos núcleos del ESP32 se diseñó para aislar completamente las operaciones de tiempo real de las operaciones de interfaz:

**Tabla 3.1: Asignación de tareas por núcleo**

| Núcleo | Tarea | Función Principal | Período | Prioridad | Stack |
|--------|-------|-------------------|---------|-----------|-------|
| Core 0 | loop() | Nextion, UI, WiFi | Continuo | 1 | Default |
| Core 0 | WiFi | AP + WebSocket server | Asíncrono | 1 | 4096 |
| Core 1 | generationTask | Generación de muestras | 1 ms | 5 | 4096 |
| Core 1 | Timer ISR | Salida DAC | 1 ms (1 kHz) | Máxima | ISR |

**Tabla 3.2: Constantes de configuración (config.h)**

| Constante | Valor | Descripción |
|-----------|-------|-------------|
| `CORE_SIGNAL_GENERATION` | 1 | Núcleo para generación |
| `CORE_UI_COMMUNICATION` | 0 | Núcleo para UI/WiFi |
| `TASK_PRIORITY_SIGNAL` | 5 | Prioridad máxima de tarea |
| `STACK_SIZE_SIGNAL` | 4096 | Stack de tarea de señal |
| `SAMPLE_RATE_HZ` | 1000 | Timer maestro DAC (1 kHz) |
| `SIGNAL_BUFFER_SIZE` | 2048 | Tamaño buffer circular |

### 3.3 Recursos Compartidos y Sincronización

Los recursos compartidos entre tareas se protegieron mediante semáforos FreeRTOS:

```
┌─────────────────────────────────────────────────────────────────┐
│                    RECURSOS COMPARTIDOS                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌──────────────────┐  ┌──────────────────┐  ┌────────────────┐ │
│  │  StateMachine    │  │  ParamController │  │  SignalEngine  │ │
│  │                  │  │                  │  │                │ │
│  │  Estados:        │  │  Parámetros:     │  │  Singleton     │ │
│  │  • INIT          │  │  • Tipo A (inm)  │  │  getInstance() │ │
│  │  • PORTADA       │  │  • Tipo B (diff) │  │                │ │
│  │  • MENU          │  │                  │  │  Mutex:        │ │
│  │  • SELECT_COND   │  │  Validación:     │  │  signalMutex   │ │
│  │  • SIMULATING    │  │  • Rangos        │  │                │ │
│  │  • PAUSED        │  │  • Límites       │  │                │ │
│  └──────────────────┘  └──────────────────┘  └────────────────┘ │
│                                                                 │
│  Sincronización:                                                │
│  • Buffer circular: índices volátiles (ISR-safe)                │
│  • Parámetros: aplicación diferida en fin de ciclo              │
│  • Estado: transiciones atómicas con mutex                      │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 4. Motor de Generación de Señales

### 4.1 Arquitectura del SignalEngine

El `SignalEngine` se implementó como un singleton que coordina la generación de muestras y la salida DAC:

```
┌─────────────────────────────────────────────────────────────────┐
│                      SignalEngine (Singleton)                    │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Entrada:                         Salida:                       │
│  ┌─────────────────┐              ┌─────────────────┐           │
│  │ startSignal()   │              │ DAC GPIO25      │           │
│  │ stopSignal()    │              │ 0-255 (8-bit)   │           │
│  │ setParameters() │              │ 4 kHz update    │           │
│  └────────┬────────┘              └────────▲────────┘           │
│           │                                │                    │
│           ▼                                │                    │
│  ┌─────────────────────────────────────────┴──────────────────┐ │
│  │                    generationTask()                         │ │
│  │                                                             │ │
│  │   while (true) {                                            │ │
│  │       if (state == RUNNING) {                               │ │
│  │           available = calcBufferSpace();                    │ │
│  │           while (available > 0) {                           │ │
│  │               sample = generateSample();  // Modelo activo  │ │
│  │               buffer[writeIdx] = sample;                    │ │
│  │               writeIdx = (writeIdx + 1) % BUFFER_SIZE;      │ │
│  │               available--;                                  │ │
│  │           }                                                 │ │
│  │       }                                                     │ │
│  │       vTaskDelay(1);  // Yield a otras tareas               │ │
│  │   }                                                         │ │
│  └─────────────────────────────────────────────────────────────┘ │
│                              │                                  │
│                              ▼                                  │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │              Buffer Circular (DRAM_ATTR)                    │ │
│  │              uint8_t signalBuffer[2048]                     │ │
│  │                                                             │ │
│  │   [0][1][2][3]...[writeIdx]...[readIdx]...[2047]           │ │
│  │                      ▲              │                       │ │
│  │                      │              ▼                       │ │
│  │               generationTask    timerISR                    │ │
│  └─────────────────────────────────────────────────────────────┘ │
│                              │                                  │
│                              ▼                                  │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │                      timerISR() @ 4 kHz                     │ │
│  │                                                             │ │
│  │   void IRAM_ATTR timerISR() {                               │ │
│  │       if (readIdx != writeIdx) {                            │ │
│  │           dacWrite(GPIO25, buffer[readIdx]);                │ │
│  │           readIdx = (readIdx + 1) % BUFFER_SIZE;            │ │
│  │       } else {                                              │ │
│  │           bufferUnderruns++;  // Estadística                │ │
│  │       }                                                     │ │
│  │   }                                                         │ │
│  └─────────────────────────────────────────────────────────────┘ │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 4.2 Timer de Hardware y ISR

El timer de hardware se configuró para generar interrupciones a 4 kHz (Fs_timer):

```cpp
// Configuración del timer (signal_engine.cpp)
void SignalEngine::setupTimer() {
    // Timer a Fs_timer (4 kHz)
    // Criterio: Fs_timer >= Fs_modelo_máximo (EMG=2000) con margen 2×
    signalTimer = timerBegin(0, 80, true);  // Timer 0, prescaler 80 (1 µs)
    timerAttachInterrupt(signalTimer, &timerISR, true);
    timerAlarmWrite(signalTimer, 250, true);  // 250 µs = 4 kHz
    timerAlarmEnable(signalTimer);
}
```

**Características del ISR:**
- Atributo `IRAM_ATTR`: código en RAM para ejecución rápida
- Tiempo de ejecución típico: < 5 µs
- Detección de buffer underruns para diagnóstico
- Sin operaciones bloqueantes ni asignación dinámica

### 4.3 Gestión del Buffer Circular

El buffer circular de 2048 muestras proporcionó aproximadamente 0.5 segundos de autonomía a 4 kHz, suficiente para absorber variaciones temporales de la tarea de generación:

**Cálculo de autonomía del buffer:**
$$T_{buffer} = \frac{N_{muestras}}{f_s} = \frac{2048}{4000\,Hz} = 0.512\,s$$

**Nivel de llenado óptimo:**
- Mínimo: 25% (512 muestras) para evitar underruns
- Máximo: 75% (1536 muestras) para permitir escritura
- Target: 50% (1024 muestras) en operación normal

---

## 5. Subsistema de Visualización Nextion

### 5.1 Comunicación Serial con Nextion

La pantalla Nextion NX8048T070 se comunicó con el ESP32 mediante UART2:

| Parámetro | Valor |
|-----------|-------|
| Baudios | 115200 |
| Bits de datos | 8 |
| Paridad | Ninguna |
| Bits de parada | 1 |
| TX (ESP32) | GPIO17 |
| RX (ESP32) | GPIO16 |
| Terminador comando | 0xFF 0xFF 0xFF |

### 5.2 Tasas de Actualización

Se definieron dos tasas de actualización independientes para optimizar el ancho de banda serial:

**Tabla 5.1: Tasas de actualización Nextion**

| Componente | Tasa | Período | Justificación |
|------------|------|---------|---------------|
| Waveform | 100 Hz | 10 ms | Fluidez visual, límite percepción |
| Métricas | 4 Hz | 250 ms | Legibilidad de números |
| Escalas | 1 Hz | 1000 ms | Cambios infrecuentes |

### 5.3 Downsampling para Waveform

El downsampling se calcula **respecto a Fs_timer (4 kHz)**, no respecto al modelo:

```
┌─────────────────────────────────────────────────────────────────┐
│                    DOWNSAMPLING PARA NEXTION                     │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Timer @ 4 kHz ────┬──► Decimación 20:1 ──► Nextion ECG 200 Hz   │
│                    │                                            │
│                    ├──► Decimación 40:1 ──► Nextion EMG 100 Hz   │
│                    │                                            │
│                    └──► Decimación 40:1 ──► Nextion PPG 100 Hz   │
│                                                                 │
│  Fórmula: Ratio = Fs_timer / Fds                                │
│                                                                 │
│  Implementación (config.h):                                     │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │  const uint16_t FS_TIMER_HZ = 4000;                      │    │
│  │  const uint16_t FDS_ECG = 200;                          │    │
│  │  const uint16_t FDS_EMG = 100;                          │    │
│  │  const uint16_t FDS_PPG = 100;                          │    │
│  │  const uint8_t NEXTION_DOWNSAMPLE_ECG = 4000/200 = 20;  │    │
│  │  const uint8_t NEXTION_DOWNSAMPLE_EMG = 4000/100 = 40;  │    │
│  │  const uint8_t NEXTION_DOWNSAMPLE_PPG = 4000/100 = 40;  │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

**Tabla 5.2: Parámetros de downsampling por señal**

| Señal | Fs_timer | Fds | Ratio = Fs_timer/Fds | Muestras/s enviadas |
|-------|----------|-----|----------------------|---------------------|
| ECG | 4000 Hz | 200 Hz | 20:1 | 200 |
| EMG | 4000 Hz | 100 Hz | 40:1 | 200 (2 canales) |
| PPG | 4000 Hz | 100 Hz | 40:1 | 100 |

### 5.4 Formato de Comandos Waveform

El componente waveform de Nextion acepta comandos `add` para agregar puntos:

```
Formato: add <id>,<canal>,<valor>\xFF\xFF\xFF

Ejemplos:
  add 1,0,128    // Waveform ID=1, canal 0, valor 128
  add 1,1,64     // Waveform ID=1, canal 1, valor 64 (envolvente EMG)

Código:
  void NextionDriver::addWaveformPoint(uint8_t id, uint8_t ch, uint8_t val) {
      char cmd[20];
      sprintf(cmd, "add %d,%d,%d", id, ch, val);
      sendCommand(cmd);
  }
```

### 5.5 Actualización de Métricas

Las métricas numéricas se actualizaron a 4 Hz (250 ms) para permitir legibilidad:

```
┌───────────────────────────────────────────────────────────────────┐
│                    MÉTRICAS POR TIPO DE SEÑAL                      │
├───────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ECG:                    EMG:                    PPG:             │
│  ┌─────────────────┐     ┌─────────────────┐     ┌─────────────┐  │
│  │ HR: 75 bpm      │     │ RMS: 0.45 mV    │     │ HR: 72 bpm  │  │
│  │ RR: 800 ms      │     │ MUs: 45         │     │ PI: 3.2%    │  │
│  │ QRS: 1.05 mV    │     │ Exc: 35%        │     │ Beats: 142  │  │
│  │ ST: +0.02 mV    │     │ FR: 22 Hz       │     │ RR: 833 ms  │  │
│  └─────────────────┘     └─────────────────┘     └─────────────┘  │
│                                                                   │
│  Actualización: 4 Hz (250 ms)                                     │
│  Comando: t_hr.txt="75"                                           │
│                                                                   │
└───────────────────────────────────────────────────────────────────┘
```

---

## 6. Subsistema de Comunicación WiFi

### 6.1 Configuración del Access Point

El ESP32 se configuró como Access Point WiFi para permitir conexiones directas sin infraestructura de red:

**Tabla 6.1: Configuración WiFi AP**

| Parámetro | Valor |
|-----------|-------|
| SSID | `BioSimulator_Pro` |
| Password | `biosignal123` |
| IP del AP | `192.168.4.1` |
| Canal | 1 |
| Máx. clientes | 4 |
| Modo | SoftAP |

### 6.2 Servidor WebSocket

Se implementó un servidor WebSocket en el puerto 81 para streaming bidireccional:

```
┌─────────────────────────────────────────────────────────────────┐
│                    ARQUITECTURA WiFi/WebSocket                   │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ESP32 (192.168.4.1)                                            │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │  WiFi AP (BioSimulator_Pro)                             │    │
│  │  ├── HTTP Server (puerto 80)                            │    │
│  │  │   └── Sirve index.html, app.js, styles.css           │    │
│  │  │                                                      │    │
│  │  └── WebSocket Server (puerto 81)                       │    │
│  │      ├── Streaming señal: 100 Hz (cada 10 ms)           │    │
│  │      ├── Streaming métricas: 4 Hz (cada 250 ms)         │    │
│  │      └── Recepción comandos: asíncrono                  │    │
│  └─────────────────────────────────────────────────────────┘    │
│                              │                                  │
│                              │ WiFi 802.11n                     │
│                              ▼                                  │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │  Cliente (Navegador Web)                                │    │
│  │  http://192.168.4.1                                     │    │
│  │                                                         │    │
│  │  ┌─────────────────────────────────────────────────┐    │    │
│  │  │  App Web (React/HTML5 Canvas)                   │    │    │
│  │  │  • Waveform en tiempo real                      │    │    │
│  │  │  • Panel de métricas                            │    │    │
│  │  │  • Controles de parámetros                      │    │    │
│  │  │  • Selector de condición                        │    │    │
│  │  └─────────────────────────────────────────────────┘    │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 6.3 Formato de Mensajes WebSocket

**Mensaje de señal (100 Hz):**
```json
{
  "type": "signal",
  "signal": "ECG",
  "value": 0.85,
  "dac": 172,
  "timestamp": 123456789
}
```

**Mensaje de métricas (4 Hz):**
```json
{
  "type": "metrics",
  "signal": "ECG",
  "data": {
    "hr": 75,
    "rr": 800,
    "qrsAmplitude": 1.05,
    "stDeviation": 0.02
  }
}
```

### 6.4 Tasas de Envío y Ancho de Banda

**Tabla 6.2: Análisis de ancho de banda WebSocket**

| Tipo mensaje | Tamaño | Frecuencia | Bytes/s | Bits/s |
|--------------|--------|------------|---------|--------|
| Señal | ~80 bytes | 100 Hz | 8000 | 64 kbps |
| Métricas | ~150 bytes | 4 Hz | 600 | 4.8 kbps |
| **Total** | - | - | **8600** | **~69 kbps** |

El ancho de banda requerido (~69 kbps) fue muy inferior al disponible en WiFi 802.11n (~50 Mbps), garantizando latencia mínima.

### 6.5 Control de Streaming

El streaming se habilitó/deshabilitó según el estado de simulación:

```cpp
// En handleUIEvent() - main.cpp
case UIEvent::BUTTON_START:
    stateMachine.processEvent(SystemEvent::START_SIMULATION);
    wifiServer.setStreamingEnabled(true);  // Habilitar streaming
    break;

case UIEvent::BUTTON_STOP:
    stateMachine.processEvent(SystemEvent::STOP);
    wifiServer.setStreamingEnabled(false);  // Deshabilitar streaming
    break;
```

---

## 7. Flujo de Datos Integrado

### 7.1 Diagrama de Flujo Completo

El siguiente diagrama muestra el flujo de datos desde la generación hasta las tres salidas del sistema:

├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────┐    │
│  │                  GENERACIÓN (Core 1, 4 kHz) + INTERPOLACIÓN              │
│  │                                                                     │
│  │   Condición ──► Modelo Activo ──► sample_mV ──► getDACValue()      │
│  │   (ECG/EMG/PPG)   (RK4/MU/Gauss)   (float)      (uint8_t 0-255)    │
│  │                                                                     │
│  └───────────────────────────────┬─────────────────────────────────────┘    │
│                                  │                                          │
│                                  ▼                                          │
│  ┌─────────────────────────────────────────────────────────────────┐    │
│  │                    BUFFER CIRCULAR (2048 muestras)                  │
│  │                    Tiempo de buffer: ~0.5 segundos                   │
│  └───────────────────────────────┬─────────────────────────────────────┘    │
│                                  │                                          │
│          ┌───────────────────────┼───────────────────────┐                  │
│          │                       │                       │                  │
│          ▼                       ▼                       ▼                  │
│  ┌───────────────┐      ┌───────────────┐      ┌───────────────┐           │
│  │  SALIDA DAC   │      │  NEXTION      │      │  WebSocket    │           │
│  │  (ISR 4 kHz)  │      │  (100-200 Hz) │      │  (100 Hz)     │           │
│  ├───────────────┤      ├───────────────┤      ├───────────────┤           │
│  │               │      │               │      │               │           │
│  │ timerISR():   │      │ updateDisplay │      │ sendSignal    │           │
│  │ dacWrite()    │      │ ()            │      │ Data()        │           │
│  │               │      │               │      │               │           │
│  │ Cada 0.25 ms  │      │ Cada 5-10 ms  │      │ Cada 10 ms    │           │
│  │ Sin filtro    │      │ Downsampled   │      │ JSON format   │           │
│  │               │      │               │      │               │           │
│  └───────┬───────┘      └───────┬───────┘      └───────┬───────┘           │
│          │                      │                      │                    │
│          ▼                      ▼                      ▼                    │
│  ┌───────────────┐      ┌───────────────┐      ┌───────────────┐           │
│  │  GPIO25       │      │  Waveform     │      │  Canvas       │           │
│  │  0-3.3V       │      │  700×380 px   │      │  HTML5        │           │
│  │  Analógico    │      │  + Métricas   │      │  + Métricas   │           │
│  └───────────────┘      └───────────────┘      └───────────────┘           │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 7.2 Latencias del Sistema

**Tabla 7.1: Análisis de latencias**

| Ruta | Latencia típica | Latencia máxima |
|------|-----------------|-----------------|
| Modelo → Buffer | < 1 ms | 2 ms |
| Buffer → DAC | 0.25 ms (fijo) | 0.25 ms + jitter (±50 µs) |
| Buffer → Nextion | 10-20 ms | 30 ms |
| Buffer → WebSocket | 10-50 ms | 100 ms (red) |

**Latencia total modelo-a-DAC:** 2-3 ms (determinista)
**Latencia total modelo-a-display:** 20-50 ms (aceptable para visualización)

---

## 8. Cálculos Computacionales Representativos

### 8.1 Memoria RAM Utilizada

**Tabla 8.1: Desglose de uso de RAM**

| Componente | Tamaño | Cálculo |
|------------|--------|---------|
| Buffer señal | 2048 bytes | `SIGNAL_BUFFER_SIZE × sizeof(uint8_t)` |
| ECGModel | ~2 KB | Estado RK4, parámetros PQRST |
| EMGModel | ~12 KB | 100 MUs × 120 bytes + buffers RMS |
| PPGModel | ~1 KB | Estado de fase, parámetros gaussianos |
| FreeRTOS stacks | 8 KB | 2 tareas × 4096 bytes |
| WiFi buffer | ~4 KB | Buffers TX/RX WebSocket |
| **Total** | **~30 KB** | < 520 KB disponibles ✓ |

### 8.2 Carga de CPU

**Tabla 8.2: Estimación de carga por núcleo**

| Núcleo | Tarea | Tiempo/ciclo | Período | Carga |
|--------|-------|--------------|---------|-------|
| Core 1 | generationTask | ~0.2 ms | 0.25 ms | ~80% |
| Core 1 | Timer ISR | ~5 µs | 0.25 ms | ~2% |
| Core 0 | loop() + Nextion | ~2 ms | 10 ms | ~20% |
| Core 0 | WiFi | ~1 ms | 10 ms | ~10% |

### 8.3 Ancho de Banda Serial (Nextion)

**Cálculo de bytes/segundo para waveform:**

```
Comando típico: "add 1,0,128" + 0xFF×3 = 14 bytes
Frecuencia: 100 Hz
Canales EMG: 2

Bytes/s ECG = 14 × 100 = 1400 B/s
Bytes/s EMG = 14 × 100 × 2 = 2800 B/s
Bytes/s PPG = 14 × 100 = 1400 B/s

Capacidad @ 115200 baud ≈ 11520 B/s
Utilización máxima (EMG) = 2800/11520 = 24% ✓
```

### 8.4 Escalado DAC por Señal

El mapeo de valores en mV a valores DAC (0-255) se realizó con las siguientes fórmulas:

**Tabla 8.3: Fórmulas de escalado DAC**

| Señal | Rango entrada (mV) | Fórmula DAC |
|-------|-------------------|-------------|
| ECG | [-0.5, +1.5] | `DAC = (mV + 0.5) / 2.0 × 255` |
| EMG | [-5.0, +5.0] | `DAC = (mV + 5.0) / 10.0 × 255` |
| PPG | [800, 1200] | `DAC = (mV - 800) / 400 × 255` |

**Ejemplo cálculo ECG (pico R = +1.0 mV):**
$$DAC = \frac{(1.0 + 0.5)}{2.0} \times 255 = \frac{1.5}{2.0} \times 255 = 191$$
$$V_{out} = \frac{191}{255} \times 3.3V = 2.47V$$

### 8.5 Tiempo Visible en Waveform

**Cálculo del tiempo visible en pantalla:**

```
Ancho waveform: 700 px
Frecuencia display: 100 Hz (1 punto cada 10 ms)
Tiempo visible: T = 700 px × 10 ms/px = 7000 ms = 7 s

Latidos visibles @ 75 BPM:
Período RR = 60/75 = 0.8 s
Latidos = 7 s / 0.8 s = 8.75 ≈ 9 latidos
```

---

## 9. Metodología de Diseño de Frecuencias

Esta sección documenta la metodología sistemática utilizada para seleccionar las frecuencias de integración del modelo, downsampling y visualización.

### 9.1 Frecuencia Natural de la Señal (fmax)

**Tabla 9.1: Contenido espectral de las señales**

| Señal | fmax (Hz) | Justificación |
|-------|-----------|---------------|
| ECG | 50 | Componentes QRS hasta 50 Hz |
| EMG | 500 | sEMG frecuencias hasta 500 Hz |
| PPG | 10 | Señal cardiovascular lenta |

### 9.2 Frecuencia de Integración del Modelo (Fs)

**Criterio:** Fs ≥ 5 × fmax (Nyquist con margen)

**Tabla 9.2: Frecuencias de integración interna**

| Señal | fmax (Hz) | Fs mínima (5×fmax) | Fs elegido |
|-------|-----------|---------------------|------------|
| ECG | 50 | 250 Hz | 750 Hz |
| EMG | 500 | 2500 Hz | 2000 Hz |
| PPG | 10 | 50 Hz | 100 Hz |

### 9.3 Timer Maestro (FS_TIMER_HZ)

**Criterios de selección:**
1. Fs_timer ≥ 2 × fmax (Nyquist para reconstrucción)
2. Fs_timer ≥ Fs_modelo_máximo (para no perder muestras del modelo)
3. Fs_timer divisible por Fds (para ratios de downsampling enteros)
4. Margen de seguridad 2× sobre Fs_modelo_máximo

**Tabla 9.3: Configuración del timer maestro**

| Parámetro | Valor | Justificación |
|-----------|-------|--------------|
| FS_TIMER_HZ | 4000 Hz | 2× EMG (2000 Hz) con margen |
| Buffer circular | 2048 muestras | ~0.5 s de autonomía |
| Jitter esperado | < 0.25 ms | 1 tick de timer |

### 9.4 Frecuencias de Salida a Display (Fds)

**Criterio:** Fds basado en ventana mínima visible y capacidad de pantalla (≤250 Hz)

**Tabla 9.4: Ventana visible y frecuencias de display**

| Señal | Ventana mín (ms) | Fds calculado | Fds propuesto | Ciclos visibles |
|-------|------------------|---------------|---------------|-----------------|
| ECG | 2250 | 311 Hz | 200 Hz | 4.67 |
| EMG | 800 | 87.5 Hz | 100 Hz | 1.75 |
| PPG | 2400 | 292 Hz | 100 Hz | 8.75 |

### 9.5 Ratios de Downsampling

**IMPORTANTE:** Los ratios se calculan respecto a **Fs_timer**, NO respecto al modelo.

**Tabla 9.5: Ratios de downsampling**

| Señal | Fs_timer | Fds | Ratio = Fs_timer/Fds |
|-------|----------|-----|----------------------|
| ECG | 4000 Hz | 200 Hz | **20:1** |
| EMG | 4000 Hz | 100 Hz | **40:1** |
| PPG | 4000 Hz | 100 Hz | **40:1** |

### 9.6 Escalado y Visualización

**Tabla 9.6: Parámetros de visualización**

| Señal | Rango (mV) | Div. vert. | mV/div | Fds (Hz) | T_pantalla (ms) | ms/div |
|-------|------------|------------|--------|----------|-----------------|--------|
| ECG | −0.5 a 1.5 | 10 | 0.2 | 200 | 3500 | 350 |
| EMG | ±5 | 10 | 1.0 | 100 | 7000 | 700 |
| PPG | 0–150 | 10 | 15 | 100 | 7000 | 700 |

### 9.7 Resumen Metodológico

```
┌─────────────────────────────────────────────────────────────────┐
│           METODOLOGÍA DE FRECUENCIAS - RESUMEN                   │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  1. Determinar fmax de cada señal (Tabla 9.1)                  │
│  2. Seleccionar Fs del modelo ≥ 5× fmax (Tabla 9.2)            │
│  3. Definir Fs_timer ≥ Fs_modelo_máximo (Tabla 9.3)            │
│  4. Definir ventana mínima visible y calcular Fds (Tabla 9.4)  │
│  5. Calcular ratios = Fs_timer / Fds (Tabla 9.5)               │
│  6. Calcular T_pantalla, ms/div y mV/div (Tabla 9.6)           │
│                                                                 │
│  Implementación en código (config.h):                          │
│  • FS_TIMER_HZ = 4000                                          │
│  • NEXTION_DOWNSAMPLE_ECG = 4000 / 200 = 20                    │
│  • NEXTION_DOWNSAMPLE_EMG = 4000 / 100 = 40                    │
│  • NEXTION_DOWNSAMPLE_PPG = 4000 / 100 = 40                    │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 10. Referencias

[1] Espressif Systems, "ESP32 Technical Reference Manual," Version 4.5, 2022.

[2] FreeRTOS, "FreeRTOS Reference Manual," Version 10.4.3, 2021.

[3] ITEAD Studio, "Nextion Instruction Set," Version 1.0, 2023.

[4] WebSocket Protocol, RFC 6455, IETF, 2011.

---

*Documento de arquitectura computacional para BioSimulator Pro v1.0.0*  
*18 de Diciembre de 2025*
