# 🔧 BioSignal Simulator Pro - Documentación Técnica

## Guía para Exposición y Comprensión Profunda del Proyecto

Este documento está diseñado para ayudarte a entender y exponer el proyecto desde una perspectiva técnica e ingenieril.

---

## 1. ARQUITECTURA DEL SISTEMA

### 1.1 Vista General

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        ARQUITECTURA BIOSIGNAL PRO                            │
└─────────────────────────────────────────────────────────────────────────────┘

                    ┌─────────────────────────────────────┐
                    │           CAPA DE USUARIO           │
                    │  ┌─────────────┐  ┌─────────────┐  │
                    │  │   NEXTION   │  │   SERIAL    │  │
                    │  │  (Touch UI) │  │  (Debug)    │  │
                    │  └──────┬──────┘  └──────┬──────┘  │
                    └─────────┼────────────────┼─────────┘
                              │                │
                              ▼                ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                        CAPA DE APLICACIÓN (main.cpp)                         │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐    │
│  │ Event Handler│  │ Command      │  │ Metrics      │  │ State        │    │
│  │ (Nextion)    │  │ Parser       │  │ Collector    │  │ Machine      │    │
│  └──────────────┘  └──────────────┘  └──────────────┘  └──────────────┘    │
└────────────────────────────────┬────────────────────────────────────────────┘
                                 │
                                 ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                     CAPA DE GENERACIÓN (SignalGenerator)                     │
│                                                                              │
│   ┌────────────────────────────────────────────────────────────────────┐    │
│   │                         BUFFER CIRCULAR                             │    │
│   │    [Write Ptr]──────────────────────────────────▶                  │    │
│   │    ████████████████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░         │    │
│   │                                  ◀──────────────[Read Ptr]          │    │
│   │    2048 bytes @ DRAM (velocidad máxima)                            │    │
│   └────────────────────────────────────────────────────────────────────┘    │
│                                                                              │
│   ┌─────────────┐     ┌─────────────┐     ┌─────────────┐                   │
│   │  ECG Model  │     │  EMG Model  │     │  PPG Model  │                   │
│   │  (McSharry) │     │ (Fuglevand) │     │ (Gaussian)  │                   │
│   └─────────────┘     └─────────────┘     └─────────────┘                   │
└────────────────────────────────┬────────────────────────────────────────────┘
                                 │
                                 ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                          CAPA DE HARDWARE                                    │
│                                                                              │
│   ┌────────────────┐    ┌────────────────┐    ┌────────────────┐           │
│   │   Timer ISR    │    │   DAC Output   │    │   Serial2      │           │
│   │   @ 1000 Hz    │───▶│   GPIO25       │    │   (Nextion)    │           │
│   └────────────────┘    └────────────────┘    └────────────────┘           │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 Flujo de Datos en Tiempo Real

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    PIPELINE DE GENERACIÓN DE SEÑAL                           │
└─────────────────────────────────────────────────────────────────────────────┘

CORE 1 (Dedicado a señales)              HARDWARE TIMER              CORE 0
────────────────────────────             ──────────────              ───────

┌──────────────────────┐
│ Tarea de Precálculo  │
│ Prioridad: 5 (Alta)  │
└──────────┬───────────┘
           │
           ▼
┌──────────────────────┐
│ Genera bloque de     │
│ 64 muestras usando   │
│ modelo matemático    │
│ (RK4 para ECG)       │
└──────────┬───────────┘
           │
           ▼
┌──────────────────────┐
│ Escribe en buffer    │◀─────────────────────────────────┐
│ circular (DRAM)      │                                  │
│ bufferWriteIndex++   │                                  │
└──────────────────────┘                                  │
                                                          │
                         ┌──────────────────────┐         │
                         │    TIMER ISR         │         │
                         │    Cada 1000 µs      │         │
                         └──────────┬───────────┘         │
                                    │                     │
                                    ▼                     │
                         ┌──────────────────────┐         │
                         │ Lee de buffer        │         │
                         │ bufferReadIndex++    │─────────┘
                         └──────────┬───────────┘
                                    │
                                    ▼
                         ┌──────────────────────┐
                         │ dacWrite(GPIO25, val)│
                         │ ~3-5 µs latencia     │
                         └──────────────────────┘


