# LIMITACIONES Y ADVERTENCIAS DEL SISTEMA - BioSignalSimulator Pro

## 📋 Tabla de Limitaciones Técnicas

| Categoría | Limitación | Descripción | Impacto | Solución Propuesta |
|-----------|------------|-------------|---------|-------------------|
| **Hardware - DAC** | Resolución 8 bits | DAC interno ESP32 solo 8 bits (256 niveles) vs. DACs clínicos 12-16 bits | Cuantización visible en señales de baja amplitud | Usar DAC externo (MCP4725 12-bit) en versiones futuras |
| **Hardware - DAC** | Rango 0-3.3V unipolar | Señal unipolar, señales bipolares requieren offset | EMG bipolar se mapea con offset artificial | Implementar amplificador diferencial con ±5V |
| **Hardware - Filtro RC** | Componentes discretos | Resistencias y capacitores tienen tolerancia ±5-10% | Frecuencia de corte real puede variar ±10% | Usar componentes de precisión (±1%) |
| **Hardware - Buffer** | LM358 no rail-to-rail | Salida máxima ~VCC-1.5V (3.5V con 5V alimentación) | Pérdida de rango dinámico en extremos | Reemplazar con MCP6002 rail-to-rail |
| **Señal ECG** | Sin variabilidad realista | HRV simplificada, no incluye variabilidad respiratoria completa | Validación limitada para estudios HRV avanzados | Implementar modelo Berger con modulación respiratoria |
| **Señal ECG** | Patologías simplificadas | FA/FV son aproximaciones, no caóticas reales | No válido para algoritmos de detección avanzados | Incorporar modelos no lineales (Lorenz, chaos) |
| **Señal ECG** | Sin artefactos realistas | No simula movimiento, contacto pobre, ruido de línea | Demasiado limpio vs. ECG real | Agregar modelos de artefactos (EMI, movimiento) |
| **Señal EMG** | Modelo Fuglevand limitado | 100 MUs fijas, no incluye adaptación por fatiga real | Fatiga es aproximación, no fisiológicamente exacta | Implementar modelo De Luca con adaptación dinámica |
| **Señal EMG** | Sin conducción neural | No simula NCV (velocidad conducción neural) | No válido para estudios de neuropatías | Agregar modelo de retardo y dispersión temporal |
| **Señal PPG** | Modelo gaussiano simple | Suma de gaussianas, no modelo hemodinámico completo | No captura efectos complejos (vasoconstricción dinámica) | Implementar modelo Windkessel de 3 elementos |
| **Señal PPG** | Sin saturación de oxígeno | No simula SpO2 variable | No apto para validar oxímetros | Requerir modelo dual-wavelength (rojo/infrarrojo) |
| **Frecuencia** | Fs_timer fijo 2 kHz | Timer ISR a 2 kHz fijo, no ajustable en runtime | No permite pruebas de diferentes Fs sin recompilar | Implementar cambio dinámico de frecuencia |
| **Frecuencia** | Interpolación lineal | Upsampling con interpolación lineal simple | Introduce artefactos de alta frecuencia (aliasing) | Usar filtro antialiasing FIR de reconstrucción |
| **Nextion** | Resolución 800×480 | Pantalla de 7" con 800×480 px limita resolución temporal | Solo 700 px útiles para forma de onda | Usar pantalla de mayor resolución (1024×600) |
| **Nextion** | Sin almacenamiento local | No graba trazos en microSD | Datos se pierden al apagar | Implementar logging a tarjeta SD ESP32 |
| **Nextion** | Comunicación UART simple | Sin checksum ni ACK confiable | Posible pérdida de datos en ruido EMI | Implementar protocolo con CRC y retransmisión |
| **App Web** | WebSocket single-client | Un cliente a la vez, no multiusuario | Solo un navegador puede visualizar simultáneamente | Implementar broadcast WebSocket (múltiples clientes) |
| **App Web** | Sin autenticación | Red WiFi sin contraseña, acceso abierto | Cualquiera en rango puede conectar | Agregar WPA2 con contraseña configurable |
| **App Web** | Buffer limitado | Solo 800 puntos en buffer JS | No permite retroceder en el tiempo | Implementar buffer circular con histórico (10000 pts) |
| **Batería** | Capacidad 5200 mAh | Autonomía ~8h uso continuo | Requiere recarga frecuente en jornadas largas | Usar pack 3P (7800 mAh) o batería externa USB-C PD |
| **Batería** | Sin indicador preciso** | Solo LED de carga, no % batería | Usuario no sabe tiempo restante | Implementar medición con INA219 y display % |
| **Calibración** | Sin calibración automática | Escalas fijas, no ajuste automático | Amplitudes pueden salir de rango en condiciones extremas | Implementar AGC (Automatic Gain Control) |
| **Validación** | Sin certificación médica | Dispositivo educativo, NO apto uso clínico | No puede usarse en humanos reales | Proceso de certificación (FDA, CE) fuera de alcance |
| **Procesamiento** | Sin FFT en tiempo real | FFT solo por scripts Python offline | No muestra espectro en pantalla | Implementar FFT embebida con librería kissfft |
| **Almacenamiento** | SPIFFS 1 MB limitado | Solo archivos web pequeños | No permite logging extenso | Usar partición mayor o tarjeta SD |
| **Conectividad** | WiFi solo 2.4 GHz | ESP32 no soporta 5 GHz | Interferencia en entornos congestionados | Usar ESP32-C6 con WiFi 6 (dual-band) |
| **Temperatura** | Sin gestión térmica | No monitorea temperatura interna | Posible throttling en uso prolongado | Agregar sensor NTC y ventilación activa |

