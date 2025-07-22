#ifndef SETTINGS_H
#define SETTINGS_H

#include "Config.h"

struct PadSettings {
    PadType type;
    uint8_t midiHeadNote;
    uint8_t midiRimNote;
    uint8_t sensitivity;    // 0-127
    uint16_t threshold;     // 0-1023
    uint8_t scantime;       // в миллисекундах
    uint8_t masktime;       // ms маскирование
    CurveType curve;
    uint8_t xtalk;
    bool rimMute;
    bool twoZoneMode;
    bool muteByPiezo;
};

struct HiHatSettings {
    uint8_t ccClosed;
    uint8_t ccOpen;
    uint8_t ccStep;
    bool invert;
};

struct Settings {
    uint8_t magic;
    PadSettings pads[NUM_JACKS];
    HiHatSettings hihat;
};

// Прототипы
void initSettings(Settings &settings);
void saveSettings(Settings &settings);
void loadSettings(Settings &settings);

#endif