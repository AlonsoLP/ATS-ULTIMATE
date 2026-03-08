/**
 * @file    RadioCtrl.cpp
 * @brief   Lógica de control SI4735: cambio de banda, modo, paso, SSB patch.
 * @author  Alonso José Lara Plana (EA7LBT)
 * @license MIT — ver ATS-ULTIMATE.ino para texto completo
 */
#include "RadioCtrl.h"
#include <Wire.h>
#include "State.h"
#include "Config.h"
#include "DisplayUI.h"
#include "patch_ssb_compressed.h"

void applyRadioState()
{
    if (g_currentMode == FM) {
        applyBandConfiguration(); 
        g_si4735.setFmBandwidth(g_bwIndexFM);
    } else if (isSSB()) {
	if (!g_ssbLoaded) {
    	    g_si4735.queryLibraryId();
    	    g_si4735.patchPowerUp();
	    g_si4735.downloadCompressedPatch(ssb_patch_content, sizeof(ssb_patch_content), cmd_0x15, sizeof(cmd_0x15) / sizeof(cmd_0x15[0]));
    	    g_si4735.setSSBConfig(
        	g_bandwidthSSB[g_bwIndexSSB].idx, // BW
        	1,   // audioBW
        	0,   // ssbaudioBWsnrBoost
        	0,   // ssbFilterPower
        	1,   // ssbSBUFIntegRate
        	0    // reserved
    	    );
    	    g_ssbLoaded = true;
	}

        g_si4735.setSSB(
            g_bandList[g_bandIndex].minimumFreq,
            g_bandList[g_bandIndex].maximumFreq,
            g_currentFrequency,
            (int)pgm_read_word(&g_tabStep[g_stepIndex]),
            g_currentMode == LSB ? 1 : (g_currentMode == CW ? (g_Settings[CWSide].param == 0 ? 1 : 2) : 2)
        );
        g_si4735.setSSBAudioBandwidth(g_bandwidthSSB[g_bwIndexSSB].idx);
        g_si4735.setSSBBfo(g_currentBFO);
        updateSSBCutoffFilter();
        showStatus(true);
    } else {
        g_ssbLoaded = false;
        applyBandConfiguration();
        g_si4735.setBandwidth(g_bandwidthAM[g_bwIndexAM].idx, 1);
    }
}

void applyMemoryState(MemoryChannel& mem)
{
    if (mem.bandIdx > LAST_BAND || mem.mode > FM) return;

    g_bandIndex = mem.bandIdx;
    g_currentFrequency = mem.freq;
    g_bandList[g_bandIndex].currentFreq = g_currentFrequency;
    g_currentMode = mem.mode;
    
    if (g_currentMode == FM) g_bwIndexFM = mem.bwIdx;
    else if (isSSB()) g_bwIndexSSB = mem.bwIdx;
    else g_bwIndexAM = mem.bwIdx;
    
    g_currentBFO = 0;
    applyRadioState();
}

int getSteps()
{
    if (isSSB())
    {
        if (g_stepIndex >= AM_TOTAL_STEPS)
            return (int)pgm_read_word(&g_tabStep[g_stepIndex]);

        return (int)pgm_read_word(&g_tabStep[g_stepIndex]) * 1000;
    }

    if (g_stepIndex >= AM_TOTAL_STEPS)
        g_stepIndex = 0;

    return (int)pgm_read_word(&g_tabStep[g_stepIndex]);
}

void updateSSBCutoffFilter()
{
    // Acceso mediante SettingsIndex unificado para evitar errores de declaración
    if (g_Settings[SettingsIndex::CutoffFilter].param == 0 || g_currentMode == CW)
        g_si4735.setSSBSidebandCutoffFilter(0); 
    else
        g_si4735.setSSBSidebandCutoffFilter(1);
}

void applyBandConfiguration()
{
    // validar ATT según modo
    if (g_bandIndex == FM_IDX && g_Settings[ATT].param > 26) {
        g_Settings[ATT].param = 26;
    }

    // aplicar AGC con el valor ya validado
    uint8_t currentAtt = g_Settings[SettingsIndex::ATT].param;
    if (currentAtt == 0) g_si4735.setAutomaticGainControl(1, 0);
    else g_si4735.setAutomaticGainControl(0, currentAtt);

    g_currentFrequency = g_bandList[g_bandIndex].currentFreq;
    if (g_bandIndex == FM_IDX) {
        g_si4735.setFM(g_bandList[g_bandIndex].minimumFreq, g_bandList[g_bandIndex].maximumFreq, g_currentFrequency, pgm_read_byte(&g_tabStepFM[g_FMStepIndex]));
    } else {
        g_si4735.setAM(g_bandList[g_bandIndex].minimumFreq, g_bandList[g_bandIndex].maximumFreq, g_currentFrequency, (int)pgm_read_word(&g_tabStep[g_stepIndex]));
    }
    showStatus(true);
}

