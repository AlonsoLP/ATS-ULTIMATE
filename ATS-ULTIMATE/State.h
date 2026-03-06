#pragma once
#include <Arduino.h>
#include <SI4735.h>
#include "Config.h"
#include "Utils.h"

// --- Enumeraciones de Control ---
enum SettingsIndex {
    DefaultVol, ATT, SoftMute, SVC, Sync, DeEmp, AutoVolControl,
    Brightness, SWUnits, SSM, CutoffFilter, CPUSpeed, RDS, BFO,
    UnitsSwitch, ScanSwitch, CWSwitch, CWSide, StepUnits,
    SETTINGS_MAX
};

struct EepromData {
    uint8_t  magic;
    uint8_t  version;
    uint8_t  bandIndex;
    uint8_t  currentMode;
    uint8_t  stepIndex;
    uint8_t  bwIndexSSB;
    uint8_t  bwIndexAM;
    uint8_t  bwIndexFM;
    uint8_t  volume;
    uint16_t currentFreq;
    int8_t   settings[SETTINGS_MAX];
};

enum BandTypeIdx : uint8_t {
    LW_IDX = 0,
    MW_IDX = 1,
    SW_IDX = 2,
    FM_IDX = 3
};
enum Modulations : uint8_t { AM, LSB, USB, CW, FM };

enum RDSActiveInfo : uint8_t { StationName, StationInfo, ProgramInfo };
extern RDSActiveInfo g_rdsMode;

// --- Declaraciones Externas ---
extern long      g_storeTime;
extern bool      g_ssbLoaded;
extern bool      g_fmStereo;
extern bool      g_cmdVolume;
extern bool      g_settingsActive, g_muteVolume;
extern bool      g_displayRDS;
extern bool      g_showSmeterBar;
extern bool      g_isEditingSetting;

extern SI4735    g_si4735;
extern int       g_currentBFO;
extern int8_t    g_currentMode;

// Ajustes
extern SettingsItem  g_Settings[];
extern int8_t        g_SettingSelected;
extern int8_t        g_SettingsPage;

// Anchos de Banda
extern int8_t     g_bwIndexSSB;
extern Bandwidth  g_bandwidthSSB[];
extern int8_t     g_bwIndexAM;
extern Bandwidth  g_bandwidthAM[];
extern int8_t     g_bwIndexFM;
extern const char* g_bandwidthFM[];

// Pasos de frecuencia
extern const int       g_tabStep[];
extern const uint8_t   g_amTotalSteps;
extern const uint8_t   g_ssbTotalSteps;
extern volatile int8_t g_stepIndex;
extern const int8_t    g_tabStepFM[];
//extern int8_t          g_tabStepFM[];
extern uint8_t         g_FMStepIndex;

// Bandas
extern Band           g_bandList[];
extern const uint16_t SWSubBands[];   // definido con PROGMEM en State.cpp
extern int8_t         g_bandIndex;

// Estado general
extern volatile int8_t g_encoderCount;
extern uint16_t        g_currentFrequency;
extern uint8_t         g_savedVolume;
extern bool            g_screenOn;
extern bool            g_scanning;
extern bool            g_bandSelectMode;
extern bool            g_usbPowered;

// Helpers inline
inline bool isSSB() { return g_currentMode > AM && g_currentMode < FM; }

// Control CPU
extern void applyCPUSpeed(int8_t level);
