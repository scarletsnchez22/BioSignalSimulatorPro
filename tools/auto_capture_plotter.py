"""
Auto Capture Serial Plotter - BioSignalSimulator Pro
Captura automáticamente datos del Serial Plotter para todas las señales/condiciones
y genera imágenes PNG de alta calidad para documentación de tesis.

Autor: GitHub Copilot
Fecha: 16 Enero 2026
"""

import serial
import serial.tools.list_ports
import matplotlib.pyplot as plt
import numpy as np
import time
import os
from datetime import datetime
import json

# ==================== CONFIGURACIÓN ====================
SAMPLES_TO_CAPTURE = 900  # Número de muestras por condición (eje X del plotter)
BAUDRATE = 115200
OUTPUT_DIR = "results/plotter_captures"

# TODAS LAS CONDICIONES SIMULADAS (20 total: ECG×8, EMG×6, PPG×6)
SIGNAL_COMMANDS = {
    'ECG': {
        'normal': '1',
        'bradycardia': '2',
        'tachycardia': '3',
        'afib': '4',
        'stemi': '5',
        'ischemia': '6',
        'svt': '7',           # Taquicardia supraventricular
        'vfib': '8'           # Fibrilación ventricular
    },
    'EMG': {
        'high': '9',
        'moderate': '10',
        'low': '11',
        'fatigue': '12',
        'tremor': '13',       # Tremor
        'spasm': '14'         # Espasmo
    },
    'PPG': {
        'normal': '15',
        'tachycardia': '16',
        'bradycardia': '17',
        'hypertension': '18', # Hipertensión
        'hypotension': '19',  # Hipotensión
        'arrhythmia': '20'    # Arritmia
    }
}

SIGNAL_NAMES = {
    # ECG (8 condiciones)
    '1': 'ECG_normal',
    '2': 'ECG_bradycardia',
    '3': 'ECG_tachycardia',
    '4': 'ECG_afib',
    '5': 'ECG_stemi',
    '6': 'ECG_ischemia',
    '7': 'ECG_svt',
    '8': 'ECG_vfib',
    # EMG (6 condiciones)
    '9': 'EMG_high',
    '10': 'EMG_moderate',
    '11': 'EMG_low',
    '12': 'EMG_fatigue',
    '13': 'EMG_tremor',
    '14': 'EMG_spasm',
    # PPG (6 condiciones)
    '15': 'PPG_normal',
    '16': 'PPG_tachycardia',
    '17': 'PPG_bradycardia',
    '18': 'PPG_hypertension',
    '19': 'PPG_hypotension',
    '20': 'PPG_arrhythmia'
}

# ==================== FUNCIONES ====================

def find_esp32_port():
    """Busca automáticamente el puerto COM del ESP32"""
    ports = serial.tools.list_ports.comports()
    for port in ports:
        # ESP32 suele aparecer como "USB Serial" o "CP210x"
        if 'USB' in port.description or 'CP210' in port.description or 'CH340' in port.description:
            return port.device
    return None

def create_output_dirs():
    """Crea directorios de salida si no existen"""
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    os.makedirs(f"{OUTPUT_DIR}/ECG", exist_ok=True)
    os.makedirs(f"{OUTPUT_DIR}/EMG", exist_ok=True)
    os.makedirs(f"{OUTPUT_DIR}/PPG", exist_ok=True)

def capture_signal_data(ser, num_samples=900, warmup_samples=50):
    """
    Captura datos del puerto serial
    
    Args:
        ser: Objeto serial
        num_samples: Número de muestras a capturar
        warmup_samples: Muestras iniciales a descartar (estabilización)
    
    Returns:
        numpy array con los datos capturados
    """
    print(f"  Descartando {warmup_samples} muestras de estabilización...")
    for _ in range(warmup_samples):
        try:
            ser.readline()
        except:
            pass
    
    print(f"  Capturando {num_samples} muestras...")
    data = []
    start_time = time.time()
    
    for i in range(num_samples):
        try:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            
            # El Serial Plotter de Arduino espera valores numéricos separados por comas o espacios
            # main_debug.cpp envía: Serial.println(signal_value);
            if line:
                try:
                    value = float(line)
                    data.append(value)
                    
                    # Progress indicator
                    if (i + 1) % 100 == 0:
                        elapsed = time.time() - start_time
                        progress = (i + 1) / num_samples * 100
                        print(f"  Progreso: {progress:.1f}% ({i+1}/{num_samples}) - {elapsed:.1f}s")
                except ValueError:
                    # Línea no numérica, probablemente mensaje de debug
                    pass
        except Exception as e:
            print(f"  Error leyendo línea: {e}")
    
    return np.array(data)

