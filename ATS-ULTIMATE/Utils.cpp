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

void saveState() {
    EEPROM.update(EEPROM_MAGIC,  EEPROM_VALID_KEY);
    EEPROM.update(EEPROM_VERSION, EEPROM_FW_VERSION);
    EEPROM.update(EEPROM_BAND,   (uint8_t)g_bandIndex);
    EEPROM.update(EEPROM_MODE,   (uint8_t)g_currentMode);
    EEPROM.update(EEPROM_STEP,   (uint8_t)g_stepIndex);
    EEPROM.update(EEPROM_BWSSB,  (uint8_t)g_bwIndexSSB);
    EEPROM.update(EEPROM_BWAM,   (uint8_t)g_bwIndexAM);
    EEPROM.update(EEPROM_BWFM,   (uint8_t)g_bwIndexFM);
    EEPROM.update(EEPROM_VOL,    g_si4735.getVolume());
    EEPROM.update(EEPROM_FREQ_L, (uint8_t)(g_currentFrequency & 0xFF));
    EEPROM.update(EEPROM_FREQ_H, (uint8_t)(g_currentFrequency >> 8));
    for (uint8_t i = 0; i < SETTINGS_MAX; i++) {
        EEPROM.update(EEPROM_SETTINGS + i, (uint8_t)g_Settings[i].param);
    }
}

void resetEEPROM() {
    EEPROM.update(EEPROM_MAGIC, 0x00); // Invalidar magic byte
}

void loadState() {
    if (EEPROM.read(EEPROM_MAGIC)   != EEPROM_VALID_KEY ||
        EEPROM.read(EEPROM_VERSION) != EEPROM_FW_VERSION) {
        // Estructura EEPROM incompatible con este firmware → reset seguro
        resetEEPROM();
        EEPROM.update(EEPROM_VERSION, EEPROM_FW_VERSION);
        return; // salir: se usarán valores por defecto de State.cpp
    }

    g_bandIndex      = EEPROM.read(EEPROM_BAND);
    g_currentMode    = EEPROM.read(EEPROM_MODE);
    g_stepIndex      = EEPROM.read(EEPROM_STEP);
    g_bwIndexSSB     = EEPROM.read(EEPROM_BWSSB);
    g_bwIndexAM      = EEPROM.read(EEPROM_BWAM);
    g_bwIndexFM      = EEPROM.read(EEPROM_BWFM);
    g_savedVolume    = EEPROM.read(EEPROM_VOL);
    g_currentFrequency = EEPROM.read(EEPROM_FREQ_L) |
                         ((uint16_t)EEPROM.read(EEPROM_FREQ_H) << 8);
    for (uint8_t i = 0; i < SETTINGS_MAX; i++) {
        g_Settings[i].param = (int8_t)EEPROM.read(EEPROM_SETTINGS + i);
    }
    g_bandList[g_bandIndex].currentFreq = g_currentFrequency;
}
