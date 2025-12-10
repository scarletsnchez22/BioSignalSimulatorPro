# BioSimulator Pro - Diseño Electrónico de Hardware

**Versión 1.1.0 | Diciembre 2024**

---

## Índice

1. [Arquitectura General](#1-arquitectura-general)
2. [Sistema de Alimentación](#2-sistema-de-alimentación)
3. [Switch de 3 Posiciones](#3-switch-de-3-posiciones)
4. [Cálculos de Consumo y Autonomía](#4-cálculos-de-consumo-y-autonomía)
5. [PCB Principal](#5-pcb-principal)
6. [Circuito de Salida Analógica](#6-circuito-de-salida-analógica)
7. [Conexiones UART](#7-conexiones-uart)
8. [Indicador LED RGB](#8-indicador-led-rgb)
9. [Esquema General](#9-esquema-general)
10. [Lista de Materiales (BOM)](#10-lista-de-materiales-bom)
11. [Manual de Usuario - Modos de Operación](#11-manual-de-usuario---modos-de-operación)

---

## 1. Arquitectura General

### 1.1 Diagrama de Bloques

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                              BIOSIMULATOR PRO v1.1                                  │
│                      Arquitectura con Switch Manual 3 Posiciones                    │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                     │
│   ┌─────────────┐    ┌─────────────┐                                               │
│   │  USB 5V     │───►│   TP4056    │                                               │
│   │  (Carga)    │    │   Cargador  │                                               │
│   └─────────────┘    └──────┬──────┘                                               │
│                             │                                                       │
│                      ┌──────▼──────┐                                               │
│                      │  BATERÍAS   │                                               │
│                      │  2P 3.7V    │                                               │
│                      │  4400mAh    │                                               │
│                      └──────┬──────┘                                               │
│                             │                                                       │
│              ┌──────────────┼──────────────┐                                       │
│              │              │              │                                        │
│              │       ┌──────▼──────┐       │                                       │
│              │       │   SWITCH    │       │                                       │
│              │       │  3 POSIC.   │       │                                       │
│              │       │ [OFF|BAT|CHG]│       │                                       │
│              │       └──────┬──────┘       │                                       │
│              │              │              │                                        │
│   ┌──────────▼──┐    ┌──────▼──────┐       │                                       │
│   │  TP4056     │    │  MT3608     │       │                                       │
│   │  OUT+       │    │  Step-Up    │       │                                       │
│   │  (5V USB)   │    │  3.7V→5V    │       │                                       │
│   └──────┬──────┘    └──────┬──────┘       │                                       │
│          │                  │              │                                        │
│          └────────┬─────────┘              │                                       │
│                   │ 5V                     │                                        │
│   ╔═══════════════▼════════════════════════╪═══════════════════════════════════╗   │
│   ║                    PCB PRINCIPAL       │                                   ║   │
│   ║                                        │                                   ║   │
│   ║   ┌─────────────┐    ┌─────────────┐   │    ┌─────────────┐               ║   │
│   ║   │  BORNERA    │───►│   ESP32     │   │    │  LED RGB    │               ║   │
│   ║   │  5V / GND   │    │   NodeMCU   │   │    │ (3 estados) │               ║   │
│   ║   └──────┬──────┘    │   + WiFi    │   │    └─────────────┘               ║   │
│   ║          │           └──────┬──────┘   │                                   ║   │
│   ║          │                  │          │                                   ║   │
│   ║          │    ┌─────────────┼──────────┘                                   ║   │
│   ║          │    │             │                                              ║   │
│   ║          │    │      ┌──────▼──────┐    ┌─────────────┐                   ║   │
│   ║          │    │      │  DIVISOR    │    │   MCP6002   │───► BNC           ║   │
│   ║          │    │      │  1k + 2k    │    │   Buffer    │   (0-3.3V)        ║   │
│   ║          │    │      └─────────────┘    └─────────────┘                   ║   │
│   ║          │    │                                                            ║   │
│   ║   ┌──────▼────▼─┐                                                          ║   │
│   ║   │  ESPADINES  │                                                          ║   │
│   ║   │  NEXTION    │                                                          ║   │
│   ║   │ 5V,GND,TX,RX│                                                          ║   │
│   ║   └──────┬──────┘                                                          ║   │
│   ╚══════════▼═════════════════════════════════════════════════════════════════╝   │
│              │                                                                      │
│              │ Cable 4 hilos                                                        │
│              ▼                                                                      │
│   ┌─────────────────┐                                                               │
│   │    NEXTION      │                                                               │
│   │   NX8048T070    │                                                               │
│   │   7" (5V)       │                                                               │
│   └─────────────────┘                                                               │
│                                                                                     │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 Especificaciones Generales

| Parámetro | Valor |
|-----------|-------|
| Voltaje de batería | 3.7V nominal (3.0V - 4.2V) |
| Configuración baterías | **2P (paralelo)** |
| Capacidad total | **4400 mAh** |
| Energía total | 16.28 Wh |
| Voltaje de operación | 5V (regulado) |
| Consumo típico | ~680 mA @ 5V |
| Consumo pico | ~980 mA @ 5V |
| **Autonomía típica** | **3.4 horas** |
| **Autonomía mínima** | **2.4 horas** |

---

## 2. Sistema de Alimentación

### 2.1 Baterías

**Modelo:** 18650 Li-ion 2200mAh (genérico o Samsung/LG)  
**Configuración:** 2P (Paralelo) para aumentar capacidad

| Parámetro | Por Celda | Pack 2P |
|-----------|-----------|---------|
| Voltaje nominal | 3.7V | **3.7V** |
| Voltaje máximo | 4.2V | 4.2V |
| Voltaje mínimo | 3.0V | 3.0V |
| Capacidad | 2200 mAh | **4400 mAh** |
| Energía | 8.14 Wh | **16.28 Wh** |

```
┌─────────────────────────────────────────────────────────────┐
│                    PACK DE BATERÍAS 2P                      │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│    ┌─────────────┐         ┌─────────────┐                 │
│    │  18650      │         │  18650      │                 │
│    │  2200mAh    │         │  2200mAh    │                 │
│    │  CELDA 1    │         │  CELDA 2    │                 │
│    │             │         │             │                 │
│    │   (+)───────┼─────────┼───(+)       │                 │
│    └──────┬──────┘         └──────┬──────┘                 │
│           │      PARALELO         │                         │
│    ┌──────┴──────┐         ┌──────┴──────┐                 │
│    │   (-)───────┼─────────┼───(-)       │                 │
│    └─────────────┘         └─────────────┘                 │
│           │                       │                         │
│           │                       │                         │
│      B+ (3.7V) ◄─────────────────►│                        │
│           │                       │                         │
│      B- (GND) ◄──────────────────►│                        │
│                                                             │
└─────────────────────────────────────────────────────────────┘

Voltajes del Pack:
- Completamente cargado: 4.2V
- Nominal: 3.7V
- Descargado: 3.0V
- Capacidad: 4400 mAh (2200 + 2200)
```

### 2.2 Módulo Cargador TP4056

**Modelo:** TP4056 con protección (DW01 + 8205A)

| Parámetro | Valor |
|-----------|-------|
| Voltaje entrada | 5V (USB) |
| Corriente de carga | 1A (ajustable) |
| Voltaje de corte | 4.2V ±1% |
| Protección sobredescarga | 2.5V |
| Protección sobrecorriente | 3A |
| Protección cortocircuito | Sí |

### 2.3 Regulador Step-Up MT3608

| Parámetro | Valor |
|-----------|-------|
| Voltaje entrada | 2V - 24V |
| Voltaje salida | **5V** (ajustable) |
| Corriente máxima | 2A |
| Eficiencia típica | **93%** @ 3.7V→5V, 0.5A |
| Eficiencia @ 1A | **88%** |

> **IMPORTANTE:** Ajustar el potenciómetro a exactamente 5.0V ANTES de conectar cargas.

---

## 3. Switch de 3 Posiciones

### 3.1 Tipo de Switch

**Switch SPDT ON-OFF-ON** (Single Pole Double Throw)

```
         ┌─────────────────────────────────────────┐
         │          SWITCH 3 POSICIONES            │
         │                                         │
         │    Posición 1      Posición 2      Posición 3
         │    [CHARGE]         [OFF]          [BATTERY]
         │                                         │
         │       ●               ●               ●  │
         │       │               │               │  │
         │       └───────────────┼───────────────┘  │
         │                       │                  │
         │                    COMÚN                 │
         └─────────────────────────────────────────┘
```

### 3.2 Conexiones del Switch

| Terminal | Conexión |
|----------|----------|
| **Común (C)** | Entrada del MT3608 (IN+) |
| **Posición 1 (CHARGE)** | Salida TP4056 (OUT+) → 5V USB directo |
| **Posición 2 (OFF)** | Sin conexión (sistema apagado) |
| **Posición 3 (BATTERY)** | Batería (B+) → 3.7V |

### 3.3 Diagrama del Circuito con Switch

```
                                    SWITCH 3 POSICIONES
                                    ┌─────────────────┐
                                    │                 │
USB 5V ──►┌─────────┐               │  1    2    3   │
          │ TP4056  │               │  ●    ●    ●   │
          │         │               │  │         │   │
          │ OUT+ ───┼───────────────┼──┘         │   │
          │         │               │            │   │
          │ B+  ────┼───┬───────────┼────────────┘   │
          │         │   │           │                │
          │ B-  ────┼───┼───┐       │     ●──────────┼──► MT3608 IN+
          │         │   │   │       │     │          │
          └─────────┘   │   │       │   COMÚN        │
                        │   │       └─────────────────┘
                        │   │
                   ┌────▼───▼────┐
                   │  BATERÍAS   │
                   │  2P 3.7V    │
                   │  4400mAh    │
                   │             │
                   │  (+)   (-) │
                   └──┬──────┬──┘
                      │      │
                      │      └──────────────────────► GND común
                      │
                      └─────────────────────────────► A switch pos. 3
```

---

## 4. Cálculos de Consumo y Autonomía

### 4.1 Consumo por Componente @ 5V

| Componente | Corriente Típica | Corriente Pico | Notas |
|------------|------------------|----------------|-------|
| ESP32 NodeMCU (WiFi activo) | 150 mA | 350 mA | Picos durante TX WiFi |
| Nextion NX8048T070 (7") | 510 mA | 600 mA | Backlight 100% |
| MCP6002 Buffer | 0.1 mA | 0.1 mA | Negligible |
| LED RGB (1 color activo) | 10 mA | 10 mA | |
| Divisor voltaje (1k+2k) | 1.7 mA | 1.7 mA | 5V / 3kΩ |
| Pérdidas PCB/cables | 10 mA | 20 mA | Estimado |
| **TOTAL @ 5V** | **~682 mA** | **~982 mA** | |

### 4.2 Eficiencia del Sistema

#### Modo BATTERY (3.7V → 5V via MT3608)

| Parámetro | Valor | Cálculo |
|-----------|-------|---------|
| Eficiencia MT3608 | 90% | Promedio @ 0.7A |
| Pérdidas cables/conectores | 98% | ~2% pérdida |
| **Eficiencia Total** | **88%** | 0.90 × 0.98 |

#### Modo CHARGE (5V USB directo)

| Parámetro | Valor | Cálculo |
|-----------|-------|---------|
| Eficiencia (paso directo) | 98% | Solo pérdidas en cables |
| **Eficiencia Total** | **98%** | Casi directo |

### 4.3 Consumo Referido a Batería (Modo BATTERY)

```
Potencia consumida @ 5V:
P_5V_típica = 0.682 A × 5V = 3.41 W
P_5V_pico = 0.982 A × 5V = 4.91 W

Potencia desde batería (con eficiencia 88%):
P_bat_típica = 3.41 W / 0.88 = 3.87 W
P_bat_pico = 4.91 W / 0.88 = 5.58 W

Corriente desde batería @ 3.7V nominal:
I_bat_típica = 3.87 W / 3.7V = 1.05 A
I_bat_pico = 5.58 W / 3.7V = 1.51 A
```

### 4.4 Cálculo de Autonomía

```
Capacidad de baterías: 4400 mAh

Factor de confianza: 90% (capacidad útil real)
Capacidad útil = 4400 mAh × 0.90 = 3960 mAh

Autonomía típica (WiFi moderado):
T_típica = 3960 mAh / 1050 mA = 3.77 horas ≈ 3.8 h

Autonomía mínima (WiFi intensivo):
T_mínima = 3960 mAh / 1510 mA = 2.62 horas ≈ 2.6 h

Autonomía promedio:
T_promedio = 3960 mAh / 1200 mA = 3.3 horas
```

### 4.5 Resumen de Autonomía

| Escenario | Consumo @ 5V | Consumo Batería | Autonomía (90%) |
|-----------|--------------|-----------------|-----------------|
| **Típico** | 682 mA | 1.05 A | **3.8 h** |
| **Intensivo** | 982 mA | 1.51 A | **2.6 h** |
| **Promedio** | 780 mA | 1.20 A | **3.3 h** |

### 4.6 Tiempo de Carga

```
Capacidad: 4400 mAh
Corriente de carga TP4056: 1000 mA

Tiempo de carga (0% → 100%):
T_carga = 4400 mAh / 1000 mA × 1.2 (factor CC/CV)
T_carga ≈ 5.3 horas

Nota: El factor 1.2 considera la fase de voltaje constante (CV)
donde la corriente disminuye gradualmente.
```

---

## 5. PCB Principal

### 5.1 Componentes en PCB

```
╔═══════════════════════════════════════════════════════════════════════════════╗
║                              PCB PRINCIPAL                                    ║
║                                                                               ║
║   ┌─────────────────────────────────────────────────────────────────────┐    ║
║   │                                                                     │    ║
║   │    ┌───────────────┐                      ┌───────────────┐        │    ║
║   │    │   BORNERA     │                      │    LED RGB    │        │    ║
║   │    │   ENTRADA     │                      │  (Cátodo      │        │    ║
║   │    │   5V   GND    │                      │   común)      │        │    ║
║   │    │   ●     ●     │                      │    [R G B]    │        │    ║
║   │    └───────────────┘                      └───────────────┘        │    ║
║   │           │                                      │                  │    ║
║   │           │                                      │ 330Ω x3          │    ║
║   │           ▼                                      │                  │    ║
║   │    ┌─────────────────────────────────────────────┼──────────┐      │    ║
║   │    │                                             │          │      │    ║
║   │    │              ESP32 NodeMCU                  │          │      │    ║
║   │    │              (+ WiFi)                       │          │      │    ║
║   │    │                                             │          │      │    ║
║   │    │    VIN ●────────────────────────────────────┘          │      │    ║
║   │    │    GND ●                                               │      │    ║
║   │    │                                                        │      │    ║
║   │    │    GPIO25 (DAC) ●──────────────────┐                   │      │    ║
║   │    │    GPIO17 (TX)  ●──────────────────┼───────────────────┼──►   │    ║
║   │    │    GPIO16 (RX)  ●◄─────────────────┼───┐               │      │    ║
║   │    │    GPIO4  (R)   ●──────────────────┼───┼───────────────┼──►LED│    ║
║   │    │    GPIO5  (G)   ●──────────────────┼───┼───────────────┼──►LED│    ║
║   │    │    GPIO18 (B)   ●──────────────────┼───┼───────────────┼──►LED│    ║
║   │    │                                    │   │               │      │    ║
║   │    └────────────────────────────────────┼───┼───────────────┘      │    ║
║   │                                         │   │                       │    ║
║   │    ┌───────────────┐              ┌─────┼───▼───────────┐          │    ║
║   │    │   MCP6002     │              │  DIVISOR VOLTAJE   │          │    ║
║   │    │   Buffer      │              │                    │          │    ║
║   │    │               │              │  Nextion TX (5V)   │          │    ║
║   │    │  IN ●─────────┘              │       │            │          │    ║
║   │    │  OUT ●────────────┐          │      ┌┴┐ 1kΩ       │          │    ║
║   │    │               │   │          │      └┬┘           │          │    ║
║   │    └───────────────┘   │          │       ├──► ESP32 RX│          │    ║
║   │                        │          │      ┌┴┐ 2kΩ       │          │    ║
║   │    ┌───────────────┐   │          │      └┬┘           │          │    ║
║   │    │     BNC       │◄──┘          │       │            │          │    ║
║   │    │    Output     │              │      GND           │          │    ║
║   │    │   0-3.3V      │              └────────────────────┘          │    ║
║   │    └───────────────┘                                              │    ║
║   │                                                                     │    ║
║   │    ┌───────────────────────────────────────────────────────────┐   │    ║
║   │    │                    ESPADINES NEXTION                      │   │    ║
║   │    │    5V ●    GND ●    TX(→RX) ●    RX(←divisor) ●          │   │    ║
║   │    └───────────────────────────────────────────────────────────┘   │    ║
║   │                                                                     │    ║
║   └─────────────────────────────────────────────────────────────────────┘    ║
║                                                                               ║
╚═══════════════════════════════════════════════════════════════════════════════╝
```

### 5.2 Elementos en PCB (Resumen)

| # | Componente | Descripción |
|---|------------|-------------|
| 1 | **Bornera entrada** | 2 posiciones: 5V, GND (desde MT3608) |
| 2 | **ESP32 NodeMCU** | Microcontrolador + WiFi |
| 3 | **MCP6002** | Buffer amplificador operacional |
| 4 | **LED RGB** | Indicador de estado (cátodo común) |
| 5 | **3× Resistencias 330Ω** | Limitadoras de corriente LED |
| 6 | **Divisor voltaje** | 1kΩ + 2kΩ (protección RX) |
| 7 | **Espadines Nextion** | 4 pines: 5V, GND, TX, RX |
| 8 | **Conector BNC** | Salida analógica 0-3.3V |

---

## 6. Circuito de Salida Analógica

### 6.1 Especificaciones

| Parámetro | Valor |
|-----------|-------|
| Conector | BNC hembra |
| Rango de voltaje | 0V - 3.3V |
| Impedancia de salida | ~100Ω |
| Resolución | 8 bits (256 niveles) |

### 6.2 Circuito Buffer MCP6002

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

## 7. Conexiones UART

### 7.1 ESP32 ↔ Nextion

| ESP32 Pin | Nextion | Notas |
|-----------|---------|-------|
| GPIO17 (TX2) | RX | Directo (3.3V → 5V tolerante) |
| GPIO16 (RX2) | TX | **Via divisor** (5V → 3.3V) |
| 5V | 5V | Desde bornera PCB |
| GND | GND | Común |

**Configuración UART:** 115200 baud, 8N1

### 7.2 Divisor de Voltaje (Protección RX)

```
Nextion TX (5V) ────┬──── 
                   ┌┴┐
                   │ │ R1 = 1kΩ
                   └┬┘
                    ├────────► ESP32 RX (GPIO16)
                   ┌┴┐         V_out = 3.33V ✓
                   │ │ R2 = 2kΩ
                   └┬┘
                    │
                   GND

Cálculo: V_out = 5V × 2k/(1k+2k) = 3.33V
Corriente: I = 5V / 3kΩ = 1.67 mA
```

---

## 8. Indicador LED RGB

### 8.1 Circuito (Cátodo Común)

```
                 GPIO4        GPIO5        GPIO18
                 (ESP32)      (ESP32)      (ESP32)
                    │            │            │
                   ┌┴┐          ┌┴┐          ┌┴┐
                   │ │ 330Ω     │ │ 330Ω     │ │ 330Ω
                   └┬┘          └┬┘          └┬┘
                    │            │            │
                    ▼            ▼            ▼
                  ┌───┐        ┌───┐        ┌───┐
                  │ R │        │ G │        │ B │
                  └─┬─┘        └─┬─┘        └─┬─┘
                    │            │            │
                    └────────────┴────────────┘
                                 │
                                GND (Cátodo común)

LED activo cuando GPIO = HIGH
Corriente por LED: (3.3V - 2V) / 330Ω ≈ 4 mA
```

### 8.2 Estados del LED (3 principales)

| Estado | Color | R | G | B | Significado |
|--------|-------|---|---|---|-------------|
| **IDLE** | Verde | 0 | 1 | 0 | Sistema listo |
| **RUNNING** | Azul | 0 | 0 | 1 | Generando señal |
| **ERROR** | Rojo | 1 | 0 | 0 | Error en sistema |

---

## 9. Esquema General

### 9.1 Diagrama Completo

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                              BIOSIMULATOR PRO v1.1                                  │
│                            ESQUEMA GENERAL DE CONEXIONES                            │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                     │
│                                                                                     │
│   ┌─────────────┐    ┌─────────────┐    ┌─────────────┐    ┌─────────────┐         │
│   │  USB 5V     │───►│   TP4056    │───►│  BATERÍAS   │    │   SWITCH    │         │
│   │  (Carga)    │    │   Módulo    │    │  2P 3.7V    │    │ 3 POSICIONES│         │
│   └─────────────┘    │             │    │  4400mAh    │    │             │         │
│                      │ OUT+ ───────┼────┼─────────────┼───►│ 1 (CHARGE)  │         │
│                      │             │    │             │    │             │         │
│                      │ B+   ───────┼────┤ (+)         ├───►│ 3 (BATTERY) │         │
│                      │ B-   ───────┼────┤ (-)         │    │             │         │
│                      └─────────────┘    └─────────────┘    │   COMÚN ────┼──┐      │
│                                                            │             │  │      │
│                                                            │ 2 (OFF)     │  │      │
│                                                            └─────────────┘  │      │
│                                                                             │      │
│                                                            ┌────────────────┘      │
│                                                            │                       │
│                                                     ┌──────▼──────┐                │
│                                                     │   MT3608    │                │
│                                                     │   Step-Up   │                │
│                                                     │  3.7V → 5V  │                │
│                                                     └──────┬──────┘                │
│                                                            │ 5V                    │
│   ╔════════════════════════════════════════════════════════▼════════════════════╗  │
│   ║                              PCB PRINCIPAL                                  ║  │
│   ║                                                                             ║  │
│   ║   ┌─────────────┐    ┌─────────────┐    ┌─────────────┐    ┌───────────┐   ║  │
│   ║   │  BORNERA    │───►│   ESP32     │───►│   MCP6002   │───►│   BNC     │   ║  │
│   ║   │  5V / GND   │    │   NodeMCU   │    │   Buffer    │    │  Output   │   ║  │
│   ║   └──────┬──────┘    │   + WiFi    │    └─────────────┘    └───────────┘   ║  │
│   ║          │           └──────┬──────┘                                        ║  │
│   ║          │                  │                                               ║  │
│   ║          │           ┌──────┴──────┐    ┌─────────────┐                    ║  │
│   ║          │           │  DIVISOR    │    │  LED RGB    │                    ║  │
│   ║          │           │  1k + 2k    │    │ (3 estados) │                    ║  │
│   ║          │           └─────────────┘    └─────────────┘                    ║  │
│   ║          │                                                                  ║  │
│   ║   ┌──────▼──────┐                                                           ║  │
│   ║   │  ESPADINES  │                                                           ║  │
│   ║   │  NEXTION    │                                                           ║  │
│   ║   │ 5V,GND,TX,RX│                                                           ║  │
│   ║   └──────┬──────┘                                                           ║  │
│   ╚══════════▼══════════════════════════════════════════════════════════════════╝  │
│              │                                                                      │
│              │ Cable 4 hilos                                                        │
│              ▼                                                                      │
│   ┌─────────────────┐                                                               │
│   │    NEXTION      │                                                               │
│   │   NX8048T070    │                                                               │
│   │   7" (5V)       │                                                               │
│   └─────────────────┘                                                               │
│                                                                                     │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

### 9.2 Modo CHARGE (USB Conectado)

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                         MODO CHARGE - USB CONECTADO                                 │
│                         Switch en posición 1 (CHARGE)                               │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                     │
│   USB 5V ──────►┌─────────┐                                                        │
│                 │ TP4056  │                                                        │
│                 │         │                                                        │
│                 │ OUT+ ───┼──────────────────────────────────────┐                 │
│                 │         │                                      │                 │
│                 │ B+  ────┼───►┌─────────┐                       │                 │
│                 │         │    │ BATERÍA │ ← Cargando            │                 │
│                 │ B-  ────┼───►│ 4400mAh │                       │                 │
│                 └─────────┘    └─────────┘                       │                 │
│                                                                  │                 │
│                                                           ┌──────▼──────┐          │
│                                                           │   SWITCH    │          │
│                                                           │ Pos 1: ON   │          │
│                                                           └──────┬──────┘          │
│                                                                  │ ~5V             │
│                                                           ┌──────▼──────┐          │
│                                                           │   MT3608    │          │
│                                                           │  (bypass)   │          │
│                                                           └──────┬──────┘          │
│                                                                  │ 5V              │
│                                                                  ▼                 │
│                                                           ┌─────────────┐          │
│                                                           │    PCB      │          │
│                                                           │  PRINCIPAL  │          │
│                                                           └─────────────┘          │
│                                                                                     │
│   ✓ Batería cargando (aislada del sistema)                                         │
│   ✓ Sistema alimentado por USB (5V directo)                                        │
│   ✓ Eficiencia: ~98%                                                               │
│                                                                                     │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

### 9.3 Modo BATTERY (Solo Batería)

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                         MODO BATTERY - PORTÁTIL                                     │
│                         Switch en posición 3 (BATTERY)                              │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                     │
│   USB ──────► (Desconectado)                                                       │
│                                                                                     │
│                 ┌─────────┐                                                        │
│                 │ TP4056  │                                                        │
│                 │         │                                                        │
│                 │ OUT+ ───┼──────────────────────────────────────┐                 │
│                 │         │                                      │ (sin conexión)  │
│                 │ B+  ────┼───►┌─────────┐                       │                 │
│                 │         │    │ BATERÍA │                       │                 │
│                 │ B-  ────┼───►│ 4400mAh │───────────────────────┼──┐              │
│                 └─────────┘    └─────────┘                       │  │              │
│                                     │                            │  │              │
│                                     │ 3.7V                       │  │              │
│                                     │                     ┌──────▼──┴───┐          │
│                                     │                     │   SWITCH    │          │
│                                     └────────────────────►│ Pos 3: ON   │          │
│                                                           └──────┬──────┘          │
│                                                                  │ 3.7V            │
│                                                           ┌──────▼──────┐          │
│                                                           │   MT3608    │          │
│                                                           │  3.7V → 5V  │          │
│                                                           └──────┬──────┘          │
│                                                                  │ 5V              │
│                                                                  ▼                 │
│                                                           ┌─────────────┐          │
│                                                           │    PCB      │          │
│                                                           │  PRINCIPAL  │          │
│                                                           └─────────────┘          │
│                                                                                     │
│   ✓ Sistema alimentado por batería                                                 │
│   ✓ Batería aislada del cargador                                                   │
│   ✓ Eficiencia: ~88%                                                               │
│   ✓ Autonomía: 2.6 - 3.8 horas                                                     │
│                                                                                     │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 10. Lista de Materiales (BOM)

### 10.1 Sistema de Alimentación

| # | Componente | Modelo | Cant. | Precio |
|---|------------|--------|-------|--------|
| 1 | Módulo cargador | TP4056 con protección | 1 | $1.50 |
| 2 | Baterías Li-ion | 18650 2200mAh | 2 | $10.00 |
| 3 | Portapilas | Holder 18650 doble (paralelo) | 1 | $2.00 |
| 4 | Step-Up | MT3608 módulo | 1 | $1.50 |
| 5 | Switch | SPDT ON-OFF-ON 3 posiciones | 1 | $1.50 |
| | **Subtotal** | | | **$16.50** |

### 10.2 PCB Principal

| # | Componente | Modelo | Cant. | Precio |
|---|------------|--------|-------|--------|
| 1 | MCU | ESP32 NodeMCU v1.1 | 1 | $8.00 |
| 2 | Op-Amp | MCP6002 DIP-8 | 1 | $1.00 |
| 3 | LED RGB | 5mm cátodo común | 1 | $0.30 |
| 4 | Resistencias | 330Ω 1/4W | 3 | $0.15 |
| 5 | Resistencia | 100Ω 1/4W (buffer) | 1 | $0.05 |
| 6 | Resistencias | 1kΩ, 2kΩ (divisor) | 2 | $0.10 |
| 7 | Bornera | Terminal 2 pos | 1 | $0.50 |
| 8 | Espadines | Header 4 pines | 1 | $0.20 |
| 9 | Conector BNC | BNC hembra panel | 1 | $1.50 |
| 10 | PCB | Placa perforada | 1 | $2.00 |
| | **Subtotal** | | | **$13.80** |

### 10.3 Display y Otros

| # | Componente | Modelo | Cant. | Precio |
|---|------------|--------|-------|--------|
| 1 | Display | Nextion NX8048T070 7" | 1 | $60.00 |
| 2 | Cables | Jumper wires | 1 | $2.00 |
| 3 | Caja | Enclosure | 1 | $8.00 |
| | **Subtotal** | | | **$70.00** |

### 10.4 Resumen de Costos

| Categoría | Costo |
|-----------|-------|
| Sistema de Alimentación | $16.50 |
| PCB Principal | $13.80 |
| Display y otros | $70.00 |
| Envío estimado | $10.00 |
| **TOTAL** | **~$110 USD** |

---

## 11. Manual de Usuario - Modos de Operación

### 11.1 Posiciones del Switch

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                           SWITCH DE 3 POSICIONES                                    │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                     │
│         Posición 1              Posición 2              Posición 3                 │
│         [CHARGE]                  [OFF]                 [BATTERY]                  │
│                                                                                     │
│            ◄──                     ───                     ──►                     │
│           /                         │                         \                    │
│          ●                          ●                          ●                   │
│                                                                                     │
│     ┌──────────────┐         ┌──────────────┐         ┌──────────────┐            │
│     │ USB + Carga  │         │   Apagado    │         │   Portátil   │            │
│     │              │         │              │         │              │            │
│     │ • Usar con   │         │ • Sistema    │         │ • Sin USB    │            │
│     │   USB        │         │   apagado    │         │              │            │
│     │   conectado  │         │              │         │ • Batería    │            │
│     │              │         │ • Batería    │         │   alimenta   │            │
│     │ • Batería    │         │   en reposo  │         │   sistema    │            │
│     │   cargando   │         │              │         │              │            │
│     │   (aislada)  │         │ • Transporte │         │ • Autonomía  │            │
│     │              │         │   seguro     │         │   ~3.3 horas │            │
│     └──────────────┘         └──────────────┘         └──────────────┘            │
│                                                                                     │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

### 11.2 Instrucciones de Uso

#### Para usar con USB conectado (escritorio/laboratorio):
1. Conectar cable USB al módulo TP4056
2. Colocar switch en **posición 1 (CHARGE)**
3. El sistema funciona con alimentación USB
4. La batería se carga simultáneamente (aislada del circuito)
5. LED del TP4056: Rojo = Cargando, Azul = Carga completa

#### Para usar con batería (modo portátil):
1. **Desconectar** el cable USB
2. Colocar switch en **posición 3 (BATTERY)**
3. El sistema funciona con la batería
4. Autonomía aproximada: 2.6 - 3.8 horas

#### Para apagar/transportar:
1. Colocar switch en **posición 2 (OFF)**
2. El sistema está completamente apagado
3. La batería no se descarga
4. Seguro para transporte

### 11.3 Advertencias Importantes

| ⚠️ ADVERTENCIA | Descripción |
|----------------|-------------|
| **NO usar posición BATTERY con USB** | Si el USB está conectado y el switch está en BATTERY, la batería podría dañarse por sobrecarga |
| **NO cargar con sistema encendido** | Siempre usar posición CHARGE cuando el USB esté conectado |
| **Verificar posición antes de conectar USB** | Asegurarse de que el switch esté en CHARGE antes de conectar el cable USB |

### 11.4 Indicadores de Estado

| Indicador | Estado | Significado |
|-----------|--------|-------------|
| LED TP4056 Rojo | Encendido | Batería cargando |
| LED TP4056 Azul | Encendido | Carga completa |
| LED RGB Verde | Encendido | Sistema listo (IDLE) |
| LED RGB Azul | Encendido | Generando señal |
| LED RGB Rojo | Encendido | Error en sistema |

---

## 12. Resumen de Especificaciones

| Parámetro | Valor |
|-----------|-------|
| **Baterías** | 2× 18650 2200mAh en paralelo (2P) |
| **Voltaje batería** | 3.7V nominal (3.0V - 4.2V) |
| **Capacidad** | 4400 mAh |
| **Regulación** | MT3608 Step-Up (3.7V → 5V) |
| **Selector de modo** | Switch SPDT 3 posiciones |
| **Consumo típico** | 682 mA @ 5V |
| **Consumo desde batería** | 1.05 A @ 3.7V |
| **Eficiencia (batería)** | 88% |
| **Autonomía típica** | **3.8 horas** |
| **Autonomía mínima** | **2.6 horas** |
| **Tiempo de carga** | ~5.3 horas |
| **Display** | Nextion NX8048T070 7" |
| **Costo total** | ~$110 USD |

---

**Documento creado:** Diciembre 2024  
**Versión:** 1.1.0  
**Proyecto:** BioSimulator Pro
