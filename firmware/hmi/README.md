# BioSignalSimulator Pro - Firmware HMI

**Firmware para la pantalla táctil ELECROW ESP32-S3 7"**

---

| Campo | Valor |
|-------|-------|
| **Versión** | 1.0.0 |
| **Estado** | 🚧 Placeholder - En desarrollo |
| **Hardware** | ELECROW ESP32-S3 7" 800x480 |
| **Framework** | Arduino + LVGL 8.x |

---

## Descripción

Este firmware controla la interfaz gráfica del BioSignalSimulator Pro, proporcionando:

- Visualización en tiempo real de señales ECG/EMG/PPG
- Control de parámetros mediante sliders táctiles
- Selección de tipo de señal y condición clínica
- Indicadores de estado y métricas

## Hardware

| Componente | Especificación |
|------------|----------------|
| **MCU** | ESP32-S3-WROOM-1-N4R8 |
| **Display** | 7" TFT LCD 800×480 RGB |
| **Touch** | Capacitivo GT911 (I2C) |
| **RAM** | 512KB SRAM + 8MB PSRAM |
| **Flash** | 4MB |

## Comunicación con Cerebro

| Parámetro | Valor |
|-----------|-------|
| **Interfaz** | UART0 |
| **Velocidad** | 921600 baud |
| **Formato** | 8N1 |

### Protocolo de Datos (Cerebro → HMI)

```
>ecg:VALUE,hr:VALUE,rr:VALUE,st:VALUE,beats:VALUE
>emg:VALUE,rms:VALUE,mvc:VALUE,freq:VALUE,units:VALUE
>ppg:VALUE,hr:VALUE,rr:VALUE,pi:VALUE,spo2:VALUE,beats:VALUE
```

### Protocolo de Comandos (HMI → Cerebro)

```
CMD:SIGNAL:ECG
CMD:SIGNAL:EMG
CMD:SIGNAL:PPG
CMD:CONDITION:NORMAL
CMD:PARAM:HR:75
CMD:START
CMD:STOP
```

## Estructura de Archivos

```
firmware/hmi/
├── platformio.ini      # Configuración PlatformIO
├── README.md           # Este archivo
├── src/
│   └── main.cpp        # Punto de entrada (placeholder)
├── include/
│   └── (headers)
├── lib/
│   └── (librerías locales)
└── ui/
    └── (archivos generados por SquareLine Studio)
```

## Dependencias

- **LVGL** v8.3.x - Librería gráfica
- **LovyanGFX** v1.1.x - Driver de display
- **TFT_eSPI** (alternativa) - Driver de display

## TODO

- [ ] Configurar driver de display para ELECROW
- [ ] Implementar inicialización LVGL
- [ ] Diseñar UI en SquareLine Studio
- [ ] Implementar recepción de datos UART
- [ ] Implementar gráfico de señal en tiempo real
- [ ] Implementar controles de parámetros
- [ ] Implementar envío de comandos al Cerebro

## Compilación

```bash
cd firmware/hmi
pio run
pio run --target upload
```

## Notas

Este firmware está actualmente en estado **placeholder**. La implementación completa requiere:

1. Configurar el driver de display específico para ELECROW
2. Diseñar la interfaz en SquareLine Studio
3. Integrar los archivos UI generados
4. Implementar la comunicación UART bidireccional
