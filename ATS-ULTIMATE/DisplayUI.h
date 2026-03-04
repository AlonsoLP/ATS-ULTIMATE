#pragma once
#include <Arduino.h>
//#include <Tiny4kOLED.h> // Para DCfont
#include "Config.h"     // Para DEFAULT_FONT

void oledPrint(const char* text, uint8_t x, uint8_t y, const DCfont* font = DEFAULT_FONT, bool invert = false);
void showStatus(bool cleanFreq = false);
void showFrequency(bool cleanDisplay = false);
void showModulation();
void showStep();
void showBandwidth();
void showVolume();
void showSettings();
void showSMeter();
void showBandTag();
#if USE_RDS
void showRDS();
#endif
