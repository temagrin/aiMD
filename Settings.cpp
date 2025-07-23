// файл Settings.cpp
#include "Settings.h"
#include <EEPROM.h>
#include <Arduino.h> // Для delay()

#define EEPROM_MAGIC 0x42

void initSettings(Settings &settings) {
    settings.magic = EEPROM_MAGIC;
    for (uint8_t i = 0; i < NUM_JACKS; i++) {
        settings.pads[i].type = PAD_SINGLE;
        settings.pads[i].midiHeadNote = 38 + i * 2;
        settings.pads[i].midiRimNote = 40 + i * 2;
        settings.pads[i].altHeadNote = 24 + i * 2;
        settings.pads[i].altRimNote = 26 + i * 2;
        settings.pads[i].sensitivity = 80;
        settings.pads[i].threshold = 30;
        settings.pads[i].scantime = 10;
        settings.pads[i].masktime = 20;
        settings.pads[i].curve = CURVE_LINEAR;
        settings.pads[i].xtalkThreshold = 5;    // Значение по умолчанию
        settings.pads[i].xtalkCancelTime = 50;  // 50 мс по умолчанию
        settings.pads[i].rimMute = false;
        settings.pads[i].choke = false;
    }
    settings.hihat.ccClosed = 0;
    settings.hihat.ccOpen = 127;
    settings.hihat.ccStep = 64;
    settings.hihat.invert = false;
    settings.hihat.hitNote = 46;           
    settings.hihat.hitVelocity = 127;
    settings.hihat.hitThresholdRaw = 100;
    settings.hihat.debounceTimeMs = 50;
}

void saveSettings(Settings &settings) {
    settings.magic = EEPROM_MAGIC; // Всегда ставим magic перед записью
    EEPROM.put(SETTINGS_ADDR, settings);
    delay(10); // Даем время завершить запись
}

void loadSettings(Settings &settings) {
    EEPROM.get(SETTINGS_ADDR, settings);
    if (settings.magic != EEPROM_MAGIC) {
        initSettings(settings);
        saveSettings(settings);
    } 
}