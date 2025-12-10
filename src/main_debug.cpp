/**
 * @file main_debug.cpp
 * @brief Modo Debug interactivo para verificacion de senales biologicas
 * @version 2.0.0
 * @date Diciembre 2024
 * 
 * Firmware ESP32 - BioSimulator Pro
 * 
 * Menu interactivo que permite seleccionar:
 * - Tipo de senal (ECG, EMG, PPG)
 * - Patologia especifica dentro de cada senal
 * - Duracion del ploteo
 * - Volver al menu en cualquier momento
 * 
 * Hardware: ESP32-WROOM-32 (NodeMCU v1.1)
 * Salida DAC: GPIO25 (0-3.3V)
 * Comunicacion: UART @ 115200 baud con Nextion
 */

#include <Arduino.h>
#include "config.h"
#include "data/signal_types.h"
#include "models/ecg_model.h"
#include "models/emg_model.h"
#include "models/ppg_model.h"

// ============================================================================
// CONFIGURACI├ôN
// ============================================================================
#define DEBUG_SAMPLE_RATE_HZ    100
#define DEBUG_SAMPLE_INTERVAL   (1000 / DEBUG_SAMPLE_RATE_HZ)
#define PLOT_DURATION_MS        10000   // 10 segundos por defecto

// ============================================================================
// CONFIGURACI├ôN AUTO-START (Para Serial Plotter)
// ============================================================================
// Cambiar estos valores para seleccionar se├▒al y patolog├¡a
#define AUTO_START              1       // 1 = inicio autom├ítico, 0 = men├║ interactivo

// Tipo de se├▒al: 0=ECG, 1=EMG, 2=PPG
#define AUTO_SIGNAL_TYPE        2

// Condiciones ECG (solo si AUTO_SIGNAL_TYPE==0):
// 0=NORMAL, 1=TACHYCARDIA, 2=BRADYCARDIA, 3=ATRIAL_FIBRILLATION
// 4=VENTRICULAR_FIBRILLATION, 5=PREMATURE_VENTRICULAR
// 6=ST_ELEVATION, 7=ST_DEPRESSION
#define AUTO_ECG_CONDITION      0

// Condiciones EMG (solo si AUTO_SIGNAL_TYPE==1):
// 0=REST, 1=LOW_CONTRACTION, 2=MODERATE_CONTRACTION, 3=HIGH_CONTRACTION
// 4=TREMOR, 5=MYOPATHY, 6=NEUROPATHY, 7=FASCICULATION
#define AUTO_EMG_CONDITION      0

// Condiciones PPG (solo si AUTO_SIGNAL_TYPE==2):
// 0=NORMAL, 1=ARRHYTHMIA, 2=WEAK_PERFUSION, 3=STRONG_PERFUSION
// 4=VASOCONSTRICTION, 5=LOW_SPO2
#define AUTO_PPG_CONDITION      4

#define AUTO_CONTINUOUS         1       // 1 = continuo, 0 = usar PLOT_DURATION_MS

// ============================================================================
// ESTADOS DEL MEN├Ü
// ============================================================================
enum class MenuState {
    MAIN_MENU,
    ECG_MENU,
    EMG_MENU,
    PPG_MENU,
    PLOTTING,
    WAITING_DURATION
};

// ============================================================================
// INSTANCIAS GLOBALES
// ============================================================================
ECGModel ecgModel;
EMGModel emgModel;
PPGModel ppgModel;

MenuState menuState = MenuState::MAIN_MENU;
SignalType currentSignalType = SignalType::ECG;
ECGCondition currentECGCondition = ECGCondition::NORMAL;
EMGCondition currentEMGCondition = EMGCondition::REST;
PPGCondition currentPPGCondition = PPGCondition::NORMAL;

unsigned long plotStartTime = 0;
unsigned long plotDuration = PLOT_DURATION_MS;
bool continuousMode = false;

// Variables para tracking
float minVal = 999.0f;
float maxVal = -999.0f;

// ============================================================================
// UTILIDADES
// ============================================================================
void clearSerialBuffer() {
    while (Serial.available()) Serial.read();
}

char waitForInput() {
    while (!Serial.available()) {
        delay(10);
    }
    char c = Serial.read();
    clearSerialBuffer();
    return c;
}

// ============================================================================
// NOMBRES
// ============================================================================
const char* getECGConditionName(ECGCondition cond) {
    switch (cond) {
        case ECGCondition::NORMAL:                  return "Normal (60-100 BPM)";
        case ECGCondition::TACHYCARDIA:             return "Taquicardia (>100 BPM)";
        case ECGCondition::BRADYCARDIA:             return "Bradicardia (<60 BPM)";
        case ECGCondition::ATRIAL_FIBRILLATION:     return "Fibrilacion Auricular";
        case ECGCondition::VENTRICULAR_FIBRILLATION: return "Fibrilacion Ventricular";
        case ECGCondition::PREMATURE_VENTRICULAR:   return "Contraccion Ventricular Prematura";
        case ECGCondition::ST_ELEVATION:            return "Elevacion ST (Infarto)";
        case ECGCondition::ST_DEPRESSION:           return "Depresion ST (Isquemia)";
        default:                                    return "Desconocido";
    }
}

