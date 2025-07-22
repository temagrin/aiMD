#include "Settings.h"
#include <EEPROM.h>

void initSettings(Settings &settings) {
    settings.magic = EEPROM_MAGIC;
    for (uint8_t i=0; i < NUM_JACKS; i++) {
        settings.pads[i].type = PAD_SINGLE;
        settings.pads[i].midiHeadNote = 38 + i*2;
        settings.pads[i].midiRimNote = 40 + i*2;
        settings.pads[i].sensitivity = 80;
        settings.pads[i].threshold = 30;
        settings.pads[i].scantime = 10;
        settings.pads[i].masktime = 20;
        settings.pads[i].curve = CURVE_LINEAR;
        settings.pads[i].xtalk = 5;
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
    settings.magic = EEPROM_MAGIC;  // Гарантируем выставление магии
    EEPROM.put(SETTINGS_ADDR, settings);
    delay(10);
}

void loadSettings(Settings &settings) {
    EEPROM.get(SETTINGS_ADDR, settings);
    if (settings.magic != EEPROM_MAGIC) {
        initSettings(settings);
        saveSettings(settings);
    }
}