TIEMPOS CRÍTICOS:
═══════════════════════════════════════════════════════════════════════════════
│ Operación                          │ Tiempo     │ Budget @ 1kHz │ Margen    │
═══════════════════════════════════════════════════════════════════════════════
│ ISR completa (read + dacWrite)     │ ~5 µs      │ 1000 µs       │ 200×      │
│ Generación 1 muestra ECG (RK4)     │ ~10 µs     │ -             │ -         │
│ Bloque 64 muestras                 │ ~700 µs    │ 64000 µs      │ 91×       │
│ Comunicación Nextion (1 comando)   │ ~2 ms      │ -             │ -         │
═══════════════════════════════════════════════════════════════════════════════
```

---

## 2. DISEÑO DE SOFTWARE

### 2.1 Estructura de Archivos

```
BioSignalSimulator Pro/
│
├── src/                          # Código fuente
│   ├── main.cpp                  # Punto de entrada, lógica principal
│   ├── signal_generator.cpp      # Gestor de generación (singleton)
│   ├── ecg_model.cpp             # Modelo ECG McSharry
│   ├── emg_model.cpp             # Modelo EMG Fuglevand
│   ├── ppg_model.cpp             # Modelo PPG Gaussiano
│   └── nextion_ui.cpp            # Driver pantalla Nextion
│
├── include/                      # Headers
│   ├── config.h                  # Configuración global (pines, frecuencias)
│   ├── signal_types.h            # Tipos y estructuras de datos
│   ├── param_limits.h            # Límites por condición clínica
│   ├── signal_generator.h        # Interfaz del generador
│   ├── ecg_model.h               # Interfaz modelo ECG
│   ├── emg_model.h               # Interfaz modelo EMG
│   ├── ppg_model.h               # Interfaz modelo PPG
│   └── nextion_ui.h              # Interfaz UI Nextion
│
├── nextion/                      # Archivos pantalla
│   └── BioSignalPro.hmi.txt      # Diseño UI (instrucciones)
│
├── docs/                         # Documentación
│   ├── README_SCIENTIFIC.md      # Modelos matemáticos
│   ├── README_SYSTEM.md          # Manual de usuario
│   └── README_TECHNICAL.md       # Este documento
│
├── platformio.ini                # Configuración PlatformIO
└── README.md                     # Descripción general
```

### 2.2 Patrones de Diseño Utilizados

#### Singleton (SignalGenerator)
```cpp
// Solo una instancia del generador en todo el sistema
class SignalGenerator {
private:
    static SignalGenerator* instance;
    SignalGenerator();  // Constructor privado
    
public:
    static SignalGenerator* getInstance() {
        if (instance == nullptr) {
            instance = new SignalGenerator();
        }
        return instance;
    }
};
```
**¿Por qué?**: Garantiza acceso coordinado al hardware (DAC, Timer) desde múltiples puntos del código.

#### Producer-Consumer (Buffer Circular)
```cpp
// Tarea productora (Core 1)           // ISR consumidora
while (true) {                         void IRAM_ATTR timerISR() {
    if (freeSpace >= 64) {                 value = buffer[readIdx];
        for (i = 0; i < 64; i++) {         readIdx = (readIdx + 1) % SIZE;
            buffer[writeIdx] = gen();       dacWrite(PIN, value);
            writeIdx = (writeIdx+1)%SIZE;  }
        }
    }
}
```
**¿Por qué?**: Desacopla la generación (puede variar en tiempo) de la salida (debe ser exacta cada 1ms).

#### State Machine (Control de Señal)
```cpp
enum class SignalState {
    STOPPED,   // Sin señal
    RUNNING,   // Generando activamente
    PAUSED     // Congelado, puede resumir
};
```
**¿Por qué?**: Control claro de transiciones y comportamiento según estado.

### 2.3 Sincronización y Concurrencia

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    MECANISMOS DE SINCRONIZACIÓN                              │
└─────────────────────────────────────────────────────────────────────────────┘

MUTEX (signalMutex):
  - Protege: currentSignal, parámetros del modelo
  - Usado por: startSignal(), updateParameters(), getCurrentData()
  - Timeout: portMAX_DELAY (bloquea hasta obtener)

VOLATILE (bufferReadIndex, bufferWriteIndex):
  - Sin mutex (operaciones atómicas en ESP32 para uint16_t)
  - Acceso desde ISR y tarea → volatile obligatorio

SEMÁFORO BINARIO (bufferSemaphore):
  - Sincronización inicial del buffer
  - Give después de prefill, take antes de iniciar timer

                    ┌──────────────┐
                    │    MUTEX     │
                    └──────┬───────┘
                           │
           ┌───────────────┼───────────────┐
           ▼               ▼               ▼
    ┌────────────┐  ┌────────────┐  ┌────────────┐
    │ start()    │  │ update()   │  │ getData()  │
    │            │  │            │  │            │
    │ take()     │  │ take()     │  │ take()     │
    │ ...modify  │  │ ...modify  │  │ ...read    │
    │ give()     │  │ give()     │  │ give()     │
    └────────────┘  └────────────┘  └────────────┘
```

