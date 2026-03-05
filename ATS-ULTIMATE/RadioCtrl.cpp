#include "RadioCtrl.h"
#include "State.h"
#include "Config.h"
#include "DisplayUI.h"
#include "patch_ssb_compressed.h"

int getSteps()
{
    if (isSSB())
    {
        if (g_stepIndex >= g_amTotalSteps)
            return (int)pgm_read_word(&g_tabStep[g_stepIndex]);

        return (int)pgm_read_word(&g_tabStep[g_stepIndex]) * 1000;
    }

    if (g_stepIndex >= g_amTotalSteps)
        g_stepIndex = 0;

    return (int)pgm_read_word(&g_tabStep[g_stepIndex]);
}

int getLastStep()
{
    if (isSSB())
        return g_amTotalSteps + g_ssbTotalSteps - 1;

    return g_amTotalSteps - 1;
}

void updateSSBCutoffFilter()
{
    // Acceso mediante SettingsIndex unificado para evitar errores de declaración
    if (g_Settings[SettingsIndex::CutoffFilter].param == 0 || g_currentMode == CW)
        g_si4735.setSSBSidebandCutoffFilter(0); 
    else
        g_si4735.setSSBSidebandCutoffFilter(1);
}

static void bandSwitch(bool up)
{
    g_scanning = false;
    if (up) {
        if (g_bandIndex < g_lastBand) g_bandIndex++;
        else g_bandIndex = 0;
    } else {
        if (g_bandIndex > 0) g_bandIndex--;
        else g_bandIndex = g_lastBand;
    }
    g_currentBFO = 0;
    oledPrint(F("                "), 0, 6, DEFAULT_FONT);
    applyBandConfiguration();
}

void applyBandConfiguration()
{
    // validar ATT según modo
    if (g_bandIndex == FM_IDX && g_Settings[ATT].param > 26) {
        g_Settings[ATT].param = 26;
    }
    agcSetFunc(); // aplicar AGC con el valor ya validado

    g_currentFrequency = g_bandList[g_bandIndex].currentFreq;
    if (g_bandIndex == FM_IDX) {
        g_si4735.setFM(g_bandList[g_bandIndex].minimumFreq, g_bandList[g_bandIndex].maximumFreq, g_currentFrequency, g_tabStepFM[g_FMStepIndex]);
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

void agcSetFunc()
{
    uint8_t currentAtt = g_Settings[SettingsIndex::ATT].param;
    if (currentAtt == 0) g_si4735.setAutomaticGainControl(1, 0);
    else g_si4735.setAutomaticGainControl(0, currentAtt);
}

void doBand(int8_t v)
{
    bandSwitch(v > 0); // Si v es 1 sube, si es -1 baja
}

static void loadSSBPatch()
{
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
}

void doMode(int8_t v) {
    // En FM no se puede cambiar de modo
    if (g_bandIndex == FM_IDX) return;

    // Ciclo: AM -> LSB -> USB -> CW -> AM
    static const uint8_t s_modeOrder[] PROGMEM = { AM, LSB, USB, CW };
    const uint8_t modeCount = 4;

    // Encontrar índice actual en modeOrder
    uint8_t idx = 0;
    for (uint8_t i = 0; i < modeCount; i++) {
	if ((Modulations)pgm_read_byte(&s_modeOrder[i]) == (Modulations)g_currentMode) {
    	    idx = i; break;
    	}
    }
    idx = (idx + modeCount + v) % modeCount;
    g_currentMode = (int8_t)pgm_read_byte(&s_modeOrder[idx]);

    g_currentBFO = 0;

    if (isSSB()) {
        loadSSBPatch();
        g_si4735.setSSB(
            g_bandList[g_bandIndex].minimumFreq,
            g_bandList[g_bandIndex].maximumFreq,
            g_currentFrequency,
            (int)pgm_read_word(&g_tabStep[g_stepIndex]),
            g_currentMode == LSB ? 1 : 2   // 1=LSB, 2=USB (CW usa LSB)
        );
        updateBFO(); // 0
        updateSSBCutoffFilter();
    } else {
        g_ssbLoaded = false;
        applyBandConfiguration();
    }
    showStatus(true);
}

void doBandwidth(int8_t v) {
    if (g_currentMode == FM) {
        g_bwIndexFM += v;
        if (g_bwIndexFM < 0) g_bwIndexFM = 4;
        if (g_bwIndexFM > 4) g_bwIndexFM = 0;
        g_si4735.setFmBandwidth(g_bwIndexFM);
    } else if (isSSB()) {
        g_bwIndexSSB += v;
        if (g_bwIndexSSB < 0) g_bwIndexSSB = (int8_t)g_bwSSBMaxIdx;
        if (g_bwIndexSSB > (int8_t)g_bwSSBMaxIdx) g_bwIndexSSB = 0;
        g_bandList[g_bandIndex].bandwidthIdx = g_bwIndexSSB;
        g_si4735.setSSBAudioBandwidth(g_bandwidthSSB[g_bwIndexSSB].idx);
        updateSSBCutoffFilter();
    } else {
        // AM/LW/MW/SW
        g_bwIndexAM += v;
        if (g_bwIndexAM < 0) g_bwIndexAM = (int8_t)g_maxFilterAM;
        if (g_bwIndexAM > (int8_t)g_maxFilterAM) g_bwIndexAM = 0;
        g_bandList[g_bandIndex].bandwidthIdx = g_bwIndexAM;
        g_si4735.setBandwidth(g_bandwidthAM[g_bwIndexAM].idx, 1);
    }
    showBandwidth();
}

void doScan() {
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

static int8_t g_swSubBandIndex = 0;

void doSWSubBand(int8_t v) {
    g_swSubBandIndex += v;
    if (g_swSubBandIndex < 0) g_swSubBandIndex = (int8_t)g_SWSubBandCount - 1;
    if (g_swSubBandIndex >= (int8_t)g_SWSubBandCount) g_swSubBandIndex = 0;
    g_currentFrequency = pgm_read_word(&SWSubBands[g_swSubBandIndex]);
    g_bandList[SW_IDX].currentFreq = g_currentFrequency;
    g_si4735.setFrequency(g_currentFrequency);
    showFrequency(false);
}
