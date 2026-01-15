/**
 * BioSimulator Pro - Web App Profesional
 * Diseño responsive con controles de navegación
 */

// ============================================================================
// CONFIGURACIÓN
// ============================================================================
const CFG = {
    wsUrl: `ws://${window.location.hostname}/ws`,
    reconnectMs: 5000,      // Reconexión cada 5s (menos agresivo)
    pingInterval: 30000,    // Ping cada 30s (solo si no hay datos)
    bufSize: 700,
    gridX: 10,              // 10 divisiones horizontales (tiempo)
    gridY: 8,               // 8 divisiones verticales (amplitud)
    colors: {
        bg: '#041020',      // Azul muy oscuro
        grid: '#0a2540',    // Grid azul oscuro
        gridMaj: '#144070', // Grid mayor azul medio
        gridMin: '#082030', // Grid menor
        ch1: '#00ff88',     // ECG verde clínico
        ch2: '#ff9f43',     // EMG/Envelope naranja
        ch3: '#ff6b9d'      // PPG rosa/rojo
    },
    ranges: {
        ECG: { min: -0.5, max: 1.5, div: 0.25, unit: 'mV' },
        EMG: { min: -5.0, max: 5.0, div: 1.25, unit: 'mV' },
        PPG: { min: -100, max: 100, div: 25, unit: 'mV' }
    },
    timeWin: { ECG: 3.5, EMG: 7.0, PPG: 7.0 }
};

// ============================================================================
// ESTADO
// ============================================================================
const S = {
    ws: null,
    connected: false,
    paused: false,
    zoom: 100,
    offsetY: 0,         // Offset vertical para pan
    sig: '--',
    cond: '--',
    state: 'IDLE',
    buf1: [],
    buf2: [],
    pts: 0,
    lastMsg: 0,
    csvData: [],
    isDragging: false,
    dragStartY: 0,
    dragStartOffset: 0
};

let pingTimer = null;
let reconTimer = null;

// ============================================================================
// DOM
// ============================================================================
let canvas, ctx;
const $ = id => document.getElementById(id);

// ============================================================================
// INICIALIZACIÓN
// ============================================================================
document.addEventListener('DOMContentLoaded', () => {
    canvas = $('canvas');
    ctx = canvas.getContext('2d');
    
    resize();
    window.addEventListener('resize', resize);
    
    // Controles
    $('btnZi').onclick = () => adjZoom(25);
    $('btnZo').onclick = () => adjZoom(-25);
    $('btnUp').onclick = () => adjOffset(0.1);
    $('btnDn').onclick = () => adjOffset(-0.1);
    $('btnRst').onclick = resetView;
    $('btnP').onclick = togglePause;
    $('btnC').onclick = clear;
    $('btnS').onclick = screenshot;
    $('btnD').onclick = downloadCSV;
    
    // Drag para pan vertical en el canvas
    canvas.addEventListener('mousedown', startDrag);
    canvas.addEventListener('mousemove', doDrag);
    canvas.addEventListener('mouseup', endDrag);
    canvas.addEventListener('mouseleave', endDrag);
    
    // Touch para móviles
    canvas.addEventListener('touchstart', e => {
        e.preventDefault();
        startDrag(e.touches[0]);
    });
    canvas.addEventListener('touchmove', e => {
        e.preventDefault();
        doDrag(e.touches[0]);
    });
    canvas.addEventListener('touchend', endDrag);
    
    // Wheel para zoom
    canvas.addEventListener('wheel', e => {
        e.preventDefault();
        if (e.deltaY < 0) adjZoom(10);
        else adjZoom(-10);
    });
    
    connect();
    requestAnimationFrame(render);
    setInterval(stats, 1000);
});

function resize() {
    const wrap = canvas.parentElement;
    const rect = wrap.getBoundingClientRect();
    // Restar el ancho del eje Y
    const yAxisWidth = 42;
    const dpr = window.devicePixelRatio || 1;
    
    const w = rect.width - yAxisWidth;
    const h = rect.height;
    
    canvas.width = w * dpr;
    canvas.height = h * dpr;
    canvas.style.width = w + 'px';
    canvas.style.height = h + 'px';
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
}

