#include "DisplayUI.h"
#include "State.h"
#include "Config.h"
#include "font14x24sevenSeg.h"

#ifdef USE_BATTERY_INDICATOR
void showBattery() {
    uint16_t raw = analogRead(BATTERY_PIN);
    // Divisor 100k/100k → VCC máx ~8.4V → en 5V ref → escala
    uint8_t pct = map(raw, 614, 818, 0, 100); // ~3.0V–4.2V * 2
    pct = constrain(pct, 0, 100);
    char buf[6];
    itoa(pct, buf, 10);
    strcat(buf, "%");
    oledPrint(buf, 100, 0, DEFAULT_FONT);
}
#endif

void oledPrint(const char* text, uint8_t x, uint8_t y, const DCfont* font = DEFAULT_FONT, bool invert = false)
{
    oled.setFont(font);
    oled.setCursor(x, y);
    if (invert) oled.invertOutput(true);
    oled.print(text);
    if (invert) oled.invertOutput(false);
}

void oledPrint(const __FlashStringHelper* text, uint8_t x, uint8_t y, const DCfont* font, bool invert)
{
    oled.setFont(font);
    oled.setCursor(x, y);
    if (invert) oled.invertOutput(true);
    oled.print(text);
    if (invert) oled.invertOutput(false);
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
        
	itoa(mhz, freqDisplay, 10);
	uint8_t len = strlen(freqDisplay);
	while (len < 3) { memmove(freqDisplay+1, freqDisplay, len+1); freqDisplay[0]=' '; len++; }
	freqDisplay[len] = '.';
	freqDisplay[len+1] = '0' + decimales / 10;
	freqDisplay[len+2] = '0' + decimales % 10;
	freqDisplay[len+3] = '\0';

        oledPrint(freqDisplay, 15, 2, FONT14X24SEVENSEG); 
        oledPrint("MHz", 102, 5, DEFAULT_FONT);
    } else {
        // Optimización CPU para AM/SW y formateo con separación de miles
        int miles = g_currentFrequency / 1000;
        int unidades = g_currentFrequency % 1000;

        if (miles > 0) {
	    freqDisplay[0] = (miles < 10) ? ' ' : ('0' + miles / 10);
	    freqDisplay[1] = '0' + miles % 10;
	    freqDisplay[2] = ' ';
	    freqDisplay[3] = '0' + unidades / 100;
	    freqDisplay[4] = '0' + (unidades % 100) / 10;
	    freqDisplay[5] = '0' + unidades % 10;
	    freqDisplay[6] = '\0';
        } else {
	    char freqDisplay[12] = "   ";
	    freqDisplay[3] = (unidades >= 100) ? ('0' + unidades / 100) : ' ';
	    freqDisplay[4] = (unidades >= 10)  ? ('0' + (unidades % 100) / 10) : ' ';
	    freqDisplay[5] = '0' + unidades % 10;
	    freqDisplay[6] = '\0';
        }
        
        oledPrint(freqDisplay, 15, 2, FONT14X24SEVENSEG);
        oledPrint("kHz", 102, 5, DEFAULT_FONT);
    }
}

void showStatus(bool cleanFreq)
{
    if (cleanFreq) oled.clear();
    if (!cleanFreq) {
	oledPrint("          ", 0, 5, DEFAULT_FONT); // Limpiar zona BW
    }
    showFrequency();
    showModulation();
    showStep();
    showBandwidth();
}

void showVolume()
{
    uint8_t vol = g_si4735.getVolume();
    char buf[8] = "VOL: ";
    buf[5] = '0' + vol / 10;
    buf[6] = '0' + vol % 10;
    buf[7] = '\0';
    oledPrint(buf, 0, 7, DEFAULT_FONT);
}

static const char* getSWBandName(uint16_t freq) {
    if (freq >= 1800  && freq <= 2000)  return "160m";
    if (freq >= 3500  && freq <= 4000)  return " 80m";
    if (freq >= 7000  && freq <= 7300)  return " 40m";
    if (freq >= 14000 && freq <= 14350) return " 20m";
    if (freq >= 21000 && freq <= 21450) return " 15m";
    if (freq >= 28000 && freq <= 29700) return " 10m";
    if (freq >= 3900  && freq <= 6200)  return " 75m";
    if (freq >= 5900  && freq <= 6200)  return " 49m";
    if (freq >= 7200  && freq <= 7450)  return " 41m";
    if (freq >= 9400  && freq <= 9900)  return " 31m";
    if (freq >= 11600 && freq <= 12100) return " 25m";
    if (freq >= 13570 && freq <= 13870) return " 22m";
    if (freq >= 15100 && freq <= 15800) return " 19m";
    if (freq >= 17480 && freq <= 17900) return " 16m";
    return "  SW";
}

