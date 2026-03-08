/**
 * @file    InputActions.cpp
 * @brief   Procesamiento de botones, encoder y callbacks de configuración.
 * @author  Alonso José Lara Plana (EA7LBT)
 * @license MIT — ver ATS-ULTIMATE.ino para texto completo
 */
#include "InputActions.h"
#include "State.h"
#include "RadioCtrl.h"
#include "DisplayUI.h"
#include "Utils.h"

// --- Instancias de botones ---
static Button s_btnVolP(VOLUME_UP_BUTTON);
static Button s_btnVolM(VOLUME_DN_BUTTON);
static Button s_btnBandP(BAND_UP_BUTTON);
static Button s_btnBandM(BAND_DN_BUTTON);
static Button s_btnStep(STEP_BUTTON);
static Button s_btnMode(MODE_SWITCH);
static Button s_btnBw(BANDWIDTH_BUTTON);
static Button s_btnAgc(AGC_BUTTON);
static Button s_btnTune(ENCODER_BUTTON);

// --- Helper interno para lógica de conmutación ---
inline void doSwitchLogic(int8_t& param, int8_t low, int8_t high, int8_t step)
{
    param += step;
    if (param < low) param = high;
    else if (param > high) param = low;
}

// --- Eventos de Botones ---
uint8_t volPlusEvent(uint8_t event, uint8_t pin)
{
    if (event == BUTTONEVENT_SHORTPRESS) {
        g_cmdVolume = !g_cmdVolume;  // toggle modo volumen
        showVolumeBar();
    } else if (event == BUTTONEVENT_FIRSTLONGPRESS || event == BUTTONEVENT_LONGPRESS) {
        g_cmdVolume = false;
        doVolume(2);
    }
    return event;
}

uint8_t volMinusEvent(uint8_t event, uint8_t pin)
{
    if (event == BUTTONEVENT_SHORTPRESS) {
        g_muteVolume = !g_muteVolume;
        if (g_muteVolume) {
            g_savedVolume = g_si4735.getVolume();
            g_si4735.setVolume(0);
        } else {
            g_si4735.setVolume(g_savedVolume);
        }
        showVolumeBar();  // ← una sola llamada, fuera del if/else
    }
    return event;
}

