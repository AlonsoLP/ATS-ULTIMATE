#include "InputActions.h"
#include "State.h"
#include "RadioCtrl.h"
#include "DisplayUI.h"

// --- Helper interno para lógica de conmutación ---
inline void doSwitchLogic(int8_t& param, int8_t low, int8_t high, int8_t step)
{
    param += step;
    if (param < low) param = high;
    else if (param > high) param = low;
}

// --- Eventos de Botones ---
uint8_t volPlusEvent(uint8_t event, uint8_t pin) {
    if (event == BUTTONEVENT_SHORTPRESS) {
        g_cmdVolume = !g_cmdVolume;  // toggle modo volumen
        showVolume();
    } else if (event == BUTTONEVENT_FIRSTLONGPRESS || event == BUTTONEVENT_LONGPRESS) {
        g_cmdVolume = false;
        doVolume(2);
    }
    return event;
}

uint8_t volMinusEvent(uint8_t event, uint8_t pin) {
    if (event == BUTTONEVENT_SHORTPRESS) {
        g_muteVolume = !g_muteVolume;
	if (g_muteVolume) {
	    g_savedVolume = g_si4735.getVolume(); // guardar antes de silenciar
	    g_si4735.setVolume(0);
    	    showStatus(true);
	} else {
	    g_si4735.setVolume(g_savedVolume);    // restaurar el guardado
	    showVolume();
	}
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
	    g_bandSelectMode = !g_bandSelectMode;
	    if (g_bandSelectMode) {
    		// Mostrar indicador de selección de banda
    		showBandTag();
	    } else {
    		applyBandConfiguration();
    		showStatus(true);
	    }
        } else if (event == BUTTONEVENT_FIRSTLONGPRESS || event == BUTTONEVENT_LONGPRESS) {
	    if (g_bandIndex == SW_IDX) {
    		doSWSubBand(1);  // En SW: navegar subbandas
	    } else {
    		doBand(1);       // En otras bandas: cambio de banda normal
	    }
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
	g_screenOn = !g_screenOn;
	if (g_screenOn) oled.on(); else oled.off();

    } else if (event == BUTTONEVENT_FIRSTLONGPRESS && isSSB()) {
	doSwitchLogic(g_Settings[SettingsIndex::Sync].param, 0, 1, 1);
	if (g_Settings[SettingsIndex::Sync].param)
    	    g_si4735.setSSBAutomaticVolumeControl(1);
	else
    	    g_si4735.setSSBAutomaticVolumeControl(0);
	showStatus(false);
    }
    return event;
}

uint8_t modeEvent(uint8_t event, uint8_t pin) {
    if (event == BUTTONEVENT_SHORTPRESS) {
        if (g_bandIndex == FM_IDX) {
#if USE_RDS
            // En FM: activar/desactivar RDS
            g_displayRDS = !g_displayRDS;
            if (!g_displayRDS)
                oledPrint("                ", 0, 6, DEFAULT_FONT);
            else
                showRDS();
#endif
        } else {
            doMode(1);
        }
    }
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
	    if (g_cmdVolume) {
		g_cmdVolume = false;  // confirmar y salir del modo volumen
		showStatus(false);
		return event;
	    }
	    if (isSSB()) {
		doStep(1);
	    } else if (g_bandIndex == FM_IDX && g_displayRDS) {
#if USE_RDS
		// Ciclar entre los 3 modos RDS
		g_rdsMode = (RDSActiveInfo)((g_rdsMode + 1) % 3);
		showRDS();
#endif
	    } else if (g_Settings[SettingsIndex::ScanSwitch].param == 1) {
		g_scanning = !g_scanning;
		if (!g_scanning) showStatus(false); // Al parar, refrescar pantalla
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
    g_bandList[g_bandIndex].currentFreq = g_currentFrequency;
    g_si4735.setFrequency(g_currentFrequency);
    showFrequency(false);
    if (g_bandIndex == SW_IDX) showModulation();
}

void doFrequencyTuneSSB(int8_t v) {
    int step = getSteps(); // En SSB devuelve Hz para pasos finos

    if (step < 1000) {
        // Paso fino: mover BFO
        g_currentBFO += (v > 0) ? step : -step;

	// A 13000 (reserva 3383 Hz para calibración + CW)
	if (g_currentBFO > BFO_MAX) {
	    g_currentBFO -= BFO_MAX * 2;
            g_currentFrequency++;
            g_si4735.setFrequency(g_currentFrequency);
            g_bandList[g_bandIndex].currentFreq = g_currentFrequency;
	} else if (g_currentBFO < -BFO_MAX) {
	    g_currentBFO += BFO_MAX * 2;
            g_currentFrequency--;
            g_si4735.setFrequency(g_currentFrequency);
            g_bandList[g_bandIndex].currentFreq = g_currentFrequency;
        }
	updateBFO();
    } else {
        // Paso grueso: mover frecuencia portadora y resetear BFO
        g_currentBFO = 0;
	updateBFO();
        int stepKHz = step / 1000;
        if (v > 0) g_currentFrequency += stepKHz;
        else       g_currentFrequency -= stepKHz;
        g_si4735.setFrequency(g_currentFrequency);
        g_bandList[g_bandIndex].currentFreq = g_currentFrequency;
    }
    showFrequency(false);
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
    g_si4735.setAMFrontEndAgcControl(g_Settings[AutoVolControl].param ? 0 : 1, 0);
}

void doStep(int8_t v) {
    int8_t tempStep = g_stepIndex; // Copiamos el valor volatile a uno normal
    int8_t maxStep = getLastStep();
    doSwitchLogic(tempStep, 0, maxStep, v);
    g_stepIndex = tempStep; // Devolvemos el valor procesado
}

void doVolume(int8_t v) {
    int8_t currentVol = (int8_t)g_si4735.getVolume() + v;
    if (currentVol < 0)  currentVol = 0;
    if (currentVol > 63) currentVol = 63;
    g_si4735.setVolume((uint8_t)currentVol);
    showVolume();
}

void doBFOCalibration(int8_t v) {
    g_Settings[BFO].param += v;  // acumula offset
    g_currentBFO = g_Settings[BFO].param;
    g_si4735.setSSBBfo(g_currentBFO);
}

void doSSBAVC(int8_t v) { doSwitchLogic(g_Settings[SVC].param, 0, 1, v); }
void doSync(int8_t v) { doSwitchLogic(g_Settings[Sync].param, 0, 1, v); }
void doDeEmp(int8_t v) { doSwitchLogic(g_Settings[DeEmp].param, 0, 1, v); }
void doSWUnits(int8_t v) { doSwitchLogic(g_Settings[SWUnits].param, 0, 1, v); }
void doSSBSoftMuteMode(int8_t v) { doSwitchLogic(g_Settings[SSM].param, 0, 1, v); }
void doCutoffFilter(int8_t v) { doSwitchLogic(g_Settings[CutoffFilter].param, 0, 1, v); updateSSBCutoffFilter(); }
void doCPUSpeed(int8_t v) { doSwitchLogic(g_Settings[CPUSpeed].param, 0, 1, v); }
void doUnitsSwitch(int8_t v) { doSwitchLogic(g_Settings[UnitsSwitch].param, 0, 1, v); }
void doScanSwitch(int8_t v) { doSwitchLogic(g_Settings[ScanSwitch].param, 0, 1, v); }
void doCWSwitch(int8_t v) { doSwitchLogic(g_Settings[CWSwitch].param, 0, 1, v); }
