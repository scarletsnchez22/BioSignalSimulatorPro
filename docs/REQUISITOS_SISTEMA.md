# Especificación de Requisitos del Sistema BioSignalSimulator Pro

**Grupo #22:** Scarlet Sánchez y Rafael Mata  
**Institución:** Escuela Superior Politécnica del Litoral (ESPOL)  
**Versión:** 1.0.0  
**Fecha:** Enero 2026

---

## Índice

1. [Requisitos Funcionales](#1-requisitos-funcionales)
2. [Requisitos No Funcionales](#2-requisitos-no-funcionales)
3. [Matriz de Trazabilidad](#3-matriz-de-trazabilidad)

---

## 1. Requisitos Funcionales

Los requisitos funcionales describen las capacidades específicas que el sistema debe proporcionar.

### 1.1 Generación de Señales Biomédicas (RF-GEN)

| ID | Requisito | Especificación | Origen |
|----|-----------|----------------|--------|
| RF-GEN-01 | Generar señal ECG sintética | Modelo McSharry ECGSYN con 8 condiciones cardíacas configurables | METODOLOGIA_SIGNALGEN |
| RF-GEN-02 | Generar señal EMG sintética | Modelo Fuglevand con 6 condiciones musculares y reclutamiento de MUs | METODOLOGIA_SIGNALGEN |
| RF-GEN-03 | Generar señal PPG sintética | Modelo empírico de suma de gaussianas con 6 condiciones vasculares | METODOLOGIA_SIGNALGEN |
| RF-GEN-04 | Frecuencia cardíaca ajustable | ECG: 40-180 BPM, PPG: 40-180 BPM | METODOLOGIA_SIGNALGEN |
| RF-GEN-05 | Excitación muscular ajustable | EMG: 0-100% MVC (contracción voluntaria máxima) | METODOLOGIA_SIGNALGEN |
| RF-GEN-06 | Variabilidad fisiológica | HRV (variabilidad RR) configurable en ECG/PPG | METODOLOGIA_SIGNALGEN |
| RF-GEN-07 | Ruido configurable | Nivel de ruido gaussiano ajustable por señal (0-20%) | METODOLOGIA_SIGNALGEN |
| RF-GEN-08 | Simulación de patologías ECG | Taquicardia, bradicardia, FA, FV, bloqueo AV, elevación/depresión ST | METODOLOGIA_SIGNALGEN |
| RF-GEN-09 | Simulación de condiciones EMG | Reposo, contracción leve/moderada/máxima, temblor, fatiga | METODOLOGIA_SIGNALGEN |
| RF-GEN-10 | Simulación de condiciones PPG | Normal, taquicardia, bradicardia, baja perfusión, hipertensión, arritmia | METODOLOGIA_SIGNALGEN |

### 1.2 Electrónica y Hardware (RF-HW)

| ID | Requisito | Especificación | Origen |
|----|-----------|----------------|--------|
| RF-HW-01 | Salida analógica única | Compatible con equipos de laboratorio y microcontroladores (0-3.3V vía DAC ESP32 + buffer LM358) | METODOLOGIA_ELECTRONICA |
| RF-HW-02 | Conector BNC | Salida para conexión a osciloscopio/equipo médico | METODOLOGIA_ELECTRONICA |
| RF-HW-03 | Filtrado selectivo por señal | RC pasabajos: ECG (23.4 Hz), EMG (159 Hz), PPG (6.37 Hz) | METODOLOGIA_ELECTRONICA |
| RF-HW-04 | Multiplexación automática | CD4051 selecciona filtro RC según tipo de señal activa | METODOLOGIA_ELECTRONICA |
| RF-HW-05 | Alimentación autónoma | Batería Li-ion 2×18650 paralelo (5200 mAh) recargable | METODOLOGIA_ELECTRONICA |
| RF-HW-06 | Carga por USB-C | Módulo IP5306 con carga hasta 2A | METODOLOGIA_ELECTRONICA |
| RF-HW-07 | Indicación visual de estado | LED RGB multicolor para estados del sistema | METODOLOGIA_ELECTRONICA |
| RF-HW-08 | Switch ON/OFF físico | Interruptor deslizable para encendido/apagado | METODOLOGIA_ELECTRONICA |

### 1.3 Interfaz de Usuario Local (RF-UI)

| ID | Requisito | Especificación | Origen |
|----|-----------|----------------|--------|
| RF-UI-01 | Pantalla táctil | Nextion NX8048T070 7" 800×480 capacitiva | METODOLOGIA_ELECTRONICA |
| RF-UI-02 | Visualización waveform | Gráfico en tiempo real de la señal generada (700×380 px) | METODOLOGIA_COMPUTACIONAL |
| RF-UI-03 | Selección de tipo de señal | Menú táctil: ECG, EMG, PPG | METODOLOGIA_COMPUTACIONAL |
| RF-UI-04 | Selección de condición | Menú táctil con condiciones disponibles por señal | METODOLOGIA_COMPUTACIONAL |
| RF-UI-05 | Ajuste de parámetros | Sliders táctiles para HR, amplitud, ruido | METODOLOGIA_COMPUTACIONAL |
| RF-UI-06 | Visualización de métricas | HR, RR, QRS, ST (ECG); RMS, activación (EMG); PI, SpO2 (PPG) | METODOLOGIA_COMPUTACIONAL |
| RF-UI-07 | Control de simulación | Botones: Iniciar, Pausar, Detener | METODOLOGIA_COMPUTACIONAL |
| RF-UI-08 | Navegación por páginas | Portada → Menú → Selección → Simulación → Pausa | METODOLOGIA_COMPUTACIONAL |

### 1.4 Conectividad y Comunicación (RF-COM)

| ID | Requisito | Especificación | Origen |
|----|-----------|----------------|--------|
| RF-COM-01 | WiFi Access Point | ESP32 como AP: SSID "BioSimulator_Pro", IP 192.168.4.1 | METODOLOGIA_APPWEB |
| RF-COM-02 | Servidor HTTP embebido | Servir archivos web desde SPIFFS (index.html, app.js, styles.css) | METODOLOGIA_APPWEB |
| RF-COM-03 | WebSocket streaming | Puerto 81, transmisión JSON a 100 Hz | METODOLOGIA_APPWEB |
| RF-COM-04 | Soporte multi-cliente | Hasta 6 conexiones simultáneas | METODOLOGIA_APPWEB |
| RF-COM-05 | Broadcast de datos | Transmitir señal y métricas a todos los clientes conectados | METODOLOGIA_APPWEB |
| RF-COM-06 | UART Nextion | Comunicación bidireccional a 115200 baud | METODOLOGIA_COMPUTACIONAL |

### 1.5 Aplicación Web (RF-APP)

| ID | Requisito | Especificación | Origen |
|----|-----------|----------------|--------|
| RF-APP-01 | Visualización tiempo real | Canvas plotter con streaming de señal a 60 FPS | METODOLOGIA_APPWEB |
| RF-APP-02 | Métricas en vivo | Mostrar HR, RR, amplitudes según tipo de señal | METODOLOGIA_APPWEB |
| RF-APP-03 | Zoom local | Control de escala vertical por usuario (sin afectar otros) | METODOLOGIA_APPWEB |
| RF-APP-04 | Pausa local | Congelar visualización sin desconectar WebSocket | METODOLOGIA_APPWEB |
| RF-APP-05 | Descarga CSV | Exportar buffer de datos a archivo CSV | METODOLOGIA_APPWEB |
| RF-APP-06 | Captura de pantalla | Guardar canvas como imagen PNG | METODOLOGIA_APPWEB |
| RF-APP-07 | Interfaz responsive | Adaptable a móvil, tablet y desktop | METODOLOGIA_APPWEB |
| RF-APP-08 | Indicador de conexión | Estado visual: Conectado/Desconectado/Conectando | METODOLOGIA_APPWEB |

### 1.6 Computacional y Software (RF-SW)

| ID | Requisito | Especificación | Origen |
|----|-----------|----------------|--------|
| RF-SW-01 | Generación en tiempo real | Tasa de muestreo DAC: 2 kHz (jitter < ±50 µs) | METODOLOGIA_COMPUTACIONAL |
| RF-SW-02 | Separación de núcleos | Core 0: UI/WiFi, Core 1: Generación de señales | METODOLOGIA_COMPUTACIONAL |
| RF-SW-03 | Buffer circular | 2048 muestras para desacoplamiento generación/DAC | METODOLOGIA_COMPUTACIONAL |
| RF-SW-04 | Detección de underruns | Monitoreo de buffer vacío con estadísticas | METODOLOGIA_COMPUTACIONAL |
| RF-SW-05 | Calibración automática | Auto-escalado de amplitud basado en picos detectados | METODOLOGIA_SIGNALGEN |
| RF-SW-06 | Máquina de estados | Estados: INIT, PORTADA, MENU, SELECT_COND, SIMULATING, PAUSED | METODOLOGIA_COMPUTACIONAL |
| RF-SW-07 | Persistencia de configuración | Almacenar última configuración en flash/EEPROM | METODOLOGIA_COMPUTACIONAL |

---

## 2. Requisitos No Funcionales

Los requisitos no funcionales describen las cualidades y restricciones del sistema.

### 2.1 Rendimiento (RNF-PERF)

| ID | Requisito | Especificación | Origen |
|----|-----------|----------------|--------|
| RNF-PERF-01 | Frecuencia de muestreo DAC | 2000 Hz (500 µs por muestra) | METODOLOGIA_COMPUTACIONAL |
| RNF-PERF-02 | Latencia DAC | < 1 ms desde generación hasta salida analógica | METODOLOGIA_COMPUTACIONAL |
| RNF-PERF-03 | Refresh UI Nextion | Waveform: 100-200 Hz, Métricas: 4 Hz | METODOLOGIA_COMPUTACIONAL |
| RNF-PERF-04 | Frecuencia WebSocket | 100 Hz señal, 4 Hz métricas | METODOLOGIA_APPWEB |
| RNF-PERF-05 | Tiempo de arranque | < 5 segundos desde encendido hasta pantalla principal | METODOLOGIA_COMPUTACIONAL |
| RNF-PERF-06 | Resolución DAC | 8-bit (256 niveles, 0-3.3V) | METODOLOGIA_ELECTRONICA |

### 2.2 Energía y Autonomía (RNF-PWR)

| ID | Requisito | Especificación | Origen |
|----|-----------|----------------|--------|
| RNF-PWR-01 | Autonomía mínima | ≥ 3 horas de uso continuo (cumple: 3.9 h calculadas) | METODOLOGIA_ELECTRONICA |
| RNF-PWR-02 | Capacidad de batería | 5200 mAh (2×18650 paralelo) | METODOLOGIA_ELECTRONICA |
| RNF-PWR-03 | Consumo promedio | 851 mA @ 5V (4.26 W) | METODOLOGIA_ELECTRONICA |
| RNF-PWR-04 | Consumo pico | 1194 mA @ 5V (5.97 W) | METODOLOGIA_ELECTRONICA |
| RNF-PWR-05 | Tiempo de carga | < 6 horas (carga a 2A vía USB-C) | METODOLOGIA_ELECTRONICA |
| RNF-PWR-06 | Eficiencia regulador | XL6009: 88-92% | METODOLOGIA_ELECTRONICA |

### 2.3 Seguridad Eléctrica (RNF-SEG)

| ID | Requisito | Especificación | Origen |
|----|-----------|----------------|--------|
| RNF-SEG-01 | Voltaje máximo de salida | < 5V DC (clasificación SELV) | METODOLOGIA_ELECTRONICA |
| RNF-SEG-02 | Corriente limitada | < 25 mA en salida BNC (resistencia serie 100Ω) | METODOLOGIA_ELECTRONICA |
| RNF-SEG-03 | Protección sobrecarga batería | IP5306 corte a 4.2V ±0.5% | METODOLOGIA_ELECTRONICA |
| RNF-SEG-04 | Protección sobredescarga | BMS DW01 corte a 2.5V | METODOLOGIA_ELECTRONICA |
| RNF-SEG-05 | Protección cortocircuito | DW01 límite 3A + Fusible 1.5A | METODOLOGIA_ELECTRONICA |
| RNF-SEG-06 | Aislamiento | Carcasa PETG no conductora, batería aislada de salidas | METODOLOGIA_ELECTRONICA |
| RNF-SEG-07 | Cumplimiento IEC 61010-1 | Categoría CAT I, Grado contaminación 2 | METODOLOGIA_ELECTRONICA |
| RNF-SEG-08 | Cumplimiento IEC 62133 | Seguridad de baterías Li-ion | METODOLOGIA_ELECTRONICA |

### 2.4 Mecánica y Encapsulado (RNF-MEC)

| ID | Requisito | Especificación | Origen |
|----|-----------|----------------|--------|
| RNF-MEC-01 | Peso total ensamblado | < 2 kg (cumple: ~350 g) | METODOLOGIA_MECANICA |
| RNF-MEC-02 | Dimensiones externas | 180 × 120 × 45 mm | METODOLOGIA_MECANICA |
| RNF-MEC-03 | Material carcasa | PETG impreso 3D, espesor 2.5 mm | METODOLOGIA_MECANICA |
| RNF-MEC-04 | Tornillería estándar | M3 para ensamblaje exterior | METODOLOGIA_MECANICA |
| RNF-MEC-05 | Vida útil mecánica | > 2 años en uso de laboratorio | METODOLOGIA_MECANICA |
| RNF-MEC-06 | Bordes redondeados | Radios ≥ 2 mm para seguridad y ergonomía | METODOLOGIA_MECANICA |

### 2.5 Térmico (RNF-TERM)

| ID | Requisito | Especificación | Origen |
|----|-----------|----------------|--------|
| RNF-TERM-01 | Temperatura interna máxima | < 75°C (modo pico: 72°C calculado) | METODOLOGIA_MECANICA |
| RNF-TERM-02 | Temperatura superficial | < 80°C (PETG no metálico, IEC 61010-1) | METODOLOGIA_MECANICA |
| RNF-TERM-03 | Disipación total | 3.1 W promedio, 4.7 W pico | METODOLOGIA_MECANICA |
| RNF-TERM-04 | Tipo de ventilación | Pasiva por convección natural (sin ventiladores) | METODOLOGIA_MECANICA |
| RNF-TERM-05 | Área de ventilación | ≥ 40 cm² total (20 cm² entrada + 20 cm² salida) | METODOLOGIA_MECANICA |

### 2.6 Fidelidad de Señales (RNF-SIG)

| ID | Requisito | Especificación | Origen |
|----|-----------|----------------|--------|
| RNF-SIG-01 | Rango ECG | -0.5 mV a +1.5 mV (morfología PQRST completa) | METODOLOGIA_SIGNALGEN |
| RNF-SIG-02 | Rango EMG | -5.0 mV a +5.0 mV (señal cruda bipolar) | METODOLOGIA_SIGNALGEN |
| RNF-SIG-03 | Rango PPG | 0 a +2.0 mV (pulso unipolar) | METODOLOGIA_SIGNALGEN |
| RNF-SIG-04 | Ancho de banda ECG (99% energía) | 21.6 Hz | METODOLOGIA_COMPUTACIONAL |
| RNF-SIG-05 | Ancho de banda EMG (99% energía) | 146.3 Hz | METODOLOGIA_COMPUTACIONAL |
| RNF-SIG-06 | Ancho de banda PPG (99% energía) | 4.9 Hz | METODOLOGIA_COMPUTACIONAL |
| RNF-SIG-07 | Frecuencia de muestreo ECG (modelo) | 300 Hz | METODOLOGIA_SIGNALGEN |
| RNF-SIG-08 | Frecuencia de muestreo EMG (modelo) | 1000 Hz | METODOLOGIA_SIGNALGEN |
| RNF-SIG-09 | Frecuencia de muestreo PPG (modelo) | 20 Hz | METODOLOGIA_SIGNALGEN |

### 2.7 Comunicación WiFi (RNF-WIFI)

| ID | Requisito | Especificación | Origen |
|----|-----------|----------------|--------|
| RNF-WIFI-01 | Estándar WiFi | 802.11 b/g/n | METODOLOGIA_APPWEB |
| RNF-WIFI-02 | Modo de operación | Access Point (sin router externo) | METODOLOGIA_APPWEB |
| RNF-WIFI-03 | Máximo de clientes | 6 conexiones simultáneas | METODOLOGIA_APPWEB |
| RNF-WIFI-04 | Ancho de banda por cliente | ~15 KB/s (150 bytes × 100 Hz) | METODOLOGIA_APPWEB |
| RNF-WIFI-05 | Rango DHCP | 192.168.4.2 - 192.168.4.10 | METODOLOGIA_APPWEB |

### 2.8 Usabilidad (RNF-USA)

| ID | Requisito | Especificación | Origen |
|----|-----------|----------------|--------|
| RNF-USA-01 | Curva de aprendizaje | Uso sin entrenamiento previo en < 5 minutos | METODOLOGIA_APPWEB |
| RNF-USA-02 | Idioma de interfaz | Español | METODOLOGIA_COMPUTACIONAL |
| RNF-USA-03 | Retroalimentación visual | Indicadores de estado en pantalla y LED RGB | METODOLOGIA_ELECTRONICA |
| RNF-USA-04 | Acceso a conectores | BNC, USB-C y switch accesibles sin desarmar | METODOLOGIA_MECANICA |

### 2.9 Manufactura y Costo (RNF-MFG)

| ID | Requisito | Especificación | Origen |
|----|-----------|----------------|--------|
| RNF-MFG-01 | Presupuesto total | < $150 USD | METODOLOGIA_ELECTRONICA |
| RNF-MFG-02 | Disponibilidad de componentes | Componentes disponibles localmente (Ecuador) | METODOLOGIA_ELECTRONICA |
| RNF-MFG-03 | Tiempo de impresión 3D | < 8 horas por set de carcasa | METODOLOGIA_MECANICA |
| RNF-MFG-04 | Complejidad de ensamblaje | Soldadura manual, encapsulados DIP/through-hole | METODOLOGIA_ELECTRONICA |
| RNF-MFG-05 | Documentación técnica | Diagramas, BOM y archivos STL completos | METODOLOGIA_MECANICA |

### 2.10 Mantenibilidad (RNF-MNT)

| ID | Requisito | Especificación | Origen |
|----|-----------|----------------|--------|
| RNF-MNT-01 | Modularidad mecánica | Tapa superior e inferior independientes | METODOLOGIA_MECANICA |
| RNF-MNT-02 | Conectores desmontables | Módulos de alimentación separables | METODOLOGIA_ELECTRONICA |
| RNF-MNT-03 | Sin pegamentos | Ensamblaje solo con tornillos | METODOLOGIA_MECANICA |
| RNF-MNT-04 | Acceso a baterías | Reemplazo sin herramientas especiales | METODOLOGIA_MECANICA |
| RNF-MNT-05 | Actualización de firmware | Vía USB (programación Arduino/PlatformIO) | METODOLOGIA_COMPUTACIONAL |

---

## 3. Matriz de Trazabilidad

### 3.1 Requisitos por Subsistema

| Subsistema | Requisitos Funcionales | Requisitos No Funcionales | Total |
|------------|------------------------|---------------------------|-------|
| Generación de Señales | RF-GEN-01 a RF-GEN-10 (10) | RNF-SIG-01 a RNF-SIG-09 (9) | 19 |
| Hardware Electrónico | RF-HW-01 a RF-HW-08 (8) | RNF-SEG-01 a RNF-SEG-08 (8), RNF-PWR-01 a RNF-PWR-06 (6) | 22 |
| Interfaz Local (Nextion) | RF-UI-01 a RF-UI-08 (8) | RNF-PERF-03 (1), RNF-USA-01 a RNF-USA-04 (4) | 13 |
| Conectividad WiFi | RF-COM-01 a RF-COM-06 (6) | RNF-WIFI-01 a RNF-WIFI-05 (5), RNF-PERF-04 (1) | 12 |
| Aplicación Web | RF-APP-01 a RF-APP-08 (8) | RNF-USA-01 (1) | 9 |
| Software Embebido | RF-SW-01 a RF-SW-07 (7) | RNF-PERF-01 a RNF-PERF-06 (6) | 13 |
| Mecánica/Carcasa | - | RNF-MEC-01 a RNF-MEC-06 (6), RNF-TERM-01 a RNF-TERM-05 (5), RNF-MNT-01 a RNF-MNT-05 (5) | 16 |
| Manufactura | - | RNF-MFG-01 a RNF-MFG-05 (5) | 5 |
| **TOTAL** | **47** | **62** | **109** |

### 3.2 Resumen de Requisitos

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                   RESUMEN DE REQUISITOS DEL SISTEMA                          │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  REQUISITOS FUNCIONALES (47)                                                │
│  ─────────────────────────────                                              │
│  • Generación de Señales:     10 requisitos (ECG, EMG, PPG + patologías)    │
│  • Hardware Electrónico:       8 requisitos (DAC, BNC, batería, carga)      │
│  • Interfaz Usuario Local:     8 requisitos (Nextion táctil, waveform)      │
│  • Conectividad:               6 requisitos (WiFi AP, WebSocket)            │
│  • Aplicación Web:             8 requisitos (plotter, zoom, CSV)            │
│  • Software Embebido:          7 requisitos (tiempo real, estados)          │
│                                                                             │
│  REQUISITOS NO FUNCIONALES (62)                                             │
│  ───────────────────────────────                                            │
│  • Rendimiento:                6 requisitos (latencia, frecuencias)         │
│  • Energía/Autonomía:          6 requisitos (≥3h, 5200mAh)                  │
│  • Seguridad Eléctrica:        8 requisitos (SELV, IEC 61010-1)             │
│  • Mecánica:                   6 requisitos (<2 kg, PETG)                   │
│  • Térmico:                    5 requisitos (<75°C, ventilación pasiva)     │
│  • Fidelidad de Señales:       9 requisitos (rangos mV, BW, Fs)             │
│  • WiFi:                       5 requisitos (AP, 6 clientes)                │
│  • Usabilidad:                 4 requisitos (idioma, accesibilidad)         │
│  • Manufactura:                5 requisitos (<$150, local)                  │
│  • Mantenibilidad:             5 requisitos (modular, sin pegamento)        │
│                                                                             │
│  NORMAS APLICABLES                                                          │
│  ─────────────────                                                          │
│  • IEC 61010-1: Seguridad de equipos de medida                              │
│  • IEC 62133: Seguridad de baterías de litio                                │
│  • IEC 60950-1: Seguridad de equipos TI                                     │
│  • RoHS: Restricción de sustancias peligrosas                               │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Historial de Cambios

| Versión | Fecha | Autor | Descripción |
|---------|-------|-------|-------------|
| 1.0.0 | Enero 2026 | Grupo #22 | Versión inicial consolidada |

---

*Documento generado a partir de las metodologías: METODOLOGIA_SIGNALGEN.md, METODOLOGIA_ELECTRONICA.md, METODOLOGIA_MECANICA.md, METODOLOGIA_APPWEB.md, METODOLOGIA_COMPUTACIONAL.md*