---

## 3. MODELOS MATEMÁTICOS - RESUMEN PARA EXPOSICIÓN

### 3.1 ECG: ¿Cómo genera un latido?

```
El modelo McSharry representa el corazón como un OSCILADOR:

    1. Un punto gira en círculo (como las agujas de un reloj)
    2. Cuando pasa por ciertos ángulos, se añaden "picos" gaussianos
    3. Cada pico corresponde a una onda del ECG (P, Q, R, S, T)

             ↑ z (potencial ECG)
             │
             │         R
             │        ╱╲
             │       ╱  ╲
             │   P  ╱    ╲  T
             │  ╱╲ ╱      ╲╱╲
    ─────────┼─────────────────────────▶ θ (ángulo)
             │    ╲    ╱
             │     ╲  ╱
             │   Q  ╲╱  S
             │
             
    ECUACIÓN CLAVE:
    dz/dt = -Σ aᵢ · Δθᵢ · exp(-Δθᵢ²/(2bᵢ²)) - (z - z₀)
            ▲            ▲
            │            │
      amplitud      forma gaussiana
      de cada onda  (ancho bᵢ)
```

**Puntos clave para exponer:**
- Sistema de 3 ecuaciones diferenciales
- Integración numérica RK4 (4º orden, muy preciso)
- Variabilidad RR mediante modulación LF/HF
- Cada patología modifica los parámetros aᵢ, bᵢ

### 3.2 EMG: ¿Cómo genera actividad muscular?

```
El modelo Fuglevand simula 100 UNIDADES MOTORAS independientes:

    Unidad Motora = 1 neurona + todas las fibras que inerva

    ┌─────────────────────────────────────────────────────────────┐
    │  MU #1  ───▶  dispara a 8 Hz   ───▶  MUAP pequeño          │
    │  MU #2  ───▶  dispara a 10 Hz  ───▶  MUAP pequeño          │
    │  ...                                                        │
    │  MU #50 ───▶  dispara a 15 Hz  ───▶  MUAP mediano          │
    │  ...                                                        │
    │  MU #100───▶  dispara a 25 Hz  ───▶  MUAP grande           │
    └─────────────────────────────────────────────────────────────┘
                              │
                              ▼
    EMG(t) = Σᵢ₌₁¹⁰⁰ Σⱼ MUAPᵢ(t - tᵢⱼ)    ← Suma de todos los disparos
    
    PRINCIPIO DEL TAMAÑO (Henneman):
    - Unidades pequeñas se reclutan PRIMERO
    - Unidades grandes se reclutan con más esfuerzo
    - Threshold aumenta exponencialmente con el índice
```

