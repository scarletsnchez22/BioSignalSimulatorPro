"""
Monitor de Métricas de Sistema ESP32
Captura latencia, estabilidad, consumo de recursos
"""

import serial
import time
import numpy as np
import matplotlib.pyplot as plt
from datetime import datetime
import json

class SystemMetricsMonitor:
    def __init__(self, port='COM4', baudrate=115200):
        self.port = port
        self.baudrate = baudrate
        self.ser = None
        
        self.metrics = {
            "latency": [],
            "timestamps": [],
            "packet_loss": 0,
            "total_packets": 0,
            "drift": []
        }
    
    def connect(self):
        """Conectar al ESP32"""
        try:
            self.ser = serial.Serial(self.port, self.baudrate, timeout=1)
            time.sleep(2)
            print(f"✓ Conectado a {self.port}")
            return True
        except Exception as e:
            print(f"✗ Error: {e}")
            return False
    
    def measure_latency(self, duration=60, expected_rate=200):
        """Mide latencia de respuesta UI (tiempo entre muestras)"""
        print(f"\n📊 Midiendo latencia por {duration}s...")
        print(f"📈 Tasa esperada: {expected_rate} Hz\n")
        
        last_time = None
        start_time = time.time()
        sample_count = 0
        
        while (time.time() - start_time) < duration:
            if self.ser and self.ser.in_waiting:
                try:
                    line = self.ser.readline().decode().strip()
                    current_time = time.time()
                    
                    if last_time is not None:
                        latency = (current_time - last_time) * 1000  # ms
                        self.metrics["latency"].append(latency)
                        self.metrics["timestamps"].append(current_time - start_time)
                    
                    last_time = current_time
                    sample_count += 1
                    self.metrics["total_packets"] += 1
                    
                    # Progreso cada 5 segundos
                    if int(current_time - start_time) % 5 == 0 and int(current_time - start_time) > 0:
                        elapsed = current_time - start_time
                        if int(elapsed) not in [int(t) for t in self.metrics["timestamps"][:-1]]:
                            actual_rate = sample_count / elapsed
                            print(f"  [{int(elapsed):3d}s] Muestras: {sample_count:5d} | Tasa: {actual_rate:.1f} Hz")
                
                except Exception as e:
                    self.metrics["packet_loss"] += 1
        
        print(f"\n✓ Captura completada: {sample_count} muestras")
    
    def analyze_stability(self):
        """Analiza estabilidad temporal (drift)"""
        if len(self.metrics["latency"]) < 10:
            print("⚠️  Datos insuficientes")
            return None
        
        latencies = np.array(self.metrics["latency"])
        
        # Dividir en ventanas de 10s para analizar drift
        window_size = 2000  # ~10s a 200Hz
        num_windows = len(latencies) // window_size
        
        window_means = []
        for i in range(num_windows):
            window = latencies[i*window_size:(i+1)*window_size]
            window_means.append(np.mean(window))
        
        if len(window_means) > 1:
            drift = np.std(window_means)
        else:
            drift = 0
        
        results = {
            "mean_latency": np.mean(latencies),
            "std_latency": np.std(latencies),
            "min_latency": np.min(latencies),
            "max_latency": np.max(latencies),
            "drift": drift,
            "packet_loss_rate": (self.metrics["packet_loss"] / self.metrics["total_packets"] * 100) if self.metrics["total_packets"] > 0 else 0,
            "total_samples": len(latencies)
        }
        
        return results
    
    def plot_metrics(self, results, save_path="results"):
        """Genera gráficos de métricas"""
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        
        fig, axes = plt.subplots(2, 2, figsize=(14, 10))
        
        latencies = np.array(self.metrics["latency"])
        timestamps = np.array(self.metrics["timestamps"])
        
        # 1. Latencia vs tiempo
        axes[0, 0].plot(timestamps, latencies, 'b-', linewidth=0.5, alpha=0.7)
        axes[0, 0].axhline(results["mean_latency"], color='r', linestyle='--', label=f'Media: {results["mean_latency"]:.2f} ms')
        axes[0, 0].set_xlabel('Tiempo (s)')
        axes[0, 0].set_ylabel('Latencia (ms)')
        axes[0, 0].set_title('Latencia de UI en Tiempo Real')
        axes[0, 0].legend()
        axes[0, 0].grid(True, alpha=0.3)
        
        # 2. Histograma de latencia
        axes[0, 1].hist(latencies, bins=50, color='skyblue', edgecolor='black', alpha=0.7)
        axes[0, 1].axvline(results["mean_latency"], color='r', linestyle='--', linewidth=2, label='Media')
        axes[0, 1].set_xlabel('Latencia (ms)')
        axes[0, 1].set_ylabel('Frecuencia')
        axes[0, 1].set_title('Distribución de Latencia')
        axes[0, 1].legend()
        axes[0, 1].grid(True, alpha=0.3)
        
        # 3. Análisis de drift (media móvil)
        window = 500
        moving_avg = np.convolve(latencies, np.ones(window)/window, mode='valid')
        axes[1, 0].plot(timestamps[:len(moving_avg)], moving_avg, 'g-', linewidth=1.5)
        axes[1, 0].set_xlabel('Tiempo (s)')
        axes[1, 0].set_ylabel('Latencia Media Móvil (ms)')
        axes[1, 0].set_title(f'Estabilidad Temporal (Drift: {results["drift"]:.3f} ms)')
        axes[1, 0].grid(True, alpha=0.3)
        
        # 4. Resumen de métricas
        axes[1, 1].axis('off')
        summary_text = f"""
RESUMEN DE MÉTRICAS DEL SISTEMA

Latencia UI:
  Media:     {results['mean_latency']:.2f} ms
  Desv. Std: {results['std_latency']:.2f} ms
  Min:       {results['min_latency']:.2f} ms
  Max:       {results['max_latency']:.2f} ms

Estabilidad:
  Drift:     {results['drift']:.3f} ms
  
Transmisión:
  Pérdida:   {results['packet_loss_rate']:.2f}%
  Muestras:  {results['total_samples']}
  
Evaluación:
  Latencia:  {"✓ EXCELENTE" if results['mean_latency'] < 10 else "⚠ ACEPTABLE" if results['mean_latency'] < 20 else "✗ MEJORAR"}
  Drift:     {"✓ ESTABLE" if results['drift'] < 1 else "⚠ MODERADO" if results['drift'] < 5 else "✗ INESTABLE"}
  Pérdidas:  {"✓ CONFIABLE" if results['packet_loss_rate'] < 1 else "⚠ ACEPTABLE" if results['packet_loss_rate'] < 5 else "✗ REVISAR"}
        """
        axes[1, 1].text(0.1, 0.5, summary_text, fontsize=11, family='monospace', 
                        verticalalignment='center')
        
        plt.tight_layout()
        plot_file = f"{save_path}/system_metrics_{timestamp}.png"
        plt.savefig(plot_file, dpi=300, bbox_inches='tight')
        print(f"\n✓ Gráficos guardados: {plot_file}")
        
        return plot_file
    
    def generate_report(self, results, save_path="results"):
        """Genera reporte de métricas"""
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        
        report = f"""
{'='*80}
REPORTE DE MÉTRICAS DEL SISTEMA ESP32
{'='*80}
Fecha: {datetime.now().strftime("%Y-%m-%d %H:%M:%S")}
Puerto: {self.port}
Baudrate: {self.baudrate}

LATENCIA DE INTERFAZ DE USUARIO
{'-'*80}
Latencia Media:        {results['mean_latency']:8.2f} ms
Desviación Estándar:   {results['std_latency']:8.2f} ms
Latencia Mínima:       {results['min_latency']:8.2f} ms
Latencia Máxima:       {results['max_latency']:8.2f} ms

ESTABILIDAD TEMPORAL
{'-'*80}
Drift:                 {results['drift']:8.3f} ms
Evaluación:            {"ESTABLE ✓" if results['drift'] < 1 else "MODERADO ⚠" if results['drift'] < 5 else "INESTABLE ✗"}

TRANSMISIÓN DE DATOS
{'-'*80}
Paquetes Totales:      {self.metrics['total_packets']:8d}
Paquetes Perdidos:     {self.metrics['packet_loss']:8d}
Tasa de Pérdida:       {results['packet_loss_rate']:8.2f} %
Muestras Válidas:      {results['total_samples']:8d}

EVALUACIÓN GENERAL
{'-'*80}
Rendimiento UI:        {"EXCELENTE ✓" if results['mean_latency'] < 10 else "ACEPTABLE ⚠" if results['mean_latency'] < 20 else "MEJORAR ✗"}
Confiabilidad:         {"CONFIABLE ✓" if results['packet_loss_rate'] < 1 else "ACEPTABLE ⚠" if results['packet_loss_rate'] < 5 else "REVISAR ✗"}

{'='*80}
"""
        
        # Guardar reporte
        report_file = f"{save_path}/system_metrics_{timestamp}.txt"
        with open(report_file, 'w') as f:
            f.write(report)
        
        # Guardar JSON
        json_file = f"{save_path}/system_metrics_{timestamp}.json"
        with open(json_file, 'w') as f:
            json.dump({
                "timestamp": timestamp,
                "port": self.port,
                "results": results,
                "raw_metrics": {
                    "latency": self.metrics["latency"][:1000],  # Solo primeros 1000 para no saturar
                    "packet_loss": self.metrics["packet_loss"],
                    "total_packets": self.metrics["total_packets"]
                }
            }, f, indent=2)
        
        print(report)
        print(f"✓ Reporte guardado: {report_file}")
        print(f"✓ JSON guardado: {json_file}")
        
        return report_file

