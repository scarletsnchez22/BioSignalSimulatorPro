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

**Última actualización:** Diciembre 2025  
**Proyecto:** BioSimulator Pro v3.0.0  
**Hardware:** ESP32-WROOM-32
