# BioSignalSimulator Pro

**Simulador de señales biológicas para educación e investigación**

---

| Campo | Valor |
|-------|-------|
| **Versión** | 1.1.0 |
| **Fecha** | Diciembre 2024 |
| **Autor** | BioSignalSimulator Pro Team |
| **Licencia** | MIT |

---

## Descripción

BioSignalSimulator Pro es un dispositivo portátil que genera señales biológicas realistas (ECG, EMG, PPG) para:

- 🎓 **Educación**: Enseñanza de fisiología y procesamiento de señales
- 🔬 **Investigación**: Desarrollo y prueba de algoritmos
- 🏥 **Calibración**: Verificación de equipos médicos
- 💻 **Desarrollo**: Prototipado de dispositivos wearables

## Características

- ✅ **3 tipos de señales**: ECG, EMG, PPG
- ✅ **22 condiciones clínicas** simuladas
- ✅ **Modelos matemáticos** validados científicamente
- ✅ **Salida analógica** 0-3.3V (conector BNC)
- ✅ **Pantalla táctil** 7" 800x480
- ✅ **Portátil**: Batería Li-ion 4400mAh (~5.5h autonomía)
- ✅ **Conectividad WiFi** para app web

## Arquitectura

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         BIOSIGNALSIMULATOR PRO                              │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   ┌─────────────────┐         UART          ┌─────────────────┐            │
│   │   ESP32         │◄─────────────────────►│   ELECROW       │            │
│   │   CEREBRO       │       921600          │   HMI 7"        │            │
│   │                 │                        │                 │            │
│   │   • Generación  │                        │   • LVGL UI     │            │
│   │   • DAC output  │                        │   • Touch       │            │
│   │   • WiFi        │                        │   • Gráficos    │            │
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
BioSignalSimulator Pro/
├── README.md                   # Este archivo
├── firmware/
│   ├── cerebro/                # ESP32 - Generación de señales
│   │   ├── src/
│   │   ├── include/
│   │   └── platformio.ini
│   └── hmi/                    # ESP32-S3 - Pantalla táctil
│       ├── src/
│       ├── include/
│       └── platformio.ini
├── webapp/                     # Aplicación web (React)
├── docs/
│   ├── PROJECT_PLAN_v1.1.md    # Plan del proyecto
│   ├── HARDWARE_ELECTRONICS.md # Diseño electrónico
│   ├── README_MATHEMATICAL_BASIS.md
│   └── README_COMPUTATIONAL_BASIS.md
└── tools/                      # Scripts de validación
    ├── signal_validator.py
    └── clinical_ranges.py
```

## Hardware

| Componente | Modelo | Función |
|------------|--------|---------|
| MCU Cerebro | ESP32-WROOM-32 | Generación de señales |
| Display HMI | ELECROW ESP32-S3 7" | Interfaz táctil |
| Baterías | 2× 18650 2200mAh (paralelo) | 3.7V, 4400mAh |
| Cargador | TP4056 con protección | Carga USB |
| Regulador | MT3608 Boost | 3.7V → 5V |
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

### Firmware Cerebro
```bash
cd firmware/cerebro
pio run
pio run --target upload
```

### Firmware HMI
```bash
cd firmware/hmi
pio run
pio run --target upload
```

## Documentación

- [Plan del Proyecto](docs/PROJECT_PLAN_v1.1.md)
- [Diseño Electrónico](docs/HARDWARE_ELECTRONICS.md)
- [Base Matemática](docs/README_MATHEMATICAL_BASIS.md)
- [Base Computacional](docs/README_COMPUTATIONAL_BASIS.md)

## Changelog

### v1.1.0 (Diciembre 2024)
- Migración de Nextion a ELECROW HMI 7"
- Reestructuración del proyecto (firmware/cerebro, firmware/hmi, webapp)
- Actualización del sistema de alimentación (2P paralelo, TP4056, MT3608)
- Documentación de hardware electrónico completa

### v1.0.0
- Versión inicial con pantalla Nextion
- Modelos ECG, EMG, PPG completos
- Validación clínica de rangos

## Licencia

MIT License - Ver archivo LICENSE para detalles.
