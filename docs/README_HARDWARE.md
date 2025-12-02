# 🔌 BioSignal Simulator Pro - Guía de Hardware

## Especificaciones Eléctricas y Esquemas de Conexión

---

## 1. COMPONENTES REQUERIDOS

### 1.1 Lista de Materiales (BOM)

| Componente | Modelo/Especificación | Cantidad | Notas |
|------------|----------------------|----------|-------|
| Microcontrolador | ESP32-WROOM-32 DevKit V1 | 1 | 240MHz dual-core, 320KB SRAM |
| Pantalla | Nextion NX4024T032 | 1 | 3.2", 320×240, UART |
| Regulador 5V | AMS1117-5.0 o LM7805 | 1 | Para Nextion desde 9-12V |
| Regulador 3.3V | AMS1117-3.3 (integrado en ESP32) | - | Ya incluido en DevKit |
| Capacitores | 100µF 16V electrolítico | 2 | Filtro alimentación |
| Capacitores | 100nF cerámico | 4 | Desacople |
| Resistencias | 10kΩ 1/4W | 2 | Pull-up/down |
| Resistencias | 330Ω 1/4W | 1 | LED indicador |
| LED RGB | 5mm cátodo común | 1 | Indicador de estado |
| Resistencias | 220Ω 1/4W | 3 | Limitadores LED RGB |
| Interruptor | SPST ON-OFF 3A | 1 | Power switch |
| Conector | Jack DC 5.5×2.1mm | 1 | Entrada alimentación |
| Conector | BNC hembra | 1 | Salida DAC (opcional) |
| Cable | Dupont hembra-hembra | 8 | Conexiones |
| PCB/Protoboard | - | 1 | Montaje |

### 1.2 Herramientas Necesarias

- Multímetro digital
- Osciloscopio (para verificar salida)
- Soldador y estaño (si usa PCB)
- Cable USB micro-B (programación)

---

## 2. ESPECIFICACIONES ESP32-WROOM-32

### 2.1 Características Relevantes

```
╔════════════════════════════════════════════════════════════════╗
║                    ESP32-WROOM-32 SPECS                        ║
╠════════════════════════════════════════════════════════════════╣
║  CPU         │ Xtensa LX6 Dual-Core @ 240 MHz                 ║
║  SRAM        │ 520 KB (usamos ~24 KB = 4.6%)                  ║
║  Flash       │ 4 MB (usamos ~330 KB = 8%)                     ║
║  DAC         │ 2 canales, 8-bit, 0-3.3V                       ║
║  ADC         │ 18 canales, 12-bit (no usado)                  ║
║  UART        │ 3 puertos (usamos UART0 + UART2)               ║
║  GPIO        │ 34 pines (usamos 5)                            ║
║  Alimentación│ 3.3V (regulador interno desde 5V USB)          ║
║  Consumo     │ ~80mA típico, ~240mA pico (WiFi off)           ║
╚════════════════════════════════════════════════════════════════╝
```

### 2.2 Pinout Utilizado

```
                         ESP32 DevKit V1
                        ┌───────────────┐
                   EN ──┤1            38├── GND
                GPIO36──┤2            37├── GPIO23
                GPIO39──┤3            36├── GPIO22
                GPIO34──┤4            35├── GPIO1 (TX0)
                GPIO35──┤5            34├── GPIO3 (RX0)
                GPIO32──┤6            33├── GPIO21
                GPIO33──┤7            32├── GND
         (DAC2) GPIO26──┤8            31├── GPIO19
    ★DAC OUT★   GPIO25──┤9            30├── GPIO18
                GPIO27──┤10           29├── GPIO5
                GPIO14──┤11           28├── GPIO17 ★TX2→Nextion RX
                GPIO12──┤12           27├── GPIO16 ★RX2→Nextion TX
                   GND──┤13           26├── GPIO4
                GPIO13──┤14           25├── GPIO0 (BOOT)
                 SD2  ──┤15           24├── GPIO2  ★LED STATUS
                 SD3  ──┤16           23├── GPIO15
                 CMD  ──┤17           22├── SD1
                 CLK  ──┤18           21├── SD0
                 SD0  ──┤19           20├── GND
                   5V ──┤20           19├── 3V3
                        └───────────────┘

    ★ = Pines utilizados en este proyecto
```

---

## 3. ESQUEMA DE CONEXIONES

