# Simulador ECG - Implementación y Arquitectura

## BioSimulator Pro v2.0.0

> **Documento Metodológico:** Este documento describe la implementación del simulador ECG desarrollado para este proyecto, incluyendo la adaptación del modelo teórico a un sistema embebido, la arquitectura computacional diseñada, y las decisiones de ingeniería tomadas.

---

## 1. Contexto del Desarrollo

### 1.1 Problema a Resolver

Se requería un simulador de señales ECG para un dispositivo embebido (ESP32) capaz de:
- Generar señales ECG realistas en **tiempo real** a 500 Hz
- Simular **8 condiciones clínicas** diferentes
- Producir salida analógica mediante **DAC de 8 bits**
- Visualizar en pantalla **Nextion** con métricas clínicas
- Ejecutarse con recursos limitados (**520 KB RAM**, sin FPU dedicado)

### 1.2 Modelo Teórico de Referencia

Se eligió el modelo de **McSharry et al. (2003)** como base teórica porque:
- Es el estándar en la literatura para simulación de ECG
- Genera morfología PQRST realista mediante ecuaciones diferenciales
- Incluye variabilidad de frecuencia cardíaca (HRV)
- Está bien documentado y validado

**Sin embargo**, el modelo original:
- Estaba implementado en **MATLAB** (lenguaje interpretado)
- Requería cálculos de punto flotante de alta precisión
- No consideraba restricciones de tiempo real
- No incluía patologías más allá de variabilidad HR

---

## 2. Arquitectura del Sistema Desarrollado

### 2.1 Diagrama de Arquitectura General

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    BIOSIMULATOR PRO - ARQUITECTURA ECG                       │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│   ┌──────────────┐    ┌──────────────┐    ┌──────────────┐                  │
│   │   NEXTION    │◄───│   ESP32      │───►│   DAC 8-bit  │                  │
│   │   Display    │    │   MCU        │    │   MCP4725    │                  │
│   │   (HMI)      │    │              │    │              │                  │
│   └──────────────┘    └──────┬───────┘    └──────────────┘                  │
│          ▲                   │                    │                          │
│          │                   │                    ▼                          │
│   ┌──────┴───────┐    ┌──────┴───────┐    ┌──────────────┐                  │
│   │ Métricas     │    │  ECGModel    │    │ Señal        │                  │
│   │ Clínicas     │    │  (C++)       │    │ Analógica    │                  │
│   │ HR,RR,QT,ST  │    │              │    │ 0-3.3V       │                  │
│   └──────────────┘    └──────────────┘    └──────────────┘                  │
│                                                                              │
│   DESARROLLADO EN ESTE PROYECTO:                                             │
│   ├── Adaptación MATLAB → C++ embebido                                      │
│   ├── Sistema de calibración automática                                     │
│   ├── Pipeline de escalado fisiológico                                      │
│   ├── 8 condiciones clínicas implementadas                                  │
│   ├── Medición de métricas en tiempo real                                   │
│   └── Integración con interfaz Nextion                                      │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 2.2 Flujo de Datos en Tiempo Real

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         PIPELINE DE GENERACIÓN                               │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│   ENTRADA                                                                    │
│   ───────                                                                    │
│   • Condición clínica (0-7)                                                 │
│   • HR objetivo (BPM)                                                       │
│   • Parámetros de patología                                                 │
│                                                                              │
│         │                                                                    │
│         ▼                                                                    │
│   ┌─────────────────────────────────────────────────────────────────┐       │
│   │  1. GENERACIÓN RR (HRV)                                         │       │
│   │     ├── Proceso espectral (LF/HF)                               │       │
│   │     ├── Modulación por condición                                │       │
│   │     └── Variabilidad fisiológica                                │       │
│   └─────────────────────────────────────────────────────────────────┘       │
│         │                                                                    │
│         ▼                                                                    │
│   ┌─────────────────────────────────────────────────────────────────┐       │
│   │  2. INTEGRACIÓN EDOs (Runge-Kutta 4)                            │       │
│   │     ├── dx/dt, dy/dt: Trayectoria circular                      │       │
│   │     ├── dz/dt: Morfología PQRST                                 │       │
│   │     └── Paso adaptativo según HR                                │       │
│   └─────────────────────────────────────────────────────────────────┘       │
│         │                                                                    │
│         ▼                                                                    │
│   ┌─────────────────────────────────────────────────────────────────┐       │
│   │  3. CALIBRACIÓN AUTOMÁTICA (desarrollada para este proyecto)    │       │
│   │     ├── Detecta picos R durante primeros 3 latidos              │       │
│   │     ├── Calcula ganancia G = R_objetivo / R_modelo              │       │
│   │     └── Aplica escalado fisiológico a mV                        │       │
│   └─────────────────────────────────────────────────────────────────┘       │
│         │                                                                    │
│         ▼                                                                    │
│   ┌─────────────────────────────────────────────────────────────────┐       │
│   │  4. CONVERSIÓN DAC                                              │       │
│   │     ├── Mapeo mV → 0-255                                        │       │
│   │     ├── Centro en 128 (0 mV)                                    │       │
│   │     └── Clampeo a rango seguro                                  │       │
│   └─────────────────────────────────────────────────────────────────┘       │
│         │                                                                    │
│         ▼                                                                    │
│   SALIDA                                                                     │
│   ──────                                                                     │
│   • Valor DAC 0-255 → Señal analógica                                       │
│   • Valor mV → Display Nextion                                              │
│   • Métricas → HR, RR, QT, QTc, ST                                          │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 3. Adaptaciones Realizadas (MATLAB → C++ Embebido)

