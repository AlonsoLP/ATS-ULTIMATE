#include <Arduino.h>
#include <Tiny4kOLED.h>
#include <SI4735.h>
#include <EEPROM.h>
#include "Config.h"
#include "State.h"
#include "Utils.h"
#include "RadioCtrl.h"
#include "DisplayUI.h"
#include "InputActions.h"

// --- INSTANCIAS DE BOTONES ---
Button btnVolP(VOLUME_UP_BUTTON);    // Pin 6
Button btnVolM(VOLUME_DN_BUTTON);    // Pin 7
Button btnBandP(BAND_UP_BUTTON);     // Pin 8
Button btnBandM(BAND_DN_BUTTON);     // Pin 9
Button btnStep(STEP_BUTTON);        // Pin 10
Button btnMode(MODE_SWITCH);        // Pin 4
Button btnBw(BANDWIDTH_BUTTON);     // Pin 5
Button btnAgc(AGC_BUTTON);          // Pin 11
Button btnTune(ENCODER_BUTTON);     // Pin 14 (A0)

// --- Manejo del Encoder por Interrupción ---
// Esta función se ejecuta cada vez que giras el encoder
void readEncoder() {
    static uint8_t old_AB = 3;
    static const int8_t enc_states[] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};
    old_AB <<= 2;
    old_AB |= ((digitalRead(ENCODER_PIN_A) << 1) | digitalRead(ENCODER_PIN_B)) & 0x03;
    int8_t diff = enc_states[old_AB & 0x0f];
#ifdef ENCODER_REVERSED
    if (diff != 0) g_encoderCount -= diff;
#else
    if (diff != 0) g_encoderCount += diff;
#endif
}

// --- Configuración Inicial ---
void setup() {
    // 0. Inicializar I2C
    Wire.begin();           
    Wire.setClock(400000);  // Aceleramos a 400 kHz (Fast Mode)

    // 1. Inicializar Pantalla OLED
    oled.begin();
    oled.clear();
    oled.on();
    oled.switchRenderFrame();

    // Lógica de Reset EEPROM (Botón pulsado al encender)
    if (digitalRead(ENCODER_BUTTON) == LOW) {
        oled.clear();
        oledPrint(F("EEPROM RESET"), 20, 2, DEFAULT_FONT);
        resetEEPROM();
        delay(2000);
    }

    oledPrint(F("ATS-EX v1.0"), 25, 0, DEFAULT_FONT);
    oledPrint(F("SISTEMA LISTO"), 20, 3, DEFAULT_FONT);
    
    // 2. Inicializar Radio (Chip SI4735)
    // Usamos el PIN de RESET definido en Config.h
    g_si4735.setup(RESET_PIN, MW_BAND_TYPE);
    delay(500);

    // 3. Configurar Pines del Encoder e Interrupciones
    pinMode(ENCODER_PIN_A, INPUT_PULLUP);
    pinMode(ENCODER_PIN_B, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), readEncoder, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_B), readEncoder, CHANGE);

    // 4. Configurar Botón del Encoder
    pinMode(ENCODER_BUTTON, INPUT_PULLUP);

    // 5. Cargar Banda por Defecto (MW)
    g_bandIndex = 1; 
    applyBandConfiguration();

    // 6. Carga desde EEPROM
    loadState();

    applyBandConfiguration();
    g_si4735.setVolume(g_savedVolume);

    delay(1000);
    oled.clear();
    showStatus(true);
}

void loop() {

    // --- 1. PROCESAR ENCODER ---
    int8_t rotacion = 0;

    // Extracción segura (Lectura Atómica)
    // Pausamos las interrupciones un microsegundo para copiar el valor sin que se corrompa
    noInterrupts(); 
    if (g_encoderCount != 0) {
        rotacion = g_encoderCount;
        g_encoderCount = 0; // Consumimos el evento aquí de forma segura
    }
    interrupts(); // Volvemos a encender las interrupciones

    // Ahora usamos nuestra copia local 'rotacion' en lugar de la variable global
    if (rotacion != 0) {
	if (g_bandSelectMode) {
	    doBand(rotacion > 0 ? 1 : -1);
	    showBandTag();
	} else if (g_settingsActive) {
	    if (g_isEditingSetting) {
    		// Modo edición: cambia el valor del setting seleccionado
    		if (g_Settings[g_SettingSelected].manipulateCallback != NULL) {
        	    g_Settings[g_SettingSelected].manipulateCallback(rotacion);
    		}
	    } else {
    		// Modo navegación: mueve el cursor entre settings
    		g_SettingSelected += rotacion;
    		// Limitar al rango de la página actual
    		int8_t pageStart = g_SettingsPage * 3;
    		int8_t pageEnd   = pageStart + 2;
    		if (pageEnd >= SETTINGS_MAX) pageEnd = SETTINGS_MAX - 1;
    		if (g_SettingSelected < pageStart) g_SettingSelected = pageEnd;
    		if (g_SettingSelected > pageEnd)   g_SettingSelected = pageStart;
	    }
	    showSettings();
        } 
        else if (g_cmdVolume) {
            // Si el modo volumen está activo (por pulsación de botón)
            doVolume(rotacion);
        } 
        else {
            // Sintonización normal de frecuencia
            if (isSSB()) {
                doFrequencyTuneSSB(rotacion);
            } else {
                doFrequencyTune(rotacion);
            }
        }
        
        g_storeTime = millis(); // Reiniciar temporizador de guardado
    }

    // --- 2. PROCESAR BOTONES ---
    btnVolP.checkEvent(volPlusEvent);
    btnVolM.checkEvent(volMinusEvent);
    btnBandP.checkEvent(bandPlusEvent);
    btnBandM.checkEvent(bandMinusEvent);
    btnStep.checkEvent(stepEvent);
    btnMode.checkEvent(modeEvent);
    btnBw.checkEvent(bwEvent);
    btnAgc.checkEvent(agcEvent);
    btnTune.checkEvent(tuneEvent);

    // --- 3. ACTUALIZACIONES DE PANTALLA (Background) ---
    // Solo actualizamos el S-Meter si no estamos en el menú
    if (!g_settingsActive) {
        static uint32_t lastUpdate = 0;
        if (millis() - lastUpdate > 250) {
	    if (g_showSmeterBar) showSMeter();
	    else oledPrint(F("                "), 0, 7, DEFAULT_FONT); // limpiar fila si está desactivado

	    if (g_scanning && (g_currentMode == FM || g_currentMode == AM)) {
		doScan();
	    }
#if USE_RDS
	    if (g_currentMode == FM && g_displayRDS) {
                g_si4735.getRdsStatus();
                showRDS();
            }
#endif
            lastUpdate = millis();
        }
    }

    // --- 4. GESTIÓN DE ENERGÍA Y GUARDADO ---
    // Si han pasado 10 segundos (STORE_TIME) desde el último cambio, podrías guardar en EEPROM
    if (g_storeTime > 0 && (millis() - g_storeTime > STORE_TIME)) {
	saveState();
        g_storeTime = 0;
    }
}
