# BioSignalSimulator Pro - Web Application

**Aplicación web para visualización y control remoto**

---

| Campo | Valor |
|-------|-------|
| **Versión** | 1.0.0 |
| **Estado** | 🚧 Placeholder - En desarrollo |
| **Framework** | React + Vite |
| **Comunicación** | WebSocket |

---

## Descripción

Aplicación web que permite:

- Visualización remota de señales en tiempo real
- Control de parámetros desde cualquier dispositivo
- Exportación de datos (CSV, JSON)
- Interfaz responsive para móvil y desktop

## Arquitectura

```
┌─────────────────┐     WiFi      ┌─────────────────┐
│   ESP32         │◄────────────►│   Web Browser   │
│   Cerebro       │   WebSocket   │   (React App)   │
│                 │               │                 │
│   - AP Mode     │               │   - Charts      │
│   - WebServer   │               │   - Controls    │
│   - WebSocket   │               │   - Export      │
└─────────────────┘               └─────────────────┘
```

## Stack Tecnológico

| Capa | Tecnología |
|------|------------|
| **Frontend** | React 18 |
| **Build** | Vite |
| **Charts** | Chart.js / Recharts |
| **Styling** | TailwindCSS |
| **WebSocket** | Native WebSocket API |

## Estructura Propuesta

```
webapp/
├── README.md           # Este archivo
├── package.json        # Dependencias npm
├── vite.config.js      # Configuración Vite
├── index.html          # Entry point
├── src/
│   ├── main.jsx        # React entry
│   ├── App.jsx         # Componente principal
│   ├── components/
│   │   ├── SignalChart.jsx
│   │   ├── ParameterSlider.jsx
│   │   ├── SignalSelector.jsx
│   │   └── MetricsDisplay.jsx
│   ├── hooks/
│   │   └── useWebSocket.js
│   ├── utils/
│   │   └── dataParser.js
│   └── styles/
│       └── index.css
└── public/
    └── (assets)
```

## Comunicación WebSocket

### Conexión

```javascript
const ws = new WebSocket('ws://192.168.4.1:81');
```

### Mensajes del Servidor (ESP32 → Web)

```json
{
  "type": "signal",
  "signal": "ecg",
  "value": 0.85,
  "metrics": {
    "hr": 72,
    "rr": 833,
    "st": 0.0
  }
}
```

### Comandos del Cliente (Web → ESP32)

```json
{
  "cmd": "setSignal",
  "value": "ECG"
}

{
  "cmd": "setParam",
  "param": "hr",
  "value": 75
}
```

## TODO

- [ ] Inicializar proyecto React + Vite
- [ ] Implementar conexión WebSocket
- [ ] Crear componente de gráfico en tiempo real
- [ ] Crear controles de parámetros
- [ ] Implementar exportación de datos
- [ ] Diseñar UI responsive
- [ ] Implementar servidor WebSocket en ESP32 Cerebro

## Desarrollo

```bash
cd webapp
npm install
npm run dev
```

## Build para Producción

```bash
npm run build
# Los archivos se generan en dist/
# Estos se pueden servir desde el ESP32 (SPIFFS/LittleFS)
```

## Notas

Esta webapp está actualmente en estado **placeholder**. La implementación requiere:

1. Implementar servidor WebSocket en el firmware Cerebro
2. Crear la aplicación React
3. Subir los archivos compilados al ESP32 (SPIFFS)
