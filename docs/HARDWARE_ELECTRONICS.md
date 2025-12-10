# BioSimulator Pro - Diseño Electrónico de Hardware

**Versión 1.1.0 | Diciembre 2024**

---

## Índice

1. [Arquitectura General](#1-arquitectura-general)
2. [Sistema de Alimentación](#2-sistema-de-alimentación)
3. [Cálculos de Consumo y Autonomía](#3-cálculos-de-consumo-y-autonomía)
4. [Circuito de Salida Analógica](#4-circuito-de-salida-analógica)
5. [Conexiones UART](#5-conexiones-uart)
6. [Indicador LED RGB](#6-indicador-led-rgb)
7. [Esquema General](#7-esquema-general)
8. [Lista de Materiales (BOM)](#8-lista-de-materiales-bom)

---

## 1. Arquitectura General

### 1.1 Diagrama de Bloques

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           BIOSIMULATOR PRO v1.1                             │
│                         Arquitectura con Nextion                            │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐    ┌─────────────┐  │
│  │  2× 18650   │───►│  BMS 2S     │───►│  SWITCH     │───►│  BORNERA    │  │
│  │  2800mAh    │    │  HX-2S-D01  │    │  ON/OFF     │    │  7.4V       │  │
│  │  (SERIE)    │    │             │    │             │    │             │  │
│  └─────────────┘    └─────────────┘    └─────────────┘    └──────┬──────┘  │
│                                                                   │         │
│                     ┌─────────────────────────────────────────────┤         │
│                     │                                             │         │
│              ┌──────▼──────┐                              ┌───────▼───────┐ │
│              │  XL4015     │                              │   NEXTION     │ │
│              │  BUCK       │                              │   NX4024T032  │ │
│              │  7.4V→5V    │                              │   3.2" 400mA  │ │
│              └──────┬──────┘                              └───────┬───────┘ │
│                     │ 5V                                          │ UART    │
│              ┌──────▼──────┐                                      │         │
│              │  BORNERA    │◄─────────────────────────────────────┘         │
│              │  5V         │                                                │
│              └──────┬──────┘                                                │
│                     │                                                       │
│         ┌───────────┼───────────┐                                           │
│         │           │           │                                           │
│  ┌──────▼──────┐ ┌──▼───┐ ┌─────▼─────┐                                    │
│  │  ESP32      │ │MCP6002│ │  LED RGB  │                                    │
│  │  NodeMCU    │ │Buffer │ │  Estado   │                                    │
│  │  ~80mA      │ └──┬───┘ └───────────┘                                    │
│  └──────┬──────┘    │                                                       │
│         │           │                                                       │
│         │    ┌──────▼──────┐                                                │
│         │    │  BNC        │                                                │
│         │    │  Output     │                                                │
│         └────┤  0-3.3V     │                                                │
│              └─────────────┘                                                │
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
| Autonomía típica | **4.5 horas** |
| Autonomía mínima | **3.7 horas** |

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
│           │                       │                         │
│      B- (GND)                B+ (7.4V)                     │
│           │                       │                         │
│           └───────────┬───────────┘                         │
│                       │                                     │
│                       ▼                                     │
│                   A BMS 2S                                  │
│                                                             │
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

```
┌─────────────────────────────────────────────────────────────┐
│                      BMS HX-2S-D01                          │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│   ENTRADA (Baterías)              SALIDA (Carga/Descarga)   │
│                                                             │
│   B-  ●────────────────────────────────● P-                │
│                    │                                        │
│   BM  ●────────────┼───── Balanceo                         │
│                    │                                        │
│   B+  ●────────────┼───────────────────● P+                │
│                    │                                        │
│                    │                                        │
│              ┌─────┴─────┐                                  │
│              │  CIRCUITO │                                  │
│              │  PROTEC.  │                                  │
│              │           │                                  │
│              │ • Sobre-  │                                  │
│              │   carga   │                                  │
│              │ • Sobre-  │                                  │
│              │   descarga│                                  │
│              │ • Corto   │                                  │
│              │ • Balance │                                  │
│              └───────────┘                                  │
│                                                             │
└─────────────────────────────────────────────────────────────┘

Conexiones:
- B-: Negativo de la celda inferior
- BM: Punto medio (entre celdas) para balanceo
- B+: Positivo de la celda superior
- P-: Salida negativa (a carga)
- P+: Salida positiva (a carga)
```

### 2.3 Módulo de Carga

**Modelo:** Cargador 2S Li-ion (8.4V)

| Parámetro | Valor |
|-----------|-------|
| Voltaje entrada | 9-12V DC |
| Voltaje salida | 8.4V (carga completa) |
| Corriente carga | 1A - 2A |
| Indicadores | LED rojo (cargando) / verde (completo) |

> **Nota:** Se puede usar un cargador externo 2S o integrar un módulo de carga balanceada.

### 2.4 Regulador Buck (DC-DC Step-Down)

**Modelo:** XL4015 (o LM2596)

| Parámetro | Valor |
|-----------|-------|
| Voltaje entrada | 6V - 40V |
| Voltaje salida | 5V (ajustable) |
| Corriente máxima | 5A |
| Eficiencia | ~90% @ 1A |
| Frecuencia | 180 kHz |

```
┌─────────────────────────────────────────────────────────────┐
│                    REGULADOR XL4015                         │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│                    ┌─────────────────┐                      │
│                    │                 │                      │
│   VIN+ ●──────────►│  IN+      OUT+ │►────────● VOUT+ (5V) │
│   (7.4V)           │                 │                      │
│                    │     XL4015      │                      │
│                    │     BUCK        │                      │
│                    │                 │                      │
│   VIN- ●──────────►│  IN-      OUT- │►────────● VOUT- (GND)│
│   (GND)            │                 │                      │
│                    └─────────────────┘                      │
│                           │                                 │
│                           │ Ajuste                          │
│                           ▼                                 │
│                    ┌─────────────┐                          │
│                    │ Potencióm.  │ ← Ajustar a 5.0V        │
│                    │ (Multiturn) │                          │
│                    └─────────────┘                          │
│                                                             │
│   IMPORTANTE: Ajustar ANTES de conectar la carga           │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### 2.5 Distribución de Energía

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         DISTRIBUCIÓN DE ENERGÍA                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                     │
│  │  PACK 2S    │    │   BMS 2S    │    │   SWITCH    │                     │
│  │  7.4V       │───►│  HX-2S-D01  │───►│   ON/OFF    │                     │
│  │  2800mAh    │    │             │    │             │                     │
│  └─────────────┘    └─────────────┘    └──────┬──────┘                     │
│                                               │                             │
│                                               │ 7.4V                        │
│                                               ▼                             │
│                     ┌─────────────────────────────────────────────────┐    │
│                     │              BORNERA 7.4V                       │    │
│                     │  ┌─────┬─────┬─────┬─────┬─────┬─────┐         │    │
│                     │  │ V+  │ V+  │ V+  │ GND │ GND │ GND │         │    │
│                     │  └──┬──┴──┬──┴──┬──┴──┬──┴──┬──┴──┬──┘         │    │
│                     └─────┼─────┼─────┼─────┼─────┼─────┼─────────────┘    │
│                           │     │     │     │     │     │                   │
│                           │     │     │     │     │     │                   │
│                           │     │     └─────┼─────┼─────┘                   │
│                           │     │           │     │                         │
│                    ┌──────▼─────▼──┐  ┌─────▼─────▼─────┐                  │
│                    │   XL4015      │  │    NEXTION      │                  │
│                    │   BUCK        │  │    NX4024T032   │                  │
│                    │   7.4V→5V     │  │    (5V directo  │                  │
│                    └───────┬───────┘  │    del buck)    │                  │
│                            │          └─────────────────┘                  │
│                            │ 5V                                             │
│                            ▼                                                │
│          ┌─────────────────────────────────────────────────┐               │
│          │              BORNERA 5V                         │               │
│          │  ┌─────┬─────┬─────┬─────┬─────┬─────┐         │               │
│          │  │ 5V  │ 5V  │ 5V  │ GND │ GND │ GND │         │               │
│          │  └──┬──┴──┬──┴──┬──┴──┬──┴──┬──┴──┬──┘         │               │
│          └─────┼─────┼─────┼─────┼─────┼─────┼─────────────┘               │
│                │     │     │     │     │     │                              │
│         ┌──────▼──┐  │  ┌──▼──┐  │     │     │                              │
│         │ ESP32   │  │  │MCP  │  │     │     │                              │
│         │ NodeMCU │  │  │6002 │  │     │     │                              │
│         └─────────┘  │  └─────┘  │     │     │                              │
│                      │           │     │     │                              │
│               ┌──────▼──────┐    │     │     │                              │
│               │  NEXTION    │◄───┘     │     │                              │
│               │  (5V desde  │          │     │                              │
│               │   bornera)  │          │     │                              │
│               └─────────────┘          │     │                              │
│                                        │     │                              │
│                                 ┌──────▼─────▼──────┐                       │
│                                 │     LED RGB       │                       │
│                                 └───────────────────┘                       │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

NOTA: La Nextion puede alimentarse directamente desde 7.4V (tiene regulador 
interno) o desde los 5V del buck. Se recomienda usar los 5V del buck para 
mayor estabilidad.
```

---

## 3. Cálculos de Consumo y Autonomía

### 3.1 Consumo por Componente

| Componente | Voltaje | Corriente Típica | Corriente Pico | Potencia |
|------------|---------|------------------|----------------|----------|
| ESP32 NodeMCU | 5V | 80 mA | 150 mA | 0.40 W |
| Nextion NX4024T032 | 5V | 400 mA | 450 mA | 2.00 W |
| MCP6002 | 5V | 0.1 mA | 0.1 mA | 0.0005 W |
| LED RGB | 5V | 20 mA | 60 mA | 0.10 W |
| Otros (pérdidas) | - | 30 mA | 40 mA | 0.15 W |
| **TOTAL @ 5V** | **5V** | **~530 mA** | **~700 mA** | **2.65 W** |

### 3.2 Consumo Referido a Batería

Con el regulador Buck XL4015 (eficiencia ~90%):

```
Corriente desde batería = (Consumo @ 5V × 5V) / (Vbat × η)

Donde:
- Consumo @ 5V = 530 mA (típico)
- Vbat = 7.4V (nominal)
- η = 0.90 (eficiencia del buck)

Corriente típica desde batería:
I_bat = (0.530 A × 5V) / (7.4V × 0.90)
I_bat = 2.65 W / 6.66 W
I_bat = 0.398 A ≈ 400 mA

Corriente pico desde batería:
I_bat_pico = (0.700 A × 5V) / (7.4V × 0.90)
I_bat_pico = 3.50 W / 6.66 W
I_bat_pico = 0.526 A ≈ 530 mA
```

### 3.3 Cálculo de Autonomía

```
Autonomía = Capacidad de batería / Consumo

Con baterías Steren BAT-LI-18650/2800 (2800 mAh):

Autonomía típica:
T_típico = 2800 mAh / 400 mA = 7.0 horas

Autonomía mínima (uso intensivo):
T_mínimo = 2800 mAh / 530 mA = 5.3 horas

Autonomía con factor de seguridad (80% capacidad útil):
T_real_típico = (2800 × 0.80) mAh / 400 mA = 5.6 horas
T_real_mínimo = (2800 × 0.80) mAh / 530 mA = 4.2 horas
```

### 3.4 Resumen de Autonomía

| Escenario | Consumo Batería | Autonomía Teórica | Autonomía Real (80%) |
|-----------|-----------------|-------------------|----------------------|
| **Típico** | 400 mA | 7.0 h | **5.6 h** |
| **Intensivo** | 530 mA | 5.3 h | **4.2 h** |
| **Promedio** | 450 mA | 6.2 h | **5.0 h** |

> **Conclusión:** Con las baterías Steren 2800mAh en configuración 2S, se obtiene una autonomía práctica de **4-6 horas** de uso continuo.

---

## 4. Circuito de Salida Analógica

### 4.1 Especificaciones de Salida

| Parámetro | Valor |
|-----------|-------|
| Conector | BNC hembra |
| Rango de voltaje | 0V - 3.3V |
| Impedancia de salida | ~100Ω (con buffer) |
| Resolución | 8 bits (256 niveles) |
| Frecuencia máxima | 1 kHz |

### 4.2 Circuito Buffer con MCP6002

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    CIRCUITO DE SALIDA ANALÓGICA                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│                                    VCC (5V)                                 │
│                                       │                                     │
│                                       │                                     │
│                              ┌────────┴────────┐                           │
│                              │        8        │                           │
│                              │      VDD        │                           │
│                              │                 │                           │
│   ESP32                      │     MCP6002     │                           │
│   GPIO25 ────────┬──────────►│3+     1        │                           │
│   (DAC1)         │           │     \_____/    │────┬────────► BNC         │
│                  │           │2-              │    │          (SEÑAL)     │
│                  │     ┌─────│                │    │                       │
│                  │     │     │        4       │    │                       │
│                  │     │     │      VSS       │    R1                      │
│                  │     │     └────────┬───────┘    │ 100Ω                  │
│                  │     │              │            │                       │
│                  │     │              │            │                       │
│                  │     └──────────────┼────────────┤                       │
│                  │                    │            │                       │
│                  │                    │            │                       │
│                 ===                  ===          ===                      │
│                 GND                  GND          GND ──────► BNC (GND)    │
│                                                                             │
│                                                                             │
│   Configuración: Seguidor de voltaje (ganancia = 1)                        │
│   - Entrada no inversora (pin 3) conectada a GPIO25                        │
│   - Salida (pin 1) realimentada a entrada inversora (pin 2)                │
│   - R1 limita corriente y protege la salida                                │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 4.3 Interpretación de Señales en BNC

#### ECG (Electrocardiograma)

| Voltaje BNC | Valor DAC | Interpretación |
|-------------|-----------|----------------|
| 0.00V | 0 | Onda S profunda (-0.5 mV) |
| 0.65V | 50 | Línea base baja |
| 1.65V | 128 | Línea isoeléctrica (0 mV) |
| 2.60V | 200 | Onda R moderada |
| 3.30V | 255 | Pico R máximo (+1.5 mV) |

**Escala:** 1.65 mV/V (centrado en 1.65V)

#### EMG (Electromiografía)

| Voltaje BNC | Valor DAC | Interpretación |
|-------------|-----------|----------------|
| 0.00V | 0 | Contracción negativa máxima |
| 0.82V | 64 | Actividad baja negativa |
| 1.65V | 128 | Reposo (línea base) |
| 2.47V | 192 | Actividad baja positiva |
| 3.30V | 255 | Contracción positiva máxima |

**Escala:** ±5 mV rango total, centrado en 1.65V

#### PPG (Fotopletismografía)

| Voltaje BNC | Valor DAC | Interpretación |
|-------------|-----------|----------------|
| 0.65V | 50 | Valle diastólico |
| 1.65V | 128 | Nivel DC (línea base) |
| 2.60V | 200 | Pico sistólico |
| 2.00V | 155 | Muesca dicrótica |

**Escala:** Señal AC sobre DC, centrado en ~1.65V

---

## 5. Conexiones UART

### 5.1 ESP32 ↔ Nextion

| ESP32 (NodeMCU) | Nextion | Función |
|-----------------|---------|---------|
| GPIO17 (TX2) | RX (cable amarillo) | Datos ESP32→Nextion |
| GPIO16 (RX2) | TX (cable azul) | Datos Nextion→ESP32 |
| GND | GND (cable negro) | Referencia común |
| 5V | 5V (cable rojo) | Alimentación |

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      CONEXIÓN UART ESP32 ↔ NEXTION                          │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│      ESP32 NodeMCU                              Nextion NX4024T032          │
│    ┌─────────────────┐                        ┌─────────────────┐          │
│    │                 │                        │                 │          │
│    │            3V3  │                        │                 │          │
│    │                 │                        │                 │          │
│    │           GND ●─┼────────────────────────┼─● GND (Negro)   │          │
│    │                 │                        │                 │          │
│    │    GPIO17 (TX) ●┼────────────────────────┼─● RX (Amarillo) │          │
│    │                 │                        │                 │          │
│    │    GPIO16 (RX) ●┼────────────────────────┼─● TX (Azul)     │          │
│    │                 │                        │                 │          │
│    │            VIN  │                        │  5V (Rojo) ●────┼──► 5V    │
│    │                 │                        │                 │          │
│    └─────────────────┘                        └─────────────────┘          │
│                                                                             │
│    Configuración UART:                                                      │
│    - Velocidad: 115200 baud                                                │
│    - Formato: 8N1 (8 bits, sin paridad, 1 bit stop)                        │
│    - Nivel lógico: 3.3V (compatible con Nextion 5V tolerante)              │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 6. Indicador LED RGB

### 6.1 Circuito

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         CIRCUITO LED RGB                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│                              5V                                             │
│                               │                                             │
│                               │                                             │
│         ┌─────────────────────┼─────────────────────┐                      │
│         │                     │                     │                      │
│         │                     │                     │                      │
│        ┌┴┐                   ┌┴┐                   ┌┴┐                     │
│        │ │ R1                │ │ R2                │ │ R3                  │
│        │ │ 330Ω              │ │ 330Ω              │ │ 330Ω               │
│        └┬┘                   └┬┘                   └┬┘                     │
│         │                     │                     │                      │
│         │                     │                     │                      │
│         ▼                     ▼                     ▼                      │
│       ┌───┐                 ┌───┐                 ┌───┐                    │
│       │ R │                 │ G │                 │ B │                    │
│       │LED│                 │LED│                 │LED│                    │
│       └─┬─┘                 └─┬─┘                 └─┬─┘                    │
│         │                     │                     │                      │
│         │                     │                     │                      │
│         ▼                     ▼                     ▼                      │
│       GPIO4                 GPIO5                 GPIO18                   │
│       (ESP32)               (ESP32)               (ESP32)                  │
│                                                                             │
│   Configuración: Cátodo común (LEDs activos en LOW)                        │
│   Corriente por LED: (5V - 2V) / 330Ω ≈ 9 mA                              │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 6.2 Estados del LED

| Estado | Rojo | Verde | Azul | Significado |
|--------|------|-------|------|-------------|
| Apagado | OFF | OFF | OFF | Sistema apagado |
| Verde fijo | OFF | ON | OFF | Sistema listo (IDLE) |
| Verde parpadeante | OFF | BLINK | OFF | Generando señal |
| Azul fijo | OFF | OFF | ON | Modo configuración |
| Rojo fijo | ON | OFF | OFF | Error |
| Amarillo | ON | ON | OFF | Batería baja |
| Cian | OFF | ON | ON | Conectado a PC |

---

## 7. Esquema General

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                              BIOSIMULATOR PRO v1.1                                  │
│                            ESQUEMA GENERAL DE CONEXIONES                            │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                     │
│   ┌─────────────┐                                                                   │
│   │  BATERÍA    │                                                                   │
│   │  2S 7.4V    │                                                                   │
│   │  2800mAh    │                                                                   │
│   └──────┬──────┘                                                                   │
│          │                                                                          │
│          ▼                                                                          │
│   ┌─────────────┐      ┌─────────────┐      ┌─────────────┐                        │
│   │  BMS 2S     │─────►│   SWITCH    │─────►│  BORNERA    │                        │
│   │  HX-2S-D01  │      │   ON/OFF    │      │   7.4V      │                        │
│   └─────────────┘      └─────────────┘      └──────┬──────┘                        │
│                                                     │                               │
│                              ┌──────────────────────┴──────────────────────┐       │
│                              │                                             │       │
│                       ┌──────▼──────┐                               ┌──────▼──────┐│
│                       │  XL4015     │                               │  NEXTION    ││
│                       │  BUCK       │                               │  NX4024T032 ││
│                       │  7.4V→5V    │                               │  (o 5V)     ││
│                       └──────┬──────┘                               └──────┬──────┘│
│                              │                                             │       │
│                              │ 5V                                    UART  │       │
│                       ┌──────▼──────┐                                      │       │
│                       │  BORNERA    │◄─────────────────────────────────────┘       │
│                       │   5V        │                                              │
│                       └──────┬──────┘                                              │
│                              │                                                      │
│            ┌─────────────────┼─────────────────┐                                   │
│            │                 │                 │                                   │
│     ┌──────▼──────┐   ┌──────▼──────┐   ┌──────▼──────┐                           │
│     │   ESP32     │   │   MCP6002   │   │   LED RGB   │                           │
│     │   NodeMCU   │   │   Buffer    │   │   Estado    │                           │
│     │             │   │             │   │             │                           │
│     │  GPIO25 ────┼──►│  IN    OUT ─┼──►│             │                           │
│     │  (DAC)      │   │             │   │             │                           │
│     │             │   └─────────────┘   └─────────────┘                           │
│     │  GPIO17 ────┼────────────────────────────────────────► Nextion RX           │
│     │  (TX2)      │                                                                │
│     │             │                                                                │
│     │  GPIO16 ◄───┼──────────────────────────────────────── Nextion TX            │
│     │  (RX2)      │                                                                │
│     │             │                                                                │
│     │  GPIO4  ────┼──────────────────────────────────────► LED Rojo               │
│     │  GPIO5  ────┼──────────────────────────────────────► LED Verde              │
│     │  GPIO18 ────┼──────────────────────────────────────► LED Azul               │
│     │             │                                                                │
│     └─────────────┘                                                                │
│            │                                                                        │
│            │ GPIO25 (buffered)                                                     │
│            ▼                                                                        │
│     ┌─────────────┐                                                                │
│     │    BNC      │                                                                │
│     │   Output    │──────────────────────────────────────► Osciloscopio           │
│     │  0-3.3V     │                                                                │
│     └─────────────┘                                                                │
│                                                                                     │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 8. Lista de Materiales (BOM)

### 8.1 Componentes Principales

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
| 11 | Resistencias | 330Ω 1/4W (pack 10) | 1 | $0.50 | $0.50 |
| 12 | Resistencia | 100Ω 1/4W | 1 | $0.10 | $0.10 |
| 13 | Switch | Interruptor palanca ON/OFF | 1 | $1.00 | $1.00 |
| 14 | Borneras | Terminal block 2 pos (pack 5) | 2 | $1.00 | $2.00 |
| 15 | Cables | Jumper wires pack | 1 | $2.00 | $2.00 |
| 16 | Caja/Enclosure | Caja proyecto plástico | 1 | $5.00 | $5.00 |

### 8.2 Resumen de Costos

| Categoría | Costo |
|-----------|-------|
| Componentes electrónicos | $64.90 |
| Envío estimado | $10.00 |
| **TOTAL ESTIMADO** | **~$75 USD** |

---

## 9. Notas de Ensamblaje

### 9.1 Orden de Conexión Recomendado

1. **Preparar pack de baterías** - Conectar 2× 18650 en serie
2. **Conectar BMS** - B-, BM, B+ a las baterías
3. **Instalar switch** - Entre P+ del BMS y bornera 7.4V
4. **Configurar Buck** - Ajustar a 5.0V ANTES de conectar cargas
5. **Conectar ESP32** - Alimentar desde bornera 5V
6. **Conectar Nextion** - Alimentar desde bornera 5V, UART a ESP32
7. **Instalar buffer** - MCP6002 entre GPIO25 y BNC
8. **Conectar LEDs** - Con resistencias de 330Ω

### 9.2 Verificaciones Antes de Encender

- [ ] Voltaje del pack: 7.0V - 8.4V
- [ ] Voltaje del buck: 5.0V ±0.1V
- [ ] Polaridad correcta en todas las conexiones
- [ ] Sin cortocircuitos visibles
- [ ] Conexiones UART cruzadas (TX↔RX)

### 9.3 Prueba Inicial

1. Encender con switch
2. Verificar LED de estado (debe encender verde)
3. Verificar que Nextion muestra pantalla inicial
4. Probar comunicación serial (115200 baud)
5. Verificar salida DAC con multímetro (~1.65V en reposo)

---

## 10. Comparativa con Versión Anterior (ELECROW)

| Parámetro | ELECROW (cancelado) | Nextion (actual) |
|-----------|---------------------|------------------|
| Display | 7" 800×480 | 3.2" 400×240 |
| MCU Display | ESP32-S3 | Integrado |
| Baterías | 2P (3.7V, 4400mAh) | 2S (7.4V, 2800mAh) |
| Regulador | MT3608 Boost | XL4015 Buck |
| Autonomía | ~5.5h | ~5.0h |
| Costo total | ~$91 | ~$75 |
| Complejidad | Alta (2 MCUs) | Media (1 MCU) |

---

**Documento creado:** Diciembre 2024  
**Versión:** 1.1.0  
**Proyecto:** BioSimulator Pro