def save_stats_file(data, signal_name, condition, metadata, timestamp):
    """
    Guarda archivo TXT con estadísticas (similar al cuadrito del Serial Plotter)
    
    Args:
        data: Array numpy con los datos
        signal_name: Tipo de señal (ECG, EMG, PPG)
        condition: Condición específica
        metadata: Diccionario con información adicional
        timestamp: Timestamp de captura
    """
    stats_filename = f"{OUTPUT_DIR}/{signal_name}/stats_{signal_name}_{condition}_{timestamp}.txt"
    
    mean_val = np.mean(data)
    std_val = np.std(data)
    min_val = np.min(data)
    max_val = np.max(data)
    
    with open(stats_filename, 'w', encoding='utf-8') as f:
        f.write("="*60 + "\n")
        f.write(f"ESTADÍSTICAS DEL SERIAL PLOTTER\n")
        f.write(f"Señal: {signal_name} - {condition.upper()}\n")
        f.write("="*60 + "\n\n")
        
        f.write(f"Fecha/Hora captura: {timestamp}\n")
        f.write(f"Comando enviado: {metadata.get('command', 'N/A')}\n")
        f.write(f"Frecuencia de muestreo: {metadata.get('sampling_rate', 100)} Hz\n")
        f.write(f"Número de muestras: {len(data)}\n")
        f.write(f"Duración: {len(data) / metadata.get('sampling_rate', 100):.2f} segundos\n\n")
        
        f.write("-"*60 + "\n")
        f.write("MÉTRICAS ESTADÍSTICAS (similar a Serial Plotter)\n")
        f.write("-"*60 + "\n")
        f.write(f"Valor Mínimo:        {min_val:.6f}\n")
        f.write(f"Valor Máximo:        {max_val:.6f}\n")
        f.write(f"Rango (Max - Min):   {max_val - min_val:.6f}\n")
        f.write(f"Media (Promedio):    {mean_val:.6f}\n")
        f.write(f"Desviación Estándar: {std_val:.6f}\n")
        f.write(f"Varianza:            {np.var(data):.6f}\n")
        
        # Percentiles
        f.write(f"\nPercentiles:\n")
        f.write(f"  P25 (Q1):          {np.percentile(data, 25):.6f}\n")
        f.write(f"  P50 (Mediana):     {np.percentile(data, 50):.6f}\n")
        f.write(f"  P75 (Q3):          {np.percentile(data, 75):.6f}\n")
        f.write(f"  P95:               {np.percentile(data, 95):.6f}\n")
        f.write(f"  P99:               {np.percentile(data, 99):.6f}\n")
        
        f.write("\n" + "="*60 + "\n")
        f.write("Archivo generado automáticamente por auto_capture_plotter.py\n")
        f.write("="*60 + "\n")
    
    print(f"  ✓ Estadísticas guardadas: {stats_filename}")
    return stats_filename

