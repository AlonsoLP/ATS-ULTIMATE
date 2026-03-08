/**
 * @file    RadioCtrl.h
 * @brief   Cabeceras para control del chip SI4735 y gestión de bandas/modos.
 * @author  Alonso José Lara Plana (EA7LBT)
 * @license MIT — ver ATS-ULTIMATE.ino para texto completo
 */
#pragma once
#include <Arduino.h>
#include "Config.h"

int getSteps();
extern bool g_scanning;

void applyBandConfiguration();
void doBand(int8_t v);
void doMode(int8_t v);
void doBandwidth(int8_t v);
void doScan();
void doSWSubBand(int8_t v);
void updateBFO();
void updateSSBCutoffFilter();
void setRDSConfig(uint8_t bias);
void checkUSBPower();
void applyCPUSpeed(int8_t level);

// sistema de memorias
void applyRadioState();
void applyMemoryState(MemoryChannel& mem);
