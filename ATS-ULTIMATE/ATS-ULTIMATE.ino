#include <Wire.h>
#include <EEPROM.h>
#include <Tiny4kOLED.h>
#include <SI4735.h>
#include "Config.h"
#include "State.h"
#include "Utils.h"
#include "RadioCtrl.h"
#include "DisplayUI.h"
#include "InputActions.h"

void setup()
{
    Wire.begin();
    Wire.setClock(400000);

    oled.begin();
    oled.clear();
    oled.on();
    oled.switchRenderFrame();

    if (digitalRead(ENCODER_BUTTON) == LOW) {
        oled.clear();
        oledPrint(F("EEPROM RESET"), 20, 2, DEFAULT_FONT);
        resetEEPROM();
        delay(2000);
    }

    oledPrint(F("ATS-ULTIMATE"),  25, 0, DEFAULT_FONT);
    oledPrint(F("GEROPPPPA"), 20, 3, DEFAULT_FONT);

    g_si4735.setup(RESET_PIN, MW_BAND_TYPE);
    delay(500);

    setupEncoder();                    // ← encoder + interrupciones
    pinMode(ENCODER_BUTTON, INPUT_PULLUP);

    g_bandIndex = 1;
    applyBandConfiguration();
    loadState();
    applyBandConfiguration();
    g_si4735.setVolume(g_savedVolume);

    delay(1000);
    oled.clear();
    showStatus(true);
}

void loop()
{
    uint32_t now = millis();

    // 1. Encoder
    int8_t rotacion = 0;
    noInterrupts();
    if (g_encoderCount != 0) { rotacion = g_encoderCount; g_encoderCount = 0; }
    interrupts();

    if (rotacion != 0) {
        if (g_bandSelectMode) {
            doBand(rotacion > 0 ? 1 : -1);
            showBandTag();
        } else if (g_settingsActive) {
            if (g_isEditingSetting) {
                if (g_Settings[g_SettingSelected].manipulateCallback != NULL)
                    g_Settings[g_SettingSelected].manipulateCallback(rotacion);
            } else {
                g_SettingSelected += rotacion;
                int8_t pageStart = g_SettingsPage * 3;
                int8_t pageEnd   = pageStart + 2;
                if (pageEnd >= SETTINGS_MAX) pageEnd = SETTINGS_MAX - 1;
                if (g_SettingSelected < pageStart) g_SettingSelected = pageEnd;
                if (g_SettingSelected > pageEnd)   g_SettingSelected = pageStart;
            }
            showSettings();
        } else if (g_cmdVolume) {
            doVolume(rotacion);
        } else {
            if (isSSB()) doFrequencyTuneSSB(rotacion);
            else         doFrequencyTune(rotacion);
        }
        g_storeTime = now;
    }

    // 2. Botones
    processButtons();                  // ← todo el hardware de entrada aquí

    // 3. Actualizaciones periódicas
    if (!g_settingsActive) {
        static uint32_t lastUpdate = 0;
        if (now - lastUpdate > 250) {
            lastUpdate = now;
            if (g_showSmeterBar) showSMeter();
            else oledPrint(F("                "), 0, 7, DEFAULT_FONT);
#if USE_RDS
            if (g_currentMode == FM && g_displayRDS) {
                g_si4735.getRdsStatus();
                showRDS();
            }
#endif
        }
    }

    // 4. Scan independiente
    if (g_scanning && (g_currentMode == FM || g_currentMode == AM)) {
        static uint32_t lastScan = 0;
        if (now - lastScan > 400) { lastScan = now; doScan(); }
    }

    // 5. EEPROM
    if (g_storeTime && (now - g_storeTime > STORE_TIME)) {
        saveState();
        g_storeTime = 0;
    }
}
