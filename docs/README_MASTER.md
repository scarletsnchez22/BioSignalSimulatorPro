# Capítulo 2: Metodología

**Autores:** Scarlet Gabriela Sánchez Aguirre, Rafael David Mata Puente  
**Institución:** Escuela Superior Politécnica del Litoral (ESPOL)  
**Facultad:** Ingeniería en Mecánica y Ciencias de la Producción  
**Fecha:** Diciembre 2025

---

## 2.1 Formulación de Alternativas de Solución

El desarrollo del simulador de señales biomédicas BioSimulator Pro requirió la evaluación sistemática de múltiples alternativas tecnológicas para cada subsistema. A continuación se presentan las matrices de decisión empleadas, fundamentadas en los criterios técnicos establecidos en el marco teórico y las restricciones del proyecto.

### 2.1.1 Subsistema de Generación de Señales

La generación de señales biomédicas sintéticas constituye el núcleo funcional del dispositivo. Se evaluaron tres enfoques fundamentalmente diferentes para la síntesis de señales ECG, EMG y PPG.

| Criterio | Peso | Tablas de Referencia | Modelos Matemáticos Dinámicos | Redes Neuronales Generativas |
|----------|------|---------------------|------------------------------|------------------------------|
| Fidelidad fisiológica | 25% | 2 (patrones fijos) | 5 (dinámica continua) | 4 (requiere entrenamiento) |
| Eficiencia computacional | 20% | 5 (lookup simple) | 4 (ODE en tiempo real) | 2 (alta carga GPU/TPU) |
| Flexibilidad paramétrica | 20% | 1 (valores discretos) | 5 (ajuste continuo) | 3 (latent space) |
| Implementabilidad en ESP32 | 20% | 5 (trivial) | 4 (optimización requerida) | 1 (memoria insuficiente) |
| Reproducibilidad científica | 15% | 3 (empírico) | 5 (ecuaciones publicadas) | 2 (caja negra) |
| **Total Ponderado** | 100% | **3.05** | **4.60** | **2.35** |

**Alternativa seleccionada: Modelos Matemáticos Dinámicos.** Esta decisión se fundamentó en la necesidad de generar señales con variabilidad fisiológica realista mientras se mantiene la trazabilidad científica. El modelo de McSharry et al. (2003) para ECG, el modelo de reclutamiento de Fuglevand et al. (1993) para EMG, y la aproximación de doble gaussiana de Allen (2007) para PPG proporcionan bases matemáticas sólidas y ampliamente validadas en la literatura biomédica. La implementación en ESP32 fue factible mediante optimizaciones específicas como la integración Runge-Kutta de cuarto orden con paso fijo y el uso de aritmética de punto flotante de precisión simple.

### 2.1.2 Subsistema Electrónico

El subsistema electrónico debía satisfacer requisitos de autonomía energética, calidad de señal analógica y compatibilidad con la pantalla táctil seleccionada.

| Criterio | Peso | Arduino Mega + Shield DAC | ESP32 + DAC Interno | Raspberry Pi Pico + MCP4725 |
|----------|------|--------------------------|--------------------|-----------------------------|
| Velocidad de procesamiento | 20% | 2 (16 MHz, single-core) | 5 (240 MHz, dual-core) | 3 (133 MHz, dual-core) |
| DAC integrado | 15% | 1 (requiere externo) | 4 (8-bit, 2 canales) | 1 (requiere I2C externo) |
| Conectividad WiFi nativa | 20% | 1 (módulo adicional) | 5 (integrado) | 1 (módulo adicional) |
| Consumo energético | 15% | 3 (50-80 mA) | 4 (80-160 mA con WiFi) | 4 (25-100 mA) |
| Ecosistema y documentación | 15% | 4 (maduro) | 5 (extenso, FreeRTOS) | 3 (emergente) |
| Costo | 15% | 3 (~$25 total) | 5 (~$8) | 4 (~$12) |
| **Total Ponderado** | 100% | **2.30** | **4.70** | **2.55** |

**Alternativa seleccionada: ESP32-WROOM-32.** El microcontrolador ESP32 fue seleccionado por su arquitectura dual-core que permite separar las tareas críticas de generación de señales de las funciones de comunicación WiFi. El DAC interno de 8 bits, aunque de resolución modesta, resultó suficiente para las aplicaciones educativas y de calibración previstas. La integración nativa de WiFi eliminó la necesidad de módulos adicionales, simplificando el diseño de PCB y reduciendo puntos de falla potenciales.

