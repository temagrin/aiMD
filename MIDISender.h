// файл MIDISender.h
#ifndef MIDISENDER_H
#define MIDISENDER_H

#include <Arduino.h>

void midiSendNoteOn(byte channel, byte note, byte velocity);
void midiSendNoteOff(byte channel, byte note, byte velocity);
void midiSendCC(byte channel, byte ccNum, byte value);

#endif