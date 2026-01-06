/**
 * BioSimulator Pro - Responsive Web App
 * Con reconexión robusta y diseño adaptivo
 */

// ============================================================================
// CONFIG
// ============================================================================
const CFG = {
    wsUrl: `ws://${window.location.hostname}/ws`,
    reconnectMin: 1000,
    reconnectMax: 10000,
    bufSize: 700,
    gridX: 7,
    gridY: 8,
    colors: {
        bg: '#0a0a12',
        grid: '#1a2535',
        gridMaj: '#2a3545',
        ch1: '#00ff88',
        ch2: '#ff6b6b'
    },
    ranges: {
        ECG: { min: -0.5, max: 1.5, div: 0.25 },
        EMG: { min: -5.0, max: 5.0, div: 1.25 },
        PPG: { min: -100, max: 100, div: 25 }
    },
    timeWin: { ECG: 3.5, EMG: 7.0, PPG: 7.0 }
};

// ============================================================================
// STATE
// ============================================================================
const S = {
    ws: null,
    connected: false,
    paused: false,
    zoom: 100,
    sig: '--',
    cond: '--',
    state: 'IDLE',
    buf1: [],
    buf2: [],
    pts: 0,
    reconDelay: CFG.reconnectMin,
    reconTimer: null
};

// ============================================================================
// DOM
// ============================================================================
let canvas, ctx;
const $ = id => document.getElementById(id);

// ============================================================================
// INIT
// ============================================================================
document.addEventListener('DOMContentLoaded', () => {
    canvas = $('canvas');
    ctx = canvas.getContext('2d');
    
    resize();
    window.addEventListener('resize', resize);
    
    // Controls
    $('btnZi').onclick = () => adjZoom(25);
    $('btnZo').onclick = () => adjZoom(-25);
    $('btnP').onclick = togglePause;
    $('btnC').onclick = clear;
    $('btnS').onclick = screenshot;
    
    connect();
    requestAnimationFrame(render);
    setInterval(stats, 1000);
});

function resize() {
    const wrap = canvas.parentElement;
    const rect = wrap.getBoundingClientRect();
    const yW = 38;
    const dpr = window.devicePixelRatio || 1;
    
    const w = rect.width - yW;
    const h = rect.height;
    
    canvas.width = w * dpr;
    canvas.height = h * dpr;
    canvas.style.width = w + 'px';
    canvas.style.height = h + 'px';
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
}

// ============================================================================
// WEBSOCKET - Reconexión robusta
// ============================================================================
function connect() {
    if (S.ws && S.ws.readyState < 2) return;
    
    console.log('[WS] Conectando...');
    S.ws = new WebSocket(CFG.wsUrl);
    
    S.ws.onopen = () => {
        S.connected = true;
        S.reconDelay = CFG.reconnectMin;
        updConn(true);
        console.log('[WS] Conectado');
    };
    
    S.ws.onclose = () => {
        S.connected = false;
        updConn(false);
        scheduleReconnect();
    };
    
    S.ws.onerror = (e) => {
        console.error('[WS] Error', e);
        S.ws.close();
    };
    
    S.ws.onmessage = e => {
        try {
            handleMsg(JSON.parse(e.data));
        } catch(err) {
            console.error('[WS] Parse error', err);
        }
    };
}

function scheduleReconnect() {
    if (S.reconTimer) clearTimeout(S.reconTimer);
    
    console.log(`[WS] Reconectando en ${S.reconDelay}ms...`);
    S.reconTimer = setTimeout(() => {
        S.reconDelay = Math.min(S.reconDelay * 1.5, CFG.reconnectMax);
        connect();
    }, S.reconDelay);
}

function handleMsg(msg) {
    switch(msg.type) {
        case 'data': handleData(msg); break;
        case 'metrics': handleMetrics(msg); break;
        case 'state': handleState(msg); break;
    }
}

function handleData(msg) {
    if (S.paused) return;
    
    if (msg.signal && msg.signal !== S.sig) {
        S.sig = msg.signal;
        clear();
        updSig();
    }
    
    S.buf1.push(msg.v);
    if (S.buf1.length > CFG.bufSize) S.buf1.shift();
    
    if (msg.env !== undefined && msg.env !== 0) {
        S.buf2.push(msg.env);
        if (S.buf2.length > CFG.bufSize) S.buf2.shift();
    }
    
    S.pts++;
}

function handleMetrics(msg) {
    const m = msg.m;
    
    if (S.sig === 'ECG') {
        $('ecgHR').textContent = m.hr ?? '--';
        $('ecgRR').textContent = m.rr ?? '--';
        $('ecgQRS').textContent = fmt(m.qrs, 2);
        $('ecgST').textContent = fmt(m.st, 2);
        $('ecgHRV').textContent = fmt(m.hrv, 1);
    } else if (S.sig === 'EMG') {
        $('emgRMS').textContent = fmt(m.rms, 2);
        $('emgEXC').textContent = m.exc ?? '--';
        $('emgMU').textContent = m.mus ?? '--';
        $('emgFRQ').textContent = m.freq ?? '--';
    } else if (S.sig === 'PPG') {
        $('ppgHR').textContent = m.hr ?? '--';
        $('ppgRR').textContent = m.rr ?? '--';
        $('ppgPI').textContent = fmt(m.pi, 1);
        $('ppgAC').textContent = fmt(m.ac, 1);
    }
}