### 3.1 Conexión Básica (Mínima)

```
                    CONEXIONES BÁSICAS
    ════════════════════════════════════════════════════════

         USB                    ESP32                  NEXTION
    ┌──────────┐           ┌──────────┐           ┌──────────┐
    │   PC     │           │          │           │NX4024T032│
    │          │◄─────────►│   3V3    │           │          │
    │ Monitor  │   USB     │          │           │  320×240 │
    │ Serial   │           │  GPIO25 ─┼───────────┼─► (ver)  │
    │          │           │          │           │          │
    └──────────┘           │  GPIO17 ─┼───────────┼─► RX     │
                           │  GPIO16 ◄┼───────────┼── TX     │
                           │      5V ─┼───────────┼─► +5V    │
                           │     GND ─┼───────────┼─► GND    │
                           │          │           │          │
                           │   GPIO2  │ (LED int) └──────────┘
                           │          │
                           └──────────┘
                                │
                                │ GPIO25 (DAC)
                                ▼
                    ┌─────────────────────┐
                    │    OSCILOSCOPIO     │
                    │   CH1 + GND común   │
                    │   0-3.3V, 1kHz      │
                    └─────────────────────┘
```

### 3.2 Esquema Completo con Alimentación Externa

```
    ESQUEMA COMPLETO - ALIMENTACIÓN EXTERNA
    ════════════════════════════════════════════════════════════════

                                    ┌─────────────────────────────┐
    FUENTE 9-12V DC                 │         ESP32 DevKit        │
    ┌──────────┐     SW1            │  ┌─────────────────────┐    │
    │  9-12V   │──┬──○/○──┐        │  │                     │    │
    │   2A     │  │       │        │  │  GPIO25 (DAC OUT)───┼────┼──► SALIDA
    │          │  │    ┌──┴──┐     │  │                     │    │    SEÑAL
    └──────────┘  │    │7805 │     │  │  GPIO17 (TX2)───────┼────┼──► Nextion RX
                  │    │     │     │  │  GPIO16 (RX2)◄──────┼────┼─── Nextion TX
              C1  │    └──┬──┘     │  │                     │    │
           100µF ═╧═     │  5V    │  │  GPIO2 (LED)────────┼──┐ │
                  │      │        │  │                     │  │ │
                 GND     ├────────┼──┼──► VIN (5V)         │  │ │
                         │        │  │                     │  │ │
                     C2 ═╧═       │  │  GND────────────────┼──┼─┼──► GND común
                   100µF │        │  │                     │  │ │
                        GND       │  └─────────────────────┘  │ │
                                  │                           │ │
                                  └───────────────────────────┼─┘
                                                              │
                                    ┌─────────────────────────┘
                                    │         R1
                                    ├────────/\/\/────┐
                                    │        330Ω     │
                                    │                LED1
                                    │                 │
                                    │               ──┴──
                                    │                GND
                                    │
                                    ▼
                           NEXTION NX4024T032
                          ┌────────────────────┐
                          │  +5V ◄─────────────┤ Rojo
                          │  GND ◄─────────────┤ Negro
                          │  RX  ◄─────────────┤ Amarillo (desde GPIO17)
                          │  TX  ──────────────┤ Azul (hacia GPIO16)
                          │                    │
                          │    ┌──────────┐    │
                          │    │ DISPLAY  │    │
                          │    │ 320×240  │    │
                          │    └──────────┘    │
                          └────────────────────┘
```

### 3.3 Tabla de Conexiones

| ESP32 Pin | Función | Conectar a | Color Cable | Notas |
|-----------|---------|------------|-------------|-------|
| GPIO25 | DAC1 Output | Osciloscopio CH1 / BNC | Verde | Señal 0-3.3V |
| GPIO17 | UART2 TX | Nextion RX | Amarillo | 9600 baud |
| GPIO16 | UART2 RX | Nextion TX | Azul | 9600 baud |
| GPIO2 | LED Status | LED + 330Ω → GND | - | Opcional externo |
| 5V (VIN) | Alimentación | Regulador 5V / USB | Rojo | Max 500mA |
| GND | Tierra | GND común | Negro | **Crítico** |
| 3V3 | Salida 3.3V | No conectar | - | Solo referencia |

---

## 4. ETAPA DE SALIDA DAC

### 4.1 Características de la Salida

