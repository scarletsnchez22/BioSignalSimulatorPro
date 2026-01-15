# Metodología de Diseño Electrónico del Sistema BioSignalSimulator Pro

**Grupo #22:** Scarlet Sánchez y Rafael Mata  
**Institución:** Escuela Superior Politécnica del Litoral (ESPOL)  
**Documento de Sustentación Técnica para Trabajo de Titulación**

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
| Protección contra sobrecorriente | Fusible 1.5A a salida XL6009 + DW01 integrado (límite 3A) |
| Marcado y etiquetado | Etiquetas de advertencia "Solo uso educativo", voltajes indicados |

#### 2.2.2 IEC 62133: Seguridad de Baterías de Litio

| Requisito | Implementación |
|-----------|----------------|
| Protección sobrecarga | IP5306 corte a 4.2V ±0.5% |
| Protección sobredescarga | DW01 corte a 2.5V |
| Protección cortocircuito batería | DW01 límite 3A |
| Protección cortocircuito carga | Fusible 1.5A a salida 5V |
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

> **Nota:** La Nextion NX8048T070 opera con alimentación de 5V pero acepta niveles lógicos TTL de 3.3V en sus pines RX/TX sin necesidad de divisores resistivos, por lo que se conecta directamente al ESP32 (GPIO16/17 UART2).

#### 2.4.3 Buffer de Salida: LM358 (Implementado) vs MCP6002 (Ideal)

**Análisis comparativo de opciones:**

| Característica | LM358 (Usado) | MCP6002 (Ideal) | TL072 | Comentario |
|----------------|---------------|-----------------|-------|------------|
| Slew Rate | 0.3 V/µs | 0.6 V/µs | 13 V/µs | LM358/MCP6002 suficientes para 5 kHz |
| GBW | 0.7 MHz | 1 MHz | 3 MHz | Ambos cubren BW educativo (0-5 kHz) |
| Ruido | 40 nV/√Hz | 29 nV/√Hz | 18 nV/√Hz | Aceptable para aplicación educativa |
| Consumo | 0.7 mA | **1 mA** | 2.5 mA | LM358 es eficiente |
| Alimentación | 3-32V single | **1.8-6V rail-to-rail** | ±5-18V | LM358 opera a 5V single |
| Vout máx. | VCC-1.5V | **Rail-to-rail** | VCC-3V | LM358 limita a ~3.5V con 5V |
| Disponibilidad Ecuador | **DIP-8 común** | Difícil importar | Importar | LM358 disponible localmente |

**Decisión de implementación:**

- **Buffer ideal:** MCP6002 (rail-to-rail, mejor para 3.3V del ESP32)
- **Buffer implementado:** LM358 (disponible localmente en Ecuador)

**Justificación:** Aunque el MCP6002 sería ideal por su característica rail-to-rail que permite aprovechar el rango completo 0-3.3V del DAC del ESP32, se implementó el LM358 debido a su disponibilidad inmediata en el mercado local ecuatoriano. El LM358 opera correctamente a 5V single-supply y, aunque su salida máxima es ~VCC-1.5V (≈3.5V), esto es suficiente para la señal del DAC (0-3.3V). Para futuras versiones se recomienda importar el MCP6002 para obtener mejor fidelidad en los extremos del rango.

#### 2.4.4 Sistema de Alimentación

**Baterías 2×18650 en Paralelo (Samsung ICR18650-26H)**

| Parámetro | Por Celda | Pack 2P |
|-----------|-----------|---------|
| Modelo | Samsung 18650 2600 mAh | - |
| Voltaje nominal | 3.7V | 3.7V |
| Capacidad | 2600 mAh | **5200 mAh** |
| Energía | 9.62 Wh | **19.24 Wh** |

---

### ⚡ 2.4.5 Análisis de Tasas de Carga y Descarga de Baterías Samsung ICR18650

> **⚠️ SECCIÓN CRÍTICA DE SEGURIDAD:** Esta información es VITAL para garantizar la operación segura del sistema y prolongar la vida útil de las baterías. El incumplimiento de las tasas de carga/descarga puede causar sobrecalentamiento, venteo térmico, o en casos extremos, incendio.

#### 2.4.5.1 Especificaciones Oficiales de la Celda Samsung ICR18650-26J

La celda Samsung ICR18650-26J (también denominada 26H en algunas versiones) es una batería de iones de litio de alta capacidad utilizada en nuestro sistema. Las especificaciones fueron obtenidas de fuentes técnicas verificadas:

| Parámetro | Valor | Unidad | Fuente |
|-----------|-------|--------|--------|
| **Modelo** | Samsung ICR18650-26J | - | Datasheet Samsung SDI |
| **Química** | LiNiCoMnO2 (NCM) | - | Datasheet |
| **Capacidad nominal** | 2600 | mAh | Datasheet |
| **Capacidad mínima** | 2550 | mAh | Datasheet |
| **Voltaje nominal** | 3.6 | V | Datasheet |
| **Voltaje de carga** | 4.20 ± 0.05 | V | Datasheet |
| **Voltaje mínimo descarga** | 2.75 | V | Datasheet |
| **Peso** | 45 ± 0.01 | g | Datasheet |
| **Dimensiones** | Ø18.20 × 64.80 | mm | Datasheet |