const char* getECGConditionShortName(ECGCondition cond) {
    switch (cond) {
        case ECGCondition::NORMAL:                  return "NORMAL";
        case ECGCondition::TACHYCARDIA:             return "TACHY";
        case ECGCondition::BRADYCARDIA:             return "BRADY";
        case ECGCondition::ATRIAL_FIBRILLATION:     return "AFIB";
        case ECGCondition::VENTRICULAR_FIBRILLATION: return "VFIB";
        case ECGCondition::PREMATURE_VENTRICULAR:   return "PVC";
        case ECGCondition::ST_ELEVATION:            return "STE";
        case ECGCondition::ST_DEPRESSION:           return "STD";
        default:                                    return "UNK";
    }
}

// ============================================================================
// NOMBRES EMG - Basado en Fuglevand 1993, De Luca 1997, Kimura 2013
// ============================================================================
const char* getEMGConditionName(EMGCondition cond) {
    switch (cond) {
        case EMGCondition::REST:                 return "Reposo (0-10% MVC)";
        case EMGCondition::LOW_CONTRACTION:      return "Baja (10-30% MVC)";
        case EMGCondition::MODERATE_CONTRACTION: return "Moderada (30-60% MVC)";
        case EMGCondition::HIGH_CONTRACTION:     return "Alta (60-100% MVC)";
        case EMGCondition::TREMOR:               return "Temblor Parkinson (4-6 Hz)";
        case EMGCondition::MYOPATHY:             return "Miopatia (MUAPs pequenos)";
        case EMGCondition::NEUROPATHY:           return "Neuropatia (MUAPs gigantes)";
        case EMGCondition::FASCICULATION:        return "Fasciculacion";
        default:                                 return "Desconocido";
    }
}

// ============================================================================
// NOMBRES PPG - Basado en Allen 2007, Elgendi 2012
// ============================================================================
const char* getPPGConditionName(PPGCondition cond) {
    switch (cond) {
        case PPGCondition::NORMAL:            return "Normal (PI 2-5%)";
        case PPGCondition::ARRHYTHMIA:        return "Arritmia (RR irregular)";
        case PPGCondition::WEAK_PERFUSION:    return "Perfusion Debil (PI<0.5%)";
        case PPGCondition::STRONG_PERFUSION:  return "Perfusion Fuerte (PI>10%)";
        case PPGCondition::VASOCONSTRICTION:  return "Vasoconstriccion";
        case PPGCondition::LOW_SPO2:          return "SpO2 Bajo (<90%)";
        default:                              return "Desconocido";
    }
}

// ============================================================================
// RANGOS CLINICOS EMG (De Luca 1997, Fuglevand 1993)
// ============================================================================
struct EMGExpectedRange {
    float rmsMin;       // RMS minimo esperado (mV)
    float rmsMax;       // RMS maximo esperado (mV)
    float excitationMin; // Excitacion minima
    float excitationMax; // Excitacion maxima
    const char* description;
};

static const EMGExpectedRange EMG_CLINICAL_RANGES[] = {
    { 0.0f, 0.05f, 0.0f, 0.1f, "Reposo: RMS<50uV" },
    { 0.1f, 0.5f, 0.1f, 0.3f, "Leve: RMS 0.1-0.5mV" },
    { 0.5f, 1.5f, 0.3f, 0.6f, "Moderada: RMS 0.5-1.5mV" },
    { 1.0f, 3.0f, 0.6f, 0.9f, "Fuerte: RMS 1-3mV" },
    { 1.5f, 4.0f, 0.8f, 1.0f, "Maxima: RMS 1.5-4mV" },
    { 0.1f, 1.0f, 0.0f, 0.5f, "Temblor: modulacion 4-6Hz" },
    { 0.05f, 0.3f, 0.3f, 0.5f, "Miopatia: MUAPs 40% amp" },
    { 0.5f, 3.0f, 0.4f, 0.6f, "Neuropatia: MUAPs 250% amp" },
    { 0.0f, 0.5f, 0.0f, 0.1f, "Fasciculacion: espontaneos" },
    { 0.5f, 2.5f, 0.6f, 0.9f, "Fatiga: FR reducida" }
};

// ============================================================================
// RANGOS CLINICOS PPG (Allen 2007, Reisner 2008)
// ============================================================================
struct PPGExpectedRange {
    float hrMin;        // HR minimo esperado
    float hrMax;        // HR maximo esperado
    float piMin;        // PI minimo
    float piMax;        // PI maximo
    const char* description;
};

static const PPGExpectedRange PPG_CLINICAL_RANGES[] = {
    { 60.0f, 100.0f, 2.0f, 5.0f, "Normal: HR 60-100, PI 2-5%" },
    { 60.0f, 180.0f, 1.0f, 5.0f, "Arritmia: RR muy variable" },
    { 100.0f, 130.0f, 0.1f, 0.5f, "Perf.Debil: PI<0.5%, tachy" },
    { 60.0f, 80.0f, 5.0f, 20.0f, "Perf.Fuerte: PI>5%" },
    { 60.0f, 100.0f, 0.5f, 2.0f, "Vasoconstric: muesca prominente" },
    { 60.0f, 100.0f, 1.0f, 5.0f, "Artefacto: ruido alto" },
    { 90.0f, 120.0f, 0.5f, 2.0f, "SpO2 bajo: tachy refleja" }
};

