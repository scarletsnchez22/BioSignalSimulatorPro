# BioSignalSimulator Pro - Metodología de Diseño Mecánico

**Versión:** 3.0.0  
**Fecha:** Enero 2025  
**Autor:** [Nombre del Tesista]  
**Documento para:** Trabajo de Titulación

---

## Índice

1. [Introducción](#1-introducción)
2. [Metodología de Diseño Mecánico](#2-metodología-de-diseño-mecánico)
3. [Lista de Materiales (BOM Mecánico)](#3-lista-de-materiales-bom-mecánico)
4. [Referencias](#4-referencias)

---

## 1. Introducción

### 1.1 Propósito del Documento

Este documento describe la estrategia completa de diseño mecánico del BioSignalSimulator Pro. Incluye los requisitos funcionales y no funcionales de la carcasa, criterios de selección de materiales, procesos de manufactura (impresión 3D en PETG), análisis térmico/ventilación y consideraciones éticas/legales asociadas.

### 1.2 Objetivos Mecánicos Generales

| Objetivo | Métrica | Criterio de Éxito |
|----------|---------|-------------------|
| Portabilidad | Peso | < 500 g ensamblado |
| Resistencia | Vida útil | > 2 años en laboratorio |
| Seguridad térmica | Temperatura superficial | < 80 °C |
| Manufactura | Tiempo de impresión | < 8 h por set |
| Accesibilidad | Fasteners | Tornillos estándar M3 |

---

## 2. Metodología de Diseño Mecánico

### 2.1 Requisitos del Sistema Mecánico

#### 2.1.1 Requisitos Funcionales

| ID | Requisito | Especificación |
|----|-----------|----------------|
| RM-01 | Alojamiento de componentes | Espacio para PCB, baterías y pantalla |
| RM-02 | Acceso a conectores | BNC, USB y switch accesibles |
| RM-03 | Visualización | Ventana para pantalla Nex­tion 7" |
| RM-04 | Portabilidad | Peso < 500 g, dimensiones manejables |
| RM-05 | Ensamblaje | Tornillos estándar M3 |

#### 2.1.2 Requisitos No Funcionales

| ID | Requisito | Especificación |
|----|-----------|----------------|
| RNM-01 | Disipación térmica | Mantener T interna < 70 °C |
| RNM-02 | Resistencia mecánica | Soportar uso repetido en laboratorio |
| RNM-03 | Estética | Acabado profesional (biselado) |
| RNM-04 | Fabricación | Impresión 3D accesible (sin soportes complejos) |

### 2.2 Normativas y Estándares Aplicables

#### 2.2.1 IEC 61010-1 (Aspectos mecánicos)

| Requisito | Implementación |
|-----------|----------------|
| Protección de partes peligrosas | Carcasa cerrada, bordes redondeados |
| Estabilidad | Base plana, centro de gravedad bajo |
| Resistencia mecánica | PETG con espesor 2.5 mm |
| Ventilación | Orificios dimensionados para convección natural |

#### 2.2.2 Límites térmicos IEC 61010-1, Cláusula 10

- Superficies metálicas accesibles: ≤ 60 °C  
- Superficies no metálicas: ≤ 80 °C  

El uso de PETG (no metálico) permite operar con temperaturas superficiales de hasta 80 °C manteniendo margen de seguridad.

### 2.3 Principios de Diseño

| Principio | Descripción | Aplicación |
|-----------|-------------|------------|
| Diseño para manufactura | Simplificar impresión 3D | Piezas planas, sin soportes extensos |
| Modularidad | Facilitar mantenimiento | Tapa superior e inferior independientes |
| Ergonomía | Uso cómodo | Bordes redondeados, agarre lateral |
| Mantenibilidad | Acceso rápido | Tornillos externos, sin pegamentos |
| Ventilación pasiva | Sin ventiladores | Rejillas laterales y perforaciones traseras |

### 2.4 Selección de Material

| Material | Conductividad (W/m·K) | T_max | Costo | Decisión |
|----------|----------------------|-------|-------|----------|
| PLA | 0.13 | 50 °C | Bajo | ❌ Baja resistencia térmica |
| ABS | 0.17 | 80 °C | Medio | ✅ Opción alternativa |
| **PETG** | **0.29** | **75 °C** | **Medio** | **✅ Seleccionado** |
| Aluminio | 205 | >200 °C | Alto | ❌ Costoso para prototipo |

**Justificación PETG:**
- Mayor conductividad térmica que PLA/ABS  
- Menor warping y mejor adherencia  
- Disponible localmente, acabado aceptable  
- Resistencia química y mecánica adecuada

### 2.5 Recursos Mecánicos y Geometría

#### 2.5.1 Especificaciones de la Carcasa

| Parámetro | Valor |
|-----------|-------|
| Material | PETG (impresión 3D) |
| Dimensiones externas | 180 × 120 × 45 mm |
| Espesor de pared | 2.5 mm |
| Peso carcasa | ~80 g |
| Peso total (ensamblado) | ~350 g |

#### 2.5.2 Características Clave

| Característica | Descripción |
|----------------|-------------|
| Ventilación trasera | Patrón circular (12 orificios Ø5 mm, ~15 cm²) |
| Ventilación lateral | Rejillas tipo persiana (6 ranuras × 2 mm × 30 mm por lado) |
| Marco pantalla | Bisel frontal integrado para Nextion 7" |
| Acceso BNC | Orificio lateral con grabado “DAC” |
| Montaje | 4 tornillos M3 + separadores |
| Identificación | Relieve “BioSignalSimulator Pro” |

### 2.6 Proceso de Diseño Mecánico

#### 2.6.1 Flujo Metodológico

```
┌─────────────────────────────────────────────────────────────────┐
│              METODOLOGÍA DE DISEÑO MECÁNICO                     │
├─────────────────────────────────────────────────────────────────┤
│  1. ANÁLISIS DE REQUISITOS                                      │
│     └─► Dimensiones de componentes, restricciones térmicas      │
│                                                                 │
│  2. DISEÑO CAD                                                  │
│     └─► Modelado en SolidWorks/Fusion, ensamblaje virtual       │
│                                                                 │
│  3. ANÁLISIS TÉRMICO                                            │
│     └─► Cálculo de disipación, dimensionado de ventilación      │
│                                                                 │
│  4. PROTOTIPADO RÁPIDO                                          │
│     └─► Impresión 3D PETG, ajustes iterativos                   │
│                                                                 │
│  5. VALIDACIÓN                                                  │
│     └─► Ensamblaje físico, medición de temperaturas             │
└─────────────────────────────────────────────────────────────────┘
```

#### 2.6.2 Análisis Térmico

**Potencia térmica disipada por componentes principales:**

| Componente | Potencia disipada | Ubicación | Base de cálculo |
|------------|-------------------|-----------|-----------------|
| XL6009 (pérdidas) | 350-820 mW | Placa filtrado (trasera) | η=88-92%, consumo 4.3-6.0 W |
| ESP32-WROOM-32 | 600-1200 mW | Placa control (centro) | WiFi AP activo |
| Nextion backlight | 2000-2550 mW | Frontal | 510 mA @ 5V |
| LM358 Buffer | 3.5 mW | Placa control | 0.7 mA @ 5V |
| CD4051 MUX | 2.5 mW | Placa control | 0.5 mA @ 5V |
| LED RGB | 160 mW | Placa control | 32 mA @ 5V |
| **TOTAL (modo promedio)** | **3.1 W** | | Consumo 853 mA @ 5V |
| **TOTAL (modo pico)** | **4.7 W** | | Consumo 1196 mA @ 5V |

**Análisis de disipación térmica:**

La potencia total disipada se debe evacuar para mantener la temperatura interna dentro de límites seguros (<70°C) y evitar:

1. **Degradación de baterías:** Li-ion pierden capacidad >60°C
2. **Reducción de vida útil del backlight Nextion:** Especificado hasta 50°C ambiente
3. **Posible activación de thermal throttling del ESP32:** >85°C internos

**Cálculo térmico simplificado (modelo resistivo):**

$$\Delta T = P_{diss} \times R_{th}$$

Donde:
- $P_{diss}$ = Potencia disipada total (W)
- $R_{th}$ = Resistencia térmica carcasa-ambiente (°C/W)
- $\Delta T$ = Incremento de temperatura interna respecto al ambiente

**Escenarios analizados:**

| Configuración | $R_{th}$ estimada | $P_{diss}$ | $\Delta T$ | $T_{int}$ @ 25°C ambiente |
|---------------|-------------------|------------|------------|---------------------------|
| Sin ventilación (carcasa sellada) | 15 °C/W | 4.7 W | 70.5 °C | **95.5 °C** ❌ NO ACEPTABLE |
| Con ventilación pasiva mínima | 12 °C/W | 4.7 W | 56.4 °C | **81.4 °C** ⚠️ LÍMITE |
| **Con ventilación pasiva optimizada** | **10 °C/W** | **3.1 W** | **31 °C** | **56 °C** ✅ ACEPTABLE |
| Con ventilación pasiva optimizada (pico) | 10 °C/W | 4.7 W | 47 °C | **72 °C** ✅ ACEPTABLE |

**Conclusión del análisis térmico:**

✅ **La ventilación pasiva es SUFICIENTE** para mantener temperaturas seguras (<75°C) bajo las siguientes condiciones:

1. **Diseño de ventilación:** Rejillas laterales inferiores (entrada) + perforaciones traseras superiores (salida)
2. **Área efectiva de ventilación:** ≥40 cm² total (20 cm² entrada + 20 cm² salida)
3. **Orientación:** Carcasa en posición vertical favorece efecto chimenea (convección natural)
4. **Separación componentes:** PCBs elevadas 10 mm del fondo para facilitar flujo de aire

**No se requiere ventilación forzada (ventiladores)** debido a:
- Consumo moderado (4.3 W promedio, picos cortos de 6.0 W)
- Distribución espacial de fuentes de calor
- Material PETG permite operar hasta 75°C según IEC 61010-1

#### 2.6.3 Estrategia de Ventilación Pasiva Implementada

**Principio de funcionamiento:**

```
┌─────────────────────────────────────────────────────────────────┐
│             VENTILACIÓN PASIVA POR CONVECCIÓN NATURAL           │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ╔═══════════════════════════════════════════════════╗         │
│  ║                 CARCASA PETG                      ║         │
│  ║                                                   ║         │
│  ║  ┌─────────────────────────────────────────┐     ║         │
│  ║  │         Nextion 7" (frontal)            │     ║         │
│  ║  │         ~2.5 W backlight                │     ║         │
│  ║  └─────────────────────────────────────────┘     ║         │
│  ║                                                   ║         │
│  ║         ┌─────────┐    ┌──────────┐              ║         │
│  ║  AIRE   │ ESP32   │    │  XL6009  │   AIRE       ║         │
│  ║  FRÍO → │ ~1.2 W  │    │  ~0.8 W  │ → CALIENTE   ║         │
│  ║  (25°C) └─────────┘    └──────────┘   (60°C)     ║         │
│  ║    ▲                                      │       ║         │
│  ║    │                                      ▼       ║         │
│  ║  [Rejillas laterales]        [Perforaciones      ║         │
│  ║   inferiores 20 cm²           traseras 20 cm²]   ║         │
│  ╚═══════════════════════════════════════════════════╝         │
│                                                                 │
│  FLUJO DE AIRE: ↑ Efecto chimenea (convección natural)         │
│  ÁREA EFECTIVA: 40 cm² total (entrada + salida)                │
│  ΔT ESPERADO: ~30-45°C (modo promedio/pico)                    │
└─────────────────────────────────────────────────────────────────┘
```

**Características del diseño de ventilación:**

| Parámetro | Especificación | Justificación |
|-----------|----------------|---------------|
| **Tipo de ventilación** | Pasiva por convección natural | Sin ruido, sin consumo adicional, sin piezas móviles |
| **Área entrada (laterales)** | 20 cm² (4× rejillas 5 cm²) | Suficiente para caudal natural |
| **Área salida (trasera)** | 20 cm² (matriz perforaciones) | Equilibra entrada, evita sobrepresión |
| **Separación PCB-fondo** | 10 mm | Permite flujo de aire bajo componentes |
| **Material carcasa** | PETG (k=0.29 W/m·K) | Aislamiento térmico moderado |
| **Orientación recomendada** | Vertical con rejillas abajo | Maximiza efecto chimenea |

**Dimensionamiento de perforaciones:**

- **Diámetro por orificio:** 3-5 mm (evita ingreso de dedos según IEC 61010-1)
- **Distribución:** Matriz regular espaciada 8-10 mm
- **Ubicación entrada:** Laterales inferiores (aire frío ambiente)
- **Ubicación salida:** Trasera superior (aire caliente asciende)

**Validación del diseño:**

Aplicando la ley de convección natural de Newton:

$$Q = h \cdot A \cdot \Delta T$$

Donde:
- $Q$ = Potencia disipada (4.7 W pico)
- $h$ = Coeficiente convección natural (~5 W/m²·K para PETG/aire)
- $A$ = Área superficial carcasa (~0.08 m²)
- $\Delta T$ = Diferencia temperatura superficie-ambiente

$$\Delta T = \frac{Q}{h \cdot A} = \frac{4.7}{5 \times 0.08} \approx 12°C$$

Sumando resistencia térmica interna (componentes → carcasa):

$$T_{int} = T_{amb} + \Delta T_{interno} + \Delta T_{superficie}$$
$$T_{int} = 25°C + 35°C + 12°C = 72°C$$

✅ **Resultado:** Temperatura interna <75°C en modo pico, cumple requisito IEC 61010-1

#### 2.6.4 Conclusión del Análisis Térmico

**✅ La ventilación pasiva es SUFICIENTE para la funcionalidad de todos los componentes electrónicos** del BioSignalSimulator Pro bajo las siguientes condiciones operativas:

1. **Nextion NX8048T070:** Especificado hasta 50°C ambiente. Con T_int=56°C (modo promedio), el backlight opera dentro de rango seguro.

2. **ESP32-WROOM-32:** Rango operativo -40°C a +85°C. Con T_int=72°C (pico), mantiene margen de 13°C antes de thermal throttling.

3. **Baterías Li-ion 18650:** Rango operativo 0-60°C. Con T_int=56-72°C, se recomienda limitar uso continuo >4 horas en ambientes cálidos (>30°C).

4. **XL6009 Step-Up:** Especificado hasta 85°C. Opera cómodamente bajo 75°C.

5. **LM358 Buffer:** Rango operativo -40°C a +85°C. Sin riesgo térmico.

**Ventajas de ventilación pasiva vs. ventilación forzada:**

| Aspecto | Pasiva | Forzada (ventilador) |
|---------|--------|----------------------|
| Consumo adicional | 0 mA | +50-100 mA |
| Ruido acústico | Silencioso | 20-30 dB |
| Complejidad | Ninguna | Control PWM, cableado |
| Costo | $0 | +$3-5 |
| Fiabilidad | Sin fallas (sin piezas móviles) | Falla de rodamiento (vida útil limitada) |
| Autonomía | Sin impacto | Reduce 10-15% |

**Recomendaciones de uso:**

- ✅ Orientar carcasa verticalmente (rejillas abajo, salidas arriba)
- ✅ No obstruir rejillas laterales ni traseras
- ✅ Uso en ambientes <30°C para maximizar autonomía
- ⚠️ En ambientes >35°C, limitar sesiones continuas a 2 horas

---

### 2.7 Limitaciones y Consideraciones

| Limitación | Descripción | Mitigación |
|------------|-------------|------------|
| Ventilación pasiva | Depende de orientación | Documentar posición óptima |
| PETG no metálico | Menor disipación que aluminio | Aumentar perforaciones |
| Impresión 3D | Tolerancias ±0.2 mm | Diseñar holguras controladas |
| Peso de la Nextion | ~180 g en frontal | Ubicar baterías en zona posterior |

### 2.8 Implicaciones Éticas y Legales (Mecánico)

#### 2.8.1 Consideraciones Éticas

| Aspecto | Consideración | Acción |
|---------|---------------|--------|
| Seguridad física | Prevenir quemaduras | Etiquetas de temperatura máxima |
| Ergonomía | Uso prolongado | Peso balanceado, bordes redondeados |
| Sostenibilidad | Impacto ambiental | PETG reciclable, piezas reemplazables |
| Accesibilidad | Fabricación local | Archivos STL liberados |

#### 2.8.2 Consideraciones Legales

| Aspecto | Requisito | Cumplimiento |
|---------|-----------|--------------|
| Propiedad intelectual | Diseño original | Modelos paramétricos propios |
| Seguridad de producto | Prototipo educativo | Exento de certificaciones |
| Documentación | Trazabilidad CAD | Versionado en repositorio |

---

## 3. Lista de Materiales (BOM Mecánico)

| # | Componente | Cantidad | Precio Unit. | Subtotal | Proveedor |
|---|------------|----------|--------------|----------|-----------|
| 1 | Filamento PETG (carcasa) | 100 g | $3.00 | $3.00 | Local |
| 2 | Tornillos M3×10 mm | 8 | $0.10 | $0.80 | Novatronic |
| 3 | Tuercas M3 | 8 | $0.05 | $0.40 | Novatronic |
| 4 | Separadores M3×5 mm | 4 | $0.15 | $0.60 | Novatronic |
| 5 | Pies de goma adhesivos | 4 | $0.10 | $0.40 | Novatronic |
| 6 | Etiquetas adhesivas | 1 | $0.50 | $0.50 | Local |
| | **SUBTOTAL MECÁNICO** | | | **$5.70** | |

---

## 4. Referencias

1. IEC 61010-1:2010 – Safety requirements for electrical equipment (partes mecánicas)  
2. IEC 62133:2012 – Requisitos térmicos para baterías Li-ion  
3. Datasheet Nextion NX8048T070 – Parámetros de backlight  
4. SolidWorks Handbook – Diseño para impresión 3D  
5. Guías de impresión PETG (Prusa Research)
