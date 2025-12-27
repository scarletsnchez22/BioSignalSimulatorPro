# BioSignalSimulator Pro - Metodología de Diseño Electrónico

**Versión:** 3.0.0  
**Fecha:** Enero 2025  
**Autor:** [Nombre del Tesista]  
**Documento para:** Trabajo de Titulación

---

## Índice

1. [Introducción](#1-introducción)
2. [Metodología de Diseño Electrónico](#2-metodología-de-diseño-electrónico)
3. [Lista de Materiales (BOM Electrónico)](#3-lista-de-materiales-bom-electrónico)
4. [Manual de Usuario](#4-manual-de-usuario)
5. [Referencias](#5-referencias)

---

## 1. Introducción

### 1.1 Propósito del Documento

Este documento describe la metodología completa de diseño electrónico del BioSignalSimulator Pro, un simulador portátil de señales biológicas (ECG, EMG, PPG) para uso educativo. Se profundiza en requisitos, criterios de selección de componentes, normativas aplicadas, cálculos de consumo/autonomía y validación previa al prototipado.

### 1.2 Objetivos Generales del Subsistema Electrónico

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

| Requisito IEC 61010-1 | Implementación en BioSignalSimulator Pro |
|-----------------------|-----------------------------------|
| Protección contra choque eléctrico | Voltaje máximo 5V DC (SELV), corriente limitada < 25 mA por resistencia serie 100Ω |
| Categoría de sobretensión | CAT I (equipos conectados a circuitos sin conexión directa a red) |
| Grado de contaminación | Grado 2 (uso en interiores, ambiente educativo) |
| Aislamiento básico | Carcasa PETG no conductora, batería aislada de salidas |
| Puesta a tierra | Nodo GND único, referencia común para todos los circuitos |
| Protección contra sobrecorriente | DW01 integrado (> 3A), resistencia serie en salida BNC |
| Marcado y etiquetado | Etiquetas de advertencia "Solo uso educativo", voltajes indicados |

#### 2.2.2 IEC 62133: Seguridad de Baterías de Litio

| Requisito | Implementación |
|-----------|----------------|
| Protección sobrecarga | IP5306 corte a 4.2V ±0.5% |
| Protección sobredescarga | DW01 corte a 2.5V |
| Protección cortocircuito | DW01 límite 3A |
| Ventilación | Orificios en carcasa para disipación térmica |

#### 2.2.3 Otras Normativas Consideradas

- IEC 60950-1: Seguridad de equipos de tecnología de la información  
- RoHS: Restricción de sustancias peligrosas en componentes

### 2.3 Criterios y Principios de Diseño Electrónico

#### 2.3.1 Principios Fundamentales

| Principio | Descripción | Aplicación |
|-----------|-------------|------------|
| Simplicidad | Minimizar complejidad del circuito | Uso de módulos integrados (IP5306, XL6009) |
| Disponibilidad | Componentes accesibles localmente | Proveedores Ecuador (Novatronic) |
| Seguridad intrínseca | Diseño inherentemente seguro | Voltajes SELV, corrientes limitadas |
| Eficiencia energética | Maximizar autonomía | Regulador XL6009 η≈88-92% |
| Modularidad | Facilitar mantenimiento | Conectores desmontables, PCB separada |
| Bajo ruido | Señales limpias para osciloscopio | Buffer MCP6002 rail-to-rail, resistencias de precisión |

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

#### 2.4.3 Buffer de Salida: MCP6002-E/SN

| Característica | MCP6002 | TL072 | Comentario |
|----------------|---------|-------|------------|
| Slew Rate | 0.6 V/µs | 13 V/µs | MCP6002 es suficiente para 5 kHz (slew requerido <0.1 V/µs) |
| GBW | 1 MHz | 3 MHz | 1 MHz cubre el ancho de banda educativo (0‑5 kHz) |
| Ruido | 29 nV/√Hz | 18 nV/√Hz | Incremento marginal, imperceptible en el BNC |
| Consumo | **1 mA típico** | 2.5 mA | MCP6002 reduce el consumo total |
| Alimentación | **1.8‑6V, rail-to-rail** | ±2.5‑18V | MCP6002 opera directo a 5 V |
| Encapsulado local | **SOIC-8 disponible** | DIP-8 importado | MCP6002 se consigue en Novatronic |

**Justificación:** El simulador entrega señales de hasta 5 kHz y 3.3 Vpp, por lo que el MCP6002 (rail-to-rail, bajo consumo y disponible localmente) cubre todo el rango sin necesidad de un TL072. Además, al usar el MCP6002 se elimina la conversión de encapsulado y se reduce el consumo de la etapa analógica, mejorando la autonomía.

#### 2.4.4 Sistema de Alimentación

**Baterías 2×18650 en Paralelo (Samsung ICR18650-26H)**

| Parámetro | Por Celda | Pack 2P |
|-----------|-----------|---------|
| Modelo | Samsung 18650 2600 mAh | - |
| Voltaje nominal | 3.7V | 3.7V |
| Capacidad | 2600 mAh | **5200 mAh** |
| Energía | 9.62 Wh | **19.24 Wh** |

**Módulo Cargador IP5306 (Tipo C)**

| Característica | Valor |
|----------------|-------|
| Corriente de carga | 2000 mA (máx) |
| Voltaje de corte | 4.2V ±0.5% |
| Conector | USB Tipo C |
| Protecciones | Sobrecarga, sobredescarga, cortocircuito, temperatura |

**Protector BMS 1S 3A (Modelo 8205A)**

| Característica | Valor |
|----------------|-------|
| Topología | PCM 1S Li-ion |
| Corriente continua | 2 A (3 A pico) |
| V protección carga | 4.25 ±0.05 V |
| V protección descarga | 2.54 ±0.10 V |
| Protecciones | Sobre/infra voltaje, cortocircuito |
| Dimensiones | 40 × 4 × 3 mm |

**Regulador XL6009 Step-Up**

| Parámetro | Valor |
|-----------|-------|
| Voltaje entrada | 3V - 32V |
| Voltaje salida | 5V (ajustado) |
| Corriente máxima | 4A |
| Eficiencia @ 0.8A | 92% |
| Eficiencia @ 1.2A | 88% |

**Cadena energética:** USB 5V (IP5306) → BMS 1S 3A → Pack 2×18650 → Switch → XL6009 → ESP32/Nextion/MCP6002. El BMS garantiza protección celda-celda antes del elevador, mientras el IP5306 gestiona el perfil CC/CV y el XL6009 entrega 5V estable al sistema.

### 2.5 Proceso y Arquitectura

#### 2.5.1 Metodología

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
│                    BIOSIGNALSIMULATOR PRO v3.0                  │
├──────────────────────────────────────────────────────────────────┤
│                                                                  │
│  USB 5V ──► IP5306 ──► BMS 1S 3A ──► BATERÍAS 2×18650 (5200mAh) │
│            (Tipo C)         (PCM)              │                  │
│                                               ▼                  │
│                                         SWITCH ON/OFF             │
│                                               │                  │
│                                               ▼                  │
│                                    XL6009 (3.7V→5V, η≈88-92%)    │
│                                               │                  │
│                            ┌───────────────────┼────────────────┐│
│                            ▼                   ▼                ▼│
│                         ESP32               NEXTION       MCP6002│
│                       WROOM-32            7" 800×480       Buffer│
│                                                          └─► BNC │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘

#### 2.5.3 Acondicionamiento de la salida analógica

Para asegurar que la forma de onda enviada al BNC mantenga la banda útil (0‑500 Hz) y llegue suavizada (sin escalones del DAC), se añadió un filtro pasabajos RC a la salida del MCP6002:

- **Resistencia serie:** 100 Ω ubicada entre la salida del MCP6002 y la bornera "BNC_OUT". Además de definir fc junto con el capacitor, protege al op-amp ante cortos o cargas capacitivas externas.
- **Capacitor de filtrado:** 1 µF cerámico X7R conectado entre el nodo filtrado y GND.

El punto de corte del filtro viene dado por:

```
f_c = 1 / (2π × R × C)
    = 1 / (2π × 100 Ω × 1 µF)
    ≈ 1.59 kHz
```

**Justificación del diseño:**

- **Señales biomédicas:** ECG (0-50 Hz), EMG (0-500 Hz), PPG (0-10 Hz) pasan sin atenuación apreciable (fc >> fmax).
- **Stepping del DAC:** El DAC del ESP32 opera a 4 kHz (Fs_timer). Con fc = 1.59 kHz, los armónicos del stepping se atenúan ~8 dB a 4 kHz y ~20 dB a 16 kHz, suavizando visualmente la señal en el osciloscopio.
- **Ripple residual del XL6009:** A 400 kHz, la atenuación es >48 dB, eliminando cualquier componente de conmutación que haya pasado el filtro π.

> **Nota:** Se eligió 1 µF (en lugar de 100 nF) para colocar fc entre la frecuencia máxima de las señales biomédicas (500 Hz) y la frecuencia de muestreo del DAC (4 kHz), cumpliendo el criterio de filtro de reconstrucción: fmax < fc < Fs/2.

#### 2.5.4 Consumos y Autonomía

**Tabla de Consumos del Sistema**

| Componente | I Promedio | P Promedio @5V | I Pico | P Pico @5V | Referencia |
|------------|------------|----------------|--------|------------|------------|
| Nextion NX8048T070 | 510 mA | 2.55 W | 650 mA | 3.25 W | Datasheet Basic Series [1] |
| ESP32-WROOM-32 (WiFi AP) | 240 mA | 1.20 W | 350 mA | 1.75 W | ESP32 Datasheet v5.2, Tabla 5-4 [2] |
| MCP6002 Buffer (2 canales) | **1 mA** | **0.005 W** | **1 mA** | **0.005 W** | Microchip MCP6002 Datasheet [3] |
| LED RGB + divisor UART | 32 mA | 0.16 W | 32 mA | 0.16 W | Cálculo: Vf≈2.0V (R), 3.0V (G/B) |
| XL6009 (pérdidas) | 70 mA equiv. | 0.35 W | 163 mA equiv. | 0.82 W | XL6009 Datasheet, η≈92%/88% [4] |
| **TOTAL** | **853 mA** | **4.27 W** | **1196 mA** | **6.00 W** | |

**Nota:** El consumo pico representa un escenario extremo donde todos los subsistemas demandan simultáneamente (brillo 100%, WiFi TX continuo, LED RGB encendido). En uso normal, el sistema opera en modo promedio.

**Parámetros del sistema:**
- Capacidad nominal batería: 5200 mAh (2×2600 mAh en paralelo)
- Capacidad útil (93%): **4836 mAh**
- Voltaje nominal batería: 3.7V
- Eficiencia XL6009: 92% @ 0.8A (modo promedio), 88% @ 1.2A (modo pico)
- Protección celda: BMS 1S 3A (8205A) entre IP5306 y pack

**Cálculo de autonomía - Modo Promedio (uso normal):**

```
1. P_sistema = 5V × 0.857A = 4.29 W
2. P_batería = 4.29W / 0.92 = 4.66 W
3. I_batería = 4.66W / 3.7V = 1.26 A
4. t = 4836 mAh / 1260 mA = 3.84 h
5. t_práctica ≈ 3.8 horas
```

**Cálculo de autonomía - Modo Pico (escenario extremo):**

```
1. P_sistema = 5V × 1.20A = 6.00 W
2. P_batería = 6.00W / 0.88 = 6.82 W
3. I_batería = 6.82W / 3.7V = 1.84 A
4. t = 4836 mAh / 1840 mA = 2.63 h
5. t_práctica ≈ 2.6 horas
```

El dispositivo está diseñado para cubrir **dos clases consecutivas de 1.5 horas** con WiFi siempre activo en modo promedio. La autonomía en modo pico (todos los subsistemas al máximo simultáneamente) sigue siendo suficiente para sesiones largas.

> **Nota de operación:** Las autonomías anteriores consideran el panel Nextion al **100 % de brillo** (escenario de mayor demanda visual). Reducir el brillo al 70‑80 % incrementa la autonomía práctica en ~20 min por sesión sin comprometer la legibilidad en laboratorios típicos.

#### 2.5.5 Tiempo de Carga

- Capacidad total: 5200 mAh  
- Corriente de carga: 2000 mA (IP5306 máx)  
- Tiempo estimado: 2.6 h (fase CC) + 0.65 h (fase CV) ≈ **3.3 horas**  
- Condición: dispositivo apagado durante carga para respetar perfil CC/CV del IP5306.
- **Advertencia:** El cargador IP5306 comparte los nodos B+/B− con el elevador. Siempre apagar el switch antes de conectar el USB para evitar corrientes inestables y posibles daños.

#### 2.5.6 Principios aplicados al diseño de la PCB

| Principio | Implementación en la placa de control |
|-----------|---------------------------------------|
| Plano de tierra dedicado | Todo el cobre se concentró en la cara inferior: allí se vierte el plano GND y se rutearon las pistas, dejando la cara superior únicamente con pads de componentes para facilitar la soldadura. |
| Cosido de GND (stitching vias) | Se eliminó el cosido porque el nuevo enfoque es monocapa efectiva (solo bottom). El plano inferior mantiene continuidad y baja impedancia mediante spokes amplios alrededor de cada pad. |
| Ruteo 0°/45° | Las pistas se trazaron con segmentos ortogonales y giros de 45° para mejorar manufacturabilidad y mantener distancias constantes entre nets. |
| Separación de potencia y señal | Aunque todas las pistas van por la cara inferior, se organizaron en zonas: los buses de potencia/retorno ocupan el contorno (ancho ≥ 1.5 mm) y las señales se mantienen al centro con clearances de 1.0 mm para evitar acoples. |
| Componentes en top layer | Todos los módulos, headers y borneras permanecen en la cara superior; al no tener cobre en top, la soldadura se realiza desde abajo sin riesgo de puentes accidentales. |
| Control de anchos | Se fijó 1.2 mm para alimentación/retornos críticos y 1.0 mm para señales, manteniendo resistencia baja y respetando el clearance frente a pads y tornillos. |
| Keepouts mecánicos | Se definieron zonas de exclusión alrededor de los cuatro tornillos y a lo largo del contorno para evitar que las arandelas o la base metálica toquen cobre expuesto. |
| Etiquetado funcional | Todas las borneras y conectores se rotularon (VCC, GND, BNC, LED, NEXTION) en serigrafía amarilla para facilitar montaje y mantenimiento. |
| Filtro RC situado en el borde | El resistor serie de 100 Ω y el capacitor de 1 µF X7R se colocaron a <5 mm del BNC para minimizar inductancias parásitas y garantizar la fc calculada (1.59 kHz). |

### 2.6 Limitaciones del Diseño Electrónico

| Limitación | Descripción | Mitigación |
|------------|-------------|------------|
| Sin power path | No permite uso durante carga | **Obligatorio apagar switch antes de cargar**. Documentado en manual |
| Carga con equipo encendido | Corrientes inestables, riesgo de daño | Advertencia clara en manual y etiqueta física |
| Baterías sin balanceador activo | El pack 2P depende del BMS 1S 3A para cortes seguros | BMS 8205A integrado entre IP5306 y pack; pruebas periódicas |
| DAC 8-bit | Resolución limitada (256 niveles) | Suficiente para señales educativas |
| Autonomía variable | 2.6h (pico) a 3.8h (promedio) | Diseñado para 2 clases de 1.5h cada una |
| Tiempo de carga | 3.3 horas con IP5306 @ 2A | Carga rápida vs. TP4056 (6.5h @ 1A) |

#### 2.6.1 Justificación: sin carga/uso simultáneo

- **Balance energético desfavorable.** El sistema requiere 5 V × 1.2 A ≈ 6 W en operación. Para cargar el pack (6.82 W incluyendo eficiencia del 88 %), el IP5306 necesitaría entregar **>12 W** en modo power-path. Aunque se conecte un adaptador USB-C de 3 A, el IP5306 limita su salida combinada (boost + cargador) a ~2 A (dato de hoja técnica), por lo que faltan **>2.8 W** para cerrar el balance y el chip termina entrando en protección por caída de tensión (undervoltage lockout). *Este cálculo toma el peor caso real medido (picos simultáneos de 1.2 A) para garantizar que la restricción cubra cualquier escenario de laboratorio.*
- **Tiempo de carga si se insistiera en uso simultáneo.** Con 2 A máximos compartidos, 1.2 A se los lleva la placa y solo 0.8 A quedarían para recargar la batería. Eso duplicaría el tiempo de carga (5200 mAh / 0.8 A ≈ 6.5 h + fase CV) y obliga al IP5306 a disipar más de 2.5 W continuos. En la práctica el fabricante solo garantiza **1–1.2 A continuos**, igual al consumo del simulador, así que la batería no recibe corriente (o se sigue descargando) aunque el adaptador esté conectado, desperdiciando energía y ciclando el pack.
- **Limitaciones del IP5306.** Aunque integra boost y cargador 1S, no posee “load sharing” nativo; cuando detecta carga USB activa el modo de carga y deshabilita el boost. Al intentar alimentar la placa mientras se carga la batería se generan corrientes recirculantes B+/B−→EN que sobrecalientan el chip (comportamiento observado en prototipos).
- **Power-path dedicados no disponibles localmente.** Controladores como **MCP73871, CN3791 con ideal-diode** o arreglos con MOSFET back-to-back son escasos en Ecuador: proveedores locales (Novatronic, Velasco Store, Kiwi) no los tienen en stock y la importación toma 4‑6 semanas. Los módulos listos (Adafruit PowerBoost, LTC4412 boards) triplican el costo del subsistema y elevan la altura del chasis.
- **Fiabilidad del usuario final.** Separar físicamente las placas y exigir “cargar con el dispositivo apagado” garantiza que el IP5306 opere en su ventana nominal (perfil CC/CV) y evita ciclos de conexión/desconexión que podrían dañar la Nextion y el ESP32 durante demostraciones.

Por estos motivos se mantuvo la restricción operacional: **no se debe usar el simulador mientras se carga**. El manual y las etiquetas en la carcasa remarcan el procedimiento seguro.

### 2.7 Implicaciones Éticas y Legales - Electrónico

#### 2.7.1 Consideraciones Éticas

| Aspecto | Consideración | Acción |
|---------|---------------|--------|
| Uso previsto | Solo simulación educativa | Etiquetado claro "NO PARA USO CLÍNICO" |
| Seguridad del usuario | Prevenir lesiones | Diseño SELV, corrientes limitadas |
| Transparencia | Documentación completa | Esquemas y cálculos públicos |
| Accesibilidad | Costo reducido | Componentes locales, diseño replicable |

#### 2.7.2 Consideraciones Legales

| Aspecto | Requisito | Cumplimiento |
|---------|-----------|--------------|
| Clasificación | Equipo educativo, no médico | No requiere certificación médica |
| Marcado CE/FCC | No aplica para prototipo educativo | Documentar limitaciones |
| Propiedad intelectual | Componentes comerciales estándar | Sin restricciones de patentes |
| Responsabilidad | Uso bajo supervisión académica | Descargo en documentación |

---

## 3. Lista de Materiales (BOM Electrónico)

El sistema se divide en tres etapas funcionales ordenadas cronológicamente según el flujo de energía: **potencia** (baterías → BMS → carga → boost), **filtrado** (protección y acondicionamiento π) y **control** (UI, generación y acondicionamiento de señales). Los componentes se agrupan metodológicamente en activos y pasivos dentro de cada etapa.

### 3.1 BOM Unificado del Sistema

#### 3.1.1 Etapa de Potencia — Módulos Activos

| # | Componente | Cantidad | Precio Unit. | Subtotal | Proveedor |
|---|------------|----------|--------------|----------|-----------|
| 1 | Batería Samsung 18650 2600 mAh 3.7 V | 2 | $9.00 | $18.00 | DCI Ecuador [7] |
| 2 | Protector BMS 1S 3 A (8205A) | 1 | $2.00 | $2.00 | AV Electronics [11] |
| 3 | Módulo IP5306 cargador USB-C 2 A | 1 | $3.50 | $3.50 | Velasco Store [8] |
| 4 | Módulo XL6009 Step-Up DC-DC 4 A | 1 | $2.50 | $2.50 | UNIT Electronics [9] |
| | **Subtotal módulos activos potencia** | | | **$26.00** | |

#### 3.1.2 Etapa de Potencia — Componentes Pasivos y Mecánicos

| # | Componente | Cantidad | Precio Unit. | Subtotal | Proveedor |
|---|------------|----------|--------------|----------|-----------|
| 5 | Portapilas 18650 doble (2P paralelo) | 1 | $1.80 | $1.80 | Novatronic |
| 6 | Switch deslizante SPST | 1 | $0.35 | $0.35 | Novatronic |
| 7 | Bornera 2 pines paso 8.05 mm | 3 | $0.50 | $1.50 | Novatronic |
| 8 | Cable sólido AWG22 rojo/negro 1 m | 1 | $0.80 | $0.80 | Novatronic |
| 9 | Tornillos M3×10 mm (4 uds, fijación XL6009) | 1 | $0.40 | $0.40 | Ferretería local |
| | **Subtotal pasivos/mecánicos potencia** | | | **$4.85** | |

#### 3.1.3 Etapa de Filtrado — Plaquita Dedicada

| # | Componente | Cantidad | Precio Unit. | Subtotal | Proveedor |
|---|------------|----------|--------------|----------|-----------|
| 10 | Portafusible 5×20 mm BLX-A | 1 | $0.70 | $0.70 | Novatronic |
| 11 | Fusible vidrio 5×20 mm 1.5 A | 1 | $0.30 | $0.30 | Novatronic |
| 12 | Inductor 22 µH / 3 A (9×12 mm, pitch 5 mm) | 1 | $0.60 | $0.60 | Novatronic |
| 13 | Capacitor electrolítico 470 µF / 25 V (8×14 mm, pitch 3.5 mm) | 1 | $0.35 | $0.35 | Novatronic |
| 14 | Capacitor cerámico 1 µF / 16 V X7R (0805) | 1 | $0.10 | $0.10 | Novatronic |
| 15 | Conector 2 pines paso 8.05 mm (PWR_XL6009 / PWR_BNC) | 2 | $0.50 | $1.00 | Novatronic |
| 16 | PCB perforada 5×7 cm (plaquita filtrado) | 1 | $0.80 | $0.80 | Novatronic |
| 17 | Tornillos M1.6×6 mm (4 uds, montaje plaquita) | 1 | $0.40 | $0.40 | Ferretería local |
| | **Subtotal etapa de filtrado** | | | **$4.25** | |

**Principios aplicados a la plaquita de filtrado**

1. **Cara única (Bottom layer) para ruteo compacto:** el cobre quedó únicamente en la cara inferior, donde se rutearon todas las pistas y el plano GND; la cara superior sólo mantiene los pads de los componentes para soldarlos desde abajo sin cruzar pistas.
2. **Anchos diferenciados:** pistas de potencia (VIN_CTRL, +5 V_CTRL) con ancho ≥1.2 mm; señales de sensado/monitorización a 1.0 mm para mantener resistencia baja sin complicar el ruteo.
3. **Componentes agrupados:** F1, C14, L1 y C15 se ubicaron a menos de 15 mm entre sí para minimizar lazo de alta frecuencia y asegurar la atenuación calculada (43 dB @ 400 kHz).
4. **Conectores enfrentados:** las borneras PWR_XL6009/PWR_BNC se alinearon para que los cables entren y salgan en línea recta, reduciendo tensión mecánica sobre el filtro.
5. **Ruta corta a chasis:** los orificios de montaje M1.6 se colocaron junto a la entrada VIN_CTRL para atornillar la plaquita directamente al chasis metálico y mantener el cableado ordenado. El plano GND inferior se conecta a estos tornillos mediante pads expuestos para asegurar referencia común con la carcasa.

#### 3.1.4 Etapa de Control — Módulos Activos

| # | Componente | Cantidad | Precio Unit. | Subtotal | Proveedor |
|---|------------|----------|--------------|----------|-----------|
| 18 | Nextion NX8048T070 7" 800×480 | 1 | $95.75 | $95.75 | Amazon [5] |
| 19 | ESP32-WROOM-32 NodeMCU | 1 | $13.35 | $13.35 | Novatronic [6] |
| 20 | MCP6002-E/SN (SOIC-8) | 1 | $1.20 | $1.20 | Novatronic |
| 21 | Adaptador SOIC-8 a DIP-8 | 1 | $0.40 | $0.40 | Novatronic |
| 22 | LED RGB 5 mm cátodo común | 1 | $0.50 | $0.50 | Novatronic |
| | **Subtotal módulos activos control** | | | **$111.20** | |

#### 3.1.5 Etapa de Control — Componentes Pasivos y Conectores

| # | Componente | Cantidad | Precio Unit. | Subtotal | Proveedor |
|---|------------|----------|--------------|----------|-----------|
| 23 | Resistencia 220 Ω 1/4 W | 3 | $0.05 | $0.15 | Novatronic |
| 24 | Resistencia 2 kΩ 1/4 W | 1 | $0.05 | $0.05 | Novatronic |
| 25 | Resistencia 1 kΩ 1/4 W | 1 | $0.05 | $0.05 | Novatronic |
| 26 | Resistencia 100 Ω 1/4 W | 2 | $0.05 | $0.10 | Novatronic |
| 27 | Capacitor cerámico 1 µF / 16 V X7R (filtro BNC) | 1 | $0.10 | $0.10 | Novatronic |
| 28 | Conector BNC hembra | 1 | $1.20 | $1.20 | Novatronic |
| 29 | Header macho 4 pines | 1 | $0.20 | $0.20 | Novatronic |
| 30 | Conector JST-XH 4 pines (Nextion) | 1 | $0.60 | $0.60 | Novatronic |
| 31 | Bornera 2 pines paso 8.05 mm (PWR_IN / BNC) | 2 | $0.50 | $1.00 | Novatronic |
| 32 | Bornera 4 pines paso 8.05 mm (LED RGB / NEXTION) | 2 | $0.80 | $1.60 | Novatronic |
| 33 | PCB perforada 10×15 cm (placa control) | 1 | $2.00 | $2.00 | Novatronic |
| 34 | Tornillos M3×10 mm (4 uds, montaje placa control) | 1 | $0.40 | $0.40 | Ferretería local |
| 35 | Tornillos M4×12 mm (6 uds, soporte chasis) | 1 | $0.60 | $0.60 | Ferretería local |
| | **Subtotal pasivos/conectores control** | | | **$8.05** | |

---

**TOTAL SISTEMA ELECTRÓNICO:** $26.00 + $4.85 + $4.25 + $111.20 + $8.05 = **$154.35**

### 3.2 Filtro y Protección de la Etapa Elevadora

Para minimizar el rizado del XL6009 y salvaguardar la placa de control se añadió un filtro π con fusible reemplazable en serie (portafusible 5×20 mm + fusible de 1.5 A). El esquema conceptual es:

```
        +5 V del XL6009
                │
          [F1]  5×20 mm 1.5 A
                │─────── Nodo VIN_CTRL
                │
               ├── C14 = 470 µF / 25 V (electrolítico, 8×14 mm)
               │
             [L1]  Inductor 22 µH / 3 A (9×12 mm)
               │─────── +5 V_CTRL hacia placa de control
               │
               └── C15 = 1 µF (cerámico X7R) → GND
```

- **F1 (portafusible BLX-A + fusible 5×20 mm 1.5 A)** se abre ante sobrecorriente >1.5 A, protegiendo el bus de 5 V antes de que llegue a los módulos sensibles. Es reemplazable y accesible desde la plaquita de filtrado.  
- **C14** absorbe los picos de corriente del elevador antes de la bobina.  
- **L1** y **C15** conforman un filtro LC de segundo orden que atenúa el rizado de conmutación y mantiene limpio el bus que alimenta el MCP6002 y el ESP32. La frecuencia de corte aproximada es:

```
f_c = 1 / (2π √(L × C15))
f_c = 1 / (2π √(22 µH × 1 µF))
f_c ≈ 34 kHz
```

El XL6009 conmuta a **400 kHz** según su datasheet [4]. Con f_c = 34 kHz, la relación es 400/34 ≈ 11.8× (1.07 décadas). Un filtro LC de segundo orden atenúa a -40 dB/década, por lo que a 400 kHz se obtiene:

```
Atenuación ≈ 1.07 décadas × 40 dB/década ≈ 43 dB
```

Esto reduce un ripple típico de 50-100 mV a menos de **1 mV** en la salida, suficiente para la etapa analógica del MCP6002. La caída DC se mantiene por debajo de 0.1 V (solo la resistencia del fusible y la DCR del inductor).

> **Nota de diseño:** Se eligió C15 = 1 µF cerámico X7R para colocar f_c a ~1/10 de la frecuencia de switching, cumpliendo la recomendación de Texas Instruments para filtros LC en convertidores DC-DC [TI SLVA462]. El capacitor electrolítico C14 (470 µF) actúa como reserva de energía y su ESR (50-200 mΩ típico) proporciona amortiguamiento natural que evita resonancias.

#### 3.2.1 Ubicación Física

El submódulo de filtrado se implementa en una plaquita dedicada (PCB 5×7 cm) que se intercala entre el XL6009 y la placa de control. Esta plaquita solo contiene F1, C14, L1 y C15, y actúa como frontera: a su entrada se conectan los módulos de potencia (BMS, IP5306, XL6009) y a su salida se alimenta la placa de control mediante el conector VIN_CTRL. Se monta con 4 tornillos M1.6×6 mm en la parte inferior del chasis.

## 4. Manual de Usuario

### 4.1 Encendido y Apagado

1. **Encender:** Deslizar switch a ON, esperar 3 segundos para inicialización  
2. **Apagar:** Deslizar switch a OFF

### 4.2 Carga de Batería

⚠️ **IMPORTANTE: Apagar el dispositivo antes de cargar**

⚠️ **ADVERTENCIA DE SEGURIDAD:** El cargador IP5306 comparte los nodos B+/B− con el elevador. Intentar cargar con el equipo encendido puede causar corrientes inestables, sobrecalentamiento y daños permanentes al sistema.

1. **Colocar switch en posición OFF** (obligatorio)  
2. Conectar cable USB Tipo C al puerto de carga  
3. Observar LEDs integrados del IP5306: Rojo = Cargando, Azul = Completa
4. Tiempo estimado de carga: ~3.3 horas
5. Desconectar cable USB antes de encender el dispositivo  

### 4.3 Conexión WiFi (App Web)

1. Encender el dispositivo  
2. Conectar a la red **BioSignalSimulator_Pro** (contraseña **biosignal123**)  
3. Abrir **http://192.168.4.1** para acceder a la app web  

### 4.4 Salidas Analógicas BNC

| Conector | Rango | Impedancia | Uso |
|----------|-------|------------|-----|
| BNC1 (principal) | 0-3.3V | ~100Ω | Osciloscopio estándar |

**Configuración recomendada:** 1 V/div, acoplamiento DC/AC según señal, base de tiempo acorde a frecuencia.

### 4.5 Indicadores LED

| Color | Estado | Significado |
|-------|--------|-------------|
| Verde | Simulando | Señal activa, generando forma de onda |
| Amarillo | Alimentado/Fuera de simulación | Sistema encendido, sin generar señal |
| Rojo | Pausado/Stop | Simulación detenida o en pausa |

---

## 5. Referencias

[1] Nextion Technology Co., Ltd. (2023). *NX8048T070 Basic Series Datasheet*. Recuperado de https://nextion.tech/datasheets/nx8048t070/

[2] Espressif Systems. (2023). *ESP32 Series Datasheet v5.2*. Tabla 5-4: Consumo de corriente WiFi. Recuperado de https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf

[3] Microchip Technology. (2023). *MCP6001/1R/1U/2/4 1 MHz, Low-Power Op Amp Datasheet*. Recuperado de https://ww1.microchip.com/downloads/en/DeviceDoc/MCP6001-1R-1U-2-4-1-MHz-Low-Power-Op-Amp-DS20001733L.pdf

[4] XLSEMI. (2023). *XL6009 400kHz 4A 40V Buck DC to DC Converter Datasheet*. Recuperado de https://www.xlsemi.com/datasheet/XL6009%20datasheet.pdf

[5] Amazon.com. (2024). *Nextion NX8048T070 7" HMI Display*. Precio: $95.75. Recuperado de https://www.amazon.com/

[6] Novatronic Ecuador. (2024). *ESP-WROOM-32 ESP32S WIFI Bluetooth*. Precio: $13.35. Recuperado de https://novatronicec.com/

[7] DCI Ecuador. (2024). *Batería Recargable 18650 3.7V 2600mAh Samsung*. Precio: $9.00. Recuperado de https://dcimecuador.com/

[8] Velasco Store. (2024). *Módulo de Carga Tipo C IP5306 5V-2A*. Precio: $3.50. Recuperado de https://velascostore.com/

[9] UNIT Electronics. (2024). *XL6009 Elevador de Voltaje Boost Step Up 10W 3A*. Precio: $2.50. Recuperado de https://uelectronics.com/

[10] Kiwi Ecuador. (2024). *Bornera P/Cable H 60A 25mm - WRT*. Precio: $3.10. Recuperado de tienda física Kiwi, Ecuador.

[11] AV Electronics. (2024). *Protector BMS 1S 3A 3.7V (Modelo 8205A)*. Precio: $2.00. Recuperado de https://avelectronics.cc/producto/protector-bms-1s-3a-3-7v/

[12] Texas Instruments. (2011). *SLVA462: Input and Output Capacitor Selection for Voltage Regulators*. Recuperado de https://www.ti.com/lit/an/slva462/slva462.pdf

---

**Normas y Estándares:**
- IEC 61010-1:2010 - Safety requirements for electrical equipment
- IEC 62133:2012 - Safety of secondary lithium cells
- IEC 60950-1 - Information technology equipment - Safety
