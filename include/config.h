#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============= IDENTIFICACIÓN DE HARDWARE =============
#define HARDWARE_MODEL          "ESP32-WROOM-32"
#define HAS_PSRAM               false    // WROOM-32 NO tiene PSRAM
#define SRAM_SIZE_KB            520      // 520KB SRAM interna
#define FLASH_SIZE_MB           4        // 4MB Flash

// ============= CONFIGURACIÓN DE HARDWARE =============

// Pines DAC (nativos ESP32)
#define DAC_SIGNAL_PIN      25      // GPIO25 - DAC1 (Canal 1)
#define DAC_REFERENCE_PIN   26      // GPIO26 - DAC2 (Canal 2, opcional)

// Puerto Serial para Nextion NX4024T032
#define NEXTION_SERIAL      Serial2
#define NEXTION_RX_PIN      16      // RX2
#define NEXTION_TX_PIN      17      // TX2
#define NEXTION_BAUD        9600

// LED de estado (built-in)
#define LED_STATUS          2       // GPIO2 (LED azul interno)

// LED RGB externo (ánodo común o cátodo común)
// Colores: ROJO=Apagado/Error, VERDE=Encendido/Listo, AZUL=Simulando
#define LED_RGB_ENABLED     true    // Cambiar a false si no usas LED RGB
#define LED_RGB_RED         4       // GPIO4 - Canal Rojo
#define LED_RGB_GREEN       5       // GPIO5 - Canal Verde  
#define LED_RGB_BLUE        18      // GPIO18 - Canal Azul
#define LED_RGB_COMMON_ANODE false  // true=ánodo común, false=cátodo común

// Botones (opcional, para control manual)
#define BTN_START           0       // GPIO0 (BOOT button)
// Agregar más pines según tu diseño

// ============= OPTIMIZACIONES PARA WROOM-32 =============

// Sin PSRAM: usar buffers más pequeños pero eficientes
#define USE_PSRAM               false

// Buffers en IRAM (RAM interna rápida)
#define SIGNAL_BUFFER_IN_IRAM   true

// Tamaño de buffers optimizado para 520KB SRAM
// Buffer principal: 2KB (deja ~500KB para el sistema)
#define SIGNAL_BUFFER_SIZE      2048    // Muestras (2KB)

// Pre-cálculo en bloques pequeños
#define PRECALC_BUFFER_SIZE     512     // Bloques de 512 muestras

// ============= CONFIGURACIÓN DE SEÑALES =============

/*
 * FRECUENCIA DE MUESTREO UNIFICADA: 1000 Hz (1 kHz)
 * ==================================================
 * JUSTIFICACIÓN CIENTÍFICA:
 * 
 * - ECG: Nyquist requiere 2× frecuencia máxima. Componente más alto ~150 Hz
 *   (ruido de alta frecuencia del QRS). 1 kHz ofrece excelente resolución
 *   temporal para detección precisa de ondas P, QRS, T.
 *   Ref: Thakor et al. (1984) "Applications of Adaptive Filtering to ECG Analysis"
 * 
 * - EMG: Contenido frecuencial 10-500 Hz. Nyquist mínimo 1000 Hz.
 *   1 kHz cumple exactamente el criterio de Nyquist para captura completa.
 *   Ref: De Luca, C.J. (1997) "The use of surface EMG in biomechanics"
 * 
 * - PPG: Señal lenta (<30 Hz fundamental). 1 kHz permite excelente
 *   caracterización de la muesca dicrótica y tiempos de subida/bajada.
 *   Ref: Allen (2007) "Photoplethysmography and its application"
 * 
 * VENTAJAS DE 1 kHz:
 * - Mejor resolución temporal (1 ms entre muestras)
 * - EMG cumple Nyquist completo (500 Hz contenido → 1 kHz muestreo)
 * - Timer único sin reconfiguración → menor latencia
 * - Buffer circular sin fragmentación
 * 
 * CARGA COMPUTACIONAL @ 1000 Hz, 8-bit DAC:
 * - ISR cada 1000 µs (1 ms) - sobrado para ESP32 @ 240 MHz
 * - Buffer de 2048 = 2.05 segundos de señal
 * - CPU estimada: ~5% adicional vs 500 Hz (aún <40% total)
 * - RAM: sin impacto (mismo buffer)
 */
