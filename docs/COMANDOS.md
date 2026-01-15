# 🚀 Comandos PlatformIO - BioSimulator Pro

## 📋 Comandos Básicos de Compilación y Subida

### 🔧 Compilar y Subir (Upload)

```powershell
# Usar environment por defecto (esp32_wroom32 - main.cpp con Nextion)
pio run --target upload

# Compilar sin subir (verificar errores)
pio run

# Limpiar compilación anterior
pio run --target clean

# Limpiar TODO (incluyendo librerías descargadas)
pio run --target fullclean
```

---

## 🎯 Comandos por Environment (Main File)

### 📱 Main.cpp - Nextion Display (PRODUCCIÓN)

```powershell
# Compilar y subir
pio run -e esp32_wroom32 --target upload

# Solo compilar
pio run -e esp32_wroom32

# Ver uso de memoria
pio run -e esp32_wroom32 --target size
```

**Archivo ejecutado:** `src/main.cpp`  
**Uso:** Pantalla Nextion + DAC analógico

---

### 🔬 Main_debug.cpp - Serial Plotter (DESARROLLO)

```powershell
# Compilar y subir
pio run -e esp32_debug --target upload

# Solo compilar
pio run -e esp32_debug

# Compilar, subir Y abrir Serial Monitor
pio run -e esp32_debug --target upload ; pio device monitor
```
platformio run -e esp32_debug --target upload

**Archivo ejecutado:** `src/main_debug.cpp`  
**Uso:** Serial Plotter para validación de señales

---

### 🚀 Release - Optimizado (PRODUCCIÓN FINAL)

```powershell
# Compilar y subir versión optimizada
pio run -e esp32_release --target upload

# Ver tamaño del firmware optimizado
pio run -e esp32_release --target size
```

**Archivo ejecutado:** `src/main.cpp`  
**Optimización:** -O3 con optimizaciones avanzadas

---

### 🛠️ Development - Verbose (DEPURACIÓN AVANZADA)

```powershell
# Compilar y subir con debug nivel 5
pio run -e esp32_dev --target upload

# Ver logs detallados durante ejecución
pio run -e esp32_dev --target upload ; pio device monitor
```

**Archivo ejecutado:** `src/main.cpp`  
**Debug Level:** 5 (máximo verbose)

---

## 📡 Serial Monitor

### Abrir Serial Monitor

```powershell
# Monitor básico
pio device monitor

# Monitor con puerto específico
pio device monitor -p COM4

# Monitor con baudrate específico
pio device monitor -b 115200

# Monitor con filtros (decodificador de excepciones ESP32)
pio device monitor --filter esp32_exception_decoder

# Monitor con todo (recomendado para debug)
pio device monitor -p COM4 -b 115200 --filter esp32_exception_decoder --filter colorize
```

### Salir del Serial Monitor

Presiona: `Ctrl+C`

---

## 🔄 Cambiar Environment por Defecto

### Editar platformio.ini

```ini
[platformio]
default_envs = esp32_wroom32     # Para Nextion (main.cpp)
# default_envs = esp32_debug     # Para Serial Plotter (main_debug.cpp)
# default_envs = esp32_release   # Para producción optimizada
```

O usar PowerShell:

```powershell
# Cambiar a main.cpp (Nextion)
(Get-Content platformio.ini) -replace 'default_envs = .*', 'default_envs = esp32_wroom32' | Set-Content platformio.ini

# Cambiar a main_debug.cpp (Serial Plotter)
(Get-Content platformio.ini) -replace 'default_envs = .*', 'default_envs = esp32_debug' | Set-Content platformio.ini
```

---

## 🔍 Comandos de Información

### Ver información del proyecto

```powershell
# Listar todos los environments disponibles
pio project config

# Ver configuración de un environment específico
pio project config -d esp32_debug

# Listar puertos seriales disponibles
pio device list

# Ver tamaño de firmware compilado
pio run --target size

# Inspeccionar uso de memoria (abre web)
pio run --target inspect
```

---

## 🐛 Solución de Problemas

### Error: "Failed to connect to ESP32"

