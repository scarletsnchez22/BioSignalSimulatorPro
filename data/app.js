/**
 * BioSimulator Pro - Web App JavaScript
 * Version 1.0.0 - 18 Diciembre 2025
 * 
 * Visualización en tiempo real de señales biomédicas via WebSocket
 */

// ============================================================================
// CONFIGURACIÓN
// ============================================================================

const CONFIG = {
    wsUrl: `ws://${window.location.hostname}/ws`,
    reconnectInterval: 3000,
    bufferSize: 700,           // Puntos en pantalla
    defaultZoom: 100,
    minZoom: 25,
    maxZoom: 400,
    zoomStep: 25,
    gridDivisionsX: 7,
    gridDivisionsY: 10,
    signalColor: '#00ff88',
    gridColor: '#2a4a6a',
    bgColor: '#0a0a15'
};

// ============================================================================
// ESTADO DE LA APLICACIÓN
// ============================================================================

const state = {
    ws: null,
    connected: false,
    paused: false,
    zoom: CONFIG.defaultZoom,
    
    // Datos de señal
    signalType: '--',
    condition: '--',
    deviceState: '--',
    
    // Buffer de datos
    dataBuffer: [],
    
    // Métricas
    metrics: {
        hr: '--', rr: '--', qrs: '--', st: '--',
        rms: '--', exc: '--', mus: '--',
        pi: '--', spo2: '--', dc: '--'
    },
    
    // Estadísticas
    dataCount: 0,
    lastDataTime: 0
};

// ============================================================================
// ELEMENTOS DOM
// ============================================================================

const elements = {
    canvas: null,
    ctx: null,
    connectionStatus: null,
    connectionText: null,
    signalType: null,
    signalCondition: null,
    signalState: null,
    scaleY: null,
    scaleX: null,
    metricHR: null,
    metricRR: null,
    metricQRS: null,
    metricST: null,
    btnZoomIn: null,
    btnZoomOut: null,
    zoomLevel: null,
    btnPause: null,
    btnDownload: null,
    btnScreenshot: null,
    clientCount: null,
    dataRate: null
};

// ============================================================================
// INICIALIZACIÓN
// ============================================================================

document.addEventListener('DOMContentLoaded', () => {
    initElements();
    initCanvas();
    initEventListeners();
    connectWebSocket();
    startRenderLoop();
    startStatsUpdate();
});

function initElements() {
    elements.canvas = document.getElementById('waveformCanvas');
    elements.ctx = elements.canvas.getContext('2d');
    elements.connectionStatus = document.getElementById('connectionStatus');
    elements.connectionText = document.getElementById('connectionText');
    elements.signalType = document.getElementById('signalType');
    elements.signalCondition = document.getElementById('signalCondition');
    elements.signalState = document.getElementById('signalState');
    elements.scaleY = document.getElementById('scaleY');
    elements.scaleX = document.getElementById('scaleX');
    elements.metricHR = document.getElementById('metricHR');
    elements.metricRR = document.getElementById('metricRR');
    elements.metricQRS = document.getElementById('metricQRS');
    elements.metricST = document.getElementById('metricST');
    elements.btnZoomIn = document.getElementById('btnZoomIn');
    elements.btnZoomOut = document.getElementById('btnZoomOut');
    elements.zoomLevel = document.getElementById('zoomLevel');
    elements.btnPause = document.getElementById('btnPause');
    elements.btnDownload = document.getElementById('btnDownload');
    elements.btnScreenshot = document.getElementById('btnScreenshot');
    elements.clientCount = document.getElementById('clientCount');
    elements.dataRate = document.getElementById('dataRate');
}

function initCanvas() {
    resizeCanvas();
    window.addEventListener('resize', resizeCanvas);
}

function resizeCanvas() {
    const container = elements.canvas.parentElement;
    const rect = container.getBoundingClientRect();
    
    // Usar devicePixelRatio para pantallas de alta densidad
    const dpr = window.devicePixelRatio || 1;
    
    elements.canvas.width = rect.width * dpr;
    elements.canvas.height = (rect.height - 30) * dpr;
    
    elements.canvas.style.width = rect.width + 'px';
    elements.canvas.style.height = (rect.height - 30) + 'px';
    
    elements.ctx.scale(dpr, dpr);
}

function initEventListeners() {
    elements.btnZoomIn.addEventListener('click', () => adjustZoom(CONFIG.zoomStep));
    elements.btnZoomOut.addEventListener('click', () => adjustZoom(-CONFIG.zoomStep));
    elements.btnPause.addEventListener('click', togglePause);
    elements.btnDownload.addEventListener('click', downloadCSV);
    elements.btnScreenshot.addEventListener('click', takeScreenshot);
    
    // Touch events para zoom en móvil
    let touchStartY = 0;
    elements.canvas.addEventListener('touchstart', (e) => {
        touchStartY = e.touches[0].clientY;
    });
    
    elements.canvas.addEventListener('touchmove', (e) => {
        const deltaY = touchStartY - e.touches[0].clientY;
        if (Math.abs(deltaY) > 30) {
            adjustZoom(deltaY > 0 ? CONFIG.zoomStep : -CONFIG.zoomStep);
            touchStartY = e.touches[0].clientY;
        }
    });
}

