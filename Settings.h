// файл Settings.h
#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>
#include "Config.h"

#define SETTINGS_ADDR 0

struct HiHatSettings {
    uint8_t ccClosed;
    uint8_t ccOpen;
    uint8_t ccStep;
    bool invert;
    uint8_t hitNote;           // MIDI нота хлопка
    uint8_t hitVelocity;       // Скорость ноты хлопка
    uint16_t hitThresholdRaw;  // Порог резкого изменения для детекции (например, 100)
    unsigned long debounceTimeMs;
};

struct PadSettings {
    PadType type;
    uint8_t midiHeadNote;
    uint8_t midiRimNote;
    uint8_t sensitivity;
    uint16_t threshold;
    uint16_t scantime;
    uint16_t masktime;
    uint8_t curve;

    uint8_t xtalkThreshold;     // Порог XTALK (0-127)
    uint16_t xtalkCancelTime;   // Время отмены XTALK в мс

    bool rimMute;
    bool twoZoneMode;
    bool muteByPiezo;
};

struct Settings {
    uint8_t magic;
    PadSettings pads[NUM_JACKS];
    HiHatSettings hihat;
};

void initSettings(Settings &settings);
void saveSettings(Settings &settings);
void loadSettings(Settings &settings);

#endif