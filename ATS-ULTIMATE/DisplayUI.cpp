/**
 * @file    DisplayUI.cpp
 * @brief   Funciones de renderizado UI: frecuencia, estado, memoria, S-meter, settings.
 * @author  Alonso José Lara Plana (EA7LBT)
 * @license MIT — ver ATS-ULTIMATE.ino para texto completo
 */
#include "DisplayUI.h"
#include "State.h"
#include "Config.h"
#include "font14x24sevenSeg.h"

#ifdef USE_BATTERY_INDICATOR
void showBattery()
{
    static uint32_t lastRead = 0;
    static uint8_t  lastPct  = 0;
    static bool     lastUsb  = false;

    uint32_t now = millis();
    if (now - lastRead > 5000) {
        lastRead = now;
        lastUsb  = g_usbPowered;  // ya detectado en loop()
        if (!lastUsb) {
            uint16_t raw = analogRead(BATTERY_PIN);
            if      (raw <= 614) lastPct = 0;
            else if (raw >= 818) lastPct = 100;
            else    lastPct = (uint8_t)((uint16_t)(raw - 614) * 100u / 204u);
        }
    }

    char buf[5];
    if (g_usbPowered) {
        // USB conectado → mostrar "USB"
	buf[0]='C'; buf[1]='H'; buf[2]='G'; buf[3]=' '; buf[4]='\0';
    } else if (lastPct <= 5) {
        // Batería crítica → mostrar "LOW"
        buf[0]='L'; buf[1]='O'; buf[2]='W'; buf[3]=' '; buf[4]='\0';
    } else if (lastPct == 100) {
        buf[0]='1'; buf[1]='0'; buf[2]='0'; buf[3]='%'; buf[4]='\0';
    } else {
        buf[0] = (lastPct >= 10) ? ('0' + lastPct / 10) : ' ';
        buf[1] = '0' + lastPct % 10;
        buf[2] = '%';
        buf[3] = ' ';
        buf[4] = '\0';
    }
    oledPrint(buf, 100, 0, DEFAULT_FONT);
}
#endif

static void formatFreqBuf(char* fd, uint16_t freq, int8_t bandIdx)
{
    if (bandIdx == FM_IDX) {
        uint16_t mhz = freq / 100;
        uint8_t  dec = freq % 100;
        fd[0] = (mhz >= 100) ? ('0' + mhz / 100) : ' ';
        fd[1] = '0' + (mhz % 100) / 10;
        fd[2] = '0' +  mhz % 10;
        fd[3] = '.';
        fd[4] = '0' + dec / 10;
        fd[5] = '0' + dec % 10;
        fd[6] = '\0';
    } else if (g_Settings[UnitsSwitch].param && freq >= 1000) {
        uint16_t mhz = freq / 1000;
        uint16_t khz = freq % 1000;
        fd[0] = ' ';
        fd[1] = (mhz >= 10) ? ('0' + mhz / 10) : ' ';
        fd[2] = '0' + mhz % 10;
        fd[3] = '.';
        fd[4] = '0' + khz / 100;
        fd[5] = '0' + (khz % 100) / 10;
        fd[6] = '\0';
    } else {
        uint16_t miles    = freq / 1000;
        uint16_t unidades = freq % 1000;
        fd[0] = fd[1] = fd[2] = ' ';
        if (miles > 0) {
            fd[0] = (miles < 10) ? ' ' : ('0' + miles / 10);
            fd[1] = '0' + miles % 10;
            fd[2] = ' ';
        }
        fd[3] = (unidades >= 100) ? ('0' + unidades / 100)        : ' ';
        fd[4] = (unidades >=  10) ? ('0' + (unidades % 100) / 10) : ' ';
        fd[5] = '0' + unidades % 10;
        fd[6] = '\0';
    }
}

static const char* const s_mods[] = {"AM", "LSB", "USB", "CW"};

