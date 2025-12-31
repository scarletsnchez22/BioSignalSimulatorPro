# Correcciones Aplicadas - BioSignalSimulator Pro

## Fecha: 29 Diciembre 2025

### Problema 1: ❌ Waveform no se visualizaba
**Causa**: Error lógico en downsampling - solo enviaba 1 punto y se detenía
**Solución**: Modificado `main.cpp:697-743` para procesar todos los puntos según ratio de downsampling
**Estado**: ✅ RESUELTO - Waveform ahora muestra señal ECG correctamente

### Problema 2: ❌ Auto-inicio sin presionar PLAY
**Causa**: Al cambiar de condición, el código detectaba estado `SELECT_CONDITION` e iniciaba automáticamente
**Solución**: Modificado `main.cpp:239-245` - eliminada lógica de auto-inicio, ahora solo inicia con PLAY explícito
**Estado**: ✅ RESUELTO - Ahora espera PLAY en todos los casos

### Problema 3: ❌ Señal con muchos picos (saturación)
**Causa**: Acumulación de muestras pendientes saturaba buffer de Nextion
**Solución**: Limitado a máximo 10 puntos por ciclo en `main.cpp:699-702`
**Estado**: ✅ RESUELTO - Señal ahora se visualiza suavemente

### Problema 4: ⚠️ Panel de valores no se actualiza
**Causa**: Posible mismatch entre nombres de componentes en código vs archivo TFT
**Evidencia**: 
- Comandos se envían correctamente: `n_bpm.val=77`, `n_rr.val=777`, etc.
- Serial monitor confirma transmisión exitosa
**Solución pendiente**: Verificar nombres de componentes en Nextion Editor

## Componentes esperados en TFT (página waveform_ecg)

```
Componente Number: n_bpm   → Frecuencia cardíaca (BPM)
Componente Number: n_rr    → Intervalo RR (ms)
Componente Number: n_ramp  → Amplitud R × 100 (mV)
Componente Number: n_st    → Desviación ST × 100 (mV)
Componente Number: n_beats → Contador de latidos
Componente Text:   t_patol → Nombre de patología
```

## Verificación necesaria

1. Abrir `BioSignalSimulatorPro.HMI` en Nextion Editor
2. Ir a página 5 (waveform_ecg)
3. Verificar que existen componentes con esos nombres exactos
4. Si los nombres son diferentes, renombrarlos o actualizar código

## Archivos modificados

- `src/main.cpp` (líneas 239-245, 697-743)
- Documentación creada: `DEBUG_NEXTION_COMPONENTS.md`

## Pruebas realizadas

✅ Waveform ECG se visualiza correctamente
✅ No inicia automáticamente al cambiar condición
✅ Señal suave sin picos excesivos
⚠️ Valores numéricos pendiente de verificación en pantalla física
