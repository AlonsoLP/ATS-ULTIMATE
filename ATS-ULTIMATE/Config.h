#pragma once
#include <Arduino.h>
#include <Tiny4kOLED_common.h> // Esto define los tipos y fuentes, pero NO crea el objeto 'oled'
extern SSD1306PrintDevice oled; // Avisamos que la 'oled' real está en otro sitio

// --- Pines de Hardware (Ajustado para ATS-20+) ---
#define RESET_PIN                   12
#define ENCODER_PIN_A                2
#define ENCODER_PIN_B                3
#define ENCODER_BUTTON              14  // Pin A0 (Botón del sintonizador)

// Botones Fila Superior
#define MODE_SWITCH                  4
#define BANDWIDTH_BUTTON             5
#define STEP_BUTTON                 10
#define AGC_BUTTON                  11

// Botones Fila Inferior (Volumen y Banda)
#define VOLUME_UP_BUTTON             6  // VOL+
#define VOLUME_DN_BUTTON             7  // VOL- (Antes causaba error como AVC_BUTTON)
#define BAND_UP_BUTTON               8  // BAND+
#define BAND_DN_BUTTON               9  // BAND- (Antes SOFTMUTE_BUTTON)

// --- Configuración Visual ---
#define DEFAULT_FONT                FONT8X16P
#define STORE_TIME                  10000      // 10 segundos para EEPROM
#define DEFAULT_VOLUME              30

// --- Definiciones de Banda (Lógica SI4735) ---
#define AM_BAND_TYPE                0
#define MW_BAND_TYPE                1
#define SW_BAND_TYPE                2
#define FM_BAND_TYPE                3

// --- Estructuras de Datos ---
struct Band {
    uint16_t minimumFreq;
    uint16_t maximumFreq;
    uint16_t currentFreq;
    int8_t currentStepIdx;
    int8_t bandwidthIdx;
};

struct Bandwidth {
    uint8_t idx;
    const char* desc;
};

enum SettingType { Num, Switch };

// Estructura de 6 parámetros para el menú de ajustes
struct SettingsItem {
    const char* name;
    int8_t param;
    int8_t min;
    int8_t max;
    SettingType type;
    void (*manipulateCallback)(int8_t);
};

// --- Límites de Frecuencia (Sistema Internacional - kHz) ---
#define LW_LIMIT_LOW                150
#define MW_LIMIT_LOW                522
#define MW_LIMIT_HIGH               1710
#define SW_LIMIT_LOW                1711
#define SW_LIMIT_HIGH               30000
#define CB_LIMIT_LOW                26965
#define FM_LIMIT_LOW                6400    // 64.00 MHz
#define FM_LIMIT_HIGH               10800   // 108.00 MHz

// --- Configuración de Funciones ---
#define USE_RDS                     1

// --- Notas del Sistema Internacional (SI) ---
// Frecuencias en kHz (AM/SW) y MHz (FM)
// Temperaturas (si se añaden) en °C
