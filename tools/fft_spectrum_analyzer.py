#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
FFT Spectrum Analyzer para BioSignalSimulator Pro
=================================================

Análisis espectral riguroso de señales biomédicas (ECG, EMG, PPG)
capturadas vía Serial desde el ESP32.

FUNDAMENTOS TEÓRICOS:
---------------------
1. FFT (Fast Fourier Transform):
   - Transforma señal del dominio temporal al dominio frecuencial
   - DFT: X[k] = Σ x[n] * e^(-j2πkn/N)  para k = 0, 1, ..., N-1
   - Frecuencias: f[k] = k * Fs / N  (resolución = Fs/N Hz)

2. Espectro de potencia (Power Spectral Density):
   - PSD = |X[k]|² / N
   - Unidades: V²/Hz (densidad espectral de potencia)
   
3. Normalización de amplitud:
   - Amplitud RMS = |X[k]| * √2 / N  para señales sinusoidales
   - Factor 2 porque solo usamos frecuencias positivas (rfft)

4. Ventaneo (Windowing):
   - Reduce "spectral leakage" por truncamiento de señal
   - Hanning: buena resolución frecuencial, baja fuga lateral
   - Blackman: menor fuga lateral, menor resolución
   - Rectangular: máxima resolución, máxima fuga

5. Criterio de Nyquist:
   - Fmax detectable = Fs / 2
   - Para reconstruir señal con Fmax, necesitamos Fs > 2*Fmax

Referencias:
- Oppenheim, A.V., Schafer, R.W. "Discrete-Time Signal Processing"
- scipy.fft documentation: https://docs.scipy.org/doc/scipy/tutorial/fft.html
- NumPy FFT: https://numpy.org/doc/stable/reference/routines.fft.html