### 3.1 Tabla Comparativa

| Aspecto | MATLAB Original | Mi Implementación C++ |
|---------|-----------------|----------------------|
| **Lenguaje** | MATLAB (interpretado) | C++ (compilado ARM) |
| **Plataforma** | PC (GB de RAM) | ESP32 (520 KB RAM) |
| **Precisión** | double (64-bit) | float (32-bit) |
| **Muestreo** | Post-procesado | Tiempo real 500 Hz |
| **HRV** | Pre-calculado offline | Generado on-the-fly |
| **Patologías** | Solo variabilidad HR | 8 condiciones clínicas |
| **Salida** | Array de valores | DAC + Display + Serial |
| **Calibración** | Manual | Automática por pico R |

### 3.2 Decisiones de Diseño Críticas

#### 3.2.1 Integración Numérica

```
┌─────────────────────────────────────────────────────────────────┐
│          ELECCIÓN: RUNGE-KUTTA 4 (RK4)                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│   Alternativas evaluadas:                                        │
│   ├── Euler simple: Inestable a altas HR (>150 BPM)             │
│   ├── Euler mejorado: Deriva acumulativa visible                │
│   └── RK4: Estable, preciso, costo computacional aceptable      │
│                                                                  │
│   Implementación:                                                │
│   ┌──────────────────────────────────────────────────────────┐  │
│   │  k1 = f(t, y)                                            │  │
│   │  k2 = f(t + dt/2, y + dt*k1/2)                          │  │
│   │  k3 = f(t + dt/2, y + dt*k2/2)                          │  │
│   │  k4 = f(t + dt, y + dt*k3)                              │  │
│   │  y_next = y + (dt/6)*(k1 + 2*k2 + 2*k3 + k4)            │  │
│   └──────────────────────────────────────────────────────────┘  │
│                                                                  │
│   Frecuencia interna: 2000 Hz (sfint)                           │
│   Frecuencia salida: 500 Hz (sfecg)                             │
│   Ratio downsampling: 4:1                                        │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

#### 3.2.2 Sistema de Calibración Automática

```
┌─────────────────────────────────────────────────────────────────┐
│         PROBLEMA: Escala Arbitraria del Modelo                   │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│   El modelo McSharry produce z(t) en unidades arbitrarias.      │
│   El rango depende de los parámetros ai (amplitudes).           │
│   Para diferentes condiciones, el rango cambia.                 │
│                                                                  │
│   SOLUCIÓN DESARROLLADA:                                         │
│   ──────────────────────                                         │
│                                                                  │
│   1. Fase de Calibración (primeros 3 latidos):                  │
│      ├── Detectar picos R (máximo z por ciclo)                  │
│      ├── Acumular en buffer calibrationRPeaks[]                 │
│      └── No mostrar señal (evita saltos visuales)               │
│                                                                  │
│   2. Cálculo de Ganancia:                                        │
│      ┌────────────────────────────────────────────┐             │
│      │  R_modelo = promedio(calibrationRPeaks[])  │             │
│      │  G = R_objetivo / R_modelo                 │             │
│      │  donde R_objetivo = 1.0 mV (clínico)       │             │
│      └────────────────────────────────────────────┘             │
│                                                                  │
│   3. Aplicación Continua:                                        │
│      ┌────────────────────────────────────────────┐             │
│      │  z_mV = (z_raw - baseline) × G            │             │
│      └────────────────────────────────────────────┘             │
│                                                                  │
│   RESULTADO: Señal siempre en escala clínica correcta           │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

