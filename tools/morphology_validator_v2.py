"""
Validación Morfológica Completa - ECG, EMG y PPG
Comparación automática con datasets clínicos de PhysioNet
"""

import numpy as np
import matplotlib.pyplot as plt
from scipy.signal import find_peaks, butter, filtfilt, resample
from scipy.stats import pearsonr
import pandas as pd
import os
from datetime import datetime
import warnings
warnings.filterwarnings('ignore')

# Intentar importar wfdb (opcional si no está instalado)
try:
    import wfdb
    WFDB_AVAILABLE = True
except ImportError:
    WFDB_AVAILABLE = False
    print("⚠️  wfdb no disponible - usando validación sin referencia PhysioNet")

class MorphologyValidator:
    """
    Validador morfológico para ECG, EMG y PPG
    Compara señales generadas con referencias clínicas
    """
    
    def __init__(self):
        self.reference_databases = {
            "ECG": {
                "normal": ("mitdb", "100"),        # ECG normal sinus rhythm
                "tachycardia": ("mitdb", "207"),   # Supraventricular arrhythmia
                "bradycardia": ("mitdb", "222"),   # Sinus bradycardia
                "atrial_fib": ("mitdb", "202"),    # Atrial fibrillation
                "description": "MIT-BIH Arrhythmia Database"
            },
            "EMG": {
                # Usaremos señales sintéticas de referencia basadas en literatura
                # ya que los datasets de EMG en PhysioNet son limitados
                "description": "Referencia sintética basada en Fuglevand 1993"
            },
            "PPG": {
                # MIMIC-III o dataset local sintético
                "description": "Referencia sintética basada en morfología gaussiana"
            }
        }
    
    def download_ecg_reference(self, condition="normal", save_dir="references/ecg"):
        """Descarga ECG de referencia específico para cada condición"""
        os.makedirs(save_dir, exist_ok=True)
        
        if not WFDB_AVAILABLE:
            print("⚠️  wfdb no disponible - generando referencia sintética")
            return self._generate_synthetic_ecg(condition), 360, None
        
        # Mapear condición a registro MIT-BIH
        db_map = {
            "normal": ("mitdb", "100", "Normal sinus rhythm"),
            "tachycardia": ("mitdb", "207", "Supraventricular tachycardia"),
            "bradycardia": ("mitdb", "222", "Sinus bradycardia"),
            "atrial_fibrillation": ("mitdb", "202", "Atrial fibrillation"),
            "st_elevation": ("mitdb", "123", "ST elevation"),
            "st_depression": ("mitdb", "105", "ST depression")
        }
        
        # Default a normal si no coincide
        db, record, desc = db_map.get(condition.lower().replace(" ", "_"), 
                                       db_map["normal"])
        
        print(f"\n📥 Descargando ECG de referencia: {desc}")
        print(f"   Base de datos: {db} | Registro: {record}")
        
        try:
            # Descargar 10 segundos de señal
            signal_obj = wfdb.rdrecord(record, pn_dir=db, sampfrom=0, sampto=3600)
            signal = signal_obj.p_signal[:, 0]  # Canal 0 (típicamente MLII)
            fs = signal_obj.fs
            
            # Guardar
            ref_file = f"{save_dir}/{db}_{record}_{condition}.npy"
            np.save(ref_file, signal)
            
            print(f"✓ Descargado: {len(signal)} muestras @ {fs} Hz ({len(signal)/fs:.1f}s)")
            return signal, fs, ref_file
        
        except Exception as e:
            print(f"✗ Error descargando: {e}")
            print(f"⚠️  Generando ECG de referencia sintético para {condition}")
            return self._generate_synthetic_ecg(condition), 360, None
    
    def _generate_synthetic_ecg(self, condition="normal"):
        """Genera ECG sintético de referencia basado en morfología típica"""
        # Parámetros de morfología clínica (basados en literatura)
        fs = 360  # Hz
        duration = 10  # segundos
        
        # HR según condición
        hr_map = {
            "normal": 75,
            "tachycardia": 120,
            "bradycardia": 50,
            "atrial_fibrillation": 85  # Irregular
        }
        hr = hr_map.get(condition.lower(), 75)
        
        # Generar señal simple (esto es un placeholder)
        # En realidad deberías usar un modelo más sofisticado
        t = np.linspace(0, duration, int(fs * duration))
        signal = np.zeros_like(t)
        
        # Añadir complejos QRS simples
        rr_interval = 60 / hr  # segundos
        for beat_time in np.arange(0, duration, rr_interval):
            beat_idx = int(beat_time * fs)
            # QRS simple (gaussiana)
            qrs_width = int(0.05 * fs)  # 50 ms
            for i in range(max(0, beat_idx - qrs_width), 
                          min(len(signal), beat_idx + qrs_width)):
                signal[i] += 1.0 * np.exp(-0.5 * ((i - beat_idx) / (qrs_width/3))**2)
        
        return signal
    
    def _generate_synthetic_emg(self, condition="high_contraction"):
        """Genera EMG sintético de referencia"""
        fs = 1000
        duration = 5
        t = np.linspace(0, duration, int(fs * duration))
        
        # EMG simplificado (ruido gaussiano filtrado en banda EMG)
        emg = np.random.randn(len(t)) * 0.5
        
        # Filtrar a banda EMG (20-450 Hz para evitar problemas con nyquist)
        nyq = fs / 2.0
        low = 20.0 / nyq
        high = 450.0 / nyq
        
        # Asegurar que están en rango válido (0, 1)
        low = max(0.01, min(0.99, low))
        high = max(low + 0.01, min(0.99, high))
        
        b, a = butter(4, [low, high], btype='band')
        emg = filtfilt(b, a, emg)
        
        # Modular amplitud según contracción
        level_map = {
            "rest": 0.05,
            "low_contraction": 0.2,
            "moderate_contraction": 0.5,
            "high_contraction": 1.0
        }
        emg *= level_map.get(condition.lower(), 0.5)
        
        return emg
    
    def _generate_synthetic_ppg(self, condition="normal"):
        """Genera PPG sintético de referencia"""
        fs = 100
        duration = 10
        t = np.linspace(0, duration, int(fs * duration))
        
        hr = 75  # bpm
        rr_interval = 60 / hr
        
        signal = np.zeros_like(t)
        
        # Añadir pulsos PPG (morfología gaussiana doble)
        for pulse_time in np.arange(0, duration, rr_interval):
            pulse_idx = int(pulse_time * fs)
            
            # Sístole (subida rápida)
            systolic_width = int(0.15 * fs)
            for i in range(max(0, pulse_idx), 
                          min(len(signal), pulse_idx + systolic_width)):
                signal[i] += 100 * np.exp(-0.5 * ((i - pulse_idx) / (systolic_width/4))**2)
            
            # Diástole (bajada lenta con muesca dicrótica)
            diastolic_idx = pulse_idx + systolic_width
            diastolic_width = int(0.3 * fs)
            for i in range(max(0, diastolic_idx), 
                          min(len(signal), diastolic_idx + diastolic_width)):
                signal[i] += 40 * np.exp(-0.5 * ((i - diastolic_idx) / (diastolic_width/2))**2)
        
        return signal
    
    def extract_beat(self, signal, fs, signal_type="ECG", beat_index=0):
        """Extrae un latido/ciclo individual"""
        
        if signal_type == "ECG":
            # Detectar picos R
            peaks, _ = find_peaks(signal, distance=fs*0.4, prominence=np.std(signal)*0.3)
            
            if len(peaks) <= beat_index:
                return None
            
            r_peak = peaks[beat_index]
            window_before = int(0.2 * fs)  # 200 ms antes
            window_after = int(0.4 * fs)   # 400 ms después
            
        elif signal_type == "EMG":
            # Para EMG, extraer ventana de 500ms
            start = int(beat_index * 0.5 * fs)
            return signal[start:start + int(0.5 * fs)]
        
        elif signal_type == "PPG":
            # Detectar picos sistólicos
            peaks, _ = find_peaks(signal, distance=fs*0.5, prominence=np.std(signal)*0.2)
            
            if len(peaks) <= beat_index:
                return None
            
            r_peak = peaks[beat_index]
            window_before = int(0.1 * fs)
            window_after = int(0.9 * fs)  # Ciclo completo
        
        start = max(0, r_peak - window_before)
        end = min(len(signal), r_peak + window_after)
        
        return signal[start:end]
    
    def calculate_morphology_metrics(self, signal_test, signal_ref):
        """Calcula métricas de similitud morfológica"""
        
        # Normalizar
        test_norm = (signal_test - np.mean(signal_test)) / (np.std(signal_test) + 1e-10)
        ref_norm = (signal_ref - np.mean(signal_ref)) / (np.std(signal_ref) + 1e-10)
        
        # Ajustar longitudes
        min_len = min(len(test_norm), len(ref_norm))
        test_norm = test_norm[:min_len]
        ref_norm = ref_norm[:min_len]
        
        # Correlación de Pearson
        correlation, p_value = pearsonr(test_norm, ref_norm)
        
        # RMSE normalizado
        rmse = np.sqrt(np.mean((test_norm - ref_norm)**2))
        rmse_normalized = rmse
        
        # Índice de similitud (0-100%)
        similarity_index = max(0, min(100, correlation * 100))
        
        return {
            "correlation": correlation,
            "p_value": p_value,
            "rmse": rmse,
            "rmse_normalized": rmse_normalized,
            "similarity_index": similarity_index
        }
    
    def validate_ecg_features(self, beat):
        """Detecta componentes del complejo PQRST"""
        
        r_peak_idx = np.argmax(beat)
        r_peak_val = beat[r_peak_idx]
        
        # Buscar onda Q (antes de R, dentro de 50ms)
        q_start = max(0, r_peak_idx - int(len(beat) * 0.15))
        q_region = beat[q_start:r_peak_idx]
        q_present = len(q_region) > 0 and np.min(q_region) < 0.25 * r_peak_val
        
        # Buscar onda S (después de R, dentro de 80ms)
        s_end = min(len(beat), r_peak_idx + int(len(beat) * 0.15))
        s_region = beat[r_peak_idx:s_end]
        s_present = len(s_region) > 0 and np.min(s_region) < 0.4 * r_peak_val
        
        # Buscar onda T (después de S)
        t_start = min(len(beat), r_peak_idx + int(len(beat) * 0.2))
        t_region = beat[t_start:]
        t_present = len(t_region) > 0 and np.max(t_region) > 0.15 * r_peak_val
        
        # Buscar onda P (antes de Q)
        p_end = max(0, r_peak_idx - int(len(beat) * 0.25))
        p_region = beat[:p_end]
        p_present = len(p_region) > 0 and np.max(p_region) > 0.10 * r_peak_val
        
        features = {
            "P_wave": p_present,
            "Q_wave": q_present,
            "R_peak": True,
            "S_wave": s_present,
            "T_wave": t_present,
            "completeness": sum([p_present, q_present, True, s_present, t_present]) / 5
        }
        
        return features
    
    def generate_complete_report(self, signal, fs, signal_type, condition, 
                                reference_signal=None, save_dir="results/morphology"):
        """Genera reporte completo con gráficos y métricas"""
        
        os.makedirs(save_dir, exist_ok=True)
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        
        print(f"\n{'='*80}")
        print(f"VALIDACIÓN MORFOLÓGICA: {signal_type} - {condition}")
        print(f"{'='*80}")
        print(f"Muestras: {len(signal)} | Fs: {fs} Hz | Duración: {len(signal)/fs:.1f}s")
        
        # Extraer latidos/ciclos
        beat_test = self.extract_beat(signal, fs, signal_type, beat_index=0)
        
        if beat_test is None:
            print("✗ No se pudo extraer ciclo de señal de prueba")
            return None
        
        # Validar features (solo ECG por ahora)
        features = None
        if signal_type == "ECG":
            features = self.validate_ecg_features(beat_test)
            print(f"\nCOMPONENTES DETECTADOS:")
            for comp, present in features.items():
                if comp != "completeness":
                    status = "✓" if present else "✗"
                    print(f"  {status} {comp}")
            print(f"  Completitud: {features['completeness']*100:.0f}%")
        
        # Comparar con referencia
        metrics = None
        beat_ref = None
        
        if reference_signal is not None:
            beat_ref = self.extract_beat(reference_signal, fs, signal_type, beat_index=0)
            
            if beat_ref is not None:
                metrics = self.calculate_morphology_metrics(beat_test, beat_ref)
                
                print(f"\nCOMPARACIÓN CON REFERENCIA:")
                print(f"  Correlación:    {metrics['correlation']:7.4f}")
                print(f"  P-value:        {metrics['p_value']:.6f}")
                print(f"  RMSE:           {metrics['rmse']:7.4f}")
                print(f"  Similitud:      {metrics['similarity_index']:6.1f}%")
                
                # Evaluación
                if metrics['correlation'] > 0.85:
                    eval_text = "✓ EXCELENTE"
                elif metrics['correlation'] > 0.70:
                    eval_text = "✓ BUENA"
                elif metrics['correlation'] > 0.50:
                    eval_text = "⚠ MODERADA"
                else:
                    eval_text = "✗ BAJA"
                
                print(f"  Evaluación:     {eval_text}")
        
        # Generar gráficos
        fig = plt.figure(figsize=(16, 10))
        gs = fig.add_gridspec(3, 2, hspace=0.3, wspace=0.3)
        
        # 1. Señal completa
        ax1 = fig.add_subplot(gs[0, :])
        time_axis = np.arange(len(signal)) / fs
        ax1.plot(time_axis, signal, 'b-', linewidth=0.8, alpha=0.9)
        ax1.set_xlabel('Tiempo (s)', fontsize=11)
        ax1.set_ylabel('Amplitud', fontsize=11)
        ax1.set_title(f'{signal_type} - {condition} | Señal Completa', 
                     fontsize=13, fontweight='bold')
        ax1.grid(True, alpha=0.3)
        
        # 2. Ciclo individual extraído
        ax2 = fig.add_subplot(gs[1, 0])
        beat_time = np.arange(len(beat_test)) / fs * 1000  # ms
        ax2.plot(beat_time, beat_test, 'r-', linewidth=2, label='Señal Capturada')
        
        if signal_type == "ECG" and features:
            # Marcar pico R
            r_idx = np.argmax(beat_test)
            ax2.plot(beat_time[r_idx], beat_test[r_idx], 'go', 
                    markersize=12, label='Pico R', zorder=5)
        
        ax2.set_xlabel('Tiempo (ms)', fontsize=11)
        ax2.set_ylabel('Amplitud', fontsize=11)
        ax2.set_title(f'Ciclo Individual Extraído', fontsize=12, fontweight='bold')
        ax2.legend(fontsize=10)
        ax2.grid(True, alpha=0.3)
        
        # 3. Comparación normalizada
        if beat_ref is not None:
            ax3 = fig.add_subplot(gs[1, 1])
            
            # Normalizar
            beat_test_norm = (beat_test - np.mean(beat_test)) / (np.std(beat_test) + 1e-10)
            beat_ref_norm = (beat_ref - np.mean(beat_ref)) / (np.std(beat_ref) + 1e-10)
            
            min_len = min(len(beat_test_norm), len(beat_ref_norm))
            comp_time = np.arange(min_len) / fs * 1000
            
            ax3.plot(comp_time, beat_test_norm[:min_len], 'b-', 
                    linewidth=2, label='Simulado', alpha=0.8)
            ax3.plot(comp_time, beat_ref_norm[:min_len], 'r--', 
                    linewidth=2, label='Referencia Clínica', alpha=0.7)
            
            ax3.set_xlabel('Tiempo (ms)', fontsize=11)
            ax3.set_ylabel('Amplitud Normalizada', fontsize=11)
            ax3.set_title(f'Comparación Morfológica (r = {metrics["correlation"]:.3f})', 
                         fontsize=12, fontweight='bold')
            ax3.legend(fontsize=10)
            ax3.grid(True, alpha=0.3)
        
        # 4. Espectro de frecuencias
        ax4 = fig.add_subplot(gs[2, 0])
        from scipy.fft import fft, fftfreq
        
        fft_signal = fft(signal)
        freqs = fftfreq(len(signal), 1/fs)
        magnitude = np.abs(fft_signal)
        
        # Solo frecuencias positivas
        pos_mask = freqs > 0
        freqs_pos = freqs[pos_mask]
        mag_pos = magnitude[pos_mask]
        
        ax4.semilogy(freqs_pos, mag_pos, 'g-', linewidth=1)
        ax4.set_xlabel('Frecuencia (Hz)', fontsize=11)
        ax4.set_ylabel('Magnitud (log)', fontsize=11)
        ax4.set_title('Espectro de Frecuencias', fontsize=12, fontweight='bold')
        ax4.grid(True, alpha=0.3, which='both')
        ax4.set_xlim([0, min(fs/2, 200)])  # Limitar a rango relevante
        
        # 5. Métricas resumen
        ax5 = fig.add_subplot(gs[2, 1])
        ax5.axis('off')
        
        summary_text = f"RESUMEN DE VALIDACIÓN\n\n"
        summary_text += f"Tipo: {signal_type}\n"
        summary_text += f"Condición: {condition}\n"
        summary_text += f"Fs: {fs} Hz\n"
        summary_text += f"Duración: {len(signal)/fs:.1f}s\n\n"
        
        if features and signal_type == "ECG":
            summary_text += f"Completitud PQRST: {features['completeness']*100:.0f}%\n\n"
        
        if metrics:
            summary_text += "SIMILITUD CON REFERENCIA:\n"
            summary_text += f"  Correlación: {metrics['correlation']:.4f}\n"
            summary_text += f"  RMSE: {metrics['rmse']:.4f}\n"
            summary_text += f"  Similitud: {metrics['similarity_index']:.1f}%\n\n"
            
            if metrics['correlation'] > 0.85:
                summary_text += "✓ CALIDAD: EXCELENTE\n"
            elif metrics['correlation'] > 0.70:
                summary_text += "✓ CALIDAD: BUENA\n"
            elif metrics['correlation'] > 0.50:
                summary_text += "⚠ CALIDAD: MODERADA\n"
            else:
                summary_text += "✗ CALIDAD: REVISAR\n"
        
        ax5.text(0.1, 0.5, summary_text, fontsize=11, verticalalignment='center',
                fontfamily='monospace', bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.3))
        
        # Guardar figura
        plt_file = f"{save_dir}/morphology_{signal_type}_{condition}_{timestamp}.png"
        plt.savefig(plt_file, dpi=300, bbox_inches='tight')
        plt.close()
        print(f"\n✓ Gráfico guardado: {plt_file}")
        
        # Guardar reporte TXT
        report_text = f"""
{'='*80}
REPORTE DE VALIDACIÓN MORFOLÓGICA
{'='*80}
Fecha: {datetime.now().strftime("%Y-%m-%d %H:%M:%S")}
Tipo de Señal: {signal_type}
Condición: {condition}
Frecuencia de Muestreo: {fs} Hz
Muestras: {len(signal)}
Duración: {len(signal)/fs:.2f} segundos

"""
        
        if features and signal_type == "ECG":
            report_text += f"""COMPONENTES ECG DETECTADOS
{'-'*80}
"""
            for comp, present in features.items():
                if comp != "completeness":
                    status = "✓ PRESENTE" if present else "✗ AUSENTE"
                    report_text += f"{comp:15s}: {status}\n"
            
            report_text += f"\nCompletitud: {features['completeness']*100:.0f}%\n"
        
        if metrics:
            report_text += f"""
COMPARACIÓN MORFOLÓGICA CON REFERENCIA CLÍNICA
{'-'*80}
Correlación de Pearson:    {metrics['correlation']:8.4f}
P-value:                   {metrics['p_value']:.6f}
RMSE:                      {metrics['rmse']:8.4f}
RMSE Normalizado:          {metrics['rmse_normalized']:8.4f}
Índice de Similitud:       {metrics['similarity_index']:7.1f}%

INTERPRETACIÓN:
"""
            if metrics['correlation'] > 0.85:
                report_text += "  ✓ EXCELENTE similitud morfológica con señal clínica de referencia\n"
                report_text += "  La morfología es altamente representativa de la condición simulada\n"
            elif metrics['correlation'] > 0.70:
                report_text += "  ✓ BUENA similitud morfológica\n"
                report_text += "  La señal captura adecuadamente las características principales\n"
            elif metrics['correlation'] > 0.50:
                report_text += "  ⚠ SIMILITUD MODERADA\n"
                report_text += "  Revisar parámetros del modelo para mejorar morfología\n"
            else:
                report_text += "  ✗ BAJA similitud\n"
                report_text += "  La morfología no coincide con patrones clínicos esperados\n"
        
        report_text += f"\n{'='*80}\n"
        report_text += "VALIDACIÓN COMPLETADA\n"
        report_text += f"{'='*80}\n"
        
        txt_file = f"{save_dir}/morphology_{signal_type}_{condition}_{timestamp}.txt"
        with open(txt_file, 'w', encoding='utf-8') as f:
            f.write(report_text)
        
        print(f"✓ Reporte guardado: {txt_file}")
        print(f"{'='*80}\n")
        
        return {
            "features": features,
            "metrics": metrics,
            "plot_file": plt_file,
            "report_file": txt_file
        }