#define SAMPLE_RATE_UNIFIED     1000    // Hz - 1 kHz para máxima calidad

// Alias para compatibilidad (todos apuntan al mismo valor)
#define SAMPLE_RATE_ECG         SAMPLE_RATE_UNIFIED
#define SAMPLE_RATE_EMG         SAMPLE_RATE_UNIFIED
#define SAMPLE_RATE_PPG         SAMPLE_RATE_UNIFIED

// Resolución DAC del ESP32
#define DAC_RESOLUTION      8       // 8 bits (0-255)
#define DAC_MAX_VALUE       255
#define DAC_CENTER_VALUE    128

// Rangos de parámetros fisiológicos
#define ECG_BPM_MIN         30
#define ECG_BPM_MAX         200
#define ECG_BPM_DEFAULT     75

#define EMG_FREQ_MIN        10
#define EMG_FREQ_MAX        150
#define EMG_FREQ_DEFAULT    50

#define PPG_BPM_MIN         40
#define PPG_BPM_MAX         180
#define PPG_BPM_DEFAULT     75

// ============= CONFIGURACIÓN DE TAREAS RTOS =============

// Prioridades (0 = más baja, 25 = más alta)
#define TASK_PRIORITY_SIGNAL_GEN    5   // Alta para generación
#define TASK_PRIORITY_PRECALC       4   // Media-alta para pre-cálculo
#define TASK_PRIORITY_UI            2   // Media para UI
#define TASK_PRIORITY_MONITORING    1   // Baja para monitoreo

// Tamaños de stack (bytes) - Ajustados para WROOM-32
#define STACK_SIZE_SIGNAL_GEN       6144    // 6KB
#define STACK_SIZE_PRECALC          4096    // 4KB
#define STACK_SIZE_UI               4096    // 4KB
#define STACK_SIZE_MONITORING       2048    // 2KB

// Asignación de cores
#define CORE_SIGNAL_GEN             1   // Core 1: Generación
#define CORE_PRECALC                1   // Core 1: Pre-cálculo
#define CORE_UI                     0   // Core 0: UI y loop
#define CORE_MONITORING             0   // Core 0: Monitoreo

// ============= OPTIMIZACIONES DE PERFORMANCE =============

// Matemáticas rápidas (trade-off: velocidad vs precisión)
#define FAST_MATH               true

// Tabla lookup para funciones trigonométricas
#define USE_TRIG_LUT            true
#define TRIG_LUT_SIZE           360     // 1 grado resolución

// Cache de cálculos frecuentes
#define CACHE_FREQUENT_CALCS    true

// Desenrollar loops críticos
#define UNROLL_CRITICAL_LOOPS   true

// ============= GESTIÓN DE MEMORIA WROOM-32 =============

// Estrategia de asignación de memoria
// Sin PSRAM: todo en SRAM interna (520KB)
#define HEAP_STRATEGY           "INTERNAL_ONLY"

// Reserva de memoria para sistema
#define SYSTEM_RESERVED_KB      100     // Reservar 100KB para sistema

// Memoria disponible para aplicación
#define APP_AVAILABLE_KB        (SRAM_SIZE_KB - SYSTEM_RESERVED_KB)  // ~420KB

// Límite de memoria por componente
#define MAX_BUFFER_MEMORY_KB    16      // Máximo 16KB en buffers
#define MAX_UI_MEMORY_KB        40      // Máximo 40KB para UI
#define MAX_MODEL_MEMORY_KB     20      // Máximo 20KB para modelos

// ============= WATCHDOG Y MONITOREO =============