#### 3.2.3 Medición de Métricas por Ventanas Angulares

```
┌─────────────────────────────────────────────────────────────────┐
│         SISTEMA DE VENTANAS ANGULARES (desarrollado)             │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│   PROBLEMA: Medir amplitudes PQRST sin detectores complejos     │
│                                                                  │
│   SOLUCIÓN: Usar la fase angular θ del modelo                   │
│                                                                  │
│   En el modelo, cada onda tiene posición angular fija:          │
│                                                                  │
│   θ (rad)     -π        -π/2        0        π/2        π       │
│               │          │          │          │          │      │
│         ──────┼──────────┼──────────┼──────────┼──────────┼───  │
│               │    P     │    QRS   │          │    T     │      │
│               │  ╭─╮     │   ╱╲     │          │   ╭─╮    │      │
│               │ ╱   ╲    │  ╱  ╲    │          │  ╱   ╲   │      │
│               │╱     ╲   │ ╱    ╲   │          │ ╱     ╲  │      │
│         ──────┴───────╲──┼╱──────╲──┼──────────┼╱───────╲─┴───  │
│                        ╲ │        ╲ │          │          │      │
│                         ╲│  Q   S  ╲│          │          │      │
│                          V                                       │
│                                                                  │
│   Implementación en AngularWindows:                              │
│   ┌──────────────────────────────────────────────────────────┐  │
│   │  P_center = -1.22 rad (-70°)  │  P_width = ±30°         │  │
│   │  Q_center = -0.26 rad (-15°)  │  Q_width = ±10°         │  │
│   │  R_center =  0.00 rad (0°)    │  R_width = ±10°         │  │
│   │  S_center = +0.26 rad (+15°)  │  S_width = ±10°         │  │
│   │  T_center = +1.75 rad (+100°) │  T_width = ±50°         │  │
│   └──────────────────────────────────────────────────────────┘  │
│                                                                  │
│   Para cada ciclo:                                               │
│   1. Almacenar muestras (θ, z_mV) en CycleSamples               │
│   2. Al fin de ciclo, buscar extremos en cada ventana           │
│   3. Calcular intervalos PR, QRS, QT por diferencia angular     │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 4. Implementación de Condiciones Clínicas

### 4.1 Arquitectura de Modificadores

```
┌─────────────────────────────────────────────────────────────────┐
│              SISTEMA DE CONDICIONES (8 patologías)               │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│   Cada condición modifica diferentes aspectos del modelo:       │
│                                                                  │
│   ┌─────────────────┬────────────┬────────────┬─────────────┐   │
│   │ Condición       │ Modifica   │ Modifica   │ Modifica    │   │
│   │                 │ Timing     │ Morfología │ Variabilidad│   │
│   ├─────────────────┼────────────┼────────────┼─────────────┤   │
│   │ NORMAL          │ -          │ -          │ Normal      │   │
│   │ TACHYCARDIA     │ HR > 100   │ -          │ Reducida    │   │
│   │ BRADYCARDIA     │ HR < 60    │ -          │ Reducida    │   │
│   │ AFIB            │ RR irreg.  │ Sin onda P │ Alta (25%)  │   │
│   │ VFIB*           │ Caótico    │ Sin PQRST  │ Máxima      │   │
│   │ AV_BLOCK_1      │ PR > 200ms │ -          │ Normal      │   │
│   │ ST_ELEVATION    │ -          │ ST elevado │ Normal      │   │
│   │ ST_DEPRESSION   │ -          │ ST depr.   │ Normal      │   │
│   └─────────────────┴────────────┴────────────┴─────────────┘   │
│                                                                  │
│   * VFIB usa modelo alternativo (ver sección 4.2)               │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 4.2 Modelo Alternativo para Fibrilación Ventricular