// ============================================================================
// DRAG PARA PAN VERTICAL
// ============================================================================
function startDrag(e) {
    S.isDragging = true;
    S.dragStartY = e.clientY || e.pageY;
    S.dragStartOffset = S.offsetY;
    canvas.style.cursor = 'grabbing';
}

function doDrag(e) {
    if (!S.isDragging) return;
    const y = e.clientY || e.pageY;
    const dy = y - S.dragStartY;
    const h = canvas.height / (window.devicePixelRatio || 1);
    // Convertir pixeles a unidades de offset
    S.offsetY = S.dragStartOffset + (dy / h) * 2;
    updateOffset();
}

function endDrag() {
    S.isDragging = false;
    canvas.style.cursor = 'grab';
}

// ============================================================================
// WEBSOCKET - Conexión Estable
// ============================================================================
function connect() {
    // Evitar múltiples conexiones simultáneas
    if (S.ws) {
        if (S.ws.readyState === WebSocket.OPEN) {
            console.log('[WS] Ya conectado');
            return;
        }
        if (S.ws.readyState === WebSocket.CONNECTING) {
            console.log('[WS] Conexión en progreso...');
            return;
        }
        
        // Limpiar listeners anteriores
        S.ws.onopen = null;
        S.ws.onclose = null;
        S.ws.onerror = null;
        S.ws.onmessage = null;
        try { S.ws.close(); } catch(e) {}
        S.ws = null;
    }
    
    updConn(false, 'Conectando...');
    
    try {
        S.ws = new WebSocket(CFG.wsUrl);
        S.ws.binaryType = 'arraybuffer';
    } catch(e) {
        console.error('[WS] Error creando socket:', e);
        scheduleReconnect();
        return;
    }
    
    // Timeout de conexión (10 segundos)
    const connTimeout = setTimeout(() => {
        if (S.ws && S.ws.readyState === WebSocket.CONNECTING) {
            console.log('[WS] Timeout de conexión');
            try { S.ws.close(); } catch(e) {}
        }
    }, 10000);
    
    S.ws.onopen = () => {
        clearTimeout(connTimeout);
        S.connected = true;
        S.lastMsg = Date.now();
        updConn(true, 'Conectado');
        console.log('[WS] Conectado exitosamente');
        startPing();
    };
    
    S.ws.onclose = (e) => {
        clearTimeout(connTimeout);
        if (S.connected) {
            console.log('[WS] Desconectado, código:', e.code, 'razón:', e.reason || 'ninguna');
        }
        S.connected = false;
        stopPing();
        updConn(false, 'Desconectado');
        scheduleReconnect();
    };
    
    S.ws.onerror = (e) => {
        console.error('[WS] Error de conexión');
        // onclose se dispara después automáticamente
    };
    
    S.ws.onmessage = e => {
        S.lastMsg = Date.now();
        try {
            const data = typeof e.data === 'string' ? e.data : new TextDecoder().decode(e.data);
            handleMsg(JSON.parse(data));
        } catch(err) {
            console.error('[WS] Error parseando mensaje:', err);
        }
    };
}

function scheduleReconnect() {
    if (reconTimer) clearTimeout(reconTimer);
    // Aumentar tiempo entre reconexiones para no saturar
    reconTimer = setTimeout(connect, CFG.reconnectMs);
}

function startPing() {
    stopPing();
    // Ping menos frecuente y solo si no hay datos recientes
    pingTimer = setInterval(() => {
        if (!S.ws || S.ws.readyState !== WebSocket.OPEN) return;
        
        // Solo hacer ping si no hemos recibido datos en los últimos 10 segundos
        const timeSinceLastMsg = Date.now() - S.lastMsg;
        if (timeSinceLastMsg > 10000) {
            try {
                S.ws.send('ping');
            } catch(e) {
                console.error('[WS] Error enviando ping');
            }
        }
    }, CFG.pingInterval);
}

function stopPing() {
    if (pingTimer) {
        clearInterval(pingTimer);
        pingTimer = null;
    }
}

