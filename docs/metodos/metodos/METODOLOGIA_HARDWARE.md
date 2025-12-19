# BioSimulator Pro - Diseño Electrónico de Hardware

**Versión:** 2.0.0  
**Fecha:** 18 Diciembre 2025  
**Autor:** [Nombre del Tesista]  
**Documento para:** Trabajo de Titulación

---

## Índice

1. [Introducción y Objetivos](#1-introducción-y-objetivos)
2. [Metodología de Diseño](#2-metodología-de-diseño)
3. [Arquitectura del Sistema](#3-arquitectura-del-sistema)
4. [Sistema de Alimentación](#4-sistema-de-alimentación)
5. [Cálculos de Consumo y Autonomía](#5-cálculos-de-consumo-y-autonomía)
6. [PCB Principal y Circuitos](#6-pcb-principal-y-circuitos)
7. [Esquema Electrónico Completo](#7-esquema-electrónico-completo)
8. [Diseño Mecánico y Análisis Térmico](#8-diseño-mecánico-y-análisis-térmico)
9. [Lista de Materiales (BOM)](#9-lista-de-materiales-bom)
10. [Manual de Usuario](#10-manual-de-usuario)
11. [Limitaciones y Precauciones](#11-limitaciones-y-precauciones)
12. [Referencias](#12-referencias)

---

## 1. Introducción y Objetivos

### 1.1 Propósito del Documento

Este documento presenta el diseño electrónico detallado del BioSimulator Pro, un simulador portátil de señales biológicas (ECG, EMG, PPG). Se incluyen cálculos, justificaciones técnicas, criterios de diseño, selección de componentes y análisis térmico.

### 1.2 Objetivos del Diseño Electrónico

| Objetivo | Métrica | Criterio de Éxito |
|----------|---------|-------------------|
| Portabilidad | Autonomía | ≥ 2.5 horas de uso continuo |
| Seguridad | Protecciones | Sobrecarga, sobredescarga, cortocircuito |
| Costo | Presupuesto | < $150 USD total |
| Simplicidad | Componentes | Disponibles localmente |
| Compatibilidad | Voltajes | 5V para todos los módulos |
| Conectividad | WiFi | Streaming en tiempo real |

### 1.3 Normativas y Estándares

- **IEC 60950-1:** Seguridad de equipos de tecnología de la información
- **IEC 62133:** Requisitos de seguridad para baterías de litio
- **Buenas prácticas:** Diseño de sistemas embebidos con baterías Li-ion

---

## 2. Metodología de Diseño

### 2.1 Proceso de Diseño

```
┌─────────────────────────────────────────────────────────────────┐
│                    METODOLOGÍA DE DISEÑO                        │
├─────────────────────────────────────────────────────────────────┤
│  1. ANÁLISIS DE REQUISITOS → Definir funciones y restricciones │
│  2. SELECCIÓN DE COMPONENTES → Evaluar alternativas            │
│  3. DISEÑO DE CIRCUITOS → Calcular y dimensionar               │
│  4. DISEÑO MECÁNICO → CAD y análisis térmico                   │
│  5. VALIDACIÓN → Prototipar y medir                            │
└─────────────────────────────────────────────────────────────────┘
```

### 2.2 Criterios de Diseño

| Criterio | Descripción | Aplicación |
|----------|-------------|------------|
| Simplicidad | Minimizar complejidad | Módulos integrados (TP4056, MT3608) |
| Disponibilidad | Componentes locales | Proveedores Ecuador |
| Seguridad | Protección contra fallas | DW01 integrado |
| Eficiencia | Maximizar autonomía | MT3608 η=88% |
| Térmica | Disipación adecuada | Ventilación pasiva |

---

## 3. Arquitectura del Sistema

### 3.1 Diagrama de Bloques

```
┌──────────────────────────────────────────────────────────────────┐
│                    BIOSIMULATOR PRO v2.0                         │
├──────────────────────────────────────────────────────────────────┤
│                                                                  │
│  USB 5V ──► TP4056+DW01 ──► BATERÍAS 2×18650 (4400mAh)          │
│                    │                                             │
│                    ▼                                             │
│              SWITCH ON/OFF                                       │
│                    │                                             │
│                    ▼                                             │
│               MT3608 (3.7V→5V, η=88%)                           │
│                    │                                             │
│     ┌──────────────┼──────────────┐                             │
│     ▼              ▼              ▼                             │
│  ESP32         NEXTION        TL072──►BNC                       │
│  WROOM-32      7" 800×480     Buffer                            │
│  (WiFi AP)                    0-3.3V                            │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

### 3.2 Especificaciones del Sistema

| Parámetro | Valor |
|-----------|-------|
| Voltaje batería | 3.7V nominal (3.0V-4.2V) |
| Configuración | 2P paralelo |
| Capacidad | 4400 mAh |
| Voltaje operación | 5V |
| Regulador | MT3608 Step-Up |
| Buffer salida | TL072 JFET Op-Amp |
| Autonomía sin WiFi | 4.2 horas |
| Autonomía con WiFi | 3.4 horas |

---

## 4. Sistema de Alimentación

### 4.1 Baterías 2×18650 en Paralelo

| Parámetro | Por Celda | Pack 2P |
|-----------|-----------|---------|
| Modelo | 18650 Li-ion Steren | - |
| Voltaje nominal | 3.7V | 3.7V |
| Voltaje máximo | 4.2V | 4.2V |
| Voltaje mínimo | 3.0V | 3.0V |
| Capacidad | 2200 mAh | **4400 mAh** |
| Energía | 8.14 Wh | **16.28 Wh** |
| Corriente máx | 2A | 4A |

### 4.2 Módulo TP4056 con Protección DW01

| Característica | Valor |
|----------------|-------|
| Interfaz | USB Micro-B |
| Corriente de carga | 1000 mA |
| Voltaje de corte | 4.2V ±1% |
| Protección sobrecarga | > 4.25V |
| Protección sobredescarga | < 2.5V |
| Protección sobrecorriente | > 3A |

### 4.3 Regulador MT3608 Step-Up

| Parámetro | Valor |
|-----------|-------|
| Voltaje entrada | 2V - 24V |
| Voltaje salida | 5V (ajustable) |
| Corriente máxima | 2A |
| Frecuencia | 1.2 MHz |
| Eficiencia @ 0.7A | 88% |
| Eficiencia @ 1.0A (WiFi) | 85% |

---

## 5. Cálculos de Consumo y Autonomía

### 5.1 Desglose de Consumos (Sin WiFi)

| Componente | I Típica | P Típica | Fuente |
|------------|----------|----------|--------|
| ESP32-WROOM-32 | 80 mA | 400 mW | Datasheet |
| Nextion NX8048T070 | 510 mA | 2550 mW | Datasheet |
| TL072 Buffer | 5 mA | 25 mW | Datasheet |
| LED RGB | 4 mA | 18 mW | Calculado |
| Divisor UART | 0.8 mA | 4 mW | Calculado |
| Pérdidas | 10 mA | 50 mW | Estimado |
| **TOTAL** | **610 mA** | **3047 mW** | |

### 5.2 Desglose de Consumos (Con WiFi)

| Componente | I Típica | P Típica | Fuente |
|------------|----------|----------|--------|
| ESP32 (WiFi AP) | 180 mA | 900 mW | Datasheet |
| Nextion NX8048T070 | 510 mA | 2550 mW | Datasheet |
| TL072 Buffer | 5 mA | 25 mW | Datasheet |
| LED RGB | 4 mA | 18 mW | Calculado |
| Divisor UART | 0.8 mA | 4 mW | Calculado |
| Pérdidas | 12 mA | 60 mW | Estimado |
| **TOTAL** | **712 mA** | **3557 mW** | |

### 5.3 Corriente desde Batería

```
┌─────────────────────────────────────────────────────────────────┐
│              CORRIENTE DESDE LA BATERÍA                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  SIN WiFi:                                                     │
│  P_sistema = 3.05 W, η = 86.7%                                 │
│  P_batería = 3.05 / 0.867 = 3.52 W                            │
│  I_batería = 3.52 / 3.7 = 0.95 A                              │
│                                                                 │
│  CON WiFi:                                                     │
│  P_sistema = 3.56 W, η = 83.7%                                 │
│  P_batería = 3.56 / 0.837 = 4.25 W                            │
│  I_batería = 4.25 / 3.7 = 1.15 A                              │
│                                                                 │
│  PICO (WiFi TX + arranque):                                    │
│  I_batería_pico = 1.58 A                                       │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 5.4 Autonomía

| Escenario | Consumo Batería | Autonomía |
|-----------|-----------------|-----------|
| Sin WiFi | 0.95 A | **4.2 h** |
| Con WiFi | 1.15 A | **3.4 h** |
| Intensivo | 1.37 A | **2.9 h** |

Capacidad útil: 3960 mAh (90% de 4400 mAh)

### 5.5 Tiempo de Carga

- Capacidad: 4400 mAh
- Corriente: 1000 mA
- **Tiempo total: ~5 horas**

---

## 6. PCB Principal y Circuitos

### 6.1 Componentes en PCB

La PCB contiene:
- **ESP32-WROOM-32** (38 pines)
- **TL072** buffer (DIP-8)
- **Divisor resistivo** 2kΩ + 1kΩ
- **Bornera** entrada 5V/GND
- **Espadines** conexión Nextion
- **Conector BNC** salida analógica
- **LED RGB** indicador de estado

### 6.2 Buffer TL072

**Justificación TL072 vs MCP6002:**

| Característica | MCP6002 | TL072 |
|----------------|---------|-------|
| Slew Rate | 0.6 V/µs | **13 V/µs** |
| GBW | 1 MHz | **3 MHz** |
| Ruido | 29 nV/√Hz | **18 nV/√Hz** |
| Consumo | 100 µA | 2.5 mA |

El TL072 ofrece mejor rendimiento para señales biológicas gracias a su entrada JFET de bajo ruido y mayor slew rate.

**Configuración Seguidor de Voltaje:**

```
                         VCC (+5V)
                            │
                     ┌──────┴──────┐
                     │   TL072     │
ESP32 GPIO25 ───────►│ IN+    OUT ├───┬───[100Ω]──► BNC
(DAC 0-3.3V)         │ IN- ◄──────┼───┘
                     └──────┬──────┘
                            │
                           GND
```

### 6.3 Divisor de Voltaje UART

```
Nextion TX (5V) ────┬────
                   ┌┴┐ R1 = 2kΩ
                   └┬┘
                    ├──────► ESP32 RX (1.67V)
                   ┌┴┐ R2 = 1kΩ
                   └┬┘
GND ────────────────┴────

V_out = 5V × 1kΩ / (2kΩ + 1kΩ) = 1.67V
```

---

## 7. Esquema Electrónico Completo

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                        ESQUEMA ELECTRÓNICO COMPLETO                          │
│                            BioSimulator Pro v2.0                             │
├──────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  USB 5V ──► TP4056+DW01 ──► [B+ B-] ──► BATERÍAS 2P 4400mAh                 │
│                    │                                                         │
│                   OUT+ ──► SWITCH ──► MT3608 ──► 5V                         │
│                                                   │                          │
│  ┌────────────────────────────────────────────────┼──────────────────────┐  │
│  │                      PCB PRINCIPAL             │                      │  │
│  │                                                │                      │  │
│  │  BORNERA 5V/GND ◄──────────────────────────────┘                      │  │
│  │       │                                                               │  │
│  │       ▼                                                               │  │
│  │  ESP32-WROOM-32                                                       │  │
│  │  ├─ GPIO25 (DAC) ──► TL072 IN+ ──► OUT ──[100Ω]──► BNC               │  │
│  │  ├─ GPIO17 (TX2) ──────────────────────────────────► Nextion RX      │  │
│  │  ├─ GPIO16 (RX2) ◄──[2kΩ]──┬──[1kΩ]──GND ◄───────── Nextion TX      │  │
│  │  ├─ GPIO4 ──[330Ω]──► LED R                                          │  │
│  │  ├─ GPIO5 ──[330Ω]──► LED G                                          │  │
│  │  └─ GPIO18 ─[330Ω]──► LED B                                          │  │
│  │                                                                       │  │
│  │  ESPADINES: 5V ─ GND ─ TX ─ RX ──► Cable 4 hilos ──► NEXTION 7"      │  │
│  │                                                                       │  │
│  └───────────────────────────────────────────────────────────────────────┘  │
│                                                                              │
└──────────────────────────────────────────────────────────────────────────────┘
```

---

## 8. Diseño Mecánico y Análisis Térmico

### 8.1 Carcasa - Diseño CAD

La carcasa fue diseñada en SolidWorks con las siguientes características:

| Parámetro | Valor |
|-----------|-------|
| Material | PETG (impresión 3D) |
| Dimensiones externas | ~180 × 120 × 45 mm |
| Espesor de pared | 2.5 mm |
| Ventilación | Orificios pasivos (patrón circular) |
| Acceso BNC | Lateral con grabado "DAC" |
| Montaje pantalla | Marco frontal con bisel naranja |

**Características de diseño:**
- Patrón de ventilación circular en la parte trasera
- Orificios de ventilación lateral tipo rejilla
- Acceso lateral para conector BNC
- Marco para pantalla Nextion 7"
- Grabado "BioSignalSimulatorPro" en frontal
- Tornillos de ensamblaje en esquinas

### 8.2 Análisis Térmico

#### 8.2.1 Potencias Disipadas

| Componente | P Disipada | Ubicación |
|------------|------------|-----------|
| MT3608 | 580 mW | Zona trasera |
| ESP32 | 400-900 mW | Centro PCB |
| TL072 | 25 mW | PCB |
| Nextion backlight | 2000 mW | Frontal |
| **TOTAL** | **3-3.5 W** | |

#### 8.2.2 Cálculo de Temperatura

```
┌─────────────────────────────────────────────────────────────────┐
│                    ANÁLISIS TÉRMICO                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Potencia total disipada: P_total ≈ 3.5 W                      │
│                                                                 │
│  Resistencia térmica carcasa PETG (estimada):                  │
│  R_th ≈ 15 °C/W (con ventilación pasiva)                       │
│                                                                 │
│  Elevación de temperatura:                                     │
│  ΔT = P × R_th = 3.5 W × 15 °C/W = 52.5 °C                    │
│                                                                 │
│  Temperatura interna máxima:                                   │
│  T_int = T_amb + ΔT = 25°C + 52.5°C = 77.5°C                  │
│                                                                 │
│  Límites de operación:                                         │
│  • ESP32: -40°C a +85°C ✓                                      │
│  • MT3608: -40°C a +85°C ✓                                     │
│  • TL072: 0°C a +70°C ⚠️ (margen reducido)                     │
│  • Nextion: 0°C a +50°C ⚠️ (requiere ventilación)             │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

#### 8.2.3 Estrategia de Ventilación

```
┌─────────────────────────────────────────────────────────────────┐
│                 VENTILACIÓN PASIVA                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  PARTE TRASERA:                                                │
│  • Patrón circular de orificios (visible en CAD)               │
│  • Área total: ~15 cm²                                         │
│  • Ubicación: sobre zona MT3608                                │
│                                                                 │
│  PARTE LATERAL:                                                │
│  • Rejilla de ventilación tipo persiana                        │
│  • Permite convección natural                                  │
│                                                                 │
│  FLUJO DE AIRE:                                                │
│  Entrada (lateral inferior) → MT3608 → Salida (trasera)       │
│                                                                 │
│  Con ventilación pasiva, R_th se reduce a ~10-12 °C/W         │
│  T_int_real ≈ 25°C + (3.5W × 12°C/W) = 67°C                   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

#### 8.2.4 Recomendaciones Térmicas

| Material Carcasa | Conductividad | Recomendación |
|------------------|---------------|---------------|
| PLA | 0.13 W/m·K | ❌ Baja resistencia térmica |
| ABS | 0.17 W/m·K | ✅ Buena opción |
| PETG | 0.29 W/m·K | ✅ **Seleccionado** |
| Aluminio | 205 W/m·K | ✅ Óptimo pero costoso |

---

## 9. Lista de Materiales (BOM)

### 9.1 Componentes Principales

| # | Componente | Cantidad | Precio | Subtotal |
|---|------------|----------|--------|----------|
| 1 | Nextion NX8048T070 7" | 1 | $95.00 | $95.00 |
| 2 | ESP32-WROOM-32 NodeMCU | 1 | $8.00 | $8.00 |
| 3 | Batería 18650 2200mAh Steren | 2 | $3.50 | $7.00 |
| 4 | TP4056 Micro-USB + DW01 | 1 | $1.80 | $1.80 |
| 5 | MT3608 Step-Up | 1 | $1.50 | $1.50 |
| 6 | TL072 DIP-8 | 1 | $0.50 | $0.50 |
| 7 | Portapilas 18650 doble | 1 | $1.50 | $1.50 |
| 8 | Switch deslizante SPST | 1 | $0.30 | $0.30 |
| 9 | Conector BNC hembra | 1 | $1.00 | $1.00 |

### 9.2 Componentes Pasivos

| # | Componente | Cantidad | Precio | Subtotal |
|---|------------|----------|--------|----------|
| 10 | Resistencia 330Ω 1/4W | 3 | $0.05 | $0.15 |
| 11 | Resistencia 2kΩ 1/4W | 1 | $0.05 | $0.05 |
| 12 | Resistencia 1kΩ 1/4W | 1 | $0.05 | $0.05 |
| 13 | Resistencia 100Ω 1/4W | 1 | $0.05 | $0.05 |
| 14 | LED RGB 5mm cátodo común | 1 | $0.50 | $0.50 |

### 9.3 Conectores y Mecánicos

| # | Componente | Cantidad | Precio | Subtotal |
|---|------------|----------|--------|----------|
| 15 | Bornera 2 posiciones | 1 | $0.30 | $0.30 |
| 16 | Header macho 4 pines | 1 | $0.20 | $0.20 |
| 17 | Cable dupont H-H 20cm | 10 | $0.10 | $1.00 |
| 18 | PCB perforada 7×9 cm | 1 | $1.00 | $1.00 |
| 19 | Filamento PETG (carcasa) | 100g | $3.00 | $3.00 |
| 20 | Tornillos M3 + separadores | 1 kit | $2.00 | $2.00 |

### 9.4 Resumen de Costos

| Categoría | Subtotal |
|-----------|----------|
| Componentes principales | $116.60 |
| Componentes pasivos | $0.80 |
| Conectores y mecánicos | $7.50 |
| **TOTAL** | **$124.90 USD** |

---

## 10. Manual de Usuario

### 10.1 Encendido y Apagado

1. **Encender:** Deslizar switch a ON, esperar 3 segundos
2. **Apagar:** Deslizar switch a OFF

### 10.2 Carga de Batería

⚠️ **IMPORTANTE: Apagar el dispositivo antes de cargar**

1. Switch en OFF
2. Conectar USB Micro-B
3. LED Rojo = Cargando
4. LED Azul = Completo (~5 horas)

### 10.3 Conexión WiFi (App Web)

1. Conectar al WiFi: **BioSimulator_Pro**
2. Password: **biosignal123**
3. Abrir navegador: **http://192.168.4.1**

### 10.4 Salida Analógica BNC

- Señal: 0V - 3.3V
- Impedancia: ~100Ω
- Uso: Conectar a osciloscopio (1V/div, AC coupling)

---

## 11. Limitaciones y Precauciones

### 11.1 Precauciones

| Precaución | Motivo |
|------------|--------|
| No usar mientras carga | Interferencia con detección fin de carga |
| No cortocircuitar BNC | Daño al TL072 |
| No exponer a humedad | Cortocircuitos |
| Usar cargador 5V/1A | Especificación TP4056 |

### 11.2 Limitaciones

| Limitación | Impacto |
|------------|---------|
| Sin power path | Requiere apagar para cargar |
| Autonomía 2.9-4.2h | Planificar sesiones |
| Salida 0-3.3V | Limitada por DAC ESP32 |

---

## 12. Referencias

### 12.1 Datasheets

1. **TP4056** - NanJing Top Power ASIC Corp.
2. **DW01** - Fortune Semiconductor Corp.
3. **MT3608** - Aerosemi Technology Co.
4. **TL072** - Texas Instruments
5. **ESP32** - Espressif Systems
6. **Nextion NX8048T070** - ITEAD Studio

### 12.2 Notas de Aplicación

1. **Microchip AN1149** - Li-Ion Battery Charger Design
2. **TI SLOA011** - Op Amp Circuit Collection