```
┌─────────────────────────────────────────────────────────────────┐
│         VFIB: MODELO ESPECTRAL CAÓTICO (desarrollado)            │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│   PROBLEMA: McSharry genera ondas PQRST organizadas.            │
│   VFib es actividad eléctrica caótica SIN ondas definidas.      │
│                                                                  │
│   SOLUCIÓN: Modelo de superposición de osciladores              │
│                                                                  │
│   ┌──────────────────────────────────────────────────────────┐  │
│   │  VFib(t) = Σᵢ Aᵢ × sin(2π × fᵢ × t + φᵢ)               │  │
│   │                                                          │  │
│   │  donde:                                                  │  │
│   │  • i = 1..5 (5 osciladores)                             │  │
│   │  • fᵢ ∈ [4, 10] Hz (rango VFib: Clayton 1993)           │  │
│   │  • Aᵢ = variable aleatoria (0.5-0.8)                    │  │
│   │  • φᵢ = fase aleatoria                                  │  │
│   │  • Parámetros actualizados cada 100-300 ms              │  │
│   └──────────────────────────────────────────────────────────┘  │
│                                                                  │
│   Escalado:                                                      │
│   • Suma máxima teórica: ~4.0 mV                                │
│   • Amplitud objetivo: 0.5 mV (coarse VFib típico)              │
│   • Factor de escala: 0.125                                      │
│   • Clamp de seguridad: ±0.6 mV                                 │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 5. Conversión a Señal Analógica (DAC)

### 5.1 Pipeline de Escalado

```
┌─────────────────────────────────────────────────────────────────┐
│                    PIPELINE DE SALIDA DAC                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│   z_raw (modelo)                                                 │
│       │                                                          │
│       ▼                                                          │
│   ┌─────────────────────────────────────────────────────────┐   │
│   │  z_mV = (z_raw - baseline) × G                          │   │
│   │  donde G = R_objetivo / R_modelo (calibrado)            │   │
│   └─────────────────────────────────────────────────────────┘   │
│       │                                                          │
│       ▼                                                          │
│   ┌─────────────────────────────────────────────────────────┐   │
│   │  z_clamped = clamp(z_mV, -0.5, +1.5)                    │   │
│   │  Rango de visualización: 2.0 mV total                   │   │
│   └─────────────────────────────────────────────────────────┘   │
│       │                                                          │
│       ▼                                                          │
│   ┌─────────────────────────────────────────────────────────┐   │
│   │  normalized = (z_clamped - MIN) / RANGE                 │   │
│   │  normalized ∈ [0, 1]                                    │   │
│   └─────────────────────────────────────────────────────────┘   │
│       │                                                          │
│       ▼                                                          │
│   ┌─────────────────────────────────────────────────────────┐   │
│   │  DAC = round(normalized × 255)                          │   │
│   │  DAC ∈ [0, 255]                                         │   │
│   └─────────────────────────────────────────────────────────┘   │
│       │                                                          │
│       ▼                                                          │
│   MCP4725 DAC → 0-3.3V salida analógica                          │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 5.2 Constantes de Escalado

```cpp
// Definidas en ecg_model.h
#define ECG_R_TARGET_MV         1.0f    // Objetivo pico R (clínico)
#define ECG_DISPLAY_MIN_MV      -0.5f   // Piso de visualización
#define ECG_DISPLAY_MAX_MV      1.5f    // Techo de visualización
#define ECG_DISPLAY_RANGE_MV    2.0f    // Rango total

// Para VFib (modelo alternativo)
#define VFIB_TARGET_AMPLITUDE   0.5f    // Amplitud coarse VFib
#define VFIB_SAFETY_CLAMP       0.6f    // Límite de seguridad
```

---

## 6. Estructura del Código

### 6.1 Organización de Archivos

```
include/models/
└── ecg_model.h          ← Definiciones, constantes, clase ECGModel

src/models/
└── ecg_model.cpp        ← Implementación completa

Componentes principales de la clase ECGModel:
├── Estado dinámico (x, y, z) + variables RK4
├── Parámetros de onda (ti, ai, bi para PQRST)
├── Generador de proceso RR (HRV)
├── Sistema de calibración automática
├── Sistema de ventanas angulares
├── Métricas clínicas medidas
├── Modelo VFIB alternativo
└── Métodos de generación y conversión
```

### 6.2 Métodos Principales

```cpp
// Generación
float generateSample(float deltaTime);  // Genera muestra en mV
uint8_t getDACValue(float deltaTime);   // Genera valor DAC 0-255

// Configuración
void setParameters(const ECGParameters& params);
void reset();

// Métricas (medidas, no calculadas)
float getCurrentBPM() const;
float getRRInterval_ms() const;
float getPRInterval_ms() const;
float getQRSDuration_ms() const;
float getQTInterval_ms() const;
float getQTcInterval_ms() const;  // Bazett
float getRAmplitude_mV() const;
float getSTDeviation_mV() const;

// Estado
bool isOutputReady() const;      // ¿Calibración completa?
int getCalibrationProgress() const;
```

---

## 7. Integración con el Sistema

