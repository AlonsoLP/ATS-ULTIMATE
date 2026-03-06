#include "Utils.h"
#include "State.h"
#include "Config.h"
#include <EEPROM.h>

// Layout EEPROM:
// [0]     magic byte (0xA5 = datos válidos)
// [1]     g_bandIndex
// [2]     g_currentMode
// [3]     stepIndex
// [4]     bwIndexSSB
// [5]     bwIndexAM
// [6]     bwIndexFM
// [7]     volumen (byte)
// [8..9]  g_currentFrequency (uint16_t)
// [10..10+SETTINGS_MAX-1]  params de cada setting

#define EEPROM_MAGIC     0
#define EEPROM_BAND      1
#define EEPROM_MODE      2
#define EEPROM_STEP      3
#define EEPROM_BWSSB     4
#define EEPROM_BWAM      5
#define EEPROM_BWFM      6
#define EEPROM_VOL       7
#define EEPROM_FREQ_L    8
#define EEPROM_FREQ_H    9
#define EEPROM_SETTINGS  10
#define EEPROM_VALID_KEY 0xA5

void saveState()
{
    EepromData data;
    data.magic       = EEPROM_VALID_KEY;
    data.bandIndex   = (uint8_t)g_bandIndex;
    data.currentMode = (uint8_t)g_currentMode;
    data.stepIndex   = (uint8_t)g_stepIndex;
    data.bwIndexSSB  = (uint8_t)g_bwIndexSSB;
    data.bwIndexAM   = (uint8_t)g_bwIndexAM;
    data.bwIndexFM   = (uint8_t)g_bwIndexFM;
    data.volume      = g_si4735.getVolume();
    data.currentFreq = g_currentFrequency;
    for (uint8_t i = 0; i < SETTINGS_MAX; i++)
        data.settings[i] = g_Settings[i].param;

    EEPROM.put(EEPROM_ADDR, data);
}

void loadState()
{
    EepromData data;
    EEPROM.get(EEPROM_ADDR, data);
    if (data.magic != EEPROM_VALID_KEY) return;

    g_bandIndex      = data.bandIndex;
    g_currentMode    = data.currentMode;
    g_stepIndex      = data.stepIndex;
    g_bwIndexSSB     = data.bwIndexSSB;
    g_bwIndexAM      = data.bwIndexAM;
    g_bwIndexFM      = data.bwIndexFM;
    g_currentFrequency = data.currentFreq;
    g_bandList[g_bandIndex].currentFreq = g_currentFrequency;
    for (uint8_t i = 0; i < SETTINGS_MAX; i++)
        g_Settings[i].param = data.settings[i];

    g_si4735.setVolume(data.volume);
    g_savedVolume = data.volume;
}

void resetEEPROM()
{
    EEPROM.update(EEPROM_ADDR, 0x00);  // solo invalida el magic byte
}