Autor: BioSignalSimulator Pro Team
Fecha: Enero 2026
"""

import numpy as np
from scipy import signal
from scipy.fft import rfft, rfftfreq
import matplotlib.pyplot as plt
import serial
import serial.tools.list_ports
import time
import argparse
import sys
from datetime import datetime
from collections import deque
from typing import Tuple, Optional, Dict, List
import json


# =============================================================================
# CONSTANTES DE CONFIGURACIÓN
# =============================================================================

# Estándares clínicos de ancho de banda (IEC 60601-2-27, IEC 60601-2-49)
CLINICAL_STANDARDS = {
    'ECG': {
        'bw_min': 0.05,     # Hz - filtro pasa-altos (línea base)
        'bw_max': 150.0,    # Hz - filtro pasa-bajos (diagnóstico)
        'nyquist_min': 300, # Hz - Fs mínimo según Nyquist
        'description': 'ECG diagnóstico (IEC 60601-2-27)'
    },
    'EMG': {
        'bw_min': 20.0,     # Hz - elimina artefactos movimiento
        'bw_max': 500.0,    # Hz - contenido EMG superficie
        'nyquist_min': 1000, # Hz
        'description': 'sEMG superficie (SENIAM guidelines)'
    },
    'PPG': {
        'bw_min': 0.5,      # Hz - elimina deriva DC lenta
        'bw_max': 10.0,     # Hz - armónicos de pulso
        'nyquist_min': 20,  # Hz
        'description': 'PPG fotopletismografía'
    }
}

# Configuración del ESP32
ESP32_CONFIG = {
    'baud_rate': 115200,
    'fs_timer': 2000,  # Hz - frecuencia del timer ISR
}


# =============================================================================
# CLASE PRINCIPAL: FFT Analyzer
# =============================================================================

class FFTSpectrumAnalyzer:
    """
    Analizador espectral FFT para señales biomédicas.
    
    Captura datos del Serial, aplica ventaneo y calcula FFT con
    normalización correcta para obtener amplitudes en unidades físicas.
    """
    
    def __init__(self, port: str, baud: int = 115200, fs: float = 2000.0):
        """
        Inicializa el analizador.
        
        Args:
            port: Puerto COM del ESP32
            baud: Velocidad de comunicación
            fs: Frecuencia de muestreo en Hz
        """
        self.port = port
        self.baud = baud
        self.fs = fs
        self.dt = 1.0 / fs
        self.serial_conn = None
        
        # Buffers de datos
        self.data_buffer: List[float] = []
        self.time_buffer: List[float] = []
        
        # Resultados de análisis
        self.frequencies: np.ndarray = None
        self.spectrum: np.ndarray = None
        self.psd: np.ndarray = None
        
        print(f"╔══════════════════════════════════════════════════════════════╗")
        print(f"║      FFT Spectrum Analyzer - BioSignalSimulator Pro          ║")
        print(f"╠══════════════════════════════════════════════════════════════╣")
        print(f"║  Puerto: {port:10s}  Baud: {baud:6d}  Fs: {fs:6.0f} Hz         ║")
        print(f"╚══════════════════════════════════════════════════════════════╝")
    
    def connect(self) -> bool:
        """Establece conexión Serial con el ESP32."""
        try:
            self.serial_conn = serial.Serial(
                port=self.port,
                baudrate=self.baud,
                timeout=1.0,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE
            )
            time.sleep(2.0)  # Esperar reset del ESP32
            self.serial_conn.reset_input_buffer()
            print(f"[OK] Conectado a {self.port}")
            return True
        except serial.SerialException as e:
            print(f"[ERROR] No se pudo conectar: {e}")
            return False
    
    def disconnect(self):
        """Cierra la conexión Serial."""
        if self.serial_conn and self.serial_conn.is_open:
            self.serial_conn.close()
            print("[OK] Conexión cerrada")
    
    def capture_data(self, duration_sec: float, signal_type: str = 'ECG',
                     show_progress: bool = True) -> Tuple[np.ndarray, float]:
        """
        Captura datos del Serial por un tiempo determinado.
        
        El ESP32 envía datos en formato: "DAC_out: X.XX, ADC_in: Y.YY"
        donde DAC_out es el voltaje de salida del DAC (señal generada).
        
        Args:
            duration_sec: Duración de captura en segundos
            signal_type: Tipo de señal (ECG, EMG, PPG) para ajustar parsing
            show_progress: Mostrar barra de progreso
            
        Returns:
            Tuple con (array de datos, frecuencia de muestreo efectiva)
        """
        if not self.serial_conn or not self.serial_conn.is_open:
            raise RuntimeError("No hay conexión Serial activa")
        
        # Limpiar buffer antes de capturar
        self.serial_conn.reset_input_buffer()
        self.data_buffer = []
        self.time_buffer = []
        
        print(f"\n[CAPTURA] Iniciando captura de {duration_sec:.1f} segundos...")
        print(f"[INFO] Señal: {signal_type} | Fs esperada: {self.fs} Hz")
        
        # Variables de timing
        start_time = time.perf_counter()
        last_sample_time = start_time
        sample_count = 0
        error_count = 0
        expected_samples = int(duration_sec * self.fs)
        
        # Timeout de seguridad
        timeout_time = start_time + duration_sec + 5.0
        
        while time.perf_counter() < start_time + duration_sec:
            if time.perf_counter() > timeout_time:
                print("\n[TIMEOUT] Se excedió el tiempo máximo de captura")
                break
            
            try:
                if self.serial_conn.in_waiting > 0:
                    line = self.serial_conn.readline().decode('utf-8', errors='ignore').strip()
                    
                    # Parsear línea - soporta múltiples formatos:
                    # Formato 1 (VS Code Serial Plotter): ">dac:1.234,adc:1.567"
                    # Formato 2 (Legacy): "DAC_out: X.XX, ADC_in: Y.YY"
                    value = None
                    
                    if line.startswith('>dac:'):
                        try:
                            # Formato: >dac:1.234,adc:1.567
                            parts = line[5:].split(',')  # Quitar ">dac:"
                            value = float(parts[0])
                        except (IndexError, ValueError):
                            pass
                            
                    elif 'DAC_out:' in line:
                        try:
                            parts = line.split(',')
                            dac_part = parts[0].split(':')[1].strip()
                            value = float(dac_part)
                        except (IndexError, ValueError):
                            pass
                    
                    if value is not None:
                        current_time = time.perf_counter() - start_time
                        self.data_buffer.append(value)
                        self.time_buffer.append(current_time)
                        sample_count += 1
                    
                    # Mostrar progreso
                    if show_progress and sample_count % 500 == 0:
                        elapsed = time.perf_counter() - start_time
                        progress = (elapsed / duration_sec) * 100
                        effective_fs = sample_count / elapsed if elapsed > 0 else 0
                        sys.stdout.write(f"\r[PROGRESO] {progress:5.1f}% | Muestras: {sample_count:6d} | Fs efectiva: {effective_fs:.1f} Hz")
                        sys.stdout.flush()
                        
            except Exception as e:
                error_count += 1
                if error_count > 100:
                    print(f"\n[ERROR] Demasiados errores de lectura: {e}")
                    break
        
        # Calcular estadísticas finales
        elapsed = time.perf_counter() - start_time
        effective_fs = sample_count / elapsed if elapsed > 0 else 0
        
        print(f"\n\n[COMPLETO] Captura finalizada")
        print(f"  • Muestras: {sample_count}")
        print(f"  • Tiempo: {elapsed:.2f} s")
        print(f"  • Fs efectiva: {effective_fs:.2f} Hz")
        print(f"  • Errores: {error_count}")
        
        if sample_count < 100:
            print("[ADVERTENCIA] Muy pocas muestras capturadas")
            
        # Actualizar Fs con la medición real
        self.fs = effective_fs
        self.dt = 1.0 / effective_fs if effective_fs > 0 else 1.0
        
        return np.array(self.data_buffer), effective_fs
    
    def compute_fft(self, data: np.ndarray, window: str = 'hanning',
                    detrend: bool = True) -> Dict:
        """
        Calcula la FFT con normalización correcta.
        
        Implementación basada en scipy.fft con correcciones:
        1. Detrending: elimina componente DC (offset medio)
        2. Ventaneo: reduce spectral leakage
        3. rfft: FFT para señales reales (solo frecuencias positivas)
        4. Normalización: amplitud en unidades físicas (V)
        
        Args:
            data: Array de datos temporales
            window: Tipo de ventana ('hanning', 'blackman', 'hamming', 'rectangular')
            detrend: Si True, elimina la media (componente DC)
            
        Returns:
            Dict con frecuencias, espectro de amplitud, PSD y métricas
        """
        N = len(data)
        
        if N < 2:
            raise ValueError("Se necesitan al menos 2 muestras para FFT")
        
        print(f"\n[FFT] Procesando {N} muestras...")
        print(f"  • Resolución frecuencial: {self.fs/N:.4f} Hz")
        print(f"  • Frecuencia máxima (Nyquist): {self.fs/2:.1f} Hz")
        print(f"  • Ventana: {window}")
        print(f"  • Detrend: {detrend}")
        
        # 1. Detrending (eliminar DC)
        if detrend:
            data_mean = np.mean(data)
            data_centered = data - data_mean
            print(f"  • Media removida: {data_mean:.4f} V")
        else:
            data_centered = data
            
        # 2. Aplicar ventana
        if window.lower() == 'hanning':
            w = np.hanning(N)
        elif window.lower() == 'blackman':
            w = np.blackman(N)
        elif window.lower() == 'hamming':
            w = np.hamming(N)
        elif window.lower() == 'rectangular':
            w = np.ones(N)
        else:
            print(f"[WARN] Ventana '{window}' no reconocida, usando Hanning")
            w = np.hanning(N)
        
        # Factor de corrección de amplitud por ventaneo
        # (la ventana reduce la energía, debemos compensar)
        amplitude_correction = N / np.sum(w)  # Coherent gain correction
        
        data_windowed = data_centered * w
        
        # 3. Calcular FFT para señales reales (solo frecuencias positivas)
        # rfft devuelve N//2 + 1 coeficientes complejos
        fft_result = rfft(data_windowed)
        
        # 4. Vector de frecuencias
        freqs = rfftfreq(N, d=self.dt)
        
        # 5. Espectro de amplitud normalizado
        # |X[k]| / N para DC, 2*|X[k]| / N para otras frecuencias
        # (factor 2 porque ignoramos frecuencias negativas simétricas)
        amplitude_spectrum = np.abs(fft_result) * 2.0 / N
        amplitude_spectrum[0] /= 2.0  # DC no se duplica
        if N % 2 == 0:  # Nyquist tampoco se duplica si N es par
            amplitude_spectrum[-1] /= 2.0
        
        # Aplicar corrección de ventana
        amplitude_spectrum *= amplitude_correction
        
        # 6. Power Spectral Density (PSD)
        # PSD = |X[k]|² normalizado, en V²/Hz
        psd = (amplitude_spectrum ** 2) / (self.fs / N)
        
        # 7. Calcular métricas espectrales
        # Frecuencia dominante
        idx_max = np.argmax(amplitude_spectrum[1:]) + 1  # Ignorar DC
        freq_dominant = freqs[idx_max]
        amp_dominant = amplitude_spectrum[idx_max]
        
        # Ancho de banda a -3dB
        threshold_3db = amp_dominant / np.sqrt(2)
        idx_above_threshold = np.where(amplitude_spectrum >= threshold_3db)[0]
        if len(idx_above_threshold) > 1:
            bw_3db = freqs[idx_above_threshold[-1]] - freqs[idx_above_threshold[0]]
        else:
            bw_3db = 0.0
        
        # Frecuencia donde cae el 99% de la energía
        cumulative_energy = np.cumsum(psd)
        total_energy = cumulative_energy[-1]
        idx_99 = np.searchsorted(cumulative_energy, 0.99 * total_energy)
        freq_99_energy = freqs[min(idx_99, len(freqs)-1)]
        
        # Guardar resultados
        self.frequencies = freqs
        self.spectrum = amplitude_spectrum
        self.psd = psd
        
        results = {
            'frequencies': freqs,
            'amplitude_spectrum': amplitude_spectrum,
            'psd': psd,
            'fft_complex': fft_result,
            'N': N,
            'fs': self.fs,
            'freq_resolution': self.fs / N,
            'freq_dominant': freq_dominant,
            'amp_dominant': amp_dominant,
            'bw_3db': bw_3db,
            'freq_99_energy': freq_99_energy,
            'window': window,
            'amplitude_correction': amplitude_correction
        }
        
        print(f"\n[RESULTADOS FFT]")
        print(f"  • Frecuencia dominante: {freq_dominant:.2f} Hz")
        print(f"  • Amplitud máxima: {amp_dominant:.4f} V")
        print(f"  • Ancho de banda (-3dB): {bw_3db:.2f} Hz")
        print(f"  • Frecuencia 99% energía: {freq_99_energy:.2f} Hz")
        
        return results
    
    def analyze_bandwidth(self, results: Dict, signal_type: str) -> Dict:
        """
        Analiza el ancho de banda y lo compara con estándares clínicos.
        
        Args:
            results: Diccionario con resultados de FFT
            signal_type: 'ECG', 'EMG', o 'PPG'
            
        Returns:
            Dict con análisis de ancho de banda
        """
        clinical = CLINICAL_STANDARDS.get(signal_type.upper(), CLINICAL_STANDARDS['ECG'])
        
        freqs = results['frequencies']
        amp = results['amplitude_spectrum']
        
        # Encontrar frecuencia de corte real (-20 dB respecto al máximo)
        max_amp = np.max(amp[1:])  # Ignorar DC
        threshold_20db = max_amp / 10  # -20 dB
        threshold_40db = max_amp / 100  # -40 dB
        
        # Encontrar última frecuencia significativa
        idx_significant = np.where(amp >= threshold_20db)[0]
        if len(idx_significant) > 0:
            freq_cutoff_20db = freqs[idx_significant[-1]]
        else:
            freq_cutoff_20db = 0
            
        idx_significant_40db = np.where(amp >= threshold_40db)[0]
        if len(idx_significant_40db) > 0:
            freq_cutoff_40db = freqs[idx_significant_40db[-1]]
        else:
            freq_cutoff_40db = 0
        
        # Calcular contenido frecuencial en bandas clínicas
        idx_clinical = (freqs >= clinical['bw_min']) & (freqs <= clinical['bw_max'])
        energy_clinical = np.sum(amp[idx_clinical] ** 2)
        energy_total = np.sum(amp[1:] ** 2)  # Sin DC
        pct_clinical = (energy_clinical / energy_total * 100) if energy_total > 0 else 0
        
        analysis = {
            'signal_type': signal_type,
            'clinical_standard': clinical['description'],
            'clinical_bw_min': clinical['bw_min'],
            'clinical_bw_max': clinical['bw_max'],
            'clinical_nyquist_min': clinical['nyquist_min'],
            'measured_cutoff_20db': freq_cutoff_20db,
            'measured_cutoff_40db': freq_cutoff_40db,
            'energy_in_clinical_band_pct': pct_clinical,
            'nyquist_satisfied': self.fs >= clinical['nyquist_min'],
            'recommended_rc_fc': 2 * freq_cutoff_20db,  # Fc = 2 × Fmax
        }
        
        print(f"\n[ANÁLISIS DE ANCHO DE BANDA - {signal_type}]")
        print(f"  • Estándar: {clinical['description']}")
        print(f"  • BW clínico: {clinical['bw_min']:.2f} - {clinical['bw_max']:.2f} Hz")
        print(f"  • Corte medido (-20dB): {freq_cutoff_20db:.2f} Hz")
        print(f"  • Corte medido (-40dB): {freq_cutoff_40db:.2f} Hz")
        print(f"  • Energía en banda clínica: {pct_clinical:.1f}%")
        print(f"  • Fs actual: {self.fs:.0f} Hz | Nyquist mínimo: {clinical['nyquist_min']} Hz")
        print(f"  • Nyquist satisfecho: {'✓ SÍ' if analysis['nyquist_satisfied'] else '✗ NO'}")
        print(f"\n[RECOMENDACIÓN FILTRO RC]")
        print(f"  • Fc sugerida: {analysis['recommended_rc_fc']:.1f} Hz")
        if analysis['recommended_rc_fc'] > 0:
            r_ohm = 1.0 / (2 * np.pi * analysis['recommended_rc_fc'] * 1e-6)
            print(f"  • Con C=1µF: R = {r_ohm:.0f} Ω")
        else:
            print(f"  • Con C=1µF: R = (N/A - Fc=0)")
        
        return analysis
    
    def plot_results(self, data: np.ndarray, results: Dict, 
                     signal_type: str, save_path: str = None):
        """
        Genera gráficos completos del análisis.
        
        4 subplots:
        1. Señal temporal
        2. Espectro de amplitud (escala lineal)
        3. Espectro de amplitud (escala logarítmica dB)
        4. PSD (Power Spectral Density)
        """
        fig, axes = plt.subplots(2, 2, figsize=(14, 10))
        fig.suptitle(f'Análisis Espectral FFT - {signal_type}\n'
                     f'N={results["N"]}, Fs={results["fs"]:.1f} Hz, '
                     f'Resolución={results["freq_resolution"]:.4f} Hz',
                     fontsize=12, fontweight='bold')
        
        clinical = CLINICAL_STANDARDS.get(signal_type.upper(), CLINICAL_STANDARDS['ECG'])
        
        # 1. Señal temporal
        ax1 = axes[0, 0]
        t = np.arange(len(data)) / results['fs']
        ax1.plot(t, data, 'b-', linewidth=0.5, alpha=0.8)
        ax1.set_xlabel('Tiempo (s)')
        ax1.set_ylabel('Amplitud (V)')
        ax1.set_title('Señal Temporal')
        ax1.grid(True, alpha=0.3)
        ax1.set_xlim([0, min(2.0, t[-1])])  # Mostrar máximo 2 segundos
        
        # 2. Espectro de amplitud (lineal)
        ax2 = axes[0, 1]
        freqs = results['frequencies']
        amp = results['amplitude_spectrum']
        
        ax2.plot(freqs, amp, 'b-', linewidth=0.8)
        ax2.axvline(clinical['bw_max'], color='r', linestyle='--', 
                    label=f'BW clínico: {clinical["bw_max"]} Hz')
        ax2.axvline(results['freq_99_energy'], color='g', linestyle=':', 
                    label=f'99% energía: {results["freq_99_energy"]:.1f} Hz')
        ax2.fill_between([clinical['bw_min'], clinical['bw_max']], 
                         0, max(amp), alpha=0.2, color='green',
                         label='Banda clínica')
        ax2.set_xlabel('Frecuencia (Hz)')
        ax2.set_ylabel('Amplitud (V)')
        ax2.set_title('Espectro de Amplitud (Lineal)')
        ax2.set_xlim([0, min(results['fs']/2, clinical['bw_max']*3)])
        ax2.legend(loc='upper right', fontsize=8)
        ax2.grid(True, alpha=0.3)
        
        # 3. Espectro en dB
        ax3 = axes[1, 0]
        amp_db = 20 * np.log10(amp + 1e-10)  # +1e-10 para evitar log(0)
        amp_db_max = np.max(amp_db)
        
        ax3.plot(freqs, amp_db, 'b-', linewidth=0.8)
        ax3.axhline(amp_db_max - 3, color='orange', linestyle='--', 
                    label='-3 dB', alpha=0.8)
        ax3.axhline(amp_db_max - 20, color='red', linestyle='--', 
                    label='-20 dB', alpha=0.8)
        ax3.axvline(clinical['bw_max'], color='r', linestyle=':', 
                    label=f'Fmax clínico', alpha=0.8)
        ax3.set_xlabel('Frecuencia (Hz)')
        ax3.set_ylabel('Amplitud (dB)')
        ax3.set_title('Espectro de Amplitud (Logarítmico)')
        ax3.set_xlim([0, min(results['fs']/2, clinical['bw_max']*3)])
        ax3.set_ylim([amp_db_max - 60, amp_db_max + 5])
        ax3.legend(loc='upper right', fontsize=8)
        ax3.grid(True, alpha=0.3)
        
        # 4. PSD
        ax4 = axes[1, 1]
        psd_db = 10 * np.log10(results['psd'] + 1e-20)
        
        ax4.plot(freqs, psd_db, 'b-', linewidth=0.8)
        ax4.axvline(clinical['bw_max'], color='r', linestyle='--', 
                    label=f'BW clínico: {clinical["bw_max"]} Hz')
        ax4.fill_between([clinical['bw_min'], clinical['bw_max']], 
                         min(psd_db), max(psd_db), alpha=0.2, color='green')
        ax4.set_xlabel('Frecuencia (Hz)')
        ax4.set_ylabel('PSD (dB/Hz)')
        ax4.set_title('Densidad Espectral de Potencia (PSD)')
        ax4.set_xlim([0, min(results['fs']/2, clinical['bw_max']*3)])
        ax4.legend(loc='upper right', fontsize=8)
        ax4.grid(True, alpha=0.3)
        
        plt.tight_layout()
        
        if save_path:
            plt.savefig(save_path, dpi=150, bbox_inches='tight')
            print(f"[OK] Gráfico guardado: {save_path}")
        
        plt.show()
    
    def generate_report(self, data: np.ndarray, results: Dict, 
                        bw_analysis: Dict, save_path: str = None) -> str:
        """
        Genera un reporte de texto con todos los resultados.
        """
        timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        
        report = f"""