function handleMsg(msg) {
    switch(msg.type) {
        case 'welcome':
            console.log('[WS] Bienvenida del servidor');
            break;
        case 'pong':
            // Respuesta al ping, conexión activa
            break;
        case 'data':
            handleData(msg);
            break;
        case 'batch':
            handleBatch(msg);
            break;
        case 'metrics':
            handleMetrics(msg);
            break;
        case 'state':
            handleState(msg);
            break;
    }
}

function handleBatch(msg) {
    // Actualizar señal
    if (msg.signal && msg.signal !== S.sig) {
        S.sig = msg.signal;
        S.buf1 = [];
        S.buf2 = [];
        S.offsetY = 0;
        updSig();
    }
    
    // Actualizar condición
    if (msg.condition) {
        S.cond = msg.condition;
        $('cond').textContent = S.cond;
    }
    
    // Actualizar estado
    if (msg.state) {
        S.state = msg.state;
        updState();
    }
    
    if (S.paused) return;
    
    // Procesar array de valores
    const vals = msg.v;
    const envs = msg.e || [];
    
    if (vals && Array.isArray(vals)) {
        for (let i = 0; i < vals.length; i++) {
            const v = parseFloat(vals[i]);
            if (!isNaN(v)) {
                S.buf1.push(v);
                if (S.buf1.length > CFG.bufSize) S.buf1.shift();
                
                S.csvData.push({
                    t: msg.t || Date.now(),
                    sig: S.sig,
                    v: v,
                    env: envs[i] ? parseFloat(envs[i]) : 0
                });
                if (S.csvData.length > 10000) S.csvData.shift();
            }
            
            // Envelope EMG
            if (envs[i] !== undefined) {
                const env = parseFloat(envs[i]);
                if (!isNaN(env) && env !== 0) {
                    S.buf2.push(env);
                    if (S.buf2.length > CFG.bufSize) S.buf2.shift();
                }
            }
        }
        S.pts += vals.length;
    }
}

function handleData(msg) {
    // Actualizar señal
    if (msg.signal && msg.signal !== S.sig) {
        S.sig = msg.signal;
        S.buf1 = [];
        S.buf2 = [];
        S.offsetY = 0;
        updSig();
    }
    
    // Actualizar condición
    if (msg.condition) {
        S.cond = msg.condition;
        $('cond').textContent = S.cond;
    }
    
    // Actualizar estado
    if (msg.state) {
        S.state = msg.state;
        updState();
    }
    
    if (S.paused) return;
    
    const v = msg.v;
    if (v !== undefined && !isNaN(v)) {
        S.buf1.push(v);
        if (S.buf1.length > CFG.bufSize) S.buf1.shift();
        
        S.csvData.push({
            t: msg.t || Date.now(),
            sig: S.sig,
            v: v,
            env: msg.env || 0
        });
        if (S.csvData.length > 10000) S.csvData.shift();
    }
    
    // Envelope EMG
    if (msg.env !== undefined && msg.env !== 0) {
        S.buf2.push(msg.env);
        if (S.buf2.length > CFG.bufSize) S.buf2.shift();
    }
    
    S.pts++;
}

function handleMetrics(msg) {
    const m = msg.m;
    if (!m) return;
    
    if (S.sig === 'ECG') {
        setText('ecgHR', m.hr);
        setText('ecgRR', m.rr);
        setText('ecgQRS', fmt(m.qrs, 2));
        setText('ecgST', fmt(m.st, 2));
        setText('ecgHRV', fmt(m.hrv, 1));
    } else if (S.sig === 'EMG') {
        setText('emgRMS', fmt(m.rms, 2));
        setText('emgEXC', m.exc);
        setText('emgMU', m.mus);
        setText('emgFRQ', m.freq);
    } else if (S.sig === 'PPG') {
        setText('ppgHR', m.hr);
        setText('ppgRR', m.rr);
        setText('ppgPI', fmt(m.pi, 1));
        setText('ppgAC', fmt(m.ac, 1));
    }
}

function handleState(msg) {
    if (msg.signal) S.sig = msg.signal;
    if (msg.condition) S.cond = msg.condition;
    if (msg.state) S.state = msg.state;
    updSig();
}

