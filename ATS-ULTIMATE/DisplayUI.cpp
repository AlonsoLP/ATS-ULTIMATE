#include "DisplayUI.h"
#include "State.h"
#include "Config.h"
#include "Utils.h"
#include "RadioCtrl.h"
#include "InputActions.h"

void showFrequency(bool cleanDisplay)
{
    if (g_settingsActive) return;

    char freqDisplay[12];
    if (g_bandIndex == FM_IDX)
    {
        // Formato para FM: 104.20
        dtostrf(g_currentFrequency / 100.0, 6, 2, freqDisplay);
        oledPrint(freqDisplay, 20, 2, DEFAULT_FONT);
        oledPrint("MHz", 85, 2, DEFAULT_FONT);
    }
    else
    {
        // Formato para AM/SW: 15400
        sprintf(freqDisplay, "%5u", g_currentFrequency);
        oledPrint(freqDisplay, 30, 2, DEFAULT_FONT);
        oledPrint("kHz", 85, 2, DEFAULT_FONT);
    }
}

void showStatus(bool cleanFreq)
{
    if (cleanFreq) oled.clear();
    showFrequency();
    showModulation();
    showStep();
    showBandwidth();
}

void showVolume()
{
    char buf[12];
    sprintf(buf, "VOL: %02d", g_si4735.getVolume());
    oledPrint(buf, 0, 6, DEFAULT_FONT);
}

void showModulation()
{
    const char* mods[] = {"AM", "LSB", "USB", "CW", "FM"};
    oledPrint(mods[g_currentMode], 0, 0, DEFAULT_FONT);
}

void showStep()
{
    char buf[12];
    int step = (g_bandIndex == FM_IDX) ? g_tabStepFM[g_FMStepIndex] : g_tabStep[g_stepIndex];
    sprintf(buf, "St:%d", step);
    oledPrint(buf, 80, 0, DEFAULT_FONT);
}

void showBandwidth()
{
    char buf[12];
    const char* bw;
    if (g_currentMode == FM) bw = g_bandwidthFM[g_bwIndexFM];
    else if (isSSB()) bw = g_bandwidthSSB[g_bwIndexSSB].desc;
    else bw = g_bandwidthAM[g_bwIndexAM].desc;
    
    sprintf(buf, "BW:%s", bw);
    oledPrint(buf, 0, 4, DEFAULT_FONT);
}

void showSettings()
{
    oled.clear();
    oledPrint("--- AJUSTES ---", 0, 0, DEFAULT_FONT);
    
    for (uint8_t i = 0; i < 3; i++)
    {
        uint8_t idx = (g_SettingsPage * 3) + i;
        if (idx >= SETTINGS_MAX) break;
        
        char line[17];
        // Imprime Nombre: Valor actual
        sprintf(line, "%s: %d", g_Settings[idx].name, g_Settings[idx].param);
        oledPrint(line, 0, (i + 1) * 2, DEFAULT_FONT, (g_SettingSelected == idx));
    }
}

void showSMeter()
{
    g_si4735.getCurrentReceivedSignalQuality();
    int rssi = g_si4735.getCurrentRSSI();
    char bar[17];
    // Mapeo de RSSI (0-64) a 16 caracteres de la pantalla
    int level = map(rssi, 0, 64, 0, 16);
    
    for(int i=0; i<16; i++) {
        bar[i] = (i < level) ? '|' : ' ';
    }
    bar[16] = '\0';
    
    oledPrint(bar, 0, 6, DEFAULT_FONT);
}
