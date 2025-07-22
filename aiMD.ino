
#include "Config.h"
#include "Mux.h"
#include "Pads.h"
#include "Buttons.h"
#include "MIDISender.h"
#include "Settings.h"
#include "HiHatPedal.h"
#include "UIAction.h"
#include "LCD_UI.h"




UIState uiState = UI_MAIN;
bool lcdNeedsUpdate = true;
bool editingParam = false;
uint8_t menuParamIndex = 0;
uint8_t editPadIndex = 0;
uint8_t mainMenuSelection = 0;

Settings deviceSettings;
PadStatus padStatus[NUM_JACKS];

MainMenu mainMenu(mainMenuSelection, uiState, lcdNeedsUpdate);
EditPadMenu editPadMenu(
    deviceSettings,         // Settings &deviceSettings
    editPadIndex,           // bool &editingPadParam
    menuParamIndex, 
    editingParam, 
    uiState, 
    lcdNeedsUpdate, 
    lcd
);
EditHiHatMenu editHiHatMenu(
    deviceSettings,          // Settings &deviceSettings
    editingHiHatParam,       // bool &editingHiHatParam
    hiHatMenuIndex,          // uint8_t &hiHatMenuIndex
    uiState,                 // UIState &uiState
    lcdNeedsUpdate,          // bool &lcdNeedsUpdate
    lcd                      // LiquidCrystal_I2C &lcd
);
EditXtalkMenu editXtalkMenu(
    deviceSettings,          // Settings &deviceSettings
    editingXtalk,            // bool &editingXtalk
    xtalkPadIndex,           // uint8_t &xtalkPadIndex
    xtalkMenuParamIndex,     // uint8_t &xtalkMenuParamIndex
    uiState,                 // UIState &uiState
    lcdNeedsUpdate,          // bool &lcdNeedsUpdate
    lcd                      // LiquidCrystal_I2C &lcd
);
ConfirmResetMenu confirmResetMenu(
    uiState,                 // UIState &uiState
    lcdNeedsUpdate,          // bool &lcdNeedsUpdate
    resetYesSelected,        // bool &resetYesSelected
    lcd,                     // LiquidCrystal_I2C &lcd
    deviceSettings           // Settings &deviceSettings
);

MenuBase *currentMenu = &mainMenu;

UIAction mapButtonToAction(int8_t button, bool isEditing) {
  if (isEditing) {
    switch (button) {
      case 1: return ACT_EDIT_PARAM_INCREASE;
      case 3: return ACT_EDIT_PARAM_DECREASE;
      case 4: return ACT_EDIT_PARAM_INCREASE;
      case 2: return ACT_EDIT_PARAM_DECREASE;
      case 5: return ACT_SAVE;
      default: return ACT_BACK;
    }
  } else {
    switch (button) {
      case 1: return ACT_MOVE_ACTIVE_ITEM_PREV;
      case 3: return ACT_MOVE_ACTIVE_ITEM_NEXT;
      case 4: return ACT_MOVE_PARAM_NEXT;
      case 2: return ACT_BACK;
      case 5: return ACT_SAVE;
      default: return ACT_BACK;
    }
  }
}

void processUI(int8_t buttonState) {
  if (buttonState == 0) return;

  bool isEditing = editingParam; // расширьте проверку для каждого меню при необходимости
  UIAction action = mapButtonToAction(buttonState, isEditing);

  if (currentMenu)
    currentMenu->handleAction(action);

  if (lcdNeedsUpdate) {
    if (currentMenu) currentMenu->render();
    lcdNeedsUpdate = false;
  }

  switch (uiState) {
    case UI_MAIN: currentMenu = &mainMenu; break;
    case UI_EDIT_PAD: currentMenu = &editPadMenu; break;
    case UI_EDIT_HIHAT: currentMenu = &editHiHatMenu; break;
    case UI_EDIT_XTALK: currentMenu = &editXtalkMenu; break;
    case UI_CONFIRM_RESET: currentMenu = &confirmResetMenu; break;
    default: currentMenu = &mainMenu; break;
  }
}


void setupPins() {
  pinMode(MUX_S0, OUTPUT);
  pinMode(MUX_S1, OUTPUT);
  pinMode(MUX_S2, OUTPUT);
  pinMode(MUX_S3, OUTPUT);
  
  pinMode(BUTTONS_PIN, INPUT);    
  pinMode(HIHAT_PEDAL_PIN, INPUT);
}

void setup() {
    setupPins();
    Serial.begin(MIDI_BAUD);
    delay(10);    
    loadSettings(deviceSettings);
}

void loop() {
    updateButtonState(buttonState);
    processUI(deviceSettings, padStatus, buttonState, debugMode,
              editingParam, menuParamIndex, editPadIndex, lastActivity, uiState, lcdNeedsUpdate);
    
    for (uint8_t i = 0; i < NUM_JACKS; i++) {
        scanPad(deviceSettings.pads[i], padStatus[i], i);
    }
    processHiHatPedal(deviceSettings.hihat);
    delay(1);

}