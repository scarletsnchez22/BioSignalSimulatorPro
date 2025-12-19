/**
 * @file config.h
 * @brief Configuración global del sistema BioSimulator Pro
 * @version 1.0.0
 * @date 18 Diciembre 2025
 * 
 * Configuración de hardware, pines, frecuencias y constantes del sistema.
 * 
 * Hardware: ESP32-WROOM-32 + Nextion NX8048T070 (7" 800x480)
 * Alimentación: 2S 18650 (7.4V) + Buck 5V
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// IDENTIFICACIÓN DEL SISTEMA
// ============================================================================
#define DEVICE_NAME             "BioSimulator Pro"
#define FIRMWARE_VERSION        "1.0.0"
#define FIRMWARE_DATE           "18 Diciembre 2025"
#define HARDWARE_MODEL          "ESP32-WROOM-32"
#define HAS_PSRAM               false
#define SRAM_SIZE_KB            520
#define FLASH_SIZE_MB           4

// ============================================================================
// CONFIGURACIÓN DE PINES - DAC
// ============================================================================
#define DAC_SIGNAL_PIN          25      // GPIO25 - DAC1 (Salida principal)
#define DAC_REFERENCE_PIN       26      // GPIO26 - DAC2 (Opcional)

// ============================================================================
// CONFIGURACIÓN DE PINES - NEXTION
// ============================================================================
#define NEXTION_SERIAL          Serial2
#define NEXTION_RX_PIN          16      // RX2
#define NEXTION_TX_PIN          17      // TX2
#define NEXTION_BAUD            115200

// ============================================================================
// CONFIGURACIÓN DE PINES - LED RGB
// ============================================================================
#define LED_RGB_ENABLED         true
#define LED_RGB_RED             4       // GPIO4
#define LED_RGB_GREEN           5       // GPIO5
#define LED_RGB_BLUE            18      // GPIO18
#define LED_RGB_COMMON_ANODE    false   // false = cátodo común

// ============================================================================
// CONFIGURACIÓN DE PINES - LED STATUS
// ============================================================================
#define LED_STATUS              2       // GPIO2 (LED interno)

// ============================================================================
// CONFIGURACIÓN DE SEÑALES
// ============================================================================

// ============================================================================
// FRECUENCIAS DE MUESTREO DE MODELOS (Nyquist 5x)
// ============================================================================
// Criterio: fs >= 5 * fmax_fisiologica
// ECG: fmax=150Hz -> fs=750Hz | PPG: fmax=20Hz -> fs=100Hz | EMG: fmax=400Hz -> fs=2000Hz

const uint16_t MODEL_SAMPLE_RATE_ECG = 750;    // Hz - QRS hasta 150Hz (5x)
const uint16_t MODEL_SAMPLE_RATE_PPG = 100;    // Hz - Pulso hasta 20Hz  (5x)
const uint16_t MODEL_SAMPLE_RATE_EMG = 2000;   // Hz - sEMG hasta 400Hz

// Alias para compatibilidad con codigo legacy (signal_engine, serial_handler)
const uint16_t SAMPLE_RATE_HZ = 1000;          // Hz - frecuencia base legacy

// deltaTime para cada modelo (segundos)
const float MODEL_DT_ECG = 1.0f / MODEL_SAMPLE_RATE_ECG;  // 1.333 ms
const float MODEL_DT_PPG = 1.0f / MODEL_SAMPLE_RATE_PPG;  // 10 ms
const float MODEL_DT_EMG = 1.0f / MODEL_SAMPLE_RATE_EMG;  // 0.5 ms

// Intervalo de tick en microsegundos
const uint32_t MODEL_TICK_US_ECG = 1000000 / MODEL_SAMPLE_RATE_ECG;  // 1333 us
const uint32_t MODEL_TICK_US_PPG = 1000000 / MODEL_SAMPLE_RATE_PPG;  // 10000 us
const uint32_t MODEL_TICK_US_EMG = 1000000 / MODEL_SAMPLE_RATE_EMG;  // 500 us

// ============================================================================
// FRECUENCIAS DE ENVIO A DISPLAYS
// ============================================================================
// Nextion 7" Basic: alineamos downsampling con Serial Plotter
const uint16_t NEXTION_SEND_RATE = 200;         // Hz efectivo solicitado
const uint16_t SERIAL_PLOTTER_RATE_ECG = 200;   // Hz - ver 3-4 latidos
const uint16_t SERIAL_PLOTTER_RATE_PPG = 100;   // Hz - igual al modelo (sin downsample)
const uint16_t SERIAL_PLOTTER_RATE_EMG = 100;   // Hz - ver ~2 ciclos de secuencia

// Ratios de downsampling para Nextion
const uint8_t NEXTION_DOWNSAMPLE_ECG = MODEL_SAMPLE_RATE_ECG / SERIAL_PLOTTER_RATE_ECG;  // 4
const uint8_t NEXTION_DOWNSAMPLE_PPG = (MODEL_SAMPLE_RATE_PPG >= NEXTION_SEND_RATE)
                                       ? (MODEL_SAMPLE_RATE_PPG / NEXTION_SEND_RATE)
                                       : 1;  // Mantener al menos 1
const uint8_t NEXTION_DOWNSAMPLE_EMG = MODEL_SAMPLE_RATE_EMG / NEXTION_SEND_RATE;  // 20

// ============================================================================
// CONFIGURACIÓN DAC
// ============================================================================
#define DAC_RESOLUTION          8       // 8 bits (0-255)
#define DAC_MAX_VALUE           255
#define DAC_CENTER_VALUE        128
#define DAC_VOLTAGE_MAX         3.3f    // Voltios
#define DAC_MV_PER_STEP         (DAC_VOLTAGE_MAX * 1000.0f / 256.0f)  // ~12.9 mV

// ============================================================================
// CONFIGURACIÓN DE BUFFERS
// ============================================================================
#define SIGNAL_BUFFER_SIZE      2048    // Muestras (~2 segundos)
#define PRECALC_BUFFER_SIZE     512     // Bloques de pre-cálculo

// ============================================================================
// CONFIGURACIÓN DE TAREAS FREERTOS
// ============================================================================
#define CORE_SIGNAL_GENERATION  1       // Core 1: Generación tiempo real
#define CORE_UI_COMMUNICATION   0       // Core 0: UI y comunicación

#define STACK_SIZE_SIGNAL       4096
#define STACK_SIZE_UI           4096
#define STACK_SIZE_MONITOR      2048

#define TASK_PRIORITY_SIGNAL    5       // Alta prioridad
#define TASK_PRIORITY_UI        2       // Media prioridad
#define TASK_PRIORITY_MONITOR   1       // Baja prioridad

// ============================================================================
// CONFIGURACIÓN DE TIEMPOS
// ============================================================================
#define UI_UPDATE_INTERVAL_MS   100     // 10 Hz refresh UI
#define METRICS_UPDATE_MS       250     // 4 Hz actualización métricas
#define WAVEFORM_UPDATE_MS      10      // 100 Hz refresh waveform

// ============================================================================
// CONFIGURACION NEXTION WAVEFORM (7" Basic)
// ============================================================================
#define NEXTION_WAVEFORM_WIDTH  700     // Ancho del waveform en pixeles
#define NEXTION_WAVEFORM_HEIGHT 380     // Altura del waveform en pixeles
#define WAVEFORM_COMPONENT_ID   1       // ID del componente waveform
#define WAVEFORM_CHANNEL        0       // Canal del waveform (solo usamos 1)

// Tiempo visible en Nextion a 50 Hz: 700px / 50Hz = 14 segundos
// A 75 BPM (800ms/latido): ~17 latidos visibles

// ============================================================================
// CONFIGURACIÓN DE DEBUG
// ============================================================================
#ifdef DEBUG_ENABLED
    #define DEBUG_PRINT(x)      Serial.print(x)
    #define DEBUG_PRINTLN(x)    Serial.println(x)
    #define DEBUG_PRINTF(...)   Serial.printf(__VA_ARGS__)
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
    #define DEBUG_PRINTF(...)
#endif

// Macro para verificar memoria
#define CHECK_HEAP(min_kb) (ESP.getFreeHeap() >= ((min_kb) * 1024))

#endif // CONFIG_H