### 7.1 Loop Principal (main.cpp)

```cpp
// Simplificado - ver código real para detalles
void loop() {
    float deltaTime = (millis() - lastUpdate) / 1000.0f;
    lastUpdate = millis();
    
    // Generar muestra ECG
    float ecgValue_mV = ecgModel.generateSample(deltaTime);
    uint8_t dacValue = ecgModel.getDACValue(deltaTime);
    
    // Salida DAC
    dac.setVoltage(dacValue, false);
    
    // Enviar a Nextion (cada N muestras)
    if (sampleCount % NEXTION_DECIMATION == 0) {
        updateNextionWaveform(ecgValue_mV);
        updateNextionMetrics(ecgModel.getDisplayMetrics());
    }
    
    // Serial para validación
    Serial.printf(">ecg:%.3f,hr:%.1f,rr:%.0f\n", 
        ecgValue_mV, 
        ecgModel.getCurrentBPM(),
        ecgModel.getRRInterval_ms());
}
```

### 7.2 Comunicación con Nextion

```
┌─────────────────────────────────────────────────────────────────┐
│              PROTOCOLO NEXTION IMPLEMENTADO                      │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│   Waveform (gráfico en tiempo real):                            │
│   ├── Componente: waveform ID 1, canal 0                        │
│   ├── Resolución: 399×211 píxeles                               │
│   ├── Actualización: ~100 Hz (decimación 5:1)                   │
│   └── Mapeo: mV → 0-255 → píxel Y invertido                     │
│                                                                  │
│   Métricas (texto):                                              │
│   ├── tHR.txt = "75"          // BPM                            │
│   ├── tRR.txt = "800"         // ms                             │
│   ├── tQT.txt = "360"         // ms                             │
│   ├── tQTc.txt = "402"        // ms (Bazett)                    │
│   ├── tST.txt = "+0.05"       // mV                             │
│   └── tCond.txt = "Normal"    // Condición                      │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 8. Validación y Pruebas

### 8.1 Herramienta de Validación (Python)

```bash
# Validar ECG contra rangos clínicos
python tools/signal_validator.py --port COM4 --signal ecg --condition NORMAL
python tools/signal_validator.py --port COM4 --signal ecg --condition STE
python tools/signal_validator.py --port COM4 --signal ecg --condition AFIB
```

### 8.2 Criterios de Validación

| Métrica | Rango Normal | Verificación |
|---------|--------------|--------------|
| HR | 60-100 BPM | ✓ Automática |
| RR | 600-1000 ms | ✓ Automática |
| PR | 120-200 ms | ✓ Automática |
| QRS | 60-100 ms | ✓ Automática |
| QT | 320-440 ms | ✓ Automática |
| QTc | 320-460 ms | ✓ Bazett |
| R amplitude | 0.5-1.5 mV | ✓ Calibrado |
| ST deviation | ±0.05 mV | ✓ Medido |

---

## 9. Resumen de Contribuciones

| Componente | Origen | Mi Contribución |
|------------|--------|-----------------|
| Ecuaciones EDO | McSharry 2003 | Adaptación a float32, RK4 optimizado |
| Parámetros PQRST | McSharry 2003 | Ajuste para escala mV clínica |
| Proceso HRV | McSharry 2003 | Generación on-the-fly (vs pre-cálculo) |
| Calibración automática | **Nuevo** | Sistema de detección de pico R |
| Ventanas angulares | **Nuevo** | Medición de métricas sin detectores |
| 8 Condiciones clínicas | **Nuevo** | Modificadores de morfología |
| Modelo VFIB | **Nuevo** | Osciladores caóticos superpuestos |
| Pipeline DAC | **Nuevo** | Escalado y conversión 8-bit |
| Integración Nextion | **Nuevo** | Protocolo y visualización |
| Validador Python | **Nuevo** | Verificación contra rangos clínicos |

---

## 10. Flujo Completo del Modelo ECG

> **Guía Didáctica:** Este es el viaje de una señal ECG desde el encendido hasta la salida del DAC. Cada paso incluye las funciones del código, la matemática involucrada, y el proceso lógico.

### PASO 1: INICIO - Constructor y Reset

**Cuándo ocurre:** Al encender el sistema, se crea el modelo desde cero.

**Funciones involucradas:**
```cpp
ECGModel::ECGModel()           // Constructor
void ECGModel::reset()         // Limpia todo
void ECGModel::initializeWaveParams()  // Configura parámetros PQRST
```

**Proceso:**
1. `ECGModel()` inicializa estado dinámico (x, y, z) = (1, 0, 0.04)
2. `reset()` limpia contadores, buffers, estado de calibración
3. `initializeWaveParams()` configura los 5 parámetros de onda (P, Q, R, S, T)
4. `generateRRProcess()` crea serie de intervalos RR con HRV

**Matemática - Estado Inicial del Sistema:**
```
Estado inicial en el círculo unitario:
  x(0) = 1.0
  y(0) = 0.0
  z(0) = 0.04 (línea base)
  θ(0) = atan2(y, x) = 0 (inicio del ciclo)