**Puntos clave para exponer:**
- Reclutamiento ordenado por tamaño
- Tasa de disparo aumenta con el esfuerzo
- La suma de MUAPs produce señal similar a ruido
- Patologías cambian forma del MUAP o patrón de disparo

### 3.3 PPG: ¿Cómo genera el pulso?

```
El modelo usa DOS GAUSSIANAS superpuestas:

    ↑ Amplitud
    │
    │         Pico Sistólico (G1)
    │            ╱╲
    │           ╱  ╲    Muesca Dicrótica (G2)
    │          ╱    ╲      ╱╲
    │         ╱      ╲    ╱  ╲
    │        ╱        ╲  ╱    ╲
    │       ╱          ╲╱      ╲
    ├──────╱────────────────────╲──────▶ Tiempo
    │
    │      │←─ σ₁ ─→│  │←─ σ₂ ─→│
    │      μ₁           μ₂
    
    PPG(t) = A₁·exp(-(t-μ₁)²/(2σ₁²)) + A₂·exp(-(t-μ₂)²/(2σ₂²))
             ▲                          ▲
             │                          │
        Onda sistólica            Onda diastólica
        (eyección del VI)         (reflexión arterial)
```

**Puntos clave para exponer:**
- Primera gaussiana = contracción del corazón
- Segunda gaussiana = reflexión de la onda de presión
- Modulación respiratoria añade variación realista
- SpO2 derivado del ratio rojo/infrarrojo (simulado)

---

## 4. ASPECTOS CRÍTICOS DE TIEMPO REAL

### 4.1 ¿Por qué es "Tiempo Real"?

```
DEFINICIÓN: Un sistema es de tiempo real cuando la CORRECCIÓN depende
            no solo del resultado, sino de CUÁNDO se produce.

EN ESTE PROYECTO:
    - El DAC DEBE actualizarse cada 1000 µs exactamente
    - Si se retrasa → la señal se distorsiona
    - Si se adelanta → imposible (timer lo controla)
    
    ┌────────────────────────────────────────────────────────┐
    │   DEADLINE                                              │
    │   ════════                                              │
    │                                                         │
    │   T=0ms      T=1ms      T=2ms      T=3ms      T=4ms   │
    │     │          │          │          │          │      │
    │     ▼          ▼          ▼          ▼          ▼      │
    │   ┌───┐      ┌───┐      ┌───┐      ┌───┐      ┌───┐   │
    │   │ISR│      │ISR│      │ISR│      │ISR│      │ISR│   │
    │   │5µs│      │5µs│      │5µs│      │5µs│      │5µs│   │
    │   └───┘      └───┘      └───┘      └───┘      └───┘   │
    │                                                         │
    │   Margen: 995 µs por período (99.5% libre)             │
    └────────────────────────────────────────────────────────┘
```

### 4.2 Estrategias de Optimización Implementadas

| Estrategia | Implementación | Beneficio |
|------------|----------------|-----------|
| **ISR mínima** | Solo lee buffer + dacWrite | Tiempo ISR < 5µs |
| **Buffer circular** | 2048 muestras = 2s headroom | Absorbe variaciones |
| **Precálculo en bloque** | 64 muestras por iteración | Amortiza overhead |
| **Core dedicado** | Core 1 solo para señales | Sin interrupciones WiFi/BT |
| **DRAM_ATTR** | Buffer en RAM rápida | Acceso en 1 ciclo |
| **Volatile** | Índices de buffer | Coherencia sin mutex |

### 4.3 ¿Qué pasa si algo falla?

```
DETECCIÓN DE BUFFER UNDERRUN:

    if (bufferReadIndex == bufferWriteIndex) {
        // ¡BUFFER VACÍO! La ISR no tiene datos
        bufferUnderruns++;           // Contar error
        dacWrite(PIN, 128);          // Salida en centro (silencio)
        return;                      // No crashear
    }

MONITOREO (cada 5 segundos):
    - Buffer Usage < 10% → WARNING en serial
    - ISR Max Time > 100µs → WARNING
    - Underruns > 0 → Problema de rendimiento
```

---

