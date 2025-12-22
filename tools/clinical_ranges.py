"""
clinical_ranges.py - Tablas de Rangos Clínicos para Validación de Señales Biológicas

================================================================================
REFERENCIAS CIENTÍFICAS COMPLETAS (Verificables en PubMed/Google Scholar)
================================================================================

ECG - ELECTROCARDIOGRAFÍA:
--------------------------
[1] McSharry PE, Clifford GD, Tarassenko L, Smith LA.
    "A dynamical model for generating synthetic electrocardiogram signals."
    IEEE Trans Biomed Eng. 2003;50(3):289-294.
    DOI: 10.1109/TBME.2003.808805
    
[2] Task Force of ESC and NASPE.
    "Heart rate variability: standards of measurement, physiological 
    interpretation and clinical use."
    Circulation. 1996;93(5):1043-1065.
    DOI: 10.1161/01.CIR.93.5.1043
    
[3] Surawicz B, Knilans TK.
    "Chou's Electrocardiography in Clinical Practice." 6th ed.
    Philadelphia: Saunders Elsevier; 2008. ISBN: 978-1416037743
    
[4] Goldberger AL, Goldberger ZD, Shvilkin A.
    "Goldberger's Clinical Electrocardiography: A Simplified Approach." 9th ed.
    Philadelphia: Elsevier; 2017. ISBN: 978-0323401692
    
[5] Thygesen K, Alpert JS, Jaffe AS, et al.
    "Fourth Universal Definition of Myocardial Infarction (2018)."
    Circulation. 2018;138(20):e618-e651.
    DOI: 10.1161/CIR.0000000000000617

EMG - ELECTROMIOGRAFÍA:
-----------------------
[6] Fuglevand AJ, Winter DA, Patla AE.
    "Models of recruitment and rate coding organization in motor-unit pools."
    J Neurophysiol. 1993;70(6):2470-2488.
    DOI: 10.1152/jn.1993.70.6.2470
    
[7] De Luca CJ.
    "The use of surface electromyography in biomechanics."
    J Appl Biomech. 1997;13(2):135-163.
    DOI: 10.1123/jab.13.2.135
    
[8] De Luca CJ, Hostage EC.
    "Relationship between firing rate and recruitment threshold of motoneurons 
    in voluntary isometric contractions."
    J Neurophysiol. 2010;104(2):1034-1046.
    DOI: 10.1152/jn.01018.2009
    
[9] Henneman E, Somjen G, Carpenter DO.
    "Functional significance of cell size in spinal motoneurons."
    J Neurophysiol. 1965;28:560-580.
    DOI: 10.1152/jn.1965.28.3.560
    
[10] Kimura J.
     "Electrodiagnosis in Diseases of Nerve and Muscle: Principles and Practice."
     4th ed. Oxford University Press; 2013. ISBN: 978-0199738687
     
[11] Deuschl G, Bain P, Brin M.
     "Consensus statement of the Movement Disorder Society on Tremor."
     Mov Disord. 1998;13(Suppl 3):2-23.
     DOI: 10.1002/mds.870131303

PPG - FOTOPLETISMOGRAFÍA:
-------------------------
[12] Allen J.
     "Photoplethysmography and its application in clinical physiological 
     measurement."
     Physiol Meas. 2007;28(3):R1-R39.
     DOI: 10.1088/0967-3334/28/3/R01
     
[13] Lima A, Bakker J.
     "Noninvasive monitoring of peripheral perfusion."
     Intensive Care Med. 2005;31(10):1316-1326.
     DOI: 10.1007/s00134-005-2790-2
     
[14] Reisner A, Shaltis PA, McCombie D, Asada HH.
     "Utility of the photoplethysmogram in circulatory monitoring."
     Anesthesiology. 2008;108(5):950-958.
     DOI: 10.1097/ALN.0b013e31816c89e1
     
[15] Jubran A.
     "Pulse oximetry."
     Crit Care. 2015;19(1):272.
     DOI: 10.1186/s13054-015-0984-8
"""

from dataclasses import dataclass
from typing import Dict, List, Tuple, Optional

