/**
 * BioSimulator Pro - Real-Time Signal Plotter
 * Streaming en tiempo real sincronizado con Nextion
 */

const CFG = {
    wsUrl: `ws://${window.location.hostname}/ws`,
    reconnectMs: 3000,
    pingInterval: 15000,
    bufSize: 800,
    gridX: 10,
    gridY: 8,
    colors: {
        bg: "#fefefe",
        grid: "#c8e0c8",
        gridMajor: "#90c090",
        ch1: "#0066cc",
        ch2: "#dc2626"
    },
    ranges: {
        ECG: { min: -0.5, max: 1.5, div: 0.25, unit: "mV" },
        EMG: { min: -5.0, max: 5.0, div: 1.25, unit: "mV" },
        PPG: { min: 0, max: 150, div: 18.75, unit: "mV" }
    },
    timeWin: { ECG: 3.5, EMG: 7.0, PPG: 7.0 }
};

const S = {
    ws: null,
    connected: false,
    viewing: false,
    startTime: 0,
    sig: "--",
    cond: "--",
    state: "IDLE",
    buf1: [],
    buf2: [],
    csvData: [],
    ptsTotal: 0,
    ptsPerSec: 0,
    ptsCounter: 0,
    zoom: 100,
    hzoom: 100,
    lastMetrics: null
};

let canvas, ctx;
let pingTimer = null;
let reconTimer = null;
let statsTimer = null;
let animFrame = null;

const $ = id => document.getElementById(id);

document.addEventListener("DOMContentLoaded", () => {
    canvas = $("plotCanvas");
    ctx = canvas.getContext("2d");
    
    resize();
    window.addEventListener("resize", resize);
    
    $("btnStart").onclick = startViewing;
    $("btnStop").onclick = stopViewing;
    $("btnReset").onclick = resetPlot;
    $("btnCapture").onclick = captureScreenshot;
    $("btnSave").onclick = downloadCSV;
    
    $("zoomSlider").oninput = () => {
        S.zoom = parseInt($("zoomSlider").value);
        $("zoomVal").textContent = S.zoom + "%";
    };
    
    $("hzoomSlider").oninput = () => {
        S.hzoom = parseInt($("hzoomSlider").value);
        $("hzoomVal").textContent = S.hzoom + "%";
    };
    
    canvas.addEventListener("wheel", e => {
        e.preventDefault();
        let z = S.zoom + (e.deltaY < 0 ? 10 : -10);
        z = Math.max(25, Math.min(400, z));
        S.zoom = z;
        $("zoomSlider").value = z;
        $("zoomVal").textContent = z + "%";
    });
    
    connect();
    statsTimer = setInterval(updateStats, 1000);
    animFrame = requestAnimationFrame(renderLoop);
});

function resize() {
    const wrap = canvas.parentElement;
    const rect = wrap.getBoundingClientRect();
    const yAxisWidth = 50;
    const dpr = window.devicePixelRatio || 1;
    const w = rect.width - yAxisWidth;
    const h = rect.height;
    canvas.width = w * dpr;
    canvas.height = h * dpr;
    canvas.style.width = w + "px";
    canvas.style.height = h + "px";
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
}

function connect() {
    if (S.ws && (S.ws.readyState === WebSocket.OPEN || S.ws.readyState === WebSocket.CONNECTING)) return;
    
    updConn(false, "Conectando...");
    
    try {
        S.ws = new WebSocket(CFG.wsUrl);
    } catch (e) {
        scheduleReconnect();
        return;
    }
    
    S.ws.onopen = () => {
        S.connected = true;
        updConn(true, "Conectado");
        startPing();
    };
    
    S.ws.onclose = () => {
        if (S.connected) {
            S.connected = false;
            stopPing();
            updConn(false, "Desconectado");
            scheduleReconnect();
        }
    };
    
    S.ws.onerror = () => {};
    
    S.ws.onmessage = e => {
        try {
            handleMsg(JSON.parse(e.data));
        } catch (err) {}
    };
}

function scheduleReconnect() {
    if (reconTimer) clearTimeout(reconTimer);
    reconTimer = setTimeout(connect, CFG.reconnectMs);
}

function startPing() {
    stopPing();
    pingTimer = setInterval(() => {
        if (S.ws && S.ws.readyState === WebSocket.OPEN) {
            try { S.ws.send("ping"); } catch (e) {}
        }
    }, CFG.pingInterval);
}

function stopPing() {
    if (pingTimer) { clearInterval(pingTimer); pingTimer = null; }
}

function handleMsg(msg) {
    switch (msg.type) {
        case "welcome": break;
        case "data": handleData(msg); break;
        case "metrics": handleMetrics(msg); break;
        case "state": handleState(msg); break;
    }
}

