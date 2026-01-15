/**
 * @file wifi_server.h
 * @brief WiFi Access Point y WebSocket Server para streaming de señales
 * @version 1.0.0
 * @date 18 Diciembre 2025
 * 
 * Permite que múltiples clientes visualicen las señales en tiempo real
 * conectándose al ESP32 como Access Point WiFi.
 */

#ifndef WIFI_SERVER_H
#define WIFI_SERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

// ============================================================================
// CONFIGURACIÓN WiFi AP
// ============================================================================

#define WIFI_SSID           "BioSignalSimulator_Pro"
#define WIFI_PASSWORD       "biosignal123"
#define WIFI_CHANNEL        1
#define WIFI_MAX_CLIENTS    6

// IP Configuration
#define WIFI_LOCAL_IP       IPAddress(192, 168, 4, 1)
#define WIFI_GATEWAY        IPAddress(192, 168, 4, 1)
#define WIFI_SUBNET         IPAddress(255, 255, 255, 0)

// Puertos
#define HTTP_PORT           80
#define WEBSOCKET_PORT      81

// ============================================================================
// CONFIGURACIÓN STREAMING
// ============================================================================

#define WS_SEND_INTERVAL_MS     50      // 20 Hz de envío, con batch de puntos
#define WS_METRICS_INTERVAL_MS  500     // 2 Hz para métricas
#define WS_MAX_QUEUE_SIZE       10      // Buffer de mensajes
#define WS_BATCH_SIZE           10      // Puntos por mensaje (10 pts @ 20Hz = 200 pts/s efectivos)

// ============================================================================
// ESTRUCTURAS DE DATOS
// ============================================================================

/**
 * @brief Datos de señal para transmitir via WebSocket
 */
struct WSSignalData {
    const char* signalType;     // "ECG", "EMG", "PPG"
    const char* condition;      // "NORMAL", "TACHYCARDIA", etc.
    const char* state;          // "RUNNING", "PAUSED", "STOPPED"
    float value;                // Valor actual de la señal (mV)
    float envelope;             // Envelope EMG (mV) - solo para EMG
    uint8_t dacValue;           // Valor DAC (0-255)
    uint32_t timestamp;         // Timestamp en ms
};

/**
 * @brief Métricas de señal para transmitir
 */
struct WSSignalMetrics {
    // ECG
    int hr;                     // Heart Rate (BPM)
    int rr;                     // RR interval (ms)
    float qrs;                  // QRS amplitude (mV)
    float st;                   // ST deviation (mV)
    float hrv;                  // HRV (%)
    
    // EMG
    float rms;                  // RMS value (mV)
    int excitation;             // Excitation level (%)
    int activeUnits;            // Active motor units
    int freq;                   // Median frequency (Hz)
    
    // PPG
    float pi;                   // Perfusion Index (%)
    float dcLevel;              // DC baseline (mV)
    float ac;                   // AC amplitude (mV)
};

// ============================================================================
// CLASE WiFiServer
// ============================================================================

class WiFiServer_BioSim {
public:
    WiFiServer_BioSim();
    
    /**
     * @brief Inicializa WiFi AP, HTTP Server y WebSocket
     * @return true si la inicialización fue exitosa
     */
    bool begin();
    
    /**
     * @brief Detiene todos los servicios
     */
    void stop();
    
    /**
     * @brief Procesa eventos (llamar en loop)
     */
    void loop();
    
    /**
     * @brief Añade un punto al buffer de batch y envía cuando esté lleno
     * @param data Estructura con datos de la señal
     */
    void sendSignalData(const WSSignalData& data);
    
    /**
     * @brief Fuerza el envío del batch actual (flush)
     */
    void flushBatch();
    
    /**
     * @brief Envía métricas a todos los clientes
     * @param metrics Estructura con métricas
     */
    void sendMetrics(const WSSignalMetrics& metrics);
    
    /**
     * @brief Envía cambio de estado (play/pause/stop, cambio de condición)
     * @param signalType Tipo de señal
     * @param condition Condición actual
     * @param state Estado actual
     */
    void sendStateChange(const char* signalType, const char* condition, const char* state);
    
    /**
     * @brief Verifica si hay clientes conectados
     * @return Número de clientes conectados
     */
    uint8_t getClientCount();
    
    /**
     * @brief Verifica si el WiFi está activo
     */
    bool isActive() { return _isActive; }
    
    /**
     * @brief Habilita/deshabilita el streaming
     */
    void setStreamingEnabled(bool enabled) { _streamingEnabled = enabled; }
    bool isStreamingEnabled() { return _streamingEnabled; }

private:
    AsyncWebServer* _server;
    AsyncWebSocket* _ws;
    
    bool _isActive;
    bool _streamingEnabled;
    
    uint32_t _lastSendTime;
    uint32_t _lastMetricsTime;
    
    // Buffer para batch de puntos
    float _batchValues[WS_BATCH_SIZE];
    float _batchEnvelopes[WS_BATCH_SIZE];
    uint8_t _batchCount;
    const char* _batchSignal;
    const char* _batchCondition;
    const char* _batchState;
    
    // Handlers
    void setupRoutes();
    void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, 
                   AwsEventType type, void* arg, uint8_t* data, size_t len);
    
    // Helpers
    String buildDataJson(const WSSignalData& data);
    String buildBatchJson();
    String buildMetricsJson(const WSSignalMetrics& metrics);
    String buildStateJson(const char* signalType, const char* condition, const char* state);
};

// Instancia global
extern WiFiServer_BioSim wifiServer;

#endif // WIFI_SERVER_H
