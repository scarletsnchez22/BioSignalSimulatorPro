/**
 * @file signal_engine_filter.cpp
 * @brief Implementación del Moving Average para suavizar señal DAC
 * @version 1.1.0
 * @date 3 Enero 2026
 * 
 * Moving Average de 8 muestras para suavizar transiciones bruscas.
 * A 4000 Hz, 8 muestras = 2ms de ventana.
 * Elimina picos abruptos sin perder morfología ECG/EMG/PPG.
 */

#include "core/signal_engine.h"

// ============================================================================
// MOVING AVERAGE PARA SUAVIZAR SEÑAL DAC
// ============================================================================

// Aplicar moving average usando buffer circular
float SignalEngine::applyMovingAverage(float input) {
    // Restar valor antiguo de la suma
    maSum -= maBuffer[maIndex];
    
    // Insertar nueva muestra
    maBuffer[maIndex] = input;
    
    // Sumar nuevo valor
    maSum += input;
    
    // Avanzar índice circular
    maIndex = (maIndex + 1) % MA_WINDOW_SIZE;
    
    // Retornar promedio
    return maSum / (float)MA_WINDOW_SIZE;
}

// Resetear estado del moving average (llamar al iniciar nueva señal)
void SignalEngine::resetMovingAverage() {
    for (int i = 0; i < MA_WINDOW_SIZE; i++) {
        maBuffer[i] = 127.5f;  // Inicializar al centro del rango DAC
    }
    maIndex = 0;
    maSum = 127.5f * MA_WINDOW_SIZE;  // Suma inicial
}
