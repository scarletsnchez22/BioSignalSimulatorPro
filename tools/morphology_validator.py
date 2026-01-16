"""
Validación Morfológica Automática
Compara señales capturadas con referencias de PhysioNet
"""

import numpy as np
import wfdb
import matplotlib.pyplot as plt
from scipy.signal import correlate, find_peaks
from scipy.stats import pearsonr
import pandas as pd
import serial
import time
import os
from datetime import datetime

class MorphologyValidator:
    """
    Valida morfología de señales biomédicas comparando con PhysioNet
    """
    
    def __init__(self):
        self.reference_databases = {
            "ECG": {
                "db": "mitdb",  # MIT-BIH Arrhythmia Database
                "records": ["100", "101", "102", "103", "104"],
                "description": "ECG normal de referencia"
            },
            "EMG": {
                "db": "emgdb",  # EMG Database
                "records": ["emg_healthy"],
                "description": "EMG saludable de referencia"
            }
        }
    
    def download_reference_ecg(self, save_dir="references/ecg"):
        """Descarga ECG de referencia de PhysioNet"""
        os.makedirs(save_dir, exist_ok=True)
        
        print("📥 Descargando ECG de referencia de MIT-BIH...")
        
        try:
            # Descargar primer registro de MIT-BIH (100)
            record = wfdb.rdrecord('100', pn_dir='mitdb', sampfrom=0, sampto=10000)
            signal = record.p_signal[:, 0]  # Canal 0 (MLII)
            fs = record.fs
            
            # Guardar como NPY
            ref_file = f"{save_dir}/mitdb_100_ecg.npy"
            np.save(ref_file, signal)
            
            print(f"✓ Descargado: {ref_file}")
            print(f"  Fs: {fs} Hz, Muestras: {len(signal)}, Duración: {len(signal)/fs:.1f}s")
            
            return signal, fs, ref_file
        
        except Exception as e:
            print(f"✗ Error descargando: {e}")
            print("⚠️  Se usará validación sin referencia PhysioNet")
            return None, None, None
    
    def extract_heartbeat(self, signal, fs, beat_index=0):
        """Extrae un latido individual de la señal"""
        # Detectar picos R
        peaks, _ = find_peaks(signal, distance=fs*0.5, prominence=np.std(signal)*0.5)
        
        if len(peaks) <= beat_index:
            return None
        
        # Extraer ventana alrededor del pico R
        r_peak = peaks[beat_index]
        window_before = int(0.2 * fs)  # 200 ms antes
        window_after = int(0.4 * fs)   # 400 ms después
        
        start = max(0, r_peak - window_before)
        end = min(len(signal), r_peak + window_after)
        
        heartbeat = signal[start:end]
        
        return heartbeat, r_peak
    
    def calculate_morphology_similarity(self, signal_test, signal_ref):
        """
        Calcula similitud morfológica entre dos señales
        Retorna: correlación, RMSE normalizado, índice de similitud
        """
        # Normalizar señales
        signal_test_norm = (signal_test - np.mean(signal_test)) / np.std(signal_test)
        signal_ref_norm = (signal_ref - np.mean(signal_ref)) / np.std(signal_ref)
        
        # Ajustar longitudes
        min_len = min(len(signal_test_norm), len(signal_ref_norm))
        signal_test_norm = signal_test_norm[:min_len]
        signal_ref_norm = signal_ref_norm[:min_len]
        
        # Correlación de Pearson
        correlation, p_value = pearsonr(signal_test_norm, signal_ref_norm)
        
        # RMSE normalizado
        rmse = np.sqrt(np.mean((signal_test_norm - signal_ref_norm)**2))
        rmse_normalized = rmse / (np.std(signal_ref_norm) + 1e-10)
        
        # Índice de similitud (0-100%)
        similarity_index = max(0, 100 * (1 - rmse_normalized))
        
        return {
            "correlation": correlation,
            "p_value": p_value,
            "rmse": rmse,
            "rmse_normalized": rmse_normalized,
            "similarity_index": similarity_index
        }
    
    def validate_ecg_features(self, signal, fs):
        """
        Valida características morfológicas de ECG
        - Presencia de ondas P, Q, R, S, T
        - Relación de amplitudes
        - Duración de segmentos
        """
        # Detectar picos R
        peaks, properties = find_peaks(signal, distance=fs*0.5, prominence=np.std(signal)*0.5)
        
        if len(peaks) == 0:
            return {"valid": False, "reason": "No se detectaron picos R"}
        
        # Extraer un latido
        heartbeat, r_idx = self.extract_heartbeat(signal, fs, beat_index=0)
        
        if heartbeat is None:
            return {"valid": False, "reason": "No se pudo extraer latido"}
        
        # Buscar componentes
        r_peak_val = np.max(heartbeat)
        r_peak_idx = np.argmax(heartbeat)
        
        # Buscar onda Q (antes de R)
        q_region = heartbeat[max(0, r_peak_idx-int(0.04*fs)):r_peak_idx]
        q_present = len(q_region) > 0 and np.min(q_region) < 0.3 * r_peak_val
        
        # Buscar onda S (después de R)
        s_region = heartbeat[r_peak_idx:min(len(heartbeat), r_peak_idx+int(0.08*fs))]
        s_present = len(s_region) > 0 and np.min(s_region) < 0.5 * r_peak_val
        
        # Buscar onda T (después de S)
        t_region = heartbeat[min(len(heartbeat), r_peak_idx+int(0.1*fs)):]
        t_present = len(t_region) > 0 and np.max(t_region) > 0.2 * r_peak_val
        
        validation = {
            "valid": True,
            "features": {
                "Q_wave_present": q_present,
                "R_peak_present": True,
                "S_wave_present": s_present,
                "T_wave_present": t_present
            },
            "r_amplitude": r_peak_val,
            "heartbeat_duration_ms": len(heartbeat) / fs * 1000
        }
        
        return validation
    
    def generate_morphology_report(self, signal, fs, signal_type, condition, 
                                   reference_signal=None, save_path="results"):
        """Genera reporte completo de validación morfológica"""
        
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        
        # Validar características
        if signal_type == "ECG":
            features = self.validate_ecg_features(signal, fs)
        else:
            features = {"valid": True, "features": {}}
        
        # Comparar con referencia si existe
        similarity = None
        if reference_signal is not None:
            similarity = self.calculate_morphology_similarity(signal, reference_signal)
        
        # Generar gráficos
        fig, axes = plt.subplots(3, 1, figsize=(14, 10))
        
        # 1. Señal completa
        time_axis = np.arange(len(signal)) / fs
        axes[0].plot(time_axis, signal, 'b-', linewidth=0.8)
        axes[0].set_xlabel('Tiempo (s)')
        axes[0].set_ylabel('Amplitud')
        axes[0].set_title(f'{signal_type} - {condition} (Señal Completa)')
        axes[0].grid(True, alpha=0.3)
        
        # 2. Latido individual (si es ECG)
        if signal_type == "ECG":
            heartbeat, r_idx = self.extract_heartbeat(signal, fs, beat_index=0)
            if heartbeat is not None:
                hb_time = np.arange(len(heartbeat)) / fs * 1000  # ms
                axes[1].plot(hb_time, heartbeat, 'r-', linewidth=1.5, label='Latido capturado')
                
                # Marcar componentes
                r_peak_idx = np.argmax(heartbeat)
                axes[1].plot(hb_time[r_peak_idx], heartbeat[r_peak_idx], 'go', 
                           markersize=10, label='Pico R')
                
                axes[1].set_xlabel('Tiempo (ms)')
                axes[1].set_ylabel('Amplitud')
                axes[1].set_title('Latido Individual Extraído')
                axes[1].legend()
                axes[1].grid(True, alpha=0.3)
        
        # 3. Comparación con referencia (si existe)
        if reference_signal is not None and signal_type == "ECG":
            ref_hb, _ = self.extract_heartbeat(reference_signal, fs, beat_index=0)
            if ref_hb is not None and heartbeat is not None:
                # Normalizar para comparar formas
                hb_norm = (heartbeat - np.mean(heartbeat)) / np.std(heartbeat)
                ref_norm = (ref_hb - np.mean(ref_hb)) / np.std(ref_hb)
                
                min_len = min(len(hb_norm), len(ref_norm))
                hb_time = np.arange(min_len) / fs * 1000
                
                axes[2].plot(hb_time, hb_norm[:min_len], 'b-', linewidth=1.5, 
                           label='Simulado (normalizado)')
                axes[2].plot(hb_time, ref_norm[:min_len], 'r--', linewidth=1.5, 
                           label='Referencia MIT-BIH (normalizado)', alpha=0.7)
                axes[2].set_xlabel('Tiempo (ms)')
                axes[2].set_ylabel('Amplitud Normalizada')
                axes[2].set_title(f'Comparación Morfológica (Correlación: {similarity["correlation"]:.3f})')
                axes[2].legend()
                axes[2].grid(True, alpha=0.3)
        
        plt.tight_layout()
        plot_file = f"{save_path}/morphology_{signal_type}_{condition}_{timestamp}.png"
        plt.savefig(plot_file, dpi=300, bbox_inches='tight')
        print(f"✓ Gráfico guardado: {plot_file}")
        
        # Generar reporte de texto
        report = f"""
{'='*80}
REPORTE DE VALIDACIÓN MORFOLÓGICA
{'='*80}
Fecha: {datetime.now().strftime("%Y-%m-%d %H:%M:%S")}
Señal: {signal_type} - {condition}
Fs: {fs} Hz | Muestras: {len(signal)} | Duración: {len(signal)/fs:.1f}s

CARACTERÍSTICAS DETECTADAS
{'-'*80}
"""
        
        if "features" in features:
            for feature, present in features["features"].items():
                status = "✓ PRESENTE" if present else "✗ AUSENTE"
                report += f"{feature:25s}: {status}\n"
        
        if similarity is not None:
            report += f"""
COMPARACIÓN CON REFERENCIA PHYSIONET
{'-'*80}
Correlación de Pearson:    {similarity['correlation']:8.4f}
P-value:                   {similarity['p_value']:8.6f}
RMSE normalizado:          {similarity['rmse_normalized']:8.4f}
Índice de Similitud:       {similarity['similarity_index']:8.1f}%

EVALUACIÓN:
"""
            if similarity['correlation'] > 0.85:
                report += "  ✓ EXCELENTE similitud morfológica con referencia clínica\n"
            elif similarity['correlation'] > 0.70:
                report += "  ✓ BUENA similitud morfológica\n"
            elif similarity['correlation'] > 0.50:
                report += "  ⚠ SIMILITUD MODERADA - Revisar morfología\n"
            else:
                report += "  ✗ BAJA similitud - Morfología no coincide con referencia\n"
        
        report += f"\n{'='*80}\n"
        
        # Guardar reporte
        report_file = f"{save_path}/morphology_{signal_type}_{condition}_{timestamp}.txt"
        with open(report_file, 'w') as f:
            f.write(report)
        
        print(report)
        print(f"✓ Reporte guardado: {report_file}")
        
        return features, similarity, report_file

