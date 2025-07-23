// файл HiHatPedal.cpp 

#include "HiHatPedal.h"
#include "MIDISender.h"
#include "Config.h"

uint8_t lastHiHatValue = 0;
unsigned long lastHiHatCCSend = 0;

uint8_t readHiHatValueRaw() {
    int raw = analogRead(HIHAT_PEDAL_PIN);
    return raw;
}

void processHiHatPedal(const HiHatSettings &hihatSettings) {
    int raw = readHiHatValueRaw();
    if(hihatSettings.invert) raw = 1023 - raw;
    uint8_t val = map(raw, 0, 1023, hihatSettings.ccClosed, hihatSettings.ccOpen);
    val = constrain(val, 0, 127);
    if(val != lastHiHatValue) {
        midiSendCC(9, 4, val);
        lastHiHatValue = val;
        lastHiHatCCSend = millis();
    }
}