#include "InputActions.h"
#include "State.h"
#include "Config.h"
#include "RadioCtrl.h"
#include "DisplayUI.h"

// --- Helper interno para lógica de conmutación ---
inline void doSwitchLogic(int8_t& param, int8_t low, int8_t high, int8_t step)
{
    param += step;
    if (param < low) param = high;
    else if (param > high) param = low;
}

inline void doBandwidthLogic(int8_t& bwIndex, uint8_t upperLimit, uint8_t v)
{
    doSwitchLogic(bwIndex, 0, upperLimit, v);
    g_bandList[g_bandIndex].bandwidthIdx = bwIndex;
}

// --- Eventos de Botones ---
uint8_t volumeEvent(uint8_t event, uint8_t pin)
{
    if (g_muteVolume)
    {
        if (event == BUTTONEVENT_SHORTPRESS || event == BUTTONEVENT_FIRSTLONGPRESS)
            doVolume(1);
    }

    if (!g_muteVolume)
    {
        if (event == BUTTONEVENT_FIRSTLONGPRESS || event == BUTTONEVENT_LONGPRESS)
            doVolume(g_encoderCount);
    }
    return event;
}

uint8_t stepEvent(uint8_t event, uint8_t pin)
{
    if (event == BUTTONEVENT_SHORTPRESS) doStep(1);
    return event;
}

uint8_t bandEvent(uint8_t event, uint8_t pin)
{
    if (event == BUTTONEVENT_SHORTPRESS) bandSwitch(true);
    else if (event == BUTTONEVENT_FIRSTLONGPRESS) bandSwitch(false);
    return event;
}

// --- Sintonización de Frecuencia ---
void doFrequencyTune()
{
    if (g_bandIndex == FM_BAND_TYPE)
    {
        g_currentFrequency += g_tabStepFM[g_FMStepIndex] * g_encoderCount;
    }
    else
    {
        g_currentFrequency += g_tabStep[g_stepIndex] * g_encoderCount;
    }

    uint16_t bMin = g_bandList[g_bandIndex].minimumFreq;
    uint16_t bMax = g_bandList[g_bandIndex].maximumFreq;

    if (g_currentFrequency > bMax) g_currentFrequency = bMin;
    else if (g_currentFrequency < bMin) g_currentFrequency = bMax;

    g_bandList[g_bandIndex].currentFreq = g_currentFrequency;
    g_processFreqChange = true;
    g_lastFreqChange = millis();

    showFrequency();
}

void doFrequencyTuneSSB()
{
    const int BFOMax = 16000;
    int step = (g_encoderCount > 0) ? getSteps() : -getSteps();
    int newBFO = g_currentBFO + step;

    if (newBFO > BFOMax) {
        g_currentFrequency += (newBFO / 1000);
        newBFO %= 1000;
    } else if (newBFO < -BFOMax) {
        g_currentFrequency -= (abs(newBFO) / 1000);
        newBFO %= 1000;
    }

    g_currentBFO = newBFO;
    updateBFO();
    g_processFreqChange = true;
    g_lastFreqChange = millis();
    showFrequency();
}

// --- Callbacks de la Estructura Settings ---
void doAttenuation(int8_t v) {
    doSwitchLogic(g_Settings[ATT].param, 0, 37, v);
    g_si4735.setAutomaticGainControl(0, g_Settings[ATT].param);
}

void doSoftMute(int8_t v) {
    doSwitchLogic(g_Settings[SoftMute].param, 0, 32, v);
    g_si4735.setAmSoftMuteMaxAttenuation(g_Settings[SoftMute].param);
}

void doBrightness(int8_t v) {
    doSwitchLogic(g_Settings[Brightness].param, 1, 15, v);
    oled.setContrast(g_Settings[Brightness].param * 15);
}

void doAvc(int8_t v) {
    doSwitchLogic(g_Settings[AutoVolControl].param, 0, 1, v);
    // Lógica de hardware SI4735 para AVC si aplica
}

void doStep(int8_t v) {
    int8_t tempStep = g_stepIndex; // Copiamos el valor volatile a uno normal
    int8_t maxStep = getLastStep();
    doSwitchLogic(tempStep, 0, maxStep, v);
    g_stepIndex = tempStep; // Devolvemos el valor procesado
}

void doVolume(int8_t v) {
    static int8_t currentVol = DEFAULT_VOLUME;
    currentVol += v;
    if (currentVol < 0) currentVol = 0;
    if (currentVol > 63) currentVol = 63;
    g_si4735.setVolume(currentVol);
    showVolume();
}

// Implementación de placeholders necesarios para evitar errores de linkado
void doSSBAVC(int8_t v) { doSwitchLogic(g_Settings[SVC].param, 0, 1, v); }
void doSync(int8_t v) { doSwitchLogic(g_Settings[Sync].param, 0, 1, v); }
void doDeEmp(int8_t v) { doSwitchLogic(g_Settings[DeEmp].param, 0, 1, v); }
void doSWUnits(int8_t v) { doSwitchLogic(g_Settings[SWUnits].param, 0, 1, v); }
void doSSBSoftMuteMode(int8_t v) { doSwitchLogic(g_Settings[SSM].param, 0, 1, v); }
void doCutoffFilter(int8_t v) { doSwitchLogic(g_Settings[CutoffFilter].param, 0, 1, v); updateSSBCutoffFilter(); }
void doCPUSpeed(int8_t v) { doSwitchLogic(g_Settings[CPUSpeed].param, 0, 1, v); }
void doBFOCalibration(int8_t v) { /* Lógica de calibración */ }
void doUnitsSwitch(int8_t v) { doSwitchLogic(g_Settings[UnitsSwitch].param, 0, 1, v); }
void doScanSwitch(int8_t v) { doSwitchLogic(g_Settings[ScanSwitch].param, 0, 1, v); }
void doCWSwitch(int8_t v) { doSwitchLogic(g_Settings[CWSwitch].param, 0, 1, v); }
