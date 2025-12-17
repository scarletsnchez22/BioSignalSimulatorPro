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
#include "data/emg_sequences.h"
#include "models/ecg_model.h"
#include "models/emg_model.h"
#include "models/ppg_model.h"

// ============================================================================
// CONFIGURACION DE MUESTREO
// ============================================================================
// Señal se genera cada 1 ms con aceleración para visualización
// Serial Plotter recibe datos cada 1 ms (1000 puntos/segundo)
#define PLOT_UPDATE_INTERVAL_MS     1    // Enviar a Serial Plotter cada 1 ms (1000 Hz)
#define SIGNAL_SPEED_MULTIPLIER     5.0f // 5x más rápido para ver varios latidos en pantalla

// ============================================================================
// CONFIGURACION AUTO-START (Para Serial Plotter)
// ============================================================================
// Cambiar estos valores para seleccionar senal y patologia

#define AUTO_START              1       // 1 = inicio automatico, 0 = menu interactivo

// --------------------------------------------------------------------------
// TIPO DE SENAL: 0=ECG, 1=EMG, 2=PPG
// --------------------------------------------------------------------------
#define AUTO_SIGNAL_TYPE        2

// --------------------------------------------------------------------------
// CONDICIONES ECG (solo si AUTO_SIGNAL_TYPE==0):
// --------------------------------------------------------------------------
// 0 = NORMAL           - Ritmo sinusal 60-100 BPM, <10% variabilidad
// 1 = TACHYCARDIA      - >100 BPM, ritmo regular
// 2 = BRADYCARDIA      - <60 BPM, ritmo regular
// 3 = ATRIAL_FIBRILLATION - RR irregular, sin ondas P
// 4 = VENTRICULAR_FIBRILLATION - Ondas caoticas (emergencia)
// 5 = AV_BLOCK_1       - Bloqueo AV 1er grado, PR > 200 ms
// 6 = ST_ELEVATION     - STEMI (infarto agudo)
// 7 = ST_DEPRESSION    - Isquemia subendocardica
#define AUTO_ECG_CONDITION      0

// --------------------------------------------------------------------------
// CONDICIONES EMG (solo si AUTO_SIGNAL_TYPE==1):
// --------------------------------------------------------------------------
// 0 = REST             - Reposo, 0-5% MVC
// 1 = LOW_CONTRACTION  - Baja, 5-20% MVC
// 2 = MODERATE_CONTRACTION - Moderada, 20-50% MVC
// 3 = HIGH_CONTRACTION - Alta, 50-100% MVC
// 4 = TREMOR           - Parkinson 4-6 Hz
// 5 = FATIGUE          - Fatiga muscular (50% MVC sostenido)
#define AUTO_EMG_CONDITION      5

// --------------------------------------------------------------------------
// CONDICIONES PPG (solo si AUTO_SIGNAL_TYPE==2):
// --------------------------------------------------------------------------
// 0 = NORMAL           - PI 2-5%, SpO2 95-100%
// 1 = ARRHYTHMIA       - RR irregular
// 2 = WEAK_PERFUSION   - PI 0.02-0.4%, shock/hipotermia
// 3 = VASODILATION     - PI 5-10%, vasodilatacion
// 4 = STRONG_PERFUSION - PI 10-20%, perfusion muy fuerte
// 5 = VASOCONSTRICTION - PI 0.2-0.8%, frio/estres
#define AUTO_PPG_CONDITION      5

// --------------------------------------------------------------------------
// MODO DE EJECUCION
// --------------------------------------------------------------------------
// AUTO_CONTINUOUS = 1: Corre indefinidamente (presiona 'r' para reiniciar)
// AUTO_CONTINUOUS = 0: Corre por PLOT_DURATION_SAMPLES y luego para
//
// ¿Que es una MUESTRA (sample)?
// - 1 muestra = 1 punto de datos (1 valor de voltaje)
// - A 1000 Hz: 1000 muestras = 1 segundo
// - 1 latido a 70 BPM = ~857 muestras
// - 10 latidos a 70 BPM = ~8570 muestras
//
#define AUTO_CONTINUOUS             1       // 1=continuo, 0=duracion fija
#define PLOT_DURATION_MS            10000   // Duracion en milisegundos (10 segundos por defecto)

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
        case ECGCondition::AV_BLOCK_1:              return "Bloqueo AV 1er Grado";
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
        case ECGCondition::AV_BLOCK_1:              return "BAV1";
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
        case EMGCondition::REST:                 return "Reposo (0-5% MVC)";
        case EMGCondition::LOW_CONTRACTION:      return "Baja (5-20% MVC)";
        case EMGCondition::MODERATE_CONTRACTION: return "Moderada (20-50% MVC)";
        case EMGCondition::HIGH_CONTRACTION:     return "Alta (50-100% MVC)";
        case EMGCondition::TREMOR:               return "Temblor Parkinson (4-6 Hz)";
        case EMGCondition::FATIGUE:              return "Fatiga Muscular (50% MVC sostenido)";
        default:                                 return "Desconocido";
    }
}

