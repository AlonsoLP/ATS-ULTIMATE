#include <EEPROM.h>
#include "State.h"

void resetEEPROM() {
    EEPROM.write(0, 0); 
}

void saveState() {
}