// ============================================================================
// MEN├Ü PRINCIPAL
// ============================================================================
void showMainMenu() {
    Serial.println();
    Serial.println("ÔòöÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòù");
    Serial.println("Ôòæ       BIOSIMULATOR PRO - DEBUG MODE v2.0           Ôòæ");
    Serial.println("Ôòæ          Verificacion de Senales                   Ôòæ");
    Serial.println("ÔòÜÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòØ");
    Serial.println();
    Serial.println("  Seleccione tipo de senal:");
    Serial.println();
    Serial.println("    [1] ECG - Electrocardiograma (McSharry 2003)");
    Serial.println("    [2] EMG - Electromiograma (Fuglevand 1993)");
    Serial.println("    [3] PPG - Fotopletismografia (Allen 2007)");
    Serial.println();
    Serial.println("    [h] Ayuda");
    Serial.println("    [r] Reiniciar");
    Serial.println();
    Serial.print("  >> Ingrese opcion: ");
}

// ============================================================================
// MEN├Ü ECG
// ============================================================================
void showECGMenu() {
    Serial.println();
    Serial.println("ÔòöÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòù");
    Serial.println("Ôòæ              ECG - PATOLOGIAS                      Ôòæ");
    Serial.println("ÔòÜÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòØ");
    Serial.println();
    Serial.println("  Seleccione patologia:");
    Serial.println();
    Serial.println("    [0] Normal (60-100 BPM)");
    Serial.println("    [1] Taquicardia (>100 BPM)");
    Serial.println("    [2] Bradicardia (<60 BPM)");
    Serial.println("    [3] Fibrilacion Auricular (AFib)");
    Serial.println("    [4] Fibrilacion Ventricular (VFib)");
    Serial.println("    [5] Contraccion Ventricular Prematura (PVC)");
    Serial.println("    [6] Bloqueo de Rama (BBB)");
    Serial.println("    [7] Elevacion ST (Infarto STEMI)");
    Serial.println("    [8] Depresion ST (Isquemia)");
    Serial.println();
    Serial.println("    [b] Volver al menu principal");
    Serial.println();
    Serial.print("  >> Ingrese opcion: ");
}

// ============================================================================
// MEN├Ü EMG - Modelo Fuglevand 1993
// ============================================================================
void showEMGMenu() {
    Serial.println();
    Serial.println("ÔòöÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòù");
    Serial.println("Ôòæ     EMG - CONDICIONES (Fuglevand 1993)             Ôòæ");
    Serial.println("ÔòÜÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòØ");
    Serial.println();
    Serial.println("  Seleccione condicion:");
    Serial.println();
    Serial.println("    [0] Reposo (0% MVC, RMS<50uV)");
    Serial.println("    [1] Contraccion Leve (20% MVC)");
    Serial.println("    [2] Contraccion Moderada (50% MVC)");
    Serial.println("    [3] Contraccion Fuerte (80% MVC)");
    Serial.println("    [4] Contraccion Maxima (100% MVC)");
    Serial.println("    [5] Temblor Parkinsoniano (4-6 Hz)");
    Serial.println("    [6] Miopatia (MUAPs pequenos)");
    Serial.println("    [7] Neuropatia (MUAPs gigantes)");
    Serial.println("    [8] Fasciculacion (disparos espontaneos)");
    Serial.println("    [9] Fatiga (FR decreciente)");
    Serial.println();
    Serial.println("    [b] Volver al menu principal");
    Serial.println();
    Serial.print("  >> Ingrese opcion: ");
}

// ============================================================================
// MEN├Ü PPG - Modelo Allen 2007, Elgendi 2012
// ============================================================================
void showPPGMenu() {
    Serial.println();
    Serial.println("ÔòöÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòù");
    Serial.println("Ôòæ      PPG - CONDICIONES (Allen 2007)                Ôòæ");
    Serial.println("ÔòÜÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòØ");
    Serial.println();
    Serial.println("  Seleccione condicion:");
    Serial.println();
    Serial.println("    [0] Normal (PI 2-5%, HR 60-100)");
    Serial.println("    [1] Arritmia (RR muy irregular)");
    Serial.println("    [2] Perfusion Debil (PI<0.5%, shock)");
    Serial.println("    [3] Perfusion Fuerte (PI>10%, fiebre)");
    Serial.println("    [4] Vasoconstriccion (frio, estres)");
    Serial.println("    [5] Artefacto Movimiento");
    Serial.println("    [6] SpO2 Bajo (<90%)");
    Serial.println();
    Serial.println("    [b] Volver al menu principal");
    Serial.println();
    Serial.print("  >> Ingrese opcion: ");
}

// ============================================================================
// MEN├Ü DURACI├ôN
// ============================================================================
void showDurationMenu() {
    Serial.println();
    Serial.println("  Duracion del ploteo:");
    Serial.println();
    Serial.println("    [1] 5 segundos");
    Serial.println("    [2] 10 segundos");
    Serial.println("    [3] 30 segundos");
    Serial.println("    [4] 1 minuto");
    Serial.println("    [c] Continuo (presione 'q' para detener)");
    Serial.println();
    Serial.println("    [b] Volver");
    Serial.println();
    Serial.print("  >> Ingrese opcion: ");
}