void updateBFO()
{
    if (isSSB()) {
        int bfo = g_currentBFO;
        if (g_currentMode == CW) bfo += CW_PITCH_OFFSET_HZ;
        g_si4735.setSSBBfo(bfo);
    }
}

void doBand(int8_t v)
{
    g_scanning = false;
    if (v > 0) {
        if (g_bandIndex < LAST_BAND) g_bandIndex++;
        else g_bandIndex = 0;
    } else {
        if (g_bandIndex > 0) g_bandIndex--;
        else g_bandIndex = LAST_BAND;
    }
    g_currentBFO = 0;
    applyBandConfiguration();
}

void doMode(int8_t v)
{
    if (g_bandIndex == FM_IDX || g_bandIndex == AIR_IDX) return;

    static const uint8_t s_modeOrder[] PROGMEM = { AM, LSB, USB, CW };
    const uint8_t modeCount = g_Settings[CWSwitch].param ? 4 : 3;

    if (g_currentMode == CW && modeCount == 3) g_currentMode = AM;

    uint8_t idx = 0;
    for (uint8_t i = 0; i < modeCount; i++) {
        if ((Modulations)pgm_read_byte(&s_modeOrder[i]) == (Modulations)g_currentMode) {
            idx = i; break;
        }
    }
    idx = (idx + modeCount + v) % modeCount;
    g_currentMode = (int8_t)pgm_read_byte(&s_modeOrder[idx]);

    g_currentBFO = 0;
    applyRadioState();
}

static inline void wrapIndex(int8_t &idx, int8_t v, int8_t maxIdx)
{
    idx += v;
    if (idx < 0)      idx = maxIdx;
    if (idx > maxIdx) idx = 0;
}

void doBandwidth(int8_t v)
{
    if (g_currentMode == FM) {
        wrapIndex(g_bwIndexFM, v, 4);
        g_si4735.setFmBandwidth(g_bwIndexFM);
    } else if (isSSB()) {
        wrapIndex(g_bwIndexSSB, v, (int8_t)BW_SSB_MAX);
        g_bandList[g_bandIndex].bandwidthIdx = g_bwIndexSSB;
        g_si4735.setSSBAudioBandwidth(g_bandwidthSSB[g_bwIndexSSB].idx);
        updateSSBCutoffFilter();
    } else {
        wrapIndex(g_bwIndexAM, v, (int8_t)BW_AM_MAX);
        g_bandList[g_bandIndex].bandwidthIdx = g_bwIndexAM;
        g_si4735.setBandwidth(g_bandwidthAM[g_bwIndexAM].idx, 1);
    }
    showBandwidth();
}

void doScan()
{
    if (!g_scanning) return;
    g_si4735.seekNextStation();
    delay(100);  // dar tiempo al chip
    uint16_t newFreq = g_si4735.getCurrentFrequency();
    if (newFreq != g_currentFrequency) {
        g_currentFrequency = newFreq;
        g_bandList[g_bandIndex].currentFreq = g_currentFrequency;
        showFrequency(false);
        g_scanning = false;  // seek encontró estación → parar
        showStatus(false);
    }
}

void setRDSConfig(uint8_t bias)
{
    g_si4735.setRdsConfig(1, bias, bias, bias, bias);
    // 1 = RDS habilitado, bias = umbral de error en cada bloque
}

static int8_t g_swSubBandIndex = 0;

void doSWSubBand(int8_t v)
{
    g_swSubBandIndex += v;
    if (g_swSubBandIndex < 0) g_swSubBandIndex = SW_SUBBAND_COUNT - 1;
    if (g_swSubBandIndex >= SW_SUBBAND_COUNT) g_swSubBandIndex = 0;
    g_currentFrequency = pgm_read_word(&SWSubBands[g_swSubBandIndex]);
    g_bandList[SW_IDX].currentFreq = g_currentFrequency;
    g_si4735.setFrequency(g_currentFrequency);
    showFrequency(false);
}

void checkUSBPower()
{
#ifdef USE_BATTERY_INDICATOR
    static uint32_t lastUsbCheck = 0;
    uint32_t now = millis();
    if (now - lastUsbCheck <= 2000) return;
    lastUsbCheck = now;

    bool wasUsb = g_usbPowered;
    g_usbPowered = (analogRead(BATTERY_PIN) > USB_DETECT_THRESHOLD);

    if (g_usbPowered && !wasUsb)
        applyCPUSpeed(0);
    else if (!g_usbPowered && wasUsb)
        applyCPUSpeed(g_Settings[CPUSpeed].param);
#endif
}

void applyCPUSpeed(int8_t level)
{
    static const uint32_t i2cClk[] = {400000UL, 200000UL, 100000UL, 100000UL};
    static const uint8_t  clkpr[]  = {0x00, 0x01, 0x02, 0x03};
    if (level > 0) Wire.setClock(i2cClk[level]);
    cli();
    CLKPR = (1 << CLKPCE);
    CLKPR = clkpr[level];
    sei();
    if (level == 0) Wire.setClock(i2cClk[level]);
}
