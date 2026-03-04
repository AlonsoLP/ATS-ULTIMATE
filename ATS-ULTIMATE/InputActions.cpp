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
uint8_t volPlusEvent(uint8_t event, uint8_t pin) {
    if (event == BUTTONEVENT_SHORTPRESS) doVolume(1);
    else if (event == BUTTONEVENT_FIRSTLONGPRESS || event == BUTTONEVENT_LONGPRESS) {
        doVolume(2); // Sube más rápido
    }
    return event;
}

uint8_t volMinusEvent(uint8_t event, uint8_t pin) {
    if (event == BUTTONEVENT_SHORTPRESS) {
        g_muteVolume = !g_muteVolume;
        if (g_muteVolume) {
            g_si4735.setVolume(0);
        } else {
            doVolume(0); // Restaura el volumen anterior
        }
        showStatus(true);
    }
    return event;
}

uint8_t bandPlusEvent(uint8_t event, uint8_t pin) {
    if (g_settingsActive) {
        // Cambiar de página en ajustes
        g_SettingsPage++;
        if (g_SettingsPage * 3 >= SETTINGS_MAX) g_SettingsPage = 0;
        showSettings();
    } else {
        if (event == BUTTONEVENT_SHORTPRESS) {
            // Entrar en modo selección de banda con encoder (puedes implementarlo)
            doBand(1);
        } else if (event == BUTTONEVENT_FIRSTLONGPRESS || event == BUTTONEVENT_LONGPRESS) {
            doBand(1); // Scroll rápido
        }
    }
    return event;
}

uint8_t bandMinusEvent(uint8_t event, uint8_t pin) {
    if (event == BUTTONEVENT_SHORTPRESS) {
        g_settingsActive = !g_settingsActive;
        if (g_settingsActive) showSettings();
        else {
            saveState(); // Guardar al cerrar
            showStatus(true);
        }
    } else if (event == BUTTONEVENT_FIRSTLONGPRESS || event == BUTTONEVENT_LONGPRESS) {
        if (!g_settingsActive) doBand(-1); // Scroll rápido atrás
    }
    return event;
}

uint8_t stepEvent(uint8_t event, uint8_t pin) {
    if (event == BUTTONEVENT_SHORTPRESS) doStep(1);
    else if (event == BUTTONEVENT_FIRSTLONGPRESS) {
        g_showSmeterBar = !g_showSmeterBar; // Debes declarar esta variable en State.h
        showStatus(true);
    }
    return event;
}

uint8_t agcEvent(uint8_t event, uint8_t pin) {
    if (event == BUTTONEVENT_SHORTPRESS) {
        static bool screenOn = true;
        screenOn = !screenOn;
        if (screenOn) oled.on(); else oled.off();
    } else if (event == BUTTONEVENT_FIRSTLONGPRESS && isSSB()) {
        // Lógica para SYNC (si tu librería/chip lo soporta en ese modo)
    }
    return event;
}

uint8_t modeEvent(uint8_t event, uint8_t pin) {
    if (event == BUTTONEVENT_SHORTPRESS) doMode(1);
    return event;
}

uint8_t bwEvent(uint8_t event, uint8_t pin) {
    if (event == BUTTONEVENT_SHORTPRESS) doBandwidth(1);
    return event;
}


uint8_t tuneEvent(uint8_t event, uint8_t pin) {
    if (event == BUTTONEVENT_SHORTPRESS) {
        if (g_settingsActive) {
            // Si estamos en ajustes, clic para seleccionar/confirmar
            g_isEditingSetting = !g_isEditingSetting; 
            showSettings();
        } else {
            // Lógica de Escaneo o cambio rápido de Step en SSB
            if (isSSB()) doStep(1);
	    else if (g_Settings[SettingsIndex::ScanSwitch].param == 1) {
                // Llamar a función de escaneo
            }
        }
    }
    return event;
}

// --- Sintonización de Frecuencia ---
void doFrequencyTune(int8_t v) {
    int step = getSteps();
    if (v > 0) g_currentFrequency += step;
    else g_currentFrequency -= step;
    
    g_si4735.setFrequency(g_currentFrequency);
    showFrequency(false);
}

void doFrequencyTuneSSB(int8_t v) {
    // Lógica para SSB (puedes copiar la de doFrequencyTune por ahora)
    doFrequencyTune(v);
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