# ============================================================================
# ECG - RANGOS CLÍNICOS POR PATOLOGÍA
# ============================================================================
@dataclass
class ECGClinicalRange:
    """Rangos clínicos esperados para ECG según patología"""
    condition: str
    hr_min: float           # BPM mínimo
    hr_max: float           # BPM máximo
    rr_min_ms: float        # Intervalo RR mínimo (ms)
    rr_max_ms: float        # Intervalo RR máximo (ms)
    r_amplitude_min_mV: float   # Amplitud R mínima (mV)
    r_amplitude_max_mV: float   # Amplitud R máxima (mV)
    st_deviation_min_mV: float  # Desviación ST mínima (mV)
    st_deviation_max_mV: float  # Desviación ST máxima (mV)
    qrs_duration_min_ms: float  # Duración QRS mínima (ms)
    qrs_duration_max_ms: float  # Duración QRS máxima (ms)
    description: str
    clinical_notes: str

ECG_CLINICAL_RANGES: Dict[str, ECGClinicalRange] = {
    # -------------------------------------------------------------------------
    # NORMAL - Goldberger 2017, Task Force ESC 1996
    # -------------------------------------------------------------------------
    "NORMAL": ECGClinicalRange(
        condition="Normal",
        hr_min=60.0, hr_max=100.0,
        rr_min_ms=600.0, rr_max_ms=1000.0,
        r_amplitude_min_mV=0.5, r_amplitude_max_mV=1.5,
        st_deviation_min_mV=-0.05, st_deviation_max_mV=0.05,
        qrs_duration_min_ms=60.0, qrs_duration_max_ms=100.0,
        description="Ritmo sinusal normal",
        clinical_notes="HR 60-100 BPM, QRS 60-100ms, ST isoeléctrico"
    ),
    
    # -------------------------------------------------------------------------
    # TAQUICARDIA SINUSAL - Surawicz 2008
    # -------------------------------------------------------------------------
    "TACHY": ECGClinicalRange(
        condition="Taquicardia Sinusal",
        hr_min=100.0, hr_max=180.0,
        rr_min_ms=333.0, rr_max_ms=600.0,
        r_amplitude_min_mV=0.5, r_amplitude_max_mV=1.5,
        st_deviation_min_mV=-0.1, st_deviation_max_mV=0.05,
        qrs_duration_min_ms=60.0, qrs_duration_max_ms=100.0,
        description="Taquicardia sinusal >100 BPM",
        clinical_notes="Puede tener ligera depresión ST por demanda"
    ),
    
    # -------------------------------------------------------------------------
    # BRADICARDIA SINUSAL - Surawicz 2008
    # -------------------------------------------------------------------------
    "BRADY": ECGClinicalRange(
        condition="Bradicardia Sinusal",
        hr_min=30.0, hr_max=59.0,
        rr_min_ms=1017.0, rr_max_ms=2000.0,
        r_amplitude_min_mV=0.5, r_amplitude_max_mV=1.5,
        st_deviation_min_mV=-0.05, st_deviation_max_mV=0.05,
        qrs_duration_min_ms=60.0, qrs_duration_max_ms=100.0,
        description="Bradicardia sinusal <60 BPM",
        clinical_notes="Común en atletas, puede ser patológica si <40 BPM"
    ),
    
    # -------------------------------------------------------------------------
    # FIBRILACIÓN AURICULAR - Surawicz 2008, Goldberger 2017
    # Modelo: 70-150 BPM base con variabilidad 15-25% → rango efectivo ~57-188 BPM
    # -------------------------------------------------------------------------
    "AFIB": ECGClinicalRange(
        condition="Fibrilación Auricular",
        hr_min=57.0, hr_max=188.0,  # Rango efectivo con variabilidad y 5% margen
        rr_min_ms=319.0, rr_max_ms=1053.0,  # RR correspondiente
        r_amplitude_min_mV=0.4, r_amplitude_max_mV=1.5,
        st_deviation_min_mV=-0.1, st_deviation_max_mV=0.1,
        qrs_duration_min_ms=60.0, qrs_duration_max_ms=120.0,
        description="Fibrilación auricular con respuesta ventricular variable",
        clinical_notes="Ausencia de ondas P, RR irregularmente irregular"
    ),
    
    # -------------------------------------------------------------------------
    # FIBRILACIÓN VENTRICULAR - Surawicz 2008
    # Ondas caóticas 4-10 Hz = 240-600 ciclos/min (pseudo-HR 300-400 típico)
    # -------------------------------------------------------------------------
    "VFIB": ECGClinicalRange(
        condition="Fibrilación Ventricular",
        hr_min=240.0, hr_max=600.0,  # Pseudo-HR basado en frecuencia ondas (4-10 Hz)
        rr_min_ms=100.0, rr_max_ms=250.0,  # Pseudo-RR correspondiente
        r_amplitude_min_mV=0.1, r_amplitude_max_mV=1.0,  # Amplitud ondas caóticas
        st_deviation_min_mV=-1.0, st_deviation_max_mV=1.0,  # No aplica
        qrs_duration_min_ms=0.0, qrs_duration_max_ms=0.0,  # No hay QRS definido
        description="Fibrilación ventricular - ondas caóticas 4-10 Hz",
        clinical_notes="Sin PQRST, ondas irregulares, EMERGENCIA - desfibrilar"
    ),
    
    # -------------------------------------------------------------------------
    # CONTRACCIÓN VENTRICULAR PREMATURA (PVC) - Goldberger 2017
    # -------------------------------------------------------------------------
    "PVC": ECGClinicalRange(
        condition="Contracción Ventricular Prematura",
        hr_min=50.0, hr_max=120.0,
        rr_min_ms=500.0, rr_max_ms=1200.0,
        r_amplitude_min_mV=0.5, r_amplitude_max_mV=2.5,  # PVC puede ser más amplio
        st_deviation_min_mV=-0.3, st_deviation_max_mV=0.3,
        qrs_duration_min_ms=120.0, qrs_duration_max_ms=200.0,  # QRS ancho en PVC
        description="Extrasístoles ventriculares",
        clinical_notes="QRS ancho (>120ms), pausa compensatoria, morfología aberrante"
    ),
    
    # -------------------------------------------------------------------------
    # ELEVACIÓN ST (STEMI) - Surawicz 2008, Thygesen 2018
    # Modelo: 60-100 BPM, ST +0.30 a +0.55 mV (elevación muy visible, didáctico)
    # Factor de aplicación 3.0x → ST efectivo ~0.9-1.65 mV en señal
    # -------------------------------------------------------------------------
    "STE": ECGClinicalRange(
        condition="Elevación ST (STEMI)",
        hr_min=60.0, hr_max=100.0,  # HR normal en STEMI
        rr_min_ms=600.0, rr_max_ms=1000.0,  # Correspondiente a 60-100 BPM
        r_amplitude_min_mV=0.5, r_amplitude_max_mV=1.8,
        st_deviation_min_mV=0.25, st_deviation_max_mV=0.60,  # Elevación base (antes de factor)
        qrs_duration_min_ms=60.0, qrs_duration_max_ms=120.0,
        description="Infarto agudo con elevación ST (STEMI) - meseta visible",
        clinical_notes="ST elevado en meseta, T hiperaguda, patrón 'lomo de delfín'"
    ),
    
    # -------------------------------------------------------------------------
    # DEPRESIÓN ST (Isquemia) - Surawicz 2008
    # Modelo: 60-100 BPM, ST -0.25 a -0.50 mV (depresión visible, downsloping)
    # -------------------------------------------------------------------------
    "STD": ECGClinicalRange(
        condition="Depresión ST (Isquemia)",
        hr_min=60.0, hr_max=100.0,
        rr_min_ms=600.0, rr_max_ms=1000.0,
        r_amplitude_min_mV=0.5, r_amplitude_max_mV=1.5,
        st_deviation_min_mV=-0.55, st_deviation_max_mV=-0.20,  # Depresión visible
        qrs_duration_min_ms=60.0, qrs_duration_max_ms=100.0,
        description="Isquemia subendocárdica - patrón downsloping",
        clinical_notes="ST descendente desde punto J, T aplanada, signo de isquemia"
    ),
}