```powershell
# Opción 1: Presionar botón BOOT manualmente cuando veas "Connecting..."
pio run --target upload
# (Presiona y mantén BOOT hasta que empiece a subir)

# Opción 2: Reducir velocidad de upload
# Editar platformio.ini: upload_speed = 115200
```

### Error: "No module named platformio"

```powershell
# Reinstalar PlatformIO Core
pip install -U platformio

# O usar el instalador completo
# https://platformio.org/install/cli
```

### Puerto COM ocupado

```powershell
# Ver qué proceso usa el puerto
netstat -ano | findstr :COM4

# Cerrar todos los monitores seriales
taskkill /F /IM python.exe
```

---

## 📊 Comandos Combinados Útiles

### Compilar, Subir y Monitorear (Todo en uno)

```powershell
# Para main.cpp (Nextion) - sin monitor
pio run -e esp32_wroom32 --target upload

# Para main_debug.cpp (Serial Plotter) - con monitor
pio run -e esp32_debug --target upload ; pio device monitor -b 115200
```

### Limpiar, Compilar y Subir

```powershell
# Limpieza completa y compilación fresca
pio run --target clean ; pio run --target upload
```

### Comparar tamaños de firmware

```powershell
# Ver tamaño de debug vs release
Write-Host "=== DEBUG ===" ; pio run -e esp32_debug --target size
Write-Host "`n=== RELEASE ===" ; pio run -e esp32_release --target size
```

---

## 📝 Compilación con Opciones Avanzadas

### Compilación verbose (ver todos los detalles)

```powershell
# Ver comandos de compilación completos
pio run -v --target upload

# Solo ver warnings y errores
pio run --target upload 2>&1 | Select-String "warning|error"
```

### Compilar archivos específicos

```powershell
# Forzar recompilación de todo
pio run --target clean ; pio run

# Compilar solo modelos (útil después de editar ecg_model.cpp)
# (PlatformIO detecta cambios automáticamente)
pio run
```

---

## 🎨 Comandos para Desarrollo

### Generar compile_commands.json (para IntelliSense)

```powershell
# Generar base de datos de compilación
pio run --target compiledb
```

### Actualizar librerías

```powershell
# Actualizar todas las librerías
pio lib update

# Listar librerías instaladas
pio lib list

# Buscar una librería
pio lib search "nextion"
```

### Actualizar plataforma ESP32

```powershell
# Actualizar Espressif 32 platform
pio platform update espressif32

# Ver versión actual
pio platform show espressif32
```

---

## 🔧 Configuración del Proyecto

### Cambiar puerto COM

```powershell
# Editar platformio.ini
# upload_port = COM4
# monitor_port = COM4

# O usar variable de entorno
$env:PLATFORMIO_UPLOAD_PORT = "COM4"
pio run --target upload
```

### Cambiar baudrate

```powershell
# Editar platformio.ini
# upload_speed = 115200
# monitor_speed = 115200
```

---

## 📦 Tabla Resumen de Environments

| Environment | Main File | Debug Level | Optimización | Uso |
|-------------|-----------|-------------|--------------|-----|
| `esp32_wroom32` | main.cpp | 3 (Info) | -O2 | **Nextion - Producción** |
| `esp32_debug` | main_debug.cpp | 4 (Debug) | -O2 | **Serial Plotter - Validación** |
| `esp32_dev` | main.cpp | 5 (Verbose) | -O2 | Depuración avanzada |
| `esp32_release` | main.cpp | 0 (None) | -O3 | Producción final optimizada |

---

## 🚨 Comandos de Emergencia

### Reset completo del proyecto

```powershell
# Borrar TODO y recompilar desde cero
Remove-Item -Recurse -Force .pio
pio run --target upload
```

### Verificar integridad de archivos

```powershell
# Comprobar sintaxis de todos los .cpp
Get-ChildItem -Recurse -Filter *.cpp | ForEach-Object { 
    Write-Host "Verificando $($_.Name)..." 
}
pio run
```

### Backup rápido del proyecto

```powershell
# Crear ZIP del proyecto (sin .pio ni .git)
$date = Get-Date -Format "yyyyMMdd_HHmmss"
Compress-Archive -Path src,include,docs,platformio.ini,README.md -DestinationPath "backup_$date.zip"
```

---

## 📱 Comandos Específicos para BioSimulator Pro

### Compilar solo modelos de señales

```powershell
# Los archivos de modelos están en src/models/
# PlatformIO recompila automáticamente archivos modificados
pio run
```

### Verificar modelos matemáticos

```powershell
# Compilar con main_debug para verificar señales
pio run -e esp32_debug --target upload
pio device monitor -b 115200
# Abrir Serial Plotter en Arduino IDE o VS Code
```

### Ejecutar validador Python

```powershell
# Ir a carpeta de tools
cd tools

