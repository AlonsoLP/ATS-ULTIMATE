#pragma once
#include <Arduino.h>
#include "Config.h"     // Para DEFAULT_FONT

void showStatus(bool cleanFreq = false);
void showFrequency(bool cleanDisplay = false);
void showModulation();
void showStep();
void showBandwidth();
void showVolume();
void showSettings();
void showSMeter();
void showBandTag();

void oledPrint(const char* text, uint8_t x, uint8_t y, const DCfont* font = DEFAULT_FONT, bool invert = false);
void oledPrint(const __FlashStringHelper* text, uint8_t x, uint8_t y, const DCfont* font = DEFAULT_FONT, bool invert = false);
void showRDS();
void showBFO();
