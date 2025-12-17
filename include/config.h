/**
 * @file config.h
 * @brief Configuración global del sistema BioSimulator Pro
 * @version 1.1.0
 * @date Diciembre 2024
 * 
 * Configuración de hardware, pines, frecuencias y constantes del sistema.
 * 
 * Hardware: ESP32-WROOM-32 + Nextion NX4024T032
 * Alimentación: 2S 18650 (7.4V) + Buck 5V
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// IDENTIFICACIÓN DEL SISTEMA
// ============================================================================
#define DEVICE_NAME             "BioSimulator Pro"
#define FIRMWARE_VERSION        "1.1.0"
#define FIRMWARE_DATE           "Diciembre 2024"
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
// FRECUENCIA DE MUESTREO
// ============================================================================
const uint16_t SAMPLE_RATE_HZ = 1000;     // 1000 Hz (1 ms)

// Aliases para claridad en el código
const uint16_t SAMPLE_RATE_ECG = SAMPLE_RATE_HZ;
const uint16_t SAMPLE_RATE_EMG = SAMPLE_RATE_HZ;
const uint16_t SAMPLE_RATE_PPG = SAMPLE_RATE_HZ;

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
// CONFIGURACIÓN NEXTION WAVEFORM
// ============================================================================
#define WAVEFORM_HEIGHT         211     // Altura del waveform en píxeles
#define WAVEFORM_WIDTH          399     // Ancho del waveform en píxeles
#define WAVEFORM_COMPONENT_ID   1       // ID del componente waveform
#define WAVEFORM_CHANNEL        0       // Canal del waveform (solo usamos 1)

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