// ============================================================================
// NOMBRES PPG - Basado en Allen 2007, Elgendi 2012
// ============================================================================
const char* getPPGConditionName(PPGCondition cond) {
    switch (cond) {
        case PPGCondition::NORMAL:            return "Normal (PI 1-5%)";
        case PPGCondition::ARRHYTHMIA:        return "Arritmia (RR irregular)";
        case PPGCondition::WEAK_PERFUSION:    return "Perfusion Debil (PI 0.02-0.4%)";
        case PPGCondition::VASODILATION:      return "Vasodilatacion (PI 5-10%)";
        case PPGCondition::STRONG_PERFUSION:  return "Perfusion Fuerte (PI 10-20%)";
        case PPGCondition::VASOCONSTRICTION:  return "Vasoconstriccion (PI 0.2-0.8%)";
        default:                              return "Desconocido";
    }
}

// ============================================================================
// RANGOS CLINICOS EMG - VALIDADOS CON FUGLEVAND 1993
// ============================================================================
/**
 * NOTA: LOW, MODERATE y HIGH usan SECUENCIAS DINAMICAS (REST → CONTRACCION)
 * Los valores RMS son el TECHO durante la fase de contracción.
 * Durante REST: RMS ≈ 0.001 mV (solo ruido)
 */
struct EMGExpectedRange {
    float rmsMin;       // RMS minimo (durante REST en secuencias dinamicas)
    float rmsMax;       // RMS maximo (TECHO durante contraccion)
    float excitationMin; // Excitacion minima
    float excitationMax; // Excitacion maxima
    const char* description;
};

