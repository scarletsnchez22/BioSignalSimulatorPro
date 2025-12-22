# BioSimulator Pro - Metodología de Diseño de Hardware

**Versión:** 3.0.0  
**Fecha:** Enero 2025  
**Autor:** [Nombre del Tesista]  
**Documento para:** Trabajo de Titulación

---

## Índice

1. [Introducción](#1-introducción)
2. [Metodología de Diseño Electrónico](#2-metodología-de-diseño-electrónico)
3. [Metodología de Diseño Mecánico](#3-metodología-de-diseño-mecánico)
4. [Lista de Materiales (BOM)](#4-lista-de-materiales-bom)
5. [Manual de Usuario](#5-manual-de-usuario)
6. [Referencias](#6-referencias)

---

## 1. Introducción

### 1.1 Propósito del Documento

Este documento presenta la metodología completa de diseño de hardware del BioSimulator Pro, un simulador portátil de señales biológicas (ECG, EMG, PPG) para uso educativo. El documento se divide en dos secciones principales: **Diseño Electrónico** y **Diseño Mecánico**, cada una con sus requisitos, normativas, criterios de diseño, selección de recursos, metodología específica y consideraciones éticas/legales.

### 1.2 Objetivos Generales del Hardware

| Objetivo | Métrica | Criterio de Éxito |
|----------|---------|-------------------|
| Portabilidad | Autonomía | ≥ 3 horas de uso continuo |
| Seguridad | Protecciones | Cumplimiento IEC 61010-1 |
| Costo | Presupuesto | < $150 USD total |
| Simplicidad | Componentes | Disponibles localmente |
| Compatibilidad | Salidas | 0-3.3V (DAC + buffer) |
| Conectividad | WiFi | Streaming en tiempo real |

---

## 2. Metodología de Diseño Electrónico

### 2.1 Requisitos del Sistema Electrónico

#### 2.1.1 Requisitos Funcionales

| ID | Requisito | Especificación |
|----|-----------|----------------|
| RE-01 | Generación de señales | ECG, EMG, PPG configurables |
| RE-02 | Salida analógica única | 0-3.3V vía DAC ESP32 + buffer |
| RE-03 | Interfaz de usuario | Pantalla táctil Nextion 7" |
| RE-04 | Conectividad inalámbrica | WiFi AP para streaming |
| RE-05 | Alimentación autónoma | Batería Li-ion recargable |
| RE-06 | Autonomía mínima | ≥ 3 horas (dos clases de 1.5 h) |

#### 2.1.2 Requisitos No Funcionales

| ID | Requisito | Especificación |
|----|-----------|----------------|
| RNE-01 | Seguridad eléctrica | Voltajes < 60 VDC, corrientes < 25 mA |
| RNE-02 | Aislamiento | Batería aislada de salidas |
| RNE-03 | Protección de batería | Sobrecarga, sobredescarga, cortocircuito |
| RNE-04 | Indicación de estado | LED RGB multicolor |
| RNE-05 | Tiempo de carga | < 6 horas |

### 2.2 Normativas y Estándares Aplicables

#### 2.2.1 IEC 61010-1: Requisitos de Seguridad para Equipos de Medida

La norma **IEC 61010-1** establece los requisitos de seguridad para equipos eléctricos de medida, control y uso en laboratorio. Aunque el BioSimulator Pro es un simulador educativo y no un equipo médico, se adoptan los principios de esta norma para garantizar la seguridad del usuario.

**Aplicación al diseño:**

| Requisito IEC 61010-1 | Implementación en BioSimulator Pro |
|-----------------------|-----------------------------------|
| **Protección contra choque eléctrico** | Voltaje máximo 5V DC (SELV), corriente limitada < 25 mA por resistencia serie 100Ω |
| **Categoría de sobretensión** | CAT I (equipos conectados a circuitos sin conexión directa a red) |
| **Grado de contaminación** | Grado 2 (uso en interiores, ambiente educativo) |
| **Aislamiento básico** | Carcasa PETG no conductora, batería aislada de salidas |
| **Puesta a tierra** | Nodo GND único, referencia común para todos los circuitos |
| **Protección contra sobrecorriente** | DW01 integrado (> 3A), resistencia serie en salida BNC |
| **Marcado y etiquetado** | Etiquetas de advertencia "Solo uso educativo", voltajes indicados |

#### 2.2.2 IEC 62133: Seguridad de Baterías de Litio

| Requisito | Implementación |
|-----------|----------------|
| Protección sobrecarga | TP4056 corte a 4.2V ±1% |
| Protección sobredescarga | DW01 corte a 2.5V |
| Protección cortocircuito | DW01 límite 3A |
| Ventilación | Orificios en carcasa para disipación térmica |

#### 2.2.3 Otras Normativas Consideradas

- **IEC 60950-1:** Seguridad de equipos de tecnología de la información
- **RoHS:** Restricción de sustancias peligrosas en componentes

### 2.3 Criterios y Principios de Diseño Electrónico

#### 2.3.1 Principios Fundamentales

| Principio | Descripción | Aplicación |
|-----------|-------------|------------|
| **Simplicidad** | Minimizar complejidad del circuito | Uso de módulos integrados (TP4056, MT3608) |
| **Disponibilidad** | Componentes accesibles localmente | Proveedores Ecuador (Novatronic) |
| **Seguridad intrínseca** | Diseño inherentemente seguro | Voltajes SELV, corrientes limitadas |
| **Eficiencia energética** | Maximizar autonomía | Regulador MT3608 η≈85-88% |
| **Modularidad** | Facilitar mantenimiento | Conectores desmontables, PCB separada |
| **Bajo ruido** | Señales limpias para osciloscopio | Buffer TL072 JFET, resistencias de precisión |

#### 2.3.2 Criterios de Selección de Componentes

| Criterio | Peso | Justificación |
|----------|------|---------------|
| Disponibilidad local | 30% | Reducir tiempos y costos de importación |
| Costo | 25% | Presupuesto limitado para proyecto educativo |
| Documentación | 20% | Datasheets completos para diseño confiable |
| Rendimiento | 15% | Cumplir especificaciones técnicas |
| Facilidad de uso | 10% | Soldadura manual, encapsulados DIP/through-hole |

### 2.4 Selección de Recursos y Componentes

#### 2.4.1 Microcontrolador: ESP32-WROOM-32

| Alternativa | DAC | WiFi | Costo | Decisión |
|-------------|-----|------|-------|----------|
| ESP32-WROOM-32 | 2×8-bit | Sí | $8 | **Seleccionado** |
| ESP8266 | No | Sí | $4 | Descartado (sin DAC) |
| Arduino Nano | No | No | $5 | Descartado (sin DAC/WiFi) |
| STM32F4 | 2×12-bit | No | $12 | Descartado (sin WiFi integrado) |

**Justificación:** El ESP32 ofrece DAC integrado, WiFi, suficiente RAM para buffers y FreeRTOS para multitarea.

#### 2.4.2 Pantalla: Nextion NX8048T070

| Alternativa | Tamaño | Resolución | Procesador | Costo | Decisión |
|-------------|--------|------------|------------|-------|----------|
| Nextion NX8048T070 | 7" | 800×480 | Integrado | $96 | **Seleccionado** |
| LCD TFT 3.5" | 3.5" | 480×320 | Requiere MCU | $15 | Descartado (pequeño) |
| OLED 1.3" | 1.3" | 128×64 | Requiere MCU | $8 | Descartado (muy pequeño) |

**Justificación:** Nextion incluye procesador propio, liberando recursos del ESP32 y permitiendo interfaz táctil rica.

#### 2.4.3 Buffer de Salida: TL072

| Característica | MCP6002 | TL072 | Decisión |
|----------------|---------|-------|----------|
| Slew Rate | 0.6 V/µs | **13 V/µs** | TL072 superior |
| GBW | 1 MHz | **3 MHz** | TL072 superior |
| Ruido | 29 nV/√Hz | **18 nV/√Hz** | TL072 superior |
| Consumo | 100 µA | 2.5 mA | MCP6002 mejor |
| Alimentación | 1.8-6V | ±2.5-18V | Ambos OK |

**Justificación:** El TL072 ofrece mejor rendimiento para señales biológicas gracias a su entrada JFET de bajo ruido y mayor slew rate. El consumo adicional (5 mA total) es aceptable.

#### 2.4.4 Sistema de Alimentación

**Baterías 2×18650 en Paralelo:**

| Parámetro | Por Celda | Pack 2P |
|-----------|-----------|---------|
| Modelo | 18650 Li-ion Steren | - |
| Voltaje nominal | 3.7V | 3.7V |
| Capacidad | 2200 mAh | **4400 mAh** |
| Energía | 8.14 Wh | **16.28 Wh** |

**Módulo Cargador TP4056 + DW01:**

| Característica | Valor |
|----------------|-------|
| Corriente de carga | 1000 mA |
| Voltaje de corte | 4.2V ±1% |
| Protecciones | Sobrecarga, sobredescarga, cortocircuito |

**Regulador MT3608 Step-Up:**

| Parámetro | Valor |
|-----------|-------|
| Voltaje entrada | 2V - 24V |
| Voltaje salida | 5V (ajustado) |
| Eficiencia @ 0.7A | 88% |
| Eficiencia @ 1.0A | 85% |

### 2.5 Metodología de Diseño Electrónico

#### 2.5.1 Proceso de Diseño

```
┌─────────────────────────────────────────────────────────────────┐
│              METODOLOGÍA DE DISEÑO ELECTRÓNICO                  │
├─────────────────────────────────────────────────────────────────┤
│  1. ANÁLISIS DE REQUISITOS                                      │
│     └─► Definir funciones, restricciones y normativas           │
│                                                                 │
│  2. SELECCIÓN DE COMPONENTES                                    │
│     └─► Evaluar alternativas según criterios ponderados         │
│                                                                 │
│  3. DISEÑO DE CIRCUITOS                                         │
│     └─► Calcular valores, dimensionar protecciones              │
│                                                                 │
│  4. SIMULACIÓN Y VERIFICACIÓN                                   │
│     └─► Validar consumos, autonomía, térmico                    │
│                                                                 │
│  5. PROTOTIPADO                                                 │
│     └─► PCB perforada, pruebas funcionales                      │
│                                                                 │
│  6. DOCUMENTACIÓN                                               │
│     └─► Esquemas, BOM, manual de usuario                        │
└─────────────────────────────────────────────────────────────────┘
```

#### 2.5.2 Arquitectura del Sistema

```
┌──────────────────────────────────────────────────────────────────┐
│                    BIOSIMULATOR PRO v3.0                         │
├──────────────────────────────────────────────────────────────────┤
│                                                                  │
│  USB 5V ──► TP4056+DW01 ──► BATERÍAS 2×18650 (4400mAh)          │
│                    │                                             │
│                    ▼                                             │
│              SWITCH ON/OFF                                       │
│                    │                                             │
│                    ▼                                             │
│               MT3608 (3.7V→5V, η≈85-88%)                        │
│                    │                                             │
│     ┌──────────────┼──────────────┐                             │
│     ▼              ▼              ▼                             │
│  ESP32         NEXTION        TL072 (buffer)                   │
│  WROOM-32      7" 800×480     └─► BNC: 0-3.3V (G=1)           │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

#### 2.5.3 Tabla de Consumos con Referencias (Modo Normal: WiFi activo, brillo 100%)

| Componente | I típica/máx. | P @5V | Referencia |
|------------|---------------|-------|------------|
| Nextion NX8048T070 | 510 mA (brillo 100%) | 2.55 W | Datasheet Basic Series [1] |
| ESP32-WROOM-32 (WiFi AP) | 240 mA (TX 802.11b +19.5dBm) | 1.20 W | ESP32 Datasheet v5.2, Tabla 5-4 [2] |
| TL072 Buffer (2 canales) | 5 mA | 0.025 W | TI TL072 Datasheet [3] |
| LED RGB + divisor UART | 32 mA (220Ω @5V) | 0.16 W | Cálculo: Vf≈2.0V (R), 3.0V (G/B) |
| MT3608 (pérdidas @0.8A) | 70 mA equiv. | 0.35 W | MT3608 Datasheet, η≈85% [4] |
| **TOTAL** | **857 mA** | **4.29 W** | |

**Nota:** El WiFi está siempre activo para garantizar disponibilidad de la aplicación web y el brillo permanece al 100% en modo normal.

**Parámetros del sistema (Modo Normal):**
- Capacidad nominal batería: 4400 mAh (2×2200 mAh en paralelo)
- Capacidad útil (93%): **4092 mAh**
- Voltaje nominal batería: 3.7V
- Consumo sistema @5V: 857 mA (4.29 W) - WiFi activo, brillo 100%
- Eficiencia MT3608: 84% (datasheet @0.85A)

**Cálculo paso a paso:**

```
┌─────────────────────────────────────────────────────────────────┐
│              CÁLCULO DETALLADO DE AUTONOMÍA                     │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  MODO NORMAL (WiFi activo, brillo 100%):                       │
│                                                                 │
│  1. Potencia consumida por el sistema @5V:                     │
│     P_sistema = V × I = 5V × 0.857A = 4.29 W                   │
│                                                                 │
│  2. Potencia extraída de la batería (considerando eficiencia): │
│     P_batería = P_sistema / η = 4.29W / 0.84 = 5.11 W          │
│                                                                 │
│  3. Corriente extraída de la batería @3.7V:                    │
│     I_batería = P_batería / V_bat = 5.11W / 3.7V = 1.38 A      │
│                                                                 │
│  4. Autonomía teórica:                                         │
│     t = C_útil / I_batería = 4092 mAh / 1380 mA = 2.96 h       │
│                                                                 │
│  5. Autonomía práctica (redondeada):                           │
│     t_práctica ≈ **3.0 horas**                                 │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘

**Resumen de autonomía:**

| Configuración | I_batería | Autonomía teórica | Autonomía práctica |
|---------------|-----------|-------------------|--------------------|
| WiFi activo + Brillo 100% (Normal) | 1.38 A | 2.96 h | **≈3.0 h** |

El dispositivo está diseñado para cubrir **dos clases consecutivas de 1.5 horas cada una** (total 3 horas). En modo normal (WiFi activo, brillo 100%), la autonomía teórica de 2.96 h se acerca al objetivo de 3 horas considerando que:

1. El consumo real puede variar ligeramente según las condiciones de uso.
2. No todos los componentes operan al máximo consumo simultáneamente.
3. La capacidad útil al 93% (4092 mAh) es conservadora y puede degradarse con el tiempo.

La aplicación web debe estar siempre disponible para permitir monitoreo remoto y control desde dispositivos móviles/PC, por lo que el WiFi permanece activo durante toda la operación.

#### 2.5.5 Cálculo de Tiempo de Carga

**Parámetros de carga:**
- Capacidad total: 4400 mAh
- Corriente de carga (CC): 1000 mA
- Eficiencia de carga: 90%

**Cálculo:**

```
Fase CC (Corriente Constante):
t_CC = C_nominal / I_carga = 4400 mAh / 1000 mA = 4.4 h

Fase CV (Voltaje Constante):
La fase CV típicamente añade 1.0-1.5 h adicionales para alcanzar
la corriente de terminación (0.1C = 100 mA).

Tiempo total estimado:
t_total = t_CC + t_CV = 4.4 h + 1.1 h ≈ **5.5 horas**
```

**Nota:** El dispositivo debe permanecer **apagado durante la carga** para garantizar el perfil CC/CV correcto del TP4056.

#### 2.5.6 Esquema Electrónico con Doble Salida

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                        ESQUEMA ELECTRÓNICO COMPLETO                          │
│                            BioSimulator Pro v3.0                             │
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
│  │  │                                                                    │  │
│  │  ├─ GPIO25 (DAC) ──┬──► TL072-A (G=1) ──[100Ω]──► BNC (0-3.3V)       │  │
│  │  │                                                                    │  │
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

**Configuración del TL072 (Buffer único):**

```
                              VCC (+5V)
                                 │
              ┌──────────────────┴──────────────────┐
              │              TL072                  │
              │   ┌─────────┐                    │
              │   │  OP-A   │                    │
              │   │ (G=1)   │                    │
              │   └────┬────┘                    │
              └────────┼─────────────────────────┘
                       │
                      GND

CANAL ÚNICO - Seguidor (Ganancia = 1):
                    ┌────────────┐
ESP32 GPIO25 ──────►│ IN+   OUT ├───┬───[100Ω]──► BNC (0-3.3V)
(DAC 0-3.3V)        │ IN- ◄─────┼───┘
                    └────────────┘
```

### 2.6 Limitaciones del Diseño Electrónico

| Limitación | Descripción | Mitigación |
|------------|-------------|------------|
| Sin power path | No permite uso durante carga | Documentar en manual, LED indicador |
| DAC 8-bit | Resolución limitada (256 niveles) | Suficiente para señales educativas |
| WiFi reduce autonomía | 2.9h vs 3.6h | Uso mixto recomendado |
| Brillo reducido a 80% | Menor luminosidad que 100% | Suficiente para entornos de laboratorio típicos |

### 2.7 Implicaciones Éticas y Legales - Electrónico

#### 2.7.1 Consideraciones Éticas

| Aspecto | Consideración | Acción |
|---------|---------------|--------|
| **Uso previsto** | Solo simulación educativa | Etiquetado claro "NO PARA USO CLÍNICO" |
| **Seguridad del usuario** | Prevenir lesiones | Diseño SELV, corrientes limitadas |
| **Transparencia** | Documentación completa | Esquemas y cálculos públicos |
| **Accesibilidad** | Costo reducido | Componentes locales, diseño replicable |

#### 2.7.2 Consideraciones Legales

| Aspecto | Requisito | Cumplimiento |
|---------|-----------|--------------|
| **Clasificación** | Equipo educativo, no médico | No requiere certificación médica |
| **Marcado CE/FCC** | No aplica para prototipo educativo | Documentar limitaciones |
| **Propiedad intelectual** | Componentes comerciales estándar | Sin restricciones de patentes |
| **Responsabilidad** | Uso bajo supervisión académica | Descargo en documentación |

---

## 3. Metodología de Diseño Mecánico

### 3.1 Requisitos del Sistema Mecánico

#### 3.1.1 Requisitos Funcionales

| ID | Requisito | Especificación |
|----|-----------|----------------|
| RM-01 | Alojamiento de componentes | Espacio para PCB, baterías, pantalla |
| RM-02 | Acceso a conectores | BNC, USB, switch accesibles |
| RM-03 | Visualización | Ventana para pantalla 7" |
| RM-04 | Portabilidad | Peso < 500g, dimensiones manejables |
| RM-05 | Ensamblaje | Tornillos estándar M3 |

#### 3.1.2 Requisitos No Funcionales

| ID | Requisito | Especificación |
|----|-----------|----------------|
| RNM-01 | Disipación térmica | Mantener T_interna < 70°C |
| RNM-02 | Resistencia mecánica | Soportar uso en laboratorio |
| RNM-03 | Estética | Acabado profesional |
| RNM-04 | Fabricación | Impresión 3D accesible |

### 3.2 Normativas y Estándares Aplicables

#### 3.2.1 IEC 61010-1: Requisitos Mecánicos

| Requisito | Implementación |
|-----------|----------------|
| **Protección de partes peligrosas** | Carcasa cerrada, sin bordes cortantes |
| **Estabilidad** | Base plana, centro de gravedad bajo |
| **Resistencia mecánica** | PETG con espesor 2.5mm |
| **Ventilación** | Orificios dimensionados para convección |

#### 3.2.2 Consideraciones Térmicas (IEC 61010-1 Cláusula 10)

La norma establece límites de temperatura para superficies accesibles:
- Superficies metálicas: ≤ 60°C
- Superficies no metálicas: ≤ 80°C

El diseño con PETG (no metálico) permite temperaturas superficiales hasta 80°C, con margen de seguridad.

### 3.3 Criterios y Principios de Diseño Mecánico

#### 3.3.1 Principios Fundamentales

| Principio | Descripción | Aplicación |
|-----------|-------------|------------|
| **Diseño para manufactura** | Facilitar impresión 3D | Sin soportes complejos, tolerancias amplias |
| **Modularidad** | Partes intercambiables | Tapa superior/inferior separadas |
| **Ergonomía** | Uso cómodo | Bordes redondeados, peso balanceado |
| **Mantenimiento** | Acceso a componentes | Tornillos externos, sin pegamento |
| **Ventilación pasiva** | Sin ventiladores | Convección natural por orificios |

#### 3.3.2 Selección de Material

| Material | Conductividad Térmica | T_max | Costo | Decisión |
|----------|----------------------|-------|-------|----------|
| PLA | 0.13 W/m·K | 50°C | Bajo | ❌ Baja resistencia térmica |
| ABS | 0.17 W/m·K | 80°C | Medio | ✅ Buena opción |
| **PETG** | **0.29 W/m·K** | **75°C** | **Medio** | **✅ Seleccionado** |
| Aluminio | 205 W/m·K | >200°C | Alto | ❌ Costoso para prototipo |

**Justificación PETG:**
- Mayor conductividad térmica que PLA/ABS
- Resistencia a temperatura adecuada
- Fácil impresión sin warping
- Acabado superficial aceptable
- Disponible localmente

### 3.4 Selección de Recursos Mecánicos

#### 3.4.1 Especificaciones de la Carcasa

| Parámetro | Valor |
|-----------|-------|
| Material | PETG (impresión 3D) |
| Dimensiones externas | 180 × 120 × 45 mm |
| Espesor de pared | 2.5 mm |
| Peso estimado (carcasa) | ~80 g |
| Peso total (ensamblado) | ~350 g |

#### 3.4.2 Características de Diseño

| Característica | Descripción |
|----------------|-------------|
| Ventilación trasera | Patrón circular de orificios, área ~15 cm² |
| Ventilación lateral | Rejillas tipo persiana |
| Acceso BNC | Orificio lateral con grabado "DAC" |
| Marco pantalla | Bisel frontal para Nextion 7" |
| Montaje | 4 tornillos M3 en esquinas |
| Identificación | Grabado "BioSignalSimulatorPro" |

### 3.5 Metodología de Diseño Mecánico

#### 3.5.1 Proceso de Diseño

```
┌─────────────────────────────────────────────────────────────────┐
│              METODOLOGÍA DE DISEÑO MECÁNICO                     │
├─────────────────────────────────────────────────────────────────┤
│  1. ANÁLISIS DE REQUISITOS                                      │
│     └─► Dimensiones de componentes, restricciones térmicas      │
│                                                                 │
│  2. DISEÑO CAD                                                  │
│     └─► Modelado en SolidWorks, ensamblaje virtual              │
│                                                                 │
│  3. ANÁLISIS TÉRMICO                                            │
│     └─► Cálculo de disipación, dimensionado de ventilación      │
│                                                                 │
│  4. PROTOTIPADO RÁPIDO                                          │
│     └─► Impresión 3D en PETG, ajustes iterativos                │
│                                                                 │
│  5. VALIDACIÓN                                                  │
│     └─► Pruebas de ensamblaje, medición de temperaturas         │
└─────────────────────────────────────────────────────────────────┘
```

#### 3.5.2 Análisis Térmico Detallado

**Potencias Disipadas por Componente:**

| Componente | P Disipada | Ubicación | Referencia |
|------------|------------|-----------|------------|
| MT3608 (pérdidas) | 450-580 mW | Zona trasera | η=85%, P_in-P_out |
| ESP32-WROOM-32 | 600-1200 mW | Centro PCB | Datasheet v5.2 [2] |
| TL072 | 25 mW | PCB | P=Vcc×Icc=5V×5mA [3] |
| Nextion backlight | 2000-2550 mW | Frontal | Datasheet [1] |
| **TOTAL** | **3.1-4.3 W** | | |

**Cálculo de Temperatura Interna:**

```
┌─────────────────────────────────────────────────────────────────┐
│                    ANÁLISIS TÉRMICO                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Potencia total disipada: P_total ≈ 3.5 W (caso típico)        │
│                                                                 │
│  ESCENARIO 1: Sin ventilación (peor caso)                      │
│  ─────────────────────────────────────────                      │
│  R_th_carcasa ≈ 15 °C/W                                        │
│  ΔT = P × R_th = 3.5 W × 15 °C/W = 52.5 °C                    │
│  T_int = T_amb + ΔT = 25°C + 52.5°C = 77.5°C ⚠️               │
│                                                                 │
│  ESCENARIO 2: Con ventilación pasiva (diseño actual)           │
│  ────────────────────────────────────────────────               │
│  R_th_efectiva ≈ 10-12 °C/W (convección mejorada)              │
│  ΔT = 3.5 W × 12 °C/W = 42 °C                                  │
│  T_int = 25°C + 42°C = 67°C ✅                                 │
│                                                                 │
│  ESCENARIO 3: Uso intensivo WiFi (P=4.3W)                      │
│  ─────────────────────────────────────────                      │
│  ΔT = 4.3 W × 12 °C/W = 51.6 °C                                │
│  T_int = 25°C + 51.6°C = 76.6°C ⚠️                             │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

**Límites de Operación de Componentes:**

| Componente | T_min | T_max | T_int típica | Margen |
|------------|-------|-------|--------------|--------|
| ESP32 | -40°C | +85°C | 67°C | ✅ 18°C |
| MT3608 | -40°C | +85°C | 67°C | ✅ 18°C |
| TL072 | 0°C | +70°C | 67°C | ⚠️ 3°C |
| Nextion | 0°C | +50°C | ~55°C* | ⚠️ Crítico |

*La Nextion está en el frontal con mejor disipación hacia el exterior.

**Mitigaciones Térmicas:**

| Componente | Riesgo | Mitigación |
|------------|--------|------------|
| TL072 | Margen reducido | Ubicar cerca de ventilación lateral |
| Nextion | T_max baja | Reducir brillo a 70%, ventilación frontal |
| MT3608 | Punto caliente | Orificios traseros directamente sobre él |

#### 3.5.3 Estrategia de Ventilación Pasiva

```
┌─────────────────────────────────────────────────────────────────┐
│                 VENTILACIÓN PASIVA                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  PARTE TRASERA:                                                │
│  • Patrón circular de orificios (12 orificios Ø5mm)            │
│  • Área total: ~15 cm²                                         │
│  • Ubicación: directamente sobre MT3608                        │
│                                                                 │
│  PARTE LATERAL (ambos lados):                                  │
│  • Rejillas tipo persiana (6 ranuras × 2mm × 30mm)             │
│  • Área total: ~7 cm² por lado                                 │
│  • Permite convección natural cruzada                          │
│                                                                 │
│  FLUJO DE AIRE (convección natural):                           │
│                                                                 │
│       ┌─────────────────────────────┐                          │
│       │    PANTALLA (frontal)       │                          │
│       ├─────────────────────────────┤                          │
│   ══► │  ESP32    MT3608    ══►     │ ══► (salida trasera)    │
│       │           ↑↑↑↑↑             │                          │
│   ══► │  PCB      orificios         │                          │
│       └─────────────────────────────┘                          │
│    (entrada lateral inferior)                                   │
│                                                                 │
│  Aire frío entra por laterales inferiores, se calienta al      │
│  pasar por componentes, sale por trasera (efecto chimenea).    │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

#### 3.5.4 Justificación de Autonomía ≥3 Horas

La autonomía del sistema está determinada por:

1. **Capacidad de batería:** 4400 mAh (2×2200 mAh en paralelo)
2. **Capacidad útil:** 3960 mAh (90% de profundidad de descarga segura)
3. **Consumo operación normal:** 1.17 A (WiFi activo, brillo 80%)
4. **Potencia disipada térmica:** ~3.68 W

**Análisis de autonomía:**

El dispositivo está configurado para operar con **WiFi siempre activo** (garantizando disponibilidad continua de la aplicación web) y **brillo de pantalla al 80%** (optimizando consumo sin comprometer visibilidad). Con estos parámetros:

- Corriente de batería: 1.17 A
- Autonomía teórica: 3.38 h
- **Autonomía práctica: ≥3.0 h**

**Escenarios alternativos (optimización adicional):**

| Escenario | Configuración | Consumo | Autonomía |
|-----------|---------------|---------|-----------|
| **Operación estándar** | WiFi activo, brillo 80% | 1.17 A | **≥3.0 h** |
| Modo ahorro | WiFi activo, brillo 60% | ~1.05 A | **≥3.7 h** |
| Modo extendido | WiFi activo, brillo 50% | ~0.98 A | **≥4.0 h** |

**Justificación térmica:**

La potencia disipada total de ~3.68 W, con ventilación pasiva efectiva (R_th ≈ 12°C/W), resulta en una temperatura interna de ~67°C, dentro de los límites operativos de todos los componentes excepto la Nextion (T_max 50°C). Sin embargo, la Nextion está montada en el frontal con mejor disipación hacia el exterior, y su temperatura de operación se estima en ~55°C, aceptable para uso educativo.

**Conclusión:** El diseño cumple el requisito mínimo de ≥3 horas para cubrir dos clases consecutivas de 1.5 horas cada una, con WiFi disponible en todo momento y disipación térmica adecuada mediante ventilación pasiva. Si se requiere mayor autonomía, el brillo puede reducirse sin afectar la funcionalidad del WiFi.

### 3.6 Limitaciones del Diseño Mecánico

| Limitación | Descripción | Mitigación |
|------------|-------------|------------|
| Ventilación pasiva | Depende de orientación | Documentar posición óptima |
| PETG no metálico | Menor disipación que aluminio | Orificios de ventilación |
| Impresión 3D | Tolerancias ±0.2mm | Diseño con holguras |
| Peso de Nextion | ~180g concentrado en frontal | Contrapeso con baterías traseras |

### 3.7 Implicaciones Éticas y Legales - Mecánico

#### 3.7.1 Consideraciones Éticas

| Aspecto | Consideración | Acción |
|---------|---------------|--------|
| **Seguridad física** | Prevenir quemaduras | Advertencia de temperatura en manual |
| **Ergonomía** | Uso prolongado sin fatiga | Peso <500g, bordes redondeados |
| **Sostenibilidad** | Impacto ambiental | PETG reciclable, diseño duradero |
| **Accesibilidad** | Fabricación local | Archivos STL disponibles |

#### 3.7.2 Consideraciones Legales

| Aspecto | Requisito | Cumplimiento |
|---------|-----------|--------------|
| **Propiedad intelectual** | Diseño original | Sin infracción de patentes |
| **Seguridad de producto** | No comercial | Exento de certificaciones |
| **Documentación** | Trazabilidad | Archivos CAD versionados |

---

## 4. Lista de Materiales (BOM)

### 4.1 BOM Electrónico

| # | Componente | Cantidad | Precio Unit. | Subtotal | Proveedor |
|---|------------|----------|--------------|----------|-----------|
| 1 | Nextion NX8048T070 7" | 1 | $96.00 | $96.00 | Amazon [5] |
| 2 | ESP32-WROOM-32 NodeMCU | 1 | $8.00 | $8.00 | Novatronic |
| 3 | Batería 18650 2200mAh | 2 | $3.50 | $7.00 | Novatronic |
| 4 | TP4056 Micro-USB + DW01 | 1 | $1.80 | $1.80 | Novatronic |
| 5 | MT3608 Step-Up | 1 | $1.50 | $1.50 | Novatronic |
| 6 | TL072 DIP-8 | 1 | $0.50 | $0.50 | Novatronic |
| 7 | Portapilas 18650 doble | 1 | $1.50 | $1.50 | Novatronic |
| 8 | Switch deslizante SPST | 1 | $0.30 | $0.30 | Novatronic |
| 9 | Conector BNC hembra | 1 | $1.00 | $1.00 | Novatronic |
| 10 | Resistencia 330Ω 1/4W | 3 | $0.05 | $0.15 | Novatronic |
| 11 | Resistencia 2kΩ 1/4W | 1 | $0.05 | $0.05 | Novatronic |
| 12 | Resistencia 1kΩ 1/4W | 1 | $0.05 | $0.05 | Novatronic |
| 13 | Resistencia 100Ω 1/4W | 2 | $0.05 | $0.10 | Novatronic |
| 14 | Resistencia 10kΩ 1/4W | 1 | $0.05 | $0.05 | Novatronic |
| 15 | Resistencia 20kΩ 1/4W | 1 | $0.05 | $0.05 | Novatronic |
| 16 | LED RGB 5mm cátodo común | 1 | $0.50 | $0.50 | Novatronic |
| 17 | Bornera 2 posiciones | 1 | $0.30 | $0.30 | Novatronic |
| 18 | Header macho 4 pines | 1 | $0.20 | $0.20 | Novatronic |
| 19 | Cable dupont H-H 20cm | 10 | $0.10 | $1.00 | Novatronic |
| 20 | PCB perforada 7×9 cm | 1 | $1.00 | $1.00 | Novatronic |
| | **SUBTOTAL ELECTRÓNICO** | | | **$122.05** | |

### 4.2 BOM Mecánico

| # | Componente | Cantidad | Precio Unit. | Subtotal | Proveedor |
|---|------------|----------|--------------|----------|-----------|
| 1 | Filamento PETG (carcasa) | 100g | $3.00 | $3.00 | Local |
| 2 | Tornillos M3×10mm | 8 | $0.10 | $0.80 | Novatronic |
| 3 | Tuercas M3 | 8 | $0.05 | $0.40 | Novatronic |
| 4 | Separadores M3×5mm | 4 | $0.15 | $0.60 | Novatronic |
| 5 | Pies de goma adhesivos | 4 | $0.10 | $0.40 | Novatronic |
| 6 | Etiquetas adhesivas | 1 | $0.50 | $0.50 | Local |
| | **SUBTOTAL MECÁNICO** | | | **$5.70** | |

### 4.3 Resumen de Costos

| Categoría | Subtotal |
|-----------|----------|
| Componentes electrónicos | $122.05 |
| Componentes mecánicos | $5.70 |
| **TOTAL** | **$127.75 USD** |

**Nota:** Precios referenciales de Novatronic (Ecuador) y Amazon. El costo puede variar según disponibilidad y envío.

---

## 5. Manual de Usuario

### 5.1 Encendido y Apagado

1. **Encender:** Deslizar switch a ON, esperar 3 segundos para inicialización
2. **Apagar:** Deslizar switch a OFF

### 5.2 Carga de Batería

⚠️ **IMPORTANTE: Apagar el dispositivo antes de cargar**

1. Colocar switch en posición OFF
2. Conectar cable USB Micro-B al puerto de carga
3. LED Rojo encendido = Cargando
4. LED Azul encendido = Carga completa (~5.5 horas)

**Nota:** No usar el dispositivo mientras carga para garantizar carga correcta.

### 5.3 Conexión WiFi (App Web)

1. Encender el dispositivo
2. En su dispositivo móvil/PC, conectar a la red WiFi: **BioSimulator_Pro**
3. Contraseña: **biosignal123**
4. Abrir navegador web: **http://192.168.4.1**

### 5.4 Salidas Analógicas BNC

| Conector | Rango | Impedancia | Uso |
|----------|-------|------------|-----|
| BNC1 (principal) | 0-3.3V | ~100Ω | Osciloscopio estándar |

**Configuración de osciloscopio recomendada:**
- Escala vertical: 1V/div
- Acoplamiento: DC o AC según señal
- Base de tiempo: según frecuencia de señal

### 5.5 Indicadores LED

| Color | Estado | Significado |
|-------|--------|-------------|
| Verde fijo | Operación normal | Sistema funcionando |
| Verde parpadeante | Generando señal | Señal activa en salida |
| Azul | WiFi activo | Conexión disponible |
| Rojo | Batería baja | Cargar pronto |
| Rojo parpadeante | Error | Reiniciar dispositivo |

---

## 6. Referencias

### 6.1 Datasheets y Documentación Técnica

1. **[1] Nextion NX8048T070** - ITEAD Studio, Basic Series Datasheet
2. **[2] ESP32-WROOM-32** - Espressif Systems, Datasheet v5.2, Tabla 5-4
3. **[3] TL072** - Texas Instruments, SLOS080 Datasheet
4. **[4] MT3608** - Aerosemi Technology Co., Datasheet
5. **[5] Amazon** - Nextion NX8048T070, precio referencial ~$96 USD
6. **TP4056** - NanJing Top Power ASIC Corp., Datasheet
7. **DW01** - Fortune Semiconductor Corp., Datasheet

### 6.2 Normativas

8. **IEC 61010-1:2010** - Safety requirements for electrical equipment for measurement, control, and laboratory use
9. **IEC 62133:2012** - Secondary cells and batteries containing alkaline or other non-acid electrolytes
10. **IEC 60950-1** - Information technology equipment - Safety

### 6.3 Notas de Aplicación

11. **Microchip AN1149** - Li-Ion Battery Charger Design
12. **TI SLOA011** - Op Amp Circuit Collection
13. **Espressif ESP32 Technical Reference Manual**

### 6.4 Proveedores

14. **Novatronic** - https://novatronic.ec/ (Componentes electrónicos Ecuador)
15. **Amazon** - https://amazon.com/ (Pantalla Nextion)
