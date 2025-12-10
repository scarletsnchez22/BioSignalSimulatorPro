/**
 * @file serial_handler.cpp
 * @brief Implementación del manejador serial
 * @version 1.0.0
 */

#include "comm/serial_handler.h"
#include "config.h"

// ============================================================================
// CONSTRUCTOR
// ============================================================================
SerialHandler::SerialHandler(HardwareSerial& serialPort) : serial(serialPort) {
    commandCallback = nullptr;
    streamingEnabled = false;
    lastStreamTime = 0;
    rxIndex = 0;
}

// ============================================================================
// INICIALIZACIÓN
// ============================================================================
void SerialHandler::begin(unsigned long baud) {
    // Serial ya inicializado en main.cpp
}

// ============================================================================
// PROCESAR DATOS
// ============================================================================
void SerialHandler::process() {
    while (serial.available()) {
        char c = serial.read();
        
        // Modo texto simple para debug
        if (c == 'h' || c == 'H') {
            printHelp();
        } else if (c == 'i' || c == 'I') {
            printSystemInfo();
        }
        
        // TODO: Implementar protocolo binario completo
    }
}

// ============================================================================
// STREAMING
// ============================================================================
void SerialHandler::startStreaming() {
    streamingEnabled = true;
    serial.println("[Stream] Iniciado");
}

void SerialHandler::stopStreaming() {
    streamingEnabled = false;
    serial.println("[Stream] Detenido");
}

void SerialHandler::streamSample(uint8_t dacValue, uint16_t flags) {
    if (!streamingEnabled) return;
    
    // Formato compacto: [0xBB] [sample] [flags_high] [flags_low]
    serial.write(0xBB);
    serial.write(dacValue);
    serial.write((uint8_t)(flags >> 8));
    serial.write((uint8_t)(flags & 0xFF));
}

// ============================================================================
// ENVIAR PAQUETE
// ============================================================================
void SerialHandler::sendPacket(uint8_t cmd, const uint8_t* data, uint16_t len) {
    SerialPacket packet;
    packet.header = CMD_HEADER;
    packet.cmd = cmd;
    packet.signalType = 0;
    packet.dataLen = len;
    
    if (len > 0 && data != nullptr) {
        memcpy(packet.data, data, min((size_t)len, sizeof(packet.data)));
    }
    
    packet.checksum = calculateChecksum(packet);
    
    serial.write((uint8_t*)&packet, 5 + len + 1);
}

uint8_t SerialHandler::calculateChecksum(const SerialPacket& packet) {
    uint8_t checksum = packet.header ^ packet.cmd ^ packet.signalType;
    checksum ^= (packet.dataLen >> 8) ^ (packet.dataLen & 0xFF);
    for (uint16_t i = 0; i < packet.dataLen; i++) {
        checksum ^= packet.data[i];
    }
    return checksum;
}

void SerialHandler::sendAck(uint8_t cmd) {
    uint8_t data[1] = {cmd};
    sendPacket(CMD_ACK, data, 1);
}

void SerialHandler::sendError(uint8_t errorCode) {
    uint8_t data[1] = {errorCode};
    sendPacket(CMD_ERROR, data, 1);
}

// ============================================================================
// DEBUG
// ============================================================================
void SerialHandler::printHelp() {
    serial.println("\n╔════════════════════════════════════════════════════════╗");
    serial.println("║     " DEVICE_NAME " v" FIRMWARE_VERSION "    ║");
    serial.println("╠════════════════════════════════════════════════════════╣");
    serial.println("║  COMANDOS:                                             ║");
    serial.println("║    h - Esta ayuda                                      ║");
    serial.println("║    i - Información del sistema                         ║");
    serial.println("║                                                        ║");
    serial.println("║  Use la pantalla Nextion para control interactivo      ║");
    serial.println("╚════════════════════════════════════════════════════════╝\n");
}

void SerialHandler::printSystemInfo() {
    serial.println("\n--- Información del Sistema ---");
    serial.printf("Firmware: %s\n", FIRMWARE_VERSION);
    serial.printf("Hardware: %s\n", HARDWARE_MODEL);
    serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
    serial.printf("CPU Freq: %d MHz\n", ESP.getCpuFreqMHz());
    serial.printf("Sample Rate: %d Hz\n", SAMPLE_RATE_HZ);
    serial.printf("Buffer Size: %d samples\n", SIGNAL_BUFFER_SIZE);
    serial.println("--------------------------------\n");
}

// ============================================================================
// CALLBACK
// ============================================================================
void SerialHandler::setCommandCallback(SerialCommandCallback callback) {
    commandCallback = callback;
}