def plot_signal(data, signal_name, condition, metadata):
    """
    Genera gráfico de alta calidad de la señal capturada (estilo Serial Plotter)
    
    Args:
        data: Array numpy con los datos
        signal_name: Tipo de señal (ECG, EMG, PPG)
        condition: Condición específica (normal, tachycardia, etc.)
        metadata: Diccionario con información adicional
    """
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = f"{OUTPUT_DIR}/{signal_name}/plotter_{signal_name}_{condition}_{timestamp}.png"
    
    # Configuración de figura de alta calidad (más ancha para emular Serial Plotter)
    fig, ax = plt.subplots(figsize=(18, 7), dpi=150)
    fig.patch.set_facecolor('#1E1E1E')  # Fondo oscuro estilo Arduino IDE
    ax.set_facecolor('#2D2D30')
    
    # Eje X en muestras (como Serial Plotter original)
    # También mostrar tiempo secundario
    sample_axis = np.arange(len(data))
    
    # Gráfico principal (color verde/cyan como Serial Plotter)
    ax.plot(sample_axis, data, linewidth=1.2, color='#00D9FF', alpha=0.95, label=f'{signal_name}_{condition}')
    
    # Grid estilo Serial Plotter
    ax.grid(True, alpha=0.2, linestyle='--', color='white', linewidth=0.5)
    ax.set_axisbelow(True)
    
    # Títulos y etiquetas (estilo claro sobre fondo oscuro)
    ax.set_title(f'{signal_name} - {condition.upper()} (Serial Plotter Capture)', 
                 fontsize=18, fontweight='bold', color='white', pad=20)
    ax.set_xlabel('Muestras', fontsize=13, color='white')
    ax.set_ylabel('Amplitud (mV / AU)', fontsize=13, color='white')
    
    # Ajustar colores de ejes
    ax.tick_params(axis='x', colors='white', labelsize=10)
    ax.tick_params(axis='y', colors='white', labelsize=10)
    ax.spines['bottom'].set_color('white')
    ax.spines['left'].set_color('white')
    ax.spines['top'].set_color('#2D2D30')
    ax.spines['right'].set_color('#2D2D30')
    
    # Leyenda
    ax.legend(loc='upper right', fontsize=10, framealpha=0.9)
    
    # CUADRITO DE ESTADÍSTICAS (estilo Serial Plotter Arduino)
    mean_val = np.mean(data)
    std_val = np.std(data)
    min_val = np.min(data)
    max_val = np.max(data)
    
    stats_text = '╔═══════════════════════════════╗\n'
    stats_text += '║   SERIAL PLOTTER STATS       ║\n'
    stats_text += '╠═══════════════════════════════╣\n'
    stats_text += f'║ Muestras: {len(data):>18} ║\n'
    stats_text += f'║ Min:      {min_val:>18.4f} ║\n'
    stats_text += f'║ Max:      {max_val:>18.4f} ║\n'
    stats_text += f'║ Rango:    {max_val - min_val:>18.4f} ║\n'
    stats_text += f'║ Media:    {mean_val:>18.4f} ║\n'
    stats_text += f'║ Std Dev:  {std_val:>18.4f} ║\n'
    stats_text += '╚═══════════════════════════════╝'
    
    # Posicionar cuadrito en esquina superior izquierda
    ax.text(0.015, 0.98, stats_text, transform=ax.transAxes,
            verticalalignment='top', fontsize=9, family='monospace',
            bbox=dict(boxstyle='round', facecolor='#1E1E1E', edgecolor='#00D9FF', 
                     alpha=0.95, linewidth=2),
            color='#00D9FF', weight='bold')
    
    # Timestamp
    ax.text(0.985, 0.015, f'Capturado: {timestamp}', transform=ax.transAxes,
            verticalalignment='bottom', horizontalalignment='right', fontsize=9,
            style='italic', color='#888888')
    
    plt.tight_layout()
    plt.savefig(filename, dpi=150, bbox_inches='tight', facecolor='#1E1E1E')
    plt.close()
    
    print(f"  ✓ Gráfico guardado: {filename}")
    
    # Guardar también archivo de estadísticas TXT
    save_stats_file(data, signal_name, condition, metadata, timestamp)
    
    return filename

def send_command_and_wait(ser, command, wait_time=2):
    """
    Envía comando serial y espera estabilización
    
    Args:
        ser: Objeto serial
        command: Comando a enviar (número de condición)
        wait_time: Tiempo de espera en segundos
    """
    print(f"  Enviando comando: {command}")
    ser.write(f"{command}\n".encode())
    ser.flush()
    
    print(f"  Esperando {wait_time}s para estabilización...")
    time.time()
    
    # Limpiar buffer
    ser.reset_input_buffer()

def capture_all_signals(port, samples=900):
    """
    Captura todas las señales y condiciones automáticamente
    
    Args:
        port: Puerto COM del ESP32
        samples: Número de muestras por captura
    
    Returns:
        Diccionario con resumen de capturas
    """
    print(f"\n{'='*60}")
    print("INICIANDO CAPTURA AUTOMÁTICA DE SERIAL PLOTTER")
    print(f"{'='*60}\n")
    
    print(f"Puerto: {port}")
    print(f"Baudrate: {BAUDRATE}")
    print(f"Muestras por señal: {samples}")
    print(f"Total de condiciones: {len(SIGNAL_NAMES)}")
    print()
    
    create_output_dirs()
    
    # Conectar al puerto serial
    try:
        ser = serial.Serial(port, BAUDRATE, timeout=1)
        time.sleep(2)  # Esperar conexión
        print(f"✓ Conectado a {port}\n")
    except Exception as e:
        print(f"✗ Error al conectar: {e}")
        return None
    
    results = {
        'timestamp': datetime.now().isoformat(),
        'port': port,
        'baudrate': BAUDRATE,
        'samples_per_signal': samples,
        'captures': []
    }
    
    total_conditions = len(SIGNAL_NAMES)
    current = 0
    
    # Iterar por todas las señales y condiciones
    for signal_type in SIGNAL_COMMANDS:
        for condition, command in SIGNAL_COMMANDS[signal_type].items():
            current += 1
            print(f"\n[{current}/{total_conditions}] Procesando: {signal_type} - {condition}")
            print("-" * 60)
            
            try:
                # Enviar comando para cambiar condición
                send_command_and_wait(ser, command, wait_time=2)
                
                # Capturar datos
                data = capture_signal_data(ser, num_samples=samples, warmup_samples=50)
                
                if len(data) > 0:
                    # Metadata
                    metadata = {
                        'signal_type': signal_type,
                        'condition': condition,
                        'command': command,
                        'samples': len(data),
                        'sampling_rate': 100 if signal_type == 'PPG' else (300 if signal_type == 'ECG' else 1000),
                        'mean': float(np.mean(data)),
                        'std': float(np.std(data)),
                        'min': float(np.min(data)),
                        'max': float(np.max(data))
                    }
                    
                    # Generar gráfico
                    filename = plot_signal(data, signal_type, condition, metadata)
                    metadata['filename'] = filename
                    
                    results['captures'].append(metadata)
                    print(f"  ✓ Completado exitosamente")
                else:
                    print(f"  ✗ No se capturaron datos")
                
            except Exception as e:
                print(f"  ✗ Error: {e}")
                continue
    
    # Cerrar puerto
    ser.close()
    print(f"\n✓ Puerto serial cerrado")
    
    # Guardar resumen JSON
    summary_file = f"{OUTPUT_DIR}/capture_summary_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
    with open(summary_file, 'w') as f:
        json.dump(results, f, indent=2)
    print(f"✓ Resumen guardado: {summary_file}")
    
    return results