================================================================================
           REPORTE DE ANÁLISIS ESPECTRAL FFT - BioSignalSimulator Pro
================================================================================
Fecha: {timestamp}
Puerto: {self.port}
Señal: {bw_analysis['signal_type']}

--------------------------------------------------------------------------------
1. PARÁMETROS DE CAPTURA
--------------------------------------------------------------------------------
  • Muestras capturadas: {results['N']}
  • Frecuencia de muestreo: {results['fs']:.2f} Hz
  • Duración: {results['N'] / results['fs']:.2f} segundos
  • Resolución frecuencial: {results['freq_resolution']:.4f} Hz
  • Frecuencia máxima (Nyquist): {results['fs']/2:.2f} Hz
  • Ventana aplicada: {results['window']}

--------------------------------------------------------------------------------
2. ESTADÍSTICAS DE LA SEÑAL TEMPORAL
--------------------------------------------------------------------------------
  • Valor medio (DC): {np.mean(data):.4f} V
  • Desviación estándar: {np.std(data):.4f} V
  • Valor mínimo: {np.min(data):.4f} V
  • Valor máximo: {np.max(data):.4f} V
  • Rango pico-pico: {np.max(data) - np.min(data):.4f} V

--------------------------------------------------------------------------------
3. RESULTADOS DEL ANÁLISIS ESPECTRAL
--------------------------------------------------------------------------------
  • Frecuencia dominante: {results['freq_dominant']:.2f} Hz
  • Amplitud en frecuencia dominante: {results['amp_dominant']:.4f} V
  • Ancho de banda a -3dB: {results['bw_3db']:.2f} Hz
  • Frecuencia donde está 99% de energía: {results['freq_99_energy']:.2f} Hz

