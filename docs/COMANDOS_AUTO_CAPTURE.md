# Auto Capture Serial Plotter - Guía de Uso

## Descripción

Script Python que captura automáticamente datos del Serial Plotter de PlatformIO para todas las señales (ECG, EMG, PPG) y todas sus condiciones, generando imágenes PNG de alta calidad para documentación de tesis.

## Requisitos Previos

### 1. Hardware
- ESP32 conectado por USB
- main_debug.cpp compilado y subido al ESP32

### 2. Software
```bash
pip install pyserial matplotlib numpy
```

O desde el entorno virtual del proyecto:
```powershell
.\.venv\Scripts\Activate.ps1
pip install pyserial matplotlib numpy
```

## Modificación de main_debug.cpp (Opcional)

Para mejor compatibilidad, asegúrate de que `main_debug.cpp` acepta comandos seriales:

```cpp
void loop() {
  // Comandos serial para cambio de condición
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    int mode = cmd.toInt();
    
    if (mode >= 1 && mode <= 13) {
      currentMode = mode;
      Serial.print("Modo cambiado a: ");
      Serial.println(SIGNAL_NAMES[mode-1]);
    }
  }
  
  // Generar señal según modo actual
  float value = generateSignal(currentMode);
  
  // Enviar valor al Serial Plotter
  Serial.println(value);
  
  delay(10); // Ajustar según señal
}
```

## Uso del Script

### Modo Automático

```powershell
cd "C:\Users\sgsa0\Desktop\MI\BioSignalSimulator Pro"
.\.venv\Scripts\Activate.ps1
python tools/auto_capture_plotter.py
```

El script:
1. Detecta automáticamente el puerto COM del ESP32
2. Se conecta al puerto serial (115200 baud)
3. Envía comandos 1-13 para cambiar condiciones
4. Captura 900 muestras por condición
5. Descarta primeras 50 muestras (estabilización)
6. Genera gráficos PNG de alta calidad
7. Guarda resumen JSON con estadísticas

### Salida Esperada

```
results/plotter_captures/
├── ECG/
│   ├── plotter_ECG_normal_20260116_043521.png
│   ├── plotter_ECG_bradycardia_20260116_043545.png
│   ├── plotter_ECG_tachycardia_20260116_043610.png
│   ├── plotter_ECG_afib_20260116_043635.png
│   ├── plotter_ECG_stemi_20260116_043700.png
│   └── plotter_ECG_ischemia_20260116_043725.png
├── EMG/
│   ├── plotter_EMG_high_20260116_043750.png
│   ├── plotter_EMG_moderate_20260116_043815.png
│   ├── plotter_EMG_low_20260116_043840.png
│   └── plotter_EMG_fatigue_20260116_043905.png
├── PPG/
│   ├── plotter_PPG_normal_20260116_043930.png
│   ├── plotter_PPG_tachycardia_20260116_043955.png
│   └── plotter_PPG_bradycardia_20260116_044020.png
└── capture_summary_20260116_044020.json
```

## Configuración

### Editar `auto_capture_plotter.py`

```python
# Número de muestras por condición (eje X del plotter)
SAMPLES_TO_CAPTURE = 900  # Ajustar según necesidad

# Directorio de salida
OUTPUT_DIR = "results/plotter_captures"

# Baudrate (debe coincidir con Serial.begin() en main_debug.cpp)
BAUDRATE = 115200

# Comandos para cada condición (números 1-13)
SIGNAL_COMMANDS = {
    'ECG': {
        'normal': '1',
        'bradycardia': '2',
        # ...
    },
    # ...
}
```

## Comandos Serial

El script envía estos comandos al ESP32:

| Comando | Señal | Condición |
|---------|-------|-----------|
| 1 | ECG | Normal |
| 2 | ECG | Bradicardia |
| 3 | ECG | Taquicardia |
| 4 | ECG | Fibrilación Auricular |
| 5 | ECG | STEMI (Elevación ST) |
| 6 | ECG | Isquemia (Depresión ST) |
| 7 | EMG | Alta intensidad |
| 8 | EMG | Moderada intensidad |
| 9 | EMG | Baja intensidad |
| 10 | EMG | Fatiga |
| 11 | PPG | Normal |
| 12 | PPG | Taquicardia |
| 13 | PPG | Bradicardia |

## Gráficos Generados

Cada gráfico incluye:
- **Señal temporal completa** (900 muestras)
- **Eje X en tiempo (segundos)** calculado según frecuencia de muestreo
- **Estadísticas:** Media, SD, Min, Max, Rango
- **Timestamp** de captura
- **Título descriptivo** (tipo de señal + condición)
- **Alta calidad:** 1600x600 px @ 150 dpi

## Resumen JSON

El archivo `capture_summary_*.json` contiene:

```json
{
  "timestamp": "2026-01-16T04:40:20",
  "port": "COM6",
  "baudrate": 115200,
  "samples_per_signal": 900,
  "captures": [
    {
      "signal_type": "ECG",
      "condition": "normal",
      "command": "1",
      "samples": 900,
      "sampling_rate": 300,
      "mean": 0.512,
      "std": 0.234,
      "min": -0.456,
      "max": 1.234,
      "filename": "results/plotter_captures/ECG/plotter_ECG_normal_20260116_043521.png"
    },
    ...
  ]
}
```

## Solución de Problemas

### Error: Puerto COM no detectado
```powershell
# Listar puertos disponibles manualmente
python -c "import serial.tools.list_ports; [print(p.device, p.description) for p in serial.tools.list_ports.comports()]"
```

### Error: Serial Monitor/Plotter ya en uso
- Cierra el Serial Monitor/Plotter de PlatformIO antes de ejecutar el script

### Error: No se capturan datos
- Verifica que main_debug.cpp esté enviando valores numéricos con `Serial.println(value)`
- Revisa el baudrate (debe ser 115200)

### Capturas incompletas
- Aumenta `warmup_samples` en `capture_signal_data()` si la señal no se estabiliza rápido
- Ajusta `wait_time` en `send_command_and_wait()` si el ESP32 es lento al cambiar condiciones

## Uso para Tesis

Los gráficos generados son de **alta calidad** y listos para incluir directamente en tesis:

```latex
\begin{figure}[h]
  \centering
  \includegraphics[width=0.9\textwidth]{results/plotter_captures/ECG/plotter_ECG_normal_20260116_043521.png}
  \caption{Señal ECG normal capturada mediante Serial Plotter. Se observan 900 muestras con morfología PQRST característica.}
  \label{fig:ecg_normal_plotter}
\end{figure}
```

## Tiempo Estimado

- **Por condición:** ~30-45 segundos (captura + gráfico)
- **Total (13 condiciones):** ~8-12 minutos
- **Muestras totales:** 11,700 (900 × 13)

---

**Autor:** GitHub Copilot  
**Fecha:** 16 Enero 2026  
**Versión:** 1.0