static const EMGExpectedRange EMG_CLINICAL_RANGES[] = {
    { 0.001f, 0.001f, 0.005f, 0.005f, "Reposo: RMS 0.001mV (solo ruido)" },
    { 0.001f, 0.60f,  0.12f,  0.12f,  "Leve: RMS techo 0.52mV (secuencia dinamica)" },
    { 0.001f, 2.00f,  0.35f,  0.35f,  "Moderada: RMS techo 1.7mV (secuencia dinamica)" },
    { 0.001f, 3.10f,  0.80f,  0.80f,  "Alta: RMS techo 2.8mV (secuencia dinamica)" },
    { 0.10f,  0.50f,  0.0f,   0.0f,   "Temblor: RMS 0.1-0.5mV, 5Hz" },
    { 1.50f,  0.40f,  0.50f,  0.50f,  "Fatiga: RMS 1.5mV->0.4mV (decay)" }
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
    Serial.println("    [5] Bloqueo AV 1er Grado (BAV1)");
    Serial.println("    [6] Elevacion ST (Infarto STEMI)");
    Serial.println("    [7] Depresion ST (Isquemia)");
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
    Serial.println("    [0] Reposo (0-5% MVC, RMS<50uV)");
    Serial.println("    [1] Contraccion Baja (5-20% MVC)");
    Serial.println("    [2] Contraccion Moderada (20-50% MVC)");
    Serial.println("    [3] Contraccion Alta (50-100% MVC)");
    Serial.println("    [4] Temblor Parkinsoniano (4-6 Hz)");
    Serial.println("    [5] Fatiga Muscular (50% MVC sostenido)");
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

// Índices alineados con ECGCondition enum:
// 0=NORMAL, 1=TACHYCARDIA, 2=BRADYCARDIA, 3=ATRIAL_FIBRILLATION,
// 4=VENTRICULAR_FIBRILLATION, 5=AV_BLOCK_1, 6=ST_ELEVATION, 7=ST_DEPRESSION
static const ExpectedRange CLINICAL_RANGES[] = {
    { 60.0f, 100.0f, "Normal: 60-100 BPM, <10% var" },           // 0 - NORMAL
    { 100.0f, 180.0f, "Taquicardia: >100 BPM" },                 // 1 - TACHYCARDIA
    { 30.0f, 59.0f, "Bradicardia: <60 BPM" },                    // 2 - BRADYCARDIA
    { 60.0f, 180.0f, "AFib: 60-180 BPM (RR irregular)" },        // 3 - ATRIAL_FIBRILLATION
    { 150.0f, 500.0f, "VFib: caotico (4-10 Hz)" },               // 4 - VENTRICULAR_FIBRILLATION
    { 60.0f, 100.0f, "BAV1: 60-100 BPM, PR>200ms" },             // 5 - AV_BLOCK_1
    { 50.0f, 110.0f, "STEMI: 50-110 BPM, ST>=0.2mV" },           // 6 - ST_ELEVATION
    { 50.0f, 150.0f, "NSTEMI: 50-150 BPM, ST<=-0.05mV" }         // 7 - ST_DEPRESSION
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
    // ECG no necesita speedMultiplier (siempre tiempo real)
    
    minVal = 999.0f;
    maxVal = -999.0f;
    
    uint8_t idx = static_cast<uint8_t>(cond);
    if (idx < 8) {  // 8 condiciones ECG: 0-7
        Serial.printf("  [Rango clinico: %s]\n", CLINICAL_RANGES[idx].description);
        Serial.printf("  [HR objetivo: %.0f BPM]\n", ecgModel.getHRMean());
        // ECG siempre en tiempo real (1.0x)
    }
}

// ============================================================================
// CONFIGURAR EMG CON SECUENCIAS DINAMICAS
// ============================================================================
void setupEMGCondition(EMGCondition cond) {
    currentSignalType = SignalType::EMG;
    currentEMGCondition = cond;
    
    // Configurar parámetros base
    EMGParameters params;
    params.condition = cond;
    params.noiseLevel = 0.02f;
    params.excitationLevel = 0.0f;  // Usar default de la condición
    params.amplitude = 1.0f;        // Sin ganancia artificial
    
    // Ajustar ruido según condición
    switch (cond) {
        case EMGCondition::REST:
            params.noiseLevel = 0.01f;
            break;
        case EMGCondition::TREMOR:
            params.noiseLevel = 0.015f;
            break;
        default:
            params.noiseLevel = 0.02f;
            break;
    }
    
    // Las SECUENCIAS se activan AUTOMÁTICAMENTE en emg_model.cpp
    // según la condición seleccionada (funciona para debug y Nextion)
    emgModel.setParameters(params);
    
    // Mostrar info de la secuencia activada
    switch (cond) {
        case EMGCondition::REST:
            Serial.println("  [Modo: Reposo estatico]");
            break;
        case EMGCondition::LOW_CONTRACTION:
            Serial.println("  [Modo: Secuencia dinamica REST->LOW]");
            Serial.println("  [Ciclo: REST 1s + LOW 3s = 4s total, ~3-4 bursts por ventana]");
            break;
        case EMGCondition::MODERATE_CONTRACTION:
            Serial.println("  [Modo: Secuencia dinamica REST->MODERATE]");
            Serial.println("  [Ciclo: REST 1s + MOD 3s = 4s total, ~3-4 bursts por ventana]");
            break;
        case EMGCondition::HIGH_CONTRACTION:
            Serial.println("  [Modo: Secuencia dinamica REST->HIGH]");
            Serial.println("  [Ciclo: REST 1s + HIGH 3s = 4s total, ~3-4 bursts por ventana]");
            break;
        case EMGCondition::TREMOR:
            Serial.println("  [Modo: Temblor continuo - Parkinson 4-6 Hz]");
            break;
        case EMGCondition::FATIGUE:
            Serial.println("  [Modo: Fatiga continua con decay progresivo]");
            break;
    }
    
    minVal = 999.0f;
    maxVal = -999.0f;
    
    uint8_t idx = static_cast<uint8_t>(cond);
    if (idx < 6) {  // 6 condiciones EMG: 0-5
        Serial.printf("  [Rango clinico: %s]\n", EMG_CLINICAL_RANGES[idx].description);
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
    params.heartRate = 75.0f;  // Default, será modificado por condición
    params.dicroticNotch = 0.25f;
    params.noiseLevel = 0.002f;  // Reducido para señal más limpia
    
    // Ajustar PI base según condición para rangos clínicos correctos
    switch (cond) {
        case PPGCondition::NORMAL:
            params.perfusionIndex = 3.5f;   // 2-5%
            break;
        case PPGCondition::ARRHYTHMIA:
            params.perfusionIndex = 3.0f;   // 1-5%
            break;
        case PPGCondition::WEAK_PERFUSION:
            params.perfusionIndex = 0.3f;   // 0.1-0.5% (shock)
            break;
        case PPGCondition::STRONG_PERFUSION:
            params.perfusionIndex = 10.0f;  // 5-20% (vasodilatación)
            break;
        case PPGCondition::VASODILATION:
            params.perfusionIndex = 7.5f;   // 5-10% (vasodilatación)
            break;
        case PPGCondition::VASOCONSTRICTION:
            params.perfusionIndex = 0.5f;   // 0.2-0.8% (frío)
            break;
        default:
            params.perfusionIndex = 3.0f;
            break;
    }
    
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
        if (idx < 8) {  // 8 condiciones ECG: 0-7
            const ExpectedRange& range = CLINICAL_RANGES[idx];
            bool inRange = (hr >= range.hrMin && hr <= range.hrMax);
            Serial.printf("  HR inicial: %.0f BPM %s\n", hr, inRange ? "[OK]" : "[FUERA DE RANGO!]");
        }
    } else if (currentSignalType == SignalType::EMG) {
        Serial.printf("  PLOTEANDO: EMG - %s\n", getEMGConditionName(currentEMGCondition));
        uint8_t idx = static_cast<uint8_t>(currentEMGCondition);
        if (idx < 6) {  // 6 condiciones EMG
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
        if (idx < 8) {  // 8 condiciones ECG: 0-7
            const ExpectedRange& range = CLINICAL_RANGES[idx];
            bool inRange = (hr >= range.hrMin && hr <= range.hrMax);
            Serial.printf("  HR final: %.1f BPM %s\n", hr, inRange ? "[OK]" : "[FUERA DE RANGO!]");
        }
        Serial.printf("  RR: %.0f ms, Latidos: %lu\n", ecgModel.getCurrentRR_ms(), ecgModel.getBeatCount());
        
    } else if (currentSignalType == SignalType::EMG) {
        Serial.printf("  Senal: EMG - %s\n", getEMGConditionName(currentEMGCondition));
        float rms = emgModel.getRMSAmplitude();
        uint8_t idx = static_cast<uint8_t>(currentEMGCondition);
        if (idx < 6) {  // 6 condiciones EMG
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
        case '5': selectedCond = ECGCondition::AV_BLOCK_1; break;
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
        case '5': selectedCond = EMGCondition::FATIGUE; break;
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
        case '3': selectedCond = PPGCondition::VASODILATION; break;
        case '4': selectedCond = PPGCondition::STRONG_PERFUSION; break;
        case '5': selectedCond = PPGCondition::VASOCONSTRICTION; break;
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
    // Variables para mostrar rango de señal
    float rangoMin = 0, rangoMax = 0;
    const char* unidadRango = "mV";
    
    #if AUTO_SIGNAL_TYPE == 0  // ECG
        setupECGCondition((ECGCondition)AUTO_ECG_CONDITION);
        Serial.printf("  Senal: ECG - %s\n", getECGConditionName((ECGCondition)AUTO_ECG_CONDITION));
        ecgModel.getOutputRange(&rangoMin, &rangoMax);
    #elif AUTO_SIGNAL_TYPE == 1  // EMG
        setupEMGCondition((EMGCondition)AUTO_EMG_CONDITION);
        Serial.printf("  Senal: EMG - %s\n", getEMGConditionName((EMGCondition)AUTO_EMG_CONDITION));
        emgModel.getOutputRange(&rangoMin, &rangoMax);
    #else  // PPG
        setupPPGCondition((PPGCondition)AUTO_PPG_CONDITION);
        Serial.printf("  Senal: PPG - %s\n", getPPGConditionName((PPGCondition)AUTO_PPG_CONDITION));
        rangoMin = 0.8f;
        rangoMax = 1.3f;
        unidadRango = "norm";  // PPG usa valores normalizados
    #endif
    
    // Mostrar rango de salida de la señal actual
    Serial.printf("  Rango de salida: [%.3f, %.3f] %s\n", rangoMin, rangoMax, unidadRango);
    
    continuousMode = AUTO_CONTINUOUS;
    plotDuration = AUTO_CONTINUOUS ? 0xFFFFFFFF : PLOT_DURATION_MS;
    
    #if AUTO_CONTINUOUS
    Serial.println("  Modo: CONTINUO (presiona 'r' para reiniciar)");
    #else
    Serial.printf("  Modo: DURACION FIJA - %.1f segundos\n", (float)PLOT_DURATION_MS / 1000.0f);
    #endif
    Serial.printf("  DAC: actualizacion continua | Serial Plotter: cada %d ms\n", PLOT_UPDATE_INTERVAL_MS);
    Serial.println("  Eje X: muestras enviadas | Eje Y: ver unidad de rango");
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
    
    // Verificar duración
    if (!continuousMode && (now - plotStartTime >= plotDuration)) {
        showPlotSummary();
        return;
    }
    
    // Actualizar señal cada 1 ms (1000 Hz) - UNA SOLA VEZ
    static unsigned long lastSample = 0;
    if (now - lastSample >= 1) {
        lastSample = now;
        
        // deltaTime acelerado para ver más latidos en pantalla
        // El modelo avanza SIGNAL_SPEED_MULTIPLIER veces más rápido
        // Las mediciones compensan multiplicando sampleCount por el mismo factor
        float deltaTime = 0.001f * SIGNAL_SPEED_MULTIPLIER;
        
        uint8_t dacValue = 128;
        if (currentSignalType == SignalType::ECG) {
            dacValue = ecgModel.getDACValue(deltaTime);
        } else if (currentSignalType == SignalType::EMG) {
            // ✅ NUEVO API: Tick del modelo (1 sola vez)
            emgModel.tick(deltaTime);
            dacValue = emgModel.getRawDACValue();  // DAC de señal cruda
        } else if (currentSignalType == SignalType::PPG) {
            dacValue = ppgModel.getDACValue(deltaTime);
        }
        dacWrite(DAC_SIGNAL_PIN, dacValue);
    }
    
    // Enviar datos al Serial Plotter cada 2 ms
    if (now - lastPlot >= PLOT_UPDATE_INTERVAL_MS) {
        float value = 0.0f;
        
        if (currentSignalType == SignalType::ECG) {
            value = ecgModel.getCurrentValueMV();  // Señal en mV clínicos
            if (value < minVal) minVal = value;
            if (value > maxVal) maxVal = value;
            
            // =====================================================================
            // FORMATO SERIAL PLOTTER - ECG Lead II (valores clínicos)
            // =====================================================================
            // Señal: mV | Intervalos: ms | Amplitudes: mV | Frecuencia: bpm
            //
            // Parámetros mostrados:
            // - ecg: señal instantánea en mV
            // - HR: frecuencia cardíaca (bpm) = 60000/RR_ms
            // - RR: intervalo R-R (ms)
            // - PR: intervalo PR (ms) - conducción AV
            // - QRS: duración QRS (ms) - despolarización ventricular
            // - QT: intervalo QT medido (ms) - sístole eléctrica
            // - QTc: intervalo QTc corregido Bazett (ms) - IMPORTANTE CLÍNICO
            // - P: amplitud onda P (mV)
            // - Q: amplitud onda Q (mV, negativa)
            // - R: amplitud onda R (mV)
            // - S: amplitud onda S (mV, negativa)
            // - ST: desviación ST (mV)
            // - T: amplitud onda T (mV)
            
            Serial.print(">ecg:");
            Serial.print(value, 3);           // Señal ECG en mV
            Serial.print(",HR:");
            Serial.print(ecgModel.getHeartRate_bpm(), 1);  // bpm
            Serial.print(",RR:");
            Serial.print(ecgModel.getRRInterval_ms(), 0);  // ms
            Serial.print(",PR:");
            Serial.print(ecgModel.getPRInterval_ms(), 0);  // ms
            Serial.print(",QRS:");
            Serial.print(ecgModel.getQRSDuration_ms(), 0); // ms
            Serial.print(",QT:");
            Serial.print(ecgModel.getQTInterval_ms(), 0);  // ms
            Serial.print(",QTc:");
            Serial.print(ecgModel.getQTcInterval_ms(), 0); // ms (Bazett)
            Serial.print(",P:");
            Serial.print(ecgModel.getPAmplitude_mV(), 2);  // mV
            Serial.print(",Q:");
            Serial.print(ecgModel.getQAmplitude_mV(), 2);  // mV (negativa)
            Serial.print(",R:");
            Serial.print(ecgModel.getRAmplitude_mV(), 2);  // mV
            Serial.print(",S:");
            Serial.print(ecgModel.getSAmplitude_mV(), 2);  // mV (negativa)
            Serial.print(",ST:");
            Serial.print(ecgModel.getSTDeviation_mV(), 2); // mV
            Serial.print(",T:");
            Serial.print(ecgModel.getTAmplitude_mV(), 2);  // mV
            Serial.println();
            
        } else if (currentSignalType == SignalType::EMG) {
            // Obtener ambas se├▒ales (ya generadas en tick())
            float rawSignal = emgModel.getRawSample();      // Cruda bipolar ┬▒5 mV
            float procSignal = emgModel.getProcessedSample(); // Procesada 0-5 mV
            
            // Track min/max de se├▒al cruda para resumen
            if (rawSignal < minVal) minVal = rawSignal;
            if (rawSignal > maxVal) maxVal = rawSignal;
            
            // FORMATO SERIAL PLOTTER - EMG Dual Channel (valores cl├¡nicos)
            // raw_emg: se├▒al cruda bipolar en mV (┬▒5 mV) -> DAC GPIO25
            // proc_emg: envolvente RMS en mV (0-5 mV) -> Nextion waveform
            // rms: amplitud RMS instant├ínea (mV)
            // mus: unidades motoras activas (count)
            // fr: frecuencia disparo media (Hz)
            // cont: nivel contracci├│n (%MVC)
            // mdf: frecuencia mediana (Hz) - solo FATIGUE
            // mfl: nivel fatiga muscular (0-1) - solo FATIGUE
            
            Serial.print(">raw_emg:");
            Serial.print(rawSignal, 4);           // Se├▒al cruda ┬▒5 mV
            Serial.print(",proc_emg:");
            Serial.print(procSignal, 4);          // Envolvente 0-5 mV
            Serial.print(",rms:");
            Serial.print(emgModel.getRMSAmplitude(), 3);  // RMS mV
            Serial.print(",mus:");
            Serial.print(emgModel.getActiveMotorUnits()); // Count
            Serial.print(",fr:");
            Serial.print(emgModel.getMeanFiringRate(), 1); // Hz
            Serial.print(",cont:");
            Serial.print(emgModel.getContractionLevel(), 1); // %MVC
            
            // M├®tricas de fatiga (solo si condici├│n = FATIGUE)
            if (emgModel.isFatigueActive()) {
                Serial.print(",mdf:");
                Serial.print(emgModel.getFatigueMDF(), 1);    // Hz (120->80)
                Serial.print(",mfl:");
                Serial.print(emgModel.getFatigueMFL(), 3);    // 0-1
            }
            Serial.println();
            
        } else if (currentSignalType == SignalType::PPG) {
            value = ppgModel.generateSample(0.001f);  // Versión antigua retorna 0-1 normalizado
            if (value < minVal) minVal = value;
            if (value > maxVal) maxVal = value;
            
            // PPG: señal, HR, RR, PI, latidos (versión antigua)
            Serial.print(">ppg:");
            Serial.print(value, 4);
            Serial.print(",hr:");
            Serial.print(ppgModel.getCurrentHeartRate(), 1);
            Serial.print(",rr:");
            Serial.print(ppgModel.getCurrentRRInterval(), 0);
            Serial.print(",pi:");
            Serial.print(ppgModel.getPerfusionIndex(), 2);
            Serial.print(",beats:");
            Serial.print(ppgModel.getBeatCount());
            Serial.println();
        }
        
        lastPlot = now;
    }
    
    // ========== ESTADISTICAS PERIODICAS (cada 2 segundos) ==========
    static unsigned long lastStats = 0;
    if (now - lastStats >= 2000) {
        Serial.println();
        Serial.println("========================================================");
        Serial.printf("  [%lu s] METRICAS EN TIEMPO REAL\n", (now - plotStartTime) / 1000);
        Serial.println("--------------------------------------------------------");
        
        if (currentSignalType == SignalType::ECG) {
            // Obtener todas las métricas clínicas
            ECGDisplayMetrics m = ecgModel.getDisplayMetrics();
            
            // Obtener rangos esperados
            uint8_t idx = static_cast<uint8_t>(currentECGCondition);
            const ExpectedRange& range = CLINICAL_RANGES[idx];
            bool hrOK = (m.bpm >= range.hrMin && m.bpm <= range.hrMax);
            bool qrsOK = (m.qrsDuration_ms >= 60.0f && m.qrsDuration_ms <= 120.0f);
            // PR: para AVB1 debe ser >200ms, para otros 120-200ms
            bool prOK;
            if (currentECGCondition == ECGCondition::AV_BLOCK_1) {
                prOK = (m.prInterval_ms > 200.0f);  // AVB1: PR prolongado >200ms
            } else if (currentECGCondition == ECGCondition::ATRIAL_FIBRILLATION) {
                prOK = true;  // AFib: sin onda P, PR no aplica
            } else {
                prOK = (m.prInterval_ms >= 120.0f && m.prInterval_ms <= 200.0f);
            }
            bool qtcOK = (m.qtcInterval_ms >= 320.0f && m.qtcInterval_ms <= 460.0f);
            
            Serial.println("  --- FRECUENCIA ---");
            Serial.printf("  HR: %.1f bpm %s (esperado: %.0f-%.0f)\n", 
                         m.bpm, hrOK ? "[OK]" : "[!]", range.hrMin, range.hrMax);
            
            Serial.println("  --- INTERVALOS (ms) ---");
            Serial.printf("  RR: %.0f ms\n", m.rrInterval_ms);
            // Mensaje de PR adaptado a la condición
            if (currentECGCondition == ECGCondition::AV_BLOCK_1) {
                Serial.printf("  PR: %.0f ms %s (AVB1: >200ms)\n", m.prInterval_ms, prOK ? "[OK]" : "[!]");
            } else if (currentECGCondition == ECGCondition::ATRIAL_FIBRILLATION) {
                Serial.printf("  PR: -- ms (AFib: sin onda P)\n");
            } else {
                Serial.printf("  PR: %.0f ms %s (normal: 120-200)\n", m.prInterval_ms, prOK ? "[OK]" : "[!]");
            }
            Serial.printf("  QRS: %.0f ms %s (normal: <120)\n", m.qrsDuration_ms, qrsOK ? "[OK]" : "[!]");
            Serial.printf("  QT: %.0f ms | QTc: %.0f ms %s (normal: 320-460)\n", 
                         m.qtInterval_ms, m.qtcInterval_ms, qtcOK ? "[OK]" : "[!]");
            
            Serial.println("  --- AMPLITUDES (mV) ---");
            Serial.printf("  P: %.2f mV (normal: <0.25)\n", m.pAmplitude_mV);
            Serial.printf("  R: %.2f mV (normal: 0.5-1.5)\n", m.rAmplitude_mV);
            // ST: validación específica por condición
            if (currentECGCondition == ECGCondition::ST_ELEVATION) {
                bool stOK = (m.stDeviation_mV >= 0.2f);  // STEMI: ST >= 0.2mV
                Serial.printf("  ST: %.2f mV %s (STEMI: >=0.2mV)\n", m.stDeviation_mV, stOK ? "[OK]" : "[!]");
            } else if (currentECGCondition == ECGCondition::ST_DEPRESSION) {
                bool stOK = (m.stDeviation_mV <= -0.05f);  // NSTEMI: ST <= -0.05mV
                Serial.printf("  ST: %.2f mV %s (NSTEMI: <=-0.05mV)\n", m.stDeviation_mV, stOK ? "[OK]" : "[!]");
            } else {
                // Condiciones normales: ST isoeléctrico
                bool stOK = (fabsf(m.stDeviation_mV) < 0.1f);
                Serial.printf("  ST: %.2f mV %s\n", m.stDeviation_mV, stOK ? "[normal]" : "[!]");
            }
            Serial.printf("  T: %.2f mV (normal: <0.5)\n", m.tAmplitude_mV);
            
            Serial.printf("  Latidos: %lu | Senal: [%.2f, %.2f] mV\n", m.beatCount, minVal, maxVal);
            
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
        
        Serial.println("========================================================");
        Serial.println();
        
        lastStats = now;
    }
}
