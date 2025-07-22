#ifndef PADS_H
#define PADS_H

#include "Settings.h"
#include "Config.h"
#include <Arduino.h>

struct PadStatus {
    bool headPressed;
    bool rimPressed;
    unsigned long lastHitTimeHead;
    unsigned long lastHitTimeRim;
};

// Прототипы
void scanPad(const PadSettings &pad, PadStatus &status, uint8_t jackIndex);

#endif