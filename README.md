# BioSimulator Pro

**Simulador de señales biológicas para educación e investigación**

---

| Campo | Valor |
|-------|-------|
| **Versión** | 1.1.0 |
| **Fecha** | Diciembre 2024 |
| **Autor** | BioSimulator Pro Team |
| **Licencia** | MIT |

---

## Descripción

BioSimulator Pro es un dispositivo portátil que genera señales biológicas realistas (ECG, EMG, PPG) para:

- **Educación**: Enseñanza de fisiología y procesamiento de señales
- **Investigación**: Desarrollo y prueba de algoritmos
- **Calibración**: Verificación de equipos médicos
- **Desarrollo**: Prototipado de dispositivos wearables

## Características

- **3 tipos de señales**: ECG, EMG, PPG
- **22 condiciones clínicas** simuladas
- **Modelos matemáticos** validados científicamente
- **Salida analógica** 0-3.3V (conector BNC)
- **Pantalla táctil** Nextion 3.2" con interfaz intuitiva
- **Portátil**: Batería Li-ion 2800mAh (~5h autonomía)

## Arquitectura

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                            BIOSIMULATOR PRO v1.1                            │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   ┌─────────────────┐         UART          ┌─────────────────┐            │
│   │   ESP32         │◄─────────────────────►│   NEXTION       │            │
│   │   NodeMCU       │       115200          │   NX4024T032    │            │
│   │                 │                        │                 │            │
│   │   • Generación  │                        │   • UI táctil   │            │
│   │   • DAC output  │                        │   • Waveform    │            │
│   │   • Modelos     │                        │   • Controles   │            │
│   └────────┬────────┘                        └─────────────────┘            │
│            │                                                                │
│            │ DAC (0-3.3V)                                                   │
│            ▼                                                                │
│   ┌─────────────────┐                                                       │
│   │   BNC Output    │──────────► Osciloscopio / Arduino / Prototipado      │
│   └─────────────────┘                                                       │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

## Estructura del Proyecto

```
BioSimulator Pro/
├── README.md                   # Este archivo
├── platformio.ini              # Configuración PlatformIO
├── src/
│   ├── main.cpp                # Punto de entrada principal
│   ├── main_debug.cpp          # Modo debug para Serial Plotter
│   ├── core/                   # Motor de señales y control
│   ├── models/                 # Modelos ECG, EMG, PPG
│   └── comm/                   # Comunicación Nextion y serial
├── include/
│   ├── config.h                # Configuración global
│   ├── core/                   # Headers del core
│   ├── models/                 # Headers de modelos
│   └── data/                   # Tipos y límites de parámetros
├── docs/
│   ├── HARDWARE_ELECTRONICS.md # Diseño electrónico
│   ├── README_NEXTION_UI.md    # Documentación de interfaz
│   ├── README_MATHEMATICAL_BASIS.md
│   └── README_COMPUTATIONAL_BASIS.md
└── tools/                      # Scripts de validación
    ├── signal_validator.py
    └── clinical_ranges.py
```

## Hardware

| Componente | Modelo | Función |
|------------|--------|---------|
| MCU | ESP32-WROOM-32 (NodeMCU) | Generación de señales |
| Display | Nextion NX4024T032 3.2" | Interfaz táctil |
| Baterías | 2× Steren 18650 2800mAh (serie) | 7.4V |
| Regulador | XL4015 Buck | 7.4V → 5V |
| BMS | HX-2S-D01 | Protección y balanceo |
| Buffer | MCP6002 | Salida analógica |
| Conector | BNC hembra | Salida de señal |

## Señales Soportadas

### ECG (8 condiciones)
Normal, Taquicardia, Bradicardia, Fibrilación auricular, Fibrilación ventricular, PVC, Elevación ST, Depresión ST

### EMG (8 condiciones)
Reposo, Contracción baja/moderada/alta, Temblor, Miopatía, Neuropatía, Fasciculación

### PPG (6 condiciones)
Normal, Arritmia, Perfusión débil/fuerte, Vasoconstricción, SpO2 bajo

## Compilación

```bash
# Compilar
pio run

# Subir al ESP32
pio run --target upload

# Monitor serial
pio device monitor --baud 115200
```

## Modo Debug (Serial Plotter)

Para verificar señales con Arduino Serial Plotter, editar `src/main_debug.cpp`:

```cpp
#define AUTO_START 1
#define AUTO_SIGNAL_TYPE 0  // 0=ECG, 1=EMG, 2=PPG
#define AUTO_ECG_CONDITION 0  // Ver opciones en el archivo
```

Luego compilar con el entorno debug en `platformio.ini`.

## Validación de Señales

```bash
# Validar señal específica
python tools/signal_validator.py --port COM4 --signal ecg --condition NORMAL

# Ver rangos clínicos
python tools/signal_validator.py --show-ranges
```

## Documentación

- [Diseño Electrónico](docs/HARDWARE_ELECTRONICS.md)
- [Interfaz Nextion](docs/README_NEXTION_UI.md)
- [Base Matemática](docs/README_MATHEMATICAL_BASIS.md)
- [Base Computacional](docs/README_COMPUTATIONAL_BASIS.md)

## Changelog

### v1.1.0 (Diciembre 2024)
- Validación completa de rangos clínicos
- Herramientas de validación Python
- Modo debug para Serial Plotter
- Documentación de hardware electrónico
- Sistema de alimentación 2S (7.4V) con baterías Steren 2800mAh

### v1.0.0
- Versión inicial
- Modelos ECG (McSharry), EMG (Fuglevand), PPG (Allen)
- Interfaz Nextion completa
- 22 condiciones clínicas

## Licencia

MIT License - Ver archivo LICENSE para detalles.
