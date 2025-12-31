# Debug: Componentes Nextion para Panel de Valores

## Problema
Los valores no se actualizan en el panel de la pantalla Nextion aunque los comandos se envían correctamente por serial.

## Comandos que se envían (verificado en serial monitor)
```
[TX] n_bpm.val=77
[TX] n_rr.val=777
[TX] n_ramp.val=110
[TX] n_st.val=0
[TX] n_beats.val=173
[TX] t_patol.txt="Normal"
```

## Componentes esperados en página waveform_ecg (página 5)

### Números (Number components)
- **n_bpm** (ID 12): Frecuencia cardíaca en BPM
- **n_rr** (ID 13): Intervalo RR en ms
- **n_ramp** (ID 14): Amplitud R × 100 (ej: 1.25mV → 125)
- **n_st** (ID 15): Desviación ST × 100 (ej: -0.15mV → -15)
- **n_beats** (ID 16): Contador de latidos

### Texto (Text components)
- **t_patol** (ID 6): Nombre de la patología/condición

## Verificación necesaria en Nextion Editor

Abre el archivo `BioSignalSimulatorPro.HMI` en Nextion Editor y verifica:

1. **Página 5 (waveform_ecg)** debe contener:
   - Componente Number con objname="n_bpm"
   - Componente Number con objname="n_rr"
   - Componente Number con objname="n_ramp"
   - Componente Number con objname="n_st"
   - Componente Number con objname="n_beats"
   - Componente Text con objname="t_patol"

2. Si los nombres son diferentes, hay 2 opciones:
   - **Opción A**: Renombrar los componentes en el HMI para que coincidan
   - **Opción B**: Modificar el código en `nextion_driver.cpp` líneas 718-725

## Solución temporal: Agregar debug en Nextion

En la página waveform_ecg, agrega un componente Text grande y en el evento Page Preinitialize:
```
t_debug.txt="Esperando datos..."
```

Luego verifica si el texto cambia cuando se envían comandos.

## Componentes EMG (página 7)
- **n_rms** (ID 11): Amplitud RMS × 100
- **n_mu** (ID 12): Unidades motoras activas
- **n_freq** (ID 13): Frecuencia × 10
- **n_cont** (ID 14): Contracción %
- **t_patol** (ID 6): Nombre de condición

## Componentes PPG (página 9)
- **n_hr** (ID 11): Frecuencia cardíaca
- **n_rr** (ID 12): Intervalo RR
- **n_pi** (ID 13): Índice de perfusión × 10
- **n_beats** (ID 14): Contador de latidos
- **t_patol** (ID 6): Nombre de condición
