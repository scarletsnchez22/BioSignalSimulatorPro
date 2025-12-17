# 📊 Tabla Completa de Elementos Nextion - BioSimulator Pro

## PÁGINA 0 - PORTADA (Splash)

| ID | Nombre Variable | Tipo | Evento Touch | UIEvent |
|----|-----------------|------|--------------|---------|
| 1 | `bt_comenzar` | Hotspot | Release | `BUTTON_COMENZAR` |

---

## PÁGINA 1 - MENU (Selección de Señal)

| ID | Nombre Variable | Tipo | Evento Touch | UIEvent |
|----|-----------------|------|--------------|---------|
| 1 | `bt_ecg` | Dual-State Button | Release | `BUTTON_ECG` |
| 2 | `bt_emg` | Dual-State Button | Release | `BUTTON_EMG` |
| 3 | `bt_ppg` | Dual-State Button | Release | `BUTTON_PPG` |
| 4 | `bt_ir` | Hotspot | Release | `BUTTON_IR` |
| 5 | `bt_atras` | Hotspot | Release | `BUTTON_ATRAS` |
| - | `sel_signal` | Number (Variable) | No | - |

---

## PÁGINA 2 - ECG_SIM (Selección Condición ECG)

| ID | Nombre Variable | Tipo | Evento Touch | UIEvent | Condición |
|----|-----------------|------|--------------|---------|-----------|
| 1 | `bt_atras` | Hotspot | Release | `BUTTON_ATRAS` | - |
| 2 | `bt_ir` | Hotspot | Release | `BUTTON_IR` | - |
| 4 | `bt_norm` (b4) | Dual-State Button | Release | `BUTTON_CONDITION` | Normal (0) |
| 5 | `bt_taq` (b5) | Dual-State Button | Release | `BUTTON_CONDITION` | Taquicardia (1) |
| 6 | `bt_bra` (b6) | Dual-State Button | Release | `BUTTON_CONDITION` | Bradicardia (2) |
| 7 | `bt_fa` (b7) | Dual-State Button | Release | `BUTTON_CONDITION` | FA (3) |
| 8 | `bt_fv` (b8) | Dual-State Button | Release | `BUTTON_CONDITION` | FV (4) |
| 9 | `bt_pvc` (b9) | Dual-State Button | Release | `BUTTON_CONDITION` | PVC (5) |
| 10 | `bt_brb` (b10) | Dual-State Button | Release | `BUTTON_CONDITION` | BRB (6) |
| 11 | `bt_stup` (b11) | Dual-State Button | Release | `BUTTON_CONDITION` | ST ↑ (7) |
| 12 | `bt_stdn` (b12) | Dual-State Button | Release | `BUTTON_CONDITION` | ST ↓ (8) |
| - | `sel_ecg` | Number (Variable) | No | - | - |

---

## PÁGINA 3 - EMG_SIM (Selección Condición EMG)

| ID | Nombre Variable | Tipo | Evento Touch | UIEvent | Condición |
|----|-----------------|------|--------------|---------|-----------|
| 1 | `bt_reposo` (b1) | Dual-State Button | Release | `BUTTON_CONDITION` | Reposo (0) |
| 2 | `bt_leve` (b2) | Dual-State Button | Release | `BUTTON_CONDITION` | Leve (1) |
| 3 | `bt_moderada` (b3) | Dual-State Button | Release | `BUTTON_CONDITION` | Moderada (2) |
| 4 | `bt_fuerte` (b4) | Dual-State Button | Release | `BUTTON_CONDITION` | Fuerte (3) |
| 5 | `bt_maxima` (b5) | Dual-State Button | Release | `BUTTON_CONDITION` | Máxima (4) |
| 6 | `bt_temblor` (b6) | Dual-State Button | Release | `BUTTON_CONDITION` | Temblor (5) |
| 7 | `bt_miopatia` (b7) | Dual-State Button | Release | `BUTTON_CONDITION` | Miopatía (6) |
| 8 | `bt_neuropatia` (b8) | Dual-State Button | Release | `BUTTON_CONDITION` | Neuropatía (7) |
| 9 | `bt_fasc` (b9) | Dual-State Button | Release | `BUTTON_CONDITION` | Fasciculación (8) |
| 10 | `bt_fatiga` (b10) | Dual-State Button | Release | `BUTTON_CONDITION` | Fatiga (9) |
| 12 | `bt_atras` | Hotspot | Release | `BUTTON_ATRAS` | - |
| 13 | `bt_ir` | Hotspot | Release | `BUTTON_IR` | - |
| - | `sel_emg` | Number (Variable) | No | - | - |

