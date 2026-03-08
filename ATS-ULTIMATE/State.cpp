/**
 * @file    State.cpp
 * @brief   Implementación de variables globales, arrays de configuración y tablas PROGMEM.
 * @author  Alonso José Lara Plana (EA7LBT)
 * @license MIT — ver ATS-ULTIMATE.ino para texto completo
 */
#include "State.h"
#include "InputActions.h"

RDSActiveInfo g_rdsMode = StationName;

// --- Variables de Estado Globales ---
long g_storeTime = 0; 
bool g_ssbLoaded = false;
bool g_fmStereo = true;
bool g_cmdVolume = false;
bool g_keyLocked = false;

bool g_settingsActive = false, g_muteVolume = false;
bool g_displayRDS = false;

SI4735 g_si4735;

int g_currentBFO = 0;
int8_t g_currentMode = AM; // AM, LSB, USB, CW, FM

// --- Configuración de AJUSTES (Settings) ---
// Formato: { "NOM", valor_inicial, min, max, tipo, callback }
SettingsItem g_Settings[] = {
    {"VOL ",30, 0, 63, Num,     doDefaultVolume},
    {"ATT",  0, 0, 37, Num,     doAttenuation},
    {"SMUT", 1, 0, 32, Num,     doSoftMute},
    {"SVC",  0, 0, 1,  Switch,  doSSBAVC},
    {"SYNC", 0, 0, 1,  Switch,  doSync},
    {"DEEM", 0, 0, 1,  Switch,  doDeEmp},
    {"AVC",  0, 0,26,  Num,     doAvc},
    {"BRIG", 10,1, 15, Num,     doBrightness},
    {"UNIT", 0, 0, 1,  Switch,  doSWUnits},
    {"SSM",  0, 0, 1,  Switch,  doSSBSoftMuteMode},
    {"FILT", 0, 0, 1,  Switch,  doCutoffFilter},
    {"CPU",  0, 0, 3,  Num,     doCPUSpeed},  // 0=16M 1=8M 2=4M 3=2M
    {"RDS ", 0, 0, 3,  Num,     doRDS},
    {"BFO",  0, 0, 0,  Num,     doBFOCalibration}, // min/max=0: ignorados, doBFOCalibration acumula libre
    {"UKHZ", 0, 0, 1,  Switch,  doUnitsSwitch},
    {"SCAN", 0, 0, 1,  Switch,  doScanSwitch},
    {"CW  ", 0, 0, 1,  Switch,  doCWSwitch},
    {"CWSd", 1, 0, 1,  Switch,  doCWSide},    // 0=LSB, 1=USB (default USB)
    {"STPU", 0, 0, 1,  Switch,  doStepUnits}  // 0=sin unidad, 1=muestra Hz en SSB
};

int8_t g_SettingSelected = 0;
int8_t g_SettingsPage = 0;

// --- Configuración de Anchos de Banda ---
int8_t g_bwIndexSSB = 4;
Bandwidth g_bandwidthSSB[] = {
    {4, "1.2"}, {5, "2.3"}, {6, "3.0"},
    {7, "4.0"}, {0, "0.5"}, {1, "1.0"}
};

int8_t g_bwIndexAM = 4;
Bandwidth g_bandwidthAM[] = {
    {4, "1.0"}, {5, "1.8"}, {6, "2.0"}, {7, "2.5"}, {0, "3.0"}, {1, "4.0"}, {2, "6.0"}
};

int8_t g_bwIndexFM = 0;
const char* g_bandwidthFM[] = { "AUTO", "110k", " 84k", " 60k", " 40k" };

// --- Pasos de Frecuencia ---
const int g_tabStep[] PROGMEM = { 1, 5, 9, 10, 50, 100, 1000, 1, 5, 10, 25, 50, 100, 500 };
volatile int8_t g_stepIndex = 3;

const int8_t g_tabStepFM[] PROGMEM = { 5, 10, 100 };
uint8_t g_FMStepIndex = 1;

// --- Lista de Bandas ---
Band g_bandList[] = {
    { LW_LIMIT_LOW, 520,           300,          0, 4 },  // LW
    { 520,          1710,          1476,         3, 4 },  // MW
    { SW_LIMIT_LOW, SW_LIMIT_HIGH, SW_LIMIT_LOW, 0, 4 },  // SW
    { 8750,         10800,         9580,         1, 0 },  // FM
    { CB_LIMIT_LOW, 27405,         27185,        0, 4 },  // CB
    { 10800,        13700,         12100,        1, 4 },  // AIR
};

int8_t g_bandIndex = 1; // Empezar en MW por defecto

const uint16_t SWSubBands[] PROGMEM = {SW_LIMIT_LOW,1800,3500,4500,5357,5500,6500,7325,7500,9000,11000,13000,14500,16000,18100,21000,24000,24940,26000,CB_LIMIT_LOW};

volatile int8_t g_encoderCount = 0;
uint16_t g_currentFrequency = 0;
uint8_t g_savedVolume = DEFAULT_VOLUME;

bool g_showSmeterBar = true;
bool g_isEditingSetting = false;

bool g_screenOn = true;
bool g_scanning = false;
bool g_bandSelectMode = false;
bool g_usbPowered = false;

// sistema de memoria
bool g_memoryMode = false;
int8_t g_memoryIndex = 0;
MemoryChannel g_previewMemory = {0, 0, 0, 0};

bool g_bandLocked = false;
bool g_snrMode = false;
uint32_t g_lastDoublePress = 0;