### 2.1.3 Subsistema Mecánico

La carcasa del dispositivo debía proporcionar protección, disipación térmica adecuada y ergonomía para uso en laboratorio.

| Criterio | Peso | Carcasa Comercial Adaptada | Impresión 3D PETG | Mecanizado CNC Aluminio |
|----------|------|---------------------------|-------------------|------------------------|
| Personalización dimensional | 25% | 2 (limitada) | 5 (total libertad) | 5 (alta precisión) |
| Disipación térmica | 20% | 3 (depende del modelo) | 3 (conductividad 0.29 W/m·K) | 5 (205 W/m·K) |
| Costo de fabricación | 20% | 4 ($15-30) | 5 ($5-10 material) | 1 ($100+) |
| Tiempo de fabricación | 15% | 5 (inmediato) | 3 (8-12 horas) | 2 (días-semanas) |
| Reproducibilidad | 10% | 2 (disponibilidad) | 5 (archivos STL) | 4 (planos técnicos) |
| Estética profesional | 10% | 3 (genérica) | 4 (acabado configurable) | 5 (industrial) |
| **Total Ponderado** | 100% | **3.05** | **4.25** | **3.35** |

**Alternativa seleccionada: Impresión 3D en PETG.** El PETG (Polietileno Tereftalato Glicol modificado) fue elegido sobre PLA y ABS por su combinación de resistencia térmica (temperatura de deflexión 70°C), resistencia química, y facilidad de impresión. El análisis térmico posterior confirmó que la potencia disipada por el sistema (aproximadamente 0.85W en operación típica) puede ser manejada mediante convección natural con ventilación pasiva integrada en el diseño de la carcasa.

---

## 2.2 Diseño Conceptual de la Solución

El sistema BioSimulator Pro se concibió como una solución mecatrónica integrada que combina cuatro subsistemas interdependientes: generación de señales, computacional, electrónico y mecánico. La arquitectura fue diseñada siguiendo principios de modularidad y separación de responsabilidades, permitiendo el desarrollo, prueba y mantenimiento independiente de cada componente.

### Arquitectura Global del Sistema

