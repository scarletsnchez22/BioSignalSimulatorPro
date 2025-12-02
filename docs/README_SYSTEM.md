# 🏥 BioSignal Simulator Pro - Manual del Sistema

## Guía Rápida de Usuario

---

## 1. DESCRIPCIÓN GENERAL

**BioSignal Simulator Pro** es un generador de señales biomédicas de alta fidelidad basado en ESP32, capaz de producir señales ECG, EMG y PPG con características fisiológicamente realistas.

### Características Principales

| Característica | Especificación |
|----------------|----------------|
| Microcontrolador | ESP32-WROOM-32 (Dual-Core 240MHz) |
| Pantalla | Nextion NX4024T032 (320×240 px) |
| Frecuencia de Muestreo | 1000 Hz (1 kHz) |
| Resolución DAC | 8 bits (256 niveles) |
| Señales Soportadas | ECG, EMG, PPG |
| Condiciones por Señal | 9 ECG, 10 EMG, 7 PPG |

---

## 2. HARDWARE REQUERIDO

### 2.1 Componentes

| Componente | Modelo | Cantidad |
|------------|--------|----------|
| Microcontrolador | ESP32-WROOM-32 DevKit | 1 |
| Pantalla | Nextion NX4024T032 | 1 |
| Cable USB | Tipo A a Micro-B | 1 |
| Cables Dupont | Hembra-Hembra | 4 |
| Osciloscopio (opcional) | Cualquier modelo | 1 |

### 2.2 Diagrama de Conexiones

```
    ┌─────────────────────────────────────────────────────────────────┐
    │                    CONEXIONES DEL SISTEMA                        │
    └─────────────────────────────────────────────────────────────────┘

         ESP32-WROOM-32                         NEXTION NX4024T032
        ┌──────────────┐                       ┌──────────────────┐
        │              │                       │    ┌────────┐    │
        │         3.3V ●───────────────────────● 5V │        │    │
        │          GND ●───────────────────────● GND│ DISPLAY│    │
        │              │                       │    │320×240 │    │
        │  GPIO17 (TX) ●───────────────────────● RX │        │    │
        │  GPIO16 (RX) ●───────────────────────● TX │        │    │
        │              │                       │    └────────┘    │
        │  GPIO25 (DAC)●────┐                  │                  │
        │              │    │                  └──────────────────┘
        │    GPIO2 (LED)●   │
        │              │    │
        └──────────────┘    │
                            │
                            ▼
                    ┌───────────────┐
                    │ OSCILOSCOPIO  │  ← Señal analógica 0-3.3V
                    │   o EQUIPO    │
                    └───────────────┘

    PINOUT RESUMIDO:
    ════════════════════════════════════════════════════════════════
    │ ESP32 Pin  │ Función           │ Conectar a        │ Notas   │
    ════════════════════════════════════════════════════════════════
    │ 3.3V / 5V  │ Alimentación      │ Nextion +5V       │ Rojo    │
    │ GND        │ Tierra            │ Nextion GND       │ Negro   │
    │ GPIO17     │ Serial TX         │ Nextion RX        │ Amarillo│
    │ GPIO16     │ Serial RX         │ Nextion TX        │ Azul    │
    │ GPIO25     │ Salida DAC        │ Osciloscopio CH1  │ Verde   │
    │ GPIO2      │ LED Status        │ (Interno)         │ -       │
    ════════════════════════════════════════════════════════════════
```

---

## 3. INTERFAZ DE USUARIO (NEXTION)

### 3.1 Flujo de Navegación

```
    ┌─────────┐     ┌───────────────┐     ┌─────────────────┐
    │ SPLASH  │────▶│ MENÚ SEÑALES  │────▶│ MENÚ CONDICIONES│
    │         │     │ ECG/EMG/PPG   │     │ (por señal)     │
    └─────────┘     └───────────────┘     └────────┬────────┘
         ▲                 ▲                       │
         │                 │                       ▼
         │                 │              ┌─────────────────┐
         │                 └──────────────│   SIMULACIÓN    │
         │                                │  (Waveform +    │
         │                                │   Métricas)     │
         │                                └────────┬────────┘
         │                                         │
         │                                         ▼
         │                                ┌─────────────────┐
         └────────────────────────────────│   PARÁMETROS    │
                                          │  (Ajustes RT)   │
                                          └─────────────────┘
```

### 3.2 Pantallas del Sistema

#### Pantalla SPLASH (Inicio)
- Logo y versión del sistema
- Botón "COMENZAR" para iniciar

