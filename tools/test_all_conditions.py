#!/usr/bin/env python3
"""
test_all_conditions.py - Prueba automatizada de todas las patologías

Este script prueba todas las combinaciones de señales y condiciones
del BioSimulator Pro, validando contra rangos clínicos.

Uso:
    python test_all_conditions.py --port COM4
    python test_all_conditions.py --port COM4 --duration 10
    python test_all_conditions.py --list  # Ver todas las condiciones
"""

import argparse
import sys
import time
from typing import Dict, List, Tuple
from dataclasses import dataclass

# Importar módulos locales
from clinical_ranges import (
    ECG_CLINICAL_RANGES, EMG_CLINICAL_RANGES, PPG_CLINICAL_RANGES,
    validate_ecg_sample, validate_emg_sample, validate_ppg_sample
)

# ============================================================================
# MAPEO DE CONDICIONES (debe coincidir con main_debug.cpp)
# ============================================================================

ECG_CONDITIONS = {
    0: ("NORMAL", "Normal"),
    1: ("TACHYCARDIA", "Taquicardia"),
    2: ("BRADYCARDIA", "Bradicardia"),
    3: ("ATRIAL_FIBRILLATION", "Fibrilación Auricular"),
    4: ("VENTRICULAR_FIBRILLATION", "Fibrilación Ventricular"),
    5: ("PREMATURE_VENTRICULAR", "Extrasístole Ventricular (PVC)"),
    6: ("ST_ELEVATION", "STEMI (Infarto)"),
    7: ("ST_DEPRESSION", "Isquemia (Depresión ST)"),
}

EMG_CONDITIONS = {
    0: ("REST", "Reposo (0-10% MVC)"),
    1: ("LOW_CONTRACTION", "Contracción Baja (10-30% MVC)"),
    2: ("MODERATE_CONTRACTION", "Contracción Moderada (30-60% MVC)"),
    3: ("HIGH_CONTRACTION", "Contracción Alta (60-100% MVC)"),
    4: ("TREMOR", "Temblor Parkinson"),
    5: ("MYOPATHY", "Miopatía"),
    6: ("NEUROPATHY", "Neuropatía"),
    7: ("FASCICULATION", "Fasciculación"),
}

PPG_CONDITIONS = {
    0: ("NORMAL", "Normal"),
    1: ("ARRHYTHMIA", "Arritmia"),
    2: ("WEAK_PERFUSION", "Perfusión Débil"),
    3: ("STRONG_PERFUSION", "Perfusión Fuerte"),
    4: ("VASOCONSTRICTION", "Vasoconstricción"),
}

SIGNAL_TYPES = {
    0: ("ECG", ECG_CONDITIONS),
    1: ("EMG", EMG_CONDITIONS),
    2: ("PPG", PPG_CONDITIONS),
}


def list_all_conditions():
    """Muestra todas las condiciones disponibles."""
    print("\n" + "="*80)
    print("TODAS LAS CONDICIONES DISPONIBLES")
    print("="*80)
    
    print("\n📊 ECG (AUTO_SIGNAL_TYPE=0):")
    print("-" * 60)
    for idx, (code, name) in ECG_CONDITIONS.items():
        print(f"  {idx}: {code:30s} - {name}")
    
    print("\n💪 EMG (AUTO_SIGNAL_TYPE=1):")
    print("-" * 60)
    for idx, (code, name) in EMG_CONDITIONS.items():
        print(f"  {idx}: {code:30s} - {name}")
    
    print("\n❤️ PPG (AUTO_SIGNAL_TYPE=2):")
    print("-" * 60)
    for idx, (code, name) in PPG_CONDITIONS.items():
        print(f"  {idx}: {code:30s} - {name}")
    
    print("\n" + "="*80)
    print("INSTRUCCIONES DE USO:")
    print("="*80)
    print("""
1. Edita main_debug.cpp y cambia:
   - AUTO_SIGNAL_TYPE: 0=ECG, 1=EMG, 2=PPG
   - AUTO_XXX_CONDITION: número de la condición

2. Compila y sube:
   pio run -e esp32_debug_ecg -t upload

3. Ejecuta el validador:
   cd tools
   python signal_validator.py --port COM4 --signal ecg --condition NORMAL
   
4. O usa este script para validar automáticamente:
   python test_all_conditions.py --port COM4 --signal ecg --condition 0
""")


