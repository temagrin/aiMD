#include "MIDISender.h"

extern bool debugMode;

void midiSendNoteOn(byte channel, byte note, byte velocity) {
    if(debugMode) {
        Serial.print("DEBUG MIDI Note On: "); Serial.print(note);
        Serial.print(" Vel: "); Serial.println(velocity);
    } else {
        Serial.write(0x90 | (channel & 0x0F));
        Serial.write(note);
        Serial.write(velocity);
    }
}

void midiSendNoteOff(byte channel, byte note, byte velocity) {
    if(debugMode) {
        Serial.print("DEBUG MIDI Note Off: "); Serial.println(note);
    } else {
        Serial.write(0x80 | (channel & 0x0F));
        Serial.write(note);
        Serial.write(velocity);
    }
}

void midiSendCC(byte channel, byte ccNum, byte value) {
    if(debugMode) {
        Serial.print("DEBUG MIDI CC: "); Serial.print(ccNum);
        Serial.print(" Val: "); Serial.println(value);
    } else {
        Serial.write(0xB0 | (channel & 0x0F));
        Serial.write(ccNum);
        Serial.write(value);
    }
}