# ============================================================================
# EMG - RANGOS CLÍNICOS POR CONDICIÓN
# ============================================================================
@dataclass
class EMGClinicalRange:
    """Rangos clínicos esperados para EMG según condición"""
    condition: str
    rms_min_mV: float           # RMS mínimo (mV)
    rms_max_mV: float           # RMS máximo (mV)
    motor_units_min: int        # MUs activas mínimo
    motor_units_max: int        # MUs activas máximo
    firing_rate_min_Hz: float   # Frecuencia de disparo mínima (Hz)
    firing_rate_max_Hz: float   # Frecuencia de disparo máxima (Hz)
    contraction_min_pct: float  # % contracción mínimo
    contraction_max_pct: float  # % contracción máximo
    description: str
    clinical_notes: str

EMG_CLINICAL_RANGES: Dict[str, EMGClinicalRange] = {
    # -------------------------------------------------------------------------
    # REPOSO (0-5% MVC) - De Luca 1997 [7]
    # Modelo: excitación 0-5%, muy pocas MUs activas
    # Rango pico: ±0.02 mV, RMS: <0.02 mV
    # -------------------------------------------------------------------------
    "REST": EMGClinicalRange(
        condition="Reposo",
        rms_min_mV=0.0, rms_max_mV=0.02,  # v1.1: corregido según RANGOS_CLINICOS.md
        motor_units_min=0, motor_units_max=20,
        firing_rate_min_Hz=0.0, firing_rate_max_Hz=8.0,
        contraction_min_pct=0.0, contraction_max_pct=6.0,
        description="Reposo muscular (0-5% MVC)",
        clinical_notes="Tono muscular basal, mínima actividad voluntaria"
    ),
    
    # -------------------------------------------------------------------------
    # CONTRACCIÓN BAJA (5-20% MVC) - Fuglevand 1993 [6], De Luca 2010 [8]
    # Modelo: excitación 5-20%, Rango pico: ±0.15 mV, RMS: 0.02-0.10 mV
    # -------------------------------------------------------------------------
    "LOW": EMGClinicalRange(
        condition="Contracción Baja",
        rms_min_mV=0.02, rms_max_mV=0.10,  # v1.1: corregido según RANGOS_CLINICOS.md
        motor_units_min=15, motor_units_max=60,
        firing_rate_min_Hz=6.0, firing_rate_max_Hz=15.0,
        contraction_min_pct=5.0, contraction_max_pct=22.0,
        description="Contracción baja (5-20% MVC)",
        clinical_notes="Reclutamiento ordenado según principio de Henneman"
    ),
    
    # -------------------------------------------------------------------------
    # CONTRACCIÓN MODERADA (20-50% MVC) - Fuglevand 1993 [6]
    # Modelo: excitación 20-50%, Rango pico: ±0.50 mV, RMS: 0.10-0.40 mV
    # -------------------------------------------------------------------------
    "MODERATE": EMGClinicalRange(
        condition="Contracción Moderada",
        rms_min_mV=0.10, rms_max_mV=0.40,  # v1.1: corregido según RANGOS_CLINICOS.md
        motor_units_min=50, motor_units_max=100,
        firing_rate_min_Hz=12.0, firing_rate_max_Hz=25.0,
        contraction_min_pct=18.0, contraction_max_pct=55.0,
        description="Contracción moderada (20-50% MVC)",
        clinical_notes="Patrón de interferencia parcial"
    ),
    
    # -------------------------------------------------------------------------
    # CONTRACCIÓN ALTA (50-100% MVC) - De Luca 1997 [7], Fuglevand 1993 [6]
    # Modelo: excitación 50-100%, Rango pico: ±2.00 mV, RMS: 0.50-1.50 mV
    # -------------------------------------------------------------------------
    "HIGH": EMGClinicalRange(
        condition="Contracción Alta",
        rms_min_mV=0.50, rms_max_mV=1.50,  # v1.1: corregido según RANGOS_CLINICOS.md
        motor_units_min=85, motor_units_max=100,
        firing_rate_min_Hz=20.0, firing_rate_max_Hz=50.0,
        contraction_min_pct=60.0, contraction_max_pct=100.0,
        description="Contracción alta/máxima (60-100% MVC)",
        clinical_notes="Patrón de interferencia completo, FR hasta 50Hz"
    ),
    
    # -------------------------------------------------------------------------
    # TEMBLOR PARKINSONIANO - Deuschl 1998, 2001
    # Oscila entre reposo y contracción a 4-6 Hz
    # Rango pico: ±0.80 mV, RMS: 0.10-0.50 mV
    # -------------------------------------------------------------------------
    "TREMOR": EMGClinicalRange(
        condition="Temblor",
        rms_min_mV=0.10, rms_max_mV=0.50,  # v1.1: corregido según RANGOS_CLINICOS.md
        motor_units_min=0, motor_units_max=100,      # Oscila entre 0 y máximo
        firing_rate_min_Hz=4.0, firing_rate_max_Hz=18.0,  # Variable por oscilación
        contraction_min_pct=0.0, contraction_max_pct=40.0,  # Oscila 0-40%
        description="Temblor parkinsoniano 4-6 Hz",
        clinical_notes="Modulación rítmica 4-6 Hz, alternante agonista-antagonista"
    ),
    
    # -------------------------------------------------------------------------
    # MIOPATÍA - Kimura 2013, Merletti 2004
    # MUAPs pequeños (<500µV), polifásicos, reclutamiento precoz
    # Rango pico: ±0.25 mV, RMS: 0.05-0.15 mV
    # -------------------------------------------------------------------------
    "MYOPATHY": EMGClinicalRange(
        condition="Miopatía",
        rms_min_mV=0.05, rms_max_mV=0.15,  # v1.1: corregido según RANGOS_CLINICOS.md
        motor_units_min=60, motor_units_max=100,    # Reclutamiento agresivo
        firing_rate_min_Hz=15.0, firing_rate_max_Hz=35.0,
        contraction_min_pct=30.0, contraction_max_pct=60.0,
        description="Patrón miopático",
        clinical_notes="MUAPs pequeños (<500µV), polifásicos, reclutamiento precoz"
    ),
    
    # -------------------------------------------------------------------------
    # NEUROPATÍA - Kimura 2013, Merletti 2004
    # MUAPs gigantes, reinervación colateral
    # Rango pico: ±2.50 mV (sEMG, atenuado), RMS: 0.30-1.50 mV
    # -------------------------------------------------------------------------
    "NEUROPATHY": EMGClinicalRange(
        condition="Neuropatía",
        rms_min_mV=0.30, rms_max_mV=1.50,  # v1.1: corregido según RANGOS_CLINICOS.md
        motor_units_min=10, motor_units_max=40,     # Pocas MUs sobreviven (70% pérdida)
        firing_rate_min_Hz=8.0, firing_rate_max_Hz=25.0,
        contraction_min_pct=25.0, contraction_max_pct=55.0,
        description="Patrón neuropático",
        clinical_notes="MUAPs gigantes (sEMG ±2.5mV), reinervación colateral, 70% pérdida MUs"
    ),
    
    # -------------------------------------------------------------------------
    # FASCICULACIÓN - Mills 2010, Kimura 2013
    # Eventos aislados espontáneos, base silenciosa con picos transitorios
    # Rango pico: ±1.00 mV, RMS: 0.10-0.50 mV
    # -------------------------------------------------------------------------
    "FASCICULATION": EMGClinicalRange(
        condition="Fasciculación",
        rms_min_mV=0.10, rms_max_mV=0.50,  # v1.1: corregido según RANGOS_CLINICOS.md
        motor_units_min=0, motor_units_max=5,       # 0-5 MUs instantáneas
        firing_rate_min_Hz=0.0, firing_rate_max_Hz=40.0,  # 0 basal, bursts 25-40Hz
        contraction_min_pct=0.0, contraction_max_pct=10.0,
        description="Fasciculaciones espontáneas",
        clinical_notes="Eventos aislados 1-3/s, bursts cortos 50-150ms, base silenciosa"
    ),
}