El siguiente diagrama ilustra la estructura jerárquica completa del producto, desde la capa de modelos matemáticos hasta las interfaces de usuario:

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                         BIOSIMULATOR PRO - ARQUITECTURA GLOBAL                  │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                 │
│  ┌─────────────────────────────────────────────────────────────────────────┐   │
│  │                    CAPA DE MODELOS MATEMÁTICOS                          │   │
│  │  ┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐           │   │
│  │  │   ECG Model     │ │   EMG Model     │ │   PPG Model     │           │   │
│  │  │  McSharry 2003  │ │ Fuglevand 1993  │ │   Allen 2007    │           │   │
│  │  │  ODE + RK4      │ │ Motor Units     │ │ Double Gaussian │           │   │
│  │  │  8 condiciones  │ │ Mexican Hat     │ │  6 condiciones  │           │   │
│  │  │  clínicas       │ │ 9 condiciones   │ │  PI dinámico    │           │   │
│  │  └────────┬────────┘ └────────┬────────┘ └────────┬────────┘           │   │
│  └───────────┼───────────────────┼───────────────────┼─────────────────────┘   │
│              │                   │                   │                         │
│              └───────────────────┼───────────────────┘                         │
│                                  ▼                                             │
│  ┌─────────────────────────────────────────────────────────────────────────┐   │
│  │                    CAPA DE PROCESAMIENTO (ESP32)                        │   │
│  │  ┌──────────────────────────┐    ┌──────────────────────────┐          │   │
│  │  │      PRO_CPU (Core 0)    │    │      APP_CPU (Core 1)    │          │   │
│  │  │  ┌────────────────────┐  │    │  ┌────────────────────┐  │          │   │
│  │  │  │ WiFi Stack         │  │    │  │ Timer ISR (1kHz)   │  │          │   │
│  │  │  │ WebSocket Server   │  │    │  │ Signal Generation  │  │          │   │
│  │  │  │ HTTP Server        │  │    │  │ DAC Output         │  │          │   │
│  │  │  │ mDNS Service       │  │    │  │ Nextion Comm       │  │          │   │
│  │  │  └────────────────────┘  │    │  └────────────────────┘  │          │   │
│  │  └──────────────────────────┘    └──────────────────────────┘          │   │
│  └─────────────────────────────────────────────────────────────────────────┘   │
│                                  │                                             │
│              ┌───────────────────┼───────────────────┐                         │
│              ▼                   ▼                   ▼                         │
│  ┌─────────────────┐ ┌─────────────────┐ ┌─────────────────────────────────┐   │
│  │  DAC Output     │ │ UART Nextion    │ │      WiFi Access Point          │   │
│  │  GPIO25/26      │ │ 115200 baud     │ │  SSID: BioSimulator_Pro         │   │
│  │  0-3.3V         │ │ HMI Protocol    │ │  IP: 192.168.4.1                │   │
│  └────────┬────────┘ └────────┬────────┘ └────────────────┬────────────────┘   │
│           │                   │                           │                    │
│           ▼                   ▼                           ▼                    │
│  ┌─────────────────────────────────────────────────────────────────────────┐   │
│  │                       CAPA DE INTERFACES DE USUARIO                     │   │
│  │  ┌─────────────────────┐  ┌─────────────────────┐  ┌─────────────────┐  │   │
│  │  │  Salida Analógica   │  │   Nextion 7" HMI    │  │   Web App       │  │   │
│  │  │  ┌───────────────┐  │  │  ┌───────────────┐  │  │  ┌───────────┐  │  │   │
│  │  │  │ Buffer TL072  │  │  │  │ Waveforms     │  │  │  │ WebSocket │  │  │   │
│  │  │  │ Divisor 2:1   │  │  │  │ Touch Control │  │  │  │ Real-time │  │  │   │
│  │  │  │ BNC Connector │  │  │  │ Parameters    │  │  │  │ Charts    │  │  │   │
│  │  │  └───────────────┘  │  │  └───────────────┘  │  │  └───────────┘  │  │   │
│  │  └─────────────────────┘  └─────────────────────┘  └─────────────────┘  │   │
│  └─────────────────────────────────────────────────────────────────────────┘   │
│                                                                                 │
│  ┌─────────────────────────────────────────────────────────────────────────┐   │
│  │                       CAPA DE ALIMENTACIÓN                              │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌───────────────┐   │   │
│  │  │ 2×18650     │─▶│ TP4056+DW01 │─▶│ MT3608      │─▶│ Reguladores   │   │   │
│  │  │ 7400mAh     │  │ Carga/Prot. │  │ Step-Up 5V  │  │ 3.3V internos │   │   │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └───────────────┘   │   │
│  └─────────────────────────────────────────────────────────────────────────┘   │
│                                                                                 │
│  ┌─────────────────────────────────────────────────────────────────────────┐   │
│  │                       CARCASA MECÁNICA (PETG)                           │   │
│  │  • Dimensiones: 200×150×45 mm    • Ventilación pasiva integrada         │   │
│  │  • Espesor pared: 3 mm           • Montaje para pantalla 7"             │   │
│  │  • Temp. operación: hasta 60°C   • Acceso a puertos USB y BNC           │   │
│  └─────────────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────────────┘
```

El diagrama anterior representa la filosofía de diseño adoptada: cada capa cumple una función específica y se comunica con las adyacentes mediante interfaces bien definidas. La **capa de modelos matemáticos** implementa los algoritmos de síntesis de señales basados en literatura científica peer-reviewed; cada modelo encapsula las ecuaciones diferenciales, parámetros fisiológicos y condiciones patológicas correspondientes. La **capa de procesamiento** aprovecha la arquitectura dual-core del ESP32 para garantizar determinismo temporal: el Core 1 (APP_CPU) ejecuta exclusivamente el timer de interrupción a 1 kHz y la generación de muestras, mientras el Core 0 (PRO_CPU) maneja toda la comunicación WiFi y el servidor web sin interferir con el timing crítico.

La **capa de interfaces** proporciona tres vías complementarias de interacción con el usuario. La salida analógica, acondicionada mediante buffer de alta impedancia y divisor resistivo, permite la conexión directa a osciloscopios y equipos de adquisición de datos. La pantalla Nextion de 7 pulgadas ofrece visualización en tiempo real y control táctil de todos los parámetros. La aplicación web, accesible mediante WiFi, extiende estas capacidades a cualquier dispositivo con navegador moderno, facilitando el uso en entornos educativos donde múltiples estudiantes pueden observar las señales simultáneamente.

La **capa de alimentación** garantiza autonomía mediante baterías recargables, con protecciones integradas contra sobrecarga, sobredescarga y cortocircuito. Finalmente, la **carcasa mecánica** proporciona el encapsulamiento físico con consideraciones térmicas y ergonómicas apropiadas para uso en laboratorio.

### Principios de Diseño Aplicados

El desarrollo siguió principios de ingeniería mecatrónica establecidos. El **principio de modularidad** permitió desarrollar y validar cada subsistema de forma independiente antes de la integración final. El **principio de separación de concerns** se aplicó tanto en software (arquitectura de capas, tareas FreeRTOS independientes) como en hardware (circuitos de potencia separados de señal). La **escalabilidad** fue considerada desde el inicio, permitiendo la adición futura de nuevos modelos de señales o interfaces sin modificar la arquitectura base.

En cuanto a normativas, el diseño electrónico siguió las recomendaciones de IPC-2221 para diseño de PCB, y la documentación técnica se estructuró según lineamientos de IEEE para especificaciones de sistemas embebidos.

---

## 2.3 Metodología del Diseño

### 2.3.1 Requerimientos del Diseño

La especificación de requerimientos se realizó mediante entrevistas con potenciales usuarios (docentes de fisiología, técnicos de laboratorio) y revisión de equipos comerciales existentes. Los requerimientos fueron clasificados según la metodología IEEE 830 en funcionales y no funcionales.

#### Requerimientos Funcionales

**RF-01: Generación de señal ECG.** El sistema debía generar señales de electrocardiograma con morfología PQRST realista, frecuencia cardíaca ajustable entre 30-200 BPM, y al menos 5 condiciones clínicas diferenciables (normal, taquicardia, bradicardia, fibrilación auricular, fibrilación ventricular, bloqueo AV, elevación ST, depresión ST). La amplitud debía ser configurable y la señal debía presentar variabilidad latido a latido (HRV) fisiológicamente plausible.

**RF-02: Generación de señal EMG.** El sistema debía generar señales de electromiografía superficial con amplitud proporcional al nivel de contracción muscular (0-100%), frecuencia de contenido espectral entre 20-500 Hz, y condiciones patológicas como miopatía, neuropatía y fasciculaciones. El modelo debía reflejar el principio de reclutamiento de Henneman.

**RF-03: Generación de señal PPG.** El sistema debía generar señales de fotopletismografía con morfología de pulso realista incluyendo onda dicrótica, índice de perfusión (PI) dinámico entre 0.1-20%, y condiciones como arritmia, vasoconstricción y perfusión débil.

**RF-04: Salida analógica calibrada.** El dispositivo debía proporcionar una salida de voltaje analógica en el rango 0-1.65V, compatible con osciloscopios y sistemas de adquisición de datos estándar, con impedancia de salida menor a 1 kΩ.

**RF-05: Visualización integrada.** Una pantalla táctil debía mostrar las formas de onda en tiempo real con actualización mínima de 20 fps, permitir la selección de tipo de señal y condición clínica, y ajustar parámetros mediante controles deslizantes.

**RF-06: Control remoto WiFi.** Una aplicación web accesible mediante conexión WiFi directa debía replicar todas las funcionalidades de la pantalla táctil, incluyendo visualización de waveforms y control de parámetros.

**RF-07: Métricas en tiempo real.** El sistema debía calcular y mostrar métricas derivadas: frecuencia cardíaca (HR), intervalo RR, índice de perfusión (PI), y amplitud RMS para EMG.

#### Requerimientos No Funcionales

**RNF-01: Autonomía.** El dispositivo debía operar al menos 4 horas con una carga completa de batería.

**RNF-02: Tiempo de arranque.** El sistema debía estar completamente operativo en menos de 5 segundos desde el encendido.

**RNF-03: Frecuencia de muestreo.** La tasa de generación de muestras debía ser de 1000 Hz para garantizar representación adecuada del contenido frecuencial de todas las señales (cumplimiento del teorema de Nyquist para componentes hasta 500 Hz).

**RNF-04: Latencia de visualización.** El retardo entre generación de muestra y visualización en pantalla debía ser inferior a 50 ms para percepción de tiempo real.

**RNF-05: Portabilidad.** El peso total del dispositivo no debía exceder 800 g y las dimensiones debían permitir transporte en maletín estándar.

**RNF-06: Temperatura de operación.** El dispositivo debía operar correctamente en el rango de temperatura ambiente de 15-35°C típico de laboratorios.

**RNF-07: Mantenibilidad.** El firmware debía ser actualizable via USB sin herramientas especiales, y el código fuente debía estar documentado para permitir modificaciones futuras.

**RNF-08: Costo.** El costo total de componentes no debía exceder $150 USD para mantener accesibilidad en contextos educativos.

---

*[Continúa en la siguiente sección: 2.3.2 Fase de Diseño]*