// ============================================================================
// RANGOS CL├ìNICOS ESPERADOS (para validaci├│n)
// ============================================================================
struct ExpectedRange {
    float hrMin;
    float hrMax;
    const char* description;
};

static const ExpectedRange CLINICAL_RANGES[] = {
    { 60.0f, 100.0f, "Normal: 60-100 BPM" },
    { 100.0f, 180.0f, "Taquicardia: >100 BPM" },
    { 30.0f, 59.0f, "Bradicardia: <60 BPM" },
    { 60.0f, 180.0f, "AFib: 60-180 BPM (irregular)" },
    { 150.0f, 500.0f, "VFib: caotico (4-10 Hz)" },
    { 50.0f, 120.0f, "PVC: 50-120 BPM base" },
    { 40.0f, 100.0f, "BBB: 40-100 BPM" },
    { 50.0f, 110.0f, "ST Elevacion: 50-110 BPM" },
    { 50.0f, 150.0f, "ST Depresion: 50-150 BPM" }
};

// ============================================================================
// CONFIGURAR ECG
// ============================================================================
void setupECGCondition(ECGCondition cond) {
    currentSignalType = SignalType::ECG;
    currentECGCondition = cond;
    
    ECGParameters params;
    params.condition = cond;
    params.qrsAmplitude = 1.0f;
    params.noiseLevel = 0.02f;
    params.heartRate = 0;
    ecgModel.setParameters(params);
    
    minVal = 999.0f;
    maxVal = -999.0f;
    
    uint8_t idx = static_cast<uint8_t>(cond);
    if (idx < 9) {
        Serial.printf("  [Rango clinico: %s]\n", CLINICAL_RANGES[idx].description);
        Serial.printf("  [HR inicial: %.0f BPM]\n", ecgModel.getCurrentBPM());
    }
}

// ============================================================================
// CONFIGURAR EMG
// ============================================================================
void setupEMGCondition(EMGCondition cond) {
    currentSignalType = SignalType::EMG;
    currentEMGCondition = cond;
    
    EMGParameters params;
    params.condition = cond;
    params.amplitude = 1.0f;
    params.noiseLevel = 0.05f;
    params.excitationLevel = 0.0f;  // Usar default de la condici├│n
    emgModel.setParameters(params);
    
    minVal = 999.0f;
    maxVal = -999.0f;
    
    uint8_t idx = static_cast<uint8_t>(cond);
    if (idx < 10) {
        Serial.printf("  [Rango clinico: %s]\n", EMG_CLINICAL_RANGES[idx].description);
        Serial.printf("  [Excitacion: %.0f%%]\n", emgModel.getContractionLevel());
    }
}

// ============================================================================
// CONFIGURAR PPG
// ============================================================================
void setupPPGCondition(PPGCondition cond) {
    currentSignalType = SignalType::PPG;
    currentPPGCondition = cond;
    
    PPGParameters params;
    params.condition = cond;
    params.heartRate = 75.0f;  // Default, ser├í modificado por condici├│n
    params.perfusionIndex = 3.0f;
    params.dicroticNotch = 0.25f;
    params.noiseLevel = 0.02f;
    ppgModel.setParameters(params);
    
    minVal = 999.0f;
    maxVal = -999.0f;
    
    uint8_t idx = static_cast<uint8_t>(cond);
    if (idx < 7) {
        Serial.printf("  [Rango clinico: %s]\n", PPG_CLINICAL_RANGES[idx].description);
        Serial.printf("  [HR inicial: %.0f BPM]\n", ppgModel.getCurrentHeartRate());
    }
}

// ============================================================================
// INICIAR PLOTEO - Soporta ECG, EMG y PPG
// ============================================================================
void startPlotting() {
    Serial.println();
    Serial.println("ÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉ");
    
    if (currentSignalType == SignalType::ECG) {
        Serial.printf("  PLOTEANDO: ECG - %s\n", getECGConditionName(currentECGCondition));
        float hr = ecgModel.getCurrentBPM();
        uint8_t idx = static_cast<uint8_t>(currentECGCondition);
        if (idx < 9) {
            const ExpectedRange& range = CLINICAL_RANGES[idx];
            bool inRange = (hr >= range.hrMin && hr <= range.hrMax);
            Serial.printf("  HR inicial: %.0f BPM %s\n", hr, inRange ? "[OK]" : "[FUERA DE RANGO!]");
        }
    } else if (currentSignalType == SignalType::EMG) {
        Serial.printf("  PLOTEANDO: EMG - %s\n", getEMGConditionName(currentEMGCondition));
        uint8_t idx = static_cast<uint8_t>(currentEMGCondition);
        if (idx < 10) {
            Serial.printf("  Excitacion: %.0f%%, MUs activas: %d\n", 
                         emgModel.getContractionLevel(), emgModel.getActiveMotorUnits());
            Serial.printf("  Rango RMS esperado: %.2f-%.2f mV\n", 
                         EMG_CLINICAL_RANGES[idx].rmsMin, EMG_CLINICAL_RANGES[idx].rmsMax);
        }
    } else if (currentSignalType == SignalType::PPG) {
        Serial.printf("  PLOTEANDO: PPG - %s\n", getPPGConditionName(currentPPGCondition));
        float hr = ppgModel.getCurrentHeartRate();
        uint8_t idx = static_cast<uint8_t>(currentPPGCondition);
        if (idx < 7) {
            const PPGExpectedRange& range = PPG_CLINICAL_RANGES[idx];
            bool inRange = (hr >= range.hrMin && hr <= range.hrMax);
            Serial.printf("  HR inicial: %.0f BPM %s\n", hr, inRange ? "[OK]" : "[FUERA DE RANGO!]");
        }
    }
    
    if (continuousMode) {
        Serial.println("  Modo: CONTINUO (presione 'q' para detener)");
    } else {
        Serial.printf("  Duracion: %lu segundos\n", plotDuration / 1000);
    }
    Serial.println("ÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉ");
    Serial.println();
    delay(1000);
    
    plotStartTime = millis();
    menuState = MenuState::PLOTTING;
}