function handleData(msg) {
    if (msg.signal && msg.signal !== S.sig) {
        S.sig = msg.signal;
        S.buf1 = [];
        S.buf2 = [];
        updateSignalUI();
    }
    if (msg.condition) {
        S.cond = msg.condition;
        $("sigCond").textContent = S.cond;
    }
    if (msg.state) {
        S.state = msg.state;
        updateStateUI();
    }
    
    if (!S.viewing) return;
    
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
        if (S.csvData.length > 50000) S.csvData.shift();
        
        S.ptsTotal++;
        S.ptsCounter++;
    }
    
    if (msg.env !== undefined && msg.env !== 0) {
        S.buf2.push(msg.env);
        if (S.buf2.length > CFG.bufSize) S.buf2.shift();
    }
    
    const elapsed = (Date.now() - S.startTime) / 1000;
    $("statTime").textContent = elapsed.toFixed(1) + "s";
}

function handleMetrics(msg) {
    const m = msg.m;
    if (!m) return;
    S.lastMetrics = m;
    
    if (S.sig === "ECG") {
        setText("ecgHR", m.hr);
        setText("ecgRR", m.rr);
        setText("ecgQRS", fmt(m.qrs, 2));
        setText("ecgST", fmt(m.st, 2));
        setText("ecgHRV", fmt(m.hrv, 1));
        setText("ecgPR", m.pr || "--");
        setText("ecgQTc", m.qtc || "--");
        setText("ecgP", fmt(m.p, 2));
        setText("ecgR", fmt(m.r, 2));
        setText("ecgT", fmt(m.t, 2));
    } else if (S.sig === "EMG") {
        setText("emgRMS", fmt(m.rms, 2));
        setText("emgEXC", m.exc);
        setText("emgMU", m.mus);
        setText("emgMDF", m.freq);
        setText("emgMVC", m.mvc || "--");
        setText("emgRAW", fmt(m.raw, 2));
    } else if (S.sig === "PPG") {
        setText("ppgHR", m.hr);
        setText("ppgRR", m.rr);
        setText("ppgPI", fmt(m.pi, 1));
        setText("ppgAC", fmt(m.ac, 1));
        setText("ppgSys", m.sys || "--");
        setText("ppgDia", m.dia || "--");
    }
}

function handleState(msg) {
    if (msg.signal) S.sig = msg.signal;
    if (msg.condition) S.cond = msg.condition;
    if (msg.state) S.state = msg.state;
    updateSignalUI();
}

function startViewing() {
    if (!S.connected) {
        alert("No hay conexion con el dispositivo");
        return;
    }
    
    S.buf1 = [];
    S.buf2 = [];
    S.csvData = [];
    S.ptsTotal = 0;
    S.ptsCounter = 0;
    S.startTime = Date.now();
    S.viewing = true;
    
    $("btnStart").disabled = true;
    $("btnStop").disabled = false;
    $("sigState").textContent = "EN VIVO";
    $("sigState").className = "badge run";
    $("plotTitle").textContent = "📡 " + S.sig + " - " + S.cond + " (En vivo)";
    updateAxisLabels();
}

function stopViewing() {
    S.viewing = false;
    
    $("btnStart").disabled = false;
    $("btnStop").disabled = true;
    $("sigState").textContent = "DETENIDO";
    $("sigState").className = "badge pause";
    $("plotTitle").textContent = S.sig + " - " + S.cond + " (" + S.ptsTotal + " pts)";
}

function resetPlot() {
    S.buf1 = [];
    S.buf2 = [];
    S.csvData = [];
    S.ptsTotal = 0;
    S.ptsCounter = 0;
    S.viewing = false;
    
    $("btnStart").disabled = false;
    $("btnStop").disabled = true;
    $("sigState").textContent = "IDLE";
    $("sigState").className = "badge";
    $("plotTitle").textContent = "Esperando senal...";
    $("statPts").textContent = "0";
    $("statTime").textContent = "0.0s";
    $("statProg").textContent = "--";
}

function renderLoop() {
    render();
    animFrame = requestAnimationFrame(renderLoop);
}

function render() {
    const w = canvas.width / (window.devicePixelRatio || 1);
    const h = canvas.height / (window.devicePixelRatio || 1);
    
    ctx.fillStyle = CFG.colors.bg;
    ctx.fillRect(0, 0, w, h);
    
    drawGrid(w, h);
    
    if (S.buf1.length > 1) {
        const r = CFG.ranges[S.sig] || CFG.ranges.ECG;
        drawSignal(S.buf1, w, h, r, CFG.colors.ch1, 2);
        
        if (S.sig === "EMG" && S.buf2.length > 1) {
            // Envolvente EMG: rango 0-2.5 mV, línea más gruesa para visibilidad
            const envR = { min: 0, max: 2.5 };
            drawSignal(S.buf2, w, h, envR, CFG.colors.ch2, 3.5);
        }
    }
}

function drawGrid(w, h) {
    const gx = CFG.gridX;
    const gy = CFG.gridY;
    
    ctx.lineWidth = 1;
    
    for (let i = 0; i <= gx; i++) {
        const x = (i / gx) * w;
        ctx.strokeStyle = (i % 5 === 0) ? CFG.colors.gridMajor : CFG.colors.grid;
        ctx.beginPath();
        ctx.moveTo(x, 0);
        ctx.lineTo(x, h);
        ctx.stroke();
    }
    
    for (let i = 0; i <= gy; i++) {
        const y = (i / gy) * h;
        const isMid = (i === gy / 2);
        ctx.strokeStyle = isMid ? CFG.colors.gridMajor : CFG.colors.grid;
        ctx.lineWidth = isMid ? 2 : 1;
        ctx.beginPath();
        ctx.moveTo(0, y);
        ctx.lineTo(w, y);
        ctx.stroke();
    }
}