--------------------------------------------------------------------------------
4. COMPARACIÓN CON ESTÁNDARES CLÍNICOS
--------------------------------------------------------------------------------
  • Estándar aplicado: {bw_analysis['clinical_standard']}
  • Ancho de banda clínico: {bw_analysis['clinical_bw_min']:.2f} - {bw_analysis['clinical_bw_max']:.2f} Hz
  • Fs mínima requerida (Nyquist): {bw_analysis['clinical_nyquist_min']} Hz
  • Fs actual: {results['fs']:.0f} Hz
  • Nyquist satisfecho: {'✓ SÍ' if bw_analysis['nyquist_satisfied'] else '✗ NO'}
  
  • Frecuencia de corte medida (-20dB): {bw_analysis['measured_cutoff_20db']:.2f} Hz
  • Frecuencia de corte medida (-40dB): {bw_analysis['measured_cutoff_40db']:.2f} Hz
  • Energía en banda clínica: {bw_analysis['energy_in_clinical_band_pct']:.1f}%

--------------------------------------------------------------------------------
5. RECOMENDACIONES PARA FILTRO RC ANTIALIASING
--------------------------------------------------------------------------------
  Fórmula: Fc = 1 / (2π × R × C)  →  R = 1 / (2π × Fc × C)
  
  Con base en Fmax medida (-20dB) = {bw_analysis['measured_cutoff_20db']:.2f} Hz:
  
  • Fc recomendada = 2 × Fmax = {bw_analysis['recommended_rc_fc']:.1f} Hz
  
  Con C = 1 µF:
    R = 1 / (2π × Fc × C)
    R = 1 / (2π × {bw_analysis['recommended_rc_fc']:.1f} × 1×10⁻⁶)
    R = {1.0 / (2 * np.pi * max(bw_analysis['recommended_rc_fc'], 0.001) * 1e-6):.0f} Ω
  
  Valores comerciales sugeridos:
    - R = {self._nearest_e24(1.0 / (2 * np.pi * max(bw_analysis['recommended_rc_fc'], 0.001) * 1e-6)):.0f} Ω (E24)
    - C = 1 µF (cerámico o film)