#### Pantalla MENÚ SEÑALES
- **ECG** - Electrocardiograma
- **EMG** - Electromiograma  
- **PPG** - Fotopletismograma

#### Pantalla MENÚ CONDICIONES

**ECG (9 condiciones):**
| # | Condición | Descripción |
|---|-----------|-------------|
| 1 | Normal | ECG sinusal normal, 60-100 BPM |
| 2 | Taquicardia | Frecuencia elevada >100 BPM |
| 3 | Bradicardia | Frecuencia baja <60 BPM |
| 4 | Fibrilación Auricular | Ausencia onda P, RR irregular |
| 5 | Fibrilación Ventricular | Caótico, emergencia vital |
| 6 | PVC | Extrasístoles ventriculares |
| 7 | Bloqueo de Rama | QRS ensanchado >120ms |
| 8 | ST Elevación | Elevación segmento ST (STEMI) |
| 9 | ST Depresión | Depresión ST (isquemia) |

**EMG (10 condiciones):**
| # | Condición | % MVC |
|---|-----------|-------|
| 1 | Reposo | 0-5% |
| 2 | Contracción Leve | 10-25% |
| 3 | Contracción Moderada | 25-50% |
| 4 | Contracción Fuerte | 50-75% |
| 5 | Contracción Máxima | 100% |
| 6 | Temblor | Oscilación 4-12 Hz |
| 7 | Miopatía | MUAP cortos |
| 8 | Neuropatía | MUAP largos |
| 9 | Fasciculación | Disparos espontáneos |
| 10 | Fatiga | Cambio espectral |

**PPG (7 condiciones):**
| # | Condición | Características |
|---|-----------|-----------------|
| 1 | Normal | PI 2-5%, SpO2 >96% |
| 2 | Arritmia | RR variable |
| 3 | Perfusión Baja | PI <1% |
| 4 | Perfusión Alta | PI >5% |
| 5 | Vasoconstricción | Pulso atenuado |
| 6 | Artefacto Movimiento | Ruido alto |
| 7 | SpO2 Bajo | 85-90% |

#### Pantalla SIMULACIÓN

```
┌─────────────────────────────────────────────────────────────┐
│ ECG Normal               ▶ RUNNING         HR: 75 BPM      │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  +1.0mV ─┬────────────────────────────────────────────     │
│          │      ╱╲                                          │
│   0.0mV ─┼────╱──╲─────────────────────────────────────    │
│          │   ╱    ╲    ╱╲                                   │
│  -0.5mV ─┴──╱──────╲──╱──╲───────────────────────────      │
│            0s        1s        2s        3s                 │
│                                                             │
├─────────────────────────────────────────────────────────────┤
│  RR: 800ms    │    Latidos: 45    │    Ruido: 5%          │
├─────────────────────────────────────────────────────────────┤
│  [▶]  [⏸]  [⏹]  [⚙]                          [MENÚ]      │
└─────────────────────────────────────────────────────────────┘
```

**Controles:**
- **▶ Play**: Iniciar/reanudar señal
- **⏸ Pause**: Pausar señal
- **⏹ Stop**: Detener señal
- **⚙ Params**: Ajustar parámetros
- **MENÚ**: Volver a selección

#### Pantalla PARÁMETROS

Permite ajustar en tiempo real:
- **Heart Rate**: Frecuencia cardíaca (BPM)
- **Amplitud**: Escala de la señal
- **Ruido**: Nivel de ruido (%)

⚠️ **Los rangos están limitados según la condición seleccionada** para mantener coherencia clínica.

---

## 4. CONTROL POR SERIAL (USB)

### 4.1 Configuración

| Parámetro | Valor |
|-----------|-------|
| Baud Rate | 115200 |
| Data Bits | 8 |
| Parity | None |
| Stop Bits | 1 |

### 4.2 Comandos Disponibles

```
SELECCIÓN DE SEÑAL:
  e, E    - Modo ECG (luego presionar 1-9 para condición)
  m, M    - Modo EMG (luego presionar 1-0 para condición)
  g, G    - Modo PPG (luego presionar 1-7 para condición)

CONTROL:
  p, P    - Pausar señal
  r, R    - Reanudar señal
  s, S    - Detener señal

INFORMACIÓN:
  i, I    - Mostrar información del sistema
  h, H    - Mostrar ayuda

PARÁMETROS EN TIEMPO REAL:
  b[valor]  - Cambiar HR (ej: b80 → 80 BPM)
  a[valor]  - Cambiar amplitud (ej: a1.2)
  n[valor]  - Cambiar ruido (ej: n10 → 10%)
  t[valor]  - Cambiar ST shift (ECG)
  f[valor]  - Cambiar frecuencia (EMG)
  d[valor]  - Cambiar dicrótica (PPG)
  w[valor]  - Cambiar onda P (ECG)
```