function drawSignal(buf, w, h, range, color, lw) {
    if (buf.length < 2) return;
    
    const zf = S.zoom / 100;   // Zoom vertical (escala Y)
    const hzf = S.hzoom / 100; // Zoom horizontal (escala X)
    const mid = (range.max + range.min) / 2;
    const span = (range.max - range.min) / zf;
    const len = buf.length;
    
    // Calcular cuántos puntos mostrar según zoom horizontal
    const visiblePoints = Math.min(len, Math.floor(CFG.bufSize / hzf));
    const startIdx = Math.max(0, len - visiblePoints);
    
    ctx.strokeStyle = color;
    ctx.lineWidth = lw;
    ctx.lineJoin = "round";
    ctx.lineCap = "round";
    ctx.beginPath();
    
    for (let i = startIdx; i < len; i++) {
        const xNorm = (i - startIdx) / (visiblePoints - 1);
        const x = xNorm * w;
        const val = buf[i];
        const norm = (val - mid) / (span / 2);
        const y = h / 2 - (norm * h / 2);
        
        if (i === startIdx) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
    }
    
    ctx.stroke();
}

function updateSignalUI() {
    $("sigType").textContent = S.sig;
    $("sigCond").textContent = S.cond;
    updateStateUI();
    
    $("metricsECG").style.display = S.sig === "ECG" ? "block" : "none";
    $("metricsEMG").style.display = S.sig === "EMG" ? "block" : "none";
    $("metricsPPG").style.display = S.sig === "PPG" ? "block" : "none";
    
    updateAxisLabels();
    
    if (S.sig === "EMG") {
        $("leg1").textContent = "Raw";
        $("leg2wrap").style.display = "flex";
        $("leg2").textContent = "Envelope";
    } else {
        $("leg1").textContent = S.sig;
        $("leg2wrap").style.display = "none";
    }
}

function updateAxisLabels() {
    const r = CFG.ranges[S.sig] || CFG.ranges.ECG;
    const tw = CFG.timeWin[S.sig] || 3.5;
    const mid = (r.max + r.min) / 2;
    
    $("yMax").textContent = "+" + r.max.toFixed(1);
    $("yMid").textContent = mid.toFixed(1);
    $("yMin").textContent = r.min.toFixed(1);
    
    $("xMid").textContent = (tw / 2).toFixed(1) + "s";
    $("xMax").textContent = tw.toFixed(1) + "s";
    
    $("plotScale").textContent = r.div + " " + r.unit + "/div | " + (tw / CFG.gridX * 1000).toFixed(0) + " ms/div";
    $("footerScale").textContent = "Escala: " + r.div + " " + r.unit + "/div";
}

function updateStateUI() {
    if (!S.viewing) {
        if (S.state === "RUNNING") {
            $("sigState").textContent = "ESP32 ACTIVO";
            $("sigState").className = "badge run";
        } else if (S.state === "PAUSED") {
            $("sigState").textContent = "PAUSADO";
            $("sigState").className = "badge pause";
        } else if (S.ptsTotal > 0) {
            $("sigState").textContent = "DETENIDO";
            $("sigState").className = "badge pause";
        } else {
            $("sigState").textContent = "IDLE";
            $("sigState").className = "badge";
        }
    }
}

function updateStats() {
    $("statPts").textContent = S.ptsTotal;
    $("statRate").textContent = S.ptsCounter + " pts/s";
    $("statProg").textContent = S.state;
    S.ptsPerSec = S.ptsCounter;
    S.ptsCounter = 0;
}

function captureScreenshot() {
    const link = document.createElement("a");
    link.download = `biosim_${S.sig}_${Date.now()}.png`;
    link.href = canvas.toDataURL("image/png");
    link.click();
}

function downloadCSV() {
    if (S.csvData.length === 0) {
        alert("No hay datos. Primero inicia la visualizacion.");
        return;
    }
    
    let csv = "Timestamp_ms,Senal,Valor_mV,Envelope_mV,Condicion\n";
    for (const d of S.csvData) {
        csv += `${d.t},${d.sig},${d.v.toFixed(4)},${d.env.toFixed(4)},${S.cond}\n`;
    }
    
    const blob = new Blob([csv], { type: "text/csv;charset=utf-8;" });
    const url = URL.createObjectURL(blob);
    const link = document.createElement("a");
    link.href = url;
    link.download = `biosim_${S.sig}_${S.cond}_${Date.now()}.csv`;
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
    URL.revokeObjectURL(url);
}

function updConn(on, text) {
    $("dot").className = "dot" + (on ? " on" : "");
    $("connText").textContent = text;
}

function setText(id, val) {
    const el = $(id);
    if (el) el.textContent = val ?? "--";
}

function fmt(v, d) {
    if (v == null || isNaN(v)) return "--";
    return Number(v).toFixed(d);
}
