// файл Pads.h
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
    unsigned long lastTriggerTime; // время последнего срабатывания головы или рима (для XTALK)
    bool activeTrigger;            // флаг, что пэд в данный момент активен (для XTALK)
};

// Прототипы
void scanPad(const PadSettings &pad, PadStatus &status, uint8_t jackIndex);

#endif