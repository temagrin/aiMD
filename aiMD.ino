//файл aiMD.ino 

#include <LiquidCrystal_I2C.h>
#include <Arduino.h>
#include "Settings.h"
#include "LCD_UI.h"
#include "Buttons.h"
#include "Mux.h"
#include "Pads.h"
#include "HiHatPedal.h"
#include "MIDISender.h"
#include "UIAction.h"
#include "Config.h"

LiquidCrystal_I2C lcd(0x27, 16, 2);

int8_t buttonState = 0;
bool debugMode = false;
unsigned long lastActivity = 0;

bool lcdNeedsUpdate = true;
bool editingParam = false;
uint8_t menuParamIndex = 0;
uint8_t editPadIndex = 0;
uint8_t mainMenuSelection = 0;

bool editingHiHatParam = false;
uint8_t hiHatMenuIndex = 0;

bool editingXtalk = false;
uint8_t xtalkPadIndex = 0;
uint8_t xtalkMenuParamIndex = 0;

bool resetYesSelected = true;
static bool buttonProcessed = false;

Settings deviceSettings;
PadStatus padStatus[NUM_JACKS];
UIState uiState = UI_MAIN;
UIState lastUiState = UI_MAIN;


MainMenu mainMenu(mainMenuSelection, uiState, lcdNeedsUpdate);
EditPadMenu editPadMenu(
    deviceSettings,     
    editPadIndex,       
    menuParamIndex, 
    editingParam, 
    uiState, 
    lcdNeedsUpdate, 
    lcd
);
EditHiHatMenu editHiHatMenu(
    deviceSettings,     
    editingHiHatParam,  
    hiHatMenuIndex,     
    uiState,            
    lcdNeedsUpdate,     
    lcd                 
);
EditXtalkMenu editXtalkMenu(
    deviceSettings,
    editingXtalk,
    xtalkPadIndex,
    xtalkMenuParamIndex,
    uiState,
    lcdNeedsUpdate,
    lcd
);
ConfirmResetMenu confirmResetMenu(
    uiState,
    lcdNeedsUpdate,
    resetYesSelected,
    lcd,
    deviceSettings
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

void processUI(Settings &deviceSettings, PadStatus padStatus[], int8_t &buttonState, bool &debugMode, bool &editingParam,
               uint8_t &menuParamIndex, uint8_t &editPadIndex, unsigned long &lastActivity, UIState &uiState,
               bool &lcdNeedsUpdate) {
    if (buttonState == 0)
        return;

    static UIState lastUiState = uiState;  // Запоминаем состояние

    bool isEditing = editingParam || editingHiHatParam || editingXtalk;

    UIAction action;
    switch (buttonState) {
        case 1: action = isEditing ? ACT_EDIT_PARAM_INCREASE : ACT_MOVE_ACTIVE_ITEM_PREV; break;
        case 2: action = isEditing ? ACT_EDIT_PARAM_DECREASE : ACT_BACK; break;
        case 3: action = isEditing ? ACT_EDIT_PARAM_DECREASE : ACT_MOVE_ACTIVE_ITEM_NEXT; break;
        case 4: action = isEditing ? ACT_EDIT_PARAM_INCREASE : ACT_MOVE_PARAM_NEXT; break;
        case 5: action = ACT_SAVE; break;
        default: action = ACT_BACK;
    }

    // 1) Получаем текущее меню по текущему состоянию uiState
    MenuBase* currentMenu = nullptr;
    switch (uiState) {
        case UI_MAIN: currentMenu = &mainMenu; break;
        case UI_EDIT_PAD: currentMenu = &editPadMenu; break;
        case UI_EDIT_HIHAT: currentMenu = &editHiHatMenu; break;
        case UI_EDIT_XTALK: currentMenu = &editXtalkMenu; break;
        case UI_CONFIRM_RESET: currentMenu = &confirmResetMenu; break;
        default: currentMenu = &mainMenu; break;
    }

    if (currentMenu) {
        currentMenu->handleAction(action);
    }

    // 2) После обработки действия, uiState мог измениться — обновим currentMenu
    MenuBase* newCurrentMenu = nullptr;
    switch (uiState) {
        case UI_MAIN: newCurrentMenu = &mainMenu; break;
        case UI_EDIT_PAD: newCurrentMenu = &editPadMenu; break;
        case UI_EDIT_HIHAT: newCurrentMenu = &editHiHatMenu; break;
        case UI_EDIT_XTALK: newCurrentMenu = &editXtalkMenu; break;
        case UI_CONFIRM_RESET: newCurrentMenu = &confirmResetMenu; break;
        default: newCurrentMenu = &mainMenu; break;
    }

    // 3) Сравним состояние до и после, если изменилось — выставим lcdNeedsUpdate
    if (lastUiState != uiState) {
        lcdNeedsUpdate = true;
        lastUiState = uiState;
    }

    // 4) Если флаг выставлен, отрисуем меню
    if (lcdNeedsUpdate && newCurrentMenu) {
        newCurrentMenu->render();
        lcdNeedsUpdate = false;  // сбросить флаг после отрисовки
    }

    lastActivity = millis();
}


void setupPins() {
  pinMode(MUX_S0, OUTPUT);
  pinMode(MUX_S1, OUTPUT);
  pinMode(MUX_S2, OUTPUT);
  pinMode(MUX_S3, OUTPUT);
  
  pinMode(BUTTONS_PIN, INPUT);    
  pinMode(HIHAT_PEDAL_PIN, INPUT);
  uiState = UI_MAIN;
  lcdNeedsUpdate = true;
  lastActivity = millis();
}

void setup() {
    setupPins();
    Serial.begin(MIDI_BAUD);
    delay(10);
    loadSettings(deviceSettings);
    lcd.init();
    lcd.backlight();
    uiState = UI_MAIN;
    lastActivity = millis();
    lcdNeedsUpdate = true;

}

void loop() {
    updateButtonState(buttonState);

    if (buttonState != 0) {
        if (!buttonProcessed) {  // обработка только один раз
            processUI(deviceSettings, padStatus, buttonState, debugMode,
                  editingParam, menuParamIndex, editPadIndex,
                  lastActivity, uiState, lcdNeedsUpdate);
            buttonProcessed = true;
        }
    } else {
        buttonProcessed = false;  // сброс после отпускания
        if (lcdNeedsUpdate) {
            MenuBase* currentMenu = nullptr;
            switch (uiState) {
                case UI_MAIN: currentMenu = &mainMenu; break;
                case UI_EDIT_PAD: currentMenu = &editPadMenu; break;
                case UI_EDIT_HIHAT: currentMenu = &editHiHatMenu; break;
                case UI_EDIT_XTALK: currentMenu = &editXtalkMenu; break;
                case UI_CONFIRM_RESET: currentMenu = &confirmResetMenu; break;
                default: currentMenu = &mainMenu; break;
            }
            if (currentMenu) currentMenu->render();
            lcdNeedsUpdate = false;
        }
    }

    // Остальное покавыключено для отладки меню
    //for (uint8_t i = 0; i < NUM_JACKS; i++) {
    //    scanPad(deviceSettings.pads[i], padStatus[i], i);
    //}
    //processHiHatPedal(deviceSettings.hihat);

    delay(1);
}