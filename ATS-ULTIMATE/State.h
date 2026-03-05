#pragma once
#include <Arduino.h>
#include <SI4735.h>
#include "Config.h"
#include "Utils.h"

// --- Enumeraciones de Control ---
enum SettingsIndex {
    ATT, SoftMute, SVC, Sync, DeEmp, AutoVolControl, Brightness,
    SWUnits, SSM, CutoffFilter, CPUSpeed,
#if USE_RDS
    RDSError,
#endif
    BFO, UnitsSwitch, ScanSwitch, CWSwitch, SETTINGS_MAX
};

// Usamos nombres únicos para evitar conflictos con Config.h
enum BandTypeIdx : uint8_t { 
    LW_IDX = 0, 
    MW_IDX = 1, 
    SW_IDX = 2, 
    FM_IDX = 3 
};
enum Modulations : uint8_t { AM, LSB, USB, CW, FM };

#if USE_RDS
enum RDSActiveInfo : uint8_t { StationName, StationInfo, ProgramInfo };
extern RDSActiveInfo g_rdsMode;
#endif

// --- Declaraciones Externas (Variables en State.cpp) ---
extern long g_storeTime;
extern bool g_ssbLoaded;
extern bool g_fmStereo;
extern bool g_cmdVolume;
extern bool g_settingsActive, g_muteVolume;
extern bool g_displayRDS;
extern bool g_showSmeterBar;
extern bool g_isEditingSetting;

extern SI4735 g_si4735;
extern int g_currentBFO;
extern int8_t g_currentMode;

// Ajustes
extern SettingsItem g_Settings[];
extern const uint8_t g_SettingsMaxPages;
extern int8_t g_SettingSelected;
extern int8_t g_SettingsPage;
extern bool g_SettingEditing;

// Filtros y Anchos de Banda
extern int8_t g_bwIndexSSB;
extern Bandwidth g_bandwidthSSB[];
extern const uint8_t g_bwSSBMaxIdx;

extern int8_t g_bwIndexAM;
extern Bandwidth g_bandwidthAM[];
extern const uint8_t g_maxFilterAM;

extern int8_t g_bwIndexFM;
extern char* g_bandwidthFM[];

// Pasos de frecuencia
extern const int g_tabStep[] PROGMEM;  // o mejor: mover a Config.h como constante
extern uint8_t g_amTotalSteps;
extern uint8_t g_ssbTotalSteps;
extern volatile int8_t g_stepIndex;

extern int8_t g_tabStepFM[];
extern int8_t g_FMStepIndex;
extern const int8_t g_lastStepFM;

// Bandas
extern Band g_bandList[];
extern const uint16_t SWSubBands[] PROGMEM;
extern const uint8_t g_SWSubBandCount;
extern const uint8_t g_lastBand;
extern int8_t g_bandIndex;

extern volatile int8_t g_encoderCount;
extern uint16_t g_currentFrequency;
extern uint8_t g_savedVolume;

extern bool g_screenOn;
extern bool g_scanning;
extern bool g_bandSelectMode;

inline bool isSSB() { return g_currentMode > AM && g_currentMode < FM; }
