/**
 * @file cd4051_mux.h
 * @brief Driver para multiplexor analógico CD4051
 * @version 1.2.0
 * @date 09 Enero 2026
 * 
 * Control del CD4051 para selección de filtro RC por señal.
 * 
 * Configuración de hardware:
 * - ESP32 GPIO25 (DAC1) → LM358 (buffer) → CD4051 COM (pin 3)
 * - ESP32 GPIO32 → CD4051 S0/A (pin 11) - Selector A (LSB)
 * - ESP32 GPIO33 → CD4051 S1/B (pin 10) - Selector B
 * - CD4051 S2/C (pin 9) → GND (fijo en 0, solo usamos canales 0-3)
 * 
 * Canales disponibles (S2=0) con filtro RC (C=1µF):
 * - CH0 (S1=0, S0=0): R=6.8kΩ  → Fc=23.4 Hz  (ECG, F99%=21.6 Hz)
 * - CH1 (S1=0, S0=1): R=1.0kΩ  → Fc=159 Hz   (EMG, F99%=146 Hz)
 * - CH2 (S1=1, S0=0): R=33kΩ   → Fc=4.8 Hz   (PPG, F99%=4.9 Hz)
 * 
 * Red de salida (filtro RC por canal, capacitor compartido C=1µF):
 * 
 *                    ┌─────────────────────┐
 *   CD4051 COM ──────┤  FILTRO RC ACTIVO   ├──────► BNC (+)
 *                    └─────────────────────┘
 * 
 *   CH0 ──[6.8kΩ]──┬── NODO ──[1µF]── GND   (Fc=23.4Hz para ECG)
 *   CH1 ──[1.0kΩ]──┤           │
 *   CH2 ──[33kΩ]───┘           └──► BNC (+)
 * 
 * NOTA: El filtro 1kΩ en CH1 (EMG) se agregó para atenuar ruido
 * de alta frecuencia introducido por el CD4051 (Ron≈80Ω @5V).
 */

#ifndef CD4051_MUX_H
#define CD4051_MUX_H

#include <Arduino.h>
#include "../config.h"

// ============================================================================
// CONFIGURACIÓN DE PINES (desde config.h)
// ============================================================================
// MUX_SELECT_S0 = GPIO32 - Selector A (LSB)
// MUX_SELECT_S1 = GPIO33 - Selector B
// S2 está conectado a GND (siempre 0)

#define MUX_S0_PIN          MUX_SELECT_S0
#define MUX_S1_PIN          MUX_SELECT_S1
#define MUX_ENABLE_PIN      -1      // No usado (ENABLE conectado a GND)

// ============================================================================
// CANALES DEL MULTIPLEXOR (todos con filtro RC, C=1µF compartido)
// ============================================================================
enum class MuxChannel : uint8_t {
    CH0_ECG_6K8     = 0,    // Canal 0: R=6.8kΩ, Fc=23.4 Hz (ECG)
    CH1_EMG_1K0     = 1,    // Canal 1: R=1.0kΩ, Fc=159 Hz  (EMG)
    CH2_PPG_33K     = 2,    // Canal 2: R=33kΩ,  Fc=4.8 Hz  (PPG)
    CH3_UNUSED      = 3,    // Canal 3: No conectado
    CH4_UNUSED      = 4,    // Canal 4: No conectado
    CH5_UNUSED      = 5,    // Canal 5: No conectado
    CH6_UNUSED      = 6,    // Canal 6: No conectado
    CH7_UNUSED      = 7     // Canal 7: No conectado (requiere S2=1)
};

// Alias para tipo de señal (más semántico)
#define MUX_CHANNEL_ECG     MuxChannel::CH0_ECG_6K8   // 6.8kΩ → Fc=23.4Hz
#define MUX_CHANNEL_EMG     MuxChannel::CH1_EMG_1K0   // 1.0kΩ → Fc=159Hz
#define MUX_CHANNEL_PPG     MuxChannel::CH2_PPG_33K   // 33kΩ  → Fc=4.8Hz

// ============================================================================
// NIVELES DE ATENUACIÓN PREDEFINIDOS
// ============================================================================
enum class AttenuationLevel : uint8_t {
    ATTEN_NONE    = 0,    // Sin atenuación (CH1 directo) - Máxima amplitud
    ATTEN_MEDIUM  = 1,    // Atenuación media (CH0: 6.8kΩ)
    ATTEN_HIGH    = 2     // Atenuación alta (CH2: 33kΩ) - Mínima amplitud
};

// ============================================================================
// CLASE CD4051 MULTIPLEXER
// ============================================================================
class CD4051Mux {
public:
    /**
     * @brief Constructor
     */
    CD4051Mux();
    
    /**
     * @brief Inicializa los pines GPIO para control del multiplexor
     * @return true si la inicialización fue exitosa
     */
    bool begin();
    
    /**
     * @brief Selecciona un canal del multiplexor (0-7)
     * @param channel Canal a seleccionar (0-7, pero solo 0-2 están conectados)
     */
    void selectChannel(uint8_t channel);
    
    /**
     * @brief Selecciona un canal usando el enum MuxChannel
     * @param channel Canal a seleccionar
     */
    void selectChannel(MuxChannel channel);
    
    /**
     * @brief Configura el nivel de atenuación
     * @param level Nivel de atenuación deseado
     */
    void setAttenuation(AttenuationLevel level);
    
    /**
     * @brief Obtiene el canal actualmente seleccionado
     * @return Canal actual (0-7)
     */
    uint8_t getCurrentChannel() const { return currentChannel; }
    
    /**
     * @brief Obtiene el nivel de atenuación actual
     * @return Nivel de atenuación
     */
    AttenuationLevel getCurrentAttenuation() const;
    
    /**
     * @brief Obtiene el nombre del canal actual
     * @return String con descripción del canal
     */
    const char* getChannelName() const;
    
    /**
     * @brief Obtiene la frecuencia de corte del filtro RC activo
     * @return Frecuencia de corte en Hz (Fc = 1/(2πRC))
     * @note C = 1µF compartido, R depende del canal seleccionado
     */
    float getCutoffFrequency() const;

private:
    uint8_t currentChannel;
    bool initialized;
    
    /**
     * @brief Aplica los bits de selección a los pines GPIO
     * @param channel Canal (0-7)
     */
    void applyChannelBits(uint8_t channel);
};

// ============================================================================
// INSTANCIA GLOBAL (SINGLETON)
// ============================================================================
extern CD4051Mux mux;

#endif // CD4051_MUX_H