def main():
    import argparse
    
    parser = argparse.ArgumentParser(description='Validación Morfológica Automática')
    parser.add_argument('--signal', required=True, help='Archivo NPY con señal capturada')
    parser.add_argument('--type', choices=['ECG', 'EMG', 'PPG'], required=True, help='Tipo de señal')
    parser.add_argument('--condition', default='Normal', help='Condición de la señal')
    parser.add_argument('--fs', type=int, default=300, help='Frecuencia de muestreo (Hz)')
    parser.add_argument('--download-ref', action='store_true', help='Descargar referencia de PhysioNet')
    parser.add_argument('--output', default='results/morphology', help='Carpeta de salida')
    
    args = parser.parse_args()
    
    os.makedirs(args.output, exist_ok=True)
    
    validator = MorphologyValidator()
    
    # Cargar señal capturada
    print(f"\n📊 Cargando señal: {args.signal}")
    signal = np.load(args.signal)
    print(f"✓ Cargado: {len(signal)} muestras @ {args.fs} Hz")
    
    # Descargar/cargar referencia
    reference_signal = None
    if args.download_ref and args.type == "ECG":
        reference_signal, ref_fs, ref_file = validator.download_reference_ecg()
        
        if reference_signal is not None and ref_fs != args.fs:
            # Resamplear si es necesario
            from scipy.signal import resample
            new_len = int(len(reference_signal) * args.fs / ref_fs)
            reference_signal = resample(reference_signal, new_len)
            print(f"✓ Referencia resampleada a {args.fs} Hz")
    
    # Generar reporte
    features, similarity, report_file = validator.generate_morphology_report(
        signal, args.fs, args.type, args.condition, reference_signal, args.output
    )

if __name__ == "__main__":
    main()
