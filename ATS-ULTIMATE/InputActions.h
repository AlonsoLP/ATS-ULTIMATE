#pragma once
#include <Arduino.h>

// --- 1. EVENTOS DE BOTONES (Mapeo ATS-20+) ---
// Estas funciones reciben el evento (corto/largo) y el pin
uint8_t volPlusEvent(uint8_t event, uint8_t pin);
uint8_t volMinusEvent(uint8_t event, uint8_t pin);
uint8_t bandPlusEvent(uint8_t event, uint8_t pin);
uint8_t bandMinusEvent(uint8_t event, uint8_t pin);
uint8_t stepEvent(uint8_t event, uint8_t pin);
uint8_t modeEvent(uint8_t event, uint8_t pin);
uint8_t bwEvent(uint8_t event, uint8_t pin);
uint8_t agcEvent(uint8_t event, uint8_t pin);
uint8_t tuneEvent(uint8_t event, uint8_t pin);

// --- 2. SINTONIZACIÓN ---
void doFrequencyTune(int8_t v);
void doFrequencyTuneSSB(int8_t v);

// --- 3. DECLARACIONES DE CONTROL (Resuelven errores de "not declared") ---
// Estas funciones están en RadioCtrl.cpp, las declaramos aquí para que InputActions.cpp las vea
void doBand(int8_t v);
void doMode(int8_t v);
void doStep(int8_t v);
void doVolume(int8_t v);
void doBandwidth(int8_t v);

// --- 4. CALLBACKS DE SETTINGS (Menú) ---
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