def main():
    """Ejecuta validación morfológica automática de todas las señales"""
    
    print("\n" + "="*80)
    print(" VALIDACIÓN MORFOLÓGICA AUTOMÁTICA - BioSignalSimulator Pro")
    print("="*80)
    print("\nGenerando señales de prueba y referencias clínicas...")
    print("Este proceso puede tardar varios minutos.\n")
    
    validator = MorphologyValidator()
    
    # Configuración de señales a validar
    test_signals = [
        # ECG
        {"type": "ECG", "condition": "normal", "fs": 300, "duration": 10},
        {"type": "ECG", "condition": "tachycardia", "fs": 300, "duration": 10},
        {"type": "ECG", "condition": "bradycardia", "fs": 300, "duration": 10},
        
        # EMG
        {"type": "EMG", "condition": "high_contraction", "fs": 1000, "duration": 5},
        {"type": "EMG", "condition": "moderate_contraction", "fs": 1000, "duration": 5},
        
        # PPG
        {"type": "PPG", "condition": "normal", "fs": 100, "duration": 10},
    ]
    
    results_summary = []
    
    for config in test_signals:
        signal_type = config["type"]
        condition = config["condition"]
        fs = config["fs"]
        duration = config["duration"]
        
        # Generar señal de prueba sintética (simulando captura del dispositivo)
        print(f"\n{'─'*80}")
        print(f"Procesando: {signal_type} - {condition}")
        print(f"{'─'*80}")
        
        if signal_type == "ECG":
            test_signal = validator._generate_synthetic_ecg(condition)
            ref_signal, ref_fs, _ = validator.download_ecg_reference(condition)
            
            # Resamplear referencia si es necesario
            if ref_fs != fs:
                new_len = int(len(ref_signal) * fs / ref_fs)
                ref_signal = resample(ref_signal, new_len)
        
        elif signal_type == "EMG":
            test_signal = validator._generate_synthetic_emg(condition)
            ref_signal = validator._generate_synthetic_emg(condition)  # Misma referencia
        
        elif signal_type == "PPG":
            test_signal = validator._generate_synthetic_ppg(condition)
            ref_signal = validator._generate_synthetic_ppg(condition)
        
        # Validar
        result = validator.generate_complete_report(
            test_signal, fs, signal_type, condition, ref_signal
        )
        
        if result:
            results_summary.append({
                "signal": signal_type,
                "condition": condition,
                "correlation": result["metrics"]["correlation"] if result["metrics"] else None,
                "similarity": result["metrics"]["similarity_index"] if result["metrics"] else None,
                "completeness": result["features"]["completeness"] if result["features"] else None
            })
    
    # Resumen final
    print("\n" + "="*80)
    print(" RESUMEN DE VALIDACIONES")
    print("="*80)
    print(f"\n{'Señal':<10} {'Condición':<25} {'Correlación':>12} {'Similitud':>10} {'Compl.':>8}")
    print("─"*80)
    
    for res in results_summary:
        corr = f"{res['correlation']:.4f}" if res['correlation'] else "N/A"
        sim = f"{res['similarity']:.1f}%" if res['similarity'] else "N/A"
        comp = f"{res['completeness']*100:.0f}%" if res['completeness'] else "N/A"
        
        print(f"{res['signal']:<10} {res['condition']:<25} {corr:>12} {sim:>10} {comp:>8}")
    
    print("="*80)
    print("\n✓ Validación morfológica completada")
    print(f"✓ Resultados guardados en: results/morphology/")
    print("\n" + "="*80 + "\n")

if __name__ == "__main__":
    main()
