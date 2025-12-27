/**
 * @file wifi_server.cpp
 * @brief Implementación del WiFi AP y WebSocket Server
 * @version 1.0.0
 * @date 18 Diciembre 2025
 */

#include "comm/wifi_server.h"

// Instancia global
WiFiServer_BioSim wifiServer;

// ============================================================================
// CONSTRUCTOR
// ============================================================================

WiFiServer_BioSim::WiFiServer_BioSim() 
    : _server(nullptr)
    , _ws(nullptr)
    , _isActive(false)
    , _streamingEnabled(false)
    , _lastSendTime(0)
    , _lastMetricsTime(0)
{
}

// ============================================================================
// INICIALIZACIÓN
// ============================================================================

bool WiFiServer_BioSim::begin() {
    Serial.println("[WiFi] Iniciando Access Point...");
    
    // Inicializar SPIFFS para archivos web
    if (!SPIFFS.begin(true)) {
        Serial.println("[WiFi] ERROR: No se pudo montar SPIFFS");
        return false;
    }
    Serial.println("[WiFi] SPIFFS montado correctamente");
    
    // Configurar WiFi como Access Point
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(WIFI_LOCAL_IP, WIFI_GATEWAY, WIFI_SUBNET);
    
    if (!WiFi.softAP(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL, false, WIFI_MAX_CLIENTS)) {
        Serial.println("[WiFi] ERROR: No se pudo crear AP");
        return false;
    }
    
    Serial.printf("[WiFi] AP creado: %s\n", WIFI_SSID);
    Serial.printf("[WiFi] IP: %s\n", WiFi.softAPIP().toString().c_str());
    
    // Crear servidor HTTP
    _server = new AsyncWebServer(HTTP_PORT);
    
    // Crear WebSocket
    _ws = new AsyncWebSocket("/ws");
    
    // Configurar evento WebSocket
    _ws->onEvent([this](AsyncWebSocket* server, AsyncWebSocketClient* client, 
                        AwsEventType type, void* arg, uint8_t* data, size_t len) {
        this->onWsEvent(server, client, type, arg, data, len);
    });
    
    // Agregar WebSocket al servidor
    _server->addHandler(_ws);
    
    // Configurar rutas HTTP
    setupRoutes();
    
    // Iniciar servidor
    _server->begin();
    Serial.printf("[WiFi] Servidor HTTP iniciado en puerto %d\n", HTTP_PORT);
    Serial.println("[WiFi] WebSocket disponible en /ws");
    
    _isActive = true;
    return true;
}

// ============================================================================
// CONFIGURACIÓN DE RUTAS HTTP
// ============================================================================

void WiFiServer_BioSim::setupRoutes() {
    // Página principal
    _server->on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(SPIFFS, "/index.html", "text/html");
    });
    
    // Archivos estáticos
    _server->on("/app.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(SPIFFS, "/app.js", "application/javascript");
    });
    
    _server->on("/styles.css", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(SPIFFS, "/styles.css", "text/css");
    });
    
    // API de estado
    _server->on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        StaticJsonDocument<256> doc;
        doc["device"] = "BioSignalSimulator Pro";
        doc["version"] = "1.0.0";
        doc["clients"] = _ws->count();
        doc["streaming"] = _streamingEnabled;
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    // 404
    _server->onNotFound([](AsyncWebServerRequest* request) {
        request->send(404, "text/plain", "Not Found");
    });
}

// ============================================================================
// EVENTOS WEBSOCKET
// ============================================================================

void WiFiServer_BioSim::onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                                   AwsEventType type, void* arg, uint8_t* data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("[WS] Cliente #%u conectado desde %s\n", 
                         client->id(), client->remoteIP().toString().c_str());
            // Enviar mensaje de bienvenida
            {
                StaticJsonDocument<128> doc;
                doc["type"] = "welcome";
                doc["message"] = "Conectado a BioSignalSimulator Pro";
                doc["clientId"] = client->id();
                String msg;
                serializeJson(doc, msg);
                client->text(msg);
            }
            break;
            
        case WS_EVT_DISCONNECT:
            Serial.printf("[WS] Cliente #%u desconectado\n", client->id());
            break;
            
        case WS_EVT_ERROR:
            Serial.printf("[WS] Error en cliente #%u\n", client->id());
            break;
            
        case WS_EVT_DATA:
            // Los clientes solo visualizan, no envían comandos
            break;
            
        default:
            break;
    }
}

// ============================================================================
// ENVÍO DE DATOS
// ============================================================================

void WiFiServer_BioSim::sendSignalData(const WSSignalData& data) {
    if (!_isActive || !_streamingEnabled || _ws->count() == 0) return;
    
    uint32_t now = millis();
    if (now - _lastSendTime < WS_SEND_INTERVAL_MS) return;
    _lastSendTime = now;
    
    String json = buildDataJson(data);
    _ws->textAll(json);
}

void WiFiServer_BioSim::sendMetrics(const WSSignalMetrics& metrics) {
    if (!_isActive || _ws->count() == 0) return;
    
    uint32_t now = millis();
    if (now - _lastMetricsTime < WS_METRICS_INTERVAL_MS) return;
    _lastMetricsTime = now;
    
    String json = buildMetricsJson(metrics);
    _ws->textAll(json);
}

void WiFiServer_BioSim::sendStateChange(const char* signalType, const char* condition, const char* state) {
    if (!_isActive || _ws->count() == 0) return;
    
    String json = buildStateJson(signalType, condition, state);
    _ws->textAll(json);
}

// ============================================================================
// CONSTRUCCIÓN DE JSON
// ============================================================================

String WiFiServer_BioSim::buildDataJson(const WSSignalData& data) {
    StaticJsonDocument<256> doc;
    doc["type"] = "data";
    doc["signal"] = data.signalType;
    doc["t"] = data.timestamp;
    doc["v"] = data.value;
    doc["dac"] = data.dacValue;
    
    String json;
    serializeJson(doc, json);
    return json;
}

String WiFiServer_BioSim::buildMetricsJson(const WSSignalMetrics& metrics) {
    StaticJsonDocument<384> doc;
    doc["type"] = "metrics";
    
    JsonObject m = doc.createNestedObject("m");
    m["hr"] = metrics.hr;
    m["rr"] = metrics.rr;
    m["qrs"] = metrics.qrs;
    m["st"] = metrics.st;
    m["rms"] = metrics.rms;
    m["exc"] = metrics.excitation;
    m["mus"] = metrics.activeUnits;
    m["pi"] = metrics.pi;
    m["dc"] = metrics.dcLevel;
    
    String json;
    serializeJson(doc, json);
    return json;
}

String WiFiServer_BioSim::buildStateJson(const char* signalType, const char* condition, const char* state) {
    StaticJsonDocument<192> doc;
    doc["type"] = "state";
    doc["signal"] = signalType;
    doc["condition"] = condition;
    doc["state"] = state;
    
    String json;
    serializeJson(doc, json);
    return json;
}

// ============================================================================
// UTILIDADES
// ============================================================================

uint8_t WiFiServer_BioSim::getClientCount() {
    return _ws ? _ws->count() : 0;
}

void WiFiServer_BioSim::loop() {
    if (_ws) {
        _ws->cleanupClients();
    }
}

void WiFiServer_BioSim::stop() {
    if (_ws) {
        _ws->closeAll();
    }
    WiFi.softAPdisconnect(true);
    _isActive = false;
    Serial.println("[WiFi] Servidor detenido");
}