// ============================================================================
// MOSTRAR RESUMEN - Soporta ECG, EMG y PPG
// ============================================================================
void showPlotSummary() {
    Serial.println();
    Serial.println("ÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉ");
    Serial.println("  RESUMEN DEL PLOTEO");
    Serial.println("ÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉ");
    
    Serial.printf("  Rango: %.4f a %.4f (amplitud: %.4f)\n", minVal, maxVal, maxVal - minVal);
    
    if (currentSignalType == SignalType::ECG) {
        Serial.printf("  Senal: ECG - %s\n", getECGConditionName(currentECGCondition));
        float hr = ecgModel.getCurrentBPM();
        uint8_t idx = static_cast<uint8_t>(currentECGCondition);
        if (idx < 9) {
            const ExpectedRange& range = CLINICAL_RANGES[idx];
            bool inRange = (hr >= range.hrMin && hr <= range.hrMax);
            Serial.printf("  HR final: %.1f BPM %s\n", hr, inRange ? "[OK]" : "[FUERA DE RANGO!]");
        }
        Serial.printf("  RR: %.0f ms, Latidos: %lu\n", ecgModel.getCurrentRR_ms(), ecgModel.getBeatCount());
        
    } else if (currentSignalType == SignalType::EMG) {
        Serial.printf("  Senal: EMG - %s\n", getEMGConditionName(currentEMGCondition));
        float rms = emgModel.getRMSAmplitude();
        uint8_t idx = static_cast<uint8_t>(currentEMGCondition);
        if (idx < 10) {
            const EMGExpectedRange& range = EMG_CLINICAL_RANGES[idx];
            bool inRange = (rms >= range.rmsMin && rms <= range.rmsMax);
            Serial.printf("  RMS: %.3f mV %s\n", rms, inRange ? "[OK]" : "[VERIFICAR]");
            Serial.printf("  Rango esperado: %.2f-%.2f mV\n", range.rmsMin, range.rmsMax);
        }
        Serial.printf("  MUs activas: %d, FR media: %.1f Hz\n", 
                     emgModel.getActiveMotorUnits(), emgModel.getMeanFiringRate());
        
    } else if (currentSignalType == SignalType::PPG) {
        Serial.printf("  Senal: PPG - %s\n", getPPGConditionName(currentPPGCondition));
        float hr = ppgModel.getCurrentHeartRate();
        uint8_t idx = static_cast<uint8_t>(currentPPGCondition);
        if (idx < 7) {
            const PPGExpectedRange& range = PPG_CLINICAL_RANGES[idx];
            bool inRange = (hr >= range.hrMin && hr <= range.hrMax);
            Serial.printf("  HR final: %.1f BPM %s\n", hr, inRange ? "[OK]" : "[FUERA DE RANGO!]");
        }
        Serial.printf("  PI: %.1f%%, Latidos: %lu\n", ppgModel.getPerfusionIndex(), ppgModel.getBeatCount());
    }
    
    Serial.println("ÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉ");
    Serial.println();
    Serial.println("  Presione cualquier tecla para continuar...");
    
    waitForInput();
    
    // Volver al men├║ correspondiente
    if (currentSignalType == SignalType::ECG) {
        menuState = MenuState::ECG_MENU;
        showECGMenu();
    } else if (currentSignalType == SignalType::EMG) {
        menuState = MenuState::EMG_MENU;
        showEMGMenu();
    } else {
        menuState = MenuState::PPG_MENU;
        showPPGMenu();
    }
}

// ============================================================================
// PROCESAR MEN├Ü PRINCIPAL
// ============================================================================
void processMainMenu(char input) {
    switch (input) {
        case '1':
            menuState = MenuState::ECG_MENU;
            showECGMenu();
            break;
        case '2':
            menuState = MenuState::EMG_MENU;
            showEMGMenu();
            break;
        case '3':
            menuState = MenuState::PPG_MENU;
            showPPGMenu();
            break;
        case 'h':
        case 'H':
            Serial.println();
            Serial.println("  AYUDA:");
            Serial.println("  - Seleccione una senal y patologia");
            Serial.println("  - Los datos se enviaran en formato Serial Plotter");
            Serial.println("  - Use Arduino IDE Serial Plotter para visualizar");
            Serial.println();
            showMainMenu();
            break;
        case 'r':
        case 'R':
            ESP.restart();
            break;
        default:
            Serial.println("\n  [!] Opcion invalida");
            showMainMenu();
            break;
    }
}