def print_summary(results):
    """Imprime resumen de capturas"""
    if not results:
        return
    
    print(f"\n{'='*60}")
    print("RESUMEN DE CAPTURAS")
    print(f"{'='*60}\n")
    
    print(f"Hora de inicio: {results['timestamp']}")
    print(f"Total de capturas exitosas: {len(results['captures'])}/{len(SIGNAL_NAMES)}")
    print()
    
    # Por tipo de señal
    for signal_type in ['ECG', 'EMG', 'PPG']:
        captures = [c for c in results['captures'] if c['signal_type'] == signal_type]
        print(f"{signal_type}: {len(captures)} capturas")
        for cap in captures:
            print(f"  - {cap['condition']}: {cap['samples']} muestras, "
                  f"Media={cap['mean']:.3f}, SD={cap['std']:.3f}")
    
    print(f"\n✓ Imágenes guardadas en: {OUTPUT_DIR}/")
    print(f"✓ Listo para incluir en tesis!\n")

# ==================== MAIN ====================

if __name__ == "__main__":
    print("\n" + "="*60)
    print("AUTO CAPTURE SERIAL PLOTTER - BioSignalSimulator Pro")
    print("="*60 + "\n")
    
    # Buscar puerto ESP32
    port = find_esp32_port()
    
    if not port:
        print("✗ No se detectó ESP32 automáticamente.")
        print("\nPuertos disponibles:")
        ports = serial.tools.list_ports.comports()
        for i, p in enumerate(ports):
            print(f"  [{i}] {p.device} - {p.description}")
        
        if not ports:
            print("  (ninguno)")
            input("\nPresiona ENTER para salir...")
            exit(1)
        
        try:
            idx = int(input("\nSelecciona número de puerto: "))
            port = ports[idx].device
        except:
            print("Selección inválida")
            exit(1)
    else:
        print(f"✓ ESP32 detectado en: {port}")
        confirm = input(f"\n¿Proceder con captura automática? (s/n): ")
        if confirm.lower() != 's':
            print("Cancelado")
            exit(0)
    
    print("\n" + "="*60)
    print("INSTRUCCIONES IMPORTANTES:")
    print("="*60)
    print("1. Asegúrate de que main_debug.cpp esté compilado y subido al ESP32")
    print("2. Cierra el Serial Monitor/Plotter de PlatformIO si está abierto")
    print("3. El script enviará comandos 1-13 para cambiar condiciones")
    print("4. Se capturarán ~900 muestras por condición (13 condiciones total)")
    print("5. Tiempo estimado: ~5-10 minutos")
    print("="*60 + "\n")
    
    input("Presiona ENTER para iniciar captura automática...")
    
    # Ejecutar captura
    start_time = time.time()
    results = capture_all_signals(port, samples=SAMPLES_TO_CAPTURE)
    elapsed_time = time.time() - start_time
    
    # Mostrar resumen
    if results:
        print_summary(results)
        print(f"Tiempo total: {elapsed_time/60:.1f} minutos")
    
    print("\n✓ Proceso completado!")
    input("\nPresiona ENTER para salir...")
