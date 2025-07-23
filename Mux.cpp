// файл Mux.cpp
#include "Mux.h"
#include <Arduino.h>

const int8_t muxJackMap[NUM_JACKS][2] = {
    {8, -1},
    {9, -1},
    {10, -1},
    {12, -1},
    {14, 15},
    {0, 1},
    {2, 3},
    {4, 5},
    {6, 7},
    {11, 13}
};

void muxSelect(uint8_t channel) {
    digitalWrite(MUX_S0, (channel & 0x01) ? HIGH : LOW);
    digitalWrite(MUX_S1, (channel & 0x02) ? HIGH : LOW);
    digitalWrite(MUX_S2, (channel & 0x04) ? HIGH : LOW);
    digitalWrite(MUX_S3, (channel & 0x08) ? HIGH : LOW);
    delayMicroseconds(5);
}

int muxRead(uint8_t channel) {
    muxSelect(channel);
    return analogRead(MUX_SIG);
}