# BioSimulator Pro - Diseño Electrónico de Hardware

**Versión:** 1.1.0  
**Fecha:** Diciembre 2024  
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
8. [Lista de Materiales (BOM)](#8-lista-de-materiales-bom)
9. [Manual de Usuario](#9-manual-de-usuario)
10. [Limitaciones y Precauciones](#10-limitaciones-y-precauciones)
11. [Mejoras Futuras](#11-mejoras-futuras)
12. [Referencias](#12-referencias)

---

## 1. Introducción y Objetivos

### 1.1 Propósito del Documento

Este documento presenta el diseño electrónico detallado del BioSimulator Pro, un simulador portátil de señales biológicas. Se incluyen los cálculos, justificaciones técnicas, criterios de diseño y selección de componentes necesarios para la implementación del hardware.

### 1.2 Objetivos del Diseño Electrónico

| Objetivo | Métrica | Criterio de Éxito |
|----------|---------|-------------------|
| Portabilidad | Autonomía | ≥ 2.5 horas de uso continuo |
| Seguridad | Protecciones | Sobrecarga, sobredescarga, cortocircuito |
| Costo | Presupuesto | < $150 USD total |
| Simplicidad | Componentes | Disponibles localmente |
| Compatibilidad | Voltajes | 5V para todos los módulos |

### 1.3 Normativas y Estándares Considerados

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
│                                                                 │
│  ┌──────────────┐                                              │
│  │ 1. ANÁLISIS  │  • Definir requisitos funcionales            │
│  │    DE        │  • Identificar restricciones                 │
│  │ REQUISITOS   │  • Establecer métricas de éxito              │
│  └──────┬───────┘                                              │
│         ▼                                                       │
│  ┌──────────────┐                                              │
│  │ 2. SELECCIÓN │  • Evaluar alternativas                      │
│  │    DE        │  • Comparar especificaciones                 │
│  │ COMPONENTES  │  • Verificar disponibilidad local            │
│  └──────┬───────┘                                              │
│         ▼                                                       │
│  ┌──────────────┐                                              │
│  │ 3. DISEÑO    │  • Calcular parámetros                       │
│  │    DE        │  • Dimensionar componentes                   │
│  │ CIRCUITOS    │  • Verificar márgenes de seguridad           │
│  └──────┬───────┘                                              │
│         ▼                                                       │
│  ┌──────────────┐                                              │
│  │ 4. VALIDACIÓN│  • Simular comportamiento                    │
│  │    Y         │  • Prototipar y medir                        │
│  │ VERIFICACIÓN │  • Documentar resultados                     │
│  └──────────────┘                                              │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 2.2 Criterios de Diseño

| Criterio | Descripción | Aplicación |
|----------|-------------|------------|
| **Simplicidad** | Minimizar componentes y complejidad | Usar módulos integrados (TP4056, MT3608) |
| **Disponibilidad** | Componentes accesibles localmente | Proveedores Ecuador (Novatronic, etc.) |
| **Seguridad** | Protección contra fallas | Módulo con protección integrada DW01 |
| **Eficiencia** | Maximizar autonomía | Selección de regulador con alta eficiencia |
| **Modularidad** | Facilitar mantenimiento | Conectores y módulos intercambiables |

---

## 3. Arquitectura del Sistema

### 3.1 Diagrama de Bloques General

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                           BIOSIMULATOR PRO v1.1                              │
│                        Arquitectura de Alimentación                          │
├──────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│   ┌───────────┐                                                              │
│   │  USB 5V   │                                                              │
│   │  Type-C   │                                                              │
│   └─────┬─────┘                                                              │
│         │                                                                    │
│         ▼                                                                    │
│   ┌─────────────────────────────┐       ┌─────────────────┐                 │
│   │   TP4056 + DW01 + 8205A     │       │    BATERÍAS     │                 │
│   │   (Cargador + Protección)   │       │   2× 18650 2P   │                 │
│   │                             │       │   3.7V 4400mAh  │                 │
│   │   IN+ ◄── USB 5V            │       │                 │                 │
│   │   IN- ◄── GND               │       │    ┌─────┐      │                 │
│   │                             │       │    │     │      │                 │
│   │   B+ ────────────────────────────────►   │ (+) │      │                 │
│   │   B- ────────────────────────────────►   │ (-) │      │                 │
│   │                             │       │    └──┬──┘      │                 │
│   │   OUT+ ─────────────────────┼───────┼──────►│         │                 │
│   │   OUT- ─────────────────────┼───────┼──────►│         │                 │
│   │                             │       └───────┼─────────┘                 │
│   └─────────────────────────────┘               │                           │
│                                                 │ 3.0V - 4.2V               │
│                                                 ▼                           │
│                                    ┌────────────────────────┐               │
│                                    │   SWITCH PRINCIPAL     │               │
│                                    │   (ON / OFF)           │               │
│                                    │   Deslizante SPST      │               │
│                                    └───────────┬────────────┘               │
│                                                │                            │
│                                                ▼                            │
│                                    ┌────────────────────────┐               │
│                                    │       MT3608           │               │
│                                    │      Step-Up           │               │
│                                    │    3.7V → 5V           │               │
│                                    │    η ≈ 88%             │               │
│                                    └───────────┬────────────┘               │
│                                                │ 5V                         │
│   ╔════════════════════════════════════════════▼═══════════════════════════╗│
│   ║                          PCB PRINCIPAL                                 ║│
│   ║                                                                        ║│
│   ║  ┌────────┐  ┌──────────┐  ┌────────┐  ┌────────┐  ┌────────┐        ║│
│   ║  │BORNERA │─►│  ESP32   │─►│MCP6002 │─►│  BNC   │  │LED RGB │        ║│
│   ║  │5V/GND  │  │ NodeMCU  │  │Buffer  │  │0-3.3V  │  │Estado  │        ║│
│   ║  └───┬────┘  └────┬─────┘  └────────┘  └────────┘  └────────┘        ║│
│   ║      │            │                                                   ║│
│   ║      │       ┌────┴────┐                                              ║│
│   ║      │       │DIVISOR  │ ◄── Protección RX ESP32                      ║│
│   ║      │       │1kΩ+2kΩ  │                                              ║│
│   ║      │       └─────────┘                                              ║│
│   ║  ┌───▼────┐                                                           ║│
│   ║  │ESPADIN │                                                           ║│
│   ║  │NEXTION │ ──► 5V, GND, TX, RX                                       ║│
│   ║  └───┬────┘                                                           ║│
│   ╚══════▼════════════════════════════════════════════════════════════════╝│
│          │ Cable 4 hilos                                                    │
│   ┌──────▼──────┐                                                           │
│   │   NEXTION   │                                                           │
│   │ NX8048T070  │                                                           │
│   │  7" 800×480 │                                                           │
│   └─────────────┘                                                           │
│                                                                              │
└──────────────────────────────────────────────────────────────────────────────┘
```

### 3.2 Especificaciones del Sistema

| Parámetro | Valor | Justificación |
|-----------|-------|---------------|
| Voltaje de batería | 3.7V nominal | Estándar Li-ion 18650 |
| Configuración | 2P (paralelo) | Mayor capacidad, sin balanceo |
| Capacidad | 4400 mAh | 2× 2200 mAh en paralelo |
| Voltaje operación | 5V | Compatible ESP32 y Nextion |
| Regulador | Step-Up MT3608 | Eficiencia 88%, bajo costo |
| **Autonomía típica** | **3.7 horas** | Calculada con 90% capacidad útil |

---

## 4. Sistema de Alimentación

### 4.1 Selección de Baterías

#### 4.1.1 Análisis de Alternativas

| Configuración | Voltaje | Capacidad | Regulador | Complejidad | Decisión |
|---------------|---------|-----------|-----------|-------------|----------|
| 1S (1 celda) | 3.7V | 2200 mAh | Step-Up | Baja | ❌ Poca autonomía |
| 2S (serie) | 7.4V | 2200 mAh | Buck | Media (balanceo) | ❌ Requiere BMS |
| **2P (paralelo)** | **3.7V** | **4400 mAh** | **Step-Up** | **Baja** | ✅ **Seleccionada** |

#### 4.1.2 Justificación de Configuración 2P

1. **Mayor capacidad:** 4400 mAh vs 2200 mAh (1S)
2. **Sin balanceo:** Las celdas en paralelo se auto-balancean
3. **Menor complejidad:** No requiere BMS especializado
4. **Costo reducido:** Módulo TP4056 estándar es suficiente
5. **Seguridad:** Menor voltaje nominal (3.7V vs 7.4V)

#### 4.1.3 Especificaciones de las Celdas 18650

| Parámetro | Por Celda | Pack 2P |
|-----------|-----------|---------|
| Modelo | 18650 Li-ion genérico | - |
| Voltaje nominal | 3.7V | 3.7V |
| Voltaje máximo | 4.2V | 4.2V |
| Voltaje mínimo | 3.0V | 3.0V |
| Capacidad nominal | 2200 mAh | **4400 mAh** |
| Energía | 8.14 Wh | **16.28 Wh** |
| Corriente máx. descarga | 2A | 4A |

#### 4.1.4 Diagrama de Conexión Pack 2P

```
┌─────────────────────────────────────────────────────────────────┐
│                      PACK BATERÍAS 2P                           │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│      ┌───────────────┐          ┌───────────────┐              │
│      │    18650      │          │    18650      │              │
│      │   CELDA 1     │          │   CELDA 2     │              │
│      │   2200mAh     │          │   2200mAh     │              │
│      │               │          │               │              │
│      │     (+)───────┼──────────┼───────(+)     │              │
│      └───────┬───────┘          └───────┬───────┘              │
│              │      CONEXIÓN            │                       │
│              │      PARALELO            │                       │
│      ┌───────┴───────┐          ┌───────┴───────┐              │
│      │     (-)───────┼──────────┼───────(-)     │              │
│      └───────────────┘          └───────────────┘              │
│              │                          │                       │
│              └──────────┬───────────────┘                       │
│                         │                                       │
│                    B+ ──┴── 3.7V nominal                        │
│                    B- ────── GND                                │
│                                                                 │
│   Resultado:                                                    │
│   • Voltaje: 3.7V (igual que 1 celda)                          │
│   • Capacidad: 4400 mAh (suma de capacidades)                  │
│   • Corriente máx: 4A (suma de corrientes)                     │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

> **PRECAUCIÓN:** Antes de conectar celdas en paralelo, verificar que ambas tengan voltaje similar (diferencia < 0.1V). Cargar por separado si es necesario.

### 4.2 Módulo Cargador TP4056 con Protección

#### 4.2.1 Selección del Módulo

**Modelo seleccionado:** TP4056 Type-C con DW01 + 8205A integrado

| Característica | Valor |
|----------------|-------|
| Interfaz entrada | USB Type-C |
| Voltaje entrada | 5V |
| Corriente de carga | 1000 mA (programable) |
| Voltaje de corte | 4.2V ±1% |
| Precisión | ±1.5% |
| Dimensiones | 2.6 × 1.7 cm |
| Precio (Ecuador) | $1.80 USD |

#### 4.2.2 Chips Integrados

**TP4056 - Controlador de Carga:**
- Algoritmo CC/CV (Corriente Constante / Voltaje Constante)
- Terminación automática cuando I < 100mA
- Indicadores LED (Rojo: cargando, Azul: completo)

**DW01 + 8205A - Circuito de Protección:**

| Protección | Umbral | Función |
|------------|--------|---------|
| Sobrecarga | > 4.25V | Desconecta carga |
| Sobredescarga | < 2.5V | Desconecta descarga |
| Sobrecorriente | > 3A | Desconecta circuito |
| Cortocircuito | Inmediato | Desconecta circuito |

#### 4.2.3 Algoritmo de Carga CC/CV

```
Voltaje (V)                          Corriente (mA)
    ▲                                     ▲
4.2 │            ┌─────────────      1000 │────────────┐
    │           /│                        │            │
    │          / │  Fase CV               │  Fase CC   │
    │         /  │                        │            │
3.0 │────────/   │                        │            └──────────┐
    │  Fase CC   │                        │       Fase CV         │
    │            │                        │                       │
    └────────────┴──────────► t           └───────────────────────┴──► t
              Transición                              I < 100mA
           (V_bat ≈ 4.2V)                          (Terminación)
```

**Fases de carga:**
1. **CC (Corriente Constante):** Carga a 1A hasta alcanzar 4.2V
2. **CV (Voltaje Constante):** Mantiene 4.2V, corriente decrece
3. **Terminación:** Cuando I < 100mA, carga completa

#### 4.2.4 Diagrama de Conexiones TP4056

```
┌─────────────────────────────────────────────────────────────────┐
│                    CONEXIONES TP4056                            │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│                    ┌─────────────────────┐                     │
│                    │      TP4056         │                     │
│                    │     Type-C          │                     │
│                    │                     │                     │
│   USB 5V ─────────►│ IN+            OUT+ │──────► Switch ON/OFF│
│                    │                     │                     │
│   GND ────────────►│ IN-            OUT- │──────► GND común    │
│                    │                     │                     │
│                    │ B+              B-  │                     │
│                    └──┬───────────────┬──┘                     │
│                       │               │                        │
│                       ▼               ▼                        │
│                   ┌───────────────────────┐                    │
│                   │       BATERÍAS        │                    │
│                   │      2P 4400mAh       │                    │
│                   │    (+)         (-)    │                    │
│                   └───────────────────────┘                    │
│                                                                 │
│   Nota: OUT+ y OUT- están conectados internamente a la         │
│         batería a través del circuito de protección DW01       │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 4.3 Switch Principal ON/OFF

#### 4.3.1 Especificaciones

| Parámetro | Valor |
|-----------|-------|
| Tipo | SPST deslizante |
| Corriente máxima | 3A |
| Voltaje máximo | 12V DC |
| Posiciones | 2 (ON / OFF) |
| Función | Cortar alimentación general |

#### 4.3.2 Ubicación en el Circuito

```
TP4056 OUT+ ────► [SWITCH ON/OFF] ────► MT3608 IN+

Posición ON:  Sistema encendido, batería alimenta MT3608
Posición OFF: Sistema apagado, batería aislada (solo carga)
```

### 4.4 Regulador Step-Up MT3608

#### 4.4.1 Justificación de Selección

| Criterio | MT3608 | XL6009 | Decisión |
|----------|--------|--------|----------|
| Eficiencia | 88-93% | 85-90% | ✅ MT3608 |
| Tamaño | Compacto | Grande | ✅ MT3608 |
| Costo | $1.50 | $2.50 | ✅ MT3608 |
| Corriente máx | 2A | 4A | Suficiente |
| Disponibilidad | Alta | Alta | Igual |

#### 4.4.2 Especificaciones MT3608

| Parámetro | Valor |
|-----------|-------|
| Voltaje entrada | 2V - 24V |
| Voltaje salida | 5V - 28V (ajustable) |
| Corriente máxima | 2A |
| Frecuencia switching | 1.2 MHz |
| Eficiencia máxima | 97% |
| Eficiencia @ 3.7V→5V, 0.7A | **88%** |
| Dimensiones | 3.6 × 1.7 cm |
| Precio (Ecuador) | $1.50 USD |

#### 4.4.3 Principio de Funcionamiento (Convertidor Boost)

```
┌─────────────────────────────────────────────────────────────────┐
│                 CONVERTIDOR BOOST (ELEVADOR)                    │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│                      L (Inductor 22µH)                          │
│   V_in ─────────────────ΩΩΩΩΩ──────────┬────────────────────   │
│   (3.7V)                               │                        │
│                                        │    D (Schottky)        │
│                             ┌──────────┴────►|────┬─────────   │
│                             │                     │             │
│                          ┌──┴──┐               ┌──┴──┐         │
│                          │ SW  │               │  C  │  V_out  │
│                          │MOSFET               │22µF │  (5V)   │
│                          └──┬──┘               └──┬──┘         │
│                             │                     │             │
│   GND ──────────────────────┴─────────────────────┴─────────   │
│                                                                 │
│   Ecuación: V_out = V_in / (1 - D)                             │
│   Donde D = duty cycle del MOSFET                              │
│                                                                 │
│   Para V_in = 3.7V, V_out = 5V:                                │
│   D = 1 - (V_in / V_out) = 1 - (3.7/5) = 0.26 = 26%           │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

#### 4.4.4 Curva de Eficiencia

```
Eficiencia (%)
     ▲
 95% │              ┌─────────────┐
 90% │         ┌────┘             └────┐
 85% │    ┌────┘                       └────┐
 80% │────┘                                 └────
     └────────────────────────────────────────────► I_out (A)
         0.1   0.3   0.5   0.7   0.9   1.1   1.3

Valores para V_in = 3.7V, V_out = 5V:
• @ 0.3A: η ≈ 93%
• @ 0.5A: η ≈ 91%
• @ 0.7A: η ≈ 88%  ◄── Punto de operación típico
• @ 1.0A: η ≈ 85%
```

> **IMPORTANTE:** Ajustar el potenciómetro del MT3608 a exactamente **5.0V ±0.1V** antes de conectar cualquier carga. Verificar con multímetro.

---

## 5. Cálculos de Consumo y Autonomía

### 5.1 Desglose Detallado de Consumos

#### 5.1.1 Consumo del ESP32 NodeMCU (38 pines)

```
┌─────────────────────────────────────────────────────────────────┐
│                    CONSUMO ESP32 NodeMCU                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Modo de operación        │ Corriente @ 5V │ Potencia          │
│  ─────────────────────────┼────────────────┼──────────────────  │
│  Deep Sleep               │     10 µA      │   0.05 mW         │
│  Light Sleep              │    800 µA      │   4.0 mW          │
│  Modem Sleep (CPU activo) │     20 mA      │  100 mW           │
│  WiFi RX                  │    100 mA      │  500 mW           │
│  WiFi TX (máx)            │    350 mA      │ 1750 mW           │
│  WiFi TX (típico)         │    150 mA      │  750 mW           │
│                                                                 │
│  Consumo típico (WiFi moderado): 150 mA @ 5V = 750 mW          │
│  Consumo pico (WiFi TX máx):     350 mA @ 5V = 1750 mW         │
│                                                                 │
│  Fuente: Espressif ESP32 Datasheet v4.0                        │
└─────────────────────────────────────────────────────────────────┘
```

#### 5.1.2 Consumo de Pantalla Nextion NX8048T070 (7")

```
┌─────────────────────────────────────────────────────────────────┐
│                 CONSUMO NEXTION 7" NX8048T070                   │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Componente interno       │ Corriente @ 5V │ Potencia          │
│  ─────────────────────────┼────────────────┼──────────────────  │
│  Backlight LED (100%)     │    400 mA      │ 2000 mW           │
│  Backlight LED (50%)      │    200 mA      │ 1000 mW           │
│  MCU + Driver TFT         │     80 mA      │  400 mW           │
│  Pantalla táctil resist.  │     30 mA      │  150 mW           │
│                                                                 │
│  TOTAL típico (backlight 100%):  510 mA @ 5V = 2550 mW         │
│  TOTAL pico (arranque):          600 mA @ 5V = 3000 mW         │
│                                                                 │
│  Fuente: ITEAD Nextion NX8048T070 Datasheet                    │
└─────────────────────────────────────────────────────────────────┘
```

#### 5.1.3 Consumo del Buffer MCP6002

```
┌─────────────────────────────────────────────────────────────────┐
│                    CONSUMO MCP6002                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Parámetro                │ Valor          │ Condición         │
│  ─────────────────────────┼────────────────┼──────────────────  │
│  Corriente quiescente     │    100 µA      │ Sin carga         │
│  Corriente con carga      │    100 µA      │ Carga alta Z      │
│  Corriente de salida máx  │    ±25 mA      │ Cortocircuito     │
│                                                                 │
│  Consumo típico: 0.1 mA @ 5V = 0.5 mW                          │
│                                                                 │
│  Fuente: Microchip MCP6002 Datasheet                           │
└─────────────────────────────────────────────────────────────────┘
```

#### 5.1.4 Consumo del LED RGB (Cátodo Común)

```
┌─────────────────────────────────────────────────────────────────┐
│                    CONSUMO LED RGB                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Configuración:                                                │
│  • LED RGB 5mm cátodo común                                    │
│  • Cátodo común → GND de la PCB                                │
│  • Ánodo R → GPIO4 (a través de R = 330Ω)                      │
│  • Ánodo G → GPIO5 (a través de R = 330Ω)                      │
│  • Ánodo B → GPIO18 (a través de R = 330Ω)                     │
│                                                                 │
│  Cálculo por color:                                            │
│  ───────────────────                                           │
│  I_LED = (V_GPIO - V_F) / R                                    │
│                                                                 │
│  Para LED Rojo (V_F = 2.0V):                                   │
│  I_R = (3.3V - 2.0V) / 330Ω = 3.9 mA                          │
│                                                                 │
│  Para LED Verde (V_F = 3.0V):                                  │
│  I_G = (3.3V - 3.0V) / 330Ω = 0.9 mA                          │
│                                                                 │
│  Para LED Azul (V_F = 3.0V):                                   │
│  I_B = (3.3V - 3.0V) / 330Ω = 0.9 mA                          │
│                                                                 │
│  Consumo típico (1 color): ~4 mA @ 3.3V ≈ 13 mW               │
│  Consumo máximo (3 colores): ~6 mA @ 3.3V ≈ 20 mW             │
│                                                                 │
│  Potencia en resistencia:                                      │
│  P_R = I² × R = (4mA)² × 330Ω = 5.3 mW                        │
│                                                                 │
│  TOTAL LED + Resistencia: ~18 mW típico                        │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

#### 5.1.5 Consumo del Divisor de Voltaje UART

```
┌─────────────────────────────────────────────────────────────────┐
│                 CONSUMO DIVISOR VOLTAJE                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Circuito: R1 = 1kΩ, R2 = 2kΩ                                  │
│                                                                 │
│  Nextion TX (5V) ────┬────                                     │
│                     ┌┴┐                                         │
│                     │ │ R1 = 1kΩ                                │
│                     └┬┘                                         │
│                      ├──────► ESP32 RX (3.3V)                  │
│                     ┌┴┐                                         │
│                     │ │ R2 = 2kΩ                                │
│                     └┬┘                                         │
│                      │                                          │
│  GND ────────────────┴────                                     │
│                                                                 │
│  Cálculo:                                                      │
│  I_divisor = V_in / (R1 + R2)                                  │
│  I_divisor = 5V / (1kΩ + 2kΩ) = 5V / 3kΩ = 1.67 mA            │
│                                                                 │
│  Nota: Solo consume cuando Nextion TX está en HIGH (5V)        │
│  Promedio considerando duty cycle ~50%: 0.83 mA                │
│                                                                 │
│  P_divisor = V × I = 5V × 0.83mA = 4.2 mW (promedio)          │
│  P_divisor_max = 5V × 1.67mA = 8.35 mW (TX HIGH)              │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

#### 5.1.6 Pérdidas en Cables y Conectores

```
┌─────────────────────────────────────────────────────────────────┐
│              PÉRDIDAS EN CABLES Y CONECTORES                    │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Elemento                 │ Resistencia    │ Pérdida @ 1A      │
│  ─────────────────────────┼────────────────┼──────────────────  │
│  Cable Nextion (4 hilos)  │   ~0.05Ω       │   50 mV           │
│  Bornera entrada          │   ~0.01Ω       │   10 mV           │
│  Conectores PCB           │   ~0.02Ω       │   20 mV           │
│  Pistas PCB               │   ~0.02Ω       │   20 mV           │
│  ─────────────────────────┼────────────────┼──────────────────  │
│  TOTAL                    │   ~0.10Ω       │  100 mV           │
│                                                                 │
│  Pérdida de potencia:                                          │
│  P_pérdida = I² × R = (0.7A)² × 0.1Ω = 49 mW ≈ 50 mW          │
│                                                                 │
│  Esto representa ~1.5% de pérdida adicional                    │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 5.2 Tabla Resumen de Consumos

| # | Componente | I Típica | I Pico | P Típica | P Pico | Fuente |
|---|------------|----------|--------|----------|--------|--------|
| 1 | ESP32 NodeMCU 38 pines | 150 mA | 350 mA | 750 mW | 1750 mW | Datasheet |
| 2 | Nextion NX8048T070 7" | 510 mA | 600 mA | 2550 mW | 3000 mW | Datasheet |
| 3 | MCP6002 Buffer | 0.1 mA | 0.1 mA | 0.5 mW | 0.5 mW | Datasheet |
| 4 | LED RGB (1 color) | 4 mA | 6 mA | 18 mW | 25 mW | Calculado |
| 5 | Divisor UART (prom.) | 0.8 mA | 1.7 mA | 4 mW | 8.5 mW | Calculado |
| 6 | Pérdidas cables/PCB | 10 mA | 15 mA | 50 mW | 75 mW | Estimado |
| | **SUBTOTAL @ 5V** | **675 mA** | **973 mA** | **3373 mW** | **4859 mW** | |

### 5.3 Análisis de Eficiencia y Pérdidas por Elevación de Voltaje

#### 5.3.1 Pérdidas en el Convertidor Step-Up MT3608

```
┌─────────────────────────────────────────────────────────────────┐
│           ANÁLISIS DE PÉRDIDAS MT3608 (3.7V → 5V)              │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  El convertidor boost tiene pérdidas en varios componentes:    │
│                                                                 │
│  1. PÉRDIDAS EN EL MOSFET DE CONMUTACIÓN                       │
│     ─────────────────────────────────────                      │
│     • Pérdidas de conducción: P_cond = I²_rms × R_DS(on)       │
│     • R_DS(on) típico MT3608: ~0.1Ω                            │
│     • I_rms ≈ 1.2A (para I_out = 0.7A)                         │
│     • P_cond = (1.2)² × 0.1 = 144 mW                           │
│                                                                 │
│     • Pérdidas de conmutación: P_sw = 0.5 × V × I × (tr+tf) × f│
│     • f = 1.2 MHz, tr+tf ≈ 20ns                                │
│     • P_sw ≈ 0.5 × 5V × 1.2A × 20ns × 1.2MHz = 72 mW          │
│                                                                 │
│  2. PÉRDIDAS EN EL DIODO SCHOTTKY                              │
│     ─────────────────────────────────                          │
│     • V_F (diodo Schottky) ≈ 0.3V                              │
│     • P_diodo = V_F × I_out = 0.3V × 0.7A = 210 mW            │
│                                                                 │
│  3. PÉRDIDAS EN EL INDUCTOR                                    │
│     ────────────────────────                                   │
│     • DCR (resistencia DC) ≈ 0.1Ω                              │
│     • P_inductor = I²_rms × DCR = (1.2)² × 0.1 = 144 mW       │
│                                                                 │
│  4. PÉRDIDAS EN CAPACITORES (ESR)                              │
│     ─────────────────────────────                              │
│     • ESR típico ≈ 0.05Ω                                       │
│     • P_cap ≈ 10 mW (despreciable)                             │
│                                                                 │
│  TOTAL PÉRDIDAS INTERNAS MT3608:                               │
│  P_pérdidas = 144 + 72 + 210 + 144 + 10 = 580 mW              │
│                                                                 │
│  EFICIENCIA CALCULADA:                                         │
│  η = P_out / (P_out + P_pérdidas)                              │
│  η = 3373 mW / (3373 + 580) mW = 85.3%                        │
│                                                                 │
│  Eficiencia práctica medida: 88% (datasheet)                   │
│  Usaremos η = 88% para cálculos conservadores                  │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

#### 5.3.2 Cadena Completa de Eficiencia

```
┌─────────────────────────────────────────────────────────────────┐
│              CADENA DE EFICIENCIA DEL SISTEMA                   │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  BATERÍA        MT3608         CABLES/PCB        SISTEMA       │
│  3.7V    ───►   Step-Up   ───►   Pérdidas   ───►   5V          │
│                 η = 88%          η = 98.5%                      │
│                                                                 │
│  Eficiencia total:                                             │
│  η_total = η_MT3608 × η_cables                                 │
│  η_total = 0.88 × 0.985 = 0.867 ≈ 86.7%                       │
│                                                                 │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  EFICIENCIA TOTAL DEL SISTEMA: 86.7%                    │   │
│  │  PÉRDIDAS TOTALES: 13.3%                                │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                 │
│  Desglose de pérdidas para P_sistema = 3.37 W:                 │
│  • Pérdidas MT3608: 3.37W × (1/0.88 - 1) = 459 mW             │
│  • Pérdidas cables: 3.37W × 0.015 = 51 mW                      │
│  • TOTAL pérdidas: 510 mW                                      │
│                                                                 │
│  Potencia desde batería:                                       │
│  P_batería = P_sistema / η_total                               │
│  P_batería = 3.37 W / 0.867 = 3.89 W                          │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

#### 5.3.3 Corriente desde la Batería

```
┌─────────────────────────────────────────────────────────────────┐
│              CORRIENTE DESDE LA BATERÍA                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ESCENARIO TÍPICO (WiFi moderado):                             │
│  ─────────────────────────────────                             │
│  P_sistema = 3.37 W                                            │
│  P_batería = 3.37 W / 0.867 = 3.89 W                          │
│                                                                 │
│  I_batería = P_batería / V_batería                             │
│  I_batería = 3.89 W / 3.7V = 1.05 A                           │
│                                                                 │
│  ESCENARIO PICO (WiFi TX máximo + arranque Nextion):           │
│  ───────────────────────────────────────────────               │
│  P_sistema_pico = 4.86 W                                       │
│  P_batería_pico = 4.86 W / 0.867 = 5.61 W                     │
│                                                                 │
│  I_batería_pico = 5.61 W / 3.7V = 1.52 A                      │
│                                                                 │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  CORRIENTE TÍPICA DESDE BATERÍA: 1.05 A                 │   │
│  │  CORRIENTE PICO DESDE BATERÍA:   1.52 A                 │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 5.4 Cálculo de Autonomía

#### 5.4.1 Fórmula y Factor de Confianza

```
                    Capacidad_útil
Autonomía (h) = ─────────────────────
                  Corriente_consumo

Donde:
• Capacidad_nominal = 4400 mAh (pack 2P)
• Factor_confianza = 90% (considera degradación, temperatura, tolerancias)
• Capacidad_útil = 4400 mAh × 0.90 = 3960 mAh
```

#### 5.4.2 Cálculos Detallados

```
┌─────────────────────────────────────────────────────────────────┐
│                    CÁLCULO DE AUTONOMÍA                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Capacidad útil: 3960 mAh (90% de 4400 mAh)                    │
│                                                                 │
│  ESCENARIO TÍPICO:                                             │
│  ─────────────────                                             │
│  I_batería = 1.05 A                                            │
│  T = 3960 mAh / 1050 mA = 3.77 horas ≈ 3.8 h                  │
│                                                                 │
│  ESCENARIO INTENSIVO:                                          │
│  ─────────────────────                                         │
│  I_batería = 1.52 A                                            │
│  T = 3960 mAh / 1520 mA = 2.61 horas ≈ 2.6 h                  │
│                                                                 │
│  ESCENARIO PROMEDIO:                                           │
│  ────────────────────                                          │
│  I_promedio = (1.05 + 1.52) / 2 = 1.29 A                       │
│  T = 3960 mAh / 1290 mA = 3.07 horas ≈ 3.1 h                  │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 5.5 Resumen de Autonomía

| Escenario | Consumo @ 5V | Consumo Batería | Autonomía |
|-----------|--------------|-----------------|-----------|
| **Típico** | 675 mA | 1.05 A | **3.8 h** |
| **Intensivo** | 973 mA | 1.52 A | **2.6 h** |
| **Promedio** | 800 mA | 1.29 A | **3.1 h** |

### 5.6 Tiempo de Carga

```
Capacidad del pack: 4400 mAh
Corriente de carga TP4056: 1000 mA

Fase CC (Corriente Constante) - 0% a ~80%:
T_CC = (4400 mAh × 0.80) / 1000 mA = 3.52 h

Fase CV (Voltaje Constante) - ~80% a 100%:
T_CV ≈ T_CC × 0.40 = 1.41 h

TIEMPO TOTAL DE CARGA: ~5 horas (0% → 100%)
```

### 5.7 Verificación de Márgenes de Seguridad

```
┌─────────────────────────────────────────────────────────────────┐
│              VERIFICACIÓN DE CORRIENTES MÁXIMAS                 │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Componente              │ Límite  │ Uso Pico │ Margen         │
│  ────────────────────────┼─────────┼──────────┼───────────────  │
│  Pack 2P (descarga)      │  4.0 A  │  1.52 A  │ 2.6× ✓         │
│  Protección DW01         │  3.0 A  │  1.52 A  │ 2.0× ✓         │
│  MT3608 (salida)         │  2.0 A  │  0.97 A  │ 2.1× ✓         │
│  MT3608 (entrada)        │  2.0 A  │  1.52 A  │ 1.3× ✓         │
│                                                                 │
│  Todos los márgenes son ≥ 1.3×, diseño seguro ✓                │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 6. PCB Principal y Circuitos

### 6.1 Componentes en PCB

```
╔═══════════════════════════════════════════════════════════════════════════════╗
║                              PCB PRINCIPAL                                    ║
║                           (Vista Superior)                                    ║
╠═══════════════════════════════════════════════════════════════════════════════╣
║                                                                               ║
║   ┌───────────────┐                              ┌───────────────┐           ║
║   │   BORNERA     │                              │    LED RGB    │           ║
║   │   ENTRADA     │                              │   Cátodo      │           ║
║   │   5V   GND    │                              │   Común       │           ║
║   │   ●     ●     │                              │  [R  G  B  C] │           ║
║   └───────────────┘                              └───────────────┘           ║
║          │                                              │                     ║
║          │                                         3× 330Ω                    ║
║          ▼                                              │                     ║
║   ┌─────────────────────────────────────────────────────┼─────────────┐      ║
║   │                                                     │             │      ║
║   │                   ESP32 NodeMCU                     │             │      ║
║   │                   (38 pines)                        │             │      ║
║   │                                                     │             │      ║
║   │    VIN ●────────────────────────────────────────────┘             │      ║
║   │    GND ●                                                          │      ║
║   │                                                                   │      ║
║   │    GPIO25 (DAC) ●──────────────────┐                              │      ║
║   │    GPIO17 (TX2) ●──────────────────┼──────────────────────────►   │      ║
║   │    GPIO16 (RX2) ●◄─────────────────┼───┐                          │      ║
║   │    GPIO4  (LED_R)●─────────────────┼───┼──────────────────────►   │      ║
║   │    GPIO5  (LED_G)●─────────────────┼───┼──────────────────────►   │      ║
║   │    GPIO18 (LED_B)●─────────────────┼───┼──────────────────────►   │      ║
║   │                                    │   │                          │      ║
║   └────────────────────────────────────┼───┼──────────────────────────┘      ║
║                                        │   │                                  ║
║   ┌───────────────┐              ┌─────┼───▼───────────┐                     ║
║   │   MCP6002     │              │  DIVISOR VOLTAJE   │                     ║
║   │   (DIP-8)     │              │                    │                     ║
║   │               │              │  Nextion TX (5V)   │                     ║
║   │  IN+ ●────────┘              │       │            │                     ║
║   │  OUT ●────────────┐          │      ┌┴┐ R1=1kΩ    │                     ║
║   │  VCC ●──► 5V      │          │      └┬┘           │                     ║
║   │  GND ●──► GND     │          │       ├──► ESP RX  │                     ║
║   └───────────────┘   │          │      ┌┴┐ R2=2kΩ    │                     ║
║                       │          │      └┬┘           │                     ║
║   ┌───────────────┐   │          │       │            │                     ║
║   │     BNC       │◄──┘          │      GND           │                     ║
║   │    Output     │              └────────────────────┘                     ║
║   │   0-3.3V      │                                                          ║
║   └───────────────┘                                                          ║
║                                                                               ║
║   ┌───────────────────────────────────────────────────────────────────────┐  ║
║   │                        ESPADINES NEXTION                              │  ║
║   │    5V ●      GND ●      TX(→RX) ●      RX(←divisor) ●                │  ║
║   └───────────────────────────────────────────────────────────────────────┘  ║
║                                                                               ║
╚═══════════════════════════════════════════════════════════════════════════════╝
```

### 6.2 Circuito Buffer MCP6002

#### 6.2.1 Justificación del Diseño

**¿Por qué se necesita un buffer para la salida analógica?**

El DAC interno del ESP32 tiene limitaciones que hacen necesario un buffer:

```
┌─────────────────────────────────────────────────────────────────┐
│        LIMITACIONES DEL DAC ESP32 (GPIO25/GPIO26)              │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Parámetro              │ Valor DAC ESP32  │ Problema          │
│  ───────────────────────┼──────────────────┼─────────────────  │
│  Impedancia de salida   │ ~1 kΩ            │ Caída de voltaje  │
│  Corriente máxima       │ ~12 mA           │ No puede manejar  │
│                         │                  │ cargas bajas      │
│  Capacidad de carga     │ ~100 pF          │ Distorsión con    │
│                         │                  │ cables largos     │
│                                                                 │
│  PROBLEMA: Si conectamos directamente un osciloscopio          │
│  (impedancia 1MΩ || 20pF) funciona, pero con cargas de         │
│  menor impedancia la señal se degrada.                         │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

**Solución: Buffer con Op-Amp en configuración seguidor de voltaje**

| Característica | Sin Buffer | Con Buffer MCP6002 |
|----------------|------------|-------------------|
| Impedancia salida | ~1 kΩ | ~100 Ω (con R serie) |
| Corriente máxima | 12 mA | 25 mA |
| Protección DAC | No | Sí (aislado) |
| Carga capacitiva | Limitada | Mejor manejo |

#### 6.2.2 Especificaciones MCP6002

| Parámetro | Valor |
|-----------|-------|
| Tipo | Op-Amp Rail-to-Rail I/O |
| Alimentación | 1.8V - 6V |
| Consumo | 100 µA típico |
| Slew Rate | 0.6 V/µs |
| GBW (Ganancia-Ancho de Banda) | 1 MHz |
| Impedancia de entrada | 10¹³ Ω |
| Corriente de salida | ±25 mA |
| Encapsulado | DIP-8 (2 op-amps) |
| Precio (Ecuador) | $0.80 USD |

#### 6.2.3 Configuración Seguidor de Voltaje (Buffer)

```
┌─────────────────────────────────────────────────────────────────┐
│           OP-AMP EN CONFIGURACIÓN SEGUIDOR DE VOLTAJE          │
│                    (Voltage Follower / Unity Gain Buffer)       │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│                              VCC (+5V)                          │
│                                 │                               │
│                                 │ Pin 8                         │
│                          ┌──────┴──────┐                       │
│                          │             │                        │
│                          │   MCP6002   │                        │
│                          │   (1/2)     │                        │
│                          │             │                        │
│   ESP32 GPIO25 ─────────►│3 IN+    OUT 1│───┬───[100Ω]──► BNC  │
│   (DAC 0-3.3V)           │             │   │                   │
│                          │2 IN-       │◄──┘                    │
│                          │             │                        │
│                          └──────┬──────┘                       │
│                                 │ Pin 4                         │
│                                 │                               │
│                                GND                              │
│                                                                 │
│   ┌─────────────────────────────────────────────────────────┐  │
│   │  ANÁLISIS DEL CIRCUITO:                                 │  │
│   │                                                         │  │
│   │  • Realimentación 100% negativa (IN- conectado a OUT)  │  │
│   │  • Ganancia: A = 1 + (R_f / R_in) = 1 + (0/∞) = 1      │  │
│   │  • V_out = V_in (seguidor perfecto)                    │  │
│   │  • Impedancia entrada: ~10¹³ Ω (no carga al DAC)       │  │
│   │  • Impedancia salida: ~100 Ω (con R serie)             │  │
│   │                                                         │  │
│   └─────────────────────────────────────────────────────────┘  │
│                                                                 │
│   PINOUT MCP6002 (DIP-8):                                      │
│   ┌────────────────────┐                                       │
│   │ 1  OUT_A    VCC  8 │                                       │
│   │ 2  IN-_A    OUT_B 7│                                       │
│   │ 3  IN+_A    IN-_B 6│                                       │
│   │ 4  GND      IN+_B 5│                                       │
│   └────────────────────┘                                       │
│   Usamos solo el Op-Amp A (pines 1,2,3)                        │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

#### 6.2.4 ¿Por qué el Seguidor de Voltaje es Suficiente?

```
┌─────────────────────────────────────────────────────────────────┐
│        JUSTIFICACIÓN DE LA CONFIGURACIÓN SEGUIDOR              │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  1. NO SE NECESITA AMPLIFICACIÓN:                              │
│     • El DAC del ESP32 produce 0-3.3V                          │
│     • El rango 0-3.3V es adecuado para osciloscopios           │
│     • Las señales biológicas simuladas están normalizadas      │
│                                                                 │
│  2. SE NECESITA AISLAMIENTO (BUFFERING):                       │
│     • Proteger el DAC de cargas externas                       │
│     • Evitar que corrientes externas afecten al ESP32          │
│     • Manejar cables BNC largos (capacitancia)                 │
│                                                                 │
│  3. VENTAJAS DEL SEGUIDOR:                                     │
│     • Ganancia = 1 (sin distorsión por ganancia)               │
│     • Sin resistencias de realimentación (menos ruido)         │
│     • Máximo ancho de banda (GBW completo = 1 MHz)             │
│     • Circuito más simple y confiable                          │
│                                                                 │
│  4. ALTERNATIVAS DESCARTADAS:                                  │
│     • Amplificador inversor: Invierte la señal (indeseable)    │
│     • Amplificador no-inversor: Innecesario si G=1             │
│     • Amplificador diferencial: Complejidad innecesaria        │
│                                                                 │
│  CONCLUSIÓN: El seguidor de voltaje es la solución óptima     │
│  para este caso de uso específico.                             │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

#### 6.2.5 Cálculo de Resistencia de Salida

```
Propósito: Proteger el op-amp y limitar corriente en cortocircuito

R_serie = 100Ω

Corriente máxima en cortocircuito:
I_max = V_out_max / R_serie = 3.3V / 100Ω = 33 mA

El MCP6002 puede entregar ±25mA, pero con la resistencia serie:
• Se limita la corriente a 33 mA máximo
• Se protege el op-amp de daños por cortocircuito
• Se mejora la estabilidad con cargas capacitivas
```

### 6.3 Divisor de Voltaje UART

#### 6.3.1 Problema a Resolver

```
Nextion TX: 5V (nivel lógico TTL)
ESP32 RX: 3.3V máximo (nivel lógico CMOS)

Sin protección: 5V en GPIO del ESP32 puede dañarlo
Solución: Divisor resistivo para reducir 5V → 3.3V
```

#### 6.3.2 Cálculo del Divisor

```
┌─────────────────────────────────────────────────────────────────┐
│                 DIVISOR DE VOLTAJE                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   Nextion TX ────┬────                                         │
│   (5V)           │                                              │
│                 ┌┴┐                                             │
│                 │ │ R1 = 1kΩ                                    │
│                 └┬┘                                             │
│                  ├──────────► ESP32 RX (GPIO16)                │
│                 ┌┴┐                                             │
│                 │ │ R2 = 2kΩ                                    │
│                 └┬┘                                             │
│                  │                                              │
│   GND ───────────┴────                                         │
│                                                                 │
│   Fórmula:                                                     │
│   V_out = V_in × R2 / (R1 + R2)                                │
│   V_out = 5V × 2kΩ / (1kΩ + 2kΩ)                               │
│   V_out = 5V × 2/3 = 3.33V ✓                                   │
│                                                                 │
│   Corriente por el divisor:                                    │
│   I = V_in / (R1 + R2) = 5V / 3kΩ = 1.67 mA                    │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 6.4 LED RGB Indicador

#### 6.4.1 Especificaciones

| Parámetro | Valor |
|-----------|-------|
| Tipo | RGB 5mm **cátodo común** |
| Corriente por color | ~4 mA (limitada por R y V_GPIO) |
| Voltaje directo (V_F) | ~2V (rojo), ~3V (verde/azul) |
| Resistencia limitadora | 330Ω por cada color |

#### 6.4.2 Configuración de Conexiones

```
┌─────────────────────────────────────────────────────────────────┐
│                 CONEXIÓN LED RGB CÁTODO COMÚN                   │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Configuración:                                                │
│  • Ánodo R → Resistencia 330Ω → GPIO4                          │
│  • Ánodo G → Resistencia 330Ω → GPIO5                          │
│  • Ánodo B → Resistencia 330Ω → GPIO18                         │
│  • Cátodo común → GND de la PCB                                │
│                                                                 │
│  Lógica: GPIO HIGH = LED encendido                             │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

#### 6.4.3 Cálculo de Resistencias

```
Los GPIOs del ESP32 operan a 3.3V (no 5V)

Para LED Rojo (V_F ≈ 2.0V):
I_R = (V_GPIO - V_F) / R
I_R = (3.3V - 2.0V) / 330Ω = 3.9 mA ≈ 4 mA

Para LED Verde (V_F ≈ 3.0V):
I_G = (3.3V - 3.0V) / 330Ω = 0.9 mA ≈ 1 mA

Para LED Azul (V_F ≈ 3.0V):
I_B = (3.3V - 3.0V) / 330Ω = 0.9 mA ≈ 1 mA

Nota: Verde y azul tendrán menor brillo que rojo debido a
que V_F está cerca de V_GPIO. Esto es aceptable para indicador.

Corriente máxima GPIO ESP32: 12 mA (estamos muy por debajo) ✓
```

#### 6.4.4 Estados del LED

| Estado | Color | GPIO4 | GPIO5 | GPIO18 | Significado |
|--------|-------|-------|-------|--------|-------------|
| Inactivo | Rojo | HIGH | LOW | LOW | Sistema listo |
| Generando | Verde | LOW | HIGH | LOW | Señal activa |
| Error/WiFi | Azul | LOW | LOW | HIGH | Conectando/Error |

#### 6.4.5 Circuito LED RGB (Cátodo Común a GND)

```
┌─────────────────────────────────────────────────────────────────┐
│              CIRCUITO LED RGB - CÁTODO COMÚN                    │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   GPIO4 (3.3V) ────[330Ω]────┬────► Ánodo R                    │
│   (LED_R)                    │                                  │
│                              │      ┌─────────┐                │
│   GPIO5 (3.3V) ────[330Ω]────┼────► │ Ánodo G │                │
│   (LED_G)                    │      │         │                │
│                              │      │   LED   │                │
│   GPIO18 (3.3V) ───[330Ω]────┼────► │ Ánodo B │                │
│   (LED_B)                    │      │   RGB   │                │
│                              │      │         │                │
│                              │      │ Cátodo  │                │
│                              │      │ Común   │                │
│                              │      └────┬────┘                │
│                              │           │                      │
│                              │           │                      │
│   GND (PCB) ─────────────────┴───────────┘                     │
│                                                                 │
│                                                                 │
│   Vista del LED RGB 5mm (4 pines):                             │
│                                                                 │
│        ┌───┬───┬───┬───┐                                       │
│        │ R │ C │ G │ B │                                       │
│        └─┬─┴─┬─┴─┬─┴─┬─┘                                       │
│          │   │   │   │                                          │
│          │   │   │   └─► Azul (pin más corto después de C)     │
│          │   │   └─────► Verde                                  │
│          │   └─────────► Cátodo común (pin más largo)          │
│          └─────────────► Rojo                                   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 6.5 Conexiones UART ESP32 ↔ Nextion

```
┌─────────────────────────────────────────────────────────────────┐
│                 CONEXIÓN UART                                   │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   ESP32                              Nextion NX8048T070         │
│   ══════                             ═════════════════          │
│                                                                 │
│   GPIO17 (TX2) ─────────────────────► RX (entrada 5V tolerante)│
│   (3.3V)                                                        │
│                                                                 │
│   GPIO16 (RX2) ◄───[DIVISOR]─────── TX (salida 5V)             │
│   (3.3V max)       1kΩ + 2kΩ                                    │
│                                                                 │
│   GND ──────────────────────────────► GND                       │
│                                                                 │
│   Configuración UART:                                          │
│   • Baudrate: 115200                                           │
│   • Data bits: 8                                               │
│   • Stop bits: 1                                               │
│   • Parity: None                                               │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 7. Esquema Electrónico Completo

### 7.1 Diagrama de Conexiones General

```
┌──────────────────────────────────────────────────────────────────────────────────────┐
│                        ESQUEMA ELECTRÓNICO COMPLETO                                  │
│                            BioSimulator Pro v1.1                                     │
├──────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│  ┌─────────────┐                                                                    │
│  │  USB 5V     │                                                                    │
│  │  Type-C     │                                                                    │
│  └──────┬──────┘                                                                    │
│         │                                                                           │
│         ▼                                                                           │
│  ┌──────────────────────────────────────┐                                          │
│  │         TP4056 + DW01 + 8205A        │                                          │
│  │                                      │                                          │
│  │  IN+ ◄─── USB 5V                     │                                          │
│  │  IN- ◄─── GND                        │                                          │
│  │                                      │        ┌─────────────────┐               │
│  │  B+ ─────────────────────────────────┼───────►│    BATERÍAS     │               │
│  │  B- ─────────────────────────────────┼───────►│   2× 18650 2P   │               │
│  │                                      │        │   3.7V 4400mAh  │               │
│  │  OUT+ ───────────────────────────────┼──┐     └─────────────────┘               │
│  │  OUT- ───────────────────────────────┼──┼──────────► GND común                  │
│  └──────────────────────────────────────┘  │                                       │
│                                            │                                        │
│                                            ▼                                        │
│                              ┌─────────────────────────┐                           │
│                              │    SWITCH PRINCIPAL     │                           │
│                              │      (ON / OFF)         │                           │
│                              │    SPST Deslizante      │                           │
│                              └────────────┬────────────┘                           │
│                                           │                                         │
│                                           ▼                                         │
│                              ┌─────────────────────────┐                           │
│                              │        MT3608           │                           │
│                              │       Step-Up           │                           │
│                              │                         │                           │
│                              │  IN+ ◄── Switch         │                           │
│                              │  IN- ◄── GND            │                           │
│                              │  OUT+ ──► 5V            │                           │
│                              │  OUT- ──► GND           │                           │
│                              └───────────┬─────────────┘                           │
│                                          │                                          │
│                                     5V   │   GND                                    │
│                                      │   │    │                                     │
│  ┌───────────────────────────────────┼───┼────┼────────────────────────────────┐   │
│  │                                   │   │    │                                │   │
│  │  BORNERA ─────────────────────────┴───┴────┘                                │   │
│  │  5V / GND                                                                   │   │
│  │      │                                                                      │   │
│  │      ▼                                                                      │   │
│  │  ┌──────────────────────────────────────────────────────────────────────┐  │   │
│  │  │                        ESP32 NodeMCU                                 │  │   │
│  │  │                                                                      │  │   │
│  │  │  VIN ◄── 5V                                                          │  │   │
│  │  │  GND ◄── GND                                                         │  │   │
│  │  │                                                                      │  │   │
│  │  │  GPIO25 ──────────────────────────────────────────────► MCP6002 IN+  │  │   │
│  │  │  GPIO17 ──────────────────────────────────────────────► Nextion RX   │  │   │
│  │  │  GPIO16 ◄──────────────[1kΩ]──┬──[2kΩ]──GND ◄───────── Nextion TX   │  │   │
│  │  │  GPIO4  ──────────────────────┼───────────────────────► LED R (330Ω)│  │   │
│  │  │  GPIO5  ──────────────────────┼───────────────────────► LED G (330Ω)│  │   │
│  │  │  GPIO18 ──────────────────────┼───────────────────────► LED B (330Ω)│  │   │
│  │  │                               │                                      │  │   │
│  │  └───────────────────────────────┼──────────────────────────────────────┘  │   │
│  │                                  │                                          │   │
│  │  ┌───────────────────────────────┼──────────────────────────────────────┐  │   │
│  │  │  MCP6002 (Buffer)             │                                      │  │   │
│  │  │                               │                                      │  │   │
│  │  │  VCC ◄── 5V                   │                                      │  │   │
│  │  │  GND ◄── GND                  │                                      │  │   │
│  │  │  IN+ ◄────────────────────────┘                                      │  │   │
│  │  │  IN- ◄── OUT (realimentación)                                        │  │   │
│  │  │  OUT ──────────[100Ω]─────────────────────────────────► BNC          │  │   │
│  │  │                                                                      │  │   │
│  │  └──────────────────────────────────────────────────────────────────────┘  │   │
│  │                                                                             │   │
│  │  ESPADINES NEXTION: 5V ─── GND ─── TX(→ESP RX) ─── RX(←ESP TX)            │   │
│  │                                                                             │   │
│  └─────────────────────────────────────────────────────────────────────────────┘   │
│                                            │                                        │
│                                            │ Cable 4 hilos                          │
│                                            ▼                                        │
│                              ┌─────────────────────────┐                           │
│                              │    NEXTION NX8048T070   │                           │
│                              │         7" 5V           │                           │
│                              │                         │                           │
│                              │  5V ◄── Espadín 5V      │                           │
│                              │  GND ◄── Espadín GND    │                           │
│                              │  TX ──► Espadín TX      │                           │
│                              │  RX ◄── Espadín RX      │                           │
│                              │                         │                           │
│                              └─────────────────────────┘                           │
│                                                                                      │
└──────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 8. Lista de Materiales (BOM)

### 8.1 Componentes Principales

| # | Componente | Descripción | Cantidad | Precio Unit. | Subtotal | Proveedor |
|---|------------|-------------|----------|--------------|----------|-----------|
| 1 | Nextion NX8048T070 | Pantalla táctil 7" 800×480 | 1 | $95.00 | $95.00 | Amazon USA |
| 2 | ESP32 NodeMCU | Microcontrolador 38 pines WiFi/BT | 1 | $8.00 | $8.00 | Novatronic |
| 3 | Batería 18650 | Li-ion 2200mAh 3.7V | 2 | $3.50 | $7.00 | Novatronic |
| 4 | TP4056 Type-C | Cargador + protección DW01 | 1 | $1.80 | $1.80 | Novatronic |
| 5 | MT3608 | Step-Up DC-DC 2A | 1 | $1.50 | $1.50 | Novatronic |
| 6 | MCP6002 | Op-Amp DIP-8 | 1 | $0.80 | $0.80 | Novatronic |
| 7 | Portapilas 18650 | Doble, paralelo | 1 | $1.50 | $1.50 | Novatronic |
| 8 | Switch deslizante | SPST ON/OFF | 1 | $0.30 | $0.30 | Novatronic |
| 9 | Conector BNC | Hembra, panel | 1 | $1.00 | $1.00 | Novatronic |

### 8.2 Componentes Pasivos

| # | Componente | Valor | Cantidad | Precio Unit. | Subtotal | Proveedor |
|---|------------|-------|----------|--------------|----------|-----------|
| 10 | Resistencia | 330Ω 1/4W | 3 | $0.05 | $0.15 | Novatronic |
| 11 | Resistencia | 1kΩ 1/4W | 1 | $0.05 | $0.05 | Novatronic |
| 12 | Resistencia | 2kΩ 1/4W | 1 | $0.05 | $0.05 | Novatronic |
| 13 | Resistencia | 100Ω 1/4W | 1 | $0.05 | $0.05 | Novatronic |
| 14 | LED RGB | 5mm cátodo común | 1 | $0.50 | $0.50 | Novatronic |

### 8.3 Conectores y Mecánicos

| # | Componente | Descripción | Cantidad | Precio Unit. | Subtotal | Proveedor |
|---|------------|-------------|----------|--------------|----------|-----------|
| 15 | Bornera | 2 posiciones 5mm | 1 | $0.30 | $0.30 | Novatronic |
| 16 | Header macho | 4 pines 2.54mm | 1 | $0.20 | $0.20 | Novatronic |
| 17 | Cable dupont | Hembra-Hembra 20cm | 10 | $0.10 | $1.00 | Novatronic |
| 18 | PCB perforada | 7×9 cm | 1 | $1.00 | $1.00 | Novatronic |
| 19 | Caja/Gabinete | Plástico para proyecto | 1 | $5.00 | $5.00 | Novatronic |
| 20 | Tornillos/Separadores | Kit variado | 1 | $2.00 | $2.00 | Novatronic |

### 8.4 Resumen de Costos

| Categoría | Subtotal |
|-----------|----------|
| Componentes principales | $116.90 |
| Componentes pasivos | $0.80 |
| Conectores y mecánicos | $9.50 |
| **TOTAL** | **$127.20 USD** |

> **Nota:** Precios referenciales Ecuador, Diciembre 2024. La pantalla Nextion fue importada de Amazon USA ($95). El resto de componentes están disponibles localmente.

---

## 9. Manual de Usuario

### 9.1 Encendido y Apagado

```
┌─────────────────────────────────────────────────────────────────┐
│                    OPERACIÓN BÁSICA                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   ENCENDER:                                                    │
│   ─────────                                                    │
│   1. Verificar que el cable USB NO esté conectado             │
│   2. Deslizar switch principal a posición ON                   │
│   3. Esperar ~3 segundos para inicialización                  │
│   4. LED RGB indica estado (ver tabla de estados)             │
│                                                                 │
│   APAGAR:                                                      │
│   ───────                                                      │
│   1. Deslizar switch principal a posición OFF                  │
│   2. El sistema se apaga inmediatamente                       │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 9.2 Carga de Batería

```
┌─────────────────────────────────────────────────────────────────┐
│                    PROCEDIMIENTO DE CARGA                       │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   ⚠️  IMPORTANTE: APAGAR EL DISPOSITIVO ANTES DE CARGAR        │
│                                                                 │
│   1. Deslizar switch principal a posición OFF                  │
│   2. Conectar cable USB Type-C al módulo TP4056               │
│   3. Conectar el otro extremo a cargador USB 5V/1A            │
│   4. LED ROJO del TP4056 = Cargando                           │
│   5. LED AZUL del TP4056 = Carga completa                     │
│   6. Desconectar USB cuando termine                           │
│   7. Tiempo de carga completa: ~5 horas                       │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 9.3 Indicadores LED

| LED TP4056 | Estado | Significado |
|------------|--------|-------------|
| Rojo | Encendido | Batería cargando |
| Azul | Encendido | Carga completa |
| Apagados | - | Sin USB conectado |

| LED RGB Sistema | Color | Significado |
|-----------------|-------|-------------|
| Rojo | Fijo | Sistema listo, sin actividad |
| Verde | Fijo | Generando señal |
| Azul | Parpadeo | Conectando WiFi |
| Azul | Fijo | Error de comunicación |

### 9.4 Conexión de Salida Analógica

```
Conector: BNC hembra
Señal: 0V - 3.3V
Impedancia: ~100Ω

Uso típico:
• Conectar cable BNC a osciloscopio
• Configurar osciloscopio: 1V/div, AC coupling
• Ajustar timebase según señal (ECG: 200ms/div)
```

---

## 10. Limitaciones y Precauciones

### 10.1 Precaución Principal

> ⚠️ **PRECAUCIÓN: No operar el dispositivo mientras la batería esté en proceso de carga.**

**Motivo técnico:** La operación simultánea puede interferir con la detección de fin de carga del TP4056 y provocar estados de flotación prolongados. El chip TP4056 detecta el fin de carga cuando la corriente cae por debajo de 100mA. Si el sistema está consumiendo corriente, esta condición nunca se cumple correctamente.

**Seguridad de la celda:** La batería está protegida contra sobrecarga, sobredescarga y sobrecorriente por el circuito DW01 + 8205A. Esta precaución es **funcional** (para asegurar carga correcta), no por riesgo químico inmediato.

**Buenas prácticas:** Realizar la carga con el equipo apagado (switch en OFF) para asegurar una terminación de carga confiable.

### 10.2 Otras Precauciones

| Precaución | Motivo | Consecuencia si se ignora |
|------------|--------|---------------------------|
| No usar mientras carga | Interferencia con detección fin de carga | Batería no llega a 100%, ciclos incompletos |
| No cortocircuitar BNC | Corriente excesiva en MCP6002 | Posible daño al op-amp |
| No exponer a humedad | Componentes electrónicos | Cortocircuitos, corrosión |
| No descargar < 3.0V | Daño a celdas Li-ion | Reducción capacidad permanente |
| Usar cargador 5V/1A | Especificación TP4056 | Carga lenta o sobrecalentamiento |

### 10.3 Limitaciones del Diseño

| Limitación | Descripción | Impacto |
|------------|-------------|---------|
| Sin power path | No hay conmutación automática USB/batería | Requiere apagar para cargar |
| Autonomía limitada | 2.6 - 3.7 horas según uso | Planificar sesiones de uso |
| Salida 0-3.3V | Limitada por DAC del ESP32 | Compatible con la mayoría de equipos |
| Sin indicador batería | No hay medición de voltaje de batería | Usuario debe estimar carga restante |

---

## 11. Mejoras Futuras

### 11.1 Power Path Automático

**Problema actual:** El usuario debe apagar el dispositivo para cargar la batería.

**Solución propuesta:** Implementar un circuito de power path que permita:
- Alimentar el sistema desde USB cuando está conectado
- Cargar la batería simultáneamente
- Conmutar automáticamente a batería cuando se desconecta USB

**Componentes sugeridos:**
- MOSFET P-channel (ej: SI2301) para conmutación
- Diodo Schottky para aislamiento
- Referencia: Microchip AN1149 "Designing A Li-Ion Battery Charger and Load Sharing System"

### 11.2 Indicador de Nivel de Batería

**Propuesta:** Agregar medición de voltaje de batería usando ADC del ESP32.

```
Batería (+) ──[100kΩ]──┬──[100kΩ]── GND
                       │
                       └──► ESP32 ADC (GPIO34)

V_ADC = V_bat / 2
Rango: 1.5V - 2.1V (corresponde a 3.0V - 4.2V batería)
```

### 11.3 Carga Rápida

**Propuesta:** Usar módulo con mayor corriente de carga (ej: 2A) para reducir tiempo de carga a ~2.5 horas.

---

## 12. Referencias

### 12.1 Datasheets

1. **TP4056** - NanJing Top Power ASIC Corp. "1A Standalone Linear Li-Ion Battery Charger"
2. **DW01** - Fortune Semiconductor Corp. "One Cell Lithium-ion/Polymer Battery Protection IC"
3. **MT3608** - Aerosemi Technology Co. "High Efficiency 1.2MHz 2A Step Up Converter"
4. **MCP6002** - Microchip Technology Inc. "1 MHz, Low-Power Op Amp"
5. **ESP32** - Espressif Systems. "ESP32 Series Datasheet"
6. **Nextion NX8048T070** - ITEAD Studio. "Nextion Instruction Set"

### 12.2 Notas de Aplicación

1. **Microchip AN1149** - "Designing A Li-Ion Battery Charger and Load Sharing System With Microchip's Stand-Alone Li-Ion Battery Charge Management Controller"
2. **Texas Instruments SLVA372** - "Battery Charging and Power Path Management"

### 12.3 Estándares

1. **IEC 62133** - Secondary cells and batteries containing alkaline or other non-acid electrolytes – Safety requirements
2. **IEC 60950-1** - Information technology equipment – Safety

---

**Documento preparado para:** Trabajo de Titulación  
**Versión:** 1.1.0  
**Última actualización:** Diciembre 2024
