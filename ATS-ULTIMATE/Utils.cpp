/**
 * @file    Utils.cpp
 * @brief   Implementación de funciones utilitarias compartidas.
 * @author  Alonso José Lara Plana (EA7LBT)
 * @license MIT — ver ATS-ULTIMATE.ino para texto completo
 */
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
    data.memoryIndex = (uint8_t)g_memoryIndex;
    data.fmStepIndex = g_FMStepIndex;
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
    if (g_bandIndex > LAST_BAND) g_bandIndex = 1;  // fallback MW

    g_currentMode    = data.currentMode;
    if (g_currentMode > FM) g_currentMode = AM;    // fallback AM

    g_stepIndex      = data.stepIndex;
    if (g_stepIndex >= AM_TOTAL_STEPS + SSB_TOTAL_STEPS) g_stepIndex = 3;  // fallback 10kHz

    g_bwIndexSSB     = data.bwIndexSSB;
    if (g_bwIndexSSB > BW_SSB_MAX) g_bwIndexSSB = 4;  // fallback 0.5kHz

    g_bwIndexAM      = data.bwIndexAM;
    if (g_bwIndexAM > BW_AM_MAX) g_bwIndexAM = 4;     // fallback 3.0kHz

    g_bwIndexFM      = data.bwIndexFM;
    if (g_bwIndexFM > FM_LAST_STEP) g_bwIndexFM = 0;  // fallback AUTO

    g_memoryIndex = (int8_t)data.memoryIndex;
    if (g_memoryIndex >= MAX_MEMORIES) g_memoryIndex = 0;  // sanity check

    g_FMStepIndex = data.fmStepIndex;
    if (g_FMStepIndex >= FM_TOTAL_STEPS) g_FMStepIndex = 1;

    // los callbacks los acotan al usarlos
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

// sistema de memorias
void saveMemory(uint8_t index, MemoryChannel& mem)
{
    EEPROM.put(EEPROM_MEM_OFFSET + (index * sizeof(MemoryChannel)), mem);
}

void loadMemory(uint8_t index, MemoryChannel& mem)
{
    EEPROM.get(EEPROM_MEM_OFFSET + (index * sizeof(MemoryChannel)), mem);
}
