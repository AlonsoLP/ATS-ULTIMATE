#pragma once
#include <Arduino.h>

int getSteps();
int getLastStep();
extern bool g_scanning;

void applyBandConfiguration();
void doBand(int8_t v);
void doMode(int8_t v);
void doBandwidth(int8_t v);
void doScan();
void doSWSubBand(int8_t v);
void updateBFO();
void agcSetFunc();
void updateSSBCutoffFilter();
void setRDSConfig(uint8_t bias);
