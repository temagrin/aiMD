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
        settings.pads[i].sensitivity = 80;
        settings.pads[i].threshold = 30;
        settings.pads[i].scantime = 10;
        settings.pads[i].masktime = 20;
        settings.pads[i].curve = CURVE_LINEAR;

        settings.pads[i].xtalkThreshold = 5;    // Значение по умолчанию
        settings.pads[i].xtalkCancelTime = 50;  // 50 мс по умолчанию

        settings.pads[i].rimMute = false;
        settings.pads[i].twoZoneMode = false;
        settings.pads[i].muteByPiezo = false;
    }
    settings.hihat.ccClosed = 0;
    settings.hihat.ccOpen = 127;
    settings.hihat.ccStep = 64;
    settings.hihat.invert = false;
}

void saveSettings(Settings &settings) {
    settings.magic = EEPROM_MAGIC; // Всегда ставим magic перед записью
    EEPROM.put(SETTINGS_ADDR, settings);
    delay(10); // Даем время завершить запись
    Serial.println(F("Settings saved to EEPROM"));
}

void loadSettings(Settings &settings) {
    EEPROM.get(SETTINGS_ADDR, settings);
    Serial.print(F("Loaded magic: 0x"));
    Serial.println(settings.magic, HEX);
    if (settings.magic != EEPROM_MAGIC) {
        Serial.println(F("Magic mismatch, initializing defaults"));
        initSettings(settings);
        saveSettings(settings);
    } else {
        Serial.println(F("Settings loaded successfully"));
    }
}