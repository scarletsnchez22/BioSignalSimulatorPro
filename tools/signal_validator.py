#!/usr/bin/env python3
"""
signal_validator.py - Comprobador de Señales Biológicas en Tiempo Real

Lee datos del puerto serial y valida contra rangos clínicos definidos.

Uso:
    python signal_validator.py --port COM4 --signal ecg --condition NORMAL
    python signal_validator.py --port COM4 --signal emg --condition STRONG
    python signal_validator.py --port COM4 --signal ppg --condition NORMAL

Formato de entrada esperado del ESP32:
    ECG: >ecg:VALUE,hr:VALUE,rr:VALUE
    EMG: >emg:VALUE,rms:VALUE,mus:VALUE,fr:VALUE
    PPG: >ppg:VALUE,hr:VALUE,rr:VALUE,pi:VALUE

Autor: BioSimulator Pro
Versión: 1.0.0
"""

import argparse
import serial
import serial.tools.list_ports
import time
import sys
import re
from datetime import datetime
from collections import deque
from typing import Dict, Optional, Tuple
import statistics

# Importar rangos clínicos
from clinical_ranges import (
    ECG_CLINICAL_RANGES, EMG_CLINICAL_RANGES, PPG_CLINICAL_RANGES,
    ECG_CONDITION_MAP, EMG_CONDITION_MAP, PPG_CONDITION_MAP,
    validate_ecg_sample, validate_emg_sample, validate_ppg_sample
)