def main():
    import argparse
    import os
    
    parser = argparse.ArgumentParser(description='Monitor de Métricas del Sistema')
    parser.add_argument('--port', default='COM4', help='Puerto serial')
    parser.add_argument('--duration', type=int, default=60, help='Duración de monitoreo (s)')
    parser.add_argument('--rate', type=int, default=200, help='Tasa esperada (Hz)')
    parser.add_argument('--output', default='results', help='Carpeta de salida')
    
    args = parser.parse_args()
    
    os.makedirs(args.output, exist_ok=True)
    
    monitor = SystemMetricsMonitor(port=args.port)
    
    if not monitor.connect():
        return
    
    print(f"\n🔬 Monitoreando métricas del sistema...")
    print(f"⏱️  Duración: {args.duration} segundos")
    print(f"📊 Tasa esperada: {args.rate} Hz\n")
    
    # Capturar métricas
    monitor.measure_latency(duration=args.duration, expected_rate=args.rate)
    
    # Analizar estabilidad
    results = monitor.analyze_stability()
    
    if results is None:
        return
    
    # Generar gráficos
    monitor.plot_metrics(results, args.output)
    
    # Generar reporte
    monitor.generate_report(results, args.output)
    
    monitor.ser.close()

if __name__ == "__main__":
    main()
