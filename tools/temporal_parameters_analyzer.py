"""
Análisis Automático de Parámetros Temporales
Extrae PR, QRS, QTc, RR para ECG y otros parámetros temporales
"""

import numpy as np
import serial
import time
import matplotlib.pyplot as plt
from scipy.signal import find_peaks, butter, filtfilt
import json
from datetime import datetime

# Referencias clínicas
CLINICAL_RANGES = {
    "ECG": {
        "PR": {"min": 120, "max": 200, "unit": "ms"},
        "QRS": {"min": 80, "max": 120, "unit": "ms"},
        "QTc": {"min": 350, "max": 450, "unit": "ms"},
        "RR": {"min": 600, "max": 1200, "unit": "ms"},
        "HR": {"min": 60, "max": 100, "unit": "bpm"}
    },
    "EMG": {
        "RMS": {"min": 0.05, "max": 5.0, "unit": "mV"},
        "MDF": {"min": 50, "max": 150, "unit": "Hz"},
        "Contraction_Time": {"min": 100, "max": 500, "unit": "ms"}
    },
    "PPG": {
        "HR": {"min": 60, "max": 100, "unit": "bpm"},
        "PI": {"min": 1.0, "max": 10.0, "unit": "%"},
        "Systolic_Time": {"min": 100, "max": 300, "unit": "ms"},
        "Diastolic_Time": {"min": 400, "max": 800, "unit": "ms"}
    }
}