```

**Resultado:** Modelo listo para generar, pero NO calibrado aún.

---

### PASO 2: CONFIGURACIÓN - setParameters()

**Cuándo ocurre:** El usuario elige la condición clínica.

**Funciones involucradas:**
```cpp
void ECGModel::setParameters(const ECGParameters& params)
void ECGModel::setNormalMorphology()      // o la condición específica
void ECGModel::applyHRFactCorrection()
```

**Proceso:**
1. `params.condition` determina QUÉ simular (ej: `ECGCondition::AFIB`)
2. Se llama a la función de morfología correspondiente
3. `applyHRFactCorrection()` ajusta duración de ondas según HR

**Modificadores por Condición:**

| Condición | Modificación HR | Modificación Morfología |
|-----------|-----------------|-------------------------|
| NORMAL | 60-100 BPM | Parámetros estándar |
| TACHYCARDIA | >100 BPM | Ondas comprimidas |
| BRADYCARDIA | <60 BPM | Ondas extendidas |
| AFIB | Irregular (±25%) | Sin onda P, RR variable |
| VFIB | Caótico | Modelo alternativo (osciladores) |
| AV_BLOCK_1 | Normal | PR > 200 ms |
| ST_ELEVATION | Normal | Segmento ST elevado |
| ST_DEPRESSION | Normal | Segmento ST deprimido |

**Matemática - Parámetros de Onda (McSharry):**
```
Cada onda (P, Q, R, S, T) tiene 3 parámetros:
  θᵢ = posición angular (rad)
  aᵢ = amplitud (unidades modelo)
  bᵢ = ancho (rad)

Ejemplo (onda R normal):
  θ_R = 0 rad (centro del ciclo)
  a_R = 1.0 (amplitud máxima)
  b_R = 0.1 rad (estrecha)
```

---

### PASO 3: BUCLE - generateSample() cada 2ms

**Cuándo ocurre:** Se llama 500 veces por segundo (500 Hz).

**Función principal:**
```cpp
float ECGModel::generateSample(float deltaTime)
```

**Proceso:**
1. Si condición = VFIB → usar `generateVFibSample()` (modelo alternativo)
2. Calcular ω = 2π/RR (velocidad angular según intervalo RR actual)
3. Ejecutar integración RK4 (4 sub-pasos)
4. Detectar nuevo latido (cruce θ = -π → π)
5. Si calibrando → acumular picos R
6. Si calibrado → aplicar escalado fisiológico
7. Medir métricas por ventanas angulares
8. Retornar valor en mV

**Matemática - Velocidad Angular:**
```
ω = 2π / RR

donde:
  RR = intervalo RR actual (segundos)
  ω = velocidad angular (rad/s)

Ejemplo: HR = 60 BPM → RR = 1.0s → ω = 6.28 rad/s
```

---

### PASO 4: INTEGRACIÓN - Runge-Kutta 4

**Cuándo ocurre:** Dentro de `generateSample()`, para cada muestra.

**Función:**
```cpp
void ECGModel::rungeKutta4Step(float dt, float omega)
void ECGModel::computeDerivatives(const ECGDynamicState& s, 
                                   ECGDynamicState& ds, float omega)
```

**Matemática - Ecuaciones Diferenciales (McSharry 2003):**
```
dx/dt = αx - ωy
dy/dt = αy + ωx

donde α = 1 - √(x² + y²)  // Atrae hacia círculo unitario

dz/dt = -Σᵢ aᵢ Δθᵢ ω exp(-Δθᵢ²/2bᵢ²) - (z - z₀)

donde:
  Δθᵢ = (θ - θᵢ) mod 2π, centrado en [-π, π]
  θ = atan2(y, x)
  z₀ = 0.04 (línea base)
```

**Proceso RK4:**
```cpp
// Paso 1: k1 en punto actual
computeDerivatives(state, k1, omega);

// Paso 2: k2 en punto intermedio
temp = state + k1 * (dt/2);
computeDerivatives(temp, k2, omega);