```
ESPECIFICACIONES DAC ESP32:
═══════════════════════════════════════════════════════════════
│ Parámetro           │ Valor                                 │
═══════════════════════════════════════════════════════════════
│ Resolución          │ 8 bits (256 niveles)                  │
│ Rango de voltaje    │ 0V a 3.3V                             │
│ Impedancia salida   │ ~100Ω (interna)                       │
│ Corriente máxima    │ ~12mA (recomendado <5mA)              │
│ Frecuencia máxima   │ ~1 MHz (limitado por software a 1kHz) │
│ Linealidad          │ ±1 LSB típico                         │
═══════════════════════════════════════════════════════════════
```

### 4.2 Circuito Buffer de Salida (Recomendado)

Para conexión a equipos de medición o simulación de paciente:

```
    BUFFER DE SALIDA (OPCIONAL PERO RECOMENDADO)
    ═══════════════════════════════════════════════════════════

                              +5V
                               │
                               │ 10kΩ
                          ┌────┴────┐
                          │         │
    GPIO25 ───────────────┤─   +    ├──────┬─────► SALIDA
    (DAC)      │          │  LM358  │      │       BUFFERED
               │          │    A    │     ═╧═
              ═╧═         │─   -    │    10µF
             100nF        └────┬────┘      │
               │               │          GND
              GND              │
                               │
                               └──► Feedback (100%)
                               
    VENTAJAS:
    - Impedancia de salida ~1Ω (vs 100Ω del DAC)
    - Protección del ESP32
    - Capacidad de corriente mayor
    - Aislamiento del DAC
```

### 4.3 Conexión Directa a Osciloscopio

```
    CONEXIÓN DIRECTA (SIN BUFFER)
    ═══════════════════════════════════════════════════════════

    ESP32 GPIO25 ────────────────► Punta osciloscopio (CH1)
    
    ESP32 GND ───────────────────► Pinza cocodrilo (GND)
    
    CONFIGURACIÓN OSCILOSCOPIO:
    - Acoplamiento: DC
    - Escala vertical: 1V/div
    - Escala horizontal: 200ms/div (ECG), 50ms/div (EMG)
    - Trigger: Auto, nivel ~1.6V
    - Ancho de banda: 20MHz suficiente
```

---

## 5. ALIMENTACIÓN

### 5.1 Opciones de Alimentación

```
╔═══════════════════════════════════════════════════════════════════╗
║                    OPCIONES DE ALIMENTACIÓN                        ║
╠═══════════════════════════════════════════════════════════════════╣
║                                                                    ║
║  OPCIÓN A: USB (Desarrollo/Pruebas)                               ║
║  ─────────────────────────────────────                            ║
║  USB PC ──► ESP32 DevKit (regulador interno)                      ║
║           ──► 5V salida a Nextion                                 ║
║  Corriente: ~300mA total (ESP32 80mA + Nextion 200mA)             ║
║  ✓ Simple, ideal para desarrollo                                  ║
║  ✗ Dependiente del PC                                             ║
║                                                                    ║
║  OPCIÓN B: Fuente externa 5V 2A (Recomendada)                     ║
║  ─────────────────────────────────────────────                    ║
║  Adaptador 5V/2A ──► VIN ESP32 + 5V Nextion                       ║
║  ✓ Portátil, estable                                              ║
║  ✓ Suficiente corriente                                           ║
║                                                                    ║
║  OPCIÓN C: Fuente 9-12V + Regulador (Robusta)                     ║
║  ─────────────────────────────────────────────                    ║
║  9-12V DC ──► LM7805 ──► 5V (ESP32 + Nextion)                     ║
║  ✓ Mayor margen de voltaje entrada                                ║
║  ✓ Mejor filtrado de ruido                                        ║
║  ✗ Requiere disipador en 7805                                     ║
║                                                                    ║
╚═══════════════════════════════════════════════════════════════════╝
```

### 5.2 Cálculo de Consumo

```
PRESUPUESTO DE CORRIENTE:
═══════════════════════════════════════════════════════════════
│ Componente          │ Corriente típica │ Corriente pico     │
═══════════════════════════════════════════════════════════════
│ ESP32-WROOM-32      │ 80 mA            │ 240 mA (WiFi ON)   │
│ Nextion NX4024T032  │ 200 mA           │ 300 mA (backlight) │
│ LED indicador       │ 10 mA            │ 20 mA              │
│ Buffer LM358        │ 5 mA             │ 10 mA              │
├─────────────────────┼──────────────────┼────────────────────┤
│ TOTAL               │ ~295 mA          │ ~570 mA            │
═══════════════════════════════════════════════════════════════

RECOMENDACIÓN: Fuente de 5V / 1A mínimo (2A recomendado)
```