// ============================================================================
// WEBSOCKET
// ============================================================================

function connectWebSocket() {
    console.log('[WS] Conectando a', CONFIG.wsUrl);
    
    state.ws = new WebSocket(CONFIG.wsUrl);
    
    state.ws.onopen = () => {
        console.log('[WS] Conectado');
        state.connected = true;
        updateConnectionUI(true);
    };
    
    state.ws.onclose = () => {
        console.log('[WS] Desconectado');
        state.connected = false;
        updateConnectionUI(false);
        
        // Reconectar automáticamente
        setTimeout(connectWebSocket, CONFIG.reconnectInterval);
    };
    
    state.ws.onerror = (error) => {
        console.error('[WS] Error:', error);
    };
    
    state.ws.onmessage = (event) => {
        handleMessage(JSON.parse(event.data));
    };
}

function handleMessage(msg) {
    switch (msg.type) {
        case 'welcome':
            console.log('[WS] Bienvenida:', msg.message);
            break;
            
        case 'data':
            handleDataMessage(msg);
            break;
            
        case 'metrics':
            handleMetricsMessage(msg);
            break;
            
        case 'state':
            handleStateMessage(msg);
            break;
    }
}

function handleDataMessage(msg) {
    if (state.paused) return;
    
    // Agregar al buffer
    state.dataBuffer.push({
        t: msg.t,
        v: msg.v,
        dac: msg.dac
    });
    
    // Limitar tamaño del buffer
    while (state.dataBuffer.length > CONFIG.bufferSize) {
        state.dataBuffer.shift();
    }
    
    // Actualizar estadísticas
    state.dataCount++;
    state.lastDataTime = Date.now();
    
    // Actualizar tipo de señal si viene en el mensaje
    if (msg.signal && msg.signal !== state.signalType) {
        state.signalType = msg.signal;
        elements.signalType.textContent = msg.signal;
    }
}

function handleMetricsMessage(msg) {
    const m = msg.m;
    state.metrics = { ...state.metrics, ...m };
    
    // Actualizar UI según tipo de señal
    if (state.signalType === 'ECG') {
        document.getElementById('metricHR').textContent = m.hr !== undefined ? m.hr : '--';
        document.getElementById('metricRR').textContent = m.rr !== undefined ? m.rr : '--';
        document.getElementById('metricQRS').textContent = m.qrs !== undefined ? m.qrs.toFixed(2) : '--';
        document.getElementById('metricST').textContent = m.st !== undefined ? m.st.toFixed(2) : '--';
    } else if (state.signalType === 'EMG') {
        document.getElementById('metricRMS').textContent = m.rms !== undefined ? m.rms.toFixed(2) : '--';
        document.getElementById('metricEXC').textContent = m.exc !== undefined ? m.exc : '--';
        document.getElementById('metricMUs').textContent = m.mus !== undefined ? m.mus : '--';
    } else if (state.signalType === 'PPG') {
        document.getElementById('metricHRppg').textContent = m.hr !== undefined ? m.hr : '--';
        document.getElementById('metricSpO2').textContent = m.spo2 !== undefined ? m.spo2 : '--';
        document.getElementById('metricPI').textContent = m.pi !== undefined ? m.pi.toFixed(1) : '--';
        document.getElementById('metricRRppg').textContent = m.rr !== undefined ? m.rr : '--';
    }
}

function handleStateMessage(msg) {
    state.signalType = msg.signal || state.signalType;
    state.condition = msg.condition || state.condition;
    state.deviceState = msg.state || state.deviceState;
    
    elements.signalType.textContent = state.signalType;
    elements.signalCondition.textContent = state.condition;
    elements.signalState.textContent = state.deviceState;
    
    // Cambiar panel de métricas según tipo de señal
    document.getElementById('metricsECG').style.display = state.signalType === 'ECG' ? 'grid' : 'none';
    document.getElementById('metricsEMG').style.display = state.signalType === 'EMG' ? 'grid' : 'none';
    document.getElementById('metricsPPG').style.display = state.signalType === 'PPG' ? 'grid' : 'none';
    
    // Actualizar clase CSS del estado
    elements.signalState.className = '';
    if (state.deviceState === 'RUNNING') {
        elements.signalState.classList.add('running');
    } else if (state.deviceState === 'PAUSED') {
        elements.signalState.classList.add('paused');
    } else {
        elements.signalState.classList.add('stopped');
    }
}

function updateConnectionUI(connected) {
    elements.connectionStatus.className = 'status-dot ' + (connected ? 'connected' : 'disconnected');
    elements.connectionText.textContent = connected ? 'Conectado' : 'Desconectado';
}

// ============================================================================
// RENDERIZADO
// ============================================================================

function startRenderLoop() {
    requestAnimationFrame(render);
}

function render() {
    drawWaveform();
    requestAnimationFrame(render);
}

