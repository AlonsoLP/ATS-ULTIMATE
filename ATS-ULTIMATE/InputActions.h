#pragma once
#include <Arduino.h>

// Eventos
uint8_t volumeEvent(uint8_t event, uint8_t pin);
uint8_t stepEvent(uint8_t event, uint8_t pin);
uint8_t bandEvent(uint8_t event, uint8_t pin);

// Sintonización
void doFrequencyTune();
void doFrequencyTuneSSB();

// Callbacks de Settings
void doAttenuation(int8_t v);
void doSoftMute(int8_t v);
void doBrightness(int8_t v);
void doSSBAVC(int8_t v);
void doAvc(int8_t v);
void doSync(int8_t v);
void doDeEmp(int8_t v);
void doSWUnits(int8_t v);
void doSSBSoftMuteMode(int8_t v);
void doCutoffFilter(int8_t v);
void doCPUSpeed(int8_t v);
void doBFOCalibration(int8_t v);
void doUnitsSwitch(int8_t v);
void doScanSwitch(int8_t v);
void doCWSwitch(int8_t v);

// Generales
void doStep(int8_t v);
void doVolume(int8_t v);
