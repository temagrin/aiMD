// файл HiHatPedal.cpp
#include "HiHatPedal.h"
#include "MIDISender.h"
#include "Config.h"
bool isHiHatClosed = false; 
uint8_t lastHiHatValue = 0;
unsigned long lastHiHatCCSend = 0;

uint16_t lastRawValue = 0;
unsigned long lastHitTime = 0;

uint8_t readHiHatValueRaw() {
    int raw = analogRead(HIHAT_PEDAL_PIN);
    return raw;
}

void processHiHatPedal(const HiHatSettings &hihatSettings) {
    int raw = analogRead(HIHAT_PEDAL_PIN);
    if(hihatSettings.invert) raw = 1023 - raw;

    uint8_t val = map(raw, 0, 1023, hihatSettings.ccClosed, hihatSettings.ccOpen);
    val = constrain(val, 0, 127);

    if(val != lastHiHatValue) {
        midiSendCC(MIDI_CHANEL, 4, val);
        lastHiHatValue = val;
        lastHiHatCCSend = millis();
    }

    int16_t diff = (int16_t)raw - (int16_t)lastRawValue;

    if (diff < -(int16_t)hihatSettings.hitThresholdRaw && (millis() - lastHitTime) > hihatSettings.debounceTimeMs) {
        midiSendNoteOn(MIDI_CHANEL, hihatSettings.hitNote, hihatSettings.hitVelocity);
        midiSendNoteOff(MIDI_CHANEL, hihatSettings.hitNote, 0);

        lastHitTime = millis();
    }

    const uint8_t closeThreshold = 5; // допустимый разброс по CC

    if (abs(val - hihatSettings.ccClosed) <= closeThreshold) {
        isHiHatClosed = true;
    } else {
        isHiHatClosed = false;
    }

    lastRawValue = raw;
}