## 5. PREGUNTAS FRECUENTES DE EXPOSICIÓN

### ¿Por qué ESP32 y no Arduino UNO?

| Característica | Arduino UNO | ESP32 |
|----------------|-------------|-------|
| CPU | 16 MHz, 1 core | 240 MHz, 2 cores |
| RAM | 2 KB | 320 KB |
| DAC | No tiene | 2 canales, 8-bit |
| FreeRTOS | No | Integrado |
| Costo | ~$5 | ~$5 |

**Respuesta**: El ESP32 ofrece dual-core para separar generación de UI, DAC integrado, y suficiente RAM para buffers grandes. Al mismo precio que un Arduino.

### ¿Por qué 1 kHz y no más?

**Respuesta**: 
1. EMG tiene contenido hasta 500 Hz → Nyquist exige mínimo 1000 Hz
2. ECG solo necesita ~300 Hz, pero uniformamos para simplificar
3. Más de 1 kHz no aporta información útil y consume más CPU
4. 1 kHz = 1 ms entre muestras, fácil de medir y debuggear

### ¿Por qué buffer circular y no generar directo?

**Respuesta**:
```
SIN BUFFER (problemático):
    ISR cada 1ms → debe calcular muestra → si tarda >1ms, se pierde el deadline

CON BUFFER (robusto):
    ISR cada 1ms → solo lee de buffer (5µs)
    Otra tarea llena el buffer cuando puede (no crítico en tiempo)
```

### ¿Cómo garantizas que los valores son clínicamente correctos?

**Respuesta**:
1. Cada parámetro tiene referencia bibliográfica (ver README_SCIENTIFIC.md)
2. Los límites están hardcodeados en `param_limits.h`
3. Si el usuario elige "Taquicardia", NO puede poner 50 BPM
4. Los rangos se basan en guías clínicas (AHA, ESC)

### ¿Qué pasa si desconecto la pantalla Nextion?

**Respuesta**:
- El sistema detecta timeout en la inicialización
- Imprime "Nextion no respondió (continuando sin UI)"
- Todo el control sigue disponible por Serial (USB)
- El DAC sigue funcionando normalmente

---

## 6. MÉTRICAS PARA IMPRESIONAR

### Rendimiento
- **Precisión temporal**: ±0.1% (timer de hardware)
- **Latencia ISR**: 5 µs (medido con GPIO toggle)
- **Buffer headroom**: 2 segundos de señal precalculada
- **Tiempo de arranque**: <2 segundos

### Eficiencia
- **RAM usada**: 7.2% (sobra 92.8% para expansión)
- **Flash usada**: 25.2% (cabe 3× más código)
- **CPU estimada**: 35-40% (60% libre para más features)

### Complejidad
- **Líneas de código**: ~3000
- **Archivos fuente**: 10
- **Condiciones médicas**: 26 (9+10+7)
- **Parámetros ajustables**: 15+

---

## 7. POSIBLES PREGUNTAS DEL JURADO Y RESPUESTAS

**P: ¿Por qué no usas un generador de funciones comercial?**
R: Porque los generadores comerciales producen señales sintéticas simples (seno, cuadrada). Este sistema genera señales con variabilidad fisiológica realista, incluyendo HRV, ruido muscular, y artefactos de movimiento.

**P: ¿Se podría usar para calibrar equipos médicos reales?**
R: No directamente. Es para educación y desarrollo. Los equipos médicos requieren certificación con phantoms calibrados trazables a estándares metrológicos.

**P: ¿Cómo verificaste que las señales son correctas?**
R: Comparación visual y espectral con bases de datos públicas (PhysioNet), verificación de parámetros morfológicos contra literatura clínica.

**P: ¿Qué mejoras harías con más tiempo?**
R: 
- Añadir resolución de 12 bits (DAC externo)
- Implementar protocolo UART aislado para equipos médicos
- Añadir más patologías (100+ condiciones)
- Conexión WiFi para control remoto

---

*Documento técnico para BioSignal Simulator Pro v2.0*
*Última actualización: Diciembre 2025*