# ============================================================================
# PPG - RANGOS CLÍNICOS POR CONDICIÓN
# ============================================================================
@dataclass
class PPGClinicalRange:
    """Rangos clínicos esperados para PPG según condición"""
    condition: str
    hr_min: float               # BPM mínimo
    hr_max: float               # BPM máximo
    rr_min_ms: float            # Intervalo RR mínimo (ms)
    rr_max_ms: float            # Intervalo RR máximo (ms)
    pi_min_pct: float           # Índice de perfusión mínimo (%)
    pi_max_pct: float           # Índice de perfusión máximo (%)
    description: str
    clinical_notes: str

PPG_CLINICAL_RANGES: Dict[str, PPGClinicalRange] = {
    # -------------------------------------------------------------------------
    # NORMAL - Allen 2007
    # -------------------------------------------------------------------------
    "NORMAL": PPGClinicalRange(
        condition="Normal",
        hr_min=60.0, hr_max=100.0,
        rr_min_ms=600.0, rr_max_ms=1000.0,
        pi_min_pct=2.0, pi_max_pct=5.0,
        description="PPG normal en reposo",
        clinical_notes="Muesca dicrótica visible, PI 2-5%"
    ),
    
    # -------------------------------------------------------------------------
    # ARRITMIA - Allen 2007
    # -------------------------------------------------------------------------
    "ARRHYTHMIA": PPGClinicalRange(
        condition="Arritmia",
        hr_min=60.0, hr_max=180.0,
        rr_min_ms=333.0, rr_max_ms=1500.0,  # Muy variable
        pi_min_pct=1.0, pi_max_pct=5.0,
        description="Arritmia con RR irregular",
        clinical_notes="Variabilidad RR >20%, amplitud variable"
    ),
    
    # -------------------------------------------------------------------------
    # PERFUSIÓN DÉBIL - Lima & Bakker 2005
    # -------------------------------------------------------------------------
    "WEAK_PERFUSION": PPGClinicalRange(
        condition="Perfusión Débil",
        hr_min=90.0, hr_max=140.0,  # Taquicardia compensatoria
        rr_min_ms=428.0, rr_max_ms=667.0,
        pi_min_pct=0.1, pi_max_pct=0.5,
        description="Hipoperfusión periférica",
        clinical_notes="PI <0.5%, señal débil, posible shock"
    ),
    
    # -------------------------------------------------------------------------
    # PERFUSIÓN FUERTE - Lima & Bakker 2005
    # -------------------------------------------------------------------------
    "STRONG_PERFUSION": PPGClinicalRange(
        condition="Perfusión Fuerte",
        hr_min=60.0, hr_max=90.0,
        rr_min_ms=667.0, rr_max_ms=1000.0,
        pi_min_pct=5.0, pi_max_pct=20.0,
        description="Vasodilatación/hiperperfusión",
        clinical_notes="PI >5%, fiebre, ejercicio, vasodilatadores"
    ),
    
    # -------------------------------------------------------------------------
    # VASOCONSTRICCIÓN - Allen 2007, Reisner 2008
    # Vasoconstricción marcada: PI muy bajo, amplitud reducida
    # -------------------------------------------------------------------------
    "VASOCONSTRICTION": PPGClinicalRange(
        condition="Vasoconstricción",
        hr_min=60.0, hr_max=100.0,
        rr_min_ms=600.0, rr_max_ms=1000.0,
        pi_min_pct=0.2, pi_max_pct=0.8,      # PI muy bajo (0.3-0.5% típico)
        description="Vasoconstricción periférica marcada",
        clinical_notes="PI <1%, amplitud reducida, muesca atenuada"
    ),
}


