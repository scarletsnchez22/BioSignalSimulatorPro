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

| Componente | Potencia disipada | Ubicación |
|------------|-------------------|-----------|
| MT3608 (pérdidas) | 450-580 mW | Zona trasera |
| ESP32-WROOM-32 | 600-1200 mW | Centro PCB |
| LM358 | 3.5 mW | PCB |
| Nextion backlight | 2000-2550 mW | Frontal |
| **TOTAL** | **3.1-4.3 W** | |

**Escenario sin ventilación:**  
R_th ≈ 15 °C/W ⇒ ΔT ≈ 52 °C ⇒ T_int ≈ 77 °C (no aceptable).

**Diseño actual (ventilación pasiva):**  
R_th ≈ 12 °C/W ⇒ ΔT ≈ 42 °C ⇒ T_int ≈ 67 °C (dentro de límites).  
En uso intensivo (4.3 W) el pico estimado es 76 °C, por lo que se recomienda orientar la carcasa para favorecer el efecto chimenea.

#### 2.6.3 Estrategia de Ventilación Pasiva

```
┌─────────────────────────────────────────────────────────────────┐
│                 VENTILACIÓN PASIVA                              │
├─────────────────────────────────────────────────────────────────┤
│  Aire frío entra por laterales inferiores                       │
│  Se calienta al pasar por ESP32/MT3608                          │
│  Sale por orificios traseros (efecto chimenea)                  │
└─────────────────────────────────────────────────────────────────┘
```

#### 2.6.4 Justificación de Autonomía ≥3h (Impacto mecánico)

La ventilación pasiva mantiene la temperatura interna en ~67 °C cuando el consumo es 1.17 A (WiFi activo, brillo 80%). Este control térmico evita derating de la batería y protege el backlight, permitiendo sostener la autonomía ≥3 horas sin ventiladores.

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