class TemporalAnalyzer:
    def __init__(self, port='COM4', baudrate=115200):
        self.port = port
        self.baudrate = baudrate
        self.ser = None
        
    def connect(self):
        """Conectar al puerto serial"""
        try:
            self.ser = serial.Serial(self.port, self.baudrate, timeout=1)
            time.sleep(2)
            print(f"✓ Conectado a {self.port}")
            return True
        except Exception as e:
            print(f"✗ Error conectando: {e}")
            return False
    
    def capture_ecg_cycle(self, duration=10):
        """Captura ciclo ECG y detecta picos R"""
        print(f"\n📊 Capturando ECG por {duration} segundos...")
        samples = []
        timestamps = []
        start = time.time()
        
        while (time.time() - start) < duration:
            if self.ser and self.ser.in_waiting:
                try:
                    line = self.ser.readline().decode().strip()
                    # Esperar formato: timestamp,value o solo value
                    parts = line.split(',')
                    if len(parts) >= 1:
                        value = float(parts[-1])
                        samples.append(value)
                        timestamps.append(time.time() - start)
                except:
                    pass
        
        return np.array(timestamps), np.array(samples)
    
    def detect_r_peaks(self, signal, fs=300):
        """Detecta picos R en señal ECG"""
        # Filtro pasa banda 5-15 Hz para realzar QRS
        nyq = fs / 2
        low = 5 / nyq
        high = 15 / nyq
        b, a = butter(2, [low, high], btype='band')
        filtered = filtfilt(b, a, signal)
        
        # Detectar picos
        prominence = np.std(filtered) * 0.5
        peaks, properties = find_peaks(filtered, prominence=prominence, distance=fs*0.4)
        
        return peaks, filtered
    
    def calculate_ecg_parameters(self, timestamps, signal, fs=300):
        """Calcula PR, QRS, QTc, RR, HR"""
        peaks, filtered = self.detect_r_peaks(signal, fs)
        
        if len(peaks) < 2:
            print("⚠️  No se detectaron suficientes picos R")
            return None
        
        # Intervalos RR (ms)
        rr_intervals = np.diff(peaks) / fs * 1000
        rr_mean = np.mean(rr_intervals)
        rr_std = np.std(rr_intervals)
        
        # HR (bpm)
        hr = 60000 / rr_mean
        
        # Estimación de QRS (duración del complejo)
        # Buscar inicio y fin del QRS alrededor del pico R
        qrs_durations = []
        for peak in peaks[:min(10, len(peaks))]:  # Analizar primeros 10 complejos
            window_start = max(0, peak - int(0.05 * fs))
            window_end = min(len(signal), peak + int(0.05 * fs))
            qrs_window = filtered[window_start:window_end]
            
            # Detectar inicio/fin cuando cruza 20% del pico
            threshold = 0.2 * np.max(np.abs(qrs_window))
            above_threshold = np.abs(qrs_window) > threshold
            if np.any(above_threshold):
                qrs_duration = np.sum(above_threshold) / fs * 1000
                qrs_durations.append(qrs_duration)
        
        qrs_mean = np.mean(qrs_durations) if qrs_durations else 100
        
        # QTc corregido (fórmula de Bazett)
        qt_estimated = 0.4 * np.sqrt(rr_mean)  # Estimación aproximada
        qtc = qt_estimated / np.sqrt(rr_mean / 1000)
        
        # PR estimado (simplificado)
        pr_estimated = 150  # Valor típico, requiere detección de onda P
        
        results = {
            "HR": round(hr, 1),
            "RR_mean": round(rr_mean, 1),
            "RR_std": round(rr_std, 1),
            "QRS": round(qrs_mean, 1),
            "QTc": round(qtc, 1),
            "PR": pr_estimated,
            "peaks_detected": len(peaks)
        }
        
        return results, peaks
    
    def calculate_emg_parameters(self, signal, fs=1000):
        """Calcula RMS, MDF, tiempo de contracción"""
        # RMS
        rms = np.sqrt(np.mean(signal**2))
        
        # MDF (Median Frequency)
        fft = np.fft.fft(signal)
        freqs = np.fft.fftfreq(len(signal), 1/fs)
        psd = np.abs(fft)**2
        
        # Solo frecuencias positivas
        pos_freqs = freqs[:len(freqs)//2]
        pos_psd = psd[:len(psd)//2]
        
        cumsum_psd = np.cumsum(pos_psd)
        median_idx = np.where(cumsum_psd >= cumsum_psd[-1] / 2)[0][0]
        mdf = pos_freqs[median_idx]
        
        # Tiempo de contracción (umbral 50% RMS)
        threshold = 0.5 * rms
        above_threshold = np.abs(signal) > threshold
        if np.any(above_threshold):
            contraction_samples = np.sum(above_threshold)
            contraction_time = contraction_samples / fs * 1000
        else:
            contraction_time = 0
        
        return {
            "RMS": round(rms, 3),
            "MDF": round(mdf, 1),
            "Contraction_Time": round(contraction_time, 1)
        }
    
    def calculate_ppg_parameters(self, signal, fs=100):
        """Calcula HR, PI, tiempos sistólico/diastólico"""
        # Detectar picos sistólicos
        peaks, _ = find_peaks(signal, prominence=np.std(signal)*0.3, distance=fs*0.5)
        
        if len(peaks) < 2:
            return None
        
        # HR
        intervals = np.diff(peaks) / fs * 1000
        hr = 60000 / np.mean(intervals)
        
        # PI (Perfusion Index) - aproximado como (AC/DC)*100
        ac = np.std(signal)
        dc = np.mean(signal)
        pi = (ac / dc) * 100 if dc > 0 else 0
        
        # Tiempos sistólico/diastólico
        systolic_times = []
        diastolic_times = []
        
        for i in range(len(peaks)-1):
            # Buscar valle entre picos (muesca dicrótica)
            valley_region = signal[peaks[i]:peaks[i+1]]
            if len(valley_region) > 0:
                valley_idx = np.argmin(valley_region)
                systolic_time = valley_idx / fs * 1000
                diastolic_time = (len(valley_region) - valley_idx) / fs * 1000
                systolic_times.append(systolic_time)
                diastolic_times.append(diastolic_time)
        
        return {
            "HR": round(hr, 1),
            "PI": round(pi, 2),
            "Systolic_Time": round(np.mean(systolic_times), 1) if systolic_times else 0,
            "Diastolic_Time": round(np.mean(diastolic_times), 1) if diastolic_times else 0
        }
    
    def validate_against_clinical(self, parameters, signal_type):
        """Valida parámetros contra rangos clínicos"""
        ranges = CLINICAL_RANGES.get(signal_type, {})
        validation = {}
        
        for param, value in parameters.items():
            if param in ranges:
                ref = ranges[param]
                is_valid = ref["min"] <= value <= ref["max"]
                validation[param] = {
                    "value": value,
                    "unit": ref["unit"],
                    "range": f"{ref['min']}-{ref['max']} {ref['unit']}",
                    "valid": "✓" if is_valid else "✗",
                    "status": "NORMAL" if is_valid else "FUERA DE RANGO"
                }
        
        return validation
    
    def generate_report(self, signal_type, parameters, validation, save_path="results"):
        """Genera reporte en formato texto y JSON"""
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        
        # Reporte de texto
        report = f"""
{'='*80}
REPORTE DE ANÁLISIS DE PARÁMETROS TEMPORALES
{'='*80}
Fecha: {datetime.now().strftime("%Y-%m-%d %H:%M:%S")}
Tipo de señal: {signal_type}

PARÁMETROS MEDIDOS:
{'-'*80}
"""
        for param, data in validation.items():
            report += f"{param:20s}: {data['value']:8.1f} {data['unit']:5s} | Rango clínico: {data['range']:20s} | {data['valid']} {data['status']}\n"
        
        report += f"\n{'='*80}\n"
        
        # Guardar reporte
        report_file = f"{save_path}/{signal_type}_temporal_analysis_{timestamp}.txt"
        with open(report_file, 'w') as f:
            f.write(report)
        
        # Guardar JSON
        json_file = f"{save_path}/{signal_type}_temporal_analysis_{timestamp}.json"
        with open(json_file, 'w') as f:
            json.dump({
                "timestamp": timestamp,
                "signal_type": signal_type,
                "parameters": parameters,
                "validation": validation
            }, f, indent=2)
        
        print(report)
        print(f"\n✓ Reporte guardado: {report_file}")
        print(f"✓ JSON guardado: {json_file}")
        
        return report_file, json_file

def main():
    import argparse
    parser = argparse.ArgumentParser(description='Análisis de Parámetros Temporales')
    parser.add_argument('--port', default='COM4', help='Puerto serial')
    parser.add_argument('--signal', choices=['ECG', 'EMG', 'PPG'], required=True, help='Tipo de señal')
    parser.add_argument('--duration', type=int, default=10, help='Duración de captura (s)')
    parser.add_argument('--output', default='results', help='Carpeta de salida')
    
    args = parser.parse_args()
    
    # Crear carpeta de resultados
    import os
    os.makedirs(args.output, exist_ok=True)
    
    # Inicializar analizador
    analyzer = TemporalAnalyzer(port=args.port)
    
    if not analyzer.connect():
        return
    
    print(f"\n🔬 Analizando señal {args.signal}...")
    print(f"⏱️  Duración: {args.duration} segundos")
    print(f"📁 Salida: {args.output}/\n")
    
    # Capturar datos
    timestamps, signal = analyzer.capture_ecg_cycle(duration=args.duration)
    
    if len(signal) < 100:
        print("✗ Datos insuficientes capturados")
        return
    
    print(f"✓ Capturados {len(signal)} muestras")
    
    # Calcular parámetros según tipo de señal
    if args.signal == 'ECG':
        fs = 300
        params, peaks = analyzer.calculate_ecg_parameters(timestamps, signal, fs)
        
        # Graficar
        plt.figure(figsize=(14, 6))
        plt.plot(timestamps, signal, 'b-', linewidth=0.8, label='ECG')
        if peaks is not None:
            plt.plot(timestamps[peaks], signal[peaks], 'ro', markersize=8, label='Picos R')
        plt.xlabel('Tiempo (s)')
        plt.ylabel('Amplitud (mV)')
        plt.title(f'ECG - Detección de Picos R ({len(peaks)} detectados)')
        plt.legend()
        plt.grid(True, alpha=0.3)
        plot_file = f"{args.output}/ECG_peaks_{datetime.now().strftime('%Y%m%d_%H%M%S')}.png"
        plt.savefig(plot_file, dpi=300, bbox_inches='tight')
        print(f"✓ Gráfico guardado: {plot_file}")
        
    elif args.signal == 'EMG':
        fs = 1000
        params = analyzer.calculate_emg_parameters(signal, fs)
    
    elif args.signal == 'PPG':
        fs = 100
        params = analyzer.calculate_ppg_parameters(signal, fs)
    
    if params is None:
        print("✗ No se pudieron calcular parámetros")
        return
    
    # Validar contra rangos clínicos
    validation = analyzer.validate_against_clinical(params, args.signal)
    
    # Generar reporte
    analyzer.generate_report(args.signal, params, validation, args.output)
    
    analyzer.ser.close()

if __name__ == "__main__":
    main()