# Ejecutar validador de señales
python signal_validator.py

# Ejecutar test de todas las condiciones
python test_all_conditions.py

# Verificar rangos clínicos
python clinical_ranges.py
```

---

## 💡 Tips y Mejores Prácticas

### 1. Workflow de Desarrollo Recomendado

```powershell
# 1. Modificar código
# 2. Compilar para verificar errores
pio run -e esp32_debug

# 3. Si compila OK, subir y probar
pio run -e esp32_debug --target upload

# 4. Verificar en Serial Plotter
pio device monitor -b 115200

# 5. Cuando todo funcione, probar en Nextion
pio run -e esp32_wroom32 --target upload
```

### 2. Antes de Sustentación/Demo

```powershell
# 1. Limpiar compilación
pio run --target clean

# 2. Compilar versión release
pio run -e esp32_release --target upload

# 3. Verificar tamaño de firmware
pio run -e esp32_release --target size

# 4. Probar todas las señales en Nextion
```

### 3. Para Documentación/Tesis

```powershell
# Ver uso de memoria de cada environment
Write-Host "=== ESP32 WROOM32 (Nextion) ===" 
pio run -e esp32_wroom32 --target size

Write-Host "`n=== ESP32 DEBUG (Serial Plotter) ===" 
pio run -e esp32_debug --target size

Write-Host "`n=== ESP32 RELEASE (Optimizado) ===" 
pio run -e esp32_release --target size
```

---

## 📚 Recursos Adicionales

- **Documentación PlatformIO**: https://docs.platformio.org/
- **ESP32 Arduino Core**: https://docs.espressif.com/projects/arduino-esp32/
- **Serial Plotter**: Herramientas → Serial Plotter en Arduino IDE
- **Nextion Editor**: https://nextion.tech/nextion-editor/

---

## 🆘 Soporte

Si encuentras errores:

1. Verificar puerto COM correcto: `pio device list`
2. Limpiar compilación: `pio run --target clean`
3. Verificar conexión ESP32: Presionar BOOT durante upload
4. Revisar logs: `pio run -v --target upload`
5. Reinstalar platform: `pio platform update espressif32`

---

## 📺 Configuración Nextion HMI - Touch Release Events

### Formato del comando `printh`

```
printh 65 [page] [component_id] [event] FF FF FF
       │    │         │           │
       │    │         │           └─ 01=release, 00=press
       │    │         └─ ID del componente
       │    └─ Número de página
       └─ Código evento touch (0x65)
