#include "DisplayUI.h"
#include "State.h"
#include "Config.h"
#include "Utils.h"
#include "RadioCtrl.h"
#include "InputActions.h"
#include "font14x24sevenSeg.h"

void oledPrint(const char* text, uint8_t x, uint8_t y, const DCfont* font = DEFAULT_FONT, bool invert = false) {
    oled.setFont(font);
    oled.setCursor(x, y);
    if (invert) {
        // Lógica de inversión si tu librería la soporta, o simplemente:
    }
    oled.print(text);
}

void showFrequency(bool cleanDisplay)
{
    if (g_settingsActive) return;

    char freqDisplay[12];
    
    // Si se nos pide limpiar (ej. al cambiar de banda), borramos solo el área numéricapara evitar parpadeos bruscos
    if (cleanDisplay) {
        oledPrint("        ", 15, 2, FONT14X24SEVENSEG);
    }

    if (g_bandIndex == FM_IDX)
    {
        // Optimización CPU: Adiós al float. Calculamos los MHz y los decimales por separado
        int mhz = g_currentFrequency / 100;
        int decimales = g_currentFrequency % 100;
        
        sprintf(freqDisplay, "%3d.%02d", mhz, decimales);
        oledPrint(freqDisplay, 15, 2, FONT14X24SEVENSEG); 
        oledPrint("MHz", 102, 4, DEFAULT_FONT);
    }
    else
    {
        // Optimización CPU para AM/SW y formateo con separación de miles
        int miles = g_currentFrequency / 1000;
        int unidades = g_currentFrequency % 1000;

        if (miles > 0) {
            sprintf(freqDisplay, "%2d %03d", miles, unidades);
        } else {
            sprintf(freqDisplay, "   %3d", unidades);
        }
        
        oledPrint(freqDisplay, 15, 2, FONT14X24SEVENSEG);
        oledPrint("kHz", 102, 4, DEFAULT_FONT);
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