function drawWaveform() {
    const canvas = elements.canvas;
    const ctx = elements.ctx;
    const width = canvas.width / (window.devicePixelRatio || 1);
    const height = canvas.height / (window.devicePixelRatio || 1);
    
    // Limpiar canvas
    ctx.fillStyle = CONFIG.bgColor;
    ctx.fillRect(0, 0, width, height);
    
    // Dibujar cuadrícula
    drawGrid(ctx, width, height);
    
    // Dibujar señal
    if (state.dataBuffer.length > 1) {
        drawSignal(ctx, width, height);
    }
    
    // Actualizar escalas
    updateScaleLabels();
}

function drawGrid(ctx, width, height) {
    ctx.strokeStyle = CONFIG.gridColor;
    ctx.lineWidth = 0.5;
    
    // Líneas verticales
    const stepX = width / CONFIG.gridDivisionsX;
    for (let i = 0; i <= CONFIG.gridDivisionsX; i++) {
        ctx.beginPath();
        ctx.moveTo(i * stepX, 0);
        ctx.lineTo(i * stepX, height);
        ctx.stroke();
    }
    
    // Líneas horizontales
    const stepY = height / CONFIG.gridDivisionsY;
    for (let i = 0; i <= CONFIG.gridDivisionsY; i++) {
        ctx.beginPath();
        ctx.moveTo(0, i * stepY);
        ctx.lineTo(width, i * stepY);
        ctx.stroke();
    }
    
    // Línea central más gruesa
    ctx.strokeStyle = '#3a5a7a';
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.moveTo(0, height / 2);
    ctx.lineTo(width, height / 2);
    ctx.stroke();
}

function drawSignal(ctx, width, height) {
    const data = state.dataBuffer;
    const zoomFactor = state.zoom / 100;
    
    ctx.strokeStyle = CONFIG.signalColor;
    ctx.lineWidth = 1.5;
    ctx.beginPath();
    
    const margin = 10;
    const drawHeight = height - 2 * margin;
    const drawWidth = width - 2 * margin;
    
    for (let i = 0; i < data.length; i++) {
        // Mapear DAC (0-255) a coordenadas de canvas
        // Aplicar zoom respecto al centro
        const normalized = (data[i].dac - 127.5) / 127.5;  // -1 a 1
        const zoomed = normalized * zoomFactor;
        const y = margin + drawHeight / 2 - (zoomed * drawHeight / 2);
        
        const x = margin + (i / CONFIG.bufferSize) * drawWidth;
        
        if (i === 0) {
            ctx.moveTo(x, y);
        } else {
            ctx.lineTo(x, y);
        }
    }
    
    ctx.stroke();
}

function updateScaleLabels() {
    // Calcular mV/div basado en zoom y tipo de señal
    let baseScale = 0.2;  // ECG default
    if (state.signalType === 'EMG') baseScale = 1.0;
    if (state.signalType === 'PPG') baseScale = 20.0;
    
    const mvDiv = (baseScale / (state.zoom / 100)).toFixed(2);
    elements.scaleY.textContent = `${mvDiv} mV/div`;
    
    // ms/div basado en buffer y frecuencia
    const msDiv = state.signalType === 'ECG' ? 500 : 1000;
    elements.scaleX.textContent = `${msDiv} ms/div`;
}

// ============================================================================
// CONTROLES
// ============================================================================

function adjustZoom(delta) {
    state.zoom = Math.max(CONFIG.minZoom, Math.min(CONFIG.maxZoom, state.zoom + delta));
    elements.zoomLevel.textContent = state.zoom + '%';
}

function togglePause() {
    state.paused = !state.paused;
    
    if (state.paused) {
        elements.btnPause.textContent = '▶ Reanudar';
        elements.btnPause.classList.add('paused');
    } else {
        elements.btnPause.textContent = '⏸ Pausar';
        elements.btnPause.classList.remove('paused');
    }
}

function downloadCSV() {
    if (state.dataBuffer.length === 0) {
        alert('No hay datos para descargar');
        return;
    }
    
    // Crear contenido CSV
    let csv = 'timestamp,value,dac\n';
    state.dataBuffer.forEach(d => {
        csv += `${d.t},${d.v},${d.dac}\n`;
    });
    
    // Descargar
    const blob = new Blob([csv], { type: 'text/csv' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `biosimulator_${state.signalType}_${Date.now()}.csv`;
    a.click();
    URL.revokeObjectURL(url);
}

function takeScreenshot() {
    const canvas = elements.canvas;
    const url = canvas.toDataURL('image/png');
    const a = document.createElement('a');
    a.href = url;
    a.download = `biosimulator_${state.signalType}_${Date.now()}.png`;
    a.click();
}

// ============================================================================
// ESTADÍSTICAS
// ============================================================================

function startStatsUpdate() {
    setInterval(updateStats, 1000);
}

function updateStats() {
    // Calcular tasa de datos
    const rate = state.dataCount;
    state.dataCount = 0;
    elements.dataRate.textContent = `${rate} pts/s`;
    
    // Actualizar contador de clientes (si disponible)
    if (state.connected) {
        fetch('/api/status')
            .then(r => r.json())
            .then(data => {
                elements.clientCount.textContent = `${data.clients} cliente${data.clients !== 1 ? 's' : ''}`;
            })
            .catch(() => {});
    }
}