--------------------------------------------------------------------------------
6. CONCLUSIONES
--------------------------------------------------------------------------------
"""
        if bw_analysis['nyquist_satisfied']:
            report += "  ✓ La frecuencia de muestreo cumple con el criterio de Nyquist.\n"
        else:
            report += "  ✗ ADVERTENCIA: La frecuencia de muestreo NO cumple Nyquist.\n"
            report += f"    Se recomienda aumentar Fs a mínimo {bw_analysis['clinical_nyquist_min']} Hz.\n"
        
        if bw_analysis['energy_in_clinical_band_pct'] > 95:
            report += "  ✓ Más del 95% de la energía está en la banda clínica.\n"
        elif bw_analysis['energy_in_clinical_band_pct'] > 80:
            report += "  ⚠ Entre 80-95% de la energía está en la banda clínica.\n"
        else:
            report += "  ✗ Menos del 80% de la energía está en la banda clínica.\n"
        
        report += """
================================================================================
                            FIN DEL REPORTE
================================================================================
"""
        
        if save_path:
            with open(save_path, 'w', encoding='utf-8') as f:
                f.write(report)
            print(f"[OK] Reporte guardado: {save_path}")
        
        return report
    
    def _nearest_e24(self, value: float) -> float:
        """Encuentra el valor E24 más cercano."""
        e24 = [1.0, 1.1, 1.2, 1.3, 1.5, 1.6, 1.8, 2.0, 2.2, 2.4, 2.7, 3.0,
               3.3, 3.6, 3.9, 4.3, 4.7, 5.1, 5.6, 6.2, 6.8, 7.5, 8.2, 9.1]
        
        # Encontrar el orden de magnitud
        magnitude = 10 ** np.floor(np.log10(value))
        normalized = value / magnitude
        
        # Encontrar el E24 más cercano
        closest = min(e24, key=lambda x: abs(x - normalized))
        
        return closest * magnitude


# =============================================================================
# MODO SIMULACIÓN (sin hardware)
# =============================================================================

def generate_test_signal(signal_type: str, duration: float, fs: float) -> np.ndarray:
    """
    Genera una señal de prueba sintética para demostración.
    Útil cuando no hay hardware conectado.
    """
    t = np.arange(0, duration, 1/fs)
    N = len(t)
    
    if signal_type.upper() == 'ECG':
        # Simular ECG: fundamental a 1 Hz (60 BPM) con armónicos
        sig_out = np.zeros(N)
        for i, freq in enumerate([1, 2, 3, 5, 10, 20, 40]):  # Armónicos del ECG
            amp = 1.0 / (i + 1)
            sig_out += amp * np.sin(2 * np.pi * freq * t)
        # Agregar componentes de alta frecuencia (QRS ~30 Hz)
        sig_out += 0.3 * np.sin(2 * np.pi * 30 * t) * np.exp(-((t % 1) - 0.3)**2 / 0.001)
        
    elif signal_type.upper() == 'EMG':
        # Simular EMG: ruido de banda ancha con espectro típico
        # Generamos ruido blanco y lo filtramos para simular banda EMG
        noise = np.random.randn(N) * 0.5
        # Usamos scipy.signal (importado arriba)
        sos = signal.butter(4, [20, min(200, fs/2-1)], btype='band', fs=fs, output='sos')
        sig_out = signal.sosfilt(sos, noise)
        
    elif signal_type.upper() == 'PPG':
        # Simular PPG: onda pulsátil lenta
        sig_out = np.zeros(N)
        hr = 1.2  # 72 BPM
        for i, freq in enumerate([hr, 2*hr, 3*hr]):
            amp = 1.0 / (i + 1)**2
            sig_out += amp * np.sin(2 * np.pi * freq * t)
        sig_out = (sig_out + 1) / 2  # Normalizar a 0-1
        
    else:
        # Señal de prueba genérica
        sig_out = np.sin(2 * np.pi * 10 * t) + 0.5 * np.sin(2 * np.pi * 50 * t)
    
    # Agregar ruido
    sig_out += 0.01 * np.random.randn(N)
    
    # Escalar a rango de voltaje del DAC (0-3.3V centrado en 1.65V)
    sig_out = (sig_out - np.min(sig_out)) / (np.max(sig_out) - np.min(sig_out))
    sig_out = sig_out * 3.0 + 0.15  # 0.15 - 3.15 V
    
    return sig_out


# =============================================================================
# FUNCIÓN PRINCIPAL
# =============================================================================

def list_serial_ports():
    """Lista los puertos seriales disponibles."""
    ports = serial.tools.list_ports.comports()
    print("\n[PUERTOS DISPONIBLES]")
    for port in ports:
        print(f"  • {port.device}: {port.description}")
    return [p.device for p in ports]


def main():
    """Función principal con interfaz de línea de comandos."""
    parser = argparse.ArgumentParser(
        description='FFT Spectrum Analyzer para BioSignalSimulator Pro',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Ejemplos de uso:
  python fft_spectrum_analyzer.py --port COM3 --signal ECG --duration 10
  python fft_spectrum_analyzer.py --port COM3 --signal EMG --duration 5 --window blackman
  python fft_spectrum_analyzer.py --simulate --signal PPG --duration 8
  python fft_spectrum_analyzer.py --list-ports
        """
    )
    
    parser.add_argument('--port', type=str, default='COM3',
                        help='Puerto COM del ESP32 (ej: COM3, /dev/ttyUSB0)')
    parser.add_argument('--baud', type=int, default=115200,
                        help='Velocidad de comunicación (default: 115200)')
    parser.add_argument('--fs', type=float, default=200.0,
                        help='Frecuencia de muestreo esperada en Hz (default: 200)')
    parser.add_argument('--signal', type=str, default='ECG',
                        choices=['ECG', 'EMG', 'PPG'],
                        help='Tipo de señal a analizar')
    parser.add_argument('--duration', type=float, default=10.0,
                        help='Duración de captura en segundos (default: 10)')
    parser.add_argument('--window', type=str, default='hanning',
                        choices=['hanning', 'blackman', 'hamming', 'rectangular'],
                        help='Tipo de ventana FFT (default: hanning)')
    parser.add_argument('--save', type=str, default=None,
                        help='Prefijo para guardar archivos (plot y reporte)')
    parser.add_argument('--list-ports', action='store_true',
                        help='Listar puertos seriales disponibles')
    parser.add_argument('--simulate', action='store_true',
                        help='Usar señal simulada en lugar de captura real')
    
    args = parser.parse_args()
    
    # Listar puertos
    if args.list_ports:
        list_serial_ports()
        return
    
    # Determinar Fs automáticamente según tipo de señal
    # ESP32 envía: ECG @ 200 Hz, EMG/PPG @ 100 Hz (según downsample en main.cpp)
    fs_by_signal = {
        'ECG': 200.0,   # 2000/10 = 200 Hz
        'EMG': 100.0,   # 2000/20 = 100 Hz
        'PPG': 100.0    # 2000/20 = 100 Hz
    }
    fs_auto = fs_by_signal.get(args.signal.upper(), 200.0)
    
    print("\n" + "="*66)
    print("       FFT SPECTRUM ANALYZER - BioSignalSimulator Pro")
    print("="*66)
    
    # Modo simulación o captura real
    if args.simulate:
        print("\n[MODO SIMULACIÓN] Generando señal de prueba...")
        print("[NOTA] Las señales simuladas son APROXIMACIONES, no los modelos reales.")
        print("       Para analizar los modelos McSharry/Fuglevand/Allen, use captura REAL.")
        # Para simulación usar Fs más alta para mejor resolución
        fs_sim = 1000.0 if args.signal == 'EMG' else 200.0
        data = generate_test_signal(args.signal, args.duration, fs_sim)
        
        analyzer = FFTSpectrumAnalyzer('SIMULATED', args.baud, fs_sim)
        analyzer.data_buffer = data.tolist()
        effective_fs = fs_sim
        
    else:
        # Usar Fs automático según tipo de señal (o el especificado por usuario)
        fs_to_use = args.fs if args.fs != 200.0 else fs_auto
        print(f"\n[CAPTURA REAL] Fs esperada: {fs_to_use} Hz (según tipo {args.signal})")
        
        # Crear analizador y conectar
        analyzer = FFTSpectrumAnalyzer(args.port, args.baud, fs_to_use)
        
        if not analyzer.connect():
            print("\n[ERROR] No se pudo conectar. Use --list-ports para ver puertos disponibles.")
            print("        O use --simulate para probar sin hardware.")
            return
        
        try:
            # Capturar datos
            data, effective_fs = analyzer.capture_data(
                duration_sec=args.duration,
                signal_type=args.signal
            )
        finally:
            analyzer.disconnect()
        
        if len(data) < 100:
            print("[ERROR] No se capturaron suficientes datos.")
            return
    
    # Calcular FFT
    results = analyzer.compute_fft(data, window=args.window, detrend=True)
    
    # Analizar ancho de banda
    bw_analysis = analyzer.analyze_bandwidth(results, args.signal)
    
    # Generar reporte
    if args.save:
        report_path = f"{args.save}_{args.signal}_report.txt"
        plot_path = f"{args.save}_{args.signal}_spectrum.png"
    else:
        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        report_path = f"fft_report_{args.signal}_{timestamp}.txt"
        plot_path = f"fft_plot_{args.signal}_{timestamp}.png"
    
    report = analyzer.generate_report(data, results, bw_analysis, report_path)
    print(report)
    
    # Graficar
    analyzer.plot_results(data, results, args.signal, plot_path)
    
    print("\n[COMPLETO] Análisis FFT finalizado.")


if __name__ == '__main__':
    main()
