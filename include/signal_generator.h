#ifndef SIGNAL_GENERATOR_H
#define SIGNAL_GENERATOR_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include "signal_types.h"
#include "ecg_model.h"
#include "emg_model.h"
#include "ppg_model.h"

// ============================================================================
// ESTRUCTURA: Estadísticas de performance
// ============================================================================
struct PerformanceStats {
    uint32_t isrCount;          // Número de interrupciones ISR
    uint32_t isrMaxTime;        // Tiempo máximo de ISR en microsegundos
    uint32_t bufferUnderruns;   // Veces que el buffer quedó vacío
    uint16_t bufferLevel;       // Nivel actual del buffer
    uint32_t freeHeap;          // Memoria heap libre
    uint16_t cpuFreq;           // Frecuencia de CPU en MHz
};

// ============================================================================
// CLASE: SignalGenerator - Gestor de generación de señales
// ============================================================================
class SignalGenerator {
private:
    // Singleton
    static SignalGenerator* instance;
    SignalGenerator();
    
    // Modelos de señales
    ECGModel ecgModel;
    EMGModel emgModel;
    PPGModel ppgModel;
    
    // Estado actual
    SignalData currentSignal;
    uint16_t currentSampleRate;
    
    // FreeRTOS handles
    TaskHandle_t precalcTaskHandle;     // Tarea de pre-cálculo (Core 1)
    TaskHandle_t monitorTaskHandle;     // Tarea de monitoreo (Core 0)
    SemaphoreHandle_t signalMutex;      // Mutex para datos compartidos
    SemaphoreHandle_t bufferSemaphore;  // Semáforo para sincronización
    
    // Timer de hardware
    hw_timer_t* signalTimer;
    
    // Métodos privados
    bool stopSignalInternal();
    void setupTimer(uint16_t sampleRate);
    void prefillBuffer();
    uint8_t generateSample(float deltaTime);
    
    // Tareas FreeRTOS
    static void precalculationTask(void* parameter);
    static void monitoringTask(void* parameter);
    static void IRAM_ATTR timerISR();
    
public:
    // Obtener instancia singleton
    static SignalGenerator* getInstance();
    
    // Inicialización
    bool begin();
    
    // Control de señales
    bool startSignal(SignalType type);
    bool stopSignal();
    bool pauseSignal();
    bool resumeSignal();
    
    // Actualizar parámetros
    bool updateECGParameters(const ECGParameters& params);
    bool updateEMGParameters(const EMGParameters& params);
    bool updatePPGParameters(const PPGParameters& params);
    
    // Obtener información
    SignalData getCurrentSignalData();
    SignalState getState();
    SignalType getCurrentType();
    PerformanceStats getPerformanceStats();
    uint8_t getLastDACValue();  // Último valor enviado al DAC
};

#endif // SIGNAL_GENERATOR_H