void showMemoryView()
{
    char buf[17] = "MEM CH:   ";
    buf[8] = '0' + (g_memoryIndex + 1) / 10;
    buf[9] = '0' + (g_memoryIndex + 1) % 10;
    buf[10] = '\0';
    oledPrint(buf, 0, 0, DEFAULT_FONT);

    if (g_previewMemory.freq == 0 || g_previewMemory.freq == 0xFFFF) {
        oledPrint(F("  [CANAL VACIO] "), 0, 4, DEFAULT_FONT);
        oledPrint(F("                "), 0, 6, DEFAULT_FONT);
    } else {
	char fBuf[8];
	formatFreqBuf(fBuf, g_previewMemory.freq, g_previewMemory.bandIdx);
	oledPrint(fBuf, 0, 3, FONT14X24SEVENSEG);

	if (g_previewMemory.bandIdx == FM_IDX || (g_Settings[UnitsSwitch].param && g_previewMemory.freq >= 1000))
	    oledPrint("MHz", 102, 5, DEFAULT_FONT);
	else
	    oledPrint("kHz", 102, 5, DEFAULT_FONT);

        const char* mStr = (g_previewMemory.mode == FM) ? "FM" : s_mods[g_previewMemory.mode];
        char mBuf[17] = "Modo:           ";
        uint8_t i = 0;
        while (mStr[i] && i < 4) { mBuf[6+i] = mStr[i]; i++; }
        mBuf[10] = '\0';
        oledPrint(mBuf, 0, 6, DEFAULT_FONT);
    }
}

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
    if (cleanDisplay) oledPrint("      ", 15, 2, FONT14X24SEVENSEG);

    char fd[8];
    formatFreqBuf(fd, g_currentFrequency, g_bandIndex);
    oledPrint(fd, 15, 2, FONT14X24SEVENSEG);

    bool showMHz = (g_bandIndex == FM_IDX) ||
                   (g_Settings[UnitsSwitch].param && g_currentFrequency >= 1000);
    oledPrint(showMHz ? "MHz" : "kHz", 102, 5, DEFAULT_FONT);
}

void showStatus(bool cleanFreq)
{
    if (cleanFreq) oled.clear();
    if (!cleanFreq) oledPrint("          ", 0, 5, DEFAULT_FONT);
    showFrequency();
    showModulation();
    showStep();
    showVolumeBar();
#ifdef USE_BATTERY_INDICATOR
    showBattery();
#endif
    showBandwidth();
    showLockIndicator();
    if (isSSB()) showBFO();
}

static const char* getSWBandName(uint16_t freq)
{
    // { minFreq, maxFreq, nombre } — todo en PROGMEM
    static const uint16_t s_bands[][2] PROGMEM = {
        {1800,  2000}, {3500,  3900}, {3900,  4000}, {4750,  5060},
        {5900,  6200}, {7000,  7200}, {7200,  7450}, {9400,  9900},
        {11600,12100}, {13570,13870}, {14000,14350}, {15100,15800},
        {17480,17900}, {21000,21450}, {28000,29700}
    };
    static const char s_names[] PROGMEM =
        "160m" " 80m" " 75m" " 60m"
        " 49m" " 40m" " 41m" " 31m"
        " 25m" " 22m" " 20m" " 19m"
        " 16m" " 15m" " 10m";

    for (uint8_t i = 0; i < 15; i++) {
        if (freq >= pgm_read_word(&s_bands[i][0]) &&
            freq <= pgm_read_word(&s_bands[i][1]))
        {
            // Leer 4 chars desde PROGMEM a un buffer estático
            static char buf[5];
            memcpy_P(buf, s_names + i * 4, 4);
            buf[4] = '\0';
            return buf;
        }
    }
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
        label = s_mods[g_currentMode];
    }
    oledPrint(label, 0, 0, DEFAULT_FONT);
}

void showStep()
{
    char buf[9] = "St:";
    int step = (g_bandIndex == FM_IDX)
        ? (int)pgm_read_byte(&g_tabStepFM[g_FMStepIndex])
        : (int)pgm_read_word(&g_tabStep[g_stepIndex]);

    if (isSSB() && g_Settings[StepUnits].param == 1) {
        // Mostrar en Hz cuando SWUnits=1 y estamos en SSB
        itoa(step, buf + 3, 10);
        strcat(buf, "H");   // "St:25H" → 25 Hz
    } else {
        itoa(step, buf + 3, 10);
        // sin unidad, igual que ahora
    }
    oledPrint(buf, 55, 0, DEFAULT_FONT);
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
    g_SettingsPage = g_SettingSelected / 4;

    for (uint8_t i = 0; i < 4; i++) {
        uint8_t idx = (g_SettingsPage * 4) + i;
        bool selected = (g_SettingSelected == idx);
        bool editing  = selected && g_isEditingSetting;

        char line[17] = "                ";  // 16 espacios

        if (idx < SETTINGS_MAX) {
            // Cursor + nombre
            line[0] = selected ? '>' : ' ';
            memcpy(line + 1, g_Settings[idx].name, 4);
            line[5] = editing ? '[' : ':';

            // Valor
            char* dst = line + 6;
            if (editing || g_Settings[idx].type == Num) {
                itoa(g_Settings[idx].param, dst, 10);
                if (editing) {
                    dst[strlen(dst)] = ']';
                }
            } else {
                memcpy(dst, g_Settings[idx].param ? "ON " : "OFF", 3);
            }
        }

        oledPrint(line, 0, i * 2, DEFAULT_FONT, selected && !editing);
    }
}