# ============================================================================
# FUNCIONES DE VALIDACIÓN
# ============================================================================

# Alias para nombres de condiciones (mapeo de nombres largos a cortos)
ECG_CONDITION_ALIASES = {
    "TACHYCARDIA": "TACHY",
    "BRADYCARDIA": "BRADY",
    "ATRIAL_FIBRILLATION": "AFIB",
    "VENTRICULAR_FIBRILLATION": "VFIB",
    "PREMATURE_VENTRICULAR": "PVC",
    "ST_ELEVATION": "STE",
    "ST_DEPRESSION": "STD",
}

EMG_CONDITION_ALIASES = {
    "LOW_CONTRACTION": "LOW",
    "MODERATE_CONTRACTION": "MODERATE",
    "HIGH_CONTRACTION": "HIGH",
}

PPG_CONDITION_ALIASES = {
    "WEAK_PERFUSION": "WEAK",
    "STRONG_PERFUSION": "STRONG",
}


def normalize_ecg_condition(condition: str) -> str:
    """Normaliza el nombre de condición ECG."""
    condition = condition.upper()
    return ECG_CONDITION_ALIASES.get(condition, condition)


def normalize_emg_condition(condition: str) -> str:
    """Normaliza el nombre de condición EMG."""
    condition = condition.upper()
    return EMG_CONDITION_ALIASES.get(condition, condition)


