#include <Arduino.h>
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

// --- Configuración Inicial ---
void setup()
{
    // 0. Inicializar I2C (velocidad provisional hasta leer EEPROM)
    Wire.begin();
    Wire.setClock(400000);

    // 1. Inicializar Pantalla OLED
    oled.begin();
    oled.clear();
    oled.on();
    oled.switchRenderFrame();

    // Lógica de Reset EEPROM (Botón pulsado al encender)
    if (digitalRead(ENCODER_BUTTON) == LOW) {
        oled.clear();
        oledPrint(F("EEPROM RESET"), 20, 2, DEFAULT_FONT);
        resetEEPROM();
        delay(2000);
    }

    oledPrint(F("ATS-ULTIMATE"), 20, 0, DEFAULT_FONT);
    oledPrint(F("SISTEMA LISTO"), 15, 3, DEFAULT_FONT);

    // 2. Inicializar Radio (Chip SI4735)
    g_si4735.setup(RESET_PIN, MW_BAND_TYPE);
    delay(500);

    // 3. Configurar Encoder e Interrupciones
    setupEncoder();

    // 4. Configurar Botón del Encoder
    pinMode(ENCODER_BUTTON, INPUT_PULLUP);

    // 5. Cargar Banda por Defecto (MW)
    g_bandIndex = 1;
    applyBandConfiguration();

    // 6. Cargar estado desde EEPROM
    loadState();
    applyBandConfiguration();
    g_si4735.setVolume(g_savedVolume);

    // 7. Detectar USB y aplicar velocidad CPU correcta
    //    Si USB conectado → forzar 16 MHz ignorando lo guardado
    //    Si batería       → restaurar velocidad guardada por el usuario
#ifdef USE_BATTERY_INDICATOR
    g_usbPowered = (analogRead(BATTERY_PIN) > USB_DETECT_THRESHOLD);
#else
    g_usbPowered = false;
#endif
    applyCPUSpeed(g_usbPowered ? 0 : g_Settings[CPUSpeed].param);

    delay(1000);
    oled.clear();
    showStatus(true);
}

// --- Bucle Principal ---
void loop()
{

    // --- 1. PROCESAR ENCODER ---
    int8_t rotacion = 0;

    noInterrupts();
    if (g_encoderCount != 0) {
        rotacion = g_encoderCount;
        g_encoderCount = 0;
    }
    interrupts();

    if (rotacion != 0) {
        if (g_bandSelectMode) {
            doBand(rotacion > 0 ? 1 : -1);
            showBandTag();
        } else if (g_settingsActive) {
            if (g_isEditingSetting) {
                // Modo edición: cambia el valor del setting seleccionado
                if (g_Settings[g_SettingSelected].manipulateCallback != NULL)
                    g_Settings[g_SettingSelected].manipulateCallback(rotacion);
            } else {
                // Modo navegación: mueve el cursor entre settings
		g_SettingSelected += rotacion;
		if (g_SettingSelected < 0) g_SettingSelected = SETTINGS_MAX - 1;
		if (g_SettingSelected >= SETTINGS_MAX) g_SettingSelected = 0;
            }
            showSettings();
        } else if (g_cmdVolume) {
            doVolume(rotacion);
        } else {
            if (isSSB()) doFrequencyTuneSSB(rotacion);
            else         doFrequencyTune(rotacion);
        }
        g_storeTime = millis();
    }

    // --- 2. PROCESAR BOTONES ---
    processButtons();

    // --- 3. ACTUALIZACIONES DE PANTALLA (Background) ---
    if (!g_settingsActive) {
        static uint32_t lastUpdate = 0;
        uint32_t now = millis();
        if (now - lastUpdate > 250) {
            lastUpdate = now;
            if (g_showSmeterBar)
                showSMeter();
            else
                oledPrint(F("                "), 0, 7, DEFAULT_FONT);

            if (g_scanning && (g_currentMode == FM || g_currentMode == AM))
                doScan();

            if (g_currentMode == FM && g_displayRDS) {
                g_si4735.getRdsStatus();
                showRDS();
            }
        }
    }

    // --- 4. DETECCIÓN USB / CARGADOR (cada 2 segundos) ---
#ifdef USE_BATTERY_INDICATOR
    {
        static uint32_t lastUsbCheck = 0;
        uint32_t now = millis();
        if (now - lastUsbCheck > 2000) {
            lastUsbCheck = now;
            bool wasUsb = g_usbPowered;
            g_usbPowered = (analogRead(BATTERY_PIN) > USB_DETECT_THRESHOLD);

            if (g_usbPowered && !wasUsb) {
                // USB recién conectado → forzar 16 MHz
                applyCPUSpeed(0);
                // g_Settings[CPUSpeed].param NO se toca — se preserva preferencia del usuario
            } else if (!g_usbPowered && wasUsb) {
                // USB recién desconectado → restaurar velocidad guardada
                applyCPUSpeed(g_Settings[CPUSpeed].param);
            }
        }
    }
#endif

    // --- 5. GUARDADO AUTOMÁTICO EN EEPROM ---
    if (g_storeTime > 0 && (millis() - g_storeTime > STORE_TIME)) {
        saveState();
        g_storeTime = 0;
    }
}