---

## ⚠️ Advertencias de Uso

### 🚨 ADVERTENCIAS CRÍTICAS

| # | Advertencia | Descripción | Mitigación |
|---|-------------|-------------|------------|
| 1 | **NO ES DISPOSITIVO MÉDICO** | No certificado para diagnóstico ni tratamiento clínico. Solo educativo. | Etiquetar claramente "SOLO USO EDUCATIVO" |
| 2 | **NO USAR EN HUMANOS** | Señales analógicas NO están aisladas galvánicamente. | No conectar a electrodos reales en pacientes |
| 3 | **BATERÍA Li-ion** | Riesgo de incendio si se cortocircuita o daña. | No perforar, no exponer a >60°C, usar protección IP5306 |
| 4 | **INTERFERENCIA EMI** | Campos electromagnéticos pueden alterar señales. | Alejar de motores, transformadores, microondas |

### ⚠️ ADVERTENCIAS OPERACIONALES

| # | Advertencia | Descripción | Recomendación |
|---|-------------|-------------|---------------|
| 5 | **Red WiFi abierta** | Red sin contraseña, accesible a cualquiera en rango | Usar solo en entorno controlado (laboratorio) |
| 6 | **Sobrecarga térmica** | Uso continuo >4h puede calentar batería y ESP32 | Permitir ventilación, apagar 15 min cada 4h |
| 7 | **Pérdida de datos** | Datos no se guardan automáticamente | Capturar con scripts Python si se requiere almacenamiento |
| 8 | **Versión firmware** | Actualizar firmware borra configuración | Documentar parámetros antes de actualizar |
| 9 | **Compatibilidad USB** | Algunos cables USB solo cargan, no transfieren datos | Usar cable USB con líneas D+/D- (datos) |
| 10 | **Serial Plotter** | Requiere baudrate exacto 115200 | Configurar correctamente en terminal |

---

## 📍 Dónde Incluir en la Tesis

### Sección Recomendada: **3.X LIMITACIONES DEL DISEÑO**

Ubicar ANTES del Capítulo de Resultados, al final de Metodología. Estructura sugerida:

```
3. METODOLOGÍA
  3.1 Diseño del Sistema
  3.2 Generación de Señales
  3.3 Hardware Implementado
  3.4 Software y Algoritmos
  3.5 Protocolo de Validación
  👉 3.6 LIMITACIONES Y ADVERTENCIAS DEL DISEÑO  <-- AQUÍ
  
4. RESULTADOS
  4.1 Validación de Señales
  ...
```

### Texto Sugerido para Tesis:

