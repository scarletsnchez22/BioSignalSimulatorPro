"""
Script de Análisis por Lotes
Ejecuta todos los análisis automáticamente para la tesis
"""

import subprocess
import os
import sys
from datetime import datetime
import json

PYTHON = sys.executable

class BatchAnalyzer:
    def __init__(self, port='COM4', output_dir='thesis_results'):
        self.port = port
        self.output_dir = output_dir
        self.timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        self.batch_dir = f"{output_dir}/batch_{self.timestamp}"
        
        os.makedirs(self.batch_dir, exist_ok=True)
        
        self.results = {
            "timestamp": self.timestamp,
            "port": port,
            "analyses": []
        }
    
    def run_command(self, cmd, description):
        """Ejecuta un comando y registra resultado"""
        print(f"\n{'='*80}")
        print(f"🔬 {description}")
        print(f"{'='*80}\n")
        
        try:
            result = subprocess.run(cmd, shell=True, check=True, capture_output=True, text=True)
            print(result.stdout)
            
            self.results["analyses"].append({
                "description": description,
                "command": cmd,
                "status": "✓ SUCCESS",
                "timestamp": datetime.now().isoformat()
            })
            
            return True
        except subprocess.CalledProcessError as e:
            print(f"✗ Error: {e}")
            print(e.stderr)
            
            self.results["analyses"].append({
                "description": description,
                "command": cmd,
                "status": "✗ FAILED",
                "error": str(e),
                "timestamp": datetime.now().isoformat()
            })
            
            return False
    
    def analyze_all_signals(self):
        """Análisis completo de las 3 señales"""
        print(f"\n{'#'*80}")
        print(f"# ANÁLISIS AUTOMÁTICO POR LOTES - BioSimulator Pro")
        print(f"# Carpeta de salida: {self.batch_dir}")
        print(f"{'#'*80}\n")
        
        signals = ['ECG', 'EMG', 'PPG']
        
        # 1. Análisis FFT de modelos (sin hardware)
        print("\n" + "="*80)
        print("FASE 1: ANÁLISIS FFT DE MODELOS MATEMÁTICOS")
        print("="*80)
        
        for signal in signals:
            self.run_command(
                f'{PYTHON} tools/model_fft_analysis.py --signal {signal} --output "{self.batch_dir}/fft_models"',
                f'Análisis FFT - Modelo {signal}'
            )
        
        # 2. Análisis de parámetros temporales (requiere hardware)
        print("\n" + "="*80)
        print("FASE 2: ANÁLISIS DE PARÁMETROS TEMPORALES")
        print("="*80)
        print("\n⚠️  REQUERIDO: ESP32 conectado y generando señales")
        print("   1. Conectar ESP32 al puerto", self.port)
        print("   2. Seleccionar señal en Nextion")
        print("   3. Presionar PLAY\n")
        
        for signal in signals:
            input(f"\n▶ Presiona ENTER cuando {signal} esté ejecutándose en Nextion...")
            
            self.run_command(
                f'{PYTHON} tools/temporal_parameters_analyzer.py --port {self.port} --signal {signal} --duration 10 --output "{self.batch_dir}/temporal"',
                f'Parámetros Temporales - {signal}'
            )
        
        # 3. Métricas de sistema
        print("\n" + "="*80)
        print("FASE 3: MÉTRICAS DE SISTEMA")
        print("="*80)
        
        input("\n▶ Presiona ENTER para iniciar monitoreo de métricas (60s)...")
        
        self.run_command(
            f'{PYTHON} tools/system_metrics_monitor.py --port {self.port} --duration 60 --output "{self.batch_dir}/metrics"',
            'Métricas de Sistema (Latencia, Drift, Pérdidas)'
        )
        
        # 4. Análisis espectral desde hardware (opcional)
        print("\n" + "="*80)
        print("FASE 4: ANÁLISIS FFT DESDE HARDWARE (OPCIONAL)")
        print("="*80)
        
        do_hardware_fft = input("\n¿Ejecutar análisis FFT desde hardware? (s/n): ").lower() == 's'
        
        if do_hardware_fft:
            for signal in signals:
                input(f"\n▶ Presiona ENTER cuando {signal} esté ejecutándose...")
                
                self.run_command(
                    f'{PYTHON} tools/fft_spectrum_analyzer.py --port {self.port} --signal {signal} --duration 10 --output "{self.batch_dir}/fft_hardware"',
                    f'FFT Hardware - {signal}'
                )
        
        # Generar resumen
        self.generate_summary()
    
    def generate_summary(self):
        """Genera resumen del análisis por lotes"""
        summary_file = f"{self.batch_dir}/RESUMEN_ANALISIS.txt"
        json_file = f"{self.batch_dir}/analisis_batch.json"
        
        # Contar éxitos/fallos
        total = len(self.results["analyses"])
        success = sum(1 for a in self.results["analyses"] if "SUCCESS" in a["status"])
        failed = total - success
        
        summary = f"""
{'='*80}
RESUMEN DE ANÁLISIS POR LOTES
{'='*80}
Fecha: {datetime.now().strftime("%Y-%m-%d %H:%M:%S")}
Carpeta: {self.batch_dir}

ESTADÍSTICAS
{'-'*80}
Total de análisis:     {total}
Exitosos:              {success} ✓
Fallidos:              {failed} ✗
Tasa de éxito:         {success/total*100:.1f}%

ANÁLISIS EJECUTADOS
{'-'*80}
"""
        
        for i, analysis in enumerate(self.results["analyses"], 1):
            summary += f"{i:2d}. [{analysis['status']}] {analysis['description']}\n"
            summary += f"    Comando: {analysis['command']}\n"
            if "error" in analysis:
                summary += f"    Error: {analysis['error']}\n"
            summary += "\n"
        
        summary += f"""
{'='*80}
ARCHIVOS GENERADOS
{'='*80}

Carpeta: {self.batch_dir}/

Subcarpetas:
  - fft_models/     : Análisis FFT de modelos matemáticos
  - temporal/       : Parámetros temporales (PR, QRS, RMS, etc.)
  - metrics/        : Métricas de sistema (latencia, drift)
  - fft_hardware/   : FFT desde señal capturada (si se ejecutó)

Cada subcarpeta contiene:
  - Gráficos PNG (alta resolución 300 dpi)
  - Reportes TXT (legibles)
  - Datos JSON (procesables)

{'='*80}
PRÓXIMOS PASOS PARA LA TESIS
{'='*80}

1. Revisar gráficos generados en cada subcarpeta
2. Incluir tablas de parámetros en sección de Resultados
3. Comparar valores obtenidos vs. literatura clínica
4. Analizar métricas de sistema para discusión
5. Calcular estadísticas descriptivas (media, desv. std)

{'='*80}
"""
        
        # Guardar resumen
        with open(summary_file, 'w', encoding='utf-8') as f:
            f.write(summary)
        
        # Guardar JSON
        with open(json_file, 'w') as f:
            json.dump(self.results, f, indent=2)
        
        print("\n" + "="*80)
        print("✓ ANÁLISIS POR LOTES COMPLETADO")
        print("="*80)
        print(summary)
        print(f"\n✓ Resumen guardado: {summary_file}")
        print(f"✓ JSON guardado: {json_file}")
        print(f"\n📁 Todos los resultados en: {self.batch_dir}/\n")

def main():
    import argparse
    
    parser = argparse.ArgumentParser(description='Análisis Automático por Lotes para Tesis')
    parser.add_argument('--port', default='COM4', help='Puerto serial del ESP32')
    parser.add_argument('--output', default='thesis_results', help='Carpeta base de resultados')
    
    args = parser.parse_args()
    
    analyzer = BatchAnalyzer(port=args.port, output_dir=args.output)
    analyzer.analyze_all_signals()

if __name__ == "__main__":
    main()
