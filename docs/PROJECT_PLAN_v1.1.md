# BioSignalSimulator Pro - Plan de Proyecto v1.1

**Diseño e implementación de un simulador de señales fisiológicas (EKG, EMG, PPG) para el Laboratorio de Neurociencias y Bioingeniería**

---

| Campo | Valor |
|-------|-------|
| **Versión** | 1.1.0 |
| **Estado** | En Desarrollo |
| **Nombre Oficial** | BioSignalSimulator Pro |
| **Propósito** | Dispositivo portátil para simular señales fisiológicas humanas con visualización en tiempo real y parametrización dinámica |

---

## Tabla de Contenidos

1. [Resumen Ejecutivo](#1-resumen-ejecutivo)
2. [Objetivos del Proyecto](#2-objetivos-del-proyecto)
3. [Arquitectura del Sistema](#3-arquitectura-del-sistema)
4. [Especificación de Hardware](#4-especificación-de-hardware)
5. [Especificación de Software](#5-especificación-de-software)
6. [Modelos de Señales](#6-modelos-de-señales)
7. [Interfaz de Usuario (LVGL)](#7-interfaz-de-usuario-lvgl)
8. [Aplicación Web](#8-aplicación-web)
9. [Diseño Electrónico](#9-diseño-electrónico)
10. [Diseño Mecánico](#10-diseño-mecánico)
11. [Validación y Métricas](#11-validación-y-métricas)
12. [Estructura del Proyecto](#12-estructura-del-proyecto)
13. [Plan de Implementación](#13-plan-de-implementación)
14. [Referencias Científicas](#14-referencias-científicas)
15. [Decisiones Pendientes](#15-decisiones-pendientes)

---

## 1. Resumen Ejecutivo

### 1.1 Descripción del Proyecto

**BioSignalSimulator Pro** es un dispositivo portátil de simulación de señales fisiológicas diseñado para el Laboratorio de Neurociencias y Bioingeniería. El sistema genera señales de **ECG, EMG y PPG** de forma **dinámica** (no estática/bucle) con características morfológicas y temporales clínicamente válidas.

### 1.2 Características Principales

| Característica | Descripción |
|----------------|-------------|
| **Señales** | ECG (8 condiciones), EMG (8 condiciones), PPG (6 condiciones) |
| **Generación** | Dinámica con variabilidad latido a latido (HRV real) |
| **Visualización** | Pantalla táctil 7" 800×480 con gráficas en tiempo real |
| **Parametrización** | En tiempo real con límites clínicos por condición |
| **Salida Analógica** | DAC para conexión con osciloscopio/equipos de laboratorio |
| **Conectividad** | WiFi para aplicación web de visualización remota |
| **Portabilidad** | Batería de litio recargable |

### 1.3 Entregables

| Entregable | Descripción | Estado |
|------------|-------------|--------|
| Firmware ESP32 Cerebro | Generación de señales (modelos matemáticos) | ✅ Completado |
| Firmware ESP32 HMI | Interfaz gráfica LVGL | 🔄 Pendiente |
| Interfaz SquareLine Studio | Diseño UI exportable | 🔄 Pendiente |
| Aplicación Web | Visualización remota para estudiantes | 🔄 Pendiente |
| Diseño PCB | Circuito integrado con alimentación | 🔄 Pendiente |
| Diseño Mecánico (STL) | Carcasa para impresión 3D | 🔄 Pendiente |
| Documentación Científica | Bases matemáticas y computacionales | ✅ Completado |
| Sistema de Validación | Scripts Python para verificación | ✅ Completado |

---

## 2. Objetivos del Proyecto

### 2.1 Objetivo General

Diseñar e implementar un dispositivo portátil que permita simular señales fisiológicas humanas (electrocardiografía, electromiografía, fotopletismografía) con visualización en tiempo real y parametrización dinámica para uso educativo en laboratorio.

### 2.2 Objetivos Específicos

| Área | Objetivo |
|------|----------|
| **Matemáticas** | Implementar modelos científicamente validados (McSharry ECG, Fuglevand EMG, Allen PPG) |
| **Computación** | Generar señales en tiempo real a 1kHz con variabilidad latido-a-latido |
| **Electrónica** | Diseñar PCB con alimentación por batería de litio y estándares IEC 60601-1 |
| **Software Embebido** | Arquitectura dual-core con comunicación UART de baja latencia |
| **Interfaz de Usuario** | Pantalla táctil con selección de señales, condiciones y parámetros |
| **Aplicación Web** | Visualización remota vía WiFi para múltiples estudiantes |
| **Mecánica** | Carcasa ergonómica para uso portátil en laboratorio |

### 2.3 Requisitos Funcionales

| ID | Requisito | Prioridad |
|----|-----------|-----------|
| RF-01 | Generar señales ECG, EMG, PPG dinámicas (no bucle estático) | Alta |
| RF-02 | Mostrar señal en tiempo real en pantalla táctil 7" | Alta |
| RF-03 | Permitir selección de tipo de señal y condición patológica | Alta |
| RF-04 | Parametrizar señales con rangos clínicos válidos | Alta |
| RF-05 | Proporcionar salida analógica DAC para osciloscopio | Alta |
| RF-06 | Transmitir señal vía WiFi a aplicación web | Media |
| RF-07 | Operar con batería de litio recargable | Media |
| RF-08 | Guardar/exportar datos de señales | Media |

---

## 3. Arquitectura del Sistema

### 3.1 Arquitectura Dual-ESP32

El sistema utiliza **dos microcontroladores ESP32** con roles especializados, comunicados por **UART de alta velocidad**:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    BIOSIGNALSIMULATOR PRO v1.1                              │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────┐     UART 921600 baud     ┌─────────────────────────────────┐
│      ESP32 CEREBRO              │◄──────────────────────────►│      ESP32-S3 HMI               │
│      (NodeMCU v1.1)             │                            │      (ELECROW 7" Display)       │
│                                 │                            │                                 │
│  ┌───────────────────────────┐  │                            │  ┌───────────────────────────┐  │
│  │       CORE 0              │  │                            │  │       CORE 0              │  │
│  │   Comunicación UART       │  │                            │  │   LVGL Task               │  │
│  │   Recepción comandos      │  │                            │  │   Touch Input             │  │
│  │   Envío datos señal       │  │                            │  │   Animaciones             │  │
│  └───────────────────────────┘  │                            │  └───────────────────────────┘  │
│                                 │                            │                                 │
│  ┌───────────────────────────┐  │                            │  ┌───────────────────────────┐  │
│  │       CORE 1              │  │                            │  │       CORE 1              │  │
│  │   Generación Señal 1kHz   │  │                            │  │   Display RGB Refresh     │  │
│  │   Modelos ECG/EMG/PPG     │  │                            │  │   Buffer Management       │  │
│  │   Salida DAC              │  │                            │  │                           │  │
│  └───────────────────────────┘  │                            │  └───────────────────────────┘  │
│                                 │                            │                                 │
│  Salidas:                       │                            │  Pantalla:                      │
│  • DAC GPIO25 → Osciloscopio    │                            │  • 7" TFT 800×480 RGB           │
│  • WiFi → App Web               │                            │  • Táctil capacitivo            │
└─────────────────────────────────┘                            └─────────────────────────────────┘
```

### 3.2 Justificación de Arquitectura Dual

| Problema con Nextion | Solución con Dual-ESP32 |
|---------------------|------------------------|
| Waveform barrido de derecha a izquierda (señal invertida en tiempo) | Control total del renderizado con LVGL |
| No permite invertir dirección de gráfica | Buffer circular con scroll izquierda→derecha |
| Latencia en comunicación serial | UART 921600 baud dedicado |
| Limitaciones de personalización | UI completamente personalizable con SquareLine Studio |

### 3.3 Flujo de Datos

```
┌──────────────┐    ┌──────────────┐    ┌──────────────┐    ┌──────────────┐
│   Modelo     │───►│   Buffer     │───►│    UART      │───►│    LVGL      │
│  ECG/EMG/PPG │    │   Circular   │    │   TX→RX      │    │   Chart      │
│  (1kHz)      │    │   (2KB)      │    │   921600     │    │   (100Hz)    │
└──────────────┘    └──────────────┘    └──────────────┘    └──────────────┘
       │                                                           │
       ▼                                                           ▼
┌──────────────┐                                          ┌──────────────┐
│    DAC       │                                          │   Display    │
│   GPIO25     │                                          │   800×480    │
│   0-3.3V     │                                          │   RGB        │
└──────────────┘                                          └──────────────┘
```

### 3.4 Protocolo de Comunicación UART

**ESP32 Cerebro → ESP32 HMI (Datos de señal):**
```
Formato: [HEADER][TYPE][VALUE_H][VALUE_L][METRICS...][CHECKSUM]

Ejemplo ECG:
0xAA 0x01 0x00 0x80 [HR_H][HR_L][RR_H][RR_L][BEATS_H][BEATS_L] 0xXX

- HEADER: 0xAA (inicio de trama)
- TYPE: 0x01=ECG, 0x02=EMG, 0x03=PPG
- VALUE: Valor DAC 0-255 (16-bit para expansión futura)
- METRICS: Métricas específicas por señal
- CHECKSUM: XOR de todos los bytes
```

**ESP32 HMI → ESP32 Cerebro (Comandos):**
```
Formato: [HEADER][CMD][PARAM1][PARAM2][CHECKSUM]

Comandos:
0x10 = Seleccionar señal (PARAM1: tipo)
0x11 = Seleccionar condición (PARAM1: condición)
0x20 = Iniciar simulación
0x21 = Pausar simulación
0x22 = Detener simulación
0x30 = Cambiar parámetro (PARAM1: id, PARAM2: valor)
```

---

## 4. Especificación de Hardware

### 4.1 Componentes Principales

| Componente | Modelo | Función | Cantidad |
|------------|--------|---------|----------|
| **MCU Cerebro** | ESP32-WROOM-32 (NodeMCU v1.1) | Generación de señales | 1 |
| **MCU + Display** | ELECROW ESP32-S3 7" 800×480 | Interfaz táctil | 1 |
| **Batería** | Li-ion 18650 2200mAh (2P paralelo) | 3.7V, 4400mAh | 2 |
| **Cargador** | TP4056 con protección | Carga USB + protección | 1 |
| **Regulador** | MT3608 Boost (3.7V→5V) | Alimentación 5V | 1 |
| **Op-Amp** | MCP6002 | Buffer salida analógica | 1 |
| **Conector Salida** | BNC hembra | Salida analógica | 1 |

### 4.2 ELECROW ESP32-S3 Display 7" - Especificaciones

| Parámetro | Valor |
|-----------|-------|
| **MCU** | ESP32-S3-WROOM-1-N4R8 |
| **Procesador** | Dual-core LX7 @ 240MHz |
| **RAM** | 512KB SRAM + 8MB PSRAM |
| **Flash** | 4MB |
| **Display** | 7" TFT LCD 800×480 RGB |
| **Touch** | Capacitivo GT911 (I2C) |
| **Interfaz Display** | RGB 16-bit paralelo |
| **Conectores** | UART0, I2C×2, GPIO, BAT, TF |
| **Dimensiones** | 183mm × 107mm |

### 4.3 Pinout ESP32 Cerebro (NodeMCU v1.1)

| Pin | Función | Descripción |
|-----|---------|-------------|
| GPIO25 | DAC1 | Salida señal principal (ECG/EMG/PPG) |
| GPIO26 | DAC2 | Salida secundaria (opcional) |
| GPIO16 | UART2_RX | Recepción desde HMI |
| GPIO17 | UART2_TX | Transmisión hacia HMI |
| GPIO4 | LED_R | LED RGB - Rojo |
| GPIO5 | LED_G | LED RGB - Verde |
| GPIO18 | LED_B | LED RGB - Azul |
| GPIO2 | LED_STATUS | LED interno |

### 4.4 Conexión UART entre ESP32s

| ESP32 Cerebro | ESP32-S3 HMI | Función |
|---------------|--------------|---------|
| GPIO17 (TX2) | UART0_RX | Datos Cerebro→HMI |
| GPIO16 (RX2) | UART0_TX | Comandos HMI→Cerebro |
| GND | GND | Referencia común |

### 4.5 Sistema de Alimentación

> **Documentación detallada:** Ver [HARDWARE_ELECTRONICS.md](HARDWARE_ELECTRONICS.md)

```
┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐
│  2× 18650       │────►│   TP4056        │────►│   SWITCH        │
│  3.7V 2200mAh   │     │   Cargador      │     │   ON/OFF        │
│  (PARALELO)     │     │   + Protección  │     │                 │
└─────────────────┘     └─────────────────┘     └────────┬────────┘
                                                         │ 3.7V
                        ┌────────────────────────────────┴────────────────────────────┐
                        │                         BORNERA                             │
                        │                      DISTRIBUCIÓN                           │
                        └────────────┬─────────────────────────────┬──────────────────┘
                                     │                             │
                              ┌──────▼──────┐               ┌──────▼──────┐
                              │  ELECROW    │               │  MT3608     │
                              │  HMI        │               │  BOOST      │
                              │  (BAT port) │               │  3.7V→5V    │
                              └──────┬──────┘               └──────┬──────┘
                                     │                             │ 5V
                              ┌──────▼──────┐               ┌──────▼──────┐
                              │  ESP32-S3   │               │  ESP32      │
                              │  + LCD 7"   │               │  Cerebro    │
                              └─────────────┘               └─────────────┘
```

**Autonomía calculada:** 3.8-7.4 horas (típico 5.5h, mínimo 3.8h)

---

## 5. Especificación de Software

### 5.1 Stack Tecnológico

| Capa | ESP32 Cerebro | ESP32-S3 HMI |
|------|---------------|--------------|
| **Framework** | Arduino + FreeRTOS | Arduino + FreeRTOS |
| **IDE** | PlatformIO | PlatformIO |
| **Gráficos** | N/A | LVGL 8.x |
| **UI Design** | N/A | SquareLine Studio |
| **Comunicación** | UART + WiFi | UART |

### 5.2 Firmware ESP32 Cerebro

**Responsabilidades:**
- Generación de señales a 1kHz (modelos matemáticos)
- Salida DAC para osciloscopio
- Comunicación UART con HMI
- Servidor WiFi para aplicación web
- Gestión de parámetros y condiciones

**Estructura de Tareas FreeRTOS:**

| Tarea | Core | Prioridad | Función |
|-------|------|-----------|---------|
| SignalGenTask | 1 | 5 (Alta) | Generación señal 1kHz |
| UARTCommTask | 0 | 3 (Media) | Comunicación con HMI |
| WiFiServerTask | 0 | 2 (Baja) | Servidor web/WebSocket |

### 5.3 Firmware ESP32-S3 HMI

**Responsabilidades:**
- Renderizado de interfaz LVGL
- Gestión de entrada táctil
- Recepción y visualización de datos de señal
- Envío de comandos al Cerebro

**Estructura de Tareas FreeRTOS:**

| Tarea | Core | Prioridad | Función |
|-------|------|-----------|---------|
| LVGLTask | 0 | 4 (Alta) | Renderizado UI |
| UARTRecvTask | 0 | 3 (Media) | Recepción datos señal |
| TouchTask | 0 | 2 (Media) | Procesamiento touch |

### 5.4 Interfaz LVGL (SquareLine Studio)

La interfaz se diseñará en **SquareLine Studio** y se exportará como código C para LVGL.

**Pantallas planificadas:**

| Pantalla | Descripción |
|----------|-------------|
| Splash | Logo y versión (2 segundos) |
| Menú Principal | Selección ECG/EMG/PPG |
| Selección Condición | Grid de condiciones por señal |
| Simulación | Waveform + métricas + controles |
| Parámetros | Sliders con rangos por condición |
| Configuración | WiFi, brillo, idioma |

---

## 6. Modelos de Señales

> **NOTA:** Los modelos matemáticos están **completamente implementados y validados**. 
> Ver `docs/README_MATHEMATICAL_BASIS.md` para fundamentos científicos.
> Ver `docs/README_COMPUTATIONAL_BASIS.md` para explicación del código.

### 6.1 Resumen de Modelos

| Señal | Modelo Base | Referencia Principal | Condiciones |
|-------|-------------|---------------------|-------------|
| **ECG** | McSharry (2003) | IEEE TBME 50(3):289-294 | 8 |
| **EMG** | Fuglevand (1993) | J Neurophysiol 70(6):2470-2488 | 8 |
| **PPG** | Allen (2007) | Physiol Meas 28(3):R1-R39 | 6 |

### 6.2 Condiciones ECG

| Condición | HR (BPM) | Características |
|-----------|----------|-----------------|
| Normal | 60-100 | Ritmo sinusal regular |
| Taquicardia | 100-180 | HR elevado |
| Bradicardia | 30-59 | HR reducido |
| Fib. Auricular | 60-180 | Sin onda P, RR irregular |
| Fib. Ventricular | 150-500 | Ondas caóticas |
| PVC | 50-120 | Extrasístoles ventriculares |
| ST Elevación | 50-110 | STEMI |
| ST Depresión | 50-150 | Isquemia |

### 6.3 Condiciones EMG

| Condición | Excitación | Características |
|-----------|------------|-----------------|
| Reposo | 0-5% MVC | Solo ruido de fondo |
| Baja | 5-20% MVC | Pocas MUs activas |
| Moderada | 20-50% MVC | Interferencia parcial |
| Alta | 50-100% MVC | Interferencia completa |
| Temblor | 10-50% | Oscilación 4-6 Hz |
| Miopatía | 10-40% | MUAPs pequeños |
| Neuropatía | 30-100% | MUAPs gigantes |
| Fasciculación | 0-30% | Disparos espontáneos |

### 6.4 Condiciones PPG

| Condición | HR (BPM) | PI (%) | SpO2 (%) |
|-----------|----------|--------|----------|
| Normal | 60-100 | 2-5 | 95-100 |
| Arritmia | 60-180 | 1-5 | 92-100 |
| Perfusión Débil | 90-140 | 0.1-0.5 | 88-98 |
| Perfusión Fuerte | 60-90 | 5-20 | 96-100 |
| Vasoconstricción | 60-100 | 0.2-0.8 | 91-100 |
| SpO2 Bajo | 90-130 | 0.5-3.5 | 70-90 |

---

## 7. Interfaz de Usuario (LVGL)

### 7.1 Diseño con SquareLine Studio

La interfaz se diseñará en **SquareLine Studio** (herramienta visual para LVGL) y se exportará como código C.

**Flujo de trabajo:**
1. Diseñar UI en SquareLine Studio
2. Exportar código C/H
3. Integrar con firmware ESP32-S3 HMI
4. Conectar callbacks a lógica de comunicación UART

### 7.2 Especificaciones de Pantalla

| Parámetro | Valor |
|-----------|-------|
| Resolución | 800 × 480 píxeles |
| Colores | 16-bit (65K colores) |
| Refresh Rate | 60 Hz (display), 30 Hz (LVGL) |
| Touch | Capacitivo multi-touch |

### 7.3 Mapa de Navegación

```
┌─────────────┐
│   SPLASH    │
│  (2 seg)    │
└──────┬──────┘
       ▼
┌─────────────┐
│    MENÚ     │◄────────────────────────────────┐
│  PRINCIPAL  │                                  │
│ ECG/EMG/PPG │                                  │
└──────┬──────┘                                  │
       ▼                                         │
┌─────────────┐                                  │
│  SELECCIÓN  │                                  │
│  CONDICIÓN  │──────────────────────────────────┤
│  (8 botones)│                                  │
└──────┬──────┘                                  │
       ▼                                         │
┌─────────────────────────────────────────────┐  │
│              SIMULACIÓN                      │  │
│  ┌─────────────────────────────────────┐    │  │
│  │         WAVEFORM (600×300)          │    │  │
│  │    ←←← Scroll tiempo real ←←←       │    │  │
│  └─────────────────────────────────────┘    │  │
│                                              │  │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐   │  │
│  │ HR: 75   │  │ RR: 800  │  │ Beats: 42│   │  │
│  └──────────┘  └──────────┘  └──────────┘   │  │
│                                              │  │
│  [⚙ Params] [⏸ Pause] [⏹ Stop] [← Atrás]   │──┘
└─────────────────────────────────────────────┘
       │
       ▼ (⚙ Params)
┌─────────────────────────────────────────────┐
│              PARÁMETROS                      │
│                                              │
│  Heart Rate (60-100 BPM):                   │
│  [====●==========] 75 BPM                   │
│                                              │
│  Amplitud QRS (0.5-2.0):                    │
│  [====●==========] 1.0x                     │
│                                              │
│  [✓ Aplicar] [✗ Cancelar] [↺ Defaults]     │
└─────────────────────────────────────────────┘
```

### 7.4 Componentes LVGL Principales

| Componente | Uso |
|------------|-----|
| `lv_chart` | Waveform de señal (modo scroll) |
| `lv_label` | Métricas numéricas |
| `lv_btn` | Botones de navegación |
| `lv_slider` | Ajuste de parámetros |
| `lv_dropdown` | Selección de condición |
| `lv_msgbox` | Confirmaciones |

---

## 8. Aplicación Web

### 8.1 Propósito

Permitir que **múltiples estudiantes** en el laboratorio visualicen la señal en tiempo real desde sus dispositivos (PC, tablet, celular) conectándose al WiFi del ESP32 Cerebro.

### 8.2 Arquitectura

```
┌─────────────┐     WiFi AP      ┌─────────────┐
│   ESP32     │◄────────────────►│  Estudiante │
│   Cerebro   │                  │  (Browser)  │
│             │                  └─────────────┘
│  WebSocket  │◄────────────────►┌─────────────┐
│   Server    │                  │  Estudiante │
│             │                  │  (Browser)  │
└─────────────┘                  └─────────────┘
```

### 8.3 Funcionalidades

| Función | Descripción |
|---------|-------------|
| **Visualización** | Gráfica en tiempo real (Canvas/WebGL) |
| **Métricas** | HR, RR, amplitudes en vivo |
| **Captura** | Guardar screenshot de señal |
| **Exportar** | Descargar datos CSV |
| **Recortar** | Seleccionar segmento de interés |

### 8.4 Stack Tecnológico

| Capa | Tecnología |
|------|------------|
| Backend | ESP32 AsyncWebServer + WebSocket |
| Frontend | HTML5 + CSS3 + JavaScript |
| Gráficas | Chart.js o Canvas nativo |
| Almacenamiento | LocalStorage (browser) |

---

## 9. Diseño Electrónico

### 9.1 Diagrama de Bloques PCB

```
┌─────────────────────────────────────────────────────────────────┐
│                         PCB PRINCIPAL                           │
│                                                                 │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐         │
│  │   USB-C     │───►│   TP4056    │───►│   Li-Po     │         │
│  │   Input     │    │   Charger   │    │   Conector  │         │
│  └─────────────┘    └─────────────┘    └─────────────┘         │
│                            │                                    │
│                     ┌──────▼──────┐                            │
│                     │  Protección │                            │
│                     │  BMS        │                            │
│                     └──────┬──────┘                            │
│                            │                                    │
│         ┌──────────────────┼──────────────────┐                │
│         │                  │                  │                │
│  ┌──────▼──────┐    ┌──────▼──────┐    ┌──────▼──────┐        │
│  │  LDO 3.3V   │    │  LDO 3.3V   │    │  LDO 5V     │        │
│  │  (Cerebro)  │    │  (HMI)      │    │  (Display)  │        │
│  └──────┬──────┘    └──────┬──────┘    └──────┬──────┘        │
│         │                  │                  │                │
│  ┌──────▼──────┐    ┌──────▼──────────────────▼──────┐        │
│  │   ESP32     │◄──►│        ELECROW 7" HMI          │        │
│  │   Cerebro   │UART│        (ESP32-S3 + LCD)        │        │
│  └──────┬──────┘    └────────────────────────────────┘        │
│         │                                                      │
│  ┌──────▼──────┐                                               │
│  │  Buffer     │                                               │
│  │  Op-Amp     │                                               │
│  └──────┬──────┘                                               │
│         │                                                      │
│  ┌──────▼──────┐                                               │
│  │  Conectores │                                               │
│  │  BNC (x3)   │                                               │
│  └─────────────┘                                               │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 9.2 Consideraciones IEC 60601-1

| Aspecto | Implementación |
|---------|----------------|
| **Aislamiento** | Galvánico entre paciente y alimentación |
| **Corriente de fuga** | < 100 µA |
| **Protección ESD** | TVS en entradas/salidas |
| **EMC** | Filtros en alimentación |

> **NOTA:** Este es un dispositivo **educativo/simulador**, no un dispositivo médico de diagnóstico. Sin embargo, se seguirán buenas prácticas de seguridad eléctrica.

### 9.3 Salidas Analógicas

| Salida | Señal | Rango | Conector |
|--------|-------|-------|----------|
| DAC1 | ECG/EMG/PPG (seleccionable) | 0-3.3V | BNC |
| DAC2 | Señal secundaria (opcional) | 0-3.3V | BNC |
| Trigger | Pulso por latido/evento | 0/3.3V | BNC |

---

## 10. Diseño Mecánico

### 10.1 Especificaciones de Carcasa

| Parámetro | Valor |
|-----------|-------|
| **Material** | PLA/PETG (impresión 3D) |
| **Dimensiones aprox.** | 200 × 130 × 40 mm |
| **Peso estimado** | < 500g (con batería) |
| **Color** | Blanco/Gris (laboratorio) |

### 10.2 Características

- Ventana para pantalla 7" táctil
- Acceso a conectores BNC
- Puerto USB-C para carga
- Indicadores LED visibles
- Ranuras de ventilación
- Tornillos de acceso para mantenimiento

### 10.3 Ergonomía

- Diseño para uso sobre mesa
- Ángulo de pantalla optimizado para visualización
- Peso balanceado para estabilidad
- Bordes redondeados para seguridad

---

## 11. Validación y Métricas

### 11.1 Sistema de Validación Implementado

El proyecto incluye un sistema completo de validación en Python:

| Archivo | Función |
|---------|---------|
| `tools/signal_validator.py` | Validación en tiempo real vía serial |
| `tools/clinical_ranges.py` | Rangos clínicos por condición |
| `tools/test_all_conditions.py` | Test automatizado de todas las condiciones |

### 11.2 Uso del Validador

```bash
# Validar ECG Normal
python signal_validator.py --port COM4 --signal ecg --condition NORMAL

# Validar PPG con SpO2 bajo
python signal_validator.py --port COM4 --signal ppg --condition LOW_SPO2

# Mostrar todos los rangos clínicos
python signal_validator.py --show-ranges

# Test automatizado
python test_all_conditions.py --port COM4
```

### 11.3 Métricas de Validación

| Métrica | Descripción | Umbral |
|---------|-------------|--------|
| HR en rango | Frecuencia cardíaca dentro de límites | 100% |
| RR en rango | Intervalo RR dentro de límites | 100% |
| Amplitud en rango | Amplitudes dentro de límites | 95% |
| Morfología correcta | Ondas PQRST/MUAPs/Pulso reconocibles | Visual |

---

## 12. Estructura del Proyecto

### 12.1 Estructura de Carpetas Propuesta

```
BioSignalSimulator_Pro/
│
├── docs/
│   ├── PROJECT_PLAN.md              ← Este documento
│   ├── README_MATHEMATICAL_BASIS.md ← Fundamentos matemáticos
│   ├── README_COMPUTATIONAL_BASIS.md← Explicación del código
│   ├── LIMITES_SLIDERS_VALIDACION.md
│   └── INTERFACE_DESIGN.md          ← (Futuro) Diseño de UI
│
├── firmware_cerebro/                 ← ESP32 NodeMCU (Generación)
│   ├── include/
│   │   ├── config.h
│   │   ├── models/
│   │   │   ├── ecg_model.h
│   │   │   ├── emg_model.h
│   │   │   └── ppg_model.h
│   │   ├── core/
│   │   │   ├── signal_engine.h
│   │   │   ├── param_controller.h
│   │   │   └── state_machine.h
│   │   ├── comm/
│   │   │   ├── uart_protocol.h
│   │   │   └── wifi_server.h
│   │   └── data/
│   │       ├── signal_types.h
│   │       └── param_limits.h
│   ├── src/
│   │   ├── main.cpp
│   │   ├── models/
│   │   ├── core/
│   │   └── comm/
│   └── platformio.ini
│
├── firmware_hmi/                     ← ESP32-S3 ELECROW (Display)
│   ├── include/
│   │   ├── config_hmi.h
│   │   ├── ui/                       ← Código exportado SquareLine
│   │   │   ├── ui.h
│   │   │   ├── ui_events.h
│   │   │   └── screens/
│   │   └── comm/
│   │       └── uart_receiver.h
│   ├── src/
│   │   ├── main.cpp
│   │   ├── ui/
│   │   └── comm/
│   ├── squareline_project/           ← Proyecto SquareLine Studio
│   └── platformio.ini
│
├── web_app/                          ← Aplicación web
│   ├── index.html
│   ├── css/
│   ├── js/
│   └── README.md
│
├── hardware/
│   ├── pcb/                          ← Proyecto KiCad
│   ├── enclosure/                    ← STL para impresión 3D
│   └── datasheets/
│
├── tools/                            ← Herramientas de validación
│   ├── clinical_ranges.py
│   ├── signal_validator.py
│   ├── test_all_conditions.py
│   └── README.md
│
└── README.md                         ← Descripción general
```

### 12.2 Archivos a Migrar/Eliminar

| Archivo Actual | Acción | Destino |
|----------------|--------|---------|
| `include/comm/nextion_driver.h` | Eliminar | N/A |
| `src/comm/nextion_driver.cpp` | Eliminar | N/A |
| `include/comm/serial_handler.h` | Renombrar | `uart_protocol.h` |
| `src/comm/serial_handler.cpp` | Modificar | `uart_protocol.cpp` |
| `include/models/*` | Mover | `firmware_cerebro/include/models/` |
| `src/models/*` | Mover | `firmware_cerebro/src/models/` |

---

## 13. Plan de Implementación

### 13.1 Fases del Proyecto

| Fase | Descripción | Estado |
|------|-------------|--------|
| **Fase 1** | Modelos matemáticos ECG/EMG/PPG | ✅ Completado |
| **Fase 2** | Sistema de validación Python | ✅ Completado |
| **Fase 3** | Reestructuración arquitectura dual-ESP32 | 🔄 En progreso |
| **Fase 4** | Diseño UI en SquareLine Studio | ⏳ Pendiente |
| **Fase 5** | Firmware ESP32-S3 HMI | ⏳ Pendiente |
| **Fase 6** | Comunicación UART Cerebro↔HMI | ⏳ Pendiente |
| **Fase 7** | Aplicación Web | ⏳ Pendiente |
| **Fase 8** | Diseño PCB | ⏳ Pendiente |
| **Fase 9** | Diseño mecánico | ⏳ Pendiente |
| **Fase 10** | Integración y pruebas finales | ⏳ Pendiente |

### 13.2 Próximos Pasos Inmediatos

1. **Reestructurar proyecto** en carpetas `firmware_cerebro/` y `firmware_hmi/`
2. **Eliminar código Nextion** obsoleto
3. **Crear placeholders** para firmware HMI
4. **Esperar archivos SquareLine Studio** del usuario
5. **Implementar protocolo UART** entre ESP32s

---

## 14. Referencias Científicas

### 14.1 Modelos de Señales

| Referencia | Aplicación |
|------------|------------|
| McSharry PE, et al. IEEE TBME 2003;50(3):289-294 | Modelo ECG dinámico |
| Fuglevand AJ, et al. J Neurophysiol 1993;70(6):2470-2488 | Reclutamiento MU |
| Allen J. Physiol Meas 2007;28(3):R1-R39 | Modelo PPG |
| Task Force ESC/NASPE. Circulation 1996;93:1043-1065 | Estándares HRV |

### 14.2 Guías Clínicas

| Referencia | Aplicación |
|------------|------------|
| AHA/ACC Guidelines 2018 | Rangos ECG |
| De Luca CJ. J Appl Biomech 1997 | Amplitudes EMG |
| Lima A, Bakker J. Intensive Care Med 2005 | Índice de Perfusión |

> Ver `docs/README_MATHEMATICAL_BASIS.md` para lista completa de referencias con DOI.

---

## 15. Decisiones Pendientes

### 15.1 Por Definir por el Usuario

| Decisión | Opciones | Impacto |
|----------|----------|---------|
| **Capacidad batería** | 3000 / 4000 / 5000 mAh | Autonomía vs peso |
| **Número de salidas BNC** | 1 / 2 / 3 | Costo vs funcionalidad |
| **Idioma interfaz** | Español / Inglés / Ambos | Complejidad UI |
| **Diseño de carcasa** | Compacto / Modular | Manufactura |

### 15.2 Pendiente de Recibir

| Item | De | Para |
|------|-----|------|
| Archivos SquareLine Studio (.spj) | Usuario | Integración firmware HMI |
| Diseño final de UI | Usuario | Implementación pantallas |
| Especificaciones exactas PCB | Usuario | Diseño KiCad |

---

## Historial de Cambios

| Versión | Fecha | Cambios |
|---------|-------|---------|
| 1.0 | 2025-12 | Documento inicial (arquitectura Nextion) |
| 1.1 | 2025-12 | Nueva arquitectura dual-ESP32, LVGL, SquareLine Studio |

---

**Fin del documento**
