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

// --- Instancias de Botones (Clase definida en Utils.h) ---
Button btnVolume(VOLUME_BUTTON);
Button btnStep(STEP_BUTTON);
Button btnBand(BAND_BUTTON);
Button btnMode(MODE_SWITCH);
Button btnBw(BANDWIDTH_BUTTON);
Button btnAvc(AGC_BUTTON);
Button btnSoftMute(SOFTMUTE_BUTTON);

// --- Manejo del Encoder por Interrupción ---
// Esta función se ejecuta cada vez que giras el encoder
void readEncoder() {
  static uint8_t old_AB = 0;
  static int8_t enc_states[] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};
  
  old_AB <<= 2;                   // Guardar estado anterior
  old_AB |= (digitalRead(ENCODER_PIN_A) << 1) | digitalRead(ENCODER_PIN_B);
  int8_t diff = enc_states[(old_AB & 0x0f)];
  
  if (diff != 0) {
    g_encoderCount = diff;
  }
}

// --- Configuración Inicial ---
void setup() {
    // 1. Inicializar Pantalla OLED
    oled.begin();
    oled.clear();
    oled.on();
    oled.switchRenderFrame();
    
    oledPrint("ATS-EX v1.0", 25, 0, DEFAULT_FONT);
    oledPrint("SISTEMA LISTO", 20, 3, DEFAULT_FONT);
    
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
    
    delay(1000);
    oled.clear();
    showStatus(true);
}

// --- Bucle Principal ---
void loop() {
    
    // --- 1. PROCESAR ENCODER ---
    if (g_encoderCount != 0) {
        if (g_settingsActive) {
            // Si estamos dentro del menú de ajustes
            g_Settings[g_SettingSelected].manipulateCallback(g_encoderCount);
            showSettings();
        } 
        else if (g_cmdVolume) {
            // Si el modo volumen está activo (por pulsación de botón)
            doVolume(g_encoderCount);
        } 
        else {
            // Sintonización normal de frecuencia
            if (isSSB()) {
                doFrequencyTuneSSB();
            } else {
                doFrequencyTune();
            }
        }
        
        g_encoderCount = 0;      // Consumir el evento
        g_storeTime = millis();  // Reiniciar temporizador de guardado
    }

    // --- 2. PROCESAR BOTONES ---
    btnVolume.checkEvent(volumeEvent);
    btnStep.checkEvent(stepEvent);
    btnBand.checkEvent(bandEvent);

    // Lógica del botón del encoder para entrar/salir de SETTINGS
    if (digitalRead(ENCODER_BUTTON) == LOW) {
        delay(200); // Debounce
        g_settingsActive = !g_settingsActive;
        
        if (g_settingsActive) {
            g_SettingSelected = 0;
            showSettings();
        } else {
            showStatus(true);
        }
        
        // Esperar a que suelte el botón
        while(digitalRead(ENCODER_BUTTON) == LOW);
    }

    // --- 3. ACTUALIZACIONES DE PANTALLA (Background) ---
    // Solo actualizamos el S-Meter si no estamos en el menú
    if (!g_settingsActive) {
        static uint32_t lastUpdate = 0;
        if (millis() - lastUpdate > 250) {
            showSMeter();
            
            #if USE_RDS
            if (g_currentMode == FM) {
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
        // saveSettingsToEeprom(); // Implementar si deseas persistencia
        g_storeTime = 0;
    }
}
