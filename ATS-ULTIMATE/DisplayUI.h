#pragma once
#include <Arduino.h>

void showStatus(bool cleanFreq = false);
void showFrequency(bool cleanDisplay = false);
void showModulation();
void showStep();
void showBandwidth();
void showVolume();
void showSettings();
void showSMeter();
#if USE_RDS
void showRDS();
#endif
void showBandTag();
