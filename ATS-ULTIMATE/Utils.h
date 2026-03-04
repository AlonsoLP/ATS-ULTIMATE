#pragma once
#include <Arduino.h>
#include <Tiny4kOLED_common.h> // Esto incluye las definiciones de tipos como DCfont sin crear el objeto oled

// Declaramos que oled existe en otro sitio con su tipo real
extern SSD1306PrintDevice oled;

// --- Macros de Control de Eventos ---
#define BUTTONEVENT_SHORTPRESS      1
#define BUTTONEVENT_LONGPRESS       2
#define BUTTONEVENT_LONGPRESSDONE   3
#define BUTTONEVENT_FIRSTLONGPRESS  4
#define BUTTONEVENT_ISLONGPRESS(e)  (e == 2 || e == 3 || e == 4)
#define BUTTONEVENT_ISDONE(e)       (e == 1 || e == 3)

class Button {
  private:
    uint8_t pin;
    uint32_t lastCheck;
    bool lastState;
    bool longPressSent;

  public:
    Button(uint8_t p) : pin(p), lastCheck(0), lastState(HIGH), longPressSent(false) {
        pinMode(pin, INPUT_PULLUP);
    }

    uint8_t checkEvent(uint8_t (*callback)(uint8_t, uint8_t)) {
        bool currentState = digitalRead(pin);
        uint32_t now = millis();
        uint8_t event = 0;

        if (currentState != lastState) {
            if (now - lastCheck > 50) {
                if (currentState == HIGH) {
                    if (!longPressSent) event = BUTTONEVENT_SHORTPRESS;
                    else event = BUTTONEVENT_LONGPRESSDONE;
                    longPressSent = false;
                }
                lastState = currentState;
                lastCheck = now;
            }
        } else if (currentState == LOW && !longPressSent && (now - lastCheck > 500)) {
            event = BUTTONEVENT_FIRSTLONGPRESS;
            longPressSent = true;
        }

        if (event > 0 && callback) return callback(event, pin);
        return event;
    }
};

// --- Funciones de Pantalla ---
inline void oledPrint(const char* text, uint8_t x, uint8_t y, const DCfont* font = FONT6X8, bool invert = false) {
    oled.setFont(font);
    oled.setCursor(x, y);
    if (invert) {
        // Lógica de inversión si tu librería la soporta, o simplemente:
    }
    oled.print(text);
}