---

## PÁGINA 4 - PPG_SIM (Selección Condición PPG)

| ID | Nombre Variable | Tipo | Evento Touch | UIEvent | Condición |
|----|-----------------|------|--------------|---------|-----------|
| 1 | `bt_norm` (b1) | Dual-State Button | Release | `BUTTON_CONDITION` | Normal (0) |
| 2 | `bt_arr` (b2) | Dual-State Button | Release | `BUTTON_CONDITION` | Arritmia (1) |
| 3 | `bt_spo2` (b3) | Dual-State Button | Release | `BUTTON_CONDITION` | SpO2 bajo (2) |
| 4 | `bt_lowp` (b4) | Dual-State Button | Release | `BUTTON_CONDITION` | Perfusión débil (3) |
| 5 | `bt_highp` (b5) | Dual-State Button | Release | `BUTTON_CONDITION` | Perfusión fuerte (4) |
| 6 | `bt_vasc` (b6) | Dual-State Button | Release | `BUTTON_CONDITION` | Vasoconstricción (5) |
| 7 | `bt_art` (b7) | Dual-State Button | Release | `BUTTON_CONDITION` | Ruido/Movimiento (6) |
| 8 | `bt_atras` | Hotspot | Release | `BUTTON_ATRAS` | - |
| 9 | `bt_ir` | Hotspot | Release | `BUTTON_IR` | - |
| - | `sel_ppg` | Number (Variable) | No | - | - |

---

## PÁGINA 5 - WAVEFORM_ECG (Visualización ECG)

| ID | Nombre Variable | Tipo | Evento Touch | UIEvent |
|----|-----------------|------|--------------|---------|
| 1 | `ecg` | Waveform (399x211) | No | - |
| 2 | `v_actual` | Hotspot | Release | `BUTTON_VALORES` |
| 3 | `parametros` | Hotspot | Release | `BUTTON_PARAMETROS` |
| 4 | `play` | Hotspot | Release | `BUTTON_START` |
| 5 | `pause` | Hotspot | Release | `BUTTON_PAUSE` |
| 6 | `stop` | Hotspot | Release | `BUTTON_STOP` |

---

## PÁGINA 6 - VALORES_ECG (Popup Valores ECG)

| ID | Nombre Variable | Tipo | Evento Touch | UIEvent |
|----|-----------------|------|--------------|---------|
| 0 | `p0` | Picture | No | - |
| 1 | `valores` | Picture | No | - |
| 2 | `bt_act` | Button | Release | `BUTTON_BACK_POPUP` |
| 3 | `t_bpm` | Text | No | - |
| 4 | `t_rr` | Text | No | - |
| 5 | `t_ramp` | Text | No | - |
| 6 | `t_st` | Text | No | - |
| 7 | `n_bpm` | XFloat | No | - |
| 8 | `n_rr` | XFloat | No | - |
| 9 | `n_ramp` | XFloat | No | - |
| 10 | `n_st` | XFloat | No | - |
| 11 | `n_beats` | Number | No | - |
| 12 | `t_beats` | Text | No | - |
| 13 | `t_patol` | Text | No | - |

---

## PÁGINA 7 - PARAMETROS_ECG (Popup Parámetros ECG)

| ID | Nombre Variable | Tipo | Evento Touch | UIEvent |
|----|-----------------|------|--------------|---------|
| 0 | `p0` | Picture | No | - |
| 1 | `parametros` | Picture | No | - |
| 2 | `t_hr` | Text | No | - |
| 3 | `t_amp` | Text | No | - |
| 4 | `t_noise` | Text | No | - |
| 5 | `t_hrv` | Text | No | - |
| 6 | `n_hr` | XFloat | No | - |
| 7 | `n_amp` | XFloat | No | - |
| 8 | `n_noise` | XFloat | No | - |
| 9 | `n_hrv` | XFloat | No | - |
| 10 | `h_hr` | Slider | Release | `SLIDER_ECG_HR` |
| 11 | `h_amp` | Slider | Release | `SLIDER_ECG_AMP` |
| 12 | `h_noise` | Slider | Release | `SLIDER_ECG_NOISE` |
| 13 | `h_hrv` | Slider | Release | `SLIDER_ECG_HRV` |
| 14 | `p_reset` | Picture | No | - |
| 15 | `bt_reset` | Hotspot | Release | `BUTTON_RESET_PARAMS` |
| 16 | `bt_act` | Button | Release | `BUTTON_APPLY_PARAMS` |
| 17 | `bt_ex` | Hotspot | Release | `BUTTON_CANCEL_PARAMS` |

---