def normalize_ppg_condition(condition: str) -> str:
    """Normaliza el nombre de condición PPG."""
    condition = condition.upper()
    return PPG_CONDITION_ALIASES.get(condition, condition)


def validate_ecg_sample(condition: str, hr: float, rr_ms: float, 
                        r_amp_mV: float, st_dev_mV: float,
                        qrs_ms: float = None) -> Dict[str, Tuple[bool, str]]:
    """
    Valida una muestra de ECG contra rangos clínicos.
    
    Returns:
        Dict con cada parámetro y tupla (está_en_rango, mensaje)
    """
    condition = normalize_ecg_condition(condition)
    if condition not in ECG_CLINICAL_RANGES:
        return {"error": (False, f"Condición '{condition}' no reconocida")}
    
    ref = ECG_CLINICAL_RANGES[condition]
    results = {}
    
    # HR
    hr_ok = ref.hr_min <= hr <= ref.hr_max
    results["hr"] = (hr_ok, f"{hr:.1f} BPM {'✓' if hr_ok else '✗'} [{ref.hr_min:.0f}-{ref.hr_max:.0f}]")
    
    # RR
    rr_ok = ref.rr_min_ms <= rr_ms <= ref.rr_max_ms
    results["rr"] = (rr_ok, f"{rr_ms:.0f} ms {'✓' if rr_ok else '✗'} [{ref.rr_min_ms:.0f}-{ref.rr_max_ms:.0f}]")
    
    # R amplitude
    r_ok = ref.r_amplitude_min_mV <= r_amp_mV <= ref.r_amplitude_max_mV
    results["r_amplitude"] = (r_ok, f"{r_amp_mV:.2f} mV {'✓' if r_ok else '✗'} [{ref.r_amplitude_min_mV:.2f}-{ref.r_amplitude_max_mV:.2f}]")
    
    # ST deviation
    st_ok = ref.st_deviation_min_mV <= st_dev_mV <= ref.st_deviation_max_mV
    results["st_deviation"] = (st_ok, f"{st_dev_mV:.3f} mV {'✓' if st_ok else '✗'} [{ref.st_deviation_min_mV:.2f}-{ref.st_deviation_max_mV:.2f}]")
    
    # QRS duration (si se proporciona)
    if qrs_ms is not None and ref.qrs_duration_max_ms > 0:
        qrs_ok = ref.qrs_duration_min_ms <= qrs_ms <= ref.qrs_duration_max_ms
        results["qrs_duration"] = (qrs_ok, f"{qrs_ms:.0f} ms {'✓' if qrs_ok else '✗'} [{ref.qrs_duration_min_ms:.0f}-{ref.qrs_duration_max_ms:.0f}]")
    
    return results


