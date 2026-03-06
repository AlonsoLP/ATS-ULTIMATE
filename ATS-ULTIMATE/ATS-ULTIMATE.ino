/**
 * @file    ATS-ULTIMATE.ino
 * @brief   Firmware alternativo para receptor ATS-20 / ATS-20+
 *          basado en el chip SI4735 de Silicon Laboratories.
 *
 * @details Firmware de código abierto con soporte para AM, SW, FM, SSB y CW.
 *          Incluye menú de configuración, indicador de batería, BFO ajustable,
 *          S-meter, RDS en FM, bandas de radioaficionado y gestión de CPU.
 *
 * @author  Alonso José Lara Plana (EA7LBT)
 * @version 1.0.0
 * @date    2026
 *
 * @hardware ATS-20 / ATS-20+ (SI4735 + ATmega328P + OLED SSD1306 128x64)
 * @repo     https://github.com/AlonsoLP/ATS-ULTIMATE
 *
 * @license MIT License
 *          Copyright (c) 2026 Alonso José Lara Plana (EA7LBT)
 *
 *          Permission is hereby granted, free of charge, to any person
 *          obtaining a copy of this software and associated documentation
 *          files (the "Software"), to deal in the Software without
 *          restriction, including without limitation the rights to use,
 *          copy, modify, merge, publish, distribute, sublicense, and/or
 *          sell copies of the Software, and to permit persons to whom the
 *          Software is furnished to do so, subject to the following
 *          conditions:
 *
 *          The above copyright notice and this permission notice shall be
 *          included in all copies or substantial portions of the Software.
 *
 *          THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *          EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 *          OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *          NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 *          HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *          WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *          FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *          OTHER DEALINGS IN THE SOFTWARE.
 *
 * @credits Basado en los trabajos de:
 *          - Ricardo Lima Caratti (PU2CLR) — Librería SI4735 Arduino
 *            https://github.com/pu2clr/SI4735
 *          - [COMPLETAR: otros proyectos o personas que hayan inspirado el firmware]
 *
 * @changelog
 *          1.0.0 (2026-XX-XX) — Primera versión pública
 *                              [COMPLETAR: fecha exacta de lanzamiento]
 */
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
    // 0. Inicializar I2C
    Wire.begin();
    Wire.setClock(400000);

    // 1. Inicializar Pantalla OLED + Splash
    oled.begin();
    oled.clear();
    oled.on();
    oled.switchRenderFrame();
    showSplash();

    // 2. Reset EEPROM si encoder pulsado al encender
    if (digitalRead(ENCODER_BUTTON) == LOW) {
        oledPrint(F("!! EEPROM RESET !!"), 0, 6, DEFAULT_FONT);
        resetEEPROM();
        delay(2000);
    }

    // 3. Inicializar Radio (SI4735)
    g_si4735.setup(RESET_PIN, MW_BAND_TYPE);
    delay(500);

    // 4. Configurar Encoder (pines + interrupts + tabla PROGMEM)
    setupEncoder();

    // 5. Configurar Botón del Encoder
    pinMode(ENCODER_BUTTON, INPUT_PULLUP);

    // 6. Banda por defecto (MW)
    g_bandIndex = 1;
    applyBandConfiguration();

    // 7. Cargar estado desde EEPROM y aplicar
    loadState();
    applyBandConfiguration();
    g_si4735.setVolume(g_savedVolume);

    // 8. Detección USB y velocidad CPU
#ifdef USE_BATTERY_INDICATOR
    g_usbPowered = (analogRead(BATTERY_PIN) > USB_DETECT_THRESHOLD);
#else
    g_usbPowered = false;
#endif
    applyCPUSpeed(g_usbPowered ? 0 : g_Settings[CPUSpeed].param);

    // 9. Arrancar pantalla principal
    delay(600);
    oled.clear();
    showStatus(true);
}

// --- Bucle Principal ---
void loop()
{
    processEncoder();
    processButtons();
    updateDisplay();
    checkUSBPower();

    if (g_storeTime > 0 && (millis() - g_storeTime > STORE_TIME)) {
        saveState();
        g_storeTime = 0;
    }
}