### 5.3 Esquema con Interruptor Power ON/OFF

```
    CIRCUITO DE ALIMENTACIÓN CON POWER SWITCH
    ═══════════════════════════════════════════════════════════

    Fuente DC 5V/2A
    ┌──────────────┐
    │    (+) ──────┼────┬────○/○────┬────────► +5V Sistema
    │              │    │   SW1     │
    │              │   ═╧═ 100µF   ═╧═ 100nF
    │              │    │           │
    │              │   GND         GND
    │              │
    │    (-) ──────┼────────────────────────► GND Sistema
    └──────────────┘

    SW1: Interruptor SPST (Single Pole Single Throw)
         - Corriente: mínimo 2A
         - Voltaje: mínimo 12V DC
         - Tipo recomendado: Rocker switch iluminado
         
    NOTA: El interruptor va en la línea POSITIVA (+5V)
          NUNCA interrumpir solo el GND
```

---

## 6. LED DE ESTADO

### 6.1 LED Interno (GPIO2)

El ESP32 DevKit tiene un LED azul conectado a GPIO2. Este se usa automáticamente:

| Estado LED | Significado |
|------------|-------------|
| Encendido fijo | Sistema listo, sin señal activa |
| Parpadeo 1Hz | Señal generándose activamente |
| Apagado | Error de inicialización |

### 6.2 LED RGB Externo (Indicador de Estado del Sistema)

El sistema soporta un LED RGB para indicar visualmente el estado:

| Color | Estado | Significado |
|-------|--------|-------------|
| 🔴 **Rojo** | OFF/ERROR | Sistema apagado o error |
| 🟢 **Verde** | READY | Sistema listo, esperando |
| 🔵 **Azul** | SIMULATING | Generando señal activamente |
| 🩵 **Cyan** | PAUSED | Señal pausada |

```
    LED RGB - CONEXIÓN (CÁTODO COMÚN)
    ═══════════════════════════════════════════════════════════

                      LED RGB (5mm cátodo común)
                           ┌─────┐
    GPIO4  ────/\/\/───────┤ R   │
               220Ω        │     │
    GPIO5  ────/\/\/───────┤ G   ├──────► GND (cátodo)
               220Ω        │     │
    GPIO18 ────/\/\/───────┤ B   │
               220Ω        └─────┘

    PINES ASIGNADOS (configurables en config.h):
    - LED_RGB_RED   = GPIO4
    - LED_RGB_GREEN = GPIO5
    - LED_RGB_BLUE  = GPIO18

    PARA LED ÁNODO COMÚN:
    - Cambiar LED_RGB_COMMON_ANODE a true en config.h
    - Conectar el pin común a +3.3V en lugar de GND
    
    CÁLCULO RESISTENCIAS:
    V_GPIO = 3.3V, V_LED ≈ 2.0V (rojo), 3.0V (azul/verde)
    I_LED = 10mA → R = (3.3 - 2.5) / 0.01 = 80Ω mínimo
    Usar 220Ω para protección y brillo moderado (~5mA)
```

---

## 7. CONSIDERACIONES TÉRMICAS

### 7.1 ¿Necesito Ventilador?

```
ANÁLISIS TÉRMICO:
═══════════════════════════════════════════════════════════════
│ Componente          │ Disipación  │ Temp. Max   │ Riesgo    │
═══════════════════════════════════════════════════════════════
│ ESP32-WROOM-32      │ ~0.4W       │ 85°C        │ BAJO      │
│ LM7805 (si se usa)  │ ~1.4W*      │ 125°C       │ MEDIO     │
│ Nextion             │ ~1W         │ 70°C        │ BAJO      │
═══════════════════════════════════════════════════════════════

* LM7805 con entrada 12V: P = (12V - 5V) × 0.2A = 1.4W

CONCLUSIÓN:
- Sin regulador externo (USB o 5V directo): NO necesita ventilador
- Con LM7805 desde 12V: Usar disipador TO-220 (no ventilador)
- Ambiente >35°C: Considerar ventilación pasiva (orificios)
```

