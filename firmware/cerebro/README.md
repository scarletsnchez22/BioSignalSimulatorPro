# BioSignalSimulator Pro - Firmware Cerebro

**Firmware principal para generación de señales biológicas**

---

| Campo | Valor |
|-------|-------|
| **Versión** | 1.1.0 |
| **Fecha** | Diciembre 2024 |
| **Estado** | ✅ Funcional |
| **Hardware** | ESP32-WROOM-32 (NodeMCU v1.1) |

---

## Descripción

Este firmware es el "cerebro" del BioSignalSimulator Pro. Se encarga de:

- Generación de señales ECG, EMG y PPG en tiempo real
- Salida analógica DAC (0-3.3V) para osciloscopio
- Comunicación UART con la pantalla HMI
- Gestión de parámetros y condiciones clínicas

## Hardware

| Componente | Pin | Función |
|------------|-----|---------|
| DAC1 | GPIO25 | Salida señal principal |
| DAC2 | GPIO26 | Salida secundaria (opcional) |
| UART TX | GPIO17 | Transmisión a HMI |
| UART RX | GPIO16 | Recepción de HMI |
| LED Rojo | GPIO4 | Indicador RGB |
| LED Verde | GPIO5 | Indicador RGB |
| LED Azul | GPIO18 | Indicador RGB |

## Señales Soportadas

### ECG (Electrocardiograma)
- Normal
- Taquicardia
- Bradicardia
- Fibrilación auricular
- Fibrilación ventricular
- Contracción ventricular prematura
- Elevación ST
- Depresión ST

### EMG (Electromiografía)
- Reposo
- Contracción baja
- Contracción moderada
- Contracción alta
- Temblor
- Miopatía
- Neuropatía
- Fasciculación

### PPG (Fotopletismografía)
- Normal
- Arritmia
- Perfusión débil
- Perfusión fuerte
- Vasoconstricción
- SpO2 bajo

## Estructura de Archivos

```
firmware/cerebro/
├── platformio.ini          # Configuración PlatformIO
├── README.md               # Este archivo
├── src/
│   ├── main_debug.cpp      # Punto de entrada (modo debug)
│   ├── core/
│   │   ├── signal_engine.cpp
│   │   ├── param_controller.cpp
│   │   └── state_machine.cpp
│   ├── models/
│   │   ├── ecg_model.cpp
│   │   ├── emg_model.cpp
│   │   └── ppg_model.cpp
│   └── comm/
│       └── serial_handler.cpp
├── include/
│   ├── config.h            # Configuración global
│   ├── core/
│   ├── models/
│   ├── data/
│   └── comm/
├── lib/
└── test/
```

## Compilación

```bash
cd firmware/cerebro
pio run
pio run --target upload
```

## Monitor Serial

```bash
pio device monitor --baud 115200
```

## Modo Debug (Serial Plotter)

El firmware incluye un modo debug interactivo que permite:

1. Seleccionar tipo de señal
2. Seleccionar condición clínica
3. Visualizar en Arduino Serial Plotter

Para usar con Serial Plotter, configurar en `main_debug.cpp`:
```cpp
#define AUTO_START 1
#define AUTO_SIGNAL_TYPE 0  // 0=ECG, 1=EMG, 2=PPG
```

## Comunicación con HMI

| Parámetro | Valor |
|-----------|-------|
| **Interfaz** | UART2 (Serial2) |
| **Velocidad** | 921600 baud |
| **Formato** | 8N1 |

### Formato de Datos (Cerebro → HMI)

```
>ecg:VALUE,hr:VALUE,rr:VALUE,st:VALUE,beats:VALUE
>emg:VALUE,rms:VALUE,mvc:VALUE,freq:VALUE,units:VALUE
>ppg:VALUE,hr:VALUE,rr:VALUE,pi:VALUE,spo2:VALUE,beats:VALUE
```

## Changelog

### v1.1.0 (Diciembre 2024)
- Migración de Nextion a ELECROW HMI
- Velocidad UART aumentada a 921600 baud
- Reestructuración de carpetas del proyecto
- Eliminación de código Nextion obsoleto

### v1.0.0
- Versión inicial con soporte Nextion
- Modelos ECG, EMG, PPG completos
- Validación clínica de rangos