// ============================================================================
// PROCESAR MEN├Ü ECG
// ============================================================================
void processECGMenu(char input) {
    ECGCondition selectedCond;
    bool validSelection = true;
    
    switch (input) {
        case '0': selectedCond = ECGCondition::NORMAL; break;
        case '1': selectedCond = ECGCondition::TACHYCARDIA; break;
        case '2': selectedCond = ECGCondition::BRADYCARDIA; break;
        case '3': selectedCond = ECGCondition::ATRIAL_FIBRILLATION; break;
        case '4': selectedCond = ECGCondition::VENTRICULAR_FIBRILLATION; break;
        case '5': selectedCond = ECGCondition::PREMATURE_VENTRICULAR; break;
        case '6': selectedCond = ECGCondition::ST_ELEVATION; break;
        case '7': selectedCond = ECGCondition::ST_DEPRESSION; break;
        case 'b':
        case 'B':
            menuState = MenuState::MAIN_MENU;
            showMainMenu();
            return;
        default:
            validSelection = false;
            Serial.println("\n  [!] Opcion invalida");
            showECGMenu();
            break;
    }
    
    if (validSelection) {
        setupECGCondition(selectedCond);
        menuState = MenuState::WAITING_DURATION;
        showDurationMenu();
    }
}

// ============================================================================
// PROCESAR MEN├Ü DURACI├ôN
// ============================================================================
void processDurationMenu(char input) {
    continuousMode = false;
    
    switch (input) {
        case '1': plotDuration = 5000; break;
        case '2': plotDuration = 10000; break;
        case '3': plotDuration = 30000; break;
        case '4': plotDuration = 60000; break;
        case 'c':
        case 'C':
            continuousMode = true;
            plotDuration = 0xFFFFFFFF;
            break;
        case 'b':
        case 'B':
            // Volver al men├║ correspondiente
            if (currentSignalType == SignalType::ECG) {
                menuState = MenuState::ECG_MENU;
                showECGMenu();
            } else if (currentSignalType == SignalType::EMG) {
                menuState = MenuState::EMG_MENU;
                showEMGMenu();
            } else {
                menuState = MenuState::PPG_MENU;
                showPPGMenu();
            }
            return;
        default:
            Serial.println("\n  [!] Opcion invalida");
            showDurationMenu();
            return;
    }
    
    startPlotting();
}

// ============================================================================
// PROCESAR MEN├Ü EMG
// ============================================================================
void processEMGMenu(char input) {
    EMGCondition selectedCond;
    bool validSelection = true;
    
    switch (input) {
        case '0': selectedCond = EMGCondition::REST; break;
        case '1': selectedCond = EMGCondition::LOW_CONTRACTION; break;
        case '2': selectedCond = EMGCondition::MODERATE_CONTRACTION; break;
        case '3': selectedCond = EMGCondition::HIGH_CONTRACTION; break;
        case '4': selectedCond = EMGCondition::TREMOR; break;
        case '5': selectedCond = EMGCondition::MYOPATHY; break;
        case '6': selectedCond = EMGCondition::NEUROPATHY; break;
        case '7': selectedCond = EMGCondition::FASCICULATION; break;
        case 'b':
        case 'B':
            menuState = MenuState::MAIN_MENU;
            showMainMenu();
            return;
        default:
            validSelection = false;
            Serial.println("\n  [!] Opcion invalida");
            showEMGMenu();
            break;
    }
    
    if (validSelection) {
        setupEMGCondition(selectedCond);
        menuState = MenuState::WAITING_DURATION;
        showDurationMenu();
    }
}

// ============================================================================
// PROCESAR MEN├Ü PPG
// ============================================================================
void processPPGMenu(char input) {
    PPGCondition selectedCond;
    bool validSelection = true;
    
    switch (input) {
        case '0': selectedCond = PPGCondition::NORMAL; break;
        case '1': selectedCond = PPGCondition::ARRHYTHMIA; break;
        case '2': selectedCond = PPGCondition::WEAK_PERFUSION; break;
        case '3': selectedCond = PPGCondition::STRONG_PERFUSION; break;
        case '4': selectedCond = PPGCondition::VASOCONSTRICTION; break;
        case '5': selectedCond = PPGCondition::LOW_SPO2; break;
        case 'b':
        case 'B':
            menuState = MenuState::MAIN_MENU;
            showMainMenu();
            return;
        default:
            validSelection = false;
            Serial.println("\n  [!] Opcion invalida");
            showPPGMenu();
            break;
    }
    
    if (validSelection) {
        setupPPGCondition(selectedCond);
        menuState = MenuState::WAITING_DURATION;
        showDurationMenu();
    }
}