void showSMeter() {
    g_si4735.getCurrentReceivedSignalQuality();
    
    if (g_snrMode) {
        char snrBuf[5];  // "255dB" = 4 chars + \0
        itoa(g_si4735.getCurrentSNR(), snrBuf, 10);
        snrBuf[3] = 'd'; snrBuf[4] = 'B'; snrBuf[5] = '\0';  // ← strcat → manual
        oledPrint(snrBuf, 0, 7, DEFAULT_FONT);
    } else {
        char bar[17];
        int level = map(g_si4735.getCurrentRSSI(), 0, 64, 0, 16);
        
        for (int i = 0; i < 16; i++) 
            bar[i] = (i < level) ? '|' : ' ';
        bar[16] = '\0';
        oledPrint(bar, 0, 7, DEFAULT_FONT);
    }
}

/*
void showSMeter()
{
    g_si4735.getCurrentReceivedSignalQuality();
    int snr = g_si4735.getCurrentSNR();
    int rssi = g_si4735.getCurrentRSSI();
    
    if (g_snrMode) {
        char snrBuf[6]; 
        itoa(snr, snrBuf, 10); 
        strcat(snrBuf, "dB"); 
        oledPrint(snrBuf, 0, 7, DEFAULT_FONT);
    } else {
        // RSSI bar actual...
	char bar[17];
	int level = map(rssi, 0, 64, 0, 16);
    
	for (int i=0; i<16; i++) bar[i] = (i < level) ? '|' : ' ';
	bar[16] = '\0';
    
	oledPrint(bar, 0, 7, DEFAULT_FONT);
    }
}
*/

void showBandTag()
{
    static const char* const bandNames[] = { "LW","MW","SW","FM","CB","AIR" };
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

    buf[15] = '0' + (LAST_BAND + 1);
    buf[16] = '\0';

    oledPrint(buf, 0, 0, DEFAULT_FONT);
}

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

void showBFO()
{
    if (!isSSB()) return;

    char buf[10] = "BFO:";
    int bfo = g_currentBFO;

    if (bfo >= 0) {
        buf[4] = '+';
    } else {
        buf[4] = '-';
        bfo = -bfo;
    }

    if (bfo >= 1000) {
        // Mostrar en kHz con 1 decimal: "+1.2k"
        buf[5] = '0' + bfo / 1000;
        buf[6] = '.';
        buf[7] = '0' + (bfo % 1000) / 100;
        buf[8] = 'k';
        buf[9] = '\0';
    } else {
        // Mostrar en Hz: "+350H"
        itoa(bfo, buf + 5, 10);
	uint8_t len = strlen(buf + 5);
	buf[5 + len] = 'H';
        buf[6 + len] = '\0';
    }

    oledPrint(buf, 0, 6, DEFAULT_FONT);
}

void showSplash()
{
    oled.clear();
    oledPrint(F("ATS-ULTIMATE"),     0, 1, DEFAULT_FONT);
    oledPrint(F("v" FW_VERSION),     0, 3, DEFAULT_FONT);
}

void showVolumeBar()
{
    char buf[5] = "V:";
    uint8_t vol = g_muteVolume ? 0 : g_si4735.getVolume();
    buf[2] = g_muteVolume ? 'M' : ('0' + vol / 10);
    buf[3] = g_muteVolume ? ' ' : ('0' + vol % 10);
    buf[4] = '\0';
    oledPrint(buf, 100, 0, DEFAULT_FONT);
}

void showLockIndicator()
{
    if (g_keyLocked) {
        oledPrint(F("LCK"), 34, 0, DEFAULT_FONT);
    } else if (g_bandLocked) {
        oledPrint("B.L", 34, 0, DEFAULT_FONT);  // ← Sin F() aquí
    } else {
        oledPrint(F("   "), 34, 0, DEFAULT_FONT);
    }
}

void updateDisplay()
{
    if (g_settingsActive) return;

    if (g_memoryMode) {
        showMemoryView();
        return; 
    }

    static uint32_t lastUpdate = 0;
    uint32_t now = millis();
    if (now - lastUpdate <= 250) return;
    lastUpdate = now;

    if (g_showSmeterBar) showSMeter();
    else oledPrint(F("                "), 0, 7, DEFAULT_FONT);

    if (g_scanning && (g_currentMode == FM || g_currentMode == AM)) doScan();

    if (g_currentMode == FM && g_displayRDS) {
        g_si4735.getRdsStatus();
        showRDS();
    }
}
