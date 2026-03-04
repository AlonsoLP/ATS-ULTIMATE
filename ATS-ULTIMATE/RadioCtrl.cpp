#include "RadioCtrl.h"
#include "State.h"
#include "Config.h"
#include "DisplayUI.h"
#include "patch_ssb_compressed.h"

bool isSSB()
{
    return g_currentMode > AM && g_currentMode < FM;
}

int getSteps()
{
    if (isSSB())
    {
        if (g_stepIndex >= g_amTotalSteps)
            return g_tabStep[g_stepIndex];

        return g_tabStep[g_stepIndex] * 1000;
    }

    if (g_stepIndex >= g_amTotalSteps)
        g_stepIndex = 0;

    return g_tabStep[g_stepIndex];
}

int getLastStep()
{
    if (isSSB())
        return g_amTotalSteps + g_ssbTotalSteps - 1;

    return g_amTotalSteps - 1;
}

void resetEepromDelay()
{
    g_storeTime = millis();
    g_previousFrequency = 0;
}

void updateSSBCutoffFilter()
{
    // Acceso mediante SettingsIndex unificado para evitar errores de declaración
    if (g_Settings[SettingsIndex::CutoffFilter].param == 0 || g_currentMode == CW)
        g_si4735.setSSBSidebandCutoffFilter(0); 
    else
        g_si4735.setSSBSidebandCutoffFilter(1);
}

void bandSwitch(bool up)
{
    if (up)
    {
        if (g_bandIndex < g_lastBand) g_bandIndex++;
        else g_bandIndex = 0;
    }
    else
    {
        if (g_bandIndex > 0) g_bandIndex--;
        else g_bandIndex = g_lastBand;
    }

    g_currentBFO = 0;
    // Limpieza de línea de estado usando la literal compartida definida en State.cpp
    oledPrint(_literal_EmptyLine, 0, 6, DEFAULT_FONT);
    
    applyBandConfiguration();
}

void applyBandConfiguration(bool extraSSBReset)
{
    g_currentFrequency = g_bandList[g_bandIndex].currentFreq;
    
    if (g_bandIndex == FM_IDX) {
        g_si4735.setFM(g_bandList[g_bandIndex].minimumFreq, g_bandList[g_bandIndex].maximumFreq, g_currentFrequency, g_tabStepFM[g_FMStepIndex]);
    } else {
        g_si4735.setAM(g_bandList[g_bandIndex].minimumFreq, g_bandList[g_bandIndex].maximumFreq, g_currentFrequency, g_tabStep[g_stepIndex]);
    }
    
    showStatus(true);
}

void updateBFO()
{
    if (isSSB()) {
        g_si4735.setSSBBfo(g_currentBFO);
    }
}

void agcSetFunc()
{
    uint8_t currentAtt = g_Settings[SettingsIndex::ATT].param;
    if (currentAtt == 0) g_si4735.setAutomaticGainControl(1, 0);
    else g_si4735.setAutomaticGainControl(0, currentAtt);
}

void doBand(int8_t v) {
    bandSwitch(v > 0); // Si v es 1 sube, si es -1 baja
}

void doMode(int8_t v) {
    // Aquí implementas el cambio de modo (AM/LSB/USB/CW/FM)
    // Por ahora puedes dejarlo vacío para que compile
}

void doBandwidth(int8_t v) {
    // Aquí implementas el cambio de ancho de banda
    // Por ahora puedes dejarlo vacío para que compile
}
