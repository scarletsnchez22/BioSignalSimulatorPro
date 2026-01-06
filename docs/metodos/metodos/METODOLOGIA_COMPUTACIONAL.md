# Metodología de Arquitectura Computacional del Sistema

**BioSignalSimulator Pro**  
**Revisado:** 06.01.2026  
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
9. [Metodología de Diseño de Frecuencias](#9-metodología-de-diseño-de-frecuencias)
10. [Salida Analógica y Acondicionamiento de Señal](#10-salida-analógica-y-acondicionamiento-de-señal)
11. [Referencias](#11-referencias)

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
│                           BioSignalSimulator Pro - ARQUITECTURA COMPLETA                    │
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
| Core 1 | Timer ISR | Salida DAC | 0.25 ms (4 kHz) | Máxima | ISR |

**Tabla 3.2: Constantes de configuración (config.h)**

| Constante | Valor | Descripción |
|-----------|-------|-------------|
| `CORE_SIGNAL_GENERATION` | 1 | Núcleo para generación |
| `CORE_UI_COMMUNICATION` | 0 | Núcleo para UI/WiFi |
| `TASK_PRIORITY_SIGNAL` | 5 | Prioridad máxima de tarea |
| `STACK_SIZE_SIGNAL` | 4096 | Stack de tarea de señal |
| `FS_TIMER_HZ` | 4000 | Timer maestro DAC (4 kHz) |
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
| Waveform ECG | 200 Hz | 5 ms | Mayor resolución temporal QRS |
| Waveform EMG/PPG | 100 Hz | 10 ms | Fluidez visual, límite percepción |
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

**Cálculo del tiempo visible en pantalla (depende de Fds por señal):**

```
Ancho waveform: 700 px

ECG @ 200 Hz: T = 700 / 200 = 3.5 s (~4 latidos @ 75 BPM)
EMG @ 100 Hz: T = 700 / 100 = 7.0 s
PPG @ 100 Hz: T = 700 / 100 = 7.0 s (~9 latidos @ 75 BPM)
```

---

## 9. Metodología de Diseño de Frecuencias

Esta sección documenta la metodología sistemática utilizada para seleccionar las frecuencias de integración del modelo, oversampling, y decimación para visualización en Nextion. El DAC escribe a 4 kHz sin decimación para preservar el espectro completo.

### 9.1 Contenido Espectral de Señales Biomédicas (Referencias Clínicas)

**Tabla 9.1: Análisis espectral basado en literatura médica**

| Señal | Rango Total | Energía Dominante | Fuente |
|-------|-------------|-------------------|--------|
| **ECG** | 0.5 - 150 Hz | 4 - 40 Hz | PubMed Central, GE HealthCare, AHA |
| **EMG** | 10 - 500 Hz | 50 - 150 Hz | Delsys, MDPI, SENIAM |
| **PPG** | 0.5 - 15 Hz | 0.5 - 5 Hz | PubMed Central, Frontiers |

**Detalle por señal:**

**ECG (Electrocardiograma):**
- Onda P: 5-30 Hz
- Complejo QRS: 4-30 Hz (potencia máxima 4-12 Hz)
- Onda T: 0-10 Hz
- Rango clínico estándar: 0.5 - 150 Hz
- ECG pediátrico: hasta 250 Hz (AHA Journals)

**EMG (Electromiograma):**
- sEMG superficie: 10-500 Hz
- Energía dominante: 50-150 Hz
- EMG invasivo: hasta 1000 Hz

**PPG (Fotopletismografía):**
- Frecuencia cardíaca: 1-2 Hz (60-120 BPM)
- Componentes armónicos: hasta 5 Hz
- Muesca dicrótica: hasta 15 Hz
- Respiración (HRV): 0.15-0.4 Hz

### 9.2 Frecuencia Interna del Modelo (Fs_modelo)

**NOTA IMPORTANTE:** El criterio de Nyquist aplica a **digitalización de señales analógicas existentes** (ADC), NO a síntesis de señales. Para generación de señales con modelos matemáticos, los criterios son:

1. **Estabilidad numérica del integrador** (RK4): dt ≤ τ_min / 5
2. **Resolución de detalles de forma de onda**: ≥5 muestras en el detalle más fino
3. **Costo computacional**: capacidad del microcontrolador

**Tabla 9.2: Frecuencias internas de los modelos**

| Señal | fmax clínico | Criterio Nyquist | Fs_modelo elegido | Justificación |
|-------|--------------|------------------|-------------------|---------------|
| **ECG** | 150 Hz | 2× fmax | **300 Hz** | Nyquist clínico (IEC 60601-2-51) |
| **EMG** | 500 Hz | 2× fmax | **1000 Hz** | Nyquist sEMG estándar |
| **PPG** | 10 Hz | 2× fmax | **20 Hz** | Forma de onda lenta, Nyquist clínico |

**Modelos matemáticos implementados:**
- **ECG:** McSharry ECGSYN (EDOs con integración RK4)
- **EMG:** Fuglevand (100 unidades motoras, proceso estocástico)
- **PPG:** Allen (modelo gaussiano de pulso arterial)

### 9.3 Timer Maestro y Técnica de Oversampling (FS_TIMER_HZ)

**Técnica:** Oversampling seguido de decimación para evitar aliasing en salidas.

**Criterios de selección del timer:**
1. **Fs_timer > Fs_modelo_máximo** (para no perder información del modelo más rápido)
2. **Factor de seguridad 4×** sobre el modelo más exigente
3. **Divisibilidad** por frecuencias de salida (100 Hz, 200 Hz) para decimación entera

**Cálculo:**
```
Fs_modelo_máximo = EMG @ 1000 Hz
Factor de seguridad = 4×
Fs_timer = 1000 Hz × 4 = 4000 Hz
```

**Tabla 9.3: Configuración del timer maestro**

| Parámetro | Valor | Justificación |
|-----------|-------|---------------|
| FS_TIMER_HZ | 4000 Hz | 4× EMG (1000 Hz), factor seguridad |
| Buffer circular | 2048 muestras | ~0.5 s de autonomía |
| Jitter esperado | < 0.25 ms | 1 tick de timer |

### 9.4 Interpolación Lineal (Upsampling de Modelo a Timer)

**Objetivo:** Rellenar huecos entre muestras del modelo para alimentar el buffer del timer a 4000 Hz.

**Fórmula de interpolación lineal:**
```
sample(t) = prev + (curr - prev) × (t / ratio)

Donde:
- prev = muestra anterior del modelo
- curr = muestra actual del modelo
- t = contador de interpolación (0 a ratio-1)
- ratio = Fs_timer / Fs_modelo
```

**Tabla 9.4: Ratios de upsampling (interpolación)**

| Señal | Fs_modelo | Fs_timer | Ratio upsampling |
|-------|-----------|----------|------------------|
| **ECG** | 300 Hz | 4000 Hz | **13:1** (~13 muestras interpoladas) |
| **EMG** | 1000 Hz | 4000 Hz | **4:1** (4 muestras interpoladas) |
| **PPG** | 20 Hz | 4000 Hz | **200:1** (~200 muestras interpoladas) |

### 9.5 Decimación para Visualización (Solo Nextion)

**Técnica:** Oversampling + Decimación para evitar aliasing en las salidas.

**Criterio de frecuencia de salida (Fds):** Basado en ventana mínima visible y capacidad del receptor.

**Fórmula de ventana visible:**
```
Fds = (Ancho_px × Ciclos_deseados) / (Ancho_px / Fds_max)

Simplificado: Fds = Ancho_waveform / T_ventana_deseada
```

**Tabla 9.5a: Cálculo de frecuencias de salida**

| Señal | Ventana mín (ms) | Ciclos visibles requeridos (prom)| Fds calculado | Fds propuesto | Ciclos visibles finales (prom)
|-------|------------------|-----------------|---------------|---------------|
| **ECG** | 2250 | ~3 latidos | 311 Hz | **200 Hz** | 4.67
| **EMG** | 800 | ~2 contracciones | 87.5 Hz | **100 Hz** | 1.75
| **PPG** | 2400 | ~3 pulsos | 292 Hz | **100 Hz** | 8.75

**Tabla 9.5b: Ratios de decimación (downsampling)**

| Señal | Fs_timer | Fds_Nextion | Ratio decimación | Aplica a |
|-------|----------|-------------|------------------|----------|
| **ECG** | 4000 Hz | 200 Hz | **20:1** | Solo Nextion (visualización) |
| **EMG** | 4000 Hz | 100 Hz | **40:1** | Solo Nextion (visualización) |
| **PPG** | 4000 Hz | 100 Hz | **40:1** | Solo Nextion (visualización) |

**IMPORTANTE:** 
- El **DAC escribe a 4000 Hz sin decimación** para preservar el espectro frecuencial completo de las señales (especialmente EMG con contenido hasta 500 Hz).
- La **decimación solo se aplica a Nextion** (visualización) debido a limitaciones de ancho de banda UART.
- El **filtro RC analógico** (fc = 1.59 kHz) actúa como filtro de reconstrucción, suavizando los escalones del DAC.

### 9.6 Escalado y Visualización en Nextion

El waveform de Nextion tiene dimensiones de **700 × 380 píxeles**. A continuación se detallan los parámetros de visualización para cada señal.

#### 9.6.1 Grid Funcional - Escala Horizontal (Tiempo)

**Cálculo del tiempo por píxel (Δt/px):**
```
Δt_por_pixel = 1 / Fds_Nextion

ECG:  Δt = 1/200 Hz = 5 ms/px
EMG:  Δt = 1/100 Hz = 10 ms/px
PPG:  Δt = 1/100 Hz = 10 ms/px
```

**Tiempo total visible en pantalla:**
```
T_total = Ancho_px × Δt_por_pixel

ECG:  T = 700 px × 5 ms  = 3500 ms = 3.5 s
EMG:  T = 700 px × 10 ms = 7000 ms = 7.0 s
PPG:  T = 700 px × 10 ms = 7000 ms = 7.0 s
```

**Tabla 9.6a: Parámetros de escala horizontal**

| Señal | Fds (Hz) | Δt/px (ms) | Ancho (px) | T_total (s) | Divisiones | ms/div |
|-------|----------|------------|------------|-------------|------------|--------|
| **ECG** | 200 | 5 | 700 | 3.5 | 10 | **350** |
| **EMG** | 100 | 10 | 700 | 7.0 | 10 | **700** |
| **PPG** | 100 | 10 | 700 | 7.0 | 10 | **700** |

#### 9.6.2 Grid Funcional - Escala Vertical (Amplitud)

**Tabla 9.6b: Parámetros de escala vertical**

| Señal | Rango (mV) | Vpp (mV) | Altura (px) | Divisiones | mV/div |
|-------|------------|----------|-------------|------------|--------|
| **ECG** | −0.5 a +1.5 | 2.0 | 380 | 10 | **0.2** |
| **EMG Raw** | −5.0 a +5.0 | 10.0 | 380 | 10 | **1.0** |
| **EMG Envelope** | 0 a +2.0 | 2.0 | *escalado en Raw* | — | — |
| **PPG AC** | 0 a 150 | 150 | 380 | 10 | **15** |

**Nota sobre EMG Envelope:** La envolvente RMS tiene rango 0–2 mV pero se grafica **superpuesta en la escala del Raw** (±5 mV) para visualización conjunta. Por tanto, el envelope ocupa aproximadamente el 20% superior del área del waveform cuando está en MVC (contracción máxima).

#### 9.6.3 Resumen del Grid Funcional

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                     GRID FUNCIONAL NEXTION (700×380 px)                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ECG (Electrocardiograma)                                                   │
│  ────────────────────────                                                   │
│  Horizontal: 350 ms/div  │  10 divisiones  │  Total: 3.5 s                 │
│  Vertical:   0.2 mV/div  │  10 divisiones  │  Rango: −0.5 a +1.5 mV        │
│  Δt/punto:   5 ms        │  Fds: 200 Hz                                    │
│                                                                             │
│  EMG (Electromiografía)                                                     │
│  ──────────────────────                                                     │
│  Horizontal: 700 ms/div  │  10 divisiones  │  Total: 7.0 s                 │
│  Vertical:   1.0 mV/div  │  10 divisiones  │  Rango: −5 a +5 mV (Raw)      │
│  Envelope:   0–2 mV superpuesto en escala Raw (20% del rango)              │
│  Δt/punto:   10 ms       │  Fds: 100 Hz                                    │
│                                                                             │
│  PPG (Fotopletismografía)                                                   │
│  ────────────────────────                                                   │
│  Horizontal: 700 ms/div  │  10 divisiones  │  Total: 7.0 s                 │
│  Vertical:   15 mV/div   │  10 divisiones  │  Rango: 0 a 150 mV (AC)       │
│  Δt/punto:   10 ms       │  Fds: 100 Hz                                    │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 9.7 Resumen Metodológico

```
┌─────────────────────────────────────────────────────────────────────────────┐
│              METODOLOGÍA DE FRECUENCIAS - RESUMEN                            │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  FLUJO COMPLETO:                                                            │
│                                                                             │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐                   │
│  │   MODELO     │───►│INTERPOLACIÓN │───►│     DAC      │                   │
│  │  (Fs_modelo) │    │  (4000 Hz)   │    │  (4000 Hz)   │                   │
│  └──────────────┘    └──────────────┘    └──────┬───────┘                   │
│   750/2000/100 Hz       Upsampling          Sin decimación                  │
│                                                  │                          │
│                              ┌───────────────────┴───────────────────┐      │
│                              │                                       │      │
│                              ▼                                       ▼      │
│                     ┌──────────────┐                        ┌────────────┐  │
│                     │  FILTRO RC   │                        │  NEXTION   │  │
│                     │  fc=1.59 kHz │                        │ (decimado) │  │
│                     └──────────────┘                        └────────────┘  │
│                       Reconstrucción                        200/100 Hz      │
│                                                                             │
│  SALIDAS:                                                                   │
│  • DAC/BNC: 4000 Hz SIN decimación → espectro completo para osciloscopio   │
│  • Nextion: Decimado 20:1/40:1 → visualización (limitación UART)           │
│                                                                             │
│  PASOS DE DISEÑO:                                                           │
│  1. Analizar contenido espectral de señales (Tabla 9.1)                    │
│  2. Elegir Fs_modelo según criterios de síntesis (Tabla 9.2)               │
│  3. Definir Fs_timer = 2× Fs_modelo_máximo (Tabla 9.3)                     │
│  4. Calcular ratios de interpolación (Tabla 9.4)                           │
│  5. Definir Fds_Nextion y ratios de decimación (Tabla 9.5)                 │
│  6. Calcular parámetros de visualización (Tabla 9.6)                       │
│                                                                             │
│  IMPLEMENTACIÓN EN config.h:                                                │
│  • FS_TIMER_HZ = 4000 (también Fs_DAC)                                     │
│  • MODEL_SAMPLE_RATE_ECG = 750                                              │
│  • MODEL_SAMPLE_RATE_EMG = 2000                                             │
│  • MODEL_SAMPLE_RATE_PPG = 100                                              │
│  • NEXTION_DOWNSAMPLE_ECG = 4000 / 200 = 20                                │
│  • NEXTION_DOWNSAMPLE_EMG = 4000 / 100 = 40                                │
│  • NEXTION_DOWNSAMPLE_PPG = 4000 / 100 = 40                                │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 10. Salida Analógica: Arquitectura DAC y BNC

Esta sección documenta la arquitectura de la **salida analógica funcional** del sistema (DAC → BNC), que es el elemento principal del dispositivo. El ADC loopback y Serial Plotter son únicamente herramientas de validación durante desarrollo.

### 10.1 Arquitectura DAC y Espectro Frecuencial

**Objetivo:** Preservar el contenido espectral completo de las señales biomédicas en la salida analógica.

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    ARQUITECTURA DE SALIDA ANALÓGICA                          │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ¿Por qué DAC a 4 kHz SIN decimación?                                       │
│  ────────────────────────────────────                                       │
│  • EMG tiene contenido espectral hasta 500 Hz (Nyquist mínimo = 1 kHz)     │
│  • Si decimamos a 100 Hz, Nyquist = 50 Hz → perdemos 90% del espectro      │
│  • Solución: DAC escribe TODAS las muestras interpoladas a 4 kHz           │
│                                                                             │
│  FLUJO DE SEÑAL:                                                            │
│  ───────────────                                                            │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐    ┌──────────┐  │
│  │   MODELO     │───►│INTERPOLACIÓN │───►│    DAC       │───►│ FILTRO   │  │
│  │ (Fs_modelo)  │    │  (4000 Hz)   │    │  (4000 Hz)   │    │ RC 1.6kHz│  │
│  └──────────────┘    └──────────────┘    └──────────────┘    └──────────┘  │
│   750/2000/100 Hz      Upsampling          Sin decimación     Reconstrucción│
│                                                                             │
│  DECIMACIÓN SELECTIVA (solo visualización):                                 │
│  ──────────────────────────────────────────                                 │
│  • Nextion: Decimado a 200/100 Hz (limitación UART ~11.5 KB/s)             │
│  • Serial Plotter: Decimado a 200/100 Hz (debug, no afecta salida)         │
│  • DAC: SIN decimación → espectro completo para osciloscopio               │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

**Tabla 10.1: Frecuencias del sistema**

| Señal | Fs_modelo | Fs_DAC | Fs_Nextion | Nyquist DAC | Contenido útil |
|-------|-----------|--------|------------|-------------|----------------|
| **ECG** | 300 Hz | **4000 Hz** | 200 Hz | 2000 Hz | 0.5–150 Hz ✓ |
| **EMG** | 1000 Hz | **4000 Hz** | 100 Hz | 2000 Hz | 20–500 Hz ✓ |
| **PPG** | 20 Hz | **4000 Hz** | 100 Hz | 2000 Hz | 0.5–10 Hz ✓ |

### 10.2 Cadena de Procesamiento Digital (Antes del DAC)

La señal pasa por tres etapas de procesamiento digital antes de llegar al DAC:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    PROCESAMIENTO DIGITAL PARA DAC                            │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ETAPA 1: MODELO MATEMÁTICO                                                 │
│  ┌─────────────────────────────────────────────────────────────────┐        │
│  │  ECG: McSharry RK4 @ 300 Hz (genera muestras discretas)         │        │
│  │  EMG: Fuglevand MU @ 1000 Hz (100 unidades motoras)             │        │
│  │  PPG: Allen Gaussiano @ 20 Hz (pulso cardiovascular)            │        │
│  └─────────────────────────────────────────────────────────────────┘        │
│           │                                                                 │
│           │ Muestras a frecuencia del modelo (variable)                     │
│           ▼                                                                 │
│  ETAPA 2: INTERPOLACIÓN LINEAL (Upsampling a 4 kHz)                         │
│  ┌─────────────────────────────────────────────────────────────────┐        │
│  │  Ratio ECG: 4000/750 ≈ 5.3:1                                    │        │
│  │  Ratio EMG: 4000/2000 = 2:1                                     │        │
│  │  Ratio PPG: 4000/100 = 40:1                                     │        │
│  │                                                                  │        │
│  │  Fórmula: sample(t) = prev + (curr - prev) × t/ratio           │        │
│  │  Suaviza transiciones entre muestras del modelo                 │        │
│  └─────────────────────────────────────────────────────────────────┘        │
│           │                                                                 │
│           │ Buffer circular @ 4000 Hz (oversampled)                        │
│           ▼                                                                 │
│  ETAPA 3: SALIDA DAC (Sin decimación - espectro completo)                   │
│  ┌─────────────────────────────────────────────────────────────────┐        │
│  │  Timer ISR @ 4000 Hz escribe directamente al DAC:               │        │
│  │                                                                  │        │
│  │  void IRAM_ATTR timerISR() {                                    │        │
│  │      dacWrite(DAC_SIGNAL_PIN, lastDACValue);  // Cada tick      │        │
│  │  }                                                               │        │
│  │                                                                  │        │
│  │  JUSTIFICACIÓN (espectro frecuencial):                          │        │
│  │  • EMG tiene contenido hasta 500 Hz (Nyquist = 1 kHz mínimo)    │        │
│  │  • DAC @ 4 kHz → Nyquist = 2 kHz → preserva todo el espectro   │        │
│  │  • Si decimáramos a 100 Hz → Nyquist = 50 Hz → pérdida del 90% │        │
│  │  • El filtro RC (fc=1.59 kHz) suaviza escalones del DAC         │        │
│  └─────────────────────────────────────────────────────────────────┘        │
│           │                                                                 │
│           ▼                                                                 │
│  [Timer ISR → DAC @ 4000 Hz (SIN decimación)]                               │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 10.3 Etapa de Acondicionamiento Analógico (Después del DAC)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    ACONDICIONAMIENTO ANALÓGICO                               │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ESP32 DAC (GPIO25)                                                         │
│  ┌─────────────────┐                                                        │
│  │  8-bit DAC      │  ← Señal interpolada a 4 kHz                          │
│  │  0-3.3V         │                                                        │
│  │  @ 4000 Hz      │  ← SIN decimación (espectro completo)                 │
│  └────────┬────────┘                                                        │
│           │                                                                 │
│           ▼                                                                 │
│  ┌─────────────────────────────────────────────────────────────────┐        │
│  │                    FILTRO RC PASIVO                              │        │
│  │                                                                  │        │
│  │    DAC ───[R=100Ω]───┬─── Vout                                  │        │
│  │                      │                                           │        │
│  │                   [C=1µF]                                        │        │
│  │                      │                                           │        │
│  │                     GND                                          │        │
│  │                                                                  │        │
│  │    fc = 1/(2πRC) ≈ 1.59 kHz                                     │        │
│  │                                                                  │        │
│  │    FUNCIÓN (filtro de reconstrucción):                          │        │
│  │    • Criterio: fmax_señal < fc < Fs_DAC/2                       │        │
│  │    •           500 Hz < 1590 Hz < 2000 Hz  ✓                    │        │
│  │    • Preserva banda útil (0-500 Hz para EMG)                    │        │
│  │    • Atenúa stepping del DAC (~8 dB @ 4 kHz)                    │        │
│  │    • Convierte señal escalonada en curva continua               │        │
│  └─────────────────────────────────────────────────────────────────┘        │
│           │                                                                 │
│           ▼                                                                 │
│  ┌─────────────────────────────────────────────────────────────────┐        │
│  │                    BUFFER SEGUIDOR (MCP6002)                     │        │
│  │                                                                  │        │
│  │    FUNCIONES:                                                    │        │
│  │    1. Aislamiento de impedancia (no carga al filtro RC)         │        │
│  │    2. Baja impedancia de salida (puede alimentar cargas)        │        │
│  │    3. Protección del DAC contra cortocircuitos                  │        │
│  └─────────────────────────────────────────────────────────────────┘        │
│           │                                                                 │
│           ▼                                                                 │
│  ┌─────────────────┐                                                        │
│  │  CONECTOR BNC   │  ← SALIDA FUNCIONAL DEL DISPOSITIVO                   │
│  │  0-3.3V         │                                                        │
│  │  Señal limpia   │  → Osciloscopio, monitor de paciente, ADC externo     │
│  └─────────────────┘                                                        │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 10.4 Función del Filtro RC: Reconstrucción Analógica

**¿Por qué necesitamos el filtro RC?**

El DAC de 8 bits del ESP32 genera una señal **escalonada** (staircase), no continua. El filtro RC convierte estos escalones en una señal analógica suave mediante un proceso llamado **reconstrucción de señal**.

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    FUNCIÓN DEL FILTRO RC                                     │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  SEÑAL DEL DAC (antes del filtro):                                          │
│  ┌────┐     ┌────┐     ┌────┐                                              │
│  │    │     │    │     │    │    ← Escalones discretos                     │
│  │    └─────┘    └─────┘    └────                                           │
│                                                                             │
│  SEÑAL POST-FILTRO RC (después):                                            │
│      ╱╲      ╱╲      ╱╲                                                     │
│     ╱  ╲    ╱  ╲    ╱  ╲        ← Curva suave continua                     │
│    ╱    ╲  ╱    ╲  ╱    ╲                                                   │
│                                                                             │
│  NOTA IMPORTANTE:                                                           │
│  • El filtro RC NO elimina offset del DAC                                   │
│  • El filtro RC NO corrige errores de voltaje                              │
│  • El filtro RC SOLO suaviza transiciones (reconstrucción)                 │
│                                                                             │
│  EVIDENCIA EXPERIMENTAL:                                                    │
│  Señal teórica: 105 mV → 2.31 V                                            │
│  DAC directo:   105 mV → 2.57 V  (error +0.26V por cuantización 8-bit)    │
│  ADC post-RC:   105 mV → 2.32 V  (cercano a teórico, filtro funciona)     │
│                                                                             │
│  El ADC lee ~2.32V (no 2.57V) porque el filtro RC PROMEDIA los escalones   │
│  del DAC, acercándose al valor teórico continuo.                           │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

**Tabla 10.2: Diseño del filtro RC**

| Parámetro | Valor | Justificación |
|-----------|-------|---------------|
| R | 10 kΩ | Compromiso entre atenuación y carga al DAC |
| C | 100 nF | Frecuencia de corte adecuada para señales biomédicas |
| fc | 159 Hz | Atenúa armónicos de muestreo (4 kHz) sin afectar señal |
| Atenuación @ 4 kHz | -28 dB | Escalones del DAC reducidos a <5% de amplitud |

**Cálculo de atenuación:**
$$H(f) = \frac{1}{\sqrt{1 + (f/f_c)^2}}$$
$$H(4000\,Hz) = \frac{1}{\sqrt{1 + (4000/159)^2}} = 0.04 = -28\,dB$$

**Explicación de la discrepancia DAC vs ADC:**

Cuando el DAC escribe un valor discreto (ej: 199 para 2.57V), genera un **escalón**. El filtro RC actúa como un **promediador analógico**, suavizando ese escalón. El ADC, al leer después del filtro, ve el **valor promediado** (~2.32V), que es más cercano al valor teórico continuo (2.31V).

**Fórmula de conversión DAC:**
```
Valor teórico: 105 mV en rango [0, 150 mV]
Escalado a 3.3V: (105 / 150) × 3.3V = 2.31V
Cuantización 8-bit: round(2.31 / 3.3 × 255) = 179 → 2.32V (ideal)

Pero el DAC del ESP32 tiene offset interno → puede dar 199 → 2.57V
El filtro RC suaviza → ADC lee ~2.32V (correcto)
```

### 10.5 Cadena Completa de Señal (DAC → BNC)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│              CADENA FUNCIONAL: MODELO → DAC → BNC                            │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  [Modelo Matemático]                                                        │
│        │ ECG: McSharry RK4 @ 300 Hz                                         │
│        │ EMG: Fuglevand MU @ 1000 Hz                                        │
│        │ PPG: Allen Gaussiano @ 20 Hz                                       │
│        ▼                                                                    │
│  [Interpolación Lineal] ──► Upsampling a 4000 Hz (OVERSAMPLING)             │
│        ▼                                                                    │
│  [Buffer Circular DRAM_ATTR] ──► 2048 muestras @ 4 kHz                      │
│        ▼                                                                    │
│  [Decimación] ──► ECG: 20:1 (200 Hz), EMG/PPG: 40:1 (100 Hz)               │
│        ▼                                                                    │
│  [Timer ISR → DAC] ──► dacWrite(GPIO25, valor) @ Fds                        │
│        ▼                                                                    │
│  [Filtro RC fc=159 Hz] ──► Reconstrucción analógica                         │
│        ▼                                                                    │
│  [Buffer Seguidor Op-Amp] ──► Aislamiento de impedancia                     │
│        ▼                                                                    │
│  [CONECTOR BNC] ──► SALIDA FUNCIONAL 0-3.3V                                 │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 10.6 Validación: ADC Loopback y Serial Plotter

**NOTA:** El ADC loopback y Serial Plotter son **herramientas de validación durante desarrollo**, no elementos funcionales del dispositivo final.

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    SISTEMA DE VALIDACIÓN (SOLO DEBUG)                        │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  [Conector BNC] ──► Cable loopback ──► [ADC GPIO34]                         │
│                                              │                              │
│                                              ▼                              │
│                                    [Lectura ADC @ 4000 Hz]                  │
│                                              │                              │
│                                              ▼                              │
│                                    [Acumulación + Promediado]               │
│                                    ECG: 200 Hz (5 ms)                       │
│                                    EMG/PPG: 100 Hz (10 ms)                  │
│                                              │                              │
│                                              ▼                              │
│                                    [Serial Plotter VS Code]                 │
│                                    Formato: >dac:X,adc:Y                    │
│                                                                             │
│  PROPÓSITO:                                                                 │
│  • Verificar que la señal en BNC es correcta                               │
│  • Comparar DAC teórico vs ADC real (post-filtro RC)                       │
│  • Detectar problemas de offset, atenuación o distorsión                   │
│  • NO afecta la señal funcional del dispositivo                            │
│                                                                             │
│  NOTA SOBRE DECIMACIÓN:                                                     │
│  • DAC escribe a 4000 Hz SIN decimación (espectro completo)                │
│  • Serial Plotter y Nextion usan decimación (visualización)                │
│  • ECG: 200 Hz, EMG/PPG: 100 Hz (limitación UART)                          │
│  • El osciloscopio ve la señal REAL a 4 kHz                                │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 10.7 Serial Plotter vs Osciloscopio: Diferencias Clave

**Pregunta:** ¿Lo que veo en Serial Plotter es igual a lo que vería en un osciloscopio?

**Respuesta:** **Casi igual, pero con diferencias importantes.**

```
┌─────────────────────────────────────────────────────────────────────────────┐
│              COMPARACIÓN: SERIAL PLOTTER vs OSCILOSCOPIO                     │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  SERIAL PLOTTER (main.cpp líneas 1200-1248):                                │
│  ┌──────────────────────────────────────────────────────────────┐           │
│  │  1. ADC lee GPIO34 @ 4000 Hz (cada 250 µs)                   │           │
│  │  2. Acumula muestras en dacAccum y adcAccum                  │           │
│  │  3. Cada 5ms (ECG) o 10ms (EMG/PPG):                         │           │
│  │     - Calcula promedio: dacAvg = dacAccum / sampleCount      │           │
│  │     - Envía a Serial: >dac:X,adc:Y                           │           │
│  │  4. Muestra: SEÑAL PROMEDIADA (más suave)                    │           │
│  └──────────────────────────────────────────────────────────────┘           │
│                                                                             │
│  OSCILOSCOPIO (equipo real):                                                │
│  ┌──────────────────────────────────────────────────────────────┐           │
│  │  1. Lee señal analógica del BNC directamente                 │           │
│  │  2. Muestreo típico: 1-10 MSa/s (1000× más rápido)           │           │
│  │  3. Sin promediado (modo normal)                             │           │
│  │  4. Muestra: SEÑAL INSTANTÁNEA (puede ver ruido)             │           │
│  └──────────────────────────────────────────────────────────────┘           │
│                                                                             │
│  DIFERENCIAS:                                                               │
│  ┌──────────────────────┬────────────────────┬──────────────────┐           │
│  │ Característica       │ Serial Plotter     │ Osciloscopio     │           │
│  ├──────────────────────┼────────────────────┼──────────────────┤           │
│  │ Frecuencia muestreo  │ 4000 Hz → 200/100  │ 1-10 MHz         │           │
│  │ Promediado           │ SÍ (20-40 muestras)│ NO (configurable)│           │
│  │ Ruido visible        │ Reducido           │ Completo         │           │
│  │ Forma de onda        │ Suavizada          │ Más detallada    │           │
│  │ Precisión voltaje    │ ±13 mV (12-bit ADC)│ ±1-10 mV típico  │           │
│  └──────────────────────┴────────────────────┴──────────────────┘           │
│                                                                             │
│  CONCLUSIÓN:                                                                │
│  • Morfología: IGUAL (PQRST, MUAPs, pulsos se ven correctos)               │
│  • Amplitud: SIMILAR (diferencia <5% típicamente)                          │
│  • Ruido: Serial Plotter muestra MENOS ruido (por promediado)              │
│  • Detalles: Osciloscopio muestra MÁS detalles de alta frecuencia          │
│                                                                             │
│  AMBOS VEN LA SEÑAL POST-FILTRO RC (suavizada, no escalonada)              │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

**Implementación en código (main.cpp):**

```cpp
// Acumular muestras a 4000 Hz
if (micros() - lastSample_us >= 250) {  // 250 µs = 4000 Hz
    uint16_t adcRaw = analogRead(ADC_LOOPBACK_PIN);
    float adcVoltage = (adcRaw / 4095.0f) * 3.3f;
    dacAccum += dacVoltage;
    adcAccum += adcVoltage;
    sampleCount++;
}

// Enviar promedio cada 5ms (ECG) o 10ms (EMG/PPG)
if (millis() - lastADCRead_ms >= downsampleInterval_ms) {
    float dacAvg = dacAccum / sampleCount;  // ← PROMEDIADO
    float adcAvg = adcAccum / sampleCount;
    Serial.print(">dac:");
    Serial.print(dacAvg, 3);
    Serial.print(",adc:");
    Serial.println(adcAvg, 3);
}
```

**Ventajas del promediado:**
- Reduce ruido del ADC (±13 mV → ±5 mV típico)
- Señal más estable y fácil de interpretar
- Coherente con Nextion (misma frecuencia de visualización)

**Desventajas del promediado:**
- Pierde detalles de muy alta frecuencia
- No muestra transitorios rápidos (<5 ms)
- Puede ocultar glitches o spikes

**Tabla 10.3: Comparación elementos funcionales vs validación**

| Elemento | Tipo | Frecuencia | Propósito |
|----------|------|------------|-----------|
| DAC GPIO25 | **Funcional** | **4000 Hz** (sin decimación) | Salida analógica principal |
| Filtro RC | **Funcional** | Analógico (fc=1.59 kHz) | Reconstrucción de señal |
| Buffer Op-Amp | **Funcional** | Analógico | Aislamiento de impedancia |
| Conector BNC | **Funcional** | Analógico | Conexión a equipos externos |
| ADC GPIO34 | Validación | 200/100 Hz (muestreo) | Lectura de señal para debug |
| Serial Plotter | Validación | 200/100 Hz (decimado) | Visualización durante desarrollo |
| Nextion Waveform | Interfaz | 200/100 Hz (decimado) | Visualización para usuario |

**Nota importante sobre frecuencias:**
- **DAC:** Escribe a **4000 Hz SIN decimación** para preservar espectro completo (EMG hasta 500 Hz)
- **Nextion/Serial Plotter:** Decimados a 200/100 Hz por limitación de ancho de banda UART
- El osciloscopio conectado al BNC ve la señal completa a 4 kHz

---

## 11. Referencias

[1] Espressif Systems, "ESP32 Technical Reference Manual," Version 4.5, 2022.

[2] FreeRTOS, "FreeRTOS Reference Manual," Version 10.4.3, 2021.

[3] ITEAD Studio, "Nextion Instruction Set," Version 1.0, 2023.

[4] WebSocket Protocol, RFC 6455, IETF, 2011.

[5] McSharry PE, Clifford GD, Tarassenko L, Smith LA. "A dynamical model for generating synthetic electrocardiogram signals." IEEE Trans Biomed Eng. 2003;50(3):289-294.

[6] Fuglevand AJ, Winter DA, Patla AE. "Models of recruitment and rate coding organization in motor-unit pools." J Neurophysiol. 1993;70(6):2470-2488.

[7] Allen J, Murray A. "Age-related changes in peripheral pulse timing characteristics at the ears, fingers and toes." J Hum Hypertens. 2002;16(10):711-717.

---

*Documento de arquitectura computacional para BioSignalSimulator Pro*  
*Revisado: 06.01.2026*
