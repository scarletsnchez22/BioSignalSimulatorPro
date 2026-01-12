# Metodología Computacional del Sistema BioSignalSimulator Pro

**Grupo #22:** Scarlet Sánchez y Rafael Mata  
**Institución:** Escuela Superior Politécnica del Litoral (ESPOL)  
**Documento de Sustentación Técnica para Trabajo de Titulación**

---

## Índice

1. [Introducción](#1-introducción)
2. [Arquitectura de Hardware](#2-arquitectura-de-hardware)
3. [Arquitectura de Software Embebido](#3-arquitectura-de-software-embebido)
4. [Motor de Generación de Señales](#4-motor-de-generación-de-señales)
5. [Análisis Espectral y Justificación de Frecuencias](#5-análisis-espectral-y-justificación-de-frecuencias)
6. [Estrategia de Muestreo: Upsampling y Downsampling](#6-estrategia-de-muestreo-upsampling-y-downsampling)
7. [Filtrado Digital de Señales](#7-filtrado-digital-de-señales)
8. [Control del Multiplexor CD4051](#8-control-del-multiplexor-cd4051)
9. [Subsistema de Visualización Nextion](#9-subsistema-de-visualización-nextion)
10. [Subsistema de Comunicación WiFi](#10-subsistema-de-comunicación-wifi)
11. [Estructura del Programa](#11-estructura-del-programa)
12. [Consideraciones de Implementación](#12-consideraciones-de-implementación)
13. [Referencias](#13-referencias)

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

Se seleccionó el **ESP32-WROOM-32** como unidad de procesamiento central mediante un análisis comparativo de plataformas microcontroladoras.

#### 2.1.1 Matriz de Decisión: Selección de Microcontrolador

**Tabla 2.1.1a: Criterios de evaluación y ponderación**

| Criterio | Peso (%) | Justificación |
|----------|----------|---------------|
| Capacidad de Procesamiento Dual-Core | 25% | Crítico: separación de dominio tiempo real (señales) e interfaz (UI/WiFi) |
| DAC Integrado para Salida Analógica | 20% | Muy importante: evitar circuitos externos DAC (MCP4725), reduce latencia |
| Conectividad WiFi Integrada | 20% | Importante: streaming a aplicación web sin módulos externos (ESP8266) |
| Memoria RAM Suficiente | 15% | Importante: buffers circulares (6 KB) + estados modelos (20 KB) |
| Ecosistema y Soporte | 10% | Moderado: disponibilidad de bibliotecas, documentación, comunidad |
| Costo y Disponibilidad | 10% | Moderado: presupuesto limitado, disponibilidad local en Ecuador |

**Tabla 2.1.1b: Evaluación de alternativas (escala 1-5)**

| Plataforma | Dual-Core | DAC Int. | WiFi | RAM | Ecosistema | Costo | Total Ponderado |
|------------|-----------|----------|------|-----|------------|-------|-----------------|
| **ESP32-WROOM-32** | 5 | 5 | 5 | 5 | 5 | 5 | **5.00** ✓ |
| STM32F407VG | 1 | 5 | 1 | 4 | 4 | 3 | **2.85** |
| Arduino Mega 2560 | 1 | 1 | 1 | 1 | 5 | 5 | **2.05** |
| Raspberry Pi Pico | 4 | 1 | 1 | 3 | 4 | 5 | **2.75** |

**Puntuación detallada:**

1. **ESP32-WROOM-32** - Puntuación: 5.00
   - **Dual-Core (5):** Xtensa LX6 dual-core @ 240 MHz, separación física Core 0 (UI) y Core 1 (señales)
   - **DAC (5):** 2 canales DAC de 8-bit integrados (GPIO25/GPIO26), salida 0-3.3V sin circuitos externos
   - **WiFi (5):** WiFi 802.11 b/g/n integrado, modo Access Point para servidor WebSocket sin módulos adicionales
   - **RAM (5):** 520 KB SRAM (suficiente para buffer 2048×3=6KB + modelos ~20KB + FreeRTOS ~30KB)
   - **Ecosistema (5):** Arduino IDE, ESP-IDF, PlatformIO, librerías WiFi/WebSocket maduras
   - **Costo (5):** $6-8 USD en Ecuador, amplia disponibilidad local (Microjeros, HeTPro)

2. **STM32F407VG Discovery** - Puntuación: 2.85
   - **Dual-Core (1):** Single-core ARM Cortex-M4 @ 168 MHz (sin separación nativa de tareas tiempo real vs UI)
   - **DAC (5):** 2 canales DAC de 12-bit (superior a ESP32), salida 0-3.3V
   - **WiFi (1):** Requiere módulo WiFi externo (ESP8266, CC3000), comunicación SPI/UART adicional
   - **RAM (4):** 192 KB SRAM (suficiente pero más ajustado que ESP32)
   - **Ecosistema (4):** STM32CubeIDE, HAL libraries, comunidad activa pero curva de aprendizaje mayor
   - **Costo (3):** $20-25 USD (board Discovery), disponibilidad limitada en Ecuador

3. **Arduino Mega 2560** - Puntuación: 2.05
   - **Dual-Core (1):** Single-core ATmega2560 @ 16 MHz (frecuencia insuficiente para generación 1kHz + WiFi concurrente)
   - **DAC (1):** Sin DAC integrado, requiere DAC externo (MCP4725 I2C), aumenta latencia ~1ms
   - **WiFi (1):** Requiere shield WiFi (ESP8266, Wiznet W5500), consume pines y memoria
   - **RAM (1):** 8 KB SRAM (insuficiente: buffer 6KB + modelos 20KB + FreeRTOS imposible)
   - **Ecosistema (5):** Arduino IDE, comunidad masiva, librerías abundantes
   - **Costo (5):** $15-20 USD, alta disponibilidad

4. **Raspberry Pi Pico (RP2040)** - Puntuación: 2.75
   - **Dual-Core (4):** ARM Cortex-M0+ dual-core @ 133 MHz (dos cores pero frecuencia menor)
   - **DAC (1):** Sin DAC integrado, requiere DAC externo (MCP4725) o PWM+filtro RC
   - **WiFi (1):** Sin WiFi integrado (Pico W tiene WiFi pero salió posterior al diseño)
   - **RAM (3):** 264 KB SRAM (suficiente)
   - **Ecosistema (4):** MicroPython, C SDK, documentación buena pero comunidad más pequeña
   - **Costo (5):** $4-6 USD, disponibilidad creciente

**Conclusión:**

El **ESP32-WROOM-32** obtuvo puntuación perfecta (5.00/5.00) al ser la única plataforma que cumple simultáneamente con procesamiento dual-core, DAC integrado, WiFi nativo, RAM suficiente y excelente relación costo-beneficio. La arquitectura dual-core permitió asignar Core 1 (APP CPU) exclusivamente a generación de señales en tiempo real con prioridad máxima (tarea FreeRTOS), mientras Core 0 (PRO CPU) manejó la interfaz Nextion UART2 y el servidor WebSocket WiFi sin interferir en el determinismo temporal del DAC.

El **STM32F407** fue descartado por carecer de WiFi integrado (requeriría módulo ESP8266 adicional, aumentando complejidad) y single-core (imposibilita separación física de dominios temporales). El **Arduino Mega** fue descartado por RAM insuficiente (8KB vs 520KB requeridos) y ausencia total de DAC/WiFi. El **Raspberry Pi Pico** fue descartado por carecer de DAC y WiFi integrados, aunque su arquitectura dual-core era atractiva.

**Tabla 2.1.1c: Especificaciones finales ESP32-WROOM-32**

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
│  │  LM358 Buffer   │     │  Waveform       │     │   App Web           │   │
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

## 5. Análisis Espectral y Justificación de Frecuencias

El diseño de las frecuencias de muestreo del sistema se fundamentó en un análisis espectral riguroso mediante Transformada Rápida de Fourier (FFT) aplicado a cada modelo matemático. Este análisis permitió determinar el contenido frecuencial real de las señales generadas y optimizar los parámetros del sistema.

### 5.1 Resultados del Análisis FFT

Se ejecutó un análisis espectral de 7 segundos de duración sobre cada modelo, aplicando ventana de Hanning para reducir la fuga espectral. Los resultados se resumen en la siguiente tabla:

**Tabla 5.1: Contenido espectral por señal biomédica**

| Señal | Frecuencia Dominante | BW -3dB | BW -20dB | F 99% Energía | Energía en Banda Clínica |
|-------|---------------------|---------|----------|---------------|--------------------------|
| **ECG** | 1.14 Hz (≈68 BPM) | 12.0 Hz | 24.0 Hz | **21.6 Hz** | 100% (0.05-150 Hz) |
| **EMG** | 55.4 Hz | 96.6 Hz | 158.0 Hz | **146.3 Hz** | 99.7% (20-500 Hz) |
| **PPG** | 1.14 Hz (≈68 BPM) | 1.29 Hz | 4.86 Hz | **4.9 Hz** | 99.9% (0.5-10 Hz) |

Los valores de F 99% Energía (21.6 Hz para ECG, 146.3 Hz para EMG, 4.9 Hz para PPG) representan las frecuencias donde se concentra el 99% de la energía espectral de cada señal, constituyendo el límite práctico del contenido útil.

### 5.2 Justificación de Fs_timer = 4 kHz

La frecuencia del timer maestro del DAC (Fs_timer = 4000 Hz) se seleccionó mediante el siguiente análisis:

**Criterio de Nyquist aplicado:**

El teorema de Nyquist-Shannon establece que la frecuencia de muestreo debe ser al menos el doble de la frecuencia máxima de la señal:

$$F_s \geq 2 \times F_{max}$$

Para el sistema BioSignalSimulator Pro, la señal con mayor contenido frecuencial es el EMG, cuyo ancho de banda clínico se extiende hasta 500 Hz. Aplicando el criterio de Nyquist con margen de seguridad:

$$F_{s,timer} = 2 \times F_{max,EMG} \times Factor_{seguridad} = 2 \times 500 \times 4 = 4000 \, Hz$$

**Verificación de cumplimiento de Nyquist:**

| Señal | Fmax Real (99% energía) | Fs Timer | Relación Fs/Fmax | Cumplimiento |
|-------|-------------------------|----------|------------------|--------------|
| ECG | 21.6 Hz | 4000 Hz | 185× | ✓ Holgado |
| EMG | 146.3 Hz | 4000 Hz | 27× | ✓ Holgado |
| PPG | 4.9 Hz | 4000 Hz | 816× | ✓ Holgado |

El factor de 4× sobre Nyquist se justificó por:
1. **Margen para filtro anti-aliasing pasivo:** Los filtros RC de primer orden requieren separación entre Fc y Fs/2.
2. **Suavizado de escalones del DAC:** Mayor sobremuestreo produce transiciones más suaves.
3. **Uniformidad del timer:** Un único Fs_timer simplifica la arquitectura de interrupciones.

### 5.3 Frecuencias de Muestreo de los Modelos

Cada modelo matemático opera a su propia frecuencia de muestreo interna (Fs_modelo), optimizada para su complejidad computacional:

**Tabla 5.2: Frecuencias de muestreo por modelo**

| Modelo | Fs Modelo | Fmax Clínica | Nyquist Mínimo | Margen Real |
|--------|-----------|--------------|----------------|-------------|
| ECG (McSharry) | 300 Hz | 150 Hz | 300 Hz | 100% |
| EMG (Fuglevand) | 1000 Hz | 500 Hz | 1000 Hz | 100% |
| PPG (Allen) | 20 Hz | 10 Hz | 20 Hz | 100% |

---

## 6. Estrategia de Muestreo: Upsampling y Downsampling

El sistema implementó una estrategia de muestreo en dos etapas para conciliar las diferentes frecuencias de los modelos con los requisitos de salida DAC y visualización.

### 6.1 Upsampling: Modelo → Timer DAC

Los modelos generan muestras a frecuencias nativas (Fs_modelo) que posteriormente se interpolan para alimentar el timer del DAC a 4 kHz:

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         UPSAMPLING (Interpolación)                       │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  PPG:   20 Hz  ──[×200]──► 4000 Hz   (interpolación lineal)            │
│  ECG:  300 Hz  ──[×13.3]─► 4000 Hz   (interpolación lineal)            │
│  EMG: 1000 Hz  ──[×4]────► 4000 Hz   (interpolación lineal)            │
│                                                                         │
│  Fórmula: Ratio_up = Fs_timer / Fs_modelo                               │
│                                                                         │
│  Implementación:                                                        │
│  ┌───────────────────────────────────────────────────────────────────┐  │
│  │  // Interpolación lineal entre muestras del modelo                │  │
│  │  float t_modelo = sampleCount / Fs_timer;                         │  │
│  │  int idx = (int)(t_modelo * Fs_modelo);                           │  │
│  │  float frac = (t_modelo * Fs_modelo) - idx;                       │  │
│  │  sample = modelo[idx] * (1-frac) + modelo[idx+1] * frac;          │  │
│  └───────────────────────────────────────────────────────────────────┘  │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

**Caso crítico: PPG (20 Hz → 4000 Hz)**

El modelo PPG opera a solo 20 Hz debido a su bajo contenido frecuencial (F99% = 4.9 Hz). La interpolación 200:1 fue validada espectralmente, confirmando que no introduce artefactos significativos dado que el contenido original está muy por debajo del límite de Nyquist.

### 6.2 Downsampling: Timer DAC → Visualización

Las interfaces de visualización (Nextion y WebSocket) no requieren la tasa completa de 4 kHz. Se aplicó decimación selectiva:

```
┌─────────────────────────────────────────────────────────────────────────┐
│                    DOWNSAMPLING (Decimación para Display)                │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  Timer @ 4 kHz ────┬──► Decimación 20:1 ──► Nextion ECG 200 Hz          │
│                    │                                                    │
│                    ├──► Decimación 40:1 ──► Nextion EMG 100 Hz          │
│                    │                                                    │
│                    ├──► Decimación 40:1 ──► Nextion PPG 100 Hz          │
│                    │                                                    │
│                    └──► Decimación 40:1 ──► WebSocket 100 Hz            │
│                                                                         │
│  Fórmula: Ratio_down = Fs_timer / Fs_display                            │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

**Tabla 6.1: Parámetros de downsampling**

| Destino | Señal | Fs_timer | Fs_display | Ratio | Justificación |
|---------|-------|----------|------------|-------|---------------|
| Nextion | ECG | 4000 Hz | 200 Hz | 20:1 | Resolución temporal QRS (5 ms) |
| Nextion | EMG | 4000 Hz | 100 Hz | 40:1 | Fluidez visual suficiente |
| Nextion | PPG | 4000 Hz | 100 Hz | 40:1 | F99%=4.9 Hz, 100 Hz es holgado |
| WebSocket | Todas | 4000 Hz | 100 Hz | 40:1 | Ancho de banda WiFi limitado |

El downsampling se implementó mediante contador de muestras:

```cpp
// Enviar cada N muestras al display
if (sampleCount % DOWNSAMPLE_RATIO == 0) {
    nextion.addWaveformPoint(waveformId, channel, dacValue);
}
```

---

## 7. Filtrado Digital de Señales

### 7.1 Filtrado del Modelo EMG

El modelo EMG de Fuglevand genera señales de potenciales de acción de unidades motoras (MUAPs) que requieren procesamiento adicional para simular la adquisición electromiográfica superficial real.

**Cadena de filtrado implementada:**

```
┌─────────────────────────────────────────────────────────────────────────┐
│                    FILTRADO DIGITAL EMG                                  │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  MUAPs crudos ──► HPF 20Hz ──► LPF 450Hz ──► Rectificación ──► ENV 6Hz │
│   (Fuglevand)    (Butterworth) (Butterworth)  (|x|)         (LPF suave) │
│                                                                         │
│  Especificaciones:                                                      │
│  ┌───────────────────────────────────────────────────────────────────┐  │
│  │  HPF: Butterworth 2do orden, Fc = 20 Hz                           │  │
│  │       Elimina artefactos de movimiento y offset DC                │  │
│  │                                                                   │  │
│  │  LPF: Butterworth 2do orden, Fc = 450 Hz                          │  │
│  │       Limita ancho de banda según SENIAM (20-500 Hz)              │  │
│  │                                                                   │  │
│  │  Envolvente: LPF 1er orden, Fc = 6 Hz                             │  │
│  │       Extrae la envolvente de activación muscular                 │  │
│  └───────────────────────────────────────────────────────────────────┘  │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

**Justificación del rango 20-450 Hz:**

El estándar SENIAM (Surface ElectroMyoGraphy for Non-Invasive Assessment of Muscles) establece el rango de frecuencias clínicamente relevante para EMG superficial entre 20 Hz y 500 Hz. El límite inferior de 20 Hz elimina artefactos de movimiento y la componente DC, mientras que el límite superior de 450 Hz (ligeramente conservador) captura el contenido principal de los MUAPs sin incluir ruido de alta frecuencia.

### 7.2 Procesamiento de Envolvente

La envolvente del EMG se calcula mediante rectificación de onda completa seguida de filtrado paso-bajo:

$$ENV(t) = LPF_{6Hz}\left[ |EMG_{filtrado}(t)| \right]$$

El filtro de envolvente a 6 Hz produce una señal suave que representa el nivel de activación muscular, útil para visualización y métricas RMS.

---

## 8. Control del Multiplexor CD4051

Se implementó un sistema de selección de filtros RC analógicos mediante el multiplexor CD4051, permitiendo optimizar la frecuencia de corte del filtro de reconstrucción según el tipo de señal activa.

### 8.1 Arquitectura del Control

```
┌─────────────────────────────────────────────────────────────────────────┐
│                    CONTROL DEL MULTIPLEXOR CD4051                        │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  ESP32                           CD4051                    Filtro RC    │
│  ┌──────────┐                   ┌──────────┐              ┌──────────┐  │
│  │ GPIO25   │──► DAC ──► LM358 ─│► COM     │              │          │  │
│  │ (PWM/DAC)│                   │          │              │   ───┬── │  │
│  │          │                   │ CH0 ◄────│──[6.8kΩ]─────│►     │   │  │
│  │ GPIO32   │──────────────────►│ S0       │              │      C   │──► BNC
│  │ (Select) │                   │ CH1 ◄────│──[1.0kΩ]─────│►   1µF   │  │
│  │          │                   │          │              │      │   │  │
│  │ GPIO33   │──────────────────►│ S1       │              │   ───┴── │  │
│  │ (Select) │                   │ CH2 ◄────│──[33kΩ]──────│►    GND  │  │
│  └──────────┘                   └──────────┘              └──────────┘  │
│                                                                         │
│  Tabla de selección:                                                    │
│  ┌─────────┬─────┬─────┬────────────┬────────────────────────────────┐  │
│  │ Canal   │ S1  │ S0  │ Resistor   │ Fc (con C=1µF)                 │  │
│  ├─────────┼─────┼─────┼────────────┼────────────────────────────────┤  │
│  │ CH0     │  0  │  0  │ 6.8 kΩ     │ 23.4 Hz → ECG (F99%=21.6 Hz)   │  │
│  │ CH1     │  0  │  1  │ 1.0 kΩ     │ 159 Hz → EMG (F99%=146 Hz)     │  │
│  │ CH2     │  1  │  0  │ 33 kΩ      │ 4.82 Hz → PPG (F99%=4.9 Hz)    │  │
│  └─────────┴─────┴─────┴────────────┴────────────────────────────────┘  │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 8.2 Cálculo de Frecuencias de Corte

Las frecuencias de corte de los filtros RC se calcularon a partir del análisis FFT para cada señal:

**ECG (CH0):** Fc = 23.4 Hz

$$F_c = \frac{1}{2\pi R C} = \frac{1}{2\pi \times 6800 \times 10^{-6}} = 23.4 \, Hz$$

Esta Fc está ligeramente por encima de F99% = 21.6 Hz del ECG, preservando todo el contenido espectral útil mientras atenúa el stepping del DAC.

**PPG (CH2):** Fc = 4.82 Hz

$$F_c = \frac{1}{2\pi \times 33000 \times 10^{-6}} = 4.82 \, Hz$$

Fc coincide con F99% = 4.9 Hz del PPG, proporcionando máxima atenuación del ruido de muestreo.

**EMG (CH1):** Fc = 159 Hz

$$F_c = \frac{1}{2\pi \times 1000 \times 10^{-6}} = 159.15 \text{ Hz}$$

El EMG utiliza R=1kΩ para obtener Fc=159 Hz, ligeramente superior al F99%=146.3 Hz del análisis FFT. Este filtro es necesario para eliminar el ruido de alta frecuencia introducido por el multiplexor CD4051 cuando se operaba sin filtro.

### 8.3 Implementación del Driver

El driver del CD4051 se implementó como clase singleton con interfaz simple:

```cpp
// Selección automática según tipo de señal
void SignalEngine::setSignalType(SignalType type) {
    switch(type) {
        case SIGNAL_ECG:
            mux.selectChannel(MUX_CH_ECG);  // CH0: 6.8kΩ, Fc=23.4 Hz
            break;
        case SIGNAL_EMG:
            mux.selectChannel(MUX_CH_EMG);  // CH1: 1.0kΩ, Fc=159 Hz
            break;
        case SIGNAL_PPG:
            mux.selectChannel(MUX_CH_PPG);  // CH2: 33kΩ, Fc=4.8 Hz
            break;
    }
}
```

### 8.4 Consideraciones de Resistencia Ron

El CD4051 presenta una resistencia de encendido (Ron) típica de 80Ω a VDD=5V. Esta resistencia se suma a la resistencia del filtro RC, introduciendo un error sistemático:

**Análisis de error por Ron:**

| Canal | R nominal | Ron | R total | Fc nominal | Fc real | Error |
|-------|-----------|-----|---------|------------|---------|-------|
| CH0 | 6.8 kΩ | 80Ω | 6.88 kΩ | 23.4 Hz | 23.1 Hz | 1.2% |
| CH2 | 33 kΩ | 80Ω | 33.08 kΩ | 4.82 Hz | 4.81 Hz | 0.2% |

El error introducido por Ron es inferior al 1.2% en todos los casos, considerado despreciable para la aplicación educativa.

---

## 9. Subsistema de Visualización Nextion

### 9.1 Matriz de Decisión: Tecnología de Display

#### 9.1.1 Criterios de Evaluación y Ponderación

**Tabla 9.1.1a: Criterios para selección de display**

| Criterio | Peso (%) | Justificación |
|----------|----------|---------------|
| Procesador Gráfico Standalone | 30% | Crítico: liberar ESP32 de renderizado gráfico para generación tiempo real |
| Interfaz Táctil Integrada | 25% | Muy importante: control de parámetros sin hardware adicional (teclado/mouse) |
| Resolución y Tamaño | 20% | Importante: waveform legible (700×380 px área útil) |
| Facilidad de Desarrollo | 15% | Importante: tiempo limitado de desarrollo |
| Costo | 10% | Moderado: presupuesto ajustado (<$50) |

**Tabla 9.1.1b: Evaluación de alternativas (escala 1-5)**

| Display | Proc. Standalone | Táctil | Resolución | Desarrollo | Costo | Total Ponderado |
|---------|------------------|--------|------------|------------|-------|-----------------|
| **Nextion NX8048T070 7"** | 5 | 5 | 5 | 5 | 3 | **4.65** ✓ |
| TFT LCD 3.5" + Touch Shield | 1 | 4 | 2 | 3 | 5 | **2.50** |
| OLED 1.3" SSD1306 | 1 | 1 | 1 | 4 | 5 | **2.00** |
| LCD 16×2 + Botones | 1 | 1 | 1 | 5 | 5 | **2.10** |

**Puntuación detallada:**

1. **Nextion NX8048T070 (Display Inteligente 7")** - Puntuación: 4.65
   - **Procesador Standalone (5):** STM32F030 dedicado renderiza gráficos independientemente, ESP32 solo envía comandos ASCII vía UART (ej: `"add 1,0,128\xFF\xFF\xFF"` → añadir punto a waveform)
   - **Táctil (5):** Pantalla táctil capacitiva integrada, eventos táctiles enviados vía UART (`0x65` + ID componente)
   - **Resolución (5):** 800×480 px, área waveform 700×380 px, visualización clara de morfología PQRST/MUAP
   - **Desarrollo (5):** Editor Nextion GUI drag-and-drop, generación automática de código, comunicación UART simple
   - **Costo (3):** ~$35-40 USD, más costoso que alternativas pero justificado por features

2. **TFT LCD 3.5" + Touch Shield (ILI9341)** - Puntuación: 2.50
   - **Procesador Standalone (1):** Sin procesador gráfico, ESP32 debe renderizar cada píxel vía SPI (consume ~40% Core 0 @ 10 FPS)
   - **Táctil (4):** Shield táctil resistivo/capacitivo disponible
   - **Resolución (2):** 320×240 px, área limitada para waveform + métricas simultáneas
   - **Desarrollo (3):** Librerías Arduino (Adafruit_GFX, TFT_eSPI) maduras pero requiere manejo manual de buffers
   - **Costo (5):** $12-15 USD, económico

3. **OLED 1.3" SSD1306** - Puntuación: 2.00
   - **Procesador Standalone (1):** Controlador interno pero limitado a buffer monocromático 128×64 px
   - **Táctil (1):** Sin capacidad táctil, requeriría botones externos
   - **Resolución (1):** 128×64 px monocromático, insuficiente para waveform detallado
   - **Desarrollo (4):** Librería Adafruit_SSD1306 simple
   - **Costo (5):** $3-5 USD

4. **LCD 16×2 Caracteres + Botones** - Puntuación: 2.10
   - **Procesador Standalone (1):** Sin gráficos, solo texto alfanumérico
   - **Táctil (1):** Requiere 4 botones externos (arriba/abajo/izq/der)
   - **Resolución (1):** Sin waveform, solo texto
   - **Desarrollo (5):** Librería LiquidCrystal trivial
   - **Costo (5):** $2-3 USD

**Conclusión:**

El **Nextion NX8048T070** obtuvo la mayor puntuación (4.65/5.00) gracias a su procesador gráfico standalone que libera al ESP32 de tareas de renderizado. Esta característica fue **crítica** para el proyecto: mientras el Core 1 del ESP32 generaba muestras a 4 kHz (carga temporal ~80%), el Core 0 solo necesitaba enviar comandos UART simples cada 10 ms (`"add 1,0,128"`) en lugar de renderizar 700×380 píxeles por frame.

La interfaz de desarrollo Nextion Editor permitió diseñar la GUI completa (waveforms, medidores, botones) en ~8 horas vs. las estimadas 40+ horas con TFT LCD manual. La pantalla táctil capacitiva eliminó la necesidad de 10+ botones externos que hubieran requerido GPIO adicionales y debouncing por software.

El **TFT LCD 3.5"** fue descartado porque renderizar waveforms en tiempo real vía SPI consumiría ~40% del Core 0 del ESP32, interfiriendo con el servidor WiFi WebSocket. El **OLED** fue descartado por resolución insuficiente (128×64 px no permite visualizar morfología ECG). El **LCD 16×2** fue descartado por carecer de capacidad gráfica.

### 9.2 Comunicación Serial con Nextion

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

### 9.5 Sistema de Escalas del Waveform: mV/div y ms/div

El waveform de Nextion opera como un **rolling display** (desplazamiento continuo), donde cada comando `add` desplaza toda la forma de onda 1 píxel a la izquierda y dibuja el nuevo valor a la derecha. Este comportamiento está documentado en el [Nextion Instruction Set](https://nextion.tech/instruction-set/#s5).

#### 9.5.1 Cálculo de la Escala Temporal (ms/div)

A diferencia de un osciloscopio digital que captura en memoria y permite re-escalar, el waveform de Nextion tiene una **escala temporal fija** determinada por:

1. **Ancho del waveform en píxeles** (700 px según `config.h`)
2. **Frecuencia de envío de puntos** (Fs_display)
3. **Comportamiento de Nextion**: 1 comando `add` = 1 píxel de desplazamiento

**Fórmula de cálculo:**

$$T_{pixel} = \frac{1}{F_{display}} \quad ; \quad T_{total} = W_{px} \times T_{pixel} \quad ; \quad \frac{ms}{div} = \frac{T_{total}}{N_{div}}$$

Donde:
- $T_{pixel}$ = tiempo que representa cada píxel
- $F_{display}$ = frecuencia de envío (puntos/segundo)
- $W_{px}$ = ancho del waveform (700 píxeles)
- $N_{div}$ = número de divisiones horizontales (10)

**Tabla 9.5: Cálculo de escalas temporales**

| Señal | Fs_display | T/pixel | T_total (700px) | Divisiones | **ms/div** |
|-------|------------|---------|-----------------|------------|------------|
| ECG | 200 Hz | 5 ms | 3500 ms | 10 | **350 ms/div** |
| EMG | 100 Hz | 10 ms | 7000 ms | 10 | **700 ms/div** |
| PPG | 100 Hz | 10 ms | 7000 ms | 10 | **700 ms/div** |

**Ejemplo de cálculo para ECG:**
```
Fs_display = 200 Hz → T_pixel = 1/200 = 5 ms/píxel
T_total = 700 px × 5 ms/px = 3500 ms = 3.5 segundos visibles
ms/div = 3500 ms / 10 divisiones = 350 ms/div
```

#### 9.5.2 Cálculo de la Escala de Amplitud (mV/div)

La escala vertical depende del rango de voltaje del modelo y la altura del waveform:

**Tabla 9.6: Escalas de amplitud por señal**

| Señal | Rango modelo | Altura útil | Divisiones | **mV/div** |
|-------|--------------|-------------|------------|------------|
| ECG | -0.5 a +1.5 mV (2.0 mV) | 380 px | 10 | **0.2 mV/div** |
| EMG RAW | -5.0 a +5.0 mV (10 mV) | 380 px | 10 | **1.0 mV/div** |
| EMG ENV | 0 a +2.0 mV | 380 px | 10 | **0.2 mV/div** |
| PPG (AC) | -100 a +100 mV (200 mV) | 380 px | 10 | **20 mV/div** |

**Nota sobre PPG:** La salida DAC y el waveform Nextion muestran únicamente la componente AC de la señal PPG. El componente DC (~1000 mV) se omite porque no aporta información clínica; la utilidad diagnóstica reside en la componente pulsátil.

#### 9.5.3 Diferencia con Osciloscopio Digital

```
┌─────────────────────────────────────────────────────────────────────────────┐
│        COMPARACIÓN: OSCILOSCOPIO DIGITAL vs NEXTION WAVEFORM                 │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  OSCILOSCOPIO DIGITAL:                   NEXTION WAVEFORM:                  │
│  ┌───────────────────────────┐           ┌───────────────────────────┐      │
│  │ 1. ADC muestrea a Fs fija │           │ 1. ESP32 envía puntos     │      │
│  │ 2. Almacena en memoria    │           │    a Fs_display fija      │      │
│  │ 3. Usuario ajusta ms/div  │           │ 2. NO hay memoria         │      │
│  │ 4. Software re-escala     │           │ 3. ms/div es FIJO         │      │
│  │ 5. Muestra ventana        │           │ 4. Rolling display        │      │
│  └───────────────────────────┘           └───────────────────────────┘      │
│                                                                             │
│  Característica         Osciloscopio      Nextion Waveform                  │
│  ─────────────────────────────────────────────────────────────────────────  │
│  Escala temporal        Ajustable         Fija (350/700 ms/div)             │
│  Escala amplitud        Ajustable         Fija (vía ganancia software)      │
│  Memoria                Sí (captura)      No (streaming continuo)           │
│  Zoom temporal          Post-captura      No disponible                     │
│  Zoom amplitud          Hardware          Software (ganancia)               │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

El usuario puede ajustar la **ganancia** (zoom de amplitud) desde los sliders de Nextion, pero la escala temporal permanece fija porque está determinada por la frecuencia de envío de puntos.

### 9.6 Actualización de Métricas

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

### 9.7 Validación de Visualización Nextion mediante Serial Plotter

Para garantizar que la visualización en la pantalla Nextion del dispositivo sea correcta, se implementó un sistema de validación cruzada utilizando el firmware `main_debug.cpp` y el Serial Plotter de VS Code. Este método permite verificar la forma de onda digital sin necesidad del hardware completo.

#### 9.7.1 Arquitectura de Equivalencia Nextion ↔ Serial Plotter

```
┌──────────────────────────────────────────────────────────────────────────────┐
│               VALIDACIÓN: EQUIVALENCIA NEXTION ↔ SERIAL PLOTTER              │
├──────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  DISPOSITIVO FÍSICO                    ENTORNO DE DESARROLLO                 │
│  ┌────────────────────┐                ┌────────────────────┐                │
│  │  Pantalla Nextion  │                │  Serial Plotter    │                │
│  │  NX8048T070 7"     │                │  VS Code           │                │
│  │  ┌──────────────┐  │     MISMO      │  ┌──────────────┐  │                │
│  │  │ Waveform     │  │  ═══════════   │  │ Gráfico RT   │  │                │
│  │  │ 700×380 px   │  │    MAPEO       │  │ Tiempo real  │  │                │
│  │  │ 0-255 (8-bit)│  │  ═══════════   │  │ 0-255 (8-bit)│  │                │
│  │  └──────────────┘  │                │  └──────────────┘  │                │
│  │  ECG @ 200 Hz      │                │  ECG @ 200 Hz      │                │
│  │  EMG @ 100 Hz      │                │  EMG @ 100 Hz      │                │
│  │  PPG @ 100 Hz      │                │  PPG @ 100 Hz      │                │
│  └────────────────────┘                └────────────────────┘                │
│                                                                              │
│  PRINCIPIO DE VALIDACIÓN:                                                    │
│  Si el Serial Plotter muestra la señal correcta con morfología, amplitud    │
│  y frecuencia esperadas, entonces la pantalla Nextion mostrará              │
│  EXACTAMENTE la misma visualización cuando el dispositivo esté operativo.    │
│                                                                              │
└──────────────────────────────────────────────────────────────────────────────┘
```

#### 9.7.2 Mapeo Idéntico de Valores

El firmware `main_debug.cpp` replica **exactamente** las fórmulas de mapeo utilizadas en `main.cpp` para enviar puntos al waveform de Nextion:

**Tabla 9.7.2: Fórmulas de mapeo Nextion (implementadas en main_debug.cpp)**

| Señal | Fórmula de Normalización | Mapeo a Nextion | Rango Resultante |
|-------|--------------------------|-----------------|------------------|
| **ECG** | `normalized = (mV + 0.5) / 2.0` | `waveValue = 20 + (normalized × 215)` | 20 – 235 |
| **EMG RAW** | `ch0_value` (0-380) | `map(ch0, 0, 380, 20, 235)` | 20 – 235 |
| **EMG ENV** | `ch1_value` (0-380) | `map(ch1, 0, 380, 20, 235)` | 20 – 235 |
| **PPG** | `normalized = acValue_mV / 150.0` | `waveValue = 20 + (normalized × 215)` | 20 – 235 |

**Código de mapeo ECG (main_debug.cpp líneas 355-362):**
```cpp
// === MAPEO IDÉNTICO A main.cpp línea 967-970 ===
float mV_value = ecgModel.getCurrentValueMV();

// Normalización: (mV + 0.5) / 2.0 para rango -0.5 a +1.5 mV
float normalized = (mV_value + 0.5f) / 2.0f;
if (normalized < 0.0f) normalized = 0.0f;
if (normalized > 1.0f) normalized = 1.0f;

// Mapeo a rango Nextion: 20 + (normalized × 215)
int waveValue = (int)(20 + (normalized * 215));
```

#### 9.7.3 Frecuencias de Muestreo Idénticas

El Serial Plotter utiliza **exactamente los mismos ratios de downsampling** que Nextion:

| Señal | Fs_timer | Ratio Downsampling | Frecuencia Envío | Definición en config.h |
|-------|----------|-------------------|------------------|------------------------|
| ECG | 4000 Hz | 20:1 | **200 Hz** | `NEXTION_DOWNSAMPLE_ECG` |
| EMG | 4000 Hz | 40:1 | **100 Hz** | `NEXTION_DOWNSAMPLE_EMG` |
| PPG | 4000 Hz | 40:1 | **100 Hz** | `NEXTION_DOWNSAMPLE_PPG` |

Esto garantiza que el número de puntos por segundo y la resolución temporal sean idénticos entre el Serial Plotter y Nextion.

#### 9.7.4 Métricas Idénticas

El firmware `main_debug.cpp` reporta las **mismas métricas** que se muestran en la pantalla Nextion:

**ECG:** HR, RR, PR, QRS, QTc, amplitudes P/Q/R/S/T, desviación ST  
**EMG:** RMS, MUs activas, frecuencia de disparo, %MVC, MDF, MFL (fatiga)  
**PPG:** HR, RR, PI%, tiempos de sístole/diástole, amplitud AC

#### 9.7.5 Uso para Presentación de Resultados

Las gráficas del Serial Plotter de VS Code pueden utilizarse directamente para documentación y presentación de resultados del proyecto, ya que:

1. **Mayor calidad visual:** El Serial Plotter genera gráficos vectoriales de alta resolución comparado con fotografías de la pantalla Nextion tomadas con celular
2. **Captura sin artefactos:** No hay reflejos de pantalla, moiré, o distorsión por ángulo de cámara
3. **Equivalencia garantizada:** Los valores graficados son matemáticamente idénticos a los mostrados en Nextion
4. **Registro de métricas:** El output serial incluye todas las métricas calculadas con marca de tiempo

**Procedimiento de validación:**
1. Compilar y cargar `main_debug.cpp` con la condición deseada (`AUTO_SIGNAL_TYPE`, `AUTO_XXX_CONDITION`)
2. Abrir Serial Plotter de VS Code (Ctrl+Shift+P → "Serial Plotter")
3. Verificar que la forma de onda y métricas correspondan a los rangos clínicos esperados
4. Capturar screenshot del Serial Plotter para documentación
5. Confirmar que el dispositivo físico (cuando disponible) muestra la misma visualización

**Ejemplo de configuración para validar ECG Normal:**
```cpp
#define AUTO_SIGNAL_TYPE        0   // ECG
#define AUTO_ECG_CONDITION      0   // NORMAL
```

**Ejemplo de output esperado:**
```
>wave:127,mV:0.00,hr:75,rr:800,pr:160,qrs:90,qtc:400,r:1.05,st:0.02
```

---

## 6. Subsistema de Comunicación WiFi

### 6.1 Matriz de Decisión: Arquitectura de Comunicación y Aplicación

#### 6.1.1 Criterios de Evaluación y Ponderación

**Tabla 6.1.1a: Criterios para selección de arquitectura de comunicación**

| Criterio | Peso (%) | Justificación |
|----------|----------|---------------|
| Soporte Multi-dispositivo Simultáneo | 30% | Crítico: visualización por múltiples usuarios (estudiantes, docente) |
| Capacidad de Streaming Tiempo Real | 25% | Muy importante: latencia <100ms para waveform fluido |
| Multiplataforma (Windows/Mac/Linux/Mobile) | 20% | Importante: acceso desde cualquier dispositivo sin instalación |
| Complejidad de Desarrollo | 15% | Importante: tiempo limitado, primera implementación WiFi del equipo |
| Alcance de Comunicación | 10% | Moderado: uso en laboratorio/aula (~10-15 metros) |

**Tabla 6.1.1b: Evaluación de alternativas (escala 1-5)**

| Arquitectura | Multi-dispositivo | Streaming RT | Multiplataforma | Desarrollo | Alcance | Total Ponderado |
|--------------|-------------------|--------------|-----------------|------------|---------|-----------------|
| **Web App (WiFi AP + WebSocket)** | 5 | 5 | 5 | 4 | 4 | **4.75** ✓ |
| Android APK (WiFi Direct) | 3 | 4 | 1 | 2 | 4 | **2.85** |
| Bluetooth Serial (SPP) | 2 | 3 | 3 | 5 | 2 | **2.90** |
| USB Serial (Virtual COM Port) | 1 | 5 | 4 | 5 | 1 | **2.90** |

**Puntuación detallada:**

1. **Web App con WiFi Access Point + WebSocket** - Puntuación: 4.75
   - **Multi-dispositivo (5):** ESP32 como AP permite hasta 4 clientes WebSocket simultáneos (configurado en WiFi.softAPConfig), ideal para demostración en aula
   - **Streaming RT (5):** WebSocket full-duplex permite enviar muestras @ 100 Hz (10ms) con latencia típica <50ms, protocolo diseñado para comunicación bidireccional en tiempo real
   - **Multiplataforma (5):** Navegador web estándar (Chrome/Firefox/Safari) sin instalación, compatible con Windows/Mac/Linux/Android/iOS
   - **Desarrollo (4):** Biblioteca AsyncWebSocket (ESP32) madura, frontend HTML5 Canvas + JavaScript vanilla (sin frameworks complejos inicialmente)
   - **Alcance (4):** WiFi 2.4 GHz alcanza 10-15 metros indoor (suficiente para laboratorio), puede degradarse con obstáculos

2. **Aplicación Android APK (WiFi Direct)** - Puntuación: 2.85
   - **Multi-dispositivo (3):** WiFi Direct permite conexiones P2P pero limitado a 1 grupo activo (1 propietario + N clientes), complejidad de gestión de grupos
   - **Streaming RT (4):** Socket TCP/UDP con buen desempeño, latencia similar a WebSocket
   - **Multiplataforma (1):** Limitado a Android, usuarios con iPhone/Windows quedarían excluidos (40% del mercado en Ecuador tiene iOS)
   - **Desarrollo (2):** Requiere aprender Android Studio, Java/Kotlin, compilación APK, permisos WiFi Direct, depuración en dispositivo físico
   - **Alcance (4):** WiFi Direct alcance similar a WiFi AP tradicional

3. **Bluetooth Serial Profile (SPP)** - Puntuación: 2.90
   - **Multi-dispositivo (2):** Bluetooth Classic permite solo 1 conexión activa a la vez (limitación SPP), no es broadcast
   - **Streaming RT (3):** Throughput máximo ~1 Mbps (vs 50 Mbps WiFi), latencia típica 100-200ms, suficiente pero con menos margen
   - **Multiplataforma (3):** Compatible con Android/iOS/Windows/Linux pero requiere app nativa en cada plataforma (SerialBluetooth en Android, CoreBluetooth en iOS)
   - **Desarrollo (5):** Biblioteca BluetoothSerial (ESP32) muy simple, protocolo serial estándar
   - **Alcance (2):** 10 metros típico (Bluetooth Class 2), más susceptible a interferencias en 2.4 GHz

4. **USB Serial (Virtual COM Port)** - Puntuación: 2.90
   - **Multi-dispositivo (1):** Cable USB limita a 1 dispositivo conectado físicamente, requiere hub USB para múltiples usuarios
   - **Streaming RT (5):** USB CDC a 115200 baud (~11.5 KB/s) suficiente, latencia mínima <10ms
   - **Multiplataforma (4):** Compatible con Windows/Mac/Linux mediante drivers CDC estándar, apps como PuTTY/CoolTerm/screen
   - **Desarrollo (5):** Serial.print() nativo de Arduino/ESP32, implementación trivial
   - **Alcance (1):** Cable USB máximo 5 metros (USB 2.0 spec), limita movilidad

**Conclusión:**

La arquitectura **Web App con WiFi Access Point + WebSocket** obtuvo la mayor puntuación (4.75/5.00) por permitir acceso simultáneo desde múltiples dispositivos mediante navegador web estándar. Esta decisión fue **crítica** para el contexto educativo del proyecto:

- **Escenario típico:** Docente demuestra señal ECG normal en pantalla Nextion, estudiantes (5-10) observan waveform/métricas en sus smartphones/laptops sin instalar software
- **Ventaja clave:** WebSocket permite streaming bidireccional (ESP32 envía muestras, app web envía comandos cambio de condición/parámetros) con protocolo estandarizado RFC 6455
- **Implementación sencilla:** Frontend en HTML5 Canvas + Vanilla JS (no React inicialmente para simplificar), backend ESP32 usa librería ESPAsyncWebServer

**Razones de descarte de alternativas:**

- **Android APK:** Excluye usuarios iPhone/iPad (40% mercado Ecuador según datos INEC 2023), requiere instalación desde APK (Google Play Store costaría $25 registro desarrollador)
- **Bluetooth SPP:** Limitación 1 conexión activa imposibilita uso multi-usuario, latencia 100-200ms marginal para waveform 100 Hz
- **USB Serial:** Requiere cables físicos (costo $2-5 cada uno × 10 estudiantes = $20-50), limita movilidad en aula

**Arquitectura implementada:**

1. **ESP32 como WiFi Access Point:** SSID `BioSimulator_Pro`, IP `192.168.4.1`, sin autenticación WPA2 (simplifica conexión inicial)
2. **Servidor HTTP (puerto 80):** Sirve `index.html`, `app.js`, `styles.css` almacenados en SPIFFS/LittleFS
3. **Servidor WebSocket (puerto 81):** Streaming de señal @ 100 Hz (JSON: `{"type":"signal","value":0.85,"dac":172}`) y métricas @ 4 Hz

**Tabla 6.1.1c: Comparativa de ancho de banda**

| Protocolo | Throughput máx | Latencia típica | Clientes simultáneos |
|-----------|----------------|-----------------|----------------------|
| WebSocket/WiFi | 50 Mbps | 20-50 ms | 4 (ESP32 limitado por RAM) |
| WiFi Direct | 50 Mbps | 30-60 ms | 1 grupo (1 propietario + N clientes) |
| Bluetooth SPP | 1 Mbps | 100-200 ms | 1 |
| USB Serial | 1.5 Mbps @ 115200 | <10 ms | 1 |

**Ancho de banda requerido:** ~69 kbps (señal 64 kbps + métricas 4.8 kbps) → Margen holgado en WiFi.

### 6.2 Configuración del Access Point

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
| ECG | [-0.5, +1.5] mV | `DAC = (mV + 0.5) / 2.0 × 255` |
| EMG | [-5.0, +5.0] mV | `DAC = (mV + 5.0) / 10.0 × 255` |
| PPG (AC) | [0, 150] mV | `DAC = AC / 150 × 255` |

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

### 10.8 Validación del Diseño Analógico mediante Análisis FFT

Para garantizar que el diseño del filtro analógico y la cadena de salida DAC→RC→BNC cumple con las especificaciones de contenido espectral de las señales biomédicas, se utiliza el conjunto de herramientas de análisis FFT desarrolladas específicamente para este proyecto.

#### 10.8.1 Herramientas de Análisis FFT Disponibles

El proyecto incluye tres scripts Python en el directorio `tools/` que permiten validar el contenido espectral de las señales:

```
tools/
├── fft_spectrum_analyzer.py    # Análisis FFT de salida analógica (osciloscopio)
├── model_fft_analysis.py       # Análisis FFT de modelos matemáticos
├── signal_validator.py         # Validación de rangos clínicos
└── clinical_ranges.py          # Definiciones de rangos de referencia
```

**Tabla 10.4: Propósito de cada herramienta FFT**

| Script | Entrada | Salida | Propósito |
|--------|---------|--------|-----------|
| `fft_spectrum_analyzer.py` | Archivo CSV de osciloscopio | Espectro de potencia, armónicos | Validar salida analógica real |
| `model_fft_analysis.py` | Parámetros de modelo ECG/EMG/PPG | Espectro teórico | Verificar contenido espectral del modelo |
| `signal_validator.py` | Señal generada | Métricas vs rangos clínicos | Confirmar parámetros en rango |

#### 10.8.2 Procedimiento de Validación del Filtro RC

```
┌─────────────────────────────────────────────────────────────────────────────┐
│             VALIDACIÓN DE FILTRO RC MEDIANTE ANÁLISIS FFT                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  PASO 1: Captura de señal con osciloscopio                                  │
│  ┌──────────────────────────────────────────────────────────────────┐       │
│  │  • Conectar sonda de osciloscopio a BNC                          │       │
│  │  • Configurar muestreo ≥ 10 kSa/s (2.5× Fs_timer)                │       │
│  │  • Exportar datos como CSV (tiempo, voltaje)                     │       │
│  └──────────────────────────────────────────────────────────────────┘       │
│                              │                                              │
│                              ▼                                              │
│  PASO 2: Análisis FFT con fft_spectrum_analyzer.py                          │
│  ┌──────────────────────────────────────────────────────────────────┐       │
│  │  > python tools/fft_spectrum_analyzer.py captura.csv              │       │
│  │                                                                   │       │
│  │  Salida esperada:                                                 │       │
│  │  • Espectro de potencia con picos en frecuencias fundamentales   │       │
│  │  • Atenuación -3dB verificada en fc = 1.59 kHz (filtro RC)       │       │
│  │  • Componentes > 1.59 kHz atenuados (aliasing eliminado)         │       │
│  └──────────────────────────────────────────────────────────────────┘       │
│                              │                                              │
│                              ▼                                              │
│  PASO 3: Comparación con modelo teórico                                     │
│  ┌──────────────────────────────────────────────────────────────────┐       │
│  │  > python tools/model_fft_analysis.py --signal ecg                │       │
│  │                                                                   │       │
│  │  Comparar:                                                        │       │
│  │  • Espectro capturado vs espectro teórico del modelo             │       │
│  │  • Verificar que HR fundamental (1.25 Hz @ 75 bpm) presente      │       │
│  │  • Confirmar armónicos hasta 40 Hz (límite ECG clínico)          │       │
│  └──────────────────────────────────────────────────────────────────┘       │
│                                                                             │
│  CRITERIO DE ACEPTACIÓN:                                                    │
│  ✓ Frecuencia fundamental coincide con parámetro configurado               │
│  ✓ Contenido espectral dentro de banda de paso (ECG: 0.05-150 Hz)          │
│  ✓ Atenuación > 20 dB para f > 2 kHz (efectividad del filtro RC)           │
│  ✓ Ausencia de picos en 4 kHz (aliasing DAC suprimido)                     │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

#### 10.8.3 Documentación de Resultados FFT

Los resultados del análisis FFT se documentan en:

```
docs/fft_analysis/
├── FFT_ANALYSIS_DOCUMENTATION.md    # Metodología y resultados
└── fft_modelos_reporte.txt          # Reporte de análisis de modelos
```

**Contenido del reporte FFT:**
- Frecuencia fundamental detectada vs configurada
- Potencia espectral en bandas de interés
- Verificación de atenuación del filtro RC
- Comparación con literatura clínica (Rangos de referencia de RANGOS_CLINICOS.md)

#### 10.8.4 Validación Cruzada: FFT vs Serial Plotter vs Osciloscopio

Para garantizar consistencia en la cadena de señal, se realiza validación cruzada entre los tres métodos de observación:

| Aspecto | Serial Plotter | Osciloscopio + FFT | Criterio |
|---------|----------------|-------------------|----------|
| Frecuencia cardíaca (ECG) | HR en métricas | Pico en 1.0-2.5 Hz | ±1 Hz |
| Amplitud R (ECG) | mV reportado | Amplitud pico-pico | ±10% |
| Contenido EMG | RMS calculado | Banda 20-500 Hz | Presencia de energía |
| Frecuencia respiratoria (PPG) | RR estimado | Modulación < 0.5 Hz | ±2 bpm |

Esta validación cruzada asegura que la señal analógica en el conector BNC es una representación fiel del modelo matemático implementado, después de pasar por la cadena DAC→filtro RC→buffer.

---

## 11. Estructura del Programa BioSignalSimulator Pro

El firmware del BioSignalSimulator Pro se organizó siguiendo principios de modularidad y separación de responsabilidades. A continuación se presenta el desglose completo de la estructura de archivos y sus funciones.

### 11.1 Árbol de Directorios

```
BioSignalSimulator Pro/
├── include/                          # Archivos de cabecera (.h)
│   ├── config.h                      # Configuración global: pines, frecuencias, constantes
│   ├── data/                         # Estructuras de datos
│   │   ├── signal_types.h            # Enums: SignalType, SignalCondition, SignalState
│   │   └── param_limits.h            # Límites de parámetros por señal y condición
│   ├── models/                       # Modelos matemáticos de señales
│   │   ├── ecg_model.h               # Modelo McSharry ECG (ODE + RK4)
│   │   ├── emg_model.h               # Modelo Fuglevand EMG (MUAPs + filtrado)
│   │   └── ppg_model.h               # Modelo Allen PPG (doble gaussiana)
│   ├── core/                         # Núcleo del sistema
│   │   ├── signal_engine.h           # Motor de generación: buffer, timer ISR, DAC
│   │   ├── state_machine.h           # Máquina de estados del sistema
│   │   └── param_controller.h        # Controlador de parámetros con validación
│   ├── comm/                         # Comunicaciones
│   │   ├── nextion_driver.h          # Driver para pantalla Nextion (UART2)
│   │   ├── serial_handler.h          # Manejador de comandos serial (debug)
│   │   └── wifi_server.h             # Servidor WiFi AP + WebSocket
│   └── hw/                           # Hardware adicional
│       └── cd4051_mux.h              # Driver del multiplexor CD4051
│
├── src/                              # Implementaciones (.cpp)
│   ├── main.cpp                      # Punto de entrada, setup(), loop()
│   ├── models/                       # Implementación de modelos
│   │   ├── ecg_model.cpp             # ~600 líneas: ODE, condiciones clínicas
│   │   ├── emg_model.cpp             # ~500 líneas: MUAPs, reclutamiento
│   │   └── ppg_model.cpp             # ~400 líneas: pulso, PI dinámico
│   ├── core/                         # Implementación del núcleo
│   │   ├── signal_engine.cpp         # ~500 líneas: timer, buffer, interpolación
│   │   ├── state_machine.cpp         # ~200 líneas: transiciones de estado
│   │   └── param_controller.cpp      # ~300 líneas: validación, aplicación
│   ├── comm/                         # Implementación de comunicaciones
│   │   ├── nextion_driver.cpp        # ~400 líneas: waveform, métricas, páginas
│   │   ├── serial_handler.cpp        # ~150 líneas: comandos de debug
│   │   └── wifi_server.cpp           # ~350 líneas: AP, HTTP, WebSocket
│   └── hw/                           # Implementación de hardware
│       └── cd4051_mux.cpp            # ~150 líneas: selección de canal
│
├── data/                             # Archivos para SPIFFS (app web)
│   ├── index.html                    # Interfaz web principal
│   ├── app.js                        # Lógica JavaScript (WebSocket, gráficos)
│   └── styles.css                    # Estilos CSS
│
├── docs/                             # Documentación técnica
│   ├── metodos/metodos/              # Metodologías de diseño
│   │   ├── METODOLOGIA_COMPUTACIONAL.md
│   │   ├── METODOLOGIA_ELECTRONICA.md
│   │   ├── METODOLOGIA_MECANICA.md
│   │   ├── METODOLOGIA_SIGNALGEN.md
│   │   ├── METODOLOGIA_APPWEB.md
│   │   └── RANGOS_CLINICOS.md
│   ├── fft_analysis/                 # Análisis espectral
│   │   ├── FFT_ANALYSIS_DOCUMENTATION.md
│   │   └── fft_modelo_*.png
│   └── COMANDOS.md                   # Referencia de comandos serial
│
├── tools/                            # Scripts de desarrollo
│   ├── model_fft_analysis.py         # Análisis FFT de modelos
│   ├── signal_validator.py           # Validador de señales vía serial
│   └── clinical_ranges.py            # Rangos clínicos de referencia
│
└── platformio.ini                    # Configuración de PlatformIO
```

### 11.2 Módulos Principales y sus Responsabilidades

**Tabla 11.1: Descripción de módulos del sistema**

| Módulo | Archivo Principal | Responsabilidad | Dependencias |
|--------|-------------------|-----------------|--------------|
| **SignalEngine** | `signal_engine.cpp` | Generación de muestras, buffer circular, timer ISR, salida DAC | Modelos, config.h |
| **ECGModel** | `ecg_model.cpp` | Síntesis de ECG (McSharry ODE + RK4), 8 condiciones clínicas | config.h |
| **EMGModel** | `emg_model.cpp` | Síntesis de EMG (Fuglevand MUAPs), filtrado Butterworth | config.h |
| **PPGModel** | `ppg_model.cpp` | Síntesis de PPG (Allen gaussiano), PI y SpO2 dinámicos | config.h |
| **StateMachine** | `state_machine.cpp` | Estados del sistema: INIT→PORTADA→MENU→SIMULATING | - |
| **ParamController** | `param_controller.cpp` | Validación y aplicación de parámetros | param_limits.h |
| **NextionDriver** | `nextion_driver.cpp` | Comunicación UART2, waveforms, métricas, eventos táctiles | Serial2 |
| **WiFiServer** | `wifi_server.cpp` | Access Point, servidor HTTP/WebSocket, streaming | ESPAsyncWebServer |
| **CD4051Mux** | `cd4051_mux.cpp` | Selección de canal del multiplexor para filtros RC | GPIO32/33 |

### 11.3 Flujo de Ejecución

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        FLUJO DE EJECUCIÓN DEL FIRMWARE                       │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  1. INICIALIZACIÓN (setup())                                                │
│     ├── Serial.begin(115200)                                                │
│     ├── SignalEngine::getInstance().begin()                                 │
│     │   ├── Configurar timer ISR @ 4 kHz                                    │
│     │   └── Inicializar buffer circular (2048 muestras)                     │
│     ├── mux.begin()  →  GPIO32/33 como salidas, CH1 por defecto             │
│     ├── nextion.begin()  →  UART2 @ 115200 baud                             │
│     ├── wifiServer.begin()  →  AP "BioSimulator_Pro", IP 192.168.4.1        │
│     ├── stateMachine.init()  →  Estado inicial: INIT                        │
│     └── xTaskCreatePinnedToCore(generationTask, Core 1)                     │
│                                                                             │
│  2. BUCLE PRINCIPAL (loop() en Core 0)                                      │
│     ├── nextion.process()  →  Leer eventos táctiles                         │
│     ├── serialHandler.process()  →  Comandos de debug                       │
│     ├── wifiServer.process()  →  Clientes WebSocket                         │
│     ├── stateMachine.update()  →  Transiciones de estado                    │
│     └── updateLEDStatus()  →  RGB según estado                              │
│                                                                             │
│  3. TAREA DE GENERACIÓN (generationTask en Core 1)                          │
│     └── while(true):                                                        │
│         ├── if (state == RUNNING):                                          │
│         │   ├── Calcular espacio disponible en buffer                       │
│         │   ├── Generar muestras con modelo activo                          │
│         │   ├── Interpolar a 4 kHz                                          │
│         │   └── Escribir en buffer circular                                 │
│         └── vTaskDelay(1)  →  Yield a otras tareas                          │
│                                                                             │
│  4. ISR DEL TIMER (timerISR @ 4 kHz, IRAM_ATTR)                             │
│     ├── Leer muestra de buffer[readIndex]                                   │
│     ├── dacWrite(GPIO25, value)                                             │
│     ├── readIndex = (readIndex + 1) % BUFFER_SIZE                           │
│     └── Detectar underruns (estadística)                                    │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 11.4 Constantes de Configuración Principales

**Tabla 11.2: Constantes definidas en config.h**

| Constante | Valor | Descripción |
|-----------|-------|-------------|
| `FS_TIMER_HZ` | 4000 | Frecuencia del timer DAC (Hz) |
| `SIGNAL_BUFFER_SIZE` | 2048 | Tamaño del buffer circular |
| `DAC_SIGNAL_PIN` | 25 | GPIO para salida DAC |
| `MUX_SELECT_S0` | 32 | GPIO selector S0 del CD4051 |
| `MUX_SELECT_S1` | 33 | GPIO selector S1 del CD4051 |
| `NEXTION_RX_PIN` | 16 | GPIO RX para Nextion |
| `NEXTION_TX_PIN` | 17 | GPIO TX para Nextion |
| `NEXTION_BAUD` | 115200 | Baudios comunicación Nextion |
| `LED_RGB_RED` | 21 | GPIO LED rojo |
| `LED_RGB_GREEN` | 22 | GPIO LED verde |
| `LED_RGB_BLUE` | 23 | GPIO LED azul |

---

## 12. Consideraciones de Implementación

### 12.1 Optimizaciones Realizadas

1. **Atributo IRAM_ATTR:** La función `timerISR()` se colocó en RAM interna para ejecución rápida (<5 µs).

2. **Aritmética de punto fijo:** Los cálculos de interpolación utilizaron multiplicaciones enteras donde fue posible.

3. **Buffer circular con índices volátiles:** Acceso ISR-safe sin necesidad de mutexes adicionales.

4. **Downsampling por contador:** Evita divisiones en tiempo real; solo operaciones módulo.

### 12.2 Limitaciones Conocidas

| Limitación | Descripción | Mitigación |
|------------|-------------|------------|
| DAC 8-bit | Resolución limitada (256 niveles) | Suficiente para aplicación educativa |
| WiFi bloquea ADC2 | No se puede leer ADC2 con WiFi activo | ADC loopback usa GPIO34 (ADC1) |
| Buffer underrun | Posible si Core 1 se bloquea | Detección y contador de eventos |

---

## 13. Referencias

[1] Espressif Systems, "ESP32 Technical Reference Manual," Version 4.5.

[2] FreeRTOS, "FreeRTOS Reference Manual," Version 10.4.3.

[3] ITEAD Studio, "Nextion Instruction Set," Version 1.0.

[4] WebSocket Protocol, RFC 6455, IETF.

[5] McSharry PE, Clifford GD, Tarassenko L, Smith LA. "A dynamical model for generating synthetic electrocardiogram signals." IEEE Trans Biomed Eng. 2003;50(3):289-294.

[6] Fuglevand AJ, Winter DA, Patla AE. "Models of recruitment and rate coding organization in motor-unit pools." J Neurophysiol. 1993;70(6):2470-2488.

[7] Allen J, Murray A. "Age-related changes in peripheral pulse timing characteristics at the ears, fingers and toes." J Hum Hypertens. 2002;16(10):711-717.

[8] SENIAM Project, "Surface ElectroMyoGraphy for the Non-Invasive Assessment of Muscles," European Recommendations.

[9] Texas Instruments, "Input and Output Capacitor Selection," Application Report SLTA055.

[10] CD4051B Datasheet, Texas Instruments, SCHS047H.

---

*Documento de Metodología Computacional - BioSignalSimulator Pro*  
*Grupo #22: Scarlet Sánchez y Rafael Mata - ESPOL*
