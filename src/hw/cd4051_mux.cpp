/**
 * @file cd4051_mux.cpp
 * @brief Implementación del driver para multiplexor analógico CD4051
 * @version 1.0.0
 * @date 08 Enero 2026
 */

#include "hw/cd4051_mux.h"

// ============================================================================
// INSTANCIA GLOBAL
// ============================================================================
CD4051Mux mux;

// ============================================================================
// CONSTRUCTOR
// ============================================================================
CD4051Mux::CD4051Mux() : currentChannel(1), initialized(false) {
    // Canal 1 (directo) por defecto - sin atenuación
}

// ============================================================================
// INICIALIZACIÓN
// ============================================================================
bool CD4051Mux::begin() {
    // Configurar pines como salidas
    pinMode(MUX_S0_PIN, OUTPUT);
    pinMode(MUX_S1_PIN, OUTPUT);
    
    // Si hay pin de enable, configurarlo
    if (MUX_ENABLE_PIN >= 0) {
        pinMode(MUX_ENABLE_PIN, OUTPUT);
        digitalWrite(MUX_ENABLE_PIN, LOW);  // Enable activo bajo
    }
    
    // Seleccionar canal por defecto (CH1 = directo, sin atenuación)
    selectChannel(MuxChannel::CH1_DIRECT);
    
    initialized = true;
    Serial.println("[CD4051] Multiplexor inicializado");
    Serial.printf("[CD4051] Pines: S0=GPIO%d, S1=GPIO%d\n", MUX_S0_PIN, MUX_S1_PIN);
    Serial.printf("[CD4051] Canal inicial: %s\n", getChannelName());
    
    return true;
}

// ============================================================================
// SELECCIÓN DE CANAL
// ============================================================================
void CD4051Mux::selectChannel(uint8_t channel) {
    if (channel > 7) {
        channel = 7;  // Limitar a rango válido
    }
    
    // Solo canales 0-2 están conectados físicamente
    if (channel > 2) {
        Serial.printf("[CD4051] ADVERTENCIA: Canal %d no está conectado\n", channel);
    }
    
    currentChannel = channel;
    applyChannelBits(channel);
    
    Serial.printf("[CD4051] Canal seleccionado: %d (%s)\n", channel, getChannelName());
}

void CD4051Mux::selectChannel(MuxChannel channel) {
    selectChannel(static_cast<uint8_t>(channel));
}

// ============================================================================
// CONFIGURACIÓN DE ATENUACIÓN
// ============================================================================
void CD4051Mux::setAttenuation(AttenuationLevel level) {
    switch (level) {
        case AttenuationLevel::ATTEN_NONE:
            selectChannel(MuxChannel::CH1_DIRECT);
            break;
        case AttenuationLevel::ATTEN_MEDIUM:
            selectChannel(MuxChannel::CH0_6K8_OHM);
            break;
        case AttenuationLevel::ATTEN_HIGH:
            selectChannel(MuxChannel::CH2_33K_OHM);
            break;
    }
}

AttenuationLevel CD4051Mux::getCurrentAttenuation() const {
    switch (currentChannel) {
        case 0: return AttenuationLevel::ATTEN_MEDIUM;
        case 1: return AttenuationLevel::ATTEN_NONE;
        case 2: return AttenuationLevel::ATTEN_HIGH;
        default: return AttenuationLevel::ATTEN_NONE;
    }
}

// ============================================================================
// INFORMACIÓN DEL CANAL
// ============================================================================
const char* CD4051Mux::getChannelName() const {
    switch (currentChannel) {
        case 0: return "CH0 (6.8k ohm - Atten Media)";
        case 1: return "CH1 (Directo - Sin Atten)";
        case 2: return "CH2 (33k ohm - Atten Alta)";
        case 3: return "CH3 (No conectado)";
        case 4: return "CH4 (No conectado)";
        case 5: return "CH5 (No conectado)";
        case 6: return "CH6 (No conectado)";
        case 7: return "CH7 (No conectado)";
        default: return "Desconocido";
    }
}

float CD4051Mux::getAttenuationFactor() const {
    // Factores aproximados basados en divisor resistivo
    // Dependen de la impedancia de entrada del BNC/osciloscopio
    // Asumiendo impedancia alta (1M ohm típico de osciloscopio)
    switch (currentChannel) {
        case 0: return 0.6f;   // 6.8k ohm - atenuación ~40%
        case 1: return 1.0f;   // Directo - sin atenuación
        case 2: return 0.2f;   // 33k ohm - atenuación ~80%
        default: return 0.0f;  // Canales no conectados
    }
}

// ============================================================================
// APLICAR BITS DE SELECCIÓN
// ============================================================================
void CD4051Mux::applyChannelBits(uint8_t channel) {
    // CD4051 usa 3 bits de selección (A, B, C)
    // S2 (C) está conectado a GND, así que solo usamos S0 (A) y S1 (B)
    // 
    // Canal | S2(C) | S1(B) | S0(A)
    // ------+-------+-------+------
    //   0   |   0   |   0   |   0
    //   1   |   0   |   0   |   1
    //   2   |   0   |   1   |   0
    //   3   |   0   |   1   |   1
    //   4   |   1   |   0   |   0  (no accesible, S2=GND)
    //   5   |   1   |   0   |   1  (no accesible)
    //   6   |   1   |   1   |   0  (no accesible)
    //   7   |   1   |   1   |   1  (no accesible)
    
    uint8_t s0 = (channel >> 0) & 0x01;  // Bit 0 → S0 (GPIO26)
    uint8_t s1 = (channel >> 1) & 0x01;  // Bit 1 → S1 (GPIO27)
    // S2 = GND (siempre 0)
    
    digitalWrite(MUX_S0_PIN, s0);
    digitalWrite(MUX_S1_PIN, s1);
}
