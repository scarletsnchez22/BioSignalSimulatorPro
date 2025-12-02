# BioSignal Simulator Pro

**Simulador de Señales Fisiológicas de Alta Fidelidad para ESP32 + Nextion**

[![Platform](https://img.shields.io/badge/Platform-ESP32--WROOM--32-blue.svg)]()
[![Display](https://img.shields.io/badge/Display-Nextion%20NX4024T032-green.svg)]()
[![Framework](https://img.shields.io/badge/Framework-Arduino%20%2B%20FreeRTOS-orange.svg)]()
[![Version](https://img.shields.io/badge/Version-2.0.0-red.svg)]()

---

## 📋 Descripción

Generador de señales biológicas (ECG, EMG, PPG) basado en modelos matemáticos validados científicamente. Interfaz táctil Nextion para control completo. Diseñado para educación médica y desarrollo de algoritmos de procesamiento de señales.

### Características Principales

| Característica | Detalle |
|----------------|---------|
| **Señales** | ECG, EMG, PPG |
| **Condiciones** | 26 (9 ECG + 10 EMG + 7 PPG) |
| **Frecuencia** | 1000 Hz (1 kHz) |
| **Display** | Nextion NX4024T032 (320×240) Touch |
| **Salida DAC** | GPIO25, 8-bit, 0-3.3V |
| **Arquitectura** | Dual-Core FreeRTOS optimizado |

### Puntos Fuertes

- ✅ **Modelos científicos validados** (McSharry, Fuglevand, Allen)
- ✅ **Señales nunca repetitivas** (HRV, variabilidad fisiológica)
- ✅ **UI táctil completa** con waveform y métricas en tiempo real
- ✅ **Parametrización limitada** por condición clínica
- ✅ **Control dual**: Pantalla táctil + Serial USB

---

## 🏗️ Arquitectura del Sistema

```
┌─────────────────────────────────────────────────────────────────────┐
│                          ESP32-WROOM-32                             │
│                                                                     │
│  ┌─────────────────┐              ┌─────────────────┐              │
│  │     CORE 0      │              │     CORE 1      │              │
│  │                 │              │                 │              │
│  │  ┌───────────┐  │              │  ┌───────────┐  │              │
│  │  │   loop()  │  │              │  │PrecalcTask│  │              │
│  │  │  (UI/CMD) │  │              │  │ (Señales) │  │              │
│  │  └───────────┘  │              │  └───────────┘  │              │
│  │                 │              │                 │              │
│  │  ┌───────────┐  │   Buffer     │  ┌───────────┐  │              │
│  │  │MonitorTask│  │◄────────────►│  │ Timer ISR │  │              │
│  │  └───────────┘  │   Circular   │  │   (DAC)   │  │              │
│  └─────────────────┘              │  └───────────┘  │              │
│                                   └─────────────────┘              │
│                                           │                        │
│                                           ▼                        │
│                                    ┌──────────────┐                │
│                                    │   GPIO25     │                │
│                                    │  (DAC OUT)   │                │
│                                    └──────────────┘                │
└─────────────────────────────────────────────────────────────────────┘
```

### Distribución de Tareas FreeRTOS

| Tarea | Core | Prioridad | Stack | Función |
|-------|------|-----------|-------|---------|
| `PrecalcTask` | 1 | 4 | 4KB | Genera muestras, llena buffer |
| `MonitorTask` | 0 | 1 | 2KB | Estadísticas de rendimiento |
| `loop()` | 0 | - | - | UI y comandos serial |
| `Timer ISR` | 1 | - | - | Lee buffer → DAC @ 500 Hz |

---

## 📊 Modelos de Señales

### ECG - Electrocardiograma

**Modelo**: McSharry et al. (2003) - Ecuaciones diferenciales dinámicas

```
dx/dt = α·x - ω·y
dy/dt = α·y + ω·x  
dz/dt = -Σᵢ aᵢ·Δθᵢ·exp(-Δθᵢ²/2bᵢ²) - (z - z₀)
```

**Referencias**:
- McSharry, P.E. et al. (2003). "A dynamical model for generating synthetic electrocardiogram signals." IEEE Trans Biomed Eng, 50(3), 289-294.
- Goldberger, A.L. et al. (2000). PhysioBank, PhysioToolkit - MIT-BIH Database.

**Condiciones disponibles** (9):
| # | Condición | HR (BPM) | Características |
|---|-----------|----------|-----------------|
| 1 | Normal | 60-100 | Ritmo sinusal regular |
| 2 | Taquicardia | >100 | HR elevada |
| 3 | Bradicardia | <60 | HR reducida |
| 4 | Fibrilación Auricular | 100-160 | Irregular, sin onda P |
| 5 | Fibrilación Ventricular | - | Caótico |
| 6 | PVC | Variable | Latidos prematuros |
| 7 | Bloqueo de Rama | 60-100 | QRS ancho (>120ms) |
| 8 | Elevación ST | 60-100 | STEMI (infarto) |
| 9 | Depresión ST | 60-100 | Isquemia |

---

### EMG - Electromiograma

**Modelo**: Fuglevand et al. (1993) + NeuroMotion (Ma et al. 2024)

- 100 Motor Units con reclutamiento ordenado (Size Principle - Henneman 1957)
- ISI con variabilidad gaussiana (CoV = 20%)
- MUAP bi-exponencial (6-14 ms duración)

**Referencias**:
- Fuglevand, A.J. et al. (1993). "Models of recruitment and rate coding organization in motor-unit pools." J Neurophysiol, 70(6), 2470-2488.
- De Luca, C.J. & Hostage, E.C. (2010). "Relationship between firing rate and recruitment threshold." J Neurophysiol, 104(2), 1034-1046.
- Ma, S. et al. (2024). "NeuroMotion: Open-source simulator for motor unit activity." PLOS Comp Biol, 20(7).

**Condiciones disponibles** (10):
| # | Condición | % MVC | Características |
|---|-----------|-------|-----------------|
| 1 | Reposo | 0% | Solo ruido de fondo |
| 2 | Contracción Leve | 20% | Tareas de precisión |
| 3 | Contracción Moderada | 50% | Sostener objetos |
| 4 | Contracción Fuerte | 80% | Levantar peso |
| 5 | Contracción Máxima | 100% | Esfuerzo máximo |
| 6 | Temblor | - | Oscilación 4-6 Hz (Parkinson) |
| 7 | Miopatía | - | MUAPs pequeños/cortos |
| 8 | Neuropatía | - | MUAPs gigantes |
| 9 | Fasciculación | - | Disparos espontáneos |
| 0 | Fatiga | - | Decaimiento progresivo |

---

### PPG - Fotopletismograma

**Modelo**: Doble Gaussiana (Elgendi et al. 2019)

```
PPG(t) = DC + A₁·exp(-(t-μ₁)²/2σ₁²) + A₂·exp(-(t-μ₂)²/2σ₂²) - D·exp(-(t-μd)²/2σd²)
              └──────────────────┘   └──────────────────┘   └──────────────────┘
                Pico Sistólico        Pico Diastólico        Muesca Dicrótica
```

**Referencias**:
- Elgendi, M. et al. (2019). "Optimal Signal Quality Index for PPG." IEEE Rev Biomed Eng, 12, 27-47.
- Allen, J. (2007). "Photoplethysmography and its application." Physiol Meas, 28, R1-R39.
- Task Force ESC/NASPE (1996). "Heart Rate Variability Standards." Circulation, 93, 1043-1065.

**Condiciones disponibles** (7):
| # | Condición | HR | PI (%) | SpO2 (%) |
|---|-----------|-----|--------|----------|
| 1 | Normal | 75 | 5.0 | 97 |
| 2 | Arritmia | 75 | 4.0 | 97 |
| 3 | Perfusión Débil | 110 | 0.8 | 94 |
| 4 | Perfusión Fuerte | 75 | 12.0 | 97 |
| 5 | Vasoconstricción | 80 | 4.0 | 97 |
| 6 | Artefacto Movimiento | 75 | 5.0 | 97 |
| 7 | SpO2 Bajo | 90 | 2.5 | 88 |

---

## 🔧 Hardware Requerido

| Componente | Especificación |
|------------|----------------|
| **MCU** | ESP32-WROOM-32 (sin PSRAM) |
| **Salida señal** | GPIO25 (DAC1) |
| **Display** | Nextion NX4024T032 (opcional) |
| **Conexión Nextion** | TX2=GPIO17, RX2=GPIO16 @ 9600 baud |

### Conexiones

```
ESP32-WROOM-32          Osciloscopio/ADC
    GPIO25 ─────────────► CH1 (Señal)
    GND    ─────────────► GND

ESP32-WROOM-32          Nextion NX4024T032
    GPIO17 (TX2) ───────► RX
    GPIO16 (RX2) ───────► TX
    5V     ─────────────► VCC
    GND    ─────────────► GND
```

---

## 📦 Instalación

### Requisitos

- PlatformIO IDE (VS Code)
- ESP32 Arduino Core 2.x

### Pasos

1. Clonar repositorio:
```bash
git clone https://github.com/tu-usuario/BioSignalSimulator-Pro.git
cd BioSignalSimulator-Pro
```

2. Abrir en PlatformIO

3. Compilar y subir:
```bash
pio run -t upload
```

4. Abrir monitor serial:
```bash
pio device monitor -b 115200
```

---

## 🎮 Uso

### Comandos Serial (115200 baud)

| Comando | Acción |
|---------|--------|
| `e` | Modo ECG |
| `m` | Modo EMG |
| `g` | Modo PPG |
| `1-9, 0` | Seleccionar condición |
| `p` | Pausar señal |
| `r` | Reanudar señal |
| `s` | Detener señal |
| `b<val>` | Cambiar BPM (ej: `b85`) |
| `a<val>` | Cambiar amplitud (ej: `a1.5`) |
| `i` | Información del sistema |
| `h` | Ayuda |

### Ejemplo de Uso

```
e         → Activa modo ECG
2         → Inicia Taquicardia
b130      → Cambia HR a 130 BPM
p         → Pausa
r         → Reanuda
s         → Detiene
```

---

## 📁 Estructura del Proyecto

```
BioSignalSimulator Pro/
├── include/
│   ├── config.h           # Configuración global y constantes
│   ├── signal_types.h     # Enums y estructuras de datos
│   ├── signal_generator.h # Gestor principal de señales
│   ├── ecg_model.h        # Modelo ECG McSharry
│   ├── emg_model.h        # Modelo EMG Fuglevand
│   ├── ppg_model.h        # Modelo PPG Doble Gaussiana
│   └── nextion_ui.h       # Interfaz Nextion
├── src/
│   ├── main.cpp           # Punto de entrada
│   ├── signal_generator.cpp
│   ├── ecg_model.cpp
│   ├── emg_model.cpp
│   ├── ppg_model.cpp
│   └── nextion_ui.cpp
├── platformio.ini
└── README.md
```

---

## 📈 Especificaciones Técnicas

| Parámetro | Valor |
|-----------|-------|
| Frecuencia muestreo | 500 Hz (unificada) |
| Resolución DAC | 8 bits (0-255) |
| Rango voltaje salida | 0 - 3.3V |
| Buffer circular | 2048 muestras (4.1 s) |
| Latencia máxima | < 5 µs (ISR) |
| Memoria RAM usada | ~25 KB |
| CPU utilización | Core 0: 10%, Core 1: 30% |

---

## 📚 Referencias Científicas Completas

### ECG
1. McSharry, P.E., Clifford, G.D., Tarassenko, L., & Smith, L.A. (2003). A dynamical model for generating synthetic electrocardiogram signals. *IEEE Transactions on Biomedical Engineering*, 50(3), 289-294.

### EMG
2. Fuglevand, A.J., Winter, D.A., & Patla, A.E. (1993). Models of recruitment and rate coding organization in motor-unit pools. *Journal of Neurophysiology*, 70(6), 2470-2488.
3. De Luca, C.J., & Hostage, E.C. (2010). Relationship between firing rate and recruitment threshold of motoneurons. *Journal of Neurophysiology*, 104(2), 1034-1046.
4. Henneman, E. (1957). Relation between size of neurons and their susceptibility to discharge. *Science*, 126(3287), 1345-1347.

### PPG
5. Elgendi, M., et al. (2019). Optimal Signal Quality Index for Photoplethysmogram Signals. *IEEE Reviews in Biomedical Engineering*, 12, 27-47.
6. Allen, J. (2007). Photoplethysmography and its application in clinical physiological measurement. *Physiological Measurement*, 28, R1-R39.

### HRV
7. Task Force of ESC/NASPE (1996). Heart Rate Variability: Standards of Measurement. *Circulation*, 93, 1043-1065.

---

## 📄 Licencia

MIT License - Ver archivo LICENSE

---

## 👥 Contribuciones

Las contribuciones son bienvenidas. Por favor, abra un issue primero para discutir cambios mayores.

---

**Desarrollado para aplicaciones educativas y de investigación en ingeniería biomédica.**