uint8_t bandPlusEvent(uint8_t event, uint8_t pin)
{
    if (g_settingsActive) {
        g_SettingSelected = ((g_SettingSelected / 4) + 1) * 4;
        if (g_SettingSelected >= SETTINGS_MAX) g_SettingSelected = 0;
        showSettings();
    } else {
        if (g_memoryMode && (event == BUTTONEVENT_FIRSTLONGPRESS || event == BUTTONEVENT_LONGPRESS)) {
            MemoryChannel mem;
            mem.freq = g_currentFrequency;
            mem.bandIdx = g_bandIndex;
            mem.mode = g_currentMode;
            if (g_currentMode == FM) mem.bwIdx = g_bwIndexFM;
            else if (isSSB()) mem.bwIdx = g_bwIndexSSB;
            else mem.bwIdx = g_bwIndexAM;

            saveMemory(g_memoryIndex, mem);
            g_previewMemory = mem; 
            
            oled.invertOutput(true);
            delay(100);
            oled.invertOutput(false);
            return event;
        }

        if (event == BUTTONEVENT_SHORTPRESS) {
	    g_bandSelectMode = !g_bandSelectMode;
	    if (g_bandSelectMode) {
    		// Mostrar indicador de selección de banda
    		showBandTag();
	    } else {
    		applyBandConfiguration();
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

uint8_t bandMinusEvent(uint8_t event, uint8_t pin)
{
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
    if (event == BUTTONEVENT_SHORTPRESS) {
        doStep(1);
        g_lastDoublePress = millis();
    } else if (event == BUTTONEVENT_FIRSTLONGPRESS) {
        g_showSmeterBar = !g_showSmeterBar;
        showStatus(true);
    } else if (event == BUTTONEVENT_LONGPRESS && (millis() - g_lastDoublePress < 500)) {
        if (!g_keyLocked) g_bandLocked = !g_bandLocked;
        showStatus(true);
    }
    return event;
}

uint8_t agcEvent(uint8_t event, uint8_t pin)
{
    if (event == BUTTONEVENT_SHORTPRESS) {
	g_screenOn = !g_screenOn;
	if (g_screenOn) oled.on(); else oled.off();
    } else if (event == BUTTONEVENT_FIRSTLONGPRESS && isSSB()) {
	doSync(1);
	showStatus(false);
    }
    return event;
}

uint8_t modeEvent(uint8_t event, uint8_t pin)
{
    if (event == BUTTONEVENT_SHORTPRESS) {
        if (g_bandIndex == FM_IDX) {
	    g_Settings[RDS].param = g_Settings[RDS].param > 0 ? 0 : 1;
	    g_displayRDS = (g_Settings[RDS].param > 0);
	    if (!g_displayRDS) oledPrint("                ", 0, 6, DEFAULT_FONT);
        } else {
            doMode(1);
        }
    }
    return event;
}

uint8_t bwEvent(uint8_t event, uint8_t pin)
{
    if (event == BUTTONEVENT_SHORTPRESS) {
        doBandwidth(1);
        g_lastDoublePress = millis();
    } else if (event == BUTTONEVENT_LONGPRESS && (millis() - g_lastDoublePress < 500)) {
        g_snrMode = !g_snrMode;
        showStatus(true);
    } else if (event == BUTTONEVENT_FIRSTLONGPRESS) {
        g_keyLocked = !g_keyLocked;
        showLockIndicator();
    }
    return event;
}

uint8_t tuneEvent(uint8_t event, uint8_t pin)
{
    if (event == BUTTONEVENT_FIRSTLONGPRESS || event == BUTTONEVENT_LONGPRESS) {
        if (!g_settingsActive && !g_cmdVolume) {
            g_memoryMode = !g_memoryMode;
            if (g_memoryMode) {
                loadMemory(g_memoryIndex, g_previewMemory);
                oled.clear();
            } else {
                showStatus(true);
            }
        }
        return event;
    }

    if (event == BUTTONEVENT_SHORTPRESS) {
        if (g_memoryMode) {
            if (g_previewMemory.freq > 0 && g_previewMemory.freq != 0xFFFF) {
                applyMemoryState(g_previewMemory);
                g_memoryMode = false;
            }
            return event;
        }
        if (g_settingsActive) {
            g_isEditingSetting = !g_isEditingSetting; 
            showSettings();
        } else {
            if (g_cmdVolume) {
                g_cmdVolume = false;
                showStatus(false);
                return event;
            }
            if (isSSB()) {
                doStep(1);
            } else if (g_bandIndex == FM_IDX && g_displayRDS) {
                g_rdsMode = (RDSActiveInfo)((g_rdsMode + 1) % 3);
                showRDS();
            } else if (g_Settings[SettingsIndex::ScanSwitch].param == 1) {
                g_scanning = !g_scanning;
                if (!g_scanning) showStatus(false);
            }
        }
    }
    return event;
}

// --- Sintonización de Frecuencia ---
void doFrequencyTune(int8_t v)
{
    int step = getSteps();
    if (v > 0) {
        g_currentFrequency += step;
        if (g_currentFrequency > g_bandList[g_bandIndex].maximumFreq)
            g_currentFrequency = g_bandList[g_bandIndex].maximumFreq;
    } else {
        if (g_currentFrequency > g_bandList[g_bandIndex].minimumFreq + step)
            g_currentFrequency -= step;
        else
            g_currentFrequency = g_bandList[g_bandIndex].minimumFreq;
    }
    g_bandList[g_bandIndex].currentFreq = g_currentFrequency;
    g_si4735.setFrequency(g_currentFrequency);
    showFrequency(false);
    if (g_bandIndex == SW_IDX) showModulation();
}

void doFrequencyTuneSSB(int8_t v)
{
    int step = getSteps();

    if (step < 1000) {
        g_currentBFO += (v > 0) ? step : -step;

	// [] — sin captura: accede a globales directamente
        auto carrierShift = [](int8_t dir) {
            g_currentFrequency += dir;
            g_bandList[g_bandIndex].currentFreq = g_currentFrequency;
            g_si4735.setFrequency(g_currentFrequency);
        };

        if      (g_currentBFO >  BFO_MAX) { g_currentBFO -= BFO_MAX * 2; carrierShift(+1); }
        else if (g_currentBFO < -BFO_MAX) { g_currentBFO += BFO_MAX * 2; carrierShift(-1); }

        updateBFO();
        showBFO();
    } else {
        g_currentBFO = 0;
        updateBFO();
        int stepKHz = step / 1000;
        g_currentFrequency += (v > 0) ? stepKHz : -stepKHz;
        g_bandList[g_bandIndex].currentFreq = g_currentFrequency;
        g_si4735.setFrequency(g_currentFrequency);
        showFrequency(false);
	showBFO();
    }
}

void doAttenuation(int8_t v)
{
    doSwitchLogic(g_Settings[ATT].param, 0, 37, v);
    g_si4735.setAutomaticGainControl(0, g_Settings[ATT].param);
}

void doSoftMute(int8_t v)
{
    doSwitchLogic(g_Settings[SoftMute].param, 0, 32, v);
    g_si4735.setAmSoftMuteMaxAttenuation(g_Settings[SoftMute].param);
}

void doBrightness(int8_t v)
{
    doSwitchLogic(g_Settings[Brightness].param, 1, 15, v);
    oled.setContrast(g_Settings[Brightness].param * 17); // 15*17=255
}

void doAvc(int8_t v)
{
    doSwitchLogic(g_Settings[AutoVolControl].param, 0, 26, v);
    g_si4735.setAMFrontEndAgcControl(g_Settings[AutoVolControl].param, 0);
    // 0 = AGC activo con atenuación indicada, valor 0-26
}

void doStep(int8_t v)
{
    int8_t tempStep = g_stepIndex; // Copiamos el valor volatile a uno normal
    int8_t maxStep = isSSB() ? (AM_TOTAL_STEPS + SSB_TOTAL_STEPS - 1) : (AM_TOTAL_STEPS - 1);
    doSwitchLogic(tempStep, 0, maxStep, v);
    g_stepIndex = tempStep; // Devolvemos el valor procesado
}

void doVolume(int8_t v)
{
    int8_t currentVol = (int8_t)g_si4735.getVolume() + v;
    if (currentVol < 0)  currentVol = 0;
    if (currentVol > 63) currentVol = 63;
    g_si4735.setVolume((uint8_t)currentVol);
    showVolumeBar();
}

void doBFOCalibration(int8_t v)
{
    g_Settings[BFO].param += v;  // acumula offset
    g_currentBFO = g_Settings[BFO].param;
    g_si4735.setSSBBfo(g_currentBFO);
}

void doDeEmp(int8_t v)
{
    doSwitchLogic(g_Settings[DeEmp].param, 0, 1, v);
    if (g_bandIndex == FM_IDX)
        g_si4735.setFMDeEmphasis(g_Settings[DeEmp].param ? 1 : 2);
    // SI4735: 1=50µs (Europa), 2=75µs (América)
}

void doSSBSoftMuteMode(int8_t v)
{
    doSwitchLogic(g_Settings[SSM].param, 0, 1, v);
    if (isSSB())
        g_si4735.setSSBSoftMute(g_Settings[SSM].param);
}

void doCPUSpeed(int8_t v)
{
    if (g_usbPowered) return;  // bloqueado si USB conectado
    doSwitchLogic(g_Settings[CPUSpeed].param, 0, 3, v);
    applyCPUSpeed(g_Settings[CPUSpeed].param);
}

void doCWSwitch(int8_t v)
{
    doSwitchLogic(g_Settings[CWSwitch].param, 0, 1, v);
    // Si se desactiva CW estando en CW, doMode se encarga de salir
    if (g_currentMode == CW && g_Settings[CWSwitch].param == 0)
        doMode(1);
}

void doSWUnits(int8_t v)
{
    doSwitchLogic(g_Settings[SWUnits].param, 0, 1, v);
    showStep();  // refrescar inmediatamente
}

void doUnitsSwitch(int8_t v)
{
    doSwitchLogic(g_Settings[UnitsSwitch].param, 0, 1, v);
    showFrequency(false);  // refrescar con nueva unidad
}

void doSync(int8_t v)
{
    doSwitchLogic(g_Settings[Sync].param, 0, 1, v);
    if (isSSB())
        g_si4735.setSSBAutomaticVolumeControl(g_Settings[Sync].param);
}

void doCutoffFilter(int8_t v)
{
    doSwitchLogic(g_Settings[CutoffFilter].param, 0, 1, v);
    updateSSBCutoffFilter();
}

void doSSBAVC(int8_t v)
{
    doSwitchLogic(g_Settings[SVC].param, 0, 1, v);
    if (isSSB())
        g_si4735.setSSBAvcDivider(g_Settings[SVC].param ? 3 : 0);
        // 0 = AVC desactivado, 3 = divisor estándar (valor recomendado por pu2clr)
}

void doScanSwitch(int8_t v)
{
    doSwitchLogic(g_Settings[ScanSwitch].param, 0, 1, v);
}

void doRDS(int8_t v)
{
    doSwitchLogic(g_Settings[RDS].param, 0, 3, v);
    g_displayRDS = (g_Settings[RDS].param > 0);  // 0=off, 1-3=on con umbral
    if (g_displayRDS)
        setRDSConfig(g_Settings[RDS].param);  // pasar umbral al chip
    else
        oledPrint("                ", 0, 6, DEFAULT_FONT);
}

void doDefaultVolume(int8_t v)
{
    doSwitchLogic(g_Settings[DefaultVol].param, 0, 63, v);
    g_si4735.setVolume(g_Settings[DefaultVol].param);
    g_savedVolume = g_Settings[DefaultVol].param;
}

void doStepUnits(int8_t v)
{
    doSwitchLogic(g_Settings[StepUnits].param, 0, 1, v);
    showStep();
}

void doCWSide(int8_t v)
{
    doSwitchLogic(g_Settings[CWSide].param, 0, 1, v);
    // Si estamos en CW ahora mismo, aplicar inmediatamente
    if (g_currentMode == CW) {
        g_si4735.setSSB(
            g_bandList[g_bandIndex].minimumFreq,
            g_bandList[g_bandIndex].maximumFreq,
            g_currentFrequency,
            (int)pgm_read_word(&g_tabStep[g_stepIndex]),
            g_Settings[CWSide].param == 0 ? 1 : 2  // LSB=1, USB=2
        );
    }
}

// Tabla de estados del encoder en PROGMEM — 16 bytes Flash, 0 RAM
static const int8_t s_encStates[] PROGMEM = { 0,-1,1,0, 1,0,0,-1, -1,0,0,1, 0,1,-1,0 };

void readEncoder()
{
    static uint8_t old_AB = 3;

    // Lectura directa de registro — 2 instrucciones vs ~20 de digitalRead()
    // PIND contiene los pines 0-7, ENCODER_PIN_A=2 y ENCODER_PIN_B=3
    uint8_t ab = (PIND >> 2) & 0x03;  // bits 2 y 3 → posición 1 y 0

    old_AB = ((old_AB << 2) | ab) & 0x0F;
    int8_t diff = (int8_t)pgm_read_byte(&s_encStates[old_AB]);

    if (diff != 0) {
#ifdef ENCODER_REVERSED
        g_encoderCount -= diff;
#else
        g_encoderCount += diff;
#endif
    }
}

void setupEncoder()
{
    pinMode(ENCODER_PIN_A, INPUT_PULLUP);
    pinMode(ENCODER_PIN_B, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), readEncoder, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_B), readEncoder, CHANGE);
}

void processButtons()
{
    if (g_keyLocked) {
        s_btnBw.checkEvent(bwEvent);  // único botón activo para desbloquear
        return;
    }
    s_btnVolP.checkEvent(volPlusEvent);
    s_btnVolM.checkEvent(volMinusEvent);
    s_btnBandP.checkEvent(bandPlusEvent);
    s_btnBandM.checkEvent(bandMinusEvent);
    s_btnStep.checkEvent(stepEvent);
    s_btnMode.checkEvent(modeEvent);
    s_btnBw.checkEvent(bwEvent);
    s_btnAgc.checkEvent(agcEvent);
    s_btnTune.checkEvent(tuneEvent);
}

void processEncoder()
{
    if (g_keyLocked) return;

    int8_t rotacion = 0;
    noInterrupts();
    if (g_encoderCount != 0) {
        rotacion = g_encoderCount;
        g_encoderCount = 0;
    }
    interrupts();

    if (rotacion == 0) return;

    if (g_memoryMode) {
        g_memoryIndex += rotacion;
        if (g_memoryIndex < 0) g_memoryIndex = MAX_MEMORIES - 1;
        if (g_memoryIndex >= MAX_MEMORIES) g_memoryIndex = 0;
        loadMemory(g_memoryIndex, g_previewMemory);
        return; // Retornamos sin actualizar g_storeTime
    }

    // --- Aceleración de encoder ---
    static uint32_t lastEncTime = 0;
    static int8_t   lastDir     = 0;
    uint32_t now    = millis();
    int8_t   accel  = ((now - lastEncTime) < 80 && (rotacion > 0) == (lastDir > 0)) ? 5 : 1;
    lastEncTime     = now;
    lastDir         = rotacion;

    if (g_bandSelectMode) {
        doBand(rotacion > 0 ? 1 : -1);
        showBandTag();
    } else if (g_settingsActive) {
        if (g_isEditingSetting) {
            if (g_Settings[g_SettingSelected].manipulateCallback != NULL)
                g_Settings[g_SettingSelected].manipulateCallback(rotacion);
        } else {
            g_SettingSelected += rotacion;
            if (g_SettingSelected < 0) g_SettingSelected = SETTINGS_MAX - 1;
            if (g_SettingSelected >= SETTINGS_MAX) g_SettingSelected = 0;
        }
        showSettings();
    } else if (g_cmdVolume) {
        doVolume(rotacion);
    } else {
	if (!g_bandLocked) {
    	    doBand(rotacion > 0 ? 1 : -1);  // Cambio banda normal
	} else {
    	    if (isSSB()) doFrequencyTuneSSB(rotacion * accel);
    	    else         doFrequencyTune(rotacion * accel);
	}
    }
    g_storeTime = millis();
}