def generate_test_instructions():
    """Genera instrucciones para probar todas las condiciones."""
    print("\n" + "="*80)
    print("COMANDOS PARA PROBAR TODAS LAS CONDICIONES")
    print("="*80)
    
    print("\n# ============ ECG ============")
    for idx, (code, name) in ECG_CONDITIONS.items():
        print(f"# {name}")
        print(f"# En main_debug.cpp: AUTO_SIGNAL_TYPE=0, AUTO_ECG_CONDITION={idx}")
        print(f"python signal_validator.py --port COM4 --signal ecg --condition {code}")
        print()
    
    print("\n# ============ EMG ============")
    for idx, (code, name) in EMG_CONDITIONS.items():
        print(f"# {name}")
        print(f"# En main_debug.cpp: AUTO_SIGNAL_TYPE=1, AUTO_EMG_CONDITION={idx}")
        print(f"python signal_validator.py --port COM4 --signal emg --condition {code}")
        print()
    
    print("\n# ============ PPG ============")
    for idx, (code, name) in PPG_CONDITIONS.items():
        print(f"# {name}")
        print(f"# En main_debug.cpp: AUTO_SIGNAL_TYPE=2, AUTO_PPG_CONDITION={idx}")
        print(f"python signal_validator.py --port COM4 --signal ppg --condition {code}")
        print()


@dataclass
class TestResult:
    """Resultado de una prueba."""
    signal_type: str
    condition: str
    condition_name: str
    total_samples: int
    valid_samples: int
    validity_pct: float
    param_results: Dict[str, Tuple[int, int]]  # param -> (valid, total)
    passed: bool


def run_single_test(port: str, baud: int, signal_type: str, condition: str, 
                    duration: int = 10, verbose: bool = False) -> TestResult:
    """Ejecuta una prueba individual."""
    import serial
    import re
    
    # Mapeo de condiciones a nombres
    condition_map = {
        'ECG': {v[0]: (k, v[1]) for k, v in ECG_CONDITIONS.items()},
        'EMG': {v[0]: (k, v[1]) for k, v in EMG_CONDITIONS.items()},
        'PPG': {v[0]: (k, v[1]) for k, v in PPG_CONDITIONS.items()},
    }
    
    cond_idx, cond_name = condition_map[signal_type].get(condition, (0, condition))
    
    # Patrones de parseo
    patterns = {
        'ECG': re.compile(r'>ecg:([-\d.]+),hr:([\d.]+),rr:([\d.]+),ramp:([\d.]+),st:([-\d.]+),qrs:([\d.]+),beats:(\d+)'),
        'EMG': re.compile(r'>emg:([-\d.]+),rms:([\d.]+),mus:(\d+),fr:([\d.]+),cont:([\d.]+)'),
        'PPG': re.compile(r'>ppg:([-\d.]+),hr:([\d.]+),rr:([\d.]+),pi:([\d.]+),beats:(\d+)'),
    }
    
    pattern = patterns[signal_type]
    
    # Estadísticas
    total_samples = 0
    valid_samples = 0
    param_stats = {}
    
    try:
        ser = serial.Serial(port, baud, timeout=1)
        time.sleep(2)  # Esperar conexión
        ser.reset_input_buffer()
        
        start_time = time.time()
        
        while (time.time() - start_time) < duration:
            try:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
            except:
                continue
            
            match = pattern.match(line)
            if not match:
                continue
            
            total_samples += 1
            
            # Parsear datos según tipo
            if signal_type == 'ECG':
                data = {
                    'ecg': float(match.group(1)),
                    'hr': float(match.group(2)),
                    'rr': float(match.group(3)),
                    'ramp': float(match.group(4)),
                    'st': float(match.group(5)),
                    'qrs': float(match.group(6)),
                    'beats': int(match.group(7))
                }
                results = validate_ecg_sample(
                    condition, data['hr'], data['rr'],
                    data['ramp'], data['st'], data['qrs']
                )
            elif signal_type == 'EMG':
                data = {
                    'emg': float(match.group(1)),
                    'rms': float(match.group(2)),
                    'mus': int(match.group(3)),
                    'fr': float(match.group(4)),
                    'cont': float(match.group(5))
                }
                results = validate_emg_sample(
                    condition, data['rms'], data['mus'],
                    data['fr'], data['cont']
                )
            elif signal_type == 'PPG':
                data = {
                    'ppg': float(match.group(1)),
                    'hr': float(match.group(2)),
                    'rr': float(match.group(3)),
                    'pi': float(match.group(4)),
                    'beats': int(match.group(5))
                }
                results = validate_ppg_sample(
                    condition, data['hr'], data['rr'],
                    data['pi']
                )
            
            # Contar resultados
            sample_valid = True
            for param, (is_valid, msg) in results.items():
                if param not in param_stats:
                    param_stats[param] = [0, 0]
                param_stats[param][1] += 1
                if is_valid:
                    param_stats[param][0] += 1
                else:
                    sample_valid = False
            
            if sample_valid:
                valid_samples += 1
            
            if verbose and total_samples % 50 == 0:
                pct = (valid_samples / total_samples * 100) if total_samples > 0 else 0
                print(f"  [{signal_type}:{condition}] {total_samples} muestras, {pct:.1f}% válidas")
        
        ser.close()
        
    except serial.SerialException as e:
        print(f"Error de puerto serial: {e}")
        return TestResult(
            signal_type=signal_type,
            condition=condition,
            condition_name=cond_name,
            total_samples=0,
            valid_samples=0,
            validity_pct=0,
            param_results={},
            passed=False
        )
    
    validity_pct = (valid_samples / total_samples * 100) if total_samples > 0 else 0
    passed = validity_pct >= 90  # 90% de muestras válidas para pasar
    
    return TestResult(
        signal_type=signal_type,
        condition=condition,
        condition_name=cond_name,
        total_samples=total_samples,
        valid_samples=valid_samples,
        validity_pct=validity_pct,
        param_results={k: tuple(v) for k, v in param_stats.items()},
        passed=passed
    )