function setText(id, val) {
    const el = $(id);
    if (el) el.textContent = val ?? '--';
}

function fmt(v, d) {
    if (v == null || isNaN(v)) return '--';
    return Number(v).toFixed(d);
}

// ============================================================================
// ACTUALIZACIONES DE UI
// ============================================================================
function updConn(on, text) {
    $('dot').className = 'dot' + (on ? ' on' : '');
    $('conn').textContent = text || (on ? 'Conectado' : 'Desconectado');
}

function updState() {
    const badge = $('state');
    badge.textContent = S.state;
    badge.className = 'badge' + 
        (S.state === 'RUNNING' ? ' run' : '') +
        (S.state === 'PAUSED' ? ' pause' : '');
}

function updSig() {
    $('sig').textContent = S.sig;
    $('cond').textContent = S.cond;
    updState();
    
    // Paneles de métricas
    $('mECG').style.display = S.sig === 'ECG' ? 'flex' : 'none';
    $('mEMG').style.display = S.sig === 'EMG' ? 'flex' : 'none';
    $('mPPG').style.display = S.sig === 'PPG' ? 'flex' : 'none';
    
    // Actualizar etiquetas de ejes con zoom actual
    updateYAxisLabels();
    
    // Eje X
    const t = CFG.timeWin[S.sig] || 3.5;
    $('xMid').textContent = (t/2).toFixed(1) + 's';
    $('xMax').textContent = t.toFixed(1) + 's';
    
    // Leyenda
    if (S.sig === 'EMG') {
        $('l1').textContent = 'Raw';
        $('l2w').style.display = 'flex';
        $('l2').textContent = 'Envelope';
    } else {
        $('l1').textContent = S.sig;
        $('l2w').style.display = 'none';
    }
}

function updateOffset() {
    $('offset').textContent = 'Offset: ' + S.offsetY.toFixed(2);
}

// ============================================================================
// RENDERIZADO
// ============================================================================
function render() {
    const w = canvas.width / (window.devicePixelRatio || 1);
    const h = canvas.height / (window.devicePixelRatio || 1);
    
    // Limpiar
    ctx.fillStyle = CFG.colors.bg;
    ctx.fillRect(0, 0, w, h);
    
    // Grid
    drawGrid(w, h);
    
    // Señales
    if (S.buf1.length > 1) {
        const r = CFG.ranges[S.sig] || CFG.ranges.ECG;
        drawLine(S.buf1, w, h, r, CFG.colors.ch1, 2);
        
        if (S.sig === 'EMG' && S.buf2.length > 1) {
            const envR = { min: 0, max: 2 };
            drawLine(S.buf2, w, h, envR, CFG.colors.ch2, 1.5);
        }
    }
    
    requestAnimationFrame(render);
}

function drawGrid(w, h) {
    const gx = CFG.gridX;  // 10 divisiones tiempo
    const gy = CFG.gridY;  // 8 divisiones amplitud
    
    // Subdivisiones menores (5 por cada división mayor)
    const subDiv = 5;
    
    // Grid menor (subdivisiones)
    ctx.strokeStyle = CFG.colors.gridMin || '#082030';
    ctx.lineWidth = 0.5;
    
    // Subdivisiones verticales
    for (let i = 0; i <= gx * subDiv; i++) {
        const x = (i / (gx * subDiv)) * w;
        ctx.beginPath();
        ctx.moveTo(x, 0);
        ctx.lineTo(x, h);
        ctx.stroke();
    }
    
    // Subdivisiones horizontales  
    for (let i = 0; i <= gy * subDiv; i++) {
        const y = (i / (gy * subDiv)) * h;
        ctx.beginPath();
        ctx.moveTo(0, y);
        ctx.lineTo(w, y);
        ctx.stroke();
    }
    
    // Grid mayor (divisiones principales)
    ctx.strokeStyle = CFG.colors.grid;
    ctx.lineWidth = 1;
    
    // Líneas verticales mayores
    for (let i = 0; i <= gx; i++) {
        const x = (i / gx) * w;
        ctx.beginPath();
        ctx.moveTo(x, 0);
        ctx.lineTo(x, h);
        ctx.stroke();
    }
    
    // Líneas horizontales mayores
    for (let i = 0; i <= gy; i++) {
        const y = (i / gy) * h;
        ctx.beginPath();
        ctx.moveTo(0, y);
        ctx.lineTo(w, y);
        ctx.stroke();
    }
    
    // Línea central (baseline) más destacada
    ctx.strokeStyle = CFG.colors.gridMaj;
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.moveTo(0, h/2);
    ctx.lineTo(w, h/2);
    ctx.stroke();
}