## PÁGINA 8 - WAVEFORM_EMG (Visualización EMG)

| ID | Nombre Variable | Tipo | Evento Touch | UIEvent |
|----|-----------------|------|--------------|---------|
| 1 | `emg` | Waveform (399x211) | No | - |
| 2 | `v_actual` | Hotspot | Release | `BUTTON_VALORES` |
| 3 | `parametros` | Hotspot | Release | `BUTTON_PARAMETROS` |
| 4 | `play` | Hotspot | Release | `BUTTON_START` |
| 5 | `pause` | Hotspot | Release | `BUTTON_PAUSE` |
| 6 | `stop` | Hotspot | Release | `BUTTON_STOP` |

---

## PÁGINA 9 - VALORES_EMG (Popup Valores EMG)

| ID | Nombre Variable | Tipo | Evento Touch | UIEvent |
|----|-----------------|------|--------------|---------|
| 0 | `p0` | Picture | No | - |
| 1 | `valores` | Picture | No | - |
| 2 | `bt_act` | Button | Release | `BUTTON_BACK_POPUP` |
| 3 | `t_rms` | Text | No | - |
| 4 | `t_mu` | Text | No | - |
| 5 | `t_freq` | Text | No | - |
| 6 | `t_cont` | Text | No | - |
| 7 | `n_rms` | XFloat/Number | No | - |
| 8 | `n_mu` | Number | No | - |
| 9 | `n_freq` | XFloat/Number | No | - |
| 10 | `n_cont` | Number | No | - |
| 11 | `t_patol` | Text | No | - |

---

## PÁGINA 10 - PARAMETROS_EMG (Popup Parámetros EMG)

| ID | Nombre Variable | Tipo | Evento Touch | UIEvent |
|----|-----------------|------|--------------|---------|
| 0 | `p0` | Picture | No | - |
| 1 | `parametros` | Picture | No | - |
| 2 | `t_exc` | Text | No | - |
| 3 | `t_amp` | Text | No | - |
| 4 | `t_noise` | Text | No | - |
| 5 | `n_exc` | XFloat | No | - |
| 6 | `n_amp` | XFloat | No | - |
| 7 | `n_noise` | XFloat | No | - |
| 8 | `h_exc` | Slider | Release | `SLIDER_EMG_EXC` |
| 9 | `h_amp` | Slider | Release | `SLIDER_EMG_AMP` |
| 10 | `h_noise` | Slider | Release | `SLIDER_EMG_NOISE` |
| 11 | `p_reset` | Picture | No | - |
| 12 | `bt_reset` | Hotspot | Release | `BUTTON_RESET_PARAMS` |
| 13 | `bt_act` | Button | Release | `BUTTON_APPLY_PARAMS` |
| 14 | `bt_ex` | Hotspot | Release | `BUTTON_CANCEL_PARAMS` |

---

## PÁGINA 11 - WAVEFORM_PPG (Visualización PPG)

| ID | Nombre Variable | Tipo | Evento Touch | UIEvent |
|----|-----------------|------|--------------|---------|
| 1 | `ppg` | Waveform (399x211) | No | - |
| 2 | `v_actual` | Hotspot | Release | `BUTTON_VALORES` |
| 3 | `parametros` | Hotspot | Release | `BUTTON_PARAMETROS` |
| 4 | `play` | Hotspot | Release | `BUTTON_START` |
| 5 | `pause` | Hotspot | Release | `BUTTON_PAUSE` |
| 6 | `stop` | Hotspot | Release | `BUTTON_STOP` |

---

## PÁGINA 12 - VALORES_PPG (Popup Valores PPG)

| ID | Nombre Variable | Tipo | Evento Touch | UIEvent |
|----|-----------------|------|--------------|---------|
| 0 | `p0` | Picture | No | - |
| 1 | `valores` | Picture | No | - |
| 2 | `bt_act` | Button | Release | `BUTTON_BACK_POPUP` |
| 3 | `t_hr` | Text | No | - |
| 4 | `t_rr` | Text | No | - |
| 5 | `t_pi` | Text | No | - |
| 6 | `t_spo2` | Text | No | - |
| 7 | `n_hr` | XFloat/Number | No | - |
| 8 | `n_rr` | XFloat/Number | No | - |
| 9 | `n_pi` | XFloat/Number | No | - |
| 10 | `n_spo2` | XFloat/Number | No | - |
| 11 | `t_beats` | Text | No | - |
| 12 | `n_beats` | Number | No | - |
| 13 | `t_patol` | Text | No | - |

---

## PÁGINA 13 - PARAMETROS_PPG (Popup Parámetros PPG)