void showModulation()
{
    const char* label;
    if (g_bandIndex == SW_IDX && g_currentMode == AM) {
        label = getSWBandName(g_currentFrequency);
    } else if (g_currentMode == FM) {
        g_si4735.getCurrentReceivedSignalQuality();
        g_fmStereo = g_si4735.getCurrentPilot();
        label = g_fmStereo ? "FM ST" : "FM   ";
    } else {
        static const char* mods[] = {"AM", "LSB", "USB", "CW"};
        label = mods[g_currentMode];
    }
    oledPrint(label, 0, 0, DEFAULT_FONT);
}

void showStep()
{
    char buf[10] = "St:";
    int step = (g_bandIndex == FM_IDX) ? g_tabStepFM[g_FMStepIndex] : pgm_read_word(g_tabStep[g_stepIndex]);
    itoa(step, buf + 3, 10);
    oledPrint(buf, 80, 0, DEFAULT_FONT);
}

void showBandwidth()
{
    char buf[10] = "BW:";
    const char* bw;

    if (g_currentMode == FM) bw = g_bandwidthFM[g_bwIndexFM];
    else if (isSSB()) bw = g_bandwidthSSB[g_bwIndexSSB].desc;
    else bw = g_bandwidthAM[g_bwIndexAM].desc;

    strncpy(buf + 3, bw, 6);
    oledPrint(buf, 0, 5, DEFAULT_FONT);
}

void showSettings()
{
    oled.clear();
    oledPrint(F("--- AJUSTES ---"), 0, 0, DEFAULT_FONT);
    
    for (uint8_t i = 0; i < 3; i++)
    {
        uint8_t idx = (g_SettingsPage * 3) + i;
        if (idx >= SETTINGS_MAX) break;
        
        char line[17];
	strncpy(line, g_Settings[idx].name, 4);
	line[4] = ':'; line[5] = ' ';
	itoa(g_Settings[idx].param, line + 6, 10);
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
    
    oledPrint(bar, 0, 7, DEFAULT_FONT);
}

void showBandTag() {
    static const char* const bandNames[] = { "LW", "MW", "SW", "FM" };
    char buf[17] = "BAND: ";

    // Copiar nombre de banda y rellenar con espacios hasta 4 chars (%-4s)
    const char* name = bandNames[g_bandIndex];
    uint8_t i = 0;
    while (name[i]) { buf[6 + i] = name[i]; i++; }
    while (i < 4)   { buf[6 + i] = ' '; i++; }  // padding hasta 4

    buf[10] = ' ';

    // "%3d" para g_bandIndex+1 — siempre 1 dígito (max 4 bandas)
    buf[11] = ' ';
    buf[12] = ' ';
    buf[13] = '0' + (g_bandIndex + 1);

    buf[14] = '/';

    // g_lastBand+1 — también 1 dígito
    buf[15] = '0' + (g_lastBand + 1);
    buf[16] = '\0';

    oledPrint(buf, 0, 0, DEFAULT_FONT);
}

#if USE_RDS
void showRDS()
{
    if (!g_displayRDS) {
        oledPrint("                ", 0, 6, DEFAULT_FONT);
        return;
    }

    char rdsLine[17];
    memset(rdsLine, ' ', 16);
    rdsLine[16] = '\0';

    if (!g_si4735.getRdsReceived()) {
        oledPrint("...", 0, 6, DEFAULT_FONT);
        return;
    }

    char* txt = nullptr;
    switch (g_rdsMode) {
        case StationName:  txt = g_si4735.getRdsText0A(); break;
        case StationInfo:  txt = g_si4735.getRdsText2A(); break;
        case ProgramInfo:  txt = g_si4735.getRdsText2B(); break;
    }

    if (txt != nullptr) strncpy(rdsLine, txt, 16);
    rdsLine[16] = '\0';
    oledPrint(rdsLine, 0, 6, DEFAULT_FONT);
}
#endif