def validate_emg_sample(condition: str, rms_mV: float, motor_units: int,
                        firing_rate_Hz: float, contraction_pct: float) -> Dict[str, Tuple[bool, str]]:
    """
    Valida una muestra de EMG contra rangos clínicos.
    """
    condition = normalize_emg_condition(condition)
    if condition not in EMG_CLINICAL_RANGES:
        return {"error": (False, f"Condición '{condition}' no reconocida")}
    
    ref = EMG_CLINICAL_RANGES[condition]
    results = {}
    
    # RMS
    rms_ok = ref.rms_min_mV <= rms_mV <= ref.rms_max_mV
    results["rms"] = (rms_ok, f"{rms_mV:.3f} mV {'✓' if rms_ok else '✗'} [{ref.rms_min_mV:.3f}-{ref.rms_max_mV:.3f}]")
    
    # Motor Units
    mu_ok = ref.motor_units_min <= motor_units <= ref.motor_units_max
    results["motor_units"] = (mu_ok, f"{motor_units} MUs {'✓' if mu_ok else '✗'} [{ref.motor_units_min}-{ref.motor_units_max}]")
    
    # Firing Rate
    fr_ok = ref.firing_rate_min_Hz <= firing_rate_Hz <= ref.firing_rate_max_Hz
    results["firing_rate"] = (fr_ok, f"{firing_rate_Hz:.1f} Hz {'✓' if fr_ok else '✗'} [{ref.firing_rate_min_Hz:.1f}-{ref.firing_rate_max_Hz:.1f}]")
    
    # Contraction
    cont_ok = ref.contraction_min_pct <= contraction_pct <= ref.contraction_max_pct
    results["contraction"] = (cont_ok, f"{contraction_pct:.1f}% {'✓' if cont_ok else '✗'} [{ref.contraction_min_pct:.1f}-{ref.contraction_max_pct:.1f}]")
    
    return results


def validate_ppg_sample(condition: str, hr: float, rr_ms: float,
                        pi_pct: float) -> Dict[str, Tuple[bool, str]]:
    """
    Valida una muestra de PPG contra rangos clínicos.
    """
    condition = normalize_ppg_condition(condition)
    if condition not in PPG_CLINICAL_RANGES:
        return {"error": (False, f"Condición '{condition}' no reconocida")}
    
    ref = PPG_CLINICAL_RANGES[condition]
    results = {}
    
    # HR
    hr_ok = ref.hr_min <= hr <= ref.hr_max
    results["hr"] = (hr_ok, f"{hr:.1f} BPM {'✓' if hr_ok else '✗'} [{ref.hr_min:.0f}-{ref.hr_max:.0f}]")
    
    # RR
    rr_ok = ref.rr_min_ms <= rr_ms <= ref.rr_max_ms
    results["rr"] = (rr_ok, f"{rr_ms:.0f} ms {'✓' if rr_ok else '✗'} [{ref.rr_min_ms:.0f}-{ref.rr_max_ms:.0f}]")
    
    # PI
    pi_ok = ref.pi_min_pct <= pi_pct <= ref.pi_max_pct
    results["pi"] = (pi_ok, f"{pi_pct:.2f}% {'✓' if pi_ok else '✗'} [{ref.pi_min_pct:.2f}-{ref.pi_max_pct:.2f}]")
    
    return results