// Watchdog timer
#define ENABLE_WATCHDOG         true
#define WATCHDOG_TIMEOUT_MS     5000    // 5 segundos

// Medición de performance
#define ENABLE_PROFILING        true
#define PROFILE_ISR_TIME        true

// Intervalo de reporte de stats
#define STATS_REPORT_INTERVAL_MS    5000    // Cada 5 segundos

// ============= DEBUG Y LOGGING =============

#define DEBUG_MODE              true
#define DEBUG_PERFORMANCE       true
#define DEBUG_MEMORY            true

#if DEBUG_MODE
    #define DEBUG_PRINT(x)          Serial.print(x)
    #define DEBUG_PRINTLN(x)        Serial.println(x)
    #define DEBUG_PRINTF(...)       Serial.printf(__VA_ARGS__)
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
    #define DEBUG_PRINTF(...)
#endif

#if DEBUG_PERFORMANCE
    #define PERF_START(name)        unsigned long _perf_##name = micros()
    #define PERF_END(name)          DEBUG_PRINTF("[PERF] %s: %lu us\n", #name, micros() - _perf_##name)
#else
    #define PERF_START(name)
    #define PERF_END(name)
#endif

#if DEBUG_MEMORY
    #define MEM_CHECK()             DEBUG_PRINTF("[MEM] Free: %d KB\n", ESP.getFreeHeap()/1024)
    #define MEM_CHECKPOINT(name)    DEBUG_PRINTF("[MEM] %s: %d KB free\n", name, ESP.getFreeHeap()/1024)
#else
    #define MEM_CHECK()
    #define MEM_CHECKPOINT(name)
#endif

// ============= CONFIGURACIÓN DE CPU =============

// Frecuencia de CPU (configurado en platformio.ini)
#ifndef CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ
    #define CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ   240
#endif

// ============= CONFIGURACIÓN DE NEXTION =============

// Timeouts
#define NEXTION_TIMEOUT_MS      100
#define NEXTION_RETRY_COUNT     3

// Páginas de la UI (definir según diseño Nextion)
#define NEXTION_PAGE_SPLASH     0
#define NEXTION_PAGE_MENU       1
#define NEXTION_PAGE_ECG        2
#define NEXTION_PAGE_EMG        3
#define NEXTION_PAGE_PPG        4
#define NEXTION_PAGE_SETTINGS   5

// ============= VERSIÓN DEL FIRMWARE =============

#define FIRMWARE_VERSION        "2.0.0-WROOM32"
#define DEVICE_NAME             "BioSignal Simulator Pro"
#define BUILD_DATE              __DATE__
#define BUILD_TIME              __TIME__

// ============= MACROS DE UTILIDAD =============

// Forzar función en IRAM (RAM interna rápida)
#define IRAM_FUNC               IRAM_ATTR

// Forzar variable en DRAM (no Flash)
#define DRAM_VAR                DRAM_ATTR

// Asignación de memoria (siempre interna en WROOM-32)
#define SAFE_MALLOC(size)       malloc(size)
#define SAFE_CALLOC(n, s)       calloc(n, s)
#define SAFE_FREE(ptr)          free(ptr)

// Verificación de memoria disponible
#define CHECK_HEAP(min_kb)      (ESP.getFreeHeap() > ((min_kb) * 1024))

// ============= VALIDACIONES EN COMPILE-TIME =============

// Verificar que los buffers no excedan límites
#if (SIGNAL_BUFFER_SIZE * 1) > (MAX_BUFFER_MEMORY_KB * 1024)
    #error "SIGNAL_BUFFER_SIZE excede MAX_BUFFER_MEMORY_KB"
#endif

// Verificar stack sizes razonables
#if (STACK_SIZE_SIGNAL_GEN + STACK_SIZE_PRECALC + STACK_SIZE_UI + STACK_SIZE_MONITORING) > 40960
    #warning "Stack total > 40KB, podría causar problemas de memoria"
#endif

#endif // CONFIG_H