| ID | Nombre Variable | Tipo | Evento Touch | UIEvent |
|----|-----------------|------|--------------|---------|
| 0 | `p0` | Picture | No | - |
| 1 | `parametros` | Picture | No | - |
| 2 | `t_hr` | Text | No | - |
| 3 | `t_pi` | Text | No | - |
| 4 | `t_noise` | Text | No | - |
| 5 | `n_hr` | XFloat | No | - |
| 6 | `n_pi` | XFloat | No | - |
| 7 | `n_noise` | XFloat | No | - |
| 8 | `h_hr` | Slider | Release | `SLIDER_PPG_HR` |
| 9 | `h_pi` | Slider | Release | `SLIDER_PPG_PI` |
| 10 | `h_noise` | Slider | Release | `SLIDER_PPG_NOISE` |
| 11 | `p_reset` | Picture | No | - |
| 12 | `bt_reset` | Hotspot | Release | `BUTTON_RESET_PARAMS` |
| 13 | `bt_act` | Button | Release | `BUTTON_APPLY_PARAMS` |
| 14 | `bt_ex` | Hotspot | Release | `BUTTON_CANCEL_PARAMS` |

---

## 📋 RESUMEN DE EVENTOS TOUCH/RELEASE

### ✅ Componentes CON Touch/Release Events
- **Botones (Buttons)**: `bt_comenzar`, `bt_ecg`, `bt_emg`, `bt_ppg`, `bt_norm`, `bt_taq`, `bt_bra`, `bt_fa`, `bt_fv`, `bt_pvc`, `bt_brb`, `bt_stup`, `bt_stdn`, `bt_reposo`, `bt_leve`, `bt_moderada`, `bt_fuerte`, `bt_maxima`, `bt_temblor`, `bt_miopatia`, `bt_neuropatia`, `bt_fasc`, `bt_fatiga`, `bt_arr`, `bt_spo2`, `bt_lowp`, `bt_highp`, `bt_vasc`, `bt_art`

- **Hotspots (Hotspot)**: `bt_ir`, `bt_atras`, `v_actual`, `parametros`, `play`, `pause`, `stop`, `bt_reset`, `bt_ex`, `bt_act`

- **Sliders (Slider)**: `h_hr`, `h_amp`, `h_noise`, `h_hrv`, `h_exc`, `h_pi`

### ❌ Componentes SIN Touch/Release Events
- **Pictures (Picture)**: `p0`, `valores`, `parametros`, `p_reset`
- **Waveforms (Waveform)**: `ecg`, `emg`, `ppg`
- **Numbers (Number)**: `sel_ecg`, `sel_emg`, `sel_ppg`, `sel_signal`, `n_beats`, `n_mu`, `n_cont`
- **Text (Text)**: Todas las etiquetas (t_bpm, t_rr, t_ramp, t_st, t_beats, t_patol, t_rms, t_mu, t_freq, t_cont, t_hr, t_amp, t_noise, t_hrv, t_exc, t_pi, t_spo2, t_rr, t_rr_ppg, t_spo2, t_pi)
- **XFloat (XFloat)**: Todas las variables display de valores (n_bpm, n_rr, n_ramp, n_st, n_rms, n_hr, n_pi, n_noise, n_hrv)

---

## 🔄 Diagrama de Flujo de Eventos

```
USUARIO TOCA BOTÓN/SLIDER EN NEXTION
  ↓
NEXTION ENVÍA: 0x65 [page] [id] [event] 0xFF 0xFF 0xFF
  ↓
NextionDriver::parseEvent() RECIBE Y DECODIFICA
  ↓
MAPEA: page + id → UIEvent (enum)
  ↓
EJECUTA: eventCallback(uiEvent, param)
  ↓
ESP32 GESTIONA EL EVENTO
  ↓
ACTUALIZA DISPLAY CON setText(), setNumber(), addWaveformPoint(), etc.
```

---

## 📝 Notas Técnicas

1. **Dual-State Buttons**: `.val=0` (deseleccionado) o `.val=1` (seleccionado)
2. **Hotspots**: Elementos invisibles que generan eventos de touch
3. **Sliders**: Generan evento al soltar (release) con el valor de 0-maxval
4. **Waveforms**: Actualizados solo con comando `add` o `cle`, no generan eventos de touch
5. **XFloat**: Variables numéricas con decimales para display
6. **Number**: Variables enteras para cálculos y lógica
7. **Touch Protocol**: Evento 1 = Release (fin de toque). Solo se procesan eventos de release.

---

**Última actualización**: 07.12.2025  
**Estado**: ✅ Completo y validado
