#include "DisplayUI.h"
#include "State.h"
#include "Config.h"
#include "font14x24sevenSeg.h"

#ifdef USE_BATTERY_INDICATOR
void showBattery() {
    // Throttle: leer ADC máximo cada 5 segundos
    static uint32_t lastRead = 0;
    static uint8_t  lastPct  = 0;
    uint32_t now = millis();
    if (now - lastRead > 5000) {
        lastRead = now;
        uint16_t raw = analogRead(BATTERY_PIN);
        // Aritmética 16-bit pura — evita la multiplicación 32-bit de map()
        if      (raw <= 614) lastPct = 0;
        else if (raw >= 818) lastPct = 100;
        else    lastPct = (uint8_t)((uint16_t)(raw - 614) * 100u / 204u);
    }

    // Construir "XXX%" sin itoa ni strcat
    char buf[5];
    if (lastPct == 100) {
        buf[0]='1'; buf[1]='0'; buf[2]='0'; buf[3]='%'; buf[4]='\0';
    } else {
        buf[0] = (lastPct >= 10) ? ('0' + lastPct / 10) : ' ';
        buf[1] = '0' + lastPct % 10;
        buf[2] = '%';
        buf[3] = '\0';
    }
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

void showFrequency(bool cleanDisplay) {
    if (g_settingsActive) return;

    char fd[8]; // freqDisplay — máximo "XX XXXMHZ" = 7 chars + \0

    if (cleanDisplay)
        oledPrint("      ", 15, 2, FONT14X24SEVENSEG);

    if (g_bandIndex == FM_IDX) {
        // FM: frecuencia en MHz con 2 decimales  →  "107.50"
        uint16_t mhz = g_currentFrequency / 100;
        uint8_t  dec = g_currentFrequency % 100;
        fd[0] = (mhz >= 100) ? ('0' + mhz / 100) : ' ';
        fd[1] = '0' + (mhz % 100) / 10;
        fd[2] = '0' +  mhz % 10;
        fd[3] = '.';
        fd[4] = '0' + dec / 10;
        fd[5] = '0' + dec % 10;
        fd[6] = '\0';
        oledPrint(fd,    15, 2, FONT14X24SEVENSEG);
        oledPrint("MHz", 102, 5, DEFAULT_FONT);

    } else if (g_Settings[UnitsSwitch].param && g_currentFrequency >= 1000) {
        // AM/SW en MHz si UnitsSwitch=1 y freq >= 1 MHz  →  " 14.200"
        uint16_t mhz = g_currentFrequency / 1000;
        uint16_t khz = g_currentFrequency % 1000;
        fd[0] = ' ';
	fd[1] = (mhz >= 10) ? ('0' + mhz / 10) : ' ';
	fd[2] = '0' + mhz % 10;
        fd[3] = '.';
        fd[4] = '0' + khz / 100;
        fd[5] = '0' + (khz % 100) / 10;
        fd[6] = '\0';
        oledPrint(fd,    15, 2, FONT14X24SEVENSEG);
        oledPrint("MHz", 102, 5, DEFAULT_FONT);

    } else {
        // AM/SW en kHz  →  "14 200" o "   300"
        uint16_t miles    = g_currentFrequency / 1000;
        uint16_t unidades = g_currentFrequency % 1000;
        if (miles > 0) {
            fd[0] = (miles < 10) ? ' ' : ('0' + miles / 10);
            fd[1] = '0' + miles % 10;
            fd[2] = ' ';
        } else {
            fd[0] = fd[1] = fd[2] = ' ';
        }
        fd[3] = (unidades >= 100) ? ('0' + unidades / 100)        : ' ';
        fd[4] = (unidades >=  10) ? ('0' + (unidades % 100) / 10) : ' ';
        fd[5] = '0' + unidades % 10;
        fd[6] = '\0';
        oledPrint(fd,    15, 2, FONT14X24SEVENSEG);
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
    if (isSSB()) showBFO();
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
        static const char* mods[] = {"AM", "LSB", "USB", "CW"};
        label = mods[g_currentMode];
    }
    oledPrint(label, 0, 0, DEFAULT_FONT);
}

void showStep() {
    char buf[10] = "St:";
    int step = (g_bandIndex == FM_IDX)
        ? (int)g_tabStepFM[g_FMStepIndex]
        : (int)pgm_read_word(&g_tabStep[g_stepIndex]);

    if (isSSB() && g_Settings[SWUnits].param == 1) {
        // Mostrar en Hz cuando SWUnits=1 y estamos en SSB
        itoa(step, buf + 3, 10);
        strcat(buf, "H");   // "St:25H" → 25 Hz
    } else {
        itoa(step, buf + 3, 10);
        // sin unidad, igual que ahora
    }
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

void showSettings() {
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

void showSMeter()
{
    g_si4735.getCurrentReceivedSignalQuality();
    int rssi = g_si4735.getCurrentRSSI();
    char bar[17];
    // Mapeo de RSSI (0-64) a 16 caracteres de la pantalla
    int level = map(rssi, 0, 64, 0, 16);
    
    for(int i=0; i<16; i++) bar[i] = (i < level) ? '|' : ' ';
    bar[16] = '\0';
    
    oledPrint(bar, 0, 7, DEFAULT_FONT);
}

void showBandTag()
{
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

void showBFO() {
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
        uint8_t len = strlen(buf);
        buf[len] = 'H';
        buf[len + 1] = '\0';
    }

    oledPrint(buf, 0, 6, DEFAULT_FONT);
}