```markdown
## 3.6 Limitaciones y Advertencias del Diseño

El diseño implementado presenta limitaciones técnicas inherentes al alcance
educativo del proyecto y las restricciones presupuestarias. A continuación se
detallan las principales limitaciones identificadas y su impacto en el desempeño
del sistema.

### 3.6.1 Limitaciones de Hardware

**Conversión Digital-Analógica:** El DAC interno del ESP32 posee una resolución
de 8 bits (256 niveles), significativamente inferior a los DACs clínicos estándar
de 12-16 bits. Esto limita la resolución vertical de las señales analógicas y
puede introducir artefactos de cuantización en señales de baja amplitud.

**Rango Dinámico:** La salida del DAC es unipolar (0-3.3V), mientras que señales
como EMG son bipolares en aplicaciones clínicas. Se implementó un offset artificial,
pero esto reduce el rango dinámico efectivo disponible.

[... continuar con otras limitaciones de la tabla ...]

### 3.6.2 Limitaciones de los Modelos de Señal

**Simplificación Fisiológica:** Los modelos matemáticos implementados (McSharry,
Fuglevand, Allen) son aproximaciones simplificadas de procesos fisiológicos complejos.
No capturan completamente la variabilidad inter-individual ni efectos dinámicos
como adaptación metabólica o regulación autonómica.

[... continuar ...]

### 3.6.3 Advertencias de Uso

⚠️ **ADVERTENCIA CRÍTICA:** Este dispositivo NO está certificado como equipo médico
y NO debe utilizarse para diagnóstico, tratamiento ni en contacto con pacientes reales.
Su propósito es exclusivamente educativo para la formación de estudiantes de ingeniería
biomédica.

[... incluir tabla de advertencias operacionales ...]

### 3.6.4 Impacto en la Validación

Las limitaciones descritas definen el alcance de la validación realizada. Los
resultados presentados en el Capítulo 4 deben interpretarse considerando estas
restricciones, particularmente en cuanto a resolución temporal/vertical y fidelidad
morfológica respecto a señales clínicas reales.
```

---

## 🔬 Sobre el Análisis Morfológico

### ¿Se puede automatizar completamente?

**Respuesta corta:** **SÍ, parcialmente**, pero requiere:

1. **Base de datos de referencia (PhysioNet)** - Script creado: `morphology_validator.py`
2. **Métricas cuantitativas** - Correlación, RMSE, DTW (Dynamic Time Warping)
3. **Validación por experto** - NO automatizable, requiere cardiólogo/fisiólogo

### Proceso Híbrido Recomendado:

#### FASE 1: Automática (Scripts Python)
- ✅ Descargar señales de referencia de MIT-BIH (ECG)
- ✅ Extraer latidos individuales
- ✅ Calcular correlación de Pearson (morfología similar si r > 0.85)
- ✅ Calcular RMSE normalizado (error morfológico)
- ✅ Detectar componentes (P, Q, R, S, T)
- ✅ Generar gráficos de comparación

#### FASE 2: Visual (Tú)
- 📸 Capturas de pantalla de Serial Plotter vs. PhysioNet
- 📊 Incluir gráficos superpuestos en tesis
- ✍️ Descripción cualitativa: "La morfología del complejo QRS..."

#### FASE 3: Validación Experta (Opcional pero IDEAL)
- 👨‍⚕️ Mostrar señales a cardiólogo o profesor de fisiología
- 📝 Obtener declaración firmada: "Las señales son representativas de..."
- 🎯 Incluir en Anexos como "Validación por Experto Clínico"

### Comandos para Ejecutar Validación Morfológica:

```powershell
# 1. Capturar señal con nuevo entorno esp32_analysis
pio run -e esp32_analysis --target upload

# 2. Guardar datos a archivo (desde serial)
python tools/temporal_parameters_analyzer.py --port COM4 --signal ECG --duration 30 --output results/morphology

# 3. Convertir CSV a NPY
python -c "import pandas as pd; import numpy as np; df = pd.read_csv('results/morphology/ecg_data.csv'); np.save('results/morphology/ecg_signal.npy', df['value_mV'].values)"

# 4. Validar morfología (descarga referencia PhysioNet automáticamente)
python tools/morphology_validator.py --signal results/morphology/ecg_signal.npy --type ECG --condition Normal --fs 300 --download-ref --output results/morphology
```

**Salida esperada:**
- Correlación > 0.80 → "Morfología EXCELENTE"
- Gráficos comparativos simulado vs. MIT-BIH
- Reporte con características detectadas (ondas P, Q, R, S, T)

---

## 📁 Archivos Creados para Ti:

1. **`main_analysis.cpp`** - Firmware sin Nextion, solo captura datos CSV
2. **`morphology_validator.py`** - Validación morfológica automática vs. PhysioNet
3. **Environment `esp32_analysis`** - Nuevo entorno en platformio.ini
4. **Esta documentación** - Limitaciones y advertencias completas

¿Necesitas ayuda con alguna parte específica?