function handleState(msg) {
    S.sig = msg.signal || S.sig;
    S.cond = msg.condition || S.cond;
    S.state = msg.state || S.state;
    updSig();
}

function fmt(v, d) {
    return v != null ? v.toFixed(d) : '--';
}

// ============================================================================
// UI Updates
// ============================================================================
function updConn(on) {
    $('dot').className = 'dot' + (on ? ' on' : '');
    $('conn').textContent = on ? 'Online' : 'Offline';
}

function updSig() {
    $('sig').textContent = S.sig;
    $('cond').textContent = S.cond;
    
    const badge = $('state');
    badge.textContent = S.state;
    badge.className = 'badge' + 
        (S.state === 'RUNNING' ? ' run' : '') +
        (S.state === 'PAUSED' ? ' pause' : '');
    
    // Metrics panels
    $('mECG').style.display = S.sig === 'ECG' ? 'flex' : 'none';
    $('mEMG').style.display = S.sig === 'EMG' ? 'flex' : 'none';
    $('mPPG').style.display = S.sig === 'PPG' ? 'flex' : 'none';
    
    // Y-axis labels
    const r = CFG.ranges[S.sig] || CFG.ranges.ECG;
    $('yMax').textContent = '+' + r.max;
    $('yMid').textContent = '0';
    $('yMin').textContent = r.min;
    
    // X-axis
    const t = CFG.timeWin[S.sig] || 3.5;
    $('xMid').textContent = (t/2).toFixed(1) + 's';
    $('xMax').textContent = t.toFixed(1) + 's';
    
    // Scale
    $('scale').textContent = r.div + ' mV/div';
    
    // Legend
    if (S.sig === 'EMG') {
        $('l1').textContent = 'Raw';
        $('l2w').style.display = 'flex';
        $('l2').textContent = 'Env';
    } else {
        $('l1').textContent = S.sig;
        $('l2w').style.display = 'none';
    }
}

// ============================================================================
// RENDERING
// ============================================================================
function render() {
    const w = canvas.width / (window.devicePixelRatio || 1);
    const h = canvas.height / (window.devicePixelRatio || 1);
    
    // Clear
    ctx.fillStyle = CFG.colors.bg;
    ctx.fillRect(0, 0, w, h);
    
    // Grid
    drawGrid(w, h);
    
    // Signals
    if (S.buf1.length > 1) {
        const r = CFG.ranges[S.sig] || CFG.ranges.ECG;
        drawLine(S.buf1, w, h, r, CFG.colors.ch1, 1.5);
        
        // Envelope for EMG
        if (S.sig === 'EMG' && S.buf2.length > 1) {
            // Envelope uses 0 to 2mV range
            const envR = { min: 0, max: 2 };
            drawLine(S.buf2, w, h, envR, CFG.colors.ch2, 1.2);
        }
    }
    
    requestAnimationFrame(render);
}

function drawGrid(w, h) {
    const gx = CFG.gridX;
    const gy = CFG.gridY;
    
    ctx.strokeStyle = CFG.colors.grid;
    ctx.lineWidth = 0.5;
    
    // Vertical
    for (let i = 0; i <= gx; i++) {
        const x = (i / gx) * w;
        ctx.beginPath();
        ctx.moveTo(x, 0);
        ctx.lineTo(x, h);
        ctx.stroke();
    }
    
    // Horizontal
    for (let i = 0; i <= gy; i++) {
        const y = (i / gy) * h;
        ctx.strokeStyle = (i === gy/2) ? CFG.colors.gridMaj : CFG.colors.grid;
        ctx.lineWidth = (i === gy/2) ? 1 : 0.5;
        ctx.beginPath();
        ctx.moveTo(0, y);
        ctx.lineTo(w, y);
        ctx.stroke();
    }
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
        const norm = (val - mid) / (span / 2);
        const y = h/2 - (norm * h/2);
        
        if (i === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
    }
    
    ctx.stroke();
}

// ============================================================================
// CONTROLS
// ============================================================================
function adjZoom(d) {
    S.zoom = Math.max(25, Math.min(400, S.zoom + d));
    $('zoom').textContent = S.zoom + '%';
}

function togglePause() {
    S.paused = !S.paused;
    $('btnP').textContent = S.paused ? '▶' : '⏸';
    $('btnP').classList.toggle('pri', !S.paused);
}

function clear() {
    S.buf1 = [];
    S.buf2 = [];
}

function screenshot() {
    const link = document.createElement('a');
    link.download = `biosim_${S.sig}_${Date.now()}.png`;
    link.href = canvas.toDataURL('image/png');
    link.click();
}

function stats() {
    $('rate').textContent = S.pts + ' pts/s';
    S.pts = 0;
}
