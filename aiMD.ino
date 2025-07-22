#include <LiquidCrystal_I2C.h>
#include "Config.h"
#include "Mux.h"
#include "Pads.h"
#include "Buttons.h"
#include "LCD_UI.h"
#include "MIDISender.h"
#include "Settings.h"
#include "HiHatPedal.h"


// Адрес и размеры LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Глобальные переменные
Settings deviceSettings;
PadStatus padStatus[NUM_JACKS];
bool debugMode = false;
bool editingParam = false;
unsigned long lastActivity = 0;
uint8_t menuParamIndex = 0;
uint8_t xtalkMenuParamIndex = 0;
uint8_t editPadIndex = 0;
bool lcdNeedsUpdate = true;

uint8_t hiHatMenuIndex = 0;
bool editingHiHatParam = false;

uint8_t xtalkPadIndex = 0;
bool editingXtalk = false;

uint8_t mainMenuSelection = 0;

UIState uiState = UI_MAIN;
int8_t buttonState = 0;

void setupPins() {
  pinMode(MUX_S0, OUTPUT);
  pinMode(MUX_S1, OUTPUT);
  pinMode(MUX_S2, OUTPUT);
  pinMode(MUX_S3, OUTPUT);
  
  pinMode(BUTTONS_PIN, INPUT);    // Кнопки подключены к А0, аналоговый вход
  pinMode(HIHAT_PEDAL_PIN, INPUT); // Педаль хайхета на A6, аналоговый вход
  // Если понадобятся другие пины — добавьте здесь их настройку
}

// Arduino setup
void setup() {
    setupPins();
    Serial.begin(MIDI_BAUD);
    delay(10);
    lcdInit();
    loadSettings(deviceSettings);
    displayMainMenu(mainMenuSelection);
}

// Arduino main loop
void loop() {
    updateButtonState(buttonState);
    processUI(deviceSettings, padStatus, buttonState, debugMode,
              editingParam, menuParamIndex, editPadIndex, lastActivity, uiState, lcdNeedsUpdate);

    if (lcdNeedsUpdate) {
        switch(uiState) {
            case UI_MAIN:
                displayMainMenu(mainMenuSelection);
                break;
            case UI_EDIT_PAD:
                displayPadEditMenu(deviceSettings, editPadIndex, menuParamIndex, editingParam, lcdNeedsUpdate);
                break;
            case UI_EDIT_HIHAT:
                displayHiHatEditMenu(deviceSettings.hihat, hiHatMenuIndex, editingHiHatParam, lcdNeedsUpdate);
                break;
            case UI_EDIT_XTALK:
                displayXtalkMenu(deviceSettings, xtalkPadIndex, xtalkMenuParamIndex, editingXtalk);
                break;
            case UI_CONFIRM_RESET:
                displayConfirmReset();
                break;
        }
        lcdNeedsUpdate = false;
    }

    for (uint8_t i = 0; i < NUM_JACKS; i++) {
        scanPad(deviceSettings.pads[i], padStatus[i], i);
    }
    processHiHatPedal(deviceSettings.hihat);
    delay(1);
}