#pragma once
#include <Arduino.h>

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

void resetEEPROM();
void saveState();
