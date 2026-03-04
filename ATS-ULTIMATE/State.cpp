#include "State.h"
#include "InputActions.h"

// --- Variables de Estado Globales ---
long g_storeTime = 0; 
bool g_voltagePinConnnected = false;
bool g_ssbLoaded = false;
bool g_fmStereo = true;

bool g_cmdVolume = false, g_cmdStep = false, g_cmdBw = false, g_cmdBand = false;
bool g_settingsActive = false, g_sMeterOn = false, g_muteVolume = false;
bool g_displayRDS = false, g_processFreqChange = false;
uint32_t g_lastFreqChange = 0;

// Definición única de la línea vacía (16 espacios + fin de cadena = 17)
char _literal_EmptyLine[17] = "                "; 

SI4735 g_si4735;
Rotary g_encoder = Rotary(ENCODER_PIN_A, ENCODER_PIN_B);

int g_currentBFO = 0;
int8_t g_currentMode = AM; // AM, LSB, USB, CW, FM

// --- Configuración de AJUSTES (Settings) ---
// Formato: { "NOM", valor_inicial, min, max, tipo, callback }
SettingsItem g_Settings[] = {
    {"ATT",  0, 0, 37, Num,        doAttenuation},
    {"SMUT", 1, 0, 32, Num,        doSoftMute},
    {"SVC",  0, 0, 1,  Switch,     doSSBAVC},
    {"SYNC", 0, 0, 1,  Switch,     doSync},
    {"DEEM", 0, 0, 1,  Switch,     doDeEmp},
    {"AVC",  1, 0, 1,  Switch,     doAvc},
    {"BRIG", 10,1, 15, Num,        doBrightness},
    {"UNIT", 0, 0, 1,  Switch,     doSWUnits},
    {"SSM",  0, 0, 1,  Switch,     doSSBSoftMuteMode},
    {"FILT", 0, 0, 1,  Switch,     doCutoffFilter},
    {"CPU",  0, 0, 1,  Switch,     doCPUSpeed},
#if USE_RDS
    {"RDS ", 0, 0, 1,  Num,        NULL}, // Ajustado dinámicamente
#endif
    {"BFO",  0, 0, 0,  Num,        doBFOCalibration},
    {"UNIT", 0, 0, 1,  Switch,     doUnitsSwitch},
    {"SCAN", 0, 0, 1,  Switch,     doScanSwitch},
    {"CW  ", 0, 0, 1,  Switch,     doCWSwitch}
};

const uint8_t g_SettingsMaxPages = (sizeof(g_Settings) / sizeof(SettingsItem) + 3) / 4;
int8_t g_SettingSelected = 0;
int8_t g_SettingsPage = 0;
bool g_SettingEditing = false;

// --- Configuración de Anchos de Banda ---
int8_t g_bwIndexSSB = 4;
Bandwidth g_bandwidthSSB[] = {
    {4, "1.2"}, {5, "2.3"}, {6, "3.0"}, {7, "4.0"}, {0, "0.5"}, {1, "1.0"}
};
const uint8_t g_bwSSBMaxIdx = (sizeof(g_bandwidthSSB) / sizeof(Bandwidth)) - 1;

int8_t g_bwIndexAM = 4;
Bandwidth g_bandwidthAM[] = {
    {4, "1.0"}, {5, "1.8"}, {6, "2.0"}, {7, "2.5"}, {0, "3.0"}, {1, "4.0"}, {2, "6.0"}
};
const uint8_t g_maxFilterAM = (sizeof(g_bandwidthAM) / sizeof(Bandwidth)) - 1;

int8_t g_bwIndexFM = 0;
char* g_bandwidthFM[] = { "AUTO", "110k", " 84k", " 60k", " 40k" };

// --- Pasos de Frecuencia ---
int g_tabStep[] = { 1, 5, 9, 10, 50, 100, 1000, 10, 25, 50, 100, 500 };
uint8_t g_amTotalSteps = 7;
uint8_t g_amTotalStepsSSB = 4;
uint8_t g_ssbTotalSteps = 5;
volatile int8_t g_stepIndex = 3;

int8_t g_tabStepFM[] = { 5, 10, 100 };
int8_t g_FMStepIndex = 1;
const int8_t g_lastStepFM = (sizeof(g_tabStepFM) / sizeof(int8_t)) - 1;

// --- Lista de Bandas ---
Band g_bandList[] = {
    { LW_LIMIT_LOW, 520, 300, 0, 4 },
    { 520, 1710, 1476, 3, 4 },
    { SW_LIMIT_LOW, SW_LIMIT_HIGH, SW_LIMIT_LOW, 0, 4 },
    { 8750, 10800, 9580, 1, 0 }, // FM
};

const uint8_t g_lastBand = (sizeof(g_bandList) / sizeof(Band)) - 1;
int8_t g_bandIndex = 1; // Empezar en MW por defecto

uint16_t SWSubBands[] = {
    SW_LIMIT_LOW, 3500, 4500, 5500, 6500, 7500, 9000, 11000, 13000, 14500, 16000, 18000, 21000, 24000, 26000, CB_LIMIT_LOW
};
const uint8_t g_SWSubBandCount = sizeof(SWSubBands) / sizeof(uint16_t);

volatile int g_encoderCount = 0;
uint16_t g_currentFrequency = 0;
uint16_t g_previousFrequency = 0;
