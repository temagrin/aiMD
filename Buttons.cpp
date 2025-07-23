// файл  Buttons.cpp

#include "Buttons.h"
#include "Config.h"

static int8_t lastButtonState = -1;
static unsigned long lastButtonChange = 0;

void updateButtonState(int8_t &buttonState) {
    int val = analogRead(BUTTONS_PIN);
    int8_t current = 0;
    if(val > 1000) current = 0;          // No button
    else if(val < 50) current = 4;       // Right
    else if(val < 200) current = 1;      // Up
    else if(val < 400) current = 3;      // Down
    else if(val < 600) current = 2;      // Left
    else if(val < 800) current = 5;      // Select

    if(current != lastButtonState) {
        lastButtonChange = millis();
    }
    if((millis() - lastButtonChange) > BUTTON_DEBOUNCE_MS) {
        buttonState = current;
    }
    lastButtonState = current;
}