// Paso 3: k3 en punto intermedio
temp = state + k2 * (dt/2);
computeDerivatives(temp, k3, omega);

// Paso 4: k4 en punto final
temp = state + k3 * dt;
computeDerivatives(temp, k4, omega);

// Combinar
state += (k1 + 2*k2 + 2*k3 + k4) * (dt/6);
```

**Frecuencias:**
- Frecuencia interna (sfint): 2000 Hz
- Frecuencia de salida (sfecg): 500 Hz
- Ratio: 4 pasos RK4 por cada muestra de salida

---

### PASO 5: CALIBRACIÓN - Primeros 3 Latidos

**Cuándo ocurre:** Durante los primeros 3-5 latidos después de reset.

**Funciones:**
```cpp
void ECGModel::performCalibration()
void ECGModel::updateCalibrationBuffer(float zValue)
```

**Proceso:**
1. Detectar picos R (máximo z en cada ciclo)
2. Acumular en `calibrationRPeaks[]`
3. Cuando `calibrationPeakCount >= 3`:
   - Calcular `R_modelo = promedio(calibrationRPeaks[])`
   - Calcular `G = R_objetivo / R_modelo`
   - `isCalibrated = true`

**Matemática - Ganancia de Calibración:**
```
G = R_objetivo / R_modelo

donde:
  R_objetivo = 1.0 mV (estándar clínico)
  R_modelo = promedio de picos R crudos detectados
  
Ejemplo: R_modelo = 0.8 → G = 1.0/0.8 = 1.25
```

**Por qué es necesario:**
- El modelo McSharry produce z(t) en unidades arbitrarias
- La amplitud depende de los parámetros aᵢ
- Diferentes condiciones producen diferentes amplitudes
- La calibración normaliza TODO a escala clínica

---

### PASO 6: ESCALADO FISIOLÓGICO

**Cuándo ocurre:** Después de calibración, en cada muestra.

**Función:**
```cpp
float ECGModel::applyScaling(float zRaw) const
```

**Matemática:**
```
z_mV = (z_raw - baseline) × G

donde:
  z_raw = valor crudo del modelo
  baseline = línea isoeléctrica estimada (~0.04)
  G = ganancia de calibración
```

**Rango de Salida:**
```
Rango modelo crudo: [-0.4, +1.2] (1.6 unidades)
Rango escalado: [-0.5, +1.5] mV (2.0 mV)

Constantes:
  ECG_DISPLAY_MIN_MV = -0.5f
  ECG_DISPLAY_MAX_MV = +1.5f
  ECG_R_TARGET_MV = 1.0f
```

---

### PASO 7: MEDICIÓN POR VENTANAS ANGULARES

**Cuándo ocurre:** Al final de cada ciclo cardíaco.

**Funciones:**
```cpp
void ECGModel::initializeAngularWindows()
float ECGModel::findPeakInWindow(float centerAngle, float widthAngle, bool findMax)
```

**Proceso:**
1. Durante el ciclo: almacenar muestras (θ, z_mV) en `CycleSamples`
2. Al fin de ciclo: buscar extremos en cada ventana angular
3. Calcular intervalos por diferencia de ángulos

**Ventanas Angulares:**
```
        θ (rad)
  -π ────────────────────────────────── +π
   │                                    │
   │  ┌─P─┐     ┌QRS┐        ┌──T──┐   │
   │  │   │     │   │        │     │   │
   │  ▼   ▼     ▼   ▼        ▼     ▼   │
  -70°       -15° 0° +15°        +100°

Ventanas definidas:
  P:  centro = -1.22 rad, ancho = ±30°
  Q:  centro = -0.26 rad, ancho = ±10°
  R:  centro =  0.00 rad, ancho = ±10°
  S:  centro = +0.26 rad, ancho = ±10°
  T:  centro = +1.75 rad, ancho = ±50°
```

**Métricas Calculadas:**
| Métrica | Cálculo |
|---------|---------|
| RR (ms) | Tiempo entre picos R × 1000 |
| PR (ms) | (θ_Q - θ_P) / ω × 1000 |
| QRS (ms) | (θ_S - θ_Q) / ω × 1000 |
| QT (ms) | (θ_T_fin - θ_Q) / ω × 1000 |
| QTc (ms) | QT / √(RR/1000) (Bazett) |

---

### PASO 8: SALIDA - Convertir a DAC

**Cuándo ocurre:** Al final de `generateSample()`.

**Funciones:**
```cpp
uint8_t ECGModel::getDACValue(float deltaTime)
```

**Proceso:**
1. Obtener valor en mV de `generateSample()`
2. Clampear a rango de visualización [-0.5, +1.5] mV
3. Normalizar a [0, 1]
4. Convertir a [0, 255]

**Matemática - Mapeo a DAC:**
```
// Clampear
z_clamped = clamp(z_mV, -0.5, +1.5)

// Normalizar
normalized = (z_clamped - ECG_DISPLAY_MIN) / ECG_DISPLAY_RANGE
           = (z_clamped + 0.5) / 2.0

// Convertir a DAC
DAC = round(normalized × 255)

Mapeo:
  -0.5 mV → DAC 0
   0.0 mV → DAC 64
  +1.0 mV → DAC 191 (pico R típico)
  +1.5 mV → DAC 255
```

---

### PASO ESPECIAL: VFIB - Modelo Alternativo

**Cuándo ocurre:** Cuando `condition == ECGCondition::VFIB`.

**Función:**
```cpp
float ECGModel::generateVFibSample(float deltaTime)
```

**Por qué es necesario:**
- McSharry genera ondas PQRST organizadas
- VFIB es actividad caótica SIN ondas definidas
- Se necesita modelo completamente diferente

**Matemática - Superposición de Osciladores:**
```
VFib(t) = Σᵢ Aᵢ × sin(2π × fᵢ × t + φᵢ)

donde:
  i = 1..5 (5 osciladores)
  fᵢ ∈ [4, 10] Hz (rango VFib típico)
  Aᵢ = variable aleatoria (0.5-0.8)
  φᵢ = fase aleatoria
  
Parámetros actualizados cada 100-300 ms para caos
```

**Escalado VFIB:**
```
Suma máxima teórica: ~4.0 mV
Amplitud objetivo: 0.5 mV (coarse VFib)
Factor de escala: 0.125
Clamp de seguridad: ±0.6 mV
```

---

### 🔁 RESUMEN DEL CICLO ECG

```
┌─────────────────────────────────────────────────────────────────┐
│                    CICLO DE VIDA ECG                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  1️⃣ UNA VEZ AL INICIO:                                          │
│     Constructor → Reset → Inicializar parámetros PQRST          │
│     Generar proceso RR (HRV)                                    │
│                                                                  │
│  2️⃣ CUANDO CAMBIAS CONDICIÓN:                                   │
│     setParameters() → Morfología → Corrección HRFact            │
│     Reset calibración (isCalibrated = false)                    │
│                                                                  │
│  3️⃣ PRIMEROS 3 LATIDOS (CALIBRACIÓN):                           │
│     Detectar picos R → Calcular ganancia G                      │
│     NO mostrar señal (evita saltos visuales)                    │
│                                                                  │
│  4️⃣ CADA 2 ms (BUCLE INFINITO):                                 │
│     RK4 (×4) → Escalado → Métricas → DAC                        │
│                                                                  │
│  🎯 SALIDA:                                                      │
│     500 valores/segundo que simulan ECG real                    │
│     Rango: 0-255 para DAC, mV para display                      │
│     Métricas: HR, RR, PR, QRS, QT, QTc, amplitudes              │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 11. Referencias

| Ref | Cita Completa | Uso en este Trabajo |
|-----|---------------|---------------------|
| [1] | McSharry PE, Clifford GD, Tarassenko L, Smith LA. "A dynamical model for generating synthetic electrocardiogram signals." IEEE Trans Biomed Eng. 2003;50(3):289-294. DOI: 10.1109/TBME.2003.808805 | Modelo base, EDOs, parámetros PQRST |
| [2] | Task Force ESC/NASPE. "Heart rate variability: Standards of measurement." Circulation. 1996;93(5):1043-1065. | HRV, componentes LF/HF |
| [3] | Clayton RH, Murray A, Campbell RWF. "Frequency analysis of human ventricular fibrillation." IEEE Trans Biomed Eng. 1993;40(7):705-711. | Modelo VFIB, rango 4-10 Hz |
| [4] | Goldberger AL, Goldberger ZD, Shvilkin A. "Clinical Electrocardiography: A Simplified Approach." 9th ed. Elsevier, 2017. ISBN: 978-0323401692 | Rangos clínicos, interpretación |
| [5] | Bazett HC. "An analysis of the time-relations of electrocardiograms." Heart. 1920;7:353-370. | Fórmula QTc |

---

*BioSimulator Pro - Documento Metodológico ECG v2.0.0*