// ============================================================================
// SETUP
// ============================================================================
void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // Inicializar DAC
    pinMode(DAC_SIGNAL_PIN, OUTPUT);
    dacWrite(DAC_SIGNAL_PIN, 128);
    
    #if AUTO_START
    // ========== MODO AUTO-START PARA SERIAL PLOTTER ==========
    Serial.println();
    Serial.println("ÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉ");
    Serial.println("  BIOSIMULATOR PRO - AUTO-START MODE");
    Serial.println("ÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉ");
    
    // Configurar se├▒al seg├║n AUTO_SIGNAL_TYPE
    #if AUTO_SIGNAL_TYPE == 0  // ECG
        setupECGCondition((ECGCondition)AUTO_ECG_CONDITION);
        Serial.printf("  Se├▒al: ECG - %s\n", getECGConditionName((ECGCondition)AUTO_ECG_CONDITION));
    #elif AUTO_SIGNAL_TYPE == 1  // EMG
        setupEMGCondition((EMGCondition)AUTO_EMG_CONDITION);
        Serial.printf("  Se├▒al: EMG - %s\n", getEMGConditionName((EMGCondition)AUTO_EMG_CONDITION));
    #else  // PPG
        setupPPGCondition((PPGCondition)AUTO_PPG_CONDITION);
        Serial.printf("  Se├▒al: PPG - %s\n", getPPGConditionName((PPGCondition)AUTO_PPG_CONDITION));
    #endif
    
    continuousMode = AUTO_CONTINUOUS;
    plotDuration = AUTO_CONTINUOUS ? 0xFFFFFFFF : PLOT_DURATION_MS;
    
    Serial.println("  Modo: CONTINUO (presione 'r' en Serial Monitor para reiniciar)");
    Serial.println("ÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉÔòÉ");
    Serial.println();
    delay(2000);
    
    plotStartTime = millis();
    menuState = MenuState::PLOTTING;
    #else
    // Mostrar men├║ principal (modo interactivo)
    showMainMenu();
    #endif
}