def print_test_result(result: TestResult):
    """Imprime el resultado de una prueba."""
    status = "✓ PASS" if result.passed else "✗ FAIL"
    print(f"\n{'-'*60}")
    print(f"{result.signal_type} - {result.condition_name} ({result.condition})")
    print(f"{'-'*60}")
    print(f"  Muestras: {result.valid_samples}/{result.total_samples} válidas ({result.validity_pct:.1f}%)")
    print(f"  Estado: {status}")
    
    if result.param_results:
        print(f"  Parámetros:")
        for param, (valid, total) in result.param_results.items():
            pct = (valid / total * 100) if total > 0 else 0
            status_icon = "✓" if pct >= 90 else "✗"
            print(f"    {status_icon} {param}: {valid}/{total} ({pct:.1f}%)")


def main():
    parser = argparse.ArgumentParser(
        description='Prueba automatizada de todas las condiciones del BioSimulator Pro'
    )
    parser.add_argument('--port', '-p', default='COM4',
                       help='Puerto serial (default: COM4)')
    parser.add_argument('--baud', '-b', type=int, default=115200,
                       help='Velocidad de baudios (default: 115200)')
    parser.add_argument('--signal', '-s', choices=['ecg', 'emg', 'ppg'],
                       help='Tipo de señal a probar (si no se especifica, muestra instrucciones)')
    parser.add_argument('--condition', '-c', type=str,
                       help='Condición a probar (nombre o número)')
    parser.add_argument('--duration', '-d', type=int, default=10,
                       help='Duración de cada prueba en segundos (default: 10)')
    parser.add_argument('--list', '-l', action='store_true',
                       help='Listar todas las condiciones disponibles')
    parser.add_argument('--commands', action='store_true',
                       help='Generar comandos para todas las pruebas')
    parser.add_argument('--verbose', '-v', action='store_true',
                       help='Mostrar progreso detallado')
    
    args = parser.parse_args()
    
    if args.list:
        list_all_conditions()
        return
    
    if args.commands:
        generate_test_instructions()
        return
    
    if not args.signal:
        print("\n" + "="*80)
        print("BIOSIMULATOR PRO - PRUEBA DE CONDICIONES")
        print("="*80)
        print("""
Uso:
  python test_all_conditions.py --list              # Ver todas las condiciones
  python test_all_conditions.py --commands          # Generar comandos de prueba
  
Para probar una condición específica:
  1. Edita main_debug.cpp con la señal y condición deseada
  2. Compila y sube: pio run -e esp32_debug_ecg -t upload
  3. Ejecuta:
     python test_all_conditions.py --port COM4 --signal ecg --condition NORMAL
     python test_all_conditions.py --port COM4 --signal emg --condition STRONG_CONTRACTION
     python test_all_conditions.py --port COM4 --signal ppg --condition VASOCONSTRICTION

O usa el validador directamente:
  python signal_validator.py --port COM4 --signal ecg --condition NORMAL
""")
        return
    
    # Ejecutar prueba específica
    signal_type = args.signal.upper()
    
    # Determinar condición
    if args.condition:
        condition = args.condition.upper()
        # Si es número, convertir a nombre
        if condition.isdigit():
            idx = int(condition)
            if signal_type == 'ECG' and idx in ECG_CONDITIONS:
                condition = ECG_CONDITIONS[idx][0]
            elif signal_type == 'EMG' and idx in EMG_CONDITIONS:
                condition = EMG_CONDITIONS[idx][0]
            elif signal_type == 'PPG' and idx in PPG_CONDITIONS:
                condition = PPG_CONDITIONS[idx][0]
    else:
        condition = 'NORMAL' if signal_type in ['ECG', 'PPG'] else 'REST'
    
    print(f"\n{'='*60}")
    print(f"PROBANDO: {signal_type} - {condition}")
    print(f"Puerto: {args.port} @ {args.baud} baud")
    print(f"Duración: {args.duration} segundos")
    print(f"{'='*60}")
    print("\nRecopilando datos...")
    
    result = run_single_test(
        args.port, args.baud, signal_type, condition,
        args.duration, args.verbose
    )
    
    print_test_result(result)
    
    # Resumen final
    print(f"\n{'='*60}")
    if result.passed:
        print("✓ PRUEBA EXITOSA - La señal cumple los rangos clínicos")
    else:
        print("✗ PRUEBA FALLIDA - Algunos parámetros fuera de rango")
    print(f"{'='*60}\n")
    
    sys.exit(0 if result.passed else 1)


if __name__ == '__main__':
    main()
