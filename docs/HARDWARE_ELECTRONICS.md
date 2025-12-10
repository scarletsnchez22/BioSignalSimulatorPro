# BioSignalSimulator Pro - Diseño Electrónico

**Especificación completa del hardware electrónico del dispositivo**

---

| Campo | Valor |
|-------|-------|
| **Versión** | 1.1.0 |
| **Fecha** | Diciembre 2024 |
| **Estado** | Diseño Final |

---

## Tabla de Contenidos

1. [Resumen del Sistema](#1-resumen-del-sistema)
2. [Sistema de Alimentación](#2-sistema-de-alimentación)
3. [Módulo TP4056](#3-módulo-tp4056)
4. [Módulo Boost MT3608](#4-módulo-boost-mt3608)
5. [Distribución de Energía](#5-distribución-de-energía)
6. [Circuito de Salida Analógica](#6-circuito-de-salida-analógica)
7. [Indicadores LED](#7-indicadores-led)
8. [Conexiones UART](#8-conexiones-uart)
9. [Esquema General](#9-esquema-general)
10. [Cálculos de Consumo y Autonomía](#10-cálculos-de-consumo-y-autonomía)
11. [Lista de Materiales (BOM)](#11-lista-de-materiales-bom)

---

## 1. Resumen del Sistema

### 1.1 Arquitectura de Hardware

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                         BIOSIGNALSIMULATOR PRO - ARQUITECTURA                       │
└─────────────────────────────────────────────────────────────────────────────────────┘

    ┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐
    │   BATERÍAS      │────►│   CARGA Y       │────►│   REGULACIÓN    │
    │   2P 18650      │     │   PROTECCIÓN    │     │   BOOST         │
    │   3.7V 4400mAh  │     │   TP4056        │     │   MT3608        │
    │   (PARALELO)    │     │                 │     │   3.7V → 5V     │
    └─────────────────┘     └─────────────────┘     └────────┬────────┘
                                                             │ 5V
                            ┌────────────────────────────────┼────────────────────────┐
                            │                                │                        │
                            ▼                                ▼                        ▼
                    ┌───────────────┐              ┌───────────────┐        ┌───────────────┐
                    │   PANTALLA    │◄────UART────►│   ESP32       │───────►│   SALIDA      │
                    │   HMI 7"      │   921600     │   CEREBRO     │        │   ANALÓGICA   │
                    │   ELECROW     │              │   NodeMCU     │        │   BNC         │
                    └───────────────┘              └───────┬───────┘        └───────────────┘
                                                           │
                                                    ┌──────┴──────┐
                                                    │  LED RGB    │
                                                    └─────────────┘
```

### 1.2 Especificaciones Generales

| Parámetro | Valor |
|-----------|-------|
| Voltaje de batería | 3.7V nominal (3.0V - 4.2V) |
| Configuración baterías | 2P (paralelo) |
| Capacidad total | 4400 mAh |
| Voltaje de operación | 5V (regulado) |
| Consumo típico | ~475 mA @ 5V |
| Consumo pico | ~691 mA @ 5V |
| Autonomía típica | **5.5 horas** |
| Autonomía mínima | **3.8 horas** |

---

## 2. Sistema de Alimentación

### 2.1 Baterías

**Configuración: 2P (Paralelo) - 3.7V nominal, 4400mAh**

| Especificación | Valor |
|----------------|-------|
| **Tipo** | Li-ion 18650 |
| **Modelo** | BAT-LI-18650 |
| **Capacidad por celda** | 2200 mAh |
| **Voltaje nominal** | 3.7V |
| **Voltaje cargada** | 4.2V |
| **Voltaje descargada** | 3.0V |
| **Configuración** | 2P (paralelo) |
| **Capacidad total** | 4400 mAh (se suma en paralelo) |
| **Precio unitario** | $7.99 USD |

### 2.2 Cálculos del Pack 2P

```
CONFIGURACIÓN PARALELO (2P):
════════════════════════════

┌─────────────────────────────────────────────────────────────────┐
│                                                                 │
│   En PARALELO: El voltaje se MANTIENE, la capacidad se SUMA    │
│                                                                 │
│   V_total = V_celda (se mantiene)                               │
│   C_total = C_celda1 + C_celda2 (se suma)                       │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘

Voltaje nominal:
    V_nom = 3.7V (igual que una celda)

Voltaje máximo (completamente cargado):
    V_max = 4.2V

Voltaje mínimo (descargado):
    V_min = 3.0V

Capacidad total:
    C_total = 2200mAh + 2200mAh = 4400 mAh ✅

Energía total almacenada:
    E = V_nom × C_total
    E = 3.7V × 4.4Ah
    E = 16.28 Wh
```

### 2.3 Diagrama del Portapilas 2P

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                       PORTAPILAS 2×18650 EN PARALELO (2P)                           │
└─────────────────────────────────────────────────────────────────────────────────────┘

    ┌─────────────────────────────────────────────────────────────────────────────┐
    │                          PORTAPILAS 2P                                      │
    │                                                                             │
    │   ┌─────────────────────────────┐   ┌─────────────────────────────┐         │
    │   │         CELDA 1             │   │         CELDA 2             │         │
    │   │    Li-ion 18650 2200mAh     │   │    Li-ion 18650 2200mAh     │         │
    │   │         3.7V                │   │         3.7V                │         │
    │   │                             │   │                             │         │
    │   │   (+) ═══════════ (-)       │   │   (+) ═══════════ (-)       │         │
    │   └────┬────────────────┬───────┘   └────┬────────────────┬───────┘         │
    │        │                │                │                │                 │
    │        │                │                │                │                 │
    │        └───────┬────────┴────────────────┴────────┬───────┘                 │
    │                │                                  │                         │
    │                │  (+) UNIDOS                      │  (-) UNIDOS             │
    │                │  (PARALELO)                      │  (PARALELO)             │
    │                │                                  │                         │
    │           ┌────┴────┐                        ┌────┴────┐                    │
    │           │   B+    │                        │   B-    │                    │
    │           │  3.7V   │                        │   GND   │                    │
    │           │ (ROJO)  │                        │ (NEGRO) │                    │
    │           └────┬────┘                        └────┬────┘                    │
    │                │                                  │                         │
    └────────────────┼──────────────────────────────────┼─────────────────────────┘
                     │                                  │
                     ▼                                  ▼
              Al TP4056 B+                       Al TP4056 B-
```

---

## 3. Módulo TP4056

### 3.1 Especificaciones

| Parámetro | Valor |
|-----------|-------|
| **Modelo** | TP4056 con DW01A + FS8205A |
| **Voltaje de entrada** | 4.5V - 5.5V (USB) |
| **Corriente de carga** | 1A máximo |
| **Voltaje de carga** | 4.2V |
| **Protección sobrecarga** | 4.25V - 4.3V |
| **Protección sobredescarga** | 2.4V - 2.5V |
| **Protección cortocircuito** | Sí |
| **LEDs indicadores** | ROJO = Cargando, AZUL = Completo |

### 3.2 Diagrama de Conexiones

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                    MÓDULO TP4056 CON PROTECCIÓN - CONEXIONES                        │
└─────────────────────────────────────────────────────────────────────────────────────┘

                         ┌─────────────────────────────────────────────────┐
                         │           MÓDULO TP4056 + PROTECCIÓN            │
                         │                                                 │
                         │   ┌─────────────────────────────────────────┐   │
                         │   │             VISTA SUPERIOR              │   │
                         │   │                                         │   │
                         │   │   ┌─────┐               ┌─────┐         │   │
                         │   │   │ LED │ ROJO          │ LED │ AZUL    │   │
                         │   │   │     │ Cargando      │     │ Listo   │   │
                         │   │   └─────┘               └─────┘         │   │
                         │   │                                         │   │
                         │   │   ┌───────────────────────────────┐     │   │
                         │   │   │         CHIP TP4056           │     │   │
                         │   │   │    (Controlador de carga)     │     │   │
                         │   │   └───────────────────────────────┘     │   │
                         │   │                                         │   │
                         │   │   ┌───────────────────────────────┐     │   │
                         │   │   │   DW01A + FS8205A             │     │   │
                         │   │   │   (Circuito de protección)    │     │   │
                         │   │   └───────────────────────────────┘     │   │
                         │   │                                         │   │
                         │   └─────────────────────────────────────────┘   │
                         │                                                 │
                         │   ┌──────┐  ┌──────┐  ┌──────┐  ┌──────┐       │
                         │   │ IN+  │  │ IN-  │  │ OUT+ │  │ OUT- │       │
                         │   └──┬───┘  └──┬───┘  └──┬───┘  └──┬───┘       │
                         │      │         │         │         │           │
                         │   ┌──────┐  ┌──────┐                           │
                         │   │  B+  │  │  B-  │                           │
                         │   └──┬───┘  └──┬───┘                           │
                         │      │         │                               │
                         └──────┼─────────┼───────────────────────────────┘
                                │         │
                                ▼         ▼
                           A Batería  A Batería
                              (+)        (-)


    PINES (6 terminales):
    ═════════════════════
    
    IN+   → Entrada USB 5V (para cargar)
    IN-   → Entrada USB GND
    B+    → Conexión directa a batería (+)
    B-    → Conexión directa a batería (-)
    OUT+  → Salida protegida (+) → Al Switch ON/OFF
    OUT-  → Salida protegida (-) → GND común
```

### 3.3 Estados del LED

| Estado | LED Rojo | LED Azul | Descripción |
|--------|----------|----------|-------------|
| Cargando | ✅ ON | ❌ OFF | Batería recibiendo carga |
| Carga completa | ❌ OFF | ✅ ON | Batería al 100% |
| Sin USB | ❌ OFF | ❌ OFF | No hay fuente de carga |

---

## 4. Módulo Boost MT3608

### 4.1 Especificaciones

| Parámetro | Valor |
|-----------|-------|
| **Modelo** | MT3608 (módulo) |
| **Voltaje entrada** | 2V - 24V |
| **Voltaje salida** | 5V - 28V (ajustable) |
| **Corriente máxima** | 2A |
| **Eficiencia típica** | 90% - 93% |
| **Ajuste** | Potenciómetro multivuelta |

### 4.2 Diagrama de Conexión

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                            MÓDULO BOOST MT3608                                      │
└─────────────────────────────────────────────────────────────────────────────────────┘

                    ┌─────────────────────────────────────────────────┐
                    │                  MT3608 MODULE                  │
                    │                                                 │
                    │   ┌─────────────────────────────────────────┐   │
                    │   │      CONVERTIDOR DC-DC BOOST            │   │
                    │   │                                         │   │
                    │   │   Vin: 3.7V (desde batería)             │   │
                    │   │   Vout: 5.0V (ajustado)                 │   │
                    │   │   Eficiencia: ~90%                      │   │
                    │   │                                         │   │
                    │   │   ┌───────────────────────────────┐     │   │
                    │   │   │       POTENCIÓMETRO           │     │   │
                    │   │   │    (Girar para ajustar 5V)    │     │   │
                    │   │   └───────────────────────────────┘     │   │
                    │   │                                         │   │
                    │   └─────────────────────────────────────────┘   │
                    │                                                 │
                    │   ┌──────┐  ┌──────┐  ┌──────┐  ┌──────┐       │
                    │   │ IN+  │  │ IN-  │  │ OUT+ │  │ OUT- │       │
                    │   └──┬───┘  └──┬───┘  └──┬───┘  └──┬───┘       │
                    │      │         │         │         │           │
                    └──────┼─────────┼─────────┼─────────┼───────────┘
                           │         │         │         │
                           ▼         ▼         ▼         ▼
                        Desde     Desde     A ESP32   A GND
                        Switch    GND       VIN       común
                        3.7V      común     5V
```

### 4.3 Cálculos de Eficiencia

```
CÁLCULO DE EFICIENCIA Y PÉRDIDAS:
═════════════════════════════════

Datos de entrada:
    Vin = 3.7V (voltaje nominal batería)
    Vout = 5.0V (voltaje de salida)
    Iout = 475 mA (consumo típico del sistema @ 5V)
    η = 90% (eficiencia del MT3608)

Potencia de salida:
    Pout = Vout × Iout
    Pout = 5.0V × 0.475A = 2.375W

Potencia de entrada:
    Pin = Pout / η
    Pin = 2.375W / 0.90 = 2.64W

Corriente de entrada (desde batería):
    Iin = Pin / Vin
    Iin = 2.64W / 3.7V = 714 mA

Pérdidas en el convertidor:
    Ploss = Pin - Pout
    Ploss = 2.64W - 2.375W = 0.265W
```

---

## 5. Distribución de Energía

### 5.1 Bornera de Distribución

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                         BORNERA DE DISTRIBUCIÓN                                     │
└─────────────────────────────────────────────────────────────────────────────────────┘

    Desde TP4056 (OUT+) después del SWITCH
                    │
                    ▼
    ┌─────────────────────────────────────────────────────────────────────────────┐
    │                                                                             │
    │   ┌─────────────────────────────────────────────────────────────────────┐   │
    │   │                    BORNERA 4 POSICIONES (+)                         │   │
    │   │                                                                     │   │
    │   │   ┌─────┐    ┌─────┐    ┌─────┐    ┌─────┐                         │   │
    │   │   │  1  │    │  2  │    │  3  │    │  4  │                         │   │
    │   │   │DESDE│    │  A  │    │  A  │    │RESER│                         │   │
    │   │   │SWIT │    │PANT │    │BOOST│    │ VA  │                         │   │
    │   │   │ CH  │    │ BAT │    │MT360│    │     │                         │   │
    │   │   └─────┘    └─────┘    └─────┘    └─────┘                         │   │
    │   │                                                                     │   │
    │   └─────────────────────────────────────────────────────────────────────┘   │
    │                                                                             │
    │   ┌─────────────────────────────────────────────────────────────────────┐   │
    │   │                    BORNERA 4 POSICIONES (-)                         │   │
    │   │                                                                     │   │
    │   │   ┌─────┐    ┌─────┐    ┌─────┐    ┌─────┐                         │   │
    │   │   │  1  │    │  2  │    │  3  │    │  4  │                         │   │
    │   │   │DESDE│    │  A  │    │  A  │    │RESER│                         │   │
    │   │   │ GND │    │PANT │    │BOOST│    │ VA  │                         │   │
    │   │   │     │    │ GND │    │ GND │    │     │                         │   │
    │   │   └─────┘    └─────┘    └─────┘    └─────┘                         │   │
    │   │                                                                     │   │
    │   └─────────────────────────────────────────────────────────────────────┘   │
    │                                                                             │
    │   MODELO: Bornera de tornillo 4 posiciones, paso 5.08mm                     │
    │                                                                             │
    └─────────────────────────────────────────────────────────────────────────────┘
```

---

## 6. Circuito de Salida Analógica

### 6.1 Especificaciones

| Parámetro | Valor |
|-----------|-------|
| **Fuente de señal** | ESP32 GPIO25 (DAC1) |
| **Rango de voltaje DAC** | 0V - 3.3V |
| **Resolución DAC** | 8 bits (256 niveles) |
| **Buffer** | MCP6002 (voltage follower) |
| **Impedancia de salida** | < 100Ω |
| **Conector** | BNC hembra |

### 6.2 Diagrama del Circuito Buffer

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                       CIRCUITO DE SALIDA ANALÓGICA                                  │
└─────────────────────────────────────────────────────────────────────────────────────┘

                                        +5V
                                         │
                                        ┌┴┐
                                        │ │ C1 = 100nF
                                        └┬┘
                                         │
    ┌────────────────────────────────────┼────────────────────────────────────────┐
    │                                    │                                        │
    │   ESP32 CEREBRO                    │         MCP6002 (U1A)                  │
    │                                    │                                        │
    │   ┌─────────────┐                  │    ┌─────────────────────────┐         │
    │   │             │                  │    │                         │         │
    │   │   GPIO25    │                  │    │    ┌───────────────┐    │         │
    │   │   (DAC1)    ├──────────────────┼────┼───►│ 3  +          │    │         │
    │   │             │                  │    │    │               │    │         │
    │   │   0-3.3V    │                  │    │    │    U1A        │ 1  │         │
    │   │             │                  │    │    │   MCP6002     ├────┼─────────┼──► BNC
    │   │             │                  │    │    │               │    │         │
    │   └─────────────┘                  │    │    │ 2  -          │    │         │
    │                                    │    │    └───────┬───────┘    │         │
    │                                    │    │            │            │         │
    │                                    │    │            └────────────┘         │
    │                                    │    │         (Realimentación 100%)     │
    │                                    │    │                                   │
    │                                    │    │    Pin 8 = VDD (+5V)              │
    │                                    │    │    Pin 4 = VSS (GND)              │
    │                                    │    │                                   │
    │   GND ─────────────────────────────┴────┴───────────────────────────────────┼──► BNC GND
    │                                                                             │
    └─────────────────────────────────────────────────────────────────────────────┘


    PINOUT MCP6002 (DIP-8):
    ═══════════════════════
              ┌─────────────┐
         OUT1 │ 1         8 │ VDD (+5V)
         -IN1 │ 2         7 │ OUT2 (no usar)
         +IN1 │ 3         6 │ -IN2 (no usar)
         VSS  │ 4         5 │ +IN2 (no usar)
        (GND) └─────────────┘
```

### 6.3 Tablas de Interpretación de Señales

#### ECG (Electrocardiograma)

| Valor del Modelo | Voltaje en BNC | Significado Fisiológico |
|------------------|----------------|-------------------------|
| -0.5 mV | 0.00 V | Onda S profunda (máxima) |
| 0.0 mV | 0.83 V | Línea base (isoeléctrica) |
| +0.5 mV | 1.65 V | Onda T típica |
| +1.0 mV | 2.48 V | Pico R típico |
| +1.5 mV | 3.30 V | Pico R máximo |

**Escala:** 1 mV del modelo = 1.65 V en BNC

**Osciloscopio:** 500 mV/div, 200 ms/div, DC coupling

#### EMG (Electromiografía)

| Valor del Modelo | Voltaje en BNC | Significado Fisiológico |
|------------------|----------------|-------------------------|
| -5.0 mV | 0.00 V | Pico negativo máximo |
| 0.0 mV | 1.65 V | Línea base (reposo) |
| +5.0 mV | 3.30 V | Pico positivo máximo |

**Escala:** 1 mV del modelo = 0.33 V en BNC

**Osciloscopio:** 500 mV/div, 50 ms/div, DC coupling

#### PPG (Fotopletismografía)

| Valor del Modelo | Voltaje en BNC | Significado Fisiológico |
|------------------|----------------|-------------------------|
| 0.0 | 0.00 V | Valle diastólico |
| 0.5 | 1.65 V | Nivel medio |
| 1.0 | 3.30 V | Pico sistólico máximo |

**Escala:** 0.1 unidades = 0.33 V en BNC

**Osciloscopio:** 500 mV/div, 200 ms/div, DC coupling

---

## 7. Indicadores LED

### 7.1 Circuito LED RGB

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                              CIRCUITO LED RGB                                       │
└─────────────────────────────────────────────────────────────────────────────────────┘

    ESP32 CEREBRO                                           LED RGB
                                                       (Cátodo Común)

    GPIO4 ────────────┬──[R1 = 330Ω]───────────────────────► ROJO
                      │
    GPIO5 ────────────┼──[R2 = 330Ω]───────────────────────► VERDE ──────► GND
                      │
    GPIO18 ───────────┴──[R3 = 330Ω]───────────────────────► AZUL
```

### 7.2 Estados del LED

| Estado del Sistema | Color | GPIO4 | GPIO5 | GPIO18 |
|--------------------|-------|-------|-------|--------|
| Sistema listo | Verde | LOW | HIGH | LOW |
| Generando señal | Azul | LOW | LOW | PWM |
| Error | Rojo | HIGH | LOW | LOW |
| Batería baja | Rojo parpadeo | BLINK | LOW | LOW |
| WiFi conectado | Cian | LOW | HIGH | HIGH |

---

## 8. Conexiones UART

### 8.1 Especificaciones

| Parámetro | Valor |
|-----------|-------|
| **Velocidad** | 921600 baud |
| **Formato** | 8N1 |
| **Nivel lógico** | 3.3V |

### 8.2 Diagrama de Conexión

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                         CONEXIÓN UART - ESP32 CEREBRO ↔ HMI                         │
└─────────────────────────────────────────────────────────────────────────────────────┘

    ESP32 CEREBRO                                              ELECROW HMI
    (NodeMCU v1.1)                                            (ESP32-S3 7")

    ┌─────────────────┐                                  ┌─────────────────┐
    │                 │         Cable VERDE              │                 │
    │    GPIO17 ──────┼─────────────────────────────────►│ UART0_RX        │
    │    (TX2)        │                                  │                 │
    │                 │         Cable AMARILLO           │                 │
    │    GPIO16 ◄─────┼──────────────────────────────────│ UART0_TX        │
    │    (RX2)        │                                  │                 │
    │                 │         Cable NEGRO              │                 │
    │    GND ─────────┼─────────────────────────────────►│ GND             │
    │                 │                                  │                 │
    └─────────────────┘                                  └─────────────────┘


    RESUMEN:
    ════════
    ESP32 GPIO17 (TX) ──────► ELECROW UART0_RX
    ESP32 GPIO16 (RX) ◄────── ELECROW UART0_TX
    ESP32 GND ─────────────── ELECROW GND
```

---

## 9. Esquema General

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                         ESQUEMA GENERAL COMPLETO                                    │
└─────────────────────────────────────────────────────────────────────────────────────┘

                    ┌─────────────────┐
                    │    USB-C        │  (Para cargar)
                    │    5V 1A        │
                    └────────┬────────┘
                             │
                             ▼
                    ┌─────────────────┐
                    │    TP4056       │
                    │  con protección │
                    │                 │
                    │  IN+ ◄── USB+   │
                    │  IN- ◄── USB-   │
                    │                 │
                    │  B+ ────────────┼──────────────────────────────────────┐
                    │  B- ────────────┼──────────────────────────────────┐   │
                    │                 │                                  │   │
                    │  OUT+ ──────────┼──────────────────────────────┐   │   │
                    │  OUT- ──────────┼──────────────────────────┐   │   │   │
                    │                 │                          │   │   │   │
                    └─────────────────┘                          │   │   │   │
                                                                 │   │   │   │
                    ┌────────────────────────────────────────────┼───┼───┼───┼───┐
                    │         PORTAPILAS 2×18650 PARALELO        │   │   │   │   │
                    │                                            │   │   │   │   │
                    │   ┌───────┐     ┌───────┐                  │   │   │   │   │
                    │   │ BAT 1 │     │ BAT 2 │                  │   │   │   │   │
                    │   │ 3.7V  │     │ 3.7V  │                  │   │   │   │   │
                    │   │2200mAh│     │2200mAh│                  │   │   │   │   │
                    │   └───┬───┘     └───┬───┘                  │   │   │   │   │
                    │       └──────┬──────┘                      │   │   │   │   │
                    │         (+)──┼─────────────────────────────┼───┼───┼───┘   │
                    │         (-)──┼─────────────────────────────┼───┼───┘       │
                    │              │                             │   │           │
                    └──────────────┼─────────────────────────────┼───┼───────────┘
                                   │                             │   │
                                   │                        ┌────┘   └────┐
                                   │                        │             │
                                   │                   ┌────▼────┐   ┌────▼────┐
                                   │                   │ SWITCH  │   │   GND   │
                                   │                   │ ON/OFF  │   │         │
                                   │                   └────┬────┘   └────┬────┘
                                   │                        │             │
                                   │                        │ 3.7V        │
                    ┌──────────────┼────────────────────────┴─────────────┴──────┐
                    │              │              BORNERA                        │
                    │         ┌────┴────────────────┬────────────────┐           │
                    │         │                     │                │           │
                    │         ▼                     ▼                ▼           │
                    │   ┌───────────┐         ┌───────────┐    ┌───────────┐     │
                    │   │ A PANTALLA│         │  A BOOST  │    │  (GND)    │     │
                    │   │ Puerto BAT│         │  MT3608   │    │           │     │
                    │   └─────┬─────┘         └─────┬─────┘    └───────────┘     │
                    │         │                     │                            │
                    └─────────┼─────────────────────┼────────────────────────────┘
                              │                     │
                              ▼                     ▼
                    ┌─────────────────┐   ┌─────────────────┐
                    │   ELECROW HMI   │   │    MT3608       │
                    │   7" 800×480    │   │    3.7V → 5V    │
                    │                 │   │        │        │
                    │   Puerto BAT ◄──┤   │        ▼        │
                    │   (3.7V)        │   │   ┌─────────┐   │
                    │                 │   │   │  ESP32  │   │
                    │   UART0 ◄───────┼───┼──►│ CEREBRO │   │
                    │                 │   │   │         │   │
                    │                 │   │   │ GPIO25──┼───┼──► MCP6002 ──► BNC
                    │                 │   │   │ GPIO4/5─┼───┼──► LED RGB
                    │                 │   │   └─────────┘   │
                    └─────────────────┘   └─────────────────┘
```

---

## 10. Cálculos de Consumo y Autonomía

### 10.1 Consumo por Componente @ 5V

| Componente | Típico | Pico |
|------------|--------|------|
| **ESP32 Cerebro** | | |
| - CPU dual-core | 80 mA | 100 mA |
| - WiFi promedio | 120 mA | 200 mA |
| - DAC activo | 5 mA | 5 mA |
| **Subtotal ESP32** | **205 mA** | **305 mA** |
| **ELECROW HMI** | | |
| - ESP32-S3 | 100 mA | 150 mA |
| - LCD 7" backlight | 150 mA | 200 mA |
| - Touch GT911 | 10 mA | 15 mA |
| **Subtotal HMI** | **260 mA** | **365 mA** |
| **Otros** | | |
| - MCP6002 | 0.5 mA | 1 mA |
| - LED RGB | 10 mA | 20 mA |
| **Subtotal Otros** | **10.5 mA** | **21 mA** |
| **TOTAL @ 5V** | **475.5 mA** | **691 mA** |

### 10.2 Consumo desde Batería

```
CÁLCULO DE CORRIENTE DESDE BATERÍA:
═══════════════════════════════════

Fórmula:
    I_batería = (I_carga × V_salida) / (V_batería × η)

CONSUMO TÍPICO:
    I_bat = (475.5mA × 5V) / (3.7V × 0.90)
    I_bat = 2377.5 / 3.33
    I_bat = 714 mA

CONSUMO PICO:
    I_bat = (691mA × 5V) / (3.7V × 0.90)
    I_bat = 3455 / 3.33
    I_bat = 1037 mA
```

### 10.3 Autonomía

```
AUTONOMÍA DEL SISTEMA:
══════════════════════

Capacidad útil (90% de 4400mAh):
    C_útil = 3960 mAh

AUTONOMÍA TÍPICA:
    T = 3960 mAh / 714 mA = 5.55 horas ✅

AUTONOMÍA MÍNIMA (pico):
    T = 3960 mAh / 1037 mA = 3.82 horas ✅

AUTONOMÍA SIN WiFi:
    I_sin_wifi = 355 mA @ 5V → 533 mA desde batería
    T = 3960 / 533 = 7.43 horas ✅
```

### 10.4 Resumen de Autonomía

| Escenario | Consumo @ 5V | Consumo Batería | Autonomía |
|-----------|--------------|-----------------|-----------|
| Típico (WiFi activo) | 475 mA | 714 mA | **5.5 horas** ✅ |
| Pico máximo | 691 mA | 1037 mA | **3.8 horas** ✅ |
| Sin WiFi | 355 mA | 533 mA | **7.4 horas** ✅ |

---

## 11. Lista de Materiales (BOM)

### 11.1 Componentes Principales

| # | Componente | Modelo | Cant. | Precio | Total |
|---|------------|--------|-------|--------|-------|
| 1 | Batería Li-ion 18650 | BAT-LI-18650 2200mAh | 2 | $7.99 | $15.98 |
| 2 | Portapilas 2×18650 | Paralelo, con cables | 1 | $3.00 | $3.00 |
| 3 | Módulo cargador | TP4056 con protección | 1 | $2.00 | $2.00 |
| 4 | Conector USB-C | Hembra, para PCB | 1 | $1.50 | $1.50 |
| 5 | Módulo Boost | MT3608 3.7V→5V | 1 | $2.50 | $2.50 |
| 6 | Switch | SPST 2A panel | 1 | $1.50 | $1.50 |
| 7 | ESP32 NodeMCU | v1.1 WROOM-32 | 1 | $8.00 | $8.00 |
| 8 | Pantalla ELECROW | 7" ESP32-S3 HMI | 1 | $50.00 | $50.00 |
| 9 | Op-Amp | MCP6002 DIP-8 | 1 | $1.00 | $1.00 |
| 10 | LED RGB | 5mm cátodo común | 1 | $0.50 | $0.50 |
| 11 | Conector BNC | Hembra panel | 1 | $2.00 | $2.00 |

### 11.2 Componentes Pasivos

| # | Componente | Valor | Cant. | Precio | Total |
|---|------------|-------|-------|--------|-------|
| 12 | Resistencia | 330Ω 1/4W | 3 | $0.05 | $0.15 |
| 13 | Capacitor cerámico | 100nF | 2 | $0.10 | $0.20 |

### 11.3 Conectores y Cables

| # | Componente | Tipo | Cant. | Precio | Total |
|---|------------|------|-------|--------|-------|
| 14 | Bornera | 4 pos, 5.08mm | 2 | $0.50 | $1.00 |
| 15 | Cable JST-PH | 2 pines, 15cm | 1 | $0.50 | $0.50 |
| 16 | Cable Dupont | H-H 3 hilos | 1 | $0.30 | $0.30 |
| 17 | Cable AWG22 | Rojo/Negro 1m | 2 | $0.50 | $1.00 |

### 11.4 Resumen de Costos

| Categoría | Total |
|-----------|-------|
| Componentes principales | $87.98 |
| Componentes pasivos | $0.35 |
| Conectores y cables | $2.80 |
| **TOTAL ESTIMADO** | **$91.13 USD** |

---

## Notas Finales

- **Baterías en PARALELO (2P):** Voltaje 3.7V, capacidad 4400mAh
- **Cargador:** TP4056 con protección integrada
- **Regulador:** MT3608 Boost (3.7V → 5V)
- **Autonomía garantizada:** Mínimo 3.8 horas, típico 5.5 horas
- **Salida analógica:** 1 BNC, 0-3.3V, buffered con MCP6002