```

### Página `ppg_sim` (page 4) - Botones de Condición

| Botón | ID | Nombre | Touch Release Event | sel_ppg.val |
|-------|-----|--------|---------------------|-------------|
| Normal | 1 | bt_norm | `printh 65 04 01 01 FF FF FF` | 0 |
| Arritmia | 2 | bt_arr | `printh 65 04 02 01 FF FF FF` | 1 |
| Perf Débil | 3 | bt_lowp | `printh 65 04 03 01 FF FF FF` | 2 |
| Vasoconstr | 4 | bt_vascon | `printh 65 04 04 01 FF FF FF` | 3 |
| Perf Fuerte | 5 | bt_highp | `printh 65 04 05 01 FF FF FF` | 4 |
| Vasodil | 6 | bt_vasod | `printh 65 04 06 01 FF FF FF` | 5 |

### Página `ecg_sim` (page 2) - Botones de Condición

| Botón | ID | Nombre | Touch Release Event | sel_ecg.val |
|-------|-----|--------|---------------------|-------------|
| Normal | 1 | bt_norm | `printh 65 02 01 01 FF FF FF` | 0 |
| Taquicardia | 2 | bt_taq | `printh 65 02 02 01 FF FF FF` | 1 |
| Bradicardia | 3 | bt_bra | `printh 65 02 03 01 FF FF FF` | 2 |
| Bloqueo AV | 4 | bt_blk | `printh 65 02 04 01 FF FF FF` | 3 |
| Fib Auricular | 5 | bt_fa | `printh 65 02 05 01 FF FF FF` | 4 |
| Fib Ventricular | 6 | bt_fv | `printh 65 02 06 01 FF FF FF` | 5 |
| ST Elevado | 7 | bt_stup | `printh 65 02 07 01 FF FF FF` | 6 |
| ST Deprimido | 8 | bt_stdn | `printh 65 02 08 01 FF FF FF` | 7 |

### Página `emg_sim` (page 3) - Botones de Condición

| Botón | ID | Nombre | Touch Release Event | sel_emg.val |
|-------|-----|--------|---------------------|-------------|
| Reposo | 1 | bt_reposo | `printh 65 03 01 01 FF FF FF` | 0 |
| Leve | 2 | bt_leve | `printh 65 03 02 01 FF FF FF` | 1 |
| Moderada | 3 | bt_moderada | `printh 65 03 03 01 FF FF FF` | 2 |
| Máxima | 4 | bt_maxima | `printh 65 03 04 01 FF FF FF` | 3 |
| Temblor | 5 | bt_temblor | `printh 65 03 05 01 FF FF FF` | 4 |
| Fatiga | 6 | bt_fatiga | `printh 65 03 06 01 FF FF FF` | 5 |

---

## 📊 Scripts de Análisis Python (tools/)

### 🔬 Análisis FFT de Modelos Matemáticos

Analiza el contenido frecuencial intrínseco de cada modelo (ECG McSharry, EMG Fuglevand, PPG Suma Gaussianas).

```powershell
# Ruta al Python correcto (no usar el de Inkscape)
$PYTHON = "C:\Users\sgsa0\AppData\Local\Programs\Python\Python314\python.exe"

# Analizar los 3 modelos (7 segundos, por defecto)
& $PYTHON "tools/model_fft_analysis.py"

# Especificar duración
& $PYTHON "tools/model_fft_analysis.py" --duration 10

# Analizar solo un modelo
& $PYTHON "tools/model_fft_analysis.py" --signal ECG
& $PYTHON "tools/model_fft_analysis.py" --signal EMG
& $PYTHON "tools/model_fft_analysis.py" --signal PPG

# EMG con nivel de excitación específico (0-1)
& $PYTHON "tools/model_fft_analysis.py" --signal EMG --exc 0.8

# Guardar en directorio específico
& $PYTHON "tools/model_fft_analysis.py" --output "docs/fft_analysis"
```

**Salida:** Genera gráficos PNG y reporte TXT en `docs/fft_analysis/`

---

### 📡 Captura FFT desde Serial (Hardware)

Captura datos del ESP32 vía Serial y analiza con FFT.

```powershell
# Listar puertos COM disponibles
& $PYTHON "tools/fft_spectrum_analyzer.py" --list-ports

# Capturar y analizar ECG (10 segundos)
& $PYTHON "tools/fft_spectrum_analyzer.py" --port COM4 --signal ECG --duration 10

# Capturar EMG con ventana Blackman
& $PYTHON "tools/fft_spectrum_analyzer.py" --port COM4 --signal EMG --duration 7 --window blackman

# Modo simulación (sin hardware)
& $PYTHON "tools/fft_spectrum_analyzer.py" --simulate --signal PPG --duration 5
```

**Nota:** El ESP32 debe estar ejecutando señales (presionar PLAY en Nextion).

---

### 🏥 Validador de Rangos Clínicos

```powershell
# Ejecutar validador
& $PYTHON "tools/clinical_ranges.py"

# Test de todas las condiciones
& $PYTHON "tools/test_all_conditions.py"

# Validador de señales
& $PYTHON "tools/signal_validator.py"
```

---

**Revisado:** 06.01.2026  
**Proyecto:** BioSignalSimulator Pro  
**Hardware:** ESP32-WROOM-32