### 7.2 Si Decides Usar Ventilador

```
    VENTILADOR OPCIONAL (Solo ambientes calientes)
    ═══════════════════════════════════════════════════════════

    5V ──────────┬────────────────► (+) Ventilador 5V 30×30mm
                 │
                ═╧═ 100µF
                 │
                GND ─────────────► (-) Ventilador

    ESPECIFICACIONES RECOMENDADAS:
    - Tamaño: 30×30×10mm o 40×40×10mm
    - Voltaje: 5V DC
    - Corriente: <100mA
    - Flujo: >5 CFM
    - Ruido: <25 dBA
```

---

## 8. CONSIDERACIONES DE SEGURIDAD (IEC 60601)

### 8.1 Advertencia Importante

```
╔═══════════════════════════════════════════════════════════════════╗
║  ⚠️  ADVERTENCIA - USO EDUCATIVO ÚNICAMENTE  ⚠️                    ║
╠═══════════════════════════════════════════════════════════════════╣
║                                                                    ║
║  Este dispositivo NO está certificado para uso clínico.           ║
║  NO conectar a pacientes reales bajo ninguna circunstancia.       ║
║                                                                    ║
║  Para uso en entornos clínicos/médicos, se requiere:              ║
║  - Certificación IEC 60601-1 (seguridad eléctrica)                ║
║  - Aislamiento galvánico de grado médico                          ║
║  - Pruebas de compatibilidad electromagnética (EMC)               ║
║  - Validación de software según IEC 62304                         ║
║                                                                    ║
╚═══════════════════════════════════════════════════════════════════╝
```

### 8.2 Buenas Prácticas Implementadas

| Práctica | Implementación |
|----------|---------------|
| Aislamiento lógico | Nextion separado por UART (aislamiento capacitivo) |
| Limitación corriente | DAC <12mA, buffer si se requiere más |
| Sin conexión a red | WiFi/BT deshabilitados por defecto |
| Voltajes seguros | Máximo 5V en cualquier punto |
| GND común | Evita lazos de tierra |

### 8.3 Mejoras Opcionales para Mayor Seguridad

```
    AISLAMIENTO ÓPTICO (Para aplicaciones más seguras)
    ═══════════════════════════════════════════════════════════

    GPIO25 ──────┬───► HCPL-0601 ───┬──► SALIDA AISLADA
                 │    (Optoacoplador)│
               3.3V               5V_ISO (fuente aislada)
                 │                   │
                GND               GND_ISO

    Proporciona aislamiento galvánico de ~3750V RMS
```

---

## 9. MONTAJE Y VERIFICACIÓN

### 9.1 Lista de Verificación Pre-Encendido

- [ ] Verificar polaridad de alimentación
- [ ] Verificar conexiones TX↔RX (cruzadas)
- [ ] Verificar GND común entre ESP32 y Nextion
- [ ] No hay cortocircuitos visibles
- [ ] Cables bien sujetos
- [ ] USB desconectado si usa alimentación externa

### 9.2 Secuencia de Prueba

```
1. ALIMENTACIÓN
   - Conectar USB al PC (o fuente externa)
   - LED GPIO2 debe encender
   - Monitor serial: mensaje de bienvenida

2. NEXTION
   - Pantalla debe mostrar SPLASH
   - Si queda en blanco: verificar TX/RX

3. SEÑAL DAC
   - Abrir monitor serial (115200 baud)
   - Escribir 'e' + Enter, luego '1' + Enter
   - LED debe parpadear
   - Osciloscopio: señal ECG en GPIO25

4. CONTROL
   - 'p' para pausar (señal se congela)
   - 'r' para reanudar
   - 's' para detener
```

---

## 10. SOLUCIÓN DE PROBLEMAS

| Síntoma | Causa Probable | Solución |
|---------|----------------|----------|
| ESP32 no enciende | Polaridad invertida | Verificar +/- |
| Nextion en blanco | TX/RX invertidos | Cruzar cables |
| Sin señal en DAC | Señal no iniciada | Comando 'e1' |
| Señal ruidosa | GND flotante | Verificar tierra |
| ESP32 se reinicia | Insuficiente corriente | Fuente 2A |
| Nextion parpadea | Alimentación débil | Capacitor 100µF |

---

*Hardware Guide - BioSignal Simulator Pro v2.0*
*Última actualización: Diciembre 2025*
