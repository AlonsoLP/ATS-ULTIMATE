#pragma once
#include <Arduino.h>

bool isSSB();
int getSteps();
int getLastStep();
void updateSSBCutoffFilter();
void bandSwitch(bool up);
void applyBandConfiguration(bool extraSSBReset = false);
void updateBFO();
void agcSetFunc();
void resetEepromDelay();
void doBand(int8_t v);
void doMode(int8_t v);
void doBandwidth(int8_t v);

#if USE_RDS
void setRDSConfig(uint8_t bias);
#endif