### 4.3 Ejemplo de Uso

```
> e                    // Seleccionar ECG
[MODO ECG] Presione 1-9 para condición

> 2                    // Seleccionar Taquicardia
✓ ECG Taquicardia iniciado @ 1000 Hz

> b130                 // Cambiar a 130 BPM
✓ HR: 130 BPM

> p                    // Pausar
⏸ Señal pausada

> r                    // Reanudar
▶ Señal reanudada

> s                    // Detener
⏹ Señal detenida
```

---

## 5. ESPECIFICACIONES DE SALIDA

### 5.1 Señal Analógica (DAC)

| Parámetro | Valor |
|-----------|-------|
| Pin | GPIO25 |
| Rango de Voltaje | 0 - 3.3V |
| Resolución | 8 bits (256 niveles) |
| Frecuencia de Actualización | 1000 Hz |
| Impedancia de Salida | ~100Ω |

### 5.2 Escalado de Señales

```
                    Valor Normalizado [-1, +1]
                              │
                              ▼
                    ┌─────────────────┐
                    │  DAC = 128 +    │
                    │  (valor × 127)  │
                    └─────────────────┘
                              │
                              ▼
                    Valor DAC [0, 255]
                              │
                              ▼
                    ┌─────────────────┐
                    │  Voltaje =      │
                    │  DAC × 3.3/255  │
                    └─────────────────┘
                              │
                              ▼
                    Voltaje [0, 3.3V]
```

### 5.3 Rangos por Tipo de Señal

| Señal | Rango Simulado | Voltaje Salida | Centro |
|-------|----------------|----------------|--------|
| ECG | -0.5 a +1.5 mV | 0.8 a 2.5V | 1.65V |
| EMG | 0 a 3 mV RMS | 0 a 3.3V | 1.65V |
| PPG | 0 a 100% AC | 0 a 3.3V | 1.65V |

---

## 6. CONSUMO DE RECURSOS ESP32

### 6.1 Memoria

```
RAM:   [=         ]   7.2% usado
       ████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
       23.7 KB / 320 KB

Flash: [===       ]  25.2% usado
       ██████████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
       330 KB / 1.3 MB
```

### 6.2 Distribución de Tareas (FreeRTOS)

| Tarea | Core | Prioridad | Stack | Función |
|-------|------|-----------|-------|---------|
| Precálculo | Core 1 | 5 (Alta) | 4096 B | Generación de muestras |
| Monitoreo | Core 0 | 2 (Baja) | 2048 B | Estadísticas |
| Loop Arduino | Core 1 | 1 | 8192 B | UI, Serial |
| Timer ISR | - | Máxima | - | DAC output @ 1kHz |

### 6.3 Rendimiento

| Métrica | Valor Típico |
|---------|--------------|
| Tiempo ISR | < 5 µs |
| Buffer Headroom | 2 segundos |
| CPU Usage (aprox) | 35-40% |
| Latencia Comando → Salida | < 10 ms |

---

## 7. SOLUCIÓN DE PROBLEMAS

### 7.1 Problemas Comunes

| Problema | Posible Causa | Solución |
|----------|---------------|----------|
| No enciende | USB mal conectado | Verificar cable y puerto |
| Nextion en blanco | Conexiones invertidas | Revisar TX↔RX |
| Sin señal en DAC | Señal no iniciada | Seleccionar condición |
| Ruido excesivo | GND mal conectado | Verificar tierra común |
| Pantalla no responde | Baud rate incorrecto | Debe ser 9600 |

### 7.2 LEDs de Estado

| Estado LED | Significado |
|------------|-------------|
| Encendido fijo | Sistema listo, sin señal |
| Parpadeo lento (1 Hz) | Señal activa, generando |
| Apagado | Error de inicialización |

---

## 8. INFORMACIÓN DE CONTACTO

**Proyecto**: BioSignal Simulator Pro v2.0  
**Plataforma**: ESP32 + Nextion  
**Licencia**: Uso educativo  

---

*Manual generado para BioSignal Simulator Pro v2.0*
*Última actualización: Diciembre 2025*