# ============================================================================
# MAPEO DE NOMBRES CORTOS A CLAVES
# ============================================================================
ECG_CONDITION_MAP = {
    "NORMAL": "NORMAL",
    "TACHYCARDIA": "TACHY", "TACHY": "TACHY",
    "BRADYCARDIA": "BRADY", "BRADY": "BRADY",
    "ATRIAL_FIBRILLATION": "AFIB", "AFIB": "AFIB",
    "VENTRICULAR_FIBRILLATION": "VFIB", "VFIB": "VFIB",
    "PREMATURE_VENTRICULAR": "PVC", "PVC": "PVC",
    "ST_ELEVATION": "STE", "STE": "STE",
    "ST_DEPRESSION": "STD", "STD": "STD",
}

EMG_CONDITION_MAP = {
    "REST": "REST",
    "LOW_CONTRACTION": "LOW", "LOW": "LOW",
    "MODERATE_CONTRACTION": "MODERATE", "MODERATE": "MODERATE",
    "HIGH_CONTRACTION": "HIGH", "HIGH": "HIGH",
    "TREMOR": "TREMOR",
    "MYOPATHY": "MYOPATHY",
    "NEUROPATHY": "NEUROPATHY",
    "FASCICULATION": "FASCICULATION",
}

PPG_CONDITION_MAP = {
    "NORMAL": "NORMAL",
    "ARRHYTHMIA": "ARRHYTHMIA",
    "WEAK_PERFUSION": "WEAK_PERFUSION",
    "STRONG_PERFUSION": "STRONG_PERFUSION",
    "VASOCONSTRICTION": "VASOCONSTRICTION",
}


def print_all_ranges():
    """Imprime todas las tablas de rangos clínicos."""
    print("\n" + "="*80)
    print("TABLAS DE RANGOS CLÍNICOS - BIOSIMULATOR PRO")
    print("="*80)
    
    print("\n" + "-"*80)
    print("ECG - ELECTROCARDIOGRAMA")
    print("-"*80)
    print(f"{'Condición':<25} {'HR (BPM)':<15} {'RR (ms)':<15} {'R (mV)':<12} {'ST (mV)':<15} {'QRS (ms)':<12}")
    print("-"*80)
    for key, r in ECG_CLINICAL_RANGES.items():
        print(f"{r.condition:<25} {r.hr_min:.0f}-{r.hr_max:.0f}{'':>5} {r.rr_min_ms:.0f}-{r.rr_max_ms:.0f}{'':>3} "
              f"{r.r_amplitude_min_mV:.1f}-{r.r_amplitude_max_mV:.1f}{'':>4} "
              f"{r.st_deviation_min_mV:+.2f} a {r.st_deviation_max_mV:+.2f} "
              f"{r.qrs_duration_min_ms:.0f}-{r.qrs_duration_max_ms:.0f}")
    
    print("\n" + "-"*80)
    print("EMG - ELECTROMIOGRAFÍA")
    print("-"*80)
    print(f"{'Condición':<20} {'RMS (mV)':<15} {'MUs':<12} {'FR (Hz)':<15} {'Contracción':<15}")
    print("-"*80)
    for key, r in EMG_CLINICAL_RANGES.items():
        print(f"{r.condition:<20} {r.rms_min_mV:.3f}-{r.rms_max_mV:.3f}{'':>4} "
              f"{r.motor_units_min:>3}-{r.motor_units_max:<3}{'':>4} "
              f"{r.firing_rate_min_Hz:.1f}-{r.firing_rate_max_Hz:.1f}{'':>5} "
              f"{r.contraction_min_pct:.0f}-{r.contraction_max_pct:.0f}%")
    
    print("\n" + "-"*80)
    print("PPG - FOTOPLETISMOGRAFÍA")
    print("-"*80)
    print(f"{'Condición':<25} {'HR (BPM)':<15} {'RR (ms)':<15} {'PI (%)':<12}")
    print("-"*80)
    for key, r in PPG_CLINICAL_RANGES.items():
        print(f"{r.condition:<25} {r.hr_min:.0f}-{r.hr_max:.0f}{'':>5} "
              f"{r.rr_min_ms:.0f}-{r.rr_max_ms:.0f}{'':>5} "
              f"{r.pi_min_pct:.1f}-{r.pi_max_pct:.1f}")
    
    print("\n" + "="*80)


if __name__ == "__main__":
    print_all_ranges()