function drawLine(buf, w, h, range, color, lw) {
    const zf = S.zoom / 100;
    const len = buf.length;
    const mid = (range.max + range.min) / 2;
    const span = (range.max - range.min) / zf;
    
    ctx.strokeStyle = color;
    ctx.lineWidth = lw;
    ctx.lineJoin = 'round';
    ctx.lineCap = 'round';
    ctx.beginPath();
    
    for (let i = 0; i < len; i++) {
        const x = (i / (CFG.bufSize - 1)) * w;
        const val = buf[i];
        // Aplicar offset vertical
        const norm = (val - mid + S.offsetY * (span / 2)) / (span / 2);
        const y = h/2 - (norm * h/2);
        
        if (i === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
    }
    
    ctx.stroke();
}

// ============================================================================
// CONTROLES
// ============================================================================
function adjZoom(d) {
    S.zoom = Math.max(25, Math.min(400, S.zoom + d));
    $('zoom').textContent = S.zoom + '%';
    updateYAxisLabels();
}

function updateYAxisLabels() {
    const r = CFG.ranges[S.sig] || CFG.ranges.ECG;
    const zf = S.zoom / 100;
    const mid = (r.max + r.min) / 2;
    const span = (r.max - r.min) / zf;
    
    // Aplicar offset vertical a las etiquetas
    const offsetMid = mid - S.offsetY * (span / 2);
    const yMax = offsetMid + span / 2;
    const yMin = offsetMid - span / 2;
    
    $('yMax').textContent = yMax >= 0 ? '+' + yMax.toFixed(2) : yMax.toFixed(2);
    $('yMid').textContent = offsetMid.toFixed(2);
    $('yMin').textContent = yMin.toFixed(2);
    
    // Actualizar escala div
    const divVal = (r.div / zf).toFixed(2);
    $('scale').textContent = divVal + ' ' + r.unit + '/div';
}

function adjOffset(d) {
    S.offsetY += d;
    updateOffset();
    updateYAxisLabels();
}

function resetView() {
    S.zoom = 100;
    S.offsetY = 0;
    $('zoom').textContent = '100%';
    updateOffset();
    updateYAxisLabels();
}

function togglePause() {
    S.paused = !S.paused;
    const btn = $('btnP');
    btn.textContent = S.paused ? '▶ Play' : '⏸ Pausar';
    btn.classList.toggle('pri', !S.paused);
}

function clear() {
    S.buf1 = [];
    S.buf2 = [];
    S.csvData = [];
}

function screenshot() {
    const link = document.createElement('a');
    link.download = `biosim_${S.sig}_${Date.now()}.png`;
    link.href = canvas.toDataURL('image/png');
    link.click();
}

function downloadCSV() {
    if (S.csvData.length === 0) {
        alert('No hay datos para descargar. Espera a que se genere la señal.');
        return;
    }
    
    let csv = 'Timestamp_ms,Signal,Value_mV,Envelope_mV\n';
    for (const d of S.csvData) {
        csv += `${d.t},${d.sig},${d.v.toFixed(4)},${d.env.toFixed(4)}\n`;
    }
    
    const blob = new Blob([csv], { type: 'text/csv;charset=utf-8;' });
    const url = URL.createObjectURL(blob);
    const link = document.createElement('a');
    link.href = url;
    link.download = `biosim_${S.sig}_${Date.now()}.csv`;
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
    URL.revokeObjectURL(url);
}

function stats() {
    $('rate').textContent = S.pts + ' pts/s';
    S.pts = 0;
}