class SignalValidator:
    """Validador de señales biológicas en tiempo real."""
    
    def __init__(self, port: str, baudrate: int, signal_type: str, condition: str):
        self.port = port
        self.baudrate = baudrate
        self.signal_type = signal_type.upper()
        self.condition = self._normalize_condition(condition)
        
        # Buffers para estadísticas
        self.sample_count = 0
        self.valid_count = 0
        self.invalid_count = 0
        self.start_time = None
        
        # Historial de valores (últimos 100)
        self.history = {
            'signal': deque(maxlen=100),
            'hr': deque(maxlen=100),
            'rr': deque(maxlen=100),
        }
        
        # Valores específicos por tipo de señal
        if self.signal_type == 'ECG':
            self.history['r_amp'] = deque(maxlen=100)
            self.history['st_dev'] = deque(maxlen=100)
        elif self.signal_type == 'EMG':
            self.history['rms'] = deque(maxlen=100)
            self.history['mus'] = deque(maxlen=100)
            self.history['fr'] = deque(maxlen=100)
        elif self.signal_type == 'PPG':
            self.history['pi'] = deque(maxlen=100)
            self.history['spo2'] = deque(maxlen=100)
        
        # Resultados de validación
        self.validation_results = {}
        
        # Serial
        self.serial = None
    
    def _normalize_condition(self, condition: str) -> str:
        """Normaliza el nombre de la condición."""
        condition = condition.upper()
        
        if self.signal_type == 'ECG':
            return ECG_CONDITION_MAP.get(condition, condition)
        elif self.signal_type == 'EMG':
            return EMG_CONDITION_MAP.get(condition, condition)
        elif self.signal_type == 'PPG':
            return PPG_CONDITION_MAP.get(condition, condition)
        return condition
    
    def connect(self) -> bool:
        """Conecta al puerto serial."""
        try:
            self.serial = serial.Serial(self.port, self.baudrate, timeout=1)
            time.sleep(2)  # Esperar reset del ESP32
            self.serial.reset_input_buffer()
            print(f"✓ Conectado a {self.port} @ {self.baudrate} baud")
            return True
        except serial.SerialException as e:
            print(f"✗ Error conectando a {self.port}: {e}")
            return False
    
    def disconnect(self):
        """Desconecta del puerto serial."""
        if self.serial and self.serial.is_open:
            self.serial.close()
            print("✓ Desconectado")
    
    def parse_ecg_line(self, line: str) -> Optional[Dict]:
        """Parsea una línea de datos ECG."""
        # Formato: >ecg:VALUE,hr:VALUE,rr:VALUE,R_mV:VALUE,qrs:VALUE,st:VALUE,beats:VALUE
        match = re.match(
            r'>ecg:([-\d.]+),hr:([\d.]+),rr:([\d.]+),R_mV:([\d.]+),qrs:([\d.]+),st:([-\d.]+),beats:(\d+)',
            line
        )
        if match:
            return {
                'ecg': float(match.group(1)),
                'hr': float(match.group(2)),
                'rr': float(match.group(3)),
                'r_amp': float(match.group(4)),
                'qrs': float(match.group(5)),
                'st': float(match.group(6)),
                'beats': int(match.group(7))
            }
        return None
    
    def parse_emg_line(self, line: str) -> Optional[Dict]:
        """Parsea una línea de datos EMG."""
        # Formato: >emg:VALUE,rms:VALUE,mus:VALUE,fr:VALUE,cont:VALUE
        match = re.match(
            r'>emg:([-\d.]+),rms:([\d.]+),mus:(\d+),fr:([\d.]+),cont:([\d.]+)',
            line
        )
        if match:
            return {
                'emg': float(match.group(1)),
                'rms': float(match.group(2)),
                'mus': int(match.group(3)),
                'fr': float(match.group(4)),
                'cont': float(match.group(5))
            }
        return None
    
    def parse_ppg_line(self, line: str) -> Optional[Dict]:
        """Parsea una línea de datos PPG."""
        # Formato: >ppg:VALUE,hr:VALUE,rr:VALUE,pi:VALUE,spo2:VALUE,beats:VALUE
        match = re.match(
            r'>ppg:([-\d.]+),hr:([\d.]+),rr:([\d.]+),pi:([\d.]+),spo2:([\d.]+),beats:(\d+)',
            line
        )
        if match:
            return {
                'ppg': float(match.group(1)),
                'hr': float(match.group(2)),
                'rr': float(match.group(3)),
                'pi': float(match.group(4)),
                'spo2': float(match.group(5)),
                'beats': int(match.group(6))
            }
        return None
    
    def validate_sample(self, data: Dict) -> Dict[str, Tuple[bool, str]]:
        """Valida una muestra según el tipo de señal."""
        if self.signal_type == 'ECG':
            # Usar valores directamente del ESP32
            return validate_ecg_sample(
                self.condition,
                hr=data['hr'],
                rr_ms=data['rr'],
                r_amp_mV=data.get('r_amp', 1.0),
                st_dev_mV=data.get('st', 0.0),
                qrs_ms=data.get('qrs', None)
            )
        
        elif self.signal_type == 'EMG':
            # Usar porcentaje de contracción directamente del ESP32
            return validate_emg_sample(
                self.condition,
                rms_mV=data['rms'],
                motor_units=data['mus'],
                firing_rate_Hz=data['fr'],
                contraction_pct=data.get('cont', 0.0)
            )
        
        elif self.signal_type == 'PPG':
            return validate_ppg_sample(
                self.condition,
                hr=data['hr'],
                rr_ms=data['rr'],
                pi_pct=data['pi'],
                spo2_pct=data.get('spo2', None)
            )
        
        return {}
    
    def print_header(self):
        """Imprime el encabezado de la sesión."""
        print("\n" + "="*80)
        print("BIOSIMULATOR PRO - VALIDADOR DE SEÑALES")
        print("="*80)
        print(f"Señal: {self.signal_type}")
        print(f"Condición: {self.condition}")
        print(f"Puerto: {self.port} @ {self.baudrate} baud")
        print(f"Inicio: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        print("="*80)
        
        # Mostrar rangos esperados
        self._print_expected_ranges()
        
        print("\nPresione Ctrl+C para detener y ver resumen.\n")
        print("-"*80)
    
    def _print_expected_ranges(self):
        """Imprime los rangos esperados para la condición actual."""
        print("\nRANGOS CLÍNICOS ESPERADOS:")
        print("-"*40)
        
        if self.signal_type == 'ECG' and self.condition in ECG_CLINICAL_RANGES:
            r = ECG_CLINICAL_RANGES[self.condition]
            print(f"  HR:     {r.hr_min:.0f} - {r.hr_max:.0f} BPM")
            print(f"  RR:     {r.rr_min_ms:.0f} - {r.rr_max_ms:.0f} ms")
            print(f"  R amp:  {r.r_amplitude_min_mV:.2f} - {r.r_amplitude_max_mV:.2f} mV")
            print(f"  ST:     {r.st_deviation_min_mV:+.2f} - {r.st_deviation_max_mV:+.2f} mV")
            print(f"  QRS:    {r.qrs_duration_min_ms:.0f} - {r.qrs_duration_max_ms:.0f} ms")
            print(f"\n  Notas: {r.clinical_notes}")
        
        elif self.signal_type == 'EMG' and self.condition in EMG_CLINICAL_RANGES:
            r = EMG_CLINICAL_RANGES[self.condition]
            print(f"  RMS:    {r.rms_min_mV:.3f} - {r.rms_max_mV:.3f} mV")
            print(f"  MUs:    {r.motor_units_min} - {r.motor_units_max}")
            print(f"  FR:     {r.firing_rate_min_Hz:.1f} - {r.firing_rate_max_Hz:.1f} Hz")
            print(f"  Contr:  {r.contraction_min_pct:.0f} - {r.contraction_max_pct:.0f}%")
            print(f"\n  Notas: {r.clinical_notes}")
        
        elif self.signal_type == 'PPG' and self.condition in PPG_CLINICAL_RANGES:
            r = PPG_CLINICAL_RANGES[self.condition]
            print(f"  HR:     {r.hr_min:.0f} - {r.hr_max:.0f} BPM")
            print(f"  RR:     {r.rr_min_ms:.0f} - {r.rr_max_ms:.0f} ms")
            print(f"  PI:     {r.pi_min_pct:.2f} - {r.pi_max_pct:.2f}%")
            print(f"  SpO2:   {r.spo2_min_pct:.0f} - {r.spo2_max_pct:.0f}%")
            print(f"\n  Notas: {r.clinical_notes}")
    
    def print_validation_result(self, data: Dict, results: Dict):
        """Imprime el resultado de validación de una muestra."""
        # Determinar si todo está OK
        all_ok = all(v[0] for v in results.values() if isinstance(v, tuple))
        status = "✓" if all_ok else "✗"
        
        # Construir línea de salida
        if self.signal_type == 'ECG':
            line = (f"{status} ECG: {data['ecg']:+.4f}mV | HR: {data['hr']:.1f} | "
                   f"RR: {data['rr']:.0f}ms | R: {data.get('ramp', 0):.2f}mV | "
                   f"ST: {data.get('st', 0):+.3f}mV | QRS: {data.get('qrs', 0):.0f}ms")
        elif self.signal_type == 'EMG':
            line = (f"{status} EMG: {data['emg']:+.4f}mV | RMS: {data['rms']:.3f}mV | "
                   f"MUs: {data['mus']} | FR: {data['fr']:.1f}Hz | Cont: {data.get('cont', 0):.0f}%")
        elif self.signal_type == 'PPG':
            line = (f"{status} PPG: {data['ppg']:.4f} | HR: {data['hr']:.1f} | "
                   f"RR: {data['rr']:.0f}ms | PI: {data['pi']:.2f}% | SpO2: {data.get('spo2', 0):.1f}%")
        
        # Mostrar parámetros fuera de rango
        out_of_range = [k for k, v in results.items() if isinstance(v, tuple) and not v[0]]
        if out_of_range:
            line += f" [FUERA: {', '.join(out_of_range)}]"
        
        print(line)
    
    def run(self, duration: int = 0, verbose: bool = True):
        """
        Ejecuta la validación.
        
        Args:
            duration: Duración en segundos (0 = infinito)
            verbose: Mostrar cada muestra
        """
        if not self.connect():
            return
        
        self.print_header()
        self.start_time = time.time()
        
        try:
            while True:
                # Verificar duración
                if duration > 0 and (time.time() - self.start_time) >= duration:
                    break
                
                # Leer línea
                if self.serial.in_waiting:
                    try:
                        line = self.serial.readline().decode('utf-8').strip()
                    except UnicodeDecodeError:
                        continue
                    
                    # Parsear según tipo de señal
                    data = None
                    if self.signal_type == 'ECG' and line.startswith('>ecg:'):
                        data = self.parse_ecg_line(line)
                    elif self.signal_type == 'EMG' and line.startswith('>emg:'):
                        data = self.parse_emg_line(line)
                    elif self.signal_type == 'PPG' and line.startswith('>ppg:'):
                        data = self.parse_ppg_line(line)
                    
                    if data:
                        self.sample_count += 1
                        
                        # Guardar en historial
                        if self.signal_type == 'ECG':
                            self.history['signal'].append(data['ecg'])
                            self.history['hr'].append(data['hr'])
                            self.history['rr'].append(data['rr'])
                        elif self.signal_type == 'EMG':
                            self.history['signal'].append(data['emg'])
                            self.history['rms'].append(data['rms'])
                            self.history['mus'].append(data['mus'])
                            self.history['fr'].append(data['fr'])
                        elif self.signal_type == 'PPG':
                            self.history['signal'].append(data['ppg'])
                            self.history['hr'].append(data['hr'])
                            self.history['rr'].append(data['rr'])
                            self.history['pi'].append(data['pi'])
                        
                        # Validar
                        results = self.validate_sample(data)
                        
                        # Contar válidos/inválidos
                        all_ok = all(v[0] for v in results.values() if isinstance(v, tuple))
                        if all_ok:
                            self.valid_count += 1
                        else:
                            self.invalid_count += 1
                        
                        # Guardar resultados
                        for key, value in results.items():
                            if key not in self.validation_results:
                                self.validation_results[key] = {'valid': 0, 'invalid': 0}
                            if isinstance(value, tuple):
                                if value[0]:
                                    self.validation_results[key]['valid'] += 1
                                else:
                                    self.validation_results[key]['invalid'] += 1
                        
                        # Mostrar si verbose
                        if verbose:
                            self.print_validation_result(data, results)
        
        except KeyboardInterrupt:
            print("\n\n[Detenido por usuario]")
        
        finally:
            self.print_summary()
            self.disconnect()
    
    def print_summary(self):
        """Imprime el resumen de la sesión."""
        elapsed = time.time() - self.start_time if self.start_time else 0
        
        print("\n" + "="*80)
        print("RESUMEN DE VALIDACIÓN")
        print("="*80)
        print(f"Duración: {elapsed:.1f} segundos")
        print(f"Muestras totales: {self.sample_count}")
        print(f"Muestras válidas: {self.valid_count} ({100*self.valid_count/max(1,self.sample_count):.1f}%)")
        print(f"Muestras inválidas: {self.invalid_count} ({100*self.invalid_count/max(1,self.sample_count):.1f}%)")
        
        print("\n" + "-"*40)
        print("VALIDACIÓN POR PARÁMETRO:")
        print("-"*40)
        for param, counts in self.validation_results.items():
            total = counts['valid'] + counts['invalid']
            pct = 100 * counts['valid'] / max(1, total)
            status = "✓" if pct >= 90 else "⚠" if pct >= 70 else "✗"
            print(f"  {param:<15}: {counts['valid']}/{total} válidos ({pct:.1f}%) {status}")
        
        # Estadísticas de señal
        print("\n" + "-"*40)
        print("ESTADÍSTICAS DE SEÑAL:")
        print("-"*40)
        
        if self.history['signal']:
            signal_vals = list(self.history['signal'])
            print(f"  Señal min:  {min(signal_vals):.4f}")
            print(f"  Señal max:  {max(signal_vals):.4f}")
            print(f"  Señal mean: {statistics.mean(signal_vals):.4f}")
            print(f"  Señal std:  {statistics.stdev(signal_vals) if len(signal_vals) > 1 else 0:.4f}")
        
        if self.signal_type == 'ECG' and self.history['hr']:
            hr_vals = list(self.history['hr'])
            print(f"\n  HR mean:    {statistics.mean(hr_vals):.1f} BPM")
            print(f"  HR std:     {statistics.stdev(hr_vals) if len(hr_vals) > 1 else 0:.1f} BPM")
        
        elif self.signal_type == 'EMG' and self.history['rms']:
            rms_vals = list(self.history['rms'])
            print(f"\n  RMS mean:   {statistics.mean(rms_vals):.3f} mV")
            print(f"  RMS std:    {statistics.stdev(rms_vals) if len(rms_vals) > 1 else 0:.3f} mV")
            if self.history['fr']:
                fr_vals = list(self.history['fr'])
                print(f"  FR mean:    {statistics.mean(fr_vals):.1f} Hz")
        
        elif self.signal_type == 'PPG' and self.history['pi']:
            pi_vals = list(self.history['pi'])
            print(f"\n  PI mean:    {statistics.mean(pi_vals):.2f}%")
            print(f"  PI std:     {statistics.stdev(pi_vals) if len(pi_vals) > 1 else 0:.2f}%")
        
        print("\n" + "="*80)
        
        # Veredicto final
        total_pct = 100 * self.valid_count / max(1, self.sample_count)
        if total_pct >= 90:
            print("VEREDICTO: ✓ SEÑAL CORRECTA - Cumple rangos clínicos")
        elif total_pct >= 70:
            print("VEREDICTO: ⚠ SEÑAL ACEPTABLE - Algunos valores fuera de rango")
        else:
            print("VEREDICTO: ✗ SEÑAL INCORRECTA - Revisar parámetros del modelo")
        print("="*80 + "\n")


def list_ports():
    """Lista los puertos seriales disponibles."""
    ports = serial.tools.list_ports.comports()
    print("\nPuertos seriales disponibles:")
    print("-"*40)
    for port in ports:
        print(f"  {port.device}: {port.description}")
    if not ports:
        print("  (ninguno encontrado)")
    print()


def main():
    parser = argparse.ArgumentParser(
        description='Validador de señales biológicas BioSimulator Pro',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Ejemplos:
  python signal_validator.py --port COM4 --signal ecg --condition NORMAL
  python signal_validator.py --port COM4 --signal emg --condition STRONG
  python signal_validator.py --port COM4 --signal ppg --condition LOW_SPO2
  python signal_validator.py --list-ports
  python signal_validator.py --show-ranges

Condiciones ECG:
  NORMAL, TACHY, BRADY, AFIB, VFIB, PVC, STE, STD

Condiciones EMG:
  REST, LOW, MODERATE, HIGH, TREMOR, MYOPATHY, NEUROPATHY, FASCICULATION

Condiciones PPG:
  NORMAL, ARRHYTHMIA, WEAK_PERFUSION, STRONG_PERFUSION, VASOCONSTRICTION, LOW_SPO2
        """
    )
    
    parser.add_argument('--port', '-p', type=str, default='COM4',
                        help='Puerto serial (default: COM4)')
    parser.add_argument('--baud', '-b', type=int, default=115200,
                        help='Baudrate (default: 115200)')
    parser.add_argument('--signal', '-s', type=str, required=False,
                        choices=['ecg', 'emg', 'ppg', 'ECG', 'EMG', 'PPG'],
                        help='Tipo de señal')
    parser.add_argument('--condition', '-c', type=str, required=False,
                        help='Condición/patología a validar')
    parser.add_argument('--duration', '-d', type=int, default=0,
                        help='Duración en segundos (0 = infinito)')
    parser.add_argument('--quiet', '-q', action='store_true',
                        help='No mostrar cada muestra')
    parser.add_argument('--list-ports', '-l', action='store_true',
                        help='Listar puertos seriales disponibles')
    parser.add_argument('--show-ranges', '-r', action='store_true',
                        help='Mostrar todas las tablas de rangos clínicos')
    
    args = parser.parse_args()
    
    # Acciones especiales
    if args.list_ports:
        list_ports()
        return
    
    if args.show_ranges:
        from clinical_ranges import print_all_ranges
        print_all_ranges()
        return
    
    # Validar argumentos requeridos
    if not args.signal:
        parser.error("Se requiere --signal (ecg, emg, ppg)")
    if not args.condition:
        parser.error("Se requiere --condition")
    
    # Crear y ejecutar validador
    validator = SignalValidator(
        port=args.port,
        baudrate=args.baud,
        signal_type=args.signal,
        condition=args.condition
    )
    
    validator.run(duration=args.duration, verbose=not args.quiet)


if __name__ == "__main__":
    main()