**Fuentes de información consultadas:**
- **Lygte-Info (Dinamarca):** [https://lygte-info.dk/review/batteries2012/Samsung%20ICR18650-26J%202600mAh%20UK.html](https://lygte-info.dk/review/batteries2012/Samsung%20ICR18650-26J%202600mAh%20UK.html) - Análisis independiente con curvas de descarga medidas en laboratorio.
- **Datasheet Samsung SDI:** Especificaciones oficiales del fabricante.

#### 2.4.5.2 Tasas de Carga (Charge Rate) - Análisis Detallado

La tasa de carga se expresa en "C", donde 1C equivale a la capacidad de la batería en mA. Para una celda de 2600 mAh:

| Tasa (C-rate) | Corriente (mA) | Tiempo aprox. carga | Uso recomendado |
|---------------|----------------|---------------------|-----------------|
| **0.5C** | 1300 mA | ~2.5 horas | ✅ **Recomendado** - Máxima longevidad |
| **1C** | 2600 mA | ~1.3 horas | ⚠️ Carga estándar |
| **2C** | 5200 mA | ~40 minutos | ❌ Máximo absoluto (reduce vida útil) |

**Especificación Samsung oficial:** La celda ICR18650-26J admite **carga máxima de 2C (5.2A)** según el datasheet, pero esto es el límite absoluto que reduce significativamente la vida útil de la batería.

> **🔋 REGLA DE ORO:** Para maximizar ciclos de vida, cargar a **0.5C o menos** (≤1.3A por celda).

#### 2.4.5.3 Tasas de Descarga (Discharge Rate) - Análisis Detallado

| Tasa (C-rate) | Corriente (mA) | Aplicación | Comportamiento |
|---------------|----------------|------------|----------------|
| **0.2C** | 520 mA | Descarga muy suave | Capacidad ~2650 mAh (100%) |
| **0.5C** | 1300 mA | Descarga normal | Capacidad ~2550 mAh (96%) |
| **1C** | 2600 mA | Descarga estándar | Capacidad ~2500 mAh (94%) |
| **2C** | 5200 mA | Descarga alta | Capacidad ~2350 mAh (88%) |
| **3C** | 7800 mA | Límite alto | Temperatura elevada, caída de voltaje |
| **4C** | 10400 mA | ❌ Excesiva | **NO RECOMENDADO** - Daño potencial |

**Observaciones de pruebas independientes (Lygte-Info):**
- A 5A de descarga, la celda funciona pero con calentamiento significativo.
- A 7A, la temperatura supera los 60°C.
- A 10A, la celda **no puede mantener** el voltaje adecuadamente y se recomienda evitar esta tasa.

> **📊 LÍMITE PRÁCTICO SEGURO:** Descarga máxima continua de **2C (5.2A)** por celda, aunque se recomienda **≤1C** para operación óptima.

#### 2.4.5.4 Compatibilidad del IP5306 con las Tasas de Carga Samsung

El módulo cargador **IP5306** fue seleccionado específicamente porque respeta las tasas de carga seguras de las celdas Samsung:

| Parámetro IP5306 | Valor | Compatibilidad con Samsung ICR18650-26J |
|------------------|-------|------------------------------------------|
| **Corriente de carga máxima** | 2000 mA (2A) | ✅ **SEGURO** - Equivale a ~0.77C por celda |
| **Perfil de carga** | CC/CV (Corriente Constante / Voltaje Constante) | ✅ Correcto para Li-ion |
| **Voltaje de corte** | 4.2V ± 0.5% | ✅ Exactamente lo requerido por Samsung |
| **Detección de fin de carga** | Corriente < 100 mA | ✅ Terminación segura |

**Análisis de compatibilidad (Pack 2P = 2 celdas en paralelo):**

```
Configuración: 2× Samsung ICR18650-26J en paralelo
Capacidad total: 5200 mAh (2600 mAh × 2)

Corriente de carga IP5306: 2000 mA (2A)
Corriente por celda: 2000 mA ÷ 2 = 1000 mA (1A por celda)

Tasa de carga por celda: 1000 mA ÷ 2600 mAh = 0.38C

✅ RESULTADO: La tasa de 0.38C está MUY POR DEBAJO del máximo de 2C
✅ BENEFICIO: Carga rápida (2A total) sin estresar las celdas
✅ LONGEVIDAD: Al cargar a <0.5C, se maximizan los ciclos de vida
```

**¿Por qué el IP5306 es la opción correcta?**

| Ventaja | Explicación |
|---------|-------------|
| **Carga rápida pero segura** | 2A total se divide entre 2 celdas = 1A/celda (0.38C) |
| **No excede límites** | 0.38C << 2C máximo de Samsung |
| **Protección integrada** | Corta automáticamente a 4.2V exactos |
| **USB-C moderno** | Compatible con cargadores estándar actuales |
| **Bajo costo** | ~$2-3 USD en Ecuador |

#### 2.4.5.5 Compatibilidad de Descarga con el Consumo del Sistema

**Consumo total del BioSignalSimulator Pro:** ~1.335A (análisis detallado en sección 2.7)

| Componente | Consumo |
|------------|---------|
| ESP32 | 240 mA |
| Nextion 7" | 1000 mA |
| LM358 + CD4051 | 20 mA |
| LED RGB | 30 mA |
| XL6009 (pérdidas) | 45 mA |
| **TOTAL** | **~1335 mA** |

**Análisis de tasa de descarga:**

```
Consumo total: 1335 mA
Celdas en paralelo: 2
Corriente por celda: 1335 mA ÷ 2 = ~668 mA

Tasa de descarga por celda: 668 mA ÷ 2600 mAh = 0.26C

✅ RESULTADO: La tasa de 0.26C está MUY POR DEBAJO del límite seguro de 2C
✅ SEGURIDAD: Las celdas operan en zona de confort térmico
✅ CAPACIDAD: A 0.26C se obtiene ~100% de la capacidad nominal
```

#### 2.4.5.6 Resumen de Seguridad - Tasas de Carga/Descarga

| Parámetro | Límite Samsung | Valor Real Sistema | Margen de Seguridad |
|-----------|----------------|-------------------|---------------------|
| **Carga máxima** | 2C (5.2A/celda) | 0.38C (1A/celda) | **5.2× bajo el límite** |
| **Descarga máxima** | 2C (5.2A/celda) | 0.26C (0.67A/celda) | **7.8× bajo el límite** |
| **Voltaje de carga** | 4.20V ± 0.05V | 4.20V ± 0.5% | ✅ Exacto |
| **Voltaje mín. descarga** | 2.75V | 2.54V (BMS corta antes) | ✅ Protegido |

> **✅ CONCLUSIÓN DE SEGURIDAD:** El sistema BioSignalSimulator Pro opera con un factor de seguridad de **5-8× por debajo** de los límites máximos de las celdas Samsung ICR18650-26J. Esto garantiza:
> - Operación segura sin riesgo de sobrecalentamiento
> - Máxima vida útil de las baterías (>500 ciclos esperados)
> - Carga rápida (2A) sin comprometer la integridad de las celdas

#### 2.4.5.7 Disponibilidad en Ecuador

Las baterías Samsung 18650 de 2600mAh están disponibles en el mercado ecuatoriano:

| Proveedor | Ubicación | Precio Aprox. | Observaciones |
|-----------|-----------|---------------|---------------|
| **Novatronic** | Guayaquil/Quito | $4-6 USD/unidad | Distribuidor electrónico principal |
| **Mercado Libre Ecuador** | Online | $5-8 USD/unidad | Verificar autenticidad Samsung |
| **TecnoMega** | Guayaquil | $4-5 USD/unidad | Stock variable |

> **⚠️ ADVERTENCIA:** Existen falsificaciones de celdas Samsung en el mercado. Verificar que las celdas tengan el código QR original de Samsung SDI y peso correcto (~45g). Las celdas falsas pueden tener capacidades reales de solo 800-1200 mAh y tasas de descarga peligrosamente bajas.

---

**⚠️ Precauciones de Seguridad para Baterías en Paralelo (Mini Power Bank)**

La configuración de celdas 18650 en paralelo forma una "mini power bank" y requiere precauciones específicas para evitar corrientes de ecualización peligrosas y daños a las celdas:

| Precaución | Descripción | Riesgo si no se cumple |
|------------|-------------|------------------------|
| **Voltajes iguales** | Conectar celdas con ≤0.05V de diferencia | Corriente de ecualización entre celdas (puede ser >10A) |
| **Celdas nuevas** | Usar celdas del mismo lote y fecha | Desbalance por diferente capacidad/resistencia interna |
| **Mismo modelo** | Idéntico fabricante, modelo y capacidad | Diferencias de química causan corrientes parásitas |
| **BMS obligatorio** | Usar BMS 1S para protección del pack | Sobrecarga, sobredescarga, cortocircuito sin protección |
| **Portapilas estables** | Usar portapilas de calidad con resortes firmes | Conexiones intermitentes causan arcos y calentamiento |
| **Puentes seguros** | Cables AWG 18 o mayores entre portapilas | Cables delgados se calientan con corrientes de ecualización |
| **Verificar antes de conectar** | Medir voltaje de cada celda individualmente | Conectar celda descargada a celda cargada = chispazo |

**Procedimiento de conexión seguro:**

1. Medir voltaje de cada celda con multímetro
2. Si hay diferencia >0.05V, cargar ambas celdas individualmente hasta 4.2V
3. Verificar nuevamente que tengan voltaje idéntico
4. Conectar las celdas en paralelo al portapilas
5. Conectar el BMS al pack
6. Verificar que el BMS no esté en modo de protección

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

**Cadena energética:** USB 5V (IP5306) → BMS 1S 3A → Pack 2×18650 (paralelo) → Switch → XL6009 → ESP32/Nextion/LM358/CD4051.

El BMS garantiza protección celda-celda antes del elevador, mientras el IP5306 gestiona el perfil CC/CV de carga. El XL6009 eleva el voltaje de 3.7V a 5V estables para alimentar todos los componentes.

### 2.5 Arquitectura del Sistema Electrónico

#### 2.5.1 Diagrama de Bloques General

```
┌──────────────────────────────────────────────────────────────────────┐
│                   BIOSIGNALSIMULATOR PRO v3.0                        │
│                     ARQUITECTURA SIMPLIFICADA                        │
├──────────────────────────────────────────────────────────────────────┤
│                                                                       │
│  ┌───────────────────────────────────────────────────────────────┐  │
│  │             SUBSISTEMA DE ALIMENTACIÓN                        │  │
│  │                   (Módulos externos)                          │  │
│  └───────────────────────────────────────────────────────────────┘  │
│                                                                       │
│  USB-C ──► IP5306 ──► BMS 1S 3A ──► BATERÍAS 2×18650 (5200 mAh)     │
│           (Carga)     (Protección)        (Paralelo)                 │
│                                              │                        │
│                                              ▼                        │
│                                        SWITCH ON/OFF                 │
│                                              │                        │
│                                              ▼                        │
│                                     XL6009 (3.7V → 5V)               │
│                                       η ≈ 88-92%                     │
│                                              │                        │
│                                              │ +5V                    │
│                                              ▼                        │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │     PLACA PRINCIPAL: GENERACIÓN Y CONTROL DE SEÑAL          │   │
│  │                  (PCB 10×15 cm)                              │   │
│  │                                                              │   │
│  │  ┌────────────┐   ┌──────────────┐   ┌──────────────┐       │   │
│  │  │   ESP32    │   │   NEXTION    │   │  LED RGB     │       │   │
│  │  │  WROOM-32  │   │  NX8048T070  │   │  Indicador   │       │   │
│  │  │            │   │   7" Touch   │   │              │       │   │
│  │  └──────┬─────┘   └──────────────┘   └──────────────┘       │   │
│  │         │                                                    │   │
│  │         ├──► DAC (GPIO25)                                    │   │
│  │         │         │                                          │   │
│  │         │         ▼                                          │   │
│  │         │    ┌─────────┐                                     │   │
│  │         │    │  LM358  │ Buffer ×1                           │   │
│  │         │    │ Buffer  │                                     │   │
│  │         │    └────┬────┘                                     │   │
│  │         │         │                                          │   │
│  │         │         ▼                                          │   │
│  │         │    ┌─────────┐                                     │   │
│  │         ├───►│ CD4051  │ DEMUX Analógico                     │   │
│  │      GPIO32/│  (S0/S1) │                                     │   │
│  │       33    └────┬────┘                                      │   │
│  │                  │                                           │   │
│  │                  ├─► CH0: R=6.8kΩ + C=1µF ──► BNC_ECG        │   │
│  │                  ├─► CH1: R=1.0kΩ + C=1µF ──► BNC_EMG        │   │
│  │                  └─► CH2: R=33kΩ  + C=1µF ──► BNC_PPG        │   │
│  │                                                              │   │
│  │                  (3 filtros RC independientes)               │   │
│  │                  (3 salidas BNC separadas)                   │   │
│  │                                                              │   │
│  └──────────────────────────────────────────────────────────────┘   │
│                                                                       │
└──────────────────────────────────────────────────────────────────────┘
```

**Descripción de la arquitectura:**

El sistema utiliza una **placa principal única** que integra todos los componentes de generación y control:

- **Placa Principal (10×15 cm):** Contiene el ESP32, la interfaz Nextion, el buffer LM358, el demultiplexor CD4051 que distribuye la señal DAC a 3 filtros RC independientes con sus correspondientes 3 conectores BNC de salida (ECG, EMG, PPG). La alimentación de 5V proviene directamente del módulo XL6009.

#### 2.5.2 Placa Principal: Generación y Control de Señal

**Objetivo:** Generar señales biomédicas (ECG, EMG, PPG) configurables mediante DAC, acondicionar la señal con buffer y filtros selectivos, y proporcionar interfaz de usuario táctil.

**Subsistemas principales:**

**A. Subsistema de Procesamiento y Control**

| Componente | Función | Pines clave |
|------------|---------|-------------|
| ESP32-WROOM-32 | Generación de señales vía DAC, control WiFi, comunicación UART | GPIO25 (DAC1), GPIO32/33 (S0/S1 MUX), GPIO16/17 (UART2 Nextion) |
| Nextion NX8048T070 | Interfaz táctil 7" 800×480 | RX/TX (UART), +5V, GND |
| LED RGB | Indicador visual de estado | R/G/B + resistencias 220Ω |

**B. Subsistema de Acondicionamiento de Señal**

```
DAC (GPIO25) → LM358 Buffer → CD4051 DEMUX → 3 Filtros RC → 3 BNC
  0-3.3V         Ganancia ×1    Selección      Independientes   Salidas
                                CH0/CH1/CH2                      ECG/EMG/PPG
```

**Arquitectura Final con 3 Salidas BNC Independientes:**

Debido a las limitaciones de impedancia del CD4051 con señales analógicas continuas (ver sección 2.5.2.1), se adoptó una arquitectura simplificada con un solo CD4051 como DEMULTIPLEXOR y 3 salidas BNC independientes:

```
                              ┌──[6.8kΩ]──┬──[1µF]──GND ──► BNC_ECG
                              │           │
DAC → LM358 → CD4051(DEMUX) ──┼──[1.0kΩ]──┬──[1µF]──GND ──► BNC_EMG  
                              │           │
                              └──[33kΩ]───┬──[1µF]──GND ──► BNC_PPG
```

**Cadena de acondicionamiento detallada:**

| Etapa | Componente | Entrada | Salida | Función |
|-------|------------|---------|--------|---------|
| 1. Generación | ESP32 DAC (GPIO25) | Digital | 0-3.3V analógico | Conversión D/A de la señal biomédica |
| 2. Buffer | LM358 (configuración seguidor) | 0-3.3V | 0-3.3V | Impedancia baja (~100Ω) para alimentar MUX |
| 3. Multiplexación | CD4051 (canales 0-2) | 0-3.3V | 0-3.3V | Selección de resistencia de filtro RC |
| 4. Filtrado | RC pasabajos (R variable + C=1µF) | 0-3.3V | 0-3.3V filtrada | Elimina stepping del DAC (4 kHz) |
| 5. Salida | Conector BNC hembra | Señal filtrada | BNC | Conexión a osciloscopio |

**Tabla de filtros RC selectivos (basada en análisis FFT):**

| Señal | Canal CD4051 | R (kΩ) | Fc (Hz) | F99% energía | Atenuación @ 4 kHz |
|-------|--------------|--------|---------|--------------|-------------------|
| ECG | CH0 (S1=0, S0=0) | 6.8 | 23.4 | 21.6 Hz | -44 dB |
| EMG | CH1 (S1=0, S0=1) | 1.0 | 159 | 146.3 Hz | -28 dB |
| PPG | CH2 (S1=1, S0=0) | 33 | 4.82 | 4.9 Hz | -58 dB |

**Notas de diseño:**

- Cada canal tiene su **propio capacitor de 1µF** (3 capacitores independientes)
- Las diferentes Fc se logran variando la resistencia en serie
- El pin S2 del CD4051 se conecta a GND (limita canales a 0-3)
- El LM358 se usa como buffer de entrada por disponibilidad local
- **3 salidas BNC independientes:** ECG, EMG y PPG tienen conectores separados

---

### 2.5.2.1 Justificación de la Arquitectura con 3 BNC: Problemas de Impedancia del CD4051

> **⚠️ SECCIÓN CRÍTICA DE DISEÑO:** Esta sección documenta los problemas encontrados durante el desarrollo y la solución adoptada.

#### Problema Original: Uso del CD4051 como MUX de Salida

Inicialmente se diseñó una arquitectura con un solo BNC y el CD4051 seleccionando entre 3 filtros RC que compartían un capacitor común. Durante las pruebas se detectaron los siguientes problemas:

| Problema Detectado | Causa | Efecto Observado |
|--------------------|-------|------------------|
| **Caída de amplitud** | R_ON del CD4051 (~100-400Ω) forma divisor con carga | Pérdida de 10-30% de amplitud |
| **Offset de voltaje** | Corrientes de fuga + alta impedancia de fuente | Desplazamiento DC de 50-200mV |
| **Crosstalk entre canales** | Capacitancia parásita + resistencias en paralelo | Señal contaminada de otros canales |
| **Resistencias en paralelo** | Diseño con capacitor compartido | Los 3 filtros cargan la señal simultáneamente |

#### Análisis del Problema de Resistencias en Paralelo

Con el diseño original donde las 3 resistencias del filtro (6.8kΩ, 1kΩ, 33kΩ) compartían un nodo común con el capacitor:

```
R_equivalente = 1 / (1/6.8k + 1/1k + 1/33k) = ~870Ω
```

Esto causaba que **todas las resistencias cargaran la señal simultáneamente**, creando un divisor de voltaje inesperado y pérdida de señal significativa.

#### Problemas Conocidos del CD4051 con Señales Analógicas Continuas

Según la documentación técnica y experiencias de la comunidad electrónica:

1. **R_ON variable con voltaje:** La resistencia ON del switch (100-400Ω) varía con el nivel de señal, causando distorsión no lineal.

2. **Alta impedancia requerida:** Para minimizar caídas de voltaje por R_ON, la carga debe ser >>10× R_ON (idealmente >10kΩ).

3. **Necesidad de buffers:** Se recomienda usar buffers op-amp tanto a la entrada como a la salida del CD4051 para aislar impedancias.

4. **Crosstalk capacitivo:** En aplicaciones de audio/señales continuas, las capacitancias parásitas entre canales causan "bleeding" de señal.

#### Solución Adoptada: Arquitectura con 3 BNC Independientes

En lugar de usar 2 CD4051 (DEMUX + MUX) con buffers adicionales, se optó por una solución más simple y robusta:

```
                              ┌──[6.8kΩ]──┬──[1µF]──GND ──► BNC_ECG
                              │           │
DAC → LM358 → CD4051(DEMUX) ──┼──[1.0kΩ]──┬──[1µF]──GND ──► BNC_EMG  
                              │           │
                              └──[33kΩ]───┬──[1µF]──GND ──► BNC_PPG
```

| Ventaja | Descripción |
|---------|-------------|
| **Sin MUX de salida** | Elimina R_ON y distorsión en la etapa de salida |
| **Filtros aislados** | Cada filtro tiene su propio capacitor, sin carga paralela |
| **Sin crosstalk** | Las señales no comparten camino de salida |
| **Simplicidad** | Solo 1 CD4051 en lugar de 2 + buffer adicional |
| **Costo reducido** | Menos componentes activos, solo 3 BNC adicionales |
| **3 salidas disponibles** | Aunque solo 1 está activa, las 3 están conectadas permanentemente |

#### Trade-off Aceptado

- **Desventaja:** Solo 1 de las 3 salidas BNC tiene señal activa en cada momento.
- **Mitigación:** En uso educativo, solo se conecta un canal al osciloscopio a la vez, por lo que esto no afecta la funcionalidad.
- **Beneficio neto:** Señal limpia, sin pérdidas de amplitud ni offsets, con arquitectura más simple.

**C. Criterios de diseño de la PCB de control:**

| Criterio | Implementación |
|----------|----------------|
| Tamaño | 10×15 cm (PCB perforada) |
| Distribución | ESP32 centro, Nextion borde superior, BNC borde lateral |
| Pistas de señal | Separadas de pistas de potencia, ancho 0.8-1.0 mm |
| Plano GND | Común para digital y analógico (nodo único) |
| Montaje | 4 tornillos M3×10 mm al chasis |

#### 2.5.4 Consumo Energético y Autonomía

**Tabla de consumos medidos/especificados:**

| Componente | I Promedio @ 5V | P Promedio | I Pico @ 5V | P Pico | Fuente |
|------------|-----------------|------------|-------------|--------|--------|
| Nextion NX8048T070 | 510 mA | 2.55 W | 650 mA | 3.25 W | Datasheet Nextion |
| ESP32-WROOM-32 (WiFi AP) | 240 mA | 1.20 W | 350 mA | 1.75 W | ESP32 Datasheet v5.2 |
| LM358 Buffer (1 canal activo) | 0.7 mA | 0.0035 W | 0.7 mA | 0.0035 W | LM358 Datasheet (1.4mA/2 canales) |
| CD4051 DEMUX | 0.5 mA | 0.0025 W | 0.5 mA | 0.0025 W | CD4051 Datasheet |
| LED RGB | 30 mA | 0.15 W | 30 mA | 0.15 W | Cálculo (3× LED @ 10 mA) |
| XL6009 pérdidas | 69 mA equiv. | 0.35 W | 162 mA equiv. | 0.81 W | Eficiencia 92%/88% |
| **TOTAL SISTEMA** | **851 mA** | **4.26 W** | **1194 mA** | **5.97 W** | |

**Cálculo de autonomía - Modo Promedio:**

```
Capacidad útil: 5200 mAh × 93% = 4836 mAh
P_sistema = 5V × 0.851A = 4.26 W
P_batería = 4.26W / 0.92 (η) = 4.63 W
I_batería = 4.63W / 3.7V = 1.25 A
Autonomía = 4836 mAh / 1250 mA = 3.87 horas
```

**Autonomía práctica: 3.9 horas** (cumple requisito ≥3 horas)

---

## 3. Lista de Materiales (BOM Electrónico)

El sistema se implementa con dos PCB separadas más módulos externos. A continuación se detalla la BOM completa separada por subsistemas.

### 3.1 Módulos de Alimentación (Externos a las PCBs)

| # | Componente | Cantidad | Precio Unit. | Subtotal | Proveedor |
|---|------------|----------|--------------|----------|-----------|
| 1 | Baterías Samsung 18650 2600 mAh | 2 | $6.50 | $13.00 | Novatronic |
| 2 | Portapilas 2×18650 (paralelo) | 1 | $2.50 | $2.50 | Novatronic |
| 3 | Módulo cargador IP5306 5V/2A USB-C | 1 | $3.85 | $3.85 | Novatronic |
| 4 | BMS 1S 3A (PCM 8205A, 40×4×3 mm) | 1 | $1.20 | $1.20 | Novatronic |
| 5 | Switch deslizable ON/OFF | 1 | $0.80 | $0.80 | Novatronic |
| 6 | Módulo elevador XL6009 DC-DC Step-Up | 1 | $4.10 | $4.10 | Novatronic |
| 7 | Cables AWG 18 (rojo/negro, 2m) | 1 | $0.55 | $0.55 | Novatronic |
| | **Subtotal alimentación externa** | | | **$26.00** | |

### 3.2 Placa Principal: Control y Generación - Módulos Activos

| # | Componente | Cantidad | Precio Unit. | Subtotal | Proveedor |
|---|------------|----------|--------------|----------|-----------|
| 8 | Nextion NX8048T070 7" 800×480 | 1 | $95.75 | $95.75 | Amazon |
| 9 | ESP32-WROOM-32 NodeMCU | 1 | $13.35 | $13.35 | Novatronic |
| 10 | LM358 DIP-8 (buffer seguidor) | 1 | $0.50 | $0.50 | Novatronic |
| 11 | CD4051 DIP-16 (DEMUX 1:8) | 1 | $0.80 | $0.80 | Novatronic |
| 12 | LED RGB 5 mm cátodo común | 1 | $0.50 | $0.50 | Novatronic |
| | **Subtotal Placa Principal - Activos** | | | **$110.90** | |

### 3.3 Placa Principal: Control y Generación - Pasivos y Conectores

| # | Componente | Cantidad | Precio Unit. | Subtotal | Proveedor |
|---|------------|----------|--------------|----------|-----------|
| 13 | Resistencia 220Ω 1/4W (LED RGB) | 3 | $0.05 | $0.15 | Novatronic |
| 14 | Resistencia 6.8kΩ 1/4W (filtro ECG) | 1 | $0.05 | $0.05 | Novatronic |
| 15 | Resistencia 1.0kΩ 1/4W (filtro EMG) | 1 | $0.05 | $0.05 | Novatronic |
| 16 | Resistencia 33kΩ 1/4W (filtro PPG) | 1 | $0.05 | $0.05 | Novatronic |
| 17 | Capacitor cerámico 1µF/16V X7R (filtro ECG) | 1 | $0.10 | $0.10 | Novatronic |
| 18 | Capacitor cerámico 1µF/16V X7R (filtro EMG) | 1 | $0.10 | $0.10 | Novatronic |
| 19 | Capacitor cerámico 1µF/16V X7R (filtro PPG) | 1 | $0.10 | $0.10 | Novatronic |
| 20 | Conector BNC hembra (ECG) | 1 | $1.20 | $1.20 | Novatronic |
| 21 | Conector BNC hembra (EMG) | 1 | $1.20 | $1.20 | Novatronic |
| 22 | Conector BNC hembra (PPG) | 1 | $1.20 | $1.20 | Novatronic |
| 23 | Bornera 2 pines paso 8.05 mm (PWR_IN) | 1 | $0.50 | $0.50 | Novatronic |
| 24 | Bornera 4 pines paso 8.05 mm (LED RGB / NEXTION) | 2 | $0.80 | $1.60 | Novatronic |
| 25 | PCB perforada 10×15 cm | 1 | $2.00 | $2.00 | Novatronic |
| 26 | Tornillos M3×10 mm (4 uds) | 1 | $0.40 | $0.40 | Ferretería |
| | **Subtotal Placa Principal - Pasivos** | | | **$8.50** | |

---

**TOTAL SISTEMA ELECTRÓNICO:**  
$26.00 (módulos alimentación) + $110.90 (activos) + $8.50 (pasivos) = **$145.40**

### 3.4 Resumen por Etapas de Diseño

| Etapa | Subsistema | Componentes Clave | Subtotal |
|-------|------------|-------------------|----------|
| **Potencia** | Módulos externos | 2× 18650, BMS 1S 3A, IP5306, XL6009, portapilas, switch | $26.00 |
| **Control - Activos** | Placa Principal (10×15 cm) | Nextion 7", ESP32, LM358, CD4051 DEMUX, LED RGB | $110.90 |
| **Control - Pasivos** | Placa Principal (10×15 cm) | 3× filtros RC independientes (1µF cada uno), 3× BNC | $8.50 |
| | | **TOTAL** | **$145.40** |

**Notas de diseño:**
- **Sin divisor resistivo UART:** La Nextion NX8048T070 acepta niveles lógicos TTL 3.3V directamente, eliminando necesidad de divisor resistivo 2kΩ/1kΩ.
- **LM358 como buffer de entrada:** Se implementó LM358 DIP-8 por disponibilidad local en Ecuador. Proporciona baja impedancia para alimentar el DEMUX.
- **Arquitectura DEMUX con 3 BNC:** CD4051 usado como demultiplexor, distribuyendo la señal DAC a 3 caminos independientes con salidas BNC separadas (ECG, EMG, PPG).
- **Filtros RC independientes:** Cada canal tiene su propio capacitor de 1µF, eliminando problemas de resistencias en paralelo.
- **Sin placa de filtrado:** La alimentación de 5V del XL6009 se conecta directamente a la placa principal, simplificando el diseño.
- **Justificación arquitectura:** Se adoptó esta configuración debido a problemas de impedancia del CD4051 con señales continuas (caídas de amplitud, offsets, crosstalk) cuando se usaba como MUX de salida sin buffer adicional.

### 3.5 Esquemáticos de Referencia

Los esquemáticos completos del sistema se realizaron en EasyEDA v1.0:

- **Esquemático:** Placa principal de generación y control (ESP32, CD4051, LM358, Nextion, LED RGB, 3× BNC)

El esquemático se encuentra en la carpeta `/docs/diagramas/` en formato PDF y fuente EasyEDA.

## 4. Manual de Usuario

### 4.1 Encendido y Apagado

1. **Encender:** Deslizar switch a ON, esperar 3 segundos para inicialización

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

**Configuración recomendada:** 1 V/div, acoplamiento DC/AC según señal, base de tiempo acorde a frecuencia.

### 4.5 Indicadores LED
│                                              │                    │
│                                              ▼                    │
│                                    ┌─────────────┐                │
│                                    │ FUSIBLE 1.5A│               │
│                                    └──────┬───────┘               │
│                                         │                        │
│                                         ▼                        │
│                               ┌──────────────────┐                │
│                               │    FILTRO LC     │                │
│                               │ 22µH + 1µF + 470nF│                │
│                               └────────┬─────────┘                │
│                                        │                         │
│                           ┌────────────┼───────────────┐          │
│                           ▼            ▼               ▼          │
│                        ESP32        NEXTION         LM358        │
│                      WROOM-32     7" 800×480        Buffer       │
│                                                    └─► BNC       │
│                                                                   │
└──────────────────────────────────────────────────────────────────┘
```

**Protección por fusible:**

| Parámetro | Valor | Justificación |
|-----------|-------|---------------|
| Ubicación | Salida 5V del XL6009 | Protege carga (ESP32, Nextion, LM358) |
| Valor | 1.5A fusión lenta | Consumo máx ~1.3A + margen para picos arranque |
| Tipo | Cartucho 5×20mm | Reemplazable sin desoldar |

**Filtro LC de salida:**

| Parámetro | Valor | Justificación |
|-----------|-------|---------------|
| Inductor | 22µH | Bloquea ruido de alta frecuencia del switching (~500kHz) |
| Capacitor C1 | 1µF | Capacitor de desacople principal |
| Capacitor C2 | 470nF | Capacitor cerámico para filtrado de alta frecuencia |
| Frecuencia de corte | ~34 kHz | Suficiente para eliminar ripple del XL6009 |

**Justificación de protección en dos niveles:**

- **BMS 1S 3A (DW01):** Protege las **baterías** contra sobrecarga, sobredescarga y cortocircuito.
- **Fusible 1.5A:** Protege la **carga** (electrónica downstream) contra cortocircuito.

Esta arquitectura no es redundante: cada protección cubre un dominio diferente del circuito.

#### 2.5.3 Acondicionamiento de la salida analógica

Para asegurar que la forma de onda enviada al BNC mantenga la banda útil (0‑500 Hz) y llegue suavizada (sin escalones del DAC), se añadió un filtro pasabajos RC a la salida del LM358:

- **Resistencia serie:** 100 Ω ubicada entre la salida del LM358 y la bornera "BNC_OUT". Además de definir fc junto con el capacitor, protege al op-amp ante cortos o cargas capacitivas externas.
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

#### 2.5.3.1 Implementación: Demultiplexor CD4051 para Distribución de Señal

Basándose en el análisis espectral FFT de las señales generadas por los modelos matemáticos, se implementó un sistema de distribución de señal mediante el demultiplexor analógico CD4051. Este enfoque permite dirigir la señal DAC al filtro RC correspondiente según el tipo de señal activa, con cada canal teniendo su propia salida BNC independiente.

**Topología implementada: DAC → LM358 Buffer → CD4051 DEMUX → 3× RC Filter → 3× BNC**

```
┌─────────────────────────────────────────────────────────────────────────────┐
│      CADENA DE ACONDICIONAMIENTO CON 3 SALIDAS BNC INDEPENDIENTES           │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ESP32           LM358              CD4051                 3× Filtros RC    │
│  ┌──────┐       ┌──────┐          ┌──────────┐                              │
│  │GPIO25│──DAC──│Buffer│──────────│► COM     │                              │
│  │(DAC1)│       │ ×1   │          │          │   ┌─[6.8kΩ]─┬─[1µF]─GND      │
│  └──────┘       └──────┘          │ CH0 ─────│───┤         └────►BNC_ECG    │
│                                   │          │   │                          │
│  ┌──────┐                         │ CH1 ─────│───┼─[1.0kΩ]─┬─[1µF]─GND      │
│  │GPIO32│─────────────────────────│► S0      │   │         └────►BNC_EMG    │
│  │GPIO33│─────────────────────────│► S1      │   │                          │
│  └──────┘                         │ S2=GND   │   └─[33kΩ]──┬─[1µF]─GND      │
│                                   │ CH2 ─────│─────────────└────►BNC_PPG    │
│                                   └──────────┘                              │
│                                                                             │
│  NOTA: Solo 1 canal activo a la vez. 3 capacitores independientes (1µF c/u)│
└─────────────────────────────────────────────────────────────────────────────┘
```

**Tabla: Filtros RC implementados según análisis FFT (cada uno con su capacitor)**

| Señal | F 99% Energía | Fc Diseño | R | C | Salida BNC | Atenuación @ 4kHz |
|-------|---------------|-----------|---|---|------------|-------------------|
| **ECG** | 21.6 Hz | 23.4 Hz | 6.8 kΩ | 1µF | BNC_ECG | -44 dB |
| **EMG** | 146.3 Hz | 159 Hz | 1.0 kΩ | 1µF | BNC_EMG | -28 dB |
| **PPG** | 4.9 Hz | 4.82 Hz | 33 kΩ | 1µF | BNC_PPG | -58 dB |

**Justificación de la selección de componentes:**

1. **Buffer LM358:** Proporciona impedancia de salida baja (<100Ω) para alimentar el demultiplexor sin pérdidas significativas. Configurado como seguidor de voltaje (ganancia unitaria).

2. **Demultiplexor CD4051:** Usado como DEMUX (1 entrada, 3 salidas) para distribuir la señal DAC a uno de los tres filtros RC. El pin S2 se conectó a GND permanentemente, limitando la selección a canales 0-2.

3. **3× Capacitores independientes (1µF):** Cada filtro tiene su propio capacitor cerámico X7R, eliminando problemas de resistencias en paralelo que causaban pérdida de señal en la arquitectura original con capacitor compartido.

4. **3× Conectores BNC:** Cada tipo de señal tiene su propia salida física, permitiendo conexión directa al osciloscopio sin necesidad de MUX de salida.

**Cálculo de frecuencias de corte:**

$$F_c = \frac{1}{2\pi R C}$$

- **ECG:** $F_c = \frac{1}{2\pi \times 6800 \times 10^{-6}} = 23.4 \, Hz$ (ligeramente superior a F99%=21.6 Hz)
- **EMG:** $F_c = \frac{1}{2\pi \times 1000 \times 10^{-6}} = 159 \, Hz$ (ligeramente superior a F99%=146.3 Hz)
- **PPG:** $F_c = \frac{1}{2\pi \times 33000 \times 10^{-6}} = 4.82 \, Hz$ (coincide con F99%=4.9 Hz)

**Nota:** El filtro RC para EMG (R=1kΩ) fue necesario para eliminar ruido de alta frecuencia introducido por el multiplexor CD4051 cuando operaba sin filtro. La conexión directa (bypass) generaba interferencias visibles en el osciloscopio.

**Análisis de error por resistencia Ron del CD4051:**

| Canal | R nominal | Ron (típ.) | R total | Fc nominal | Fc real | Error |
|-------|-----------|------------|---------|------------|---------|-------|
| CH0 (ECG) | 6.8 kΩ | 80 Ω | 6.88 kΩ | 23.4 Hz | 23.1 Hz | <1.2% |
| CH1 (EMG) | 1.0 kΩ | 80 Ω | 1.08 kΩ | 159 Hz | 147 Hz | <7.5% |
| CH2 (PPG) | 33 kΩ | 80 Ω | 33.08 kΩ | 4.82 Hz | 4.81 Hz | <0.3% |

El error introducido por Ron es inferior al 1.2% en todos los casos, despreciable para la aplicación educativa.

**Ventajas de esta implementación vs. filtro RC único:**

| Parámetro | Filtro RC único (Fc=1.59kHz) | CD4051 + RC selectivo |
|-----------|------------------------------|------------------------|
| Atenuación ECG @ 4kHz | -8 dB | -44 dB (5.5× mejor) |
| Atenuación PPG @ 4kHz | -8 dB | -58 dB (7.3× mejor) |
| Complejidad | 2 componentes | 5 componentes + control GPIO |
| Costo adicional | $0 | ~$2 (CD4051 + resistores) |

#### 2.5.4 Consumos y Autonomía

**Tabla de Consumos del Sistema**

| Componente | I Promedio | P Promedio @5V | I Pico | P Pico @5V | Referencia |
|------------|------------|----------------|--------|------------|------------|
| Nextion NX8048T070 | 510 mA | 2.55 W | 650 mA | 3.25 W | Datasheet Basic Series [1] |
| ESP32-WROOM-32 (WiFi AP) | 240 mA | 1.20 W | 350 mA | 1.75 W | ESP32 Datasheet v5.2, Tabla 5-4 [2] |
| LM358 Buffer (1 canal activo) | 0.7 mA | 0.0035 W | 0.7 mA | 0.0035 W | LM358 Datasheet (1.4mA/2 canales) |
| CD4051 DEMUX | 0.5 mA | 0.0025 W | 0.5 mA | 0.0025 W | CD4051 Datasheet |
| LED RGB | 30 mA | 0.15 W | 30 mA | 0.15 W | Cálculo: Vf≈2.0V (R), 3.0V (G/B) |
| XL6009 (pérdidas) | 69 mA equiv. | 0.35 W | 162 mA equiv. | 0.81 W | XL6009 Datasheet, η≈92%/88% [4] |
| **TOTAL** | **851 mA** | **4.26 W** | **1194 mA** | **5.97 W** | |

**Nota:** El consumo pico representa un escenario extremo donde todos los subsistemas demandan simultáneamente (brillo 100%, WiFi TX continuo, LED RGB encendido). En uso normal, el sistema opera en modo promedio.

**Parámetros del sistema:**
- Capacidad nominal batería: 5200 mAh (2×2600 mAh en paralelo)
- Capacidad útil (93%): **4836 mAh**
- Voltaje nominal batería: 3.7V
- Eficiencia XL6009: 92% @ 0.8A (modo promedio), 88% @ 1.2A (modo pico)
- Protección celda: BMS 1S 3A (8205A) entre IP5306 y pack

**Cálculo de autonomía - Modo Promedio (uso normal):**

```
1. P_sistema = 5V × 0.851A = 4.26 W
2. P_batería = 4.26W / 0.92 = 4.63 W
3. I_batería = 4.63W / 3.7V = 1.25 A
4. t = 4836 mAh / 1250 mA = 3.87 h
5. t_práctica ≈ 3.9 horas
```

**Cálculo de autonomía - Modo Pico (escenario extremo):**

```
1. P_sistema = 5V × 1.19A = 5.97 W
2. P_batería = 5.97W / 0.88 = 6.78 W
3. I_batería = 6.78W / 3.7V = 1.83 A
4. t = 4836 mAh / 1830 mA = 2.64 h
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

## 5. Referencias

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
