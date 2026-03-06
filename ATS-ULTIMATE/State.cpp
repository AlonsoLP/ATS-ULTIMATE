#include "State.h"
#include "InputActions.h"

#if USE_RDS
RDSActiveInfo g_rdsMode = StationName;
#endif

// --- Variables de Estado Globales ---
long g_storeTime = 0; 
bool g_ssbLoaded = false;
bool g_fmStereo = true;
bool g_cmdVolume = false;

bool g_settingsActive = false, g_muteVolume = false;
bool g_displayRDS = false;

SI4735 g_si4735;

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
    {"UKHZ", 0, 0, 1,  Switch,     doUnitsSwitch},
    {"SCAN", 0, 0, 1,  Switch,     doScanSwitch},
    {"CW  ", 0, 0, 1,  Switch,     doCWSwitch}
};

int8_t g_SettingSelected = 0;
int8_t g_SettingsPage = 0;

// --- Configuración de Anchos de Banda ---
int8_t g_bwIndexSSB = 4;
static const char bw_ssb0[] PROGMEM = "1.2";
static const char bw_ssb1[] PROGMEM = "2.3";
static const char bw_ssb2[] PROGMEM = "3.0";
static const char bw_ssb3[] PROGMEM = "4.0";
static const char bw_ssb4[] PROGMEM = "0.5";
static const char bw_ssb5[] PROGMEM = "1.0";
Bandwidth g_bandwidthSSB[] = {
    {4, bw_ssb0}, {5, bw_ssb1}, {6, bw_ssb2}, {7, bw_ssb3}, {0, bw_ssb4}, {1, bw_ssb5}
};

int8_t g_bwIndexAM = 4;
Bandwidth g_bandwidthAM[] = {
    {4, "1.0"}, {5, "1.8"}, {6, "2.0"}, {7, "2.5"}, {0, "3.0"}, {1, "4.0"}, {2, "6.0"}
};

int8_t g_bwIndexFM = 0;
const char* g_bandwidthFM[] = { "AUTO", "110k", " 84k", " 60k", " 40k" };

// --- Pasos de Frecuencia ---
const int g_tabStep[] PROGMEM = { 1, 5, 9, 10, 50, 100, 1000, 10, 25, 50, 100, 500 };
uint8_t g_amTotalSteps = 7;
uint8_t g_ssbTotalSteps = 5;
volatile int8_t g_stepIndex = 3;

int8_t g_tabStepFM[] = { 5, 10, 100 };
uint8_t g_FMStepIndex = 1;

// --- Lista de Bandas ---
Band g_bandList[] = {
    { LW_LIMIT_LOW, 520, 300, 0, 4 },
    { 520, 1710, 1476, 3, 4 },
    { SW_LIMIT_LOW, SW_LIMIT_HIGH, SW_LIMIT_LOW, 0, 4 },
    { 8750, 10800, 9580, 1, 0 }, // FM
    { 1800,  2000,  1850, 0, 4 },  // 160m Ham
    { 3500,  4000,  3700, 0, 4 },  // 80m Ham
    { 7000,  7300,  7100, 0, 4 },  // 40m Ham
    { 14000, 14350, 14200, 0, 4 }, // 20m Ham
    { 21000, 21450, 21200, 0, 4 }, // 15m Ham
    { 28000, 29700, 28400, 0, 4 }, // 10m Ham
};

int8_t g_bandIndex = 1; // Empezar en MW por defecto

const uint16_t SWSubBands[] PROGMEM = { SW_LIMIT_LOW, 3500, 4500, 5500, 6500, 7500, 9000, 11000, 13000, 14500, 16000, 18000, 21000, 24000, 26000, CB_LIMIT_LOW };

volatile int8_t g_encoderCount = 0;
uint16_t g_currentFrequency = 0;
uint8_t g_savedVolume = DEFAULT_VOLUME;

bool g_showSmeterBar = true;
bool g_isEditingSetting = false;

bool g_screenOn = true;
bool g_scanning = false;
bool g_bandSelectMode = false;
