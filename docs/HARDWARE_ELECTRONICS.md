# BioSimulator Pro - Diseño Electrónico de Hardware

**Versión 1.1.0 | Diciembre 2024**

---

## Índice

1. [Arquitectura General](#1-arquitectura-general)
2. [Sistema de Alimentación](#2-sistema-de-alimentación)
3. [PCB Principal](#3-pcb-principal)
4. [Cálculos de Consumo y Autonomía](#4-cálculos-de-consumo-y-autonomía)
5. [Circuito de Salida Analógica](#5-circuito-de-salida-analógica)
6. [Conexiones UART](#6-conexiones-uart)
7. [Indicador LED RGB](#7-indicador-led-rgb)
8. [Esquema General](#8-esquema-general)
9. [Lista de Materiales (BOM)](#9-lista-de-materiales-bom)

---

## 1. Arquitectura General

### 1.1 Diagrama de Bloques

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           BIOSIMULATOR PRO v1.1                             │
│                         Arquitectura con Nextion                            │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                     │
│  │  2x 18650   │───►│  BMS 2S     │───►│  SWITCH     │                     │
│  │  2800mAh    │    │  HX-2S-D01  │    │  ON/OFF     │                     │
│  │  (SERIE)    │    │             │    │             │                     │
│  └─────────────┘    └─────────────┘    └──────┬──────┘                     │
│                                               │ 7.4V                        │
│                                        ┌──────▼──────┐                      │
│                                        │  XL4015     │                      │
│                                        │  BUCK       │                      │
│                                        │  7.4V → 5V  │                      │
│                                        └──────┬──────┘                      │
│                                               │ 5V                          │
│  ┌────────────────────────────────────────────▼─────────────────────────┐  │
│  │                           PCB PRINCIPAL                              │  │
│  │  ╔═══════════════════════════════════════════════════════════════╗  │  │
│  │  ║                                                               ║  │  │
│  │  ║  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐       ║  │  │
│  │  ║  │  BORNERA    │    │   ESP32     │    │   LED RGB   │       ║  │  │
│  │  ║  │  5V / GND   │───►│   NodeMCU   │    │  (Catodo    │       ║  │  │
│  │  ║  │             │    │             │    │   comun)    │       ║  │  │
│  │  ║  └──────┬──────┘    └──────┬──────┘    └─────────────┘       ║  │  │
│  │  ║         │                  │                                  ║  │  │
│  │  ║         │           ┌──────▼──────┐                          ║  │  │
│  │  ║         │           │   MCP6002   │                          ║  │  │
│  │  ║         │           │   Buffer    │                          ║  │  │
│  │  ║         │           └──────┬──────┘                          ║  │  │
│  │  ║         │                  │                                  ║  │  │
│  │  ║  ┌──────▼──────┐    ┌──────▼──────┐                          ║  │  │
│  │  ║  │  ESPADINES  │    │    BNC      │                          ║  │  │
│  │  ║  │  NEXTION    │    │   Output    │                          ║  │  │
│  │  ║  │  5V,GND,    │    │   0-3.3V    │                          ║  │  │
│  │  ║  │  TX,RX      │    │             │                          ║  │  │
│  │  ║  └─────────────┘    └─────────────┘                          ║  │  │
│  │  ║                                                               ║  │  │
│  │  ╚═══════════════════════════════════════════════════════════════╝  │  │
│  └──────────────────────────────────────────────────────────────────────┘  │
│                     │                                                       │
│                     │ Cable 4 hilos                                         │
│                     ▼                                                       │
│            ┌─────────────────┐                                              │
│            │    NEXTION      │                                              │
│            │   NX4024T032    │                                              │
│            │   3.2" (5V)     │                                              │
│            └─────────────────┘                                              │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 Especificaciones Generales

| Parámetro | Valor |
|-----------|-------|
| Voltaje de batería | 7.4V nominal (6.0V - 8.4V) |
| Configuración baterías | 2S (serie) |
| Capacidad total | 2800 mAh |
| Voltaje de operación | 5V (regulado) |
| Consumo típico | ~530 mA @ 5V |
| Consumo pico | ~650 mA @ 5V |
| Autonomía típica | **5.0 horas** |
| Autonomía mínima | **4.2 horas** |

---

## 2. Sistema de Alimentación

### 2.1 Baterías

**Modelo:** Steren BAT-LI-18650/2800  
**Configuración:** 2S (Serie) para aumentar voltaje

| Parámetro | Por Celda | Pack 2S |
|-----------|-----------|---------|
| Voltaje nominal | 3.7V | 7.4V |
| Voltaje máximo | 4.2V | 8.4V |
| Voltaje mínimo | 3.0V | 6.0V |
| Capacidad | 2800 mAh | 2800 mAh |
| Energía | 10.36 Wh | 20.72 Wh |

```
┌─────────────────────────────────────────────────────────────┐
│                    PACK DE BATERÍAS 2S                      │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│    ┌─────────────┐         ┌─────────────┐                 │
│    │  18650      │         │  18650      │                 │
│    │  2800mAh    │         │  2800mAh    │                 │
│    │  CELDA 1    │         │  CELDA 2    │                 │
│    │             │         │             │                 │
│    │   (+)       │         │   (+)       │                 │
│    └──────┬──────┘         └──────┬──────┘                 │
│           │                       │                         │
│    ┌──────┴──────┐         ┌──────┴──────┐                 │
│    │   (-)       │─────────│   (+)       │                 │
│    └─────────────┘  SERIE  └─────────────┘                 │
│           │                       │                         │
│      B- (GND)                B+ (7.4V)                     │
│           │                       │                         │
│           └───────────┬───────────┘                         │
│                       ▼                                     │
│                   A BMS 2S                                  │
└─────────────────────────────────────────────────────────────┘

Voltajes del Pack:
- Completamente cargado: 8.4V (4.2V + 4.2V)
- Nominal: 7.4V (3.7V + 3.7V)
- Descargado: 6.0V (3.0V + 3.0V)
```

### 2.2 BMS (Battery Management System)

**Modelo:** HX-2S-D01 (o equivalente 2S 8A)

| Parámetro | Valor |
|-----------|-------|
| Configuración | 2S (7.4V) |
| Corriente continua | 8A |
| Corriente pico | 15A |
| Protección sobrecarga | 8.4V ±0.05V |
| Protección sobredescarga | 5.6V ±0.1V |
| Protección cortocircuito | Sí |
| Balanceo | Sí (pasivo) |

### 2.3 Regulador Buck (DC-DC Step-Down)

**Modelo:** XL4015 (o LM2596)

| Parámetro | Valor |
|-----------|-------|
| Voltaje entrada | 6V - 40V |
| Voltaje salida | **5V** (ajustable) |
| Corriente máxima | 5A |
| Eficiencia | ~90% @ 1A |

> **IMPORTANTE:** Ajustar el potenciómetro a exactamente 5.0V ANTES de conectar cualquier carga.

### 2.4 Flujo de Alimentación

```
┌─────────────┐    ┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│  BATERÍAS   │───►│    BMS      │───►│   SWITCH    │───►│   BUCK      │
│  2S 7.4V    │    │  HX-2S-D01  │    │   ON/OFF    │    │  XL4015     │
│  2800mAh    │    │             │    │             │    │  7.4V→5V    │
└─────────────┘    └─────────────┘    └─────────────┘    └──────┬──────┘
                                                                │
                                                                │ 5V
                                                                ▼
                                                    ┌───────────────────┐
                                                    │   PCB PRINCIPAL   │
                                                    │   (Bornera 5V)    │
                                                    └─────────┬─────────┘
                                                              │
                              ┌────────────────┬──────────────┼──────────────┐
                              │                │              │              │
                              ▼                ▼              ▼              ▼
                        ┌──────────┐    ┌──────────┐   ┌──────────┐   ┌──────────┐
                        │  ESP32   │    │  MCP6002 │   │ LED RGB  │   │ NEXTION  │
                        │  ~80mA   │    │  ~0.1mA  │   │  ~20mA   │   │ ~400mA   │
                        └──────────┘    └──────────┘   └──────────┘   └──────────┘
                                                                      (via espadines)
```

---

## 3. PCB Principal

### 3.1 Componentes en PCB

```
╔═══════════════════════════════════════════════════════════════════════════════╗
║                              PCB PRINCIPAL                                    ║
║                                                                               ║
║   ┌─────────────────────────────────────────────────────────────────────┐    ║
║   │                                                                     │    ║
║   │    ┌───────────────┐                      ┌───────────────┐        │    ║
║   │    │   BORNERA     │                      │    LED RGB    │        │    ║
║   │    │   ENTRADA     │                      │  (Catodo      │        │    ║
║   │    │   5V   GND    │                      │   comun)      │        │    ║
║   │    │   ●     ●     │                      │    [R G B]    │        │    ║
║   │    └───────────────┘                      └───────────────┘        │    ║
║   │           │                                      │                  │    ║
║   │           │                                      │ 330Ω x3          │    ║
║   │           ▼                                      │                  │    ║
║   │    ┌─────────────────────────────────────────────┼──────────┐      │    ║
║   │    │                                             │          │      │    ║
║   │    │              ESP32 NodeMCU                  │          │      │    ║
║   │    │                                             │          │      │    ║
║   │    │    VIN ●────────────────────────────────────┘          │      │    ║
║   │    │    GND ●                                               │      │    ║
║   │    │                                                        │      │    ║
║   │    │    GPIO25 (DAC) ●──────────────────┐                   │      │    ║
║   │    │    GPIO17 (TX)  ●──────────────────┼───┐               │      │    ║
║   │    │    GPIO16 (RX)  ●──────────────────┼───┼───┐           │      │    ║
║   │    │    GPIO4  (R)   ●──────────────────┼───┼───┼───► LED   │      │    ║
║   │    │    GPIO5  (G)   ●──────────────────┼───┼───┼───► LED   │      │    ║
║   │    │    GPIO18 (B)   ●──────────────────┼───┼───┼───► LED   │      │    ║
║   │    │                                    │   │   │           │      │    ║
║   │    └────────────────────────────────────┼───┼───┼───────────┘      │    ║
║   │                                         │   │   │                   │    ║
║   │    ┌───────────────┐              ┌─────▼───┼───┼─────┐            │    ║
║   │    │   MCP6002     │              │  ESPADINES       │            │    ║
║   │    │   Buffer      │              │  NEXTION         │            │    ║
║   │    │               │              │                  │            │    ║
║   │    │  IN ●─────────┘              │  5V  ● ──────────┼── (Rojo)   │    ║
║   │    │  OUT ●────────────┐          │  GND ● ──────────┼── (Negro)  │    ║
║   │    │               │   │          │  TX  ● ──────────┼── (Azul)   │    ║
║   │    └───────────────┘   │          │  RX  ● ──────────┼── (Amarillo)    ║
║   │                        │          └──────────────────┘            │    ║
║   │    ┌───────────────┐   │                                          │    ║
║   │    │     BNC       │◄──┘                                          │    ║
║   │    │    Output     │                                              │    ║
║   │    │   0-3.3V      │                                              │    ║
║   │    └───────────────┘                                              │    ║
║   │                                                                     │    ║
║   └─────────────────────────────────────────────────────────────────────┘    ║
║                                                                               ║
╚═══════════════════════════════════════════════════════════════════════════════╝
```

### 3.2 Elementos en PCB (Resumen)

| # | Componente | Descripción |
|---|------------|-------------|
| 1 | **Bornera entrada** | 2 posiciones: 5V, GND (entrada desde Buck) |
| 2 | **ESP32 NodeMCU** | Microcontrolador principal |
| 3 | **MCP6002** | Buffer amplificador operacional |
| 4 | **LED RGB** | Indicador de estado (cátodo común) |
| 5 | **3x Resistencias 330Ω** | Limitadoras de corriente LED |
| 6 | **Espadines Nextion** | 4 pines: 5V, GND, TX, RX |
| 7 | **Conector BNC** | Salida analógica 0-3.3V |

### 3.3 Conexión Nextion (Cable 4 hilos)

```
PCB (Espadines)              Cable               Nextion
┌──────────────┐         ┌──────────┐        ┌──────────────┐
│  5V   (pin 1)│─────────│  Rojo    │────────│  5V          │
│  GND  (pin 2)│─────────│  Negro   │────────│  GND         │
│  TX   (pin 3)│─────────│  Azul    │────────│  RX          │
│  RX   (pin 4)│─────────│  Amarillo│────────│  TX          │
└──────────────┘         └──────────┘        └──────────────┘

NOTA: TX y RX están cruzados (TX del ESP32 → RX del Nextion)
```

---

## 4. Cálculos de Consumo y Autonomía

### 4.1 Consumo por Componente

| Componente | Voltaje | Corriente Típica | Corriente Pico |
|------------|---------|------------------|----------------|
| ESP32 NodeMCU | 5V | 80 mA | 150 mA |
| Nextion NX4024T032 | 5V | 400 mA | 450 mA |
| MCP6002 | 5V | 0.1 mA | 0.1 mA |
| LED RGB (1 color) | 5V | 10 mA | 10 mA |
| Otros (pérdidas) | - | 20 mA | 30 mA |
| **TOTAL @ 5V** | **5V** | **~510 mA** | **~640 mA** |

### 4.2 Consumo Referido a Batería

Con el regulador Buck XL4015 (eficiencia ~90%):

```
Corriente desde batería = (Consumo @ 5V × 5V) / (Vbat × η)

Corriente típica desde batería:
I_bat = (0.510 A × 5V) / (7.4V × 0.90)
I_bat = 2.55 W / 6.66 W
I_bat ≈ 383 mA

Corriente pico desde batería:
I_bat_pico = (0.640 A × 5V) / (7.4V × 0.90)
I_bat_pico ≈ 480 mA
```

### 4.3 Cálculo de Autonomía

```
Con baterías Steren BAT-LI-18650/2800 (2800 mAh):

Autonomía típica:
T_típico = 2800 mAh / 383 mA = 7.3 horas

Con factor de seguridad (80% capacidad útil):
T_real_típico = (2800 × 0.80) mAh / 383 mA = 5.8 horas
T_real_mínimo = (2800 × 0.80) mAh / 480 mA = 4.7 horas
```

### 4.4 Resumen de Autonomía

| Escenario | Consumo Batería | Autonomía Real (80%) |
|-----------|-----------------|----------------------|
| **Típico** | 383 mA | **5.8 h** |
| **Intensivo** | 480 mA | **4.7 h** |
| **Promedio** | 430 mA | **5.2 h** |

---

## 5. Circuito de Salida Analógica

### 5.1 Especificaciones

| Parámetro | Valor |
|-----------|-------|
| Conector | BNC hembra |
| Rango de voltaje | 0V - 3.3V |
| Impedancia de salida | ~100Ω |
| Resolución | 8 bits (256 niveles) |

### 5.2 Circuito Buffer MCP6002

```
                         VCC (5V)
                            │
                   ┌────────┴────────┐
                   │        8        │
                   │      VDD        │
                   │                 │
ESP32              │     MCP6002     │
GPIO25 ────────────►│3+     1        │
(DAC1)             │     \_____/    │────┬────► BNC (SEÑAL)
             ┌─────│2-              │    │
             │     │        4       │    R1
             │     │      VSS       │    100Ω
             │     └────────┬───────┘    │
             │              │            │
             └──────────────┴────────────┤
                            │            │
                           GND          GND ──► BNC (GND)
```

---

## 6. Conexiones UART

### 6.1 ESP32 ↔ Nextion

| ESP32 Pin | Nextion | Color Cable |
|-----------|---------|-------------|
| GPIO17 (TX2) | RX | Azul |
| GPIO16 (RX2) | TX | Amarillo |
| 5V (desde bornera) | 5V | Rojo |
| GND | GND | Negro |

**Configuración UART:** 115200 baud, 8N1

---

## 7. Indicador LED RGB

### 7.1 Circuito (Cátodo Común)

```
                    5V
                     │
      ┌──────────────┼──────────────┐
      │              │              │
     ┌┴┐            ┌┴┐            ┌┴┐
     │ │ 330Ω       │ │ 330Ω       │ │ 330Ω
     └┬┘            └┬┘            └┬┘
      │              │              │
      ▼              ▼              ▼
    ┌───┐          ┌───┐          ┌───┐
    │ R │          │ G │          │ B │
    └─┬─┘          └─┬─┘          └─┬─┘
      │              │              │
      └──────────────┴──────────────┘
                     │
                    GND (Cátodo común)
                     │
      ┌──────────────┼──────────────┐
      │              │              │
    GPIO4          GPIO5         GPIO18
    (ESP32)        (ESP32)       (ESP32)

Nota: LED activo cuando GPIO = HIGH
Corriente por LED: (5V - 2V - 0V) / 330Ω ≈ 9 mA
```

### 7.2 Estados del LED (Solo 3 principales)

| Estado | Color | GPIO4 | GPIO5 | GPIO18 | Significado |
|--------|-------|-------|-------|--------|-------------|
| **IDLE** | Verde | LOW | HIGH | LOW | Sistema listo, esperando |
| **RUNNING** | Azul | LOW | LOW | HIGH | Generando señal |
| **ERROR** | Rojo | HIGH | LOW | LOW | Error en el sistema |

---

## 8. Esquema General

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                              BIOSIMULATOR PRO v1.1                                  │
│                            ESQUEMA GENERAL DE CONEXIONES                            │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                     │
│   ┌─────────────┐    ┌─────────────┐    ┌─────────────┐    ┌─────────────┐         │
│   │  BATERÍAS   │───►│    BMS      │───►│   SWITCH    │───►│    BUCK     │         │
│   │  2S 7.4V    │    │  HX-2S-D01  │    │   ON/OFF    │    │   XL4015    │         │
│   │  2800mAh    │    │             │    │             │    │  7.4V→5V    │         │
│   └─────────────┘    └─────────────┘    └─────────────┘    └──────┬──────┘         │
│                                                                   │ 5V              │
│   ╔═══════════════════════════════════════════════════════════════▼═══════════════╗│
│   ║                              PCB PRINCIPAL                                    ║│
│   ║                                                                               ║│
│   ║   ┌─────────────┐    ┌─────────────┐    ┌─────────────┐    ┌─────────────┐   ║│
│   ║   │  BORNERA    │───►│   ESP32     │───►│   MCP6002   │───►│    BNC      │   ║│
│   ║   │  5V / GND   │    │   NodeMCU   │    │   Buffer    │    │   Output    │   ║│
│   ║   └──────┬──────┘    └──────┬──────┘    └─────────────┘    └─────────────┘   ║│
│   ║          │                  │                                                 ║│
│   ║          │           ┌──────┴──────┐                                          ║│
│   ║          │           │  LED RGB    │                                          ║│
│   ║          │           │  (3 estados)│                                          ║│
│   ║          │           └─────────────┘                                          ║│
│   ║          │                                                                    ║│
│   ║   ┌──────▼──────┐                                                             ║│
│   ║   │  ESPADINES  │                                                             ║│
│   ║   │  NEXTION    │                                                             ║│
│   ║   │ 5V,GND,TX,RX│                                                             ║│
│   ║   └──────┬──────┘                                                             ║│
│   ║          │                                                                    ║│
│   ╚══════════▼════════════════════════════════════════════════════════════════════╝│
│              │                                                                      │
│              │ Cable 4 hilos                                                        │
│              ▼                                                                      │
│   ┌─────────────────┐                                                               │
│   │    NEXTION      │                                                               │
│   │   NX4024T032    │                                                               │
│   │   3.2" 400mA    │                                                               │
│   │   (5V input)    │                                                               │
│   └─────────────────┘                                                               │
│                                                                                     │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 9. Lista de Materiales (BOM)

### 9.1 Componentes

| # | Componente | Modelo | Cantidad | Precio Unit. | Subtotal |
|---|------------|--------|----------|--------------|----------|
| 1 | MCU | ESP32 NodeMCU v1.1 | 1 | $8.00 | $8.00 |
| 2 | Display | Nextion NX4024T032 3.2" | 1 | $25.00 | $25.00 |
| 3 | Batería Li-ion | Steren BAT-LI-18650/2800 | 2 | $6.00 | $12.00 |
| 4 | Portapilas | Holder 18650 doble (serie) | 1 | $2.00 | $2.00 |
| 5 | BMS 2S | HX-2S-D01 8A | 1 | $2.50 | $2.50 |
| 6 | Cargador 2S | Módulo carga 8.4V 2A | 1 | $4.00 | $4.00 |
| 7 | Regulador Buck | XL4015 5A | 1 | $3.00 | $3.00 |
| 8 | Op-Amp | MCP6002 DIP-8 | 1 | $1.00 | $1.00 |
| 9 | Conector BNC | BNC hembra panel | 1 | $1.50 | $1.50 |
| 10 | LED RGB | 5mm cátodo común | 1 | $0.30 | $0.30 |
| 11 | Resistencias | 330Ω 1/4W | 3 | $0.10 | $0.30 |
| 12 | Resistencia | 100Ω 1/4W | 1 | $0.10 | $0.10 |
| 13 | Switch | Interruptor ON/OFF | 1 | $1.00 | $1.00 |
| 14 | Bornera | Terminal block 2 pos | 1 | $0.50 | $0.50 |
| 15 | Espadines | Header macho 4 pines | 1 | $0.20 | $0.20 |
| 16 | PCB | Placa perforada o custom | 1 | $3.00 | $3.00 |
| 17 | Cables | Jumper wires | 1 | $2.00 | $2.00 |
| 18 | Caja | Enclosure plástico | 1 | $5.00 | $5.00 |

### 9.2 Resumen de Costos

| Categoría | Costo |
|-----------|-------|
| Componentes electrónicos | $71.40 |
| Envío estimado | $8.00 |
| **TOTAL ESTIMADO** | **~$80 USD** |

---

## 10. Notas de Ensamblaje

### 10.1 Orden de Conexión

1. Preparar pack de baterías 2S
2. Conectar BMS a las baterías
3. Instalar switch entre BMS y Buck
4. **Ajustar Buck a 5.0V antes de conectar cargas**
5. Conectar Buck a bornera de PCB
6. Montar ESP32, MCP6002, LED en PCB
7. Conectar Nextion via espadines

### 10.2 Verificaciones

- [ ] Voltaje del pack: 7.0V - 8.4V
- [ ] Voltaje del buck: 5.0V ±0.1V
- [ ] Polaridad correcta
- [ ] TX↔RX cruzados para Nextion

---

**Documento creado:** Diciembre 2024  
**Versión:** 1.1.0  
**Proyecto:** BioSimulator Pro