// ============================================================================
// LOOP
// ============================================================================
void loop() {
    static unsigned long lastSample = 0;
    static unsigned long lastPlot = 0;
    unsigned long now = millis();
    
    // ========== MODO MEN├Ü ==========
    if (menuState != MenuState::PLOTTING) {
        if (Serial.available()) {
            char input = Serial.read();
            clearSerialBuffer();
            Serial.println(input);  // Echo
            
            switch (menuState) {
                case MenuState::MAIN_MENU:
                    processMainMenu(input);
                    break;
                case MenuState::ECG_MENU:
                    processECGMenu(input);
                    break;
                case MenuState::EMG_MENU:
                    processEMGMenu(input);
                    break;
                case MenuState::PPG_MENU:
                    processPPGMenu(input);
                    break;
                case MenuState::WAITING_DURATION:
                    processDurationMenu(input);
                    break;
                default:
                    break;
            }
        }
        return;
    }
    
    // ========== MODO PLOTTING ==========
    
    // Verificar si hay que detener
    if (Serial.available()) {
        char c = Serial.read();
        clearSerialBuffer();
        if (c == 'q' || c == 'Q') {
            showPlotSummary();
            return;
        }
    }
    
    // Verificar duraci├│n
    if (!continuousMode && (now - plotStartTime >= plotDuration)) {
        showPlotSummary();
        return;
    }
    
    // Generar muestras a 1000 Hz seg├║n tipo de se├▒al
    if (now - lastSample >= 1) {
        uint8_t dacValue = 128;
        if (currentSignalType == SignalType::ECG) {
            dacValue = ecgModel.getDACValue(0.001f);
        } else if (currentSignalType == SignalType::EMG) {
            dacValue = emgModel.getDACValue(0.001f);
        } else if (currentSignalType == SignalType::PPG) {
            dacValue = ppgModel.getDACValue(0.001f);
        }
        dacWrite(DAC_SIGNAL_PIN, dacValue);
        lastSample = now;
    }
    
    // Enviar datos seg├║n tipo de se├▒al
    if (now - lastPlot >= DEBUG_SAMPLE_INTERVAL) {
        float value = 0.0f;
        
        if (currentSignalType == SignalType::ECG) {
            value = ecgModel.getCurrentValueMV();
            if (value < minVal) minVal = value;
            if (value > maxVal) maxVal = value;
            
            // Formato Serial Plotter: >nombre:valor
            // ECG: se├▒al, HR, RR, amplitud R, desviaci├│n ST, duraci├│n QRS, latidos
            Serial.print(">ecg:");
            Serial.print(value, 4);
            Serial.print(",hr:");
            Serial.print(ecgModel.getCurrentBPM(), 1);
            Serial.print(",rr:");
            Serial.print(ecgModel.getCurrentRR_ms(), 0);
            Serial.print(",ramp:");
            Serial.print(ecgModel.getRWaveAmplitude_mV(), 2);
            Serial.print(",st:");
            Serial.print(ecgModel.getSTDeviation_mV(), 3);
            Serial.print(",qrs:");
            Serial.print(ecgModel.getQRSDuration_ms(), 0);
            Serial.print(",beats:");
            Serial.print(ecgModel.getBeatCount());
            Serial.println();
            
        } else if (currentSignalType == SignalType::EMG) {
            value = emgModel.getCurrentValueMV();
            if (value < minVal) minVal = value;
            if (value > maxVal) maxVal = value;
            
            // EMG: se├▒al, RMS, MUs activas, frecuencia de disparo, % contracci├│n
            Serial.print(">emg:");
            Serial.print(value, 4);
            Serial.print(",rms:");
            Serial.print(emgModel.getRMSAmplitude(), 3);
            Serial.print(",mus:");
            Serial.print(emgModel.getActiveMotorUnits());
            Serial.print(",fr:");
            Serial.print(emgModel.getMeanFiringRate(), 1);
            Serial.print(",cont:");
            Serial.print(emgModel.getContractionLevel(), 1);
            Serial.println();
            
        } else if (currentSignalType == SignalType::PPG) {
            value = ppgModel.getCurrentValueNormalized();
            if (value < minVal) minVal = value;
            if (value > maxVal) maxVal = value;
            
            // PPG: se├▒al, HR, RR, PI, SpO2, latidos
            Serial.print(">ppg:");
            Serial.print(value, 4);
            Serial.print(",hr:");
            Serial.print(ppgModel.getCurrentHeartRate(), 1);
            Serial.print(",rr:");
            Serial.print(ppgModel.getCurrentRRInterval(), 0);
            Serial.print(",pi:");
            Serial.print(ppgModel.getPerfusionIndex(), 2);
            Serial.print(",spo2:");
            Serial.print(ppgModel.getSpO2(), 1);
            Serial.print(",beats:");
            Serial.print(ppgModel.getBeatCount());
            Serial.println();
        }
        
        lastPlot = now;
    }
    
    // ========== ESTAD├ìSTICAS PERI├ôDICAS (cada 3 segundos) ==========
    static unsigned long lastStats = 0;
    if (now - lastStats >= 3000) {
        Serial.println();
        Serial.println("ÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇ");
        Serial.printf("  [%lu s] METRICAS EN TIEMPO REAL\n", (now - plotStartTime) / 1000);
        Serial.println("ÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇ");
        
        if (currentSignalType == SignalType::ECG) {
            float hr = ecgModel.getCurrentBPM();
            float rr = ecgModel.getCurrentRR_ms();
            float rAmp = ecgModel.getRWaveAmplitude_mV();
            float stDev = ecgModel.getSTDeviation_mV();
            uint32_t beats = ecgModel.getBeatCount();
            
            // Obtener rangos esperados
            uint8_t idx = static_cast<uint8_t>(currentECGCondition);
            const ExpectedRange& range = CLINICAL_RANGES[idx];
            bool hrOK = (hr >= range.hrMin && hr <= range.hrMax);
            
            Serial.printf("  HR: %.1f BPM %s (esperado: %.0f-%.0f)\n", 
                         hr, hrOK ? "[OK]" : "[!]", range.hrMin, range.hrMax);
            Serial.printf("  RR: %.0f ms (calculado: %.0f ms)\n", rr, 60000.0f/hr);
            Serial.printf("  Onda R: %.2f mV (tipico: 0.5-1.5 mV)\n", rAmp);
            Serial.printf("  ST: %.3f mV %s\n", stDev, 
                         (fabsf(stDev) < 0.1f) ? "[normal]" : "[elevado/deprimido]");
            Serial.printf("  Latidos: %lu | Rango se├▒al: [%.3f, %.3f]\n", beats, minVal, maxVal);
            
        } else if (currentSignalType == SignalType::EMG) {
            float rms = emgModel.getRMSAmplitude();
            int mus = emgModel.getActiveMotorUnits();
            float fr = emgModel.getMeanFiringRate();
            float excit = emgModel.getContractionLevel();
            
            uint8_t idx = static_cast<uint8_t>(currentEMGCondition);
            const EMGExpectedRange& range = EMG_CLINICAL_RANGES[idx];
            bool rmsOK = (rms >= range.rmsMin && rms <= range.rmsMax);
            
            Serial.printf("  RMS: %.3f mV %s (esperado: %.2f-%.2f mV)\n", 
                         rms, rmsOK ? "[OK]" : "[!]", range.rmsMin, range.rmsMax);
            Serial.printf("  MUs activas: %d/100 (Henneman: reclutamiento ordenado)\n", mus);
            Serial.printf("  FR media: %.1f Hz (rango fisiol: 6-50 Hz)\n", fr);
            Serial.printf("  Excitacion: %.0f%% | Rango se├▒al: [%.3f, %.3f] mV\n", 
                         excit, minVal, maxVal);
            
        } else if (currentSignalType == SignalType::PPG) {
            float hr = ppgModel.getCurrentHeartRate();
            float rr = ppgModel.getCurrentRRInterval();
            float pi = ppgModel.getPerfusionIndex();
            uint32_t beats = ppgModel.getBeatCount();
            
            uint8_t idx = static_cast<uint8_t>(currentPPGCondition);
            const PPGExpectedRange& range = PPG_CLINICAL_RANGES[idx];
            bool hrOK = (hr >= range.hrMin && hr <= range.hrMax);
            bool piOK = (pi >= range.piMin && pi <= range.piMax);
            
            Serial.printf("  HR: %.1f BPM %s (esperado: %.0f-%.0f)\n", 
                         hr, hrOK ? "[OK]" : "[!]", range.hrMin, range.hrMax);
            Serial.printf("  RR: %.0f ms\n", rr);
            Serial.printf("  PI: %.2f%% %s (esperado: %.1f-%.1f%%)\n", 
                         pi, piOK ? "[OK]" : "[!]", range.piMin, range.piMax);
            Serial.printf("  Latidos: %lu | Rango se├▒al: [%.3f, %.3f]\n", beats, minVal, maxVal);
        }
        
        Serial.println("ÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇÔöÇ");
        Serial.println();
        
        lastStats = now;
    }
}
