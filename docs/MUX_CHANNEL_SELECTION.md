# Selección Automática de Canal MUX por Señal

**Fecha:** 09 Enero 2026  
**Versión:** 1.1.0  
**Archivo:** `src/core/signal_engine.cpp`

## Problema Identificado

El multiplexor CD4051 tiene 3 canales físicos conectados con diferentes filtros RC:

- **CH0**: R=6.8kΩ → Fc=23.4 Hz (óptimo para ECG)
- **CH1**: R=1.0kΩ → Fc=159 Hz (óptimo para EMG)
- **CH2**: R=33kΩ → Fc=4.8 Hz (óptimo para PPG)

### ❌ Problema Anterior
El canal MUX se mantenía en **CH1 (por defecto)** sin importar qué señal se generaba:
- ECG usaba filtro incorrecto (159 Hz en vez de 23.4 Hz)
- PPG usaba filtro incorrecto (159 Hz en vez de 4.8 Hz)
- Solo EMG usaba el filtro correcto

Esto causaba:
1. **Espectro de frecuencia incorrecto** en ECG y PPG
2. **Pérdida de morfología** en señales de baja frecuencia
3. **Atenuación incorrecta** de componentes de alta frecuencia

## Solución Implementada

### Selección Automática en `startSignal()`

```cpp
switch (type) {
    case SignalType::ECG:
        mux.selectChannel(MuxChannel::CH0_6K8_OHM);  // Fc=23.4 Hz
        break;
    case SignalType::EMG:
        mux.selectChannel(MuxChannel::CH1_DIRECT);    // Fc=159 Hz
        break;
    case SignalType::PPG:
        mux.selectChannel(MuxChannel::CH2_33K_OHM);   // Fc=4.8 Hz
        break;
}
```

### Logging Mejorado

Ahora se muestra en el Serial Monitor:
```
[MUX] Canal seleccionado: CH0 (6.8k ohm, Fc=23.4 Hz) para ECG
[MUX] Canal activo: 0 (CH0 (6.8k ohm - Atten Media)), Factor atenuación: 0.60
```

## Preocupación sobre Suma Pasiva de Resistencias

### Teoría del CD4051

En el CD4051, cuando un canal está **desactivado**:
- **Impedancia OFF**: >100 MΩ (según datasheet)
- Las resistencias de los canales inactivos **NO deberían** afectar la señal
- Solo el canal activo tiene baja resistencia (<100Ω)

### Red Física (según diagrama)

```
DAC (GPIO25) → LM358 → CD4051 COM

             CH0 ──[6.8kΩ]──┐
             CH1 ──[1.0kΩ]──┼── NODO_A ──[1µF]── GND
             CH2 ──[33kΩ]───┘              │
                                           └──► BNC (+)
```

### ⚠️ Posibles Problemas

1. **Crosstalk entre canales** si:
   - El CD4051 tiene fugas (chip defectuoso)
   - Las pistas PCB están muy cercanas
   - Hay capacitancia parásita entre canales

2. **Divisor resistivo no deseado** si:
   - La impedancia OFF no es tan alta como especifica el datasheet
   - Hay humedad o contaminación en el PCB

## Verificación Experimental

### Comandos de Prueba Serial

```
# Cambiar canales manualmente
mux0    # CH0 - 6.8k ohm (ECG)
mux1    # CH1 - 1.0k ohm (EMG)
mux2    # CH2 - 33k ohm (PPG)
info    # Ver canal actual
```

### Mediciones Recomendadas

1. **Con osciloscopio conectado al BNC**:
   - Generar ECG → Verificar que CH0 está activo
   - Medir frecuencia de corte real (debería ser ~23 Hz)
   - Repetir para EMG (CH1, ~159 Hz) y PPG (CH2, ~4.8 Hz)

2. **Prueba de crosstalk**:
   - Seleccionar CH0
   - Medir resistencia DC entre NODO_A y CH1/CH2
   - Debería ser >100 MΩ

3. **FFT de la señal de salida**:
   - Comparar con FFT esperado del modelo
   - Verificar que Fc medido coincide con el calculado

## Resultados Esperados

### ✅ Después del Fix

| Señal | Canal | R (kΩ) | Fc (Hz) | F99% (Hz) |
|-------|-------|--------|---------|-----------|
| ECG   | CH0   | 6.8    | 23.4    | 21.6      |
| EMG   | CH1   | 1.0    | 159.2   | 146.4     |
| PPG   | CH2   | 33.0   | 4.8     | 4.9       |

## Archivos Modificados

- [src/core/signal_engine.cpp](../src/core/signal_engine.cpp#L118-L145)
  - Agregado `#include "hw/cd4051_mux.h"`
  - Agregado `extern CD4051Mux mux;`
  - Agregada selección automática en `startSignal()`
  - Agregado logging de canal MUX

## Compilación

```bash
✅ Compilado exitosamente
RAM:   16.7% (54660 bytes)
Flash: 71.6% (938733 bytes)
```
