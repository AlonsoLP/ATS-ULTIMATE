/**
 * @file    DisplayUI.h
 * @brief   Interfaz de cabeceras para funciones de visualización OLED.
 * @author  Alonso José Lara Plana (EA7LBT)
 * @license MIT — ver ATS-ULTIMATE.ino para texto completo
 */
#pragma once
#include <Arduino.h>
#include "Config.h"
#include "RadioCtrl.h"

void showStatus(bool cleanFreq = false);
void showFrequency(bool cleanDisplay = false);
void showModulation();
void showStep();
void showBandwidth();
void showSettings();
void showSMeter();
void showBandTag();
void showRDS();
void showBFO();
void showSplash();
void showVolumeBar();
void showLockIndicator();
void updateDisplay();
void showMemoryView();

void oledPrint(const char* text, uint8_t x, uint8_t y, const DCfont* font = DEFAULT_FONT, bool invert = false);
void oledPrint(const __FlashStringHelper* text, uint8_t x, uint8_t y, const DCfont* font = DEFAULT_FONT, bool invert = false);
