#include "LCD_UI.h"
#include <LiquidCrystal_I2C.h>
#include "Settings.h"   // Нужен доступ к структуре Settings
#include "Config.h"     // Нужен доступ к константам и энумам
#include "Mux.h"        // Нужен для JackMuxMap
#include "Pads.h"       // для PadStatus

// Экземпляр LCD
extern LiquidCrystal_I2C lcd; // Объявлен в main.ino

// Глобальные переменные, к которым обращается LCD_UI (из main.ino или других модулей)
extern bool editingParam;
extern uint8_t menuParamIndex;
extern uint8_t editPadIndex;
extern UIState uiState;
extern bool lcdNeedsUpdate;
extern bool debugMode;
extern unsigned long lastActivity;
extern int8_t buttonState;

// Отдельные переменные для HiHat и XTALK редактирования
extern bool editingHiHatParam;
extern bool editingXtalk;
extern uint8_t hiHatMenuIndex;
extern uint8_t xtalkPadIndex;
extern uint8_t xtalkMenuParamIndex;

// Основной выбор в главном меню
extern uint8_t mainMenuSelection;

static bool resetYesSelected = true; // Для UI_CONFIRM_RESET


// Прототипы внешних функций из Settings.cpp
extern void saveSettings(const Settings &settings);
extern void initSettings(Settings &settings);

// Функция для центрированного вывода на LCD
void lcdPrintCentered(const char *str) {
  int pad = (LCD_COLS - strlen(str)) / 2;
  if (pad < 0) pad = 0;
  lcd.setCursor(pad, 0);
  lcd.print(str);
}

// Инициализация LCD
void lcdInit() {
  lcd.init();
  lcd.backlight();
}

// Отобразить главное меню
void displayMainMenu(uint8_t selectedItem) {
  lcd.clear();

  const char* menuItems[MENU_ITEMS_COUNT] = {
    "Inputs",
    "HiHat Pedal",
    "XTALK",
    "Reset Defaults"
  };

  uint8_t displayOffset = 0;

  if (selectedItem >= 1 && MENU_ITEMS_COUNT > 2) {
    displayOffset = selectedItem - 1;
  }
  if (displayOffset >= MENU_ITEMS_COUNT - 1 && MENU_ITEMS_COUNT > 1) {
    displayOffset = MENU_ITEMS_COUNT - 2;
  }
  if (MENU_ITEMS_COUNT == 1) displayOffset = 0;

  for (uint8_t i = 0; i < 2; ++i) {
    uint8_t itemIndex = displayOffset + i;
    if (itemIndex < MENU_ITEMS_COUNT) {
      lcd.setCursor(0, i);
      lcd.print((itemIndex == selectedItem) ? ">" : " ");
      lcd.print(menuItems[itemIndex]);
    } else {
      lcd.setCursor(0, i);
      lcd.print("                ");
    }
  }
}

// Отобразить меню редактирования пэда
void displayPadEditMenu(const Settings &deviceSettings, uint8_t padIdx, uint8_t menuParamIdx, bool editingParam, bool &lcdNeedsUpdateRef) {
  const PadSettings &ps = deviceSettings.pads[padIdx];
  static const CurveType allowedCurves[] = {
      CURVE_LINEAR,
      CURVE_EXPONENTIAL,
      CURVE_LOG,
      CURVE_MAX_VELOCITY
  };

  lcd.clear();
  delay(3); 
  lcd.setCursor(0, 0);
  lcd.print("Input ");
  lcd.print(padIdx + 1);

  uint8_t currentParamIndices[NUM_TOTAL_PAD_PARAMS];
  uint8_t numAvailableParams = 0;

  currentParamIndices[numAvailableParams++] = PARAM_TYPE;
  currentParamIndices[numAvailableParams++] = PARAM_HEAD_MIDI;
  currentParamIndices[numAvailableParams++] = PARAM_SENSITIVITY;
  currentParamIndices[numAvailableParams++] = PARAM_THRESHOLD;
  currentParamIndices[numAvailableParams++] = PARAM_CURVE;

  if (muxJackMap[padIdx][1] != -1) {
    currentParamIndices[numAvailableParams++] = PARAM_RIM_MIDI;
    currentParamIndices[numAvailableParams++] = PARAM_RIM_MUTE;
    currentParamIndices[numAvailableParams++] = PARAM_TWO_ZONE_MODE;
    currentParamIndices[numAvailableParams++] = PARAM_MUTE_BY_PIEZO;
  }

  if (menuParamIdx >= numAvailableParams && numAvailableParams > 0) {
    menuParamIdx = numAvailableParams - 1;
  } else if (numAvailableParams == 0) {
    menuParamIdx = 0;
  }

  uint8_t actualParamIndex = currentParamIndices[menuParamIdx];

  lcd.setCursor(0, 1);
  lcd.print(allParamNames[actualParamIndex]);
  lcd.print(":");

  switch(actualParamIndex) {
    case PARAM_TYPE: {
      const char* typesStr[] = {"Dis", "Sing", "Dual", "HiHat", "Cymbal"};
      lcd.setCursor(9, 1);
      lcd.print(typesStr[ps.type]);
      break;
    }
    case PARAM_HEAD_MIDI:
      lcd.setCursor(9, 1);
      lcd.print(ps.midiHeadNote);
      break;
    case PARAM_RIM_MIDI:
      lcd.setCursor(9, 1);
      lcd.print(ps.midiRimNote);
      break;
    case PARAM_SENSITIVITY:
      lcd.setCursor(9, 1);
      lcd.print(ps.sensitivity);
      break;
    case PARAM_THRESHOLD:
      lcd.setCursor(9, 1);
      lcd.print(ps.threshold);
      break;
    case PARAM_CURVE: {
      uint8_t curveIndex = 0;
      for(uint8_t i = 0; i < sizeof(allowedCurves)/sizeof(allowedCurves[0]); i++) {
          if (ps.curve == allowedCurves[i]) {
              curveIndex = i;
              break;
          }
      }
      lcd.setCursor(9, 1);
      lcd.print(curveNames[curveIndex]);
      break;
    }
    case PARAM_RIM_MUTE:
      lcd.setCursor(9, 1);
      lcd.print(ps.rimMute ? "On" : "Off");
      break;
    case PARAM_TWO_ZONE_MODE:
      lcd.setCursor(9, 1);
      lcd.print(ps.twoZoneMode ? "On" : "Off");
      break;
    case PARAM_MUTE_BY_PIEZO:
      lcd.setCursor(9, 1);
      lcd.print(ps.muteByPiezo ? "On" : "Off");
      break;
  }

  if (editingParam) {
    lcd.setCursor(LCD_COLS - 1, 1);
    lcd.print("*");
  }
  lcdNeedsUpdateRef = false;
}

// Отобразить меню редактирования HiHat
void displayHiHatEditMenu(const HiHatSettings &hihat, uint8_t menuIndex, bool editing, bool &lcdNeedsUpdateRef) {
  lcd.clear();
  delay(3); 
  lcd.setCursor(0, 0);
  lcd.print("HiHat Setting");
  lcd.setCursor(0, 1);
  lcd.print(hiHatParamNames[menuIndex]);
  lcd.print(": ");
  switch(menuIndex) {
    case HIHAT_CC_CLOSED:
      lcd.print(hihat.ccClosed);
      break;
    case HIHAT_CC_OPEN:
      lcd.print(hihat.ccOpen);
      break;
    case HIHAT_CC_STEP:
      lcd.print(hihat.ccStep);
      break;
    case HIHAT_INVERT:
      lcd.print(hihat.invert ? "On" : "Off");
      break;
  }
  if(editing) {
    lcd.setCursor(LCD_COLS-1, 1);
    lcd.print("*");
  }
  lcdNeedsUpdateRef = false;
}

// Отобразить меню XTALK
void displayXtalkMenu(const Settings &deviceSettings, uint8_t padIdx, uint8_t menuParamIndex, bool editing) {
  const PadSettings &ps = deviceSettings.pads[padIdx];
  lcd.clear();
  delay(3);
  lcd.setCursor(0, 0);
  lcd.print("XTALK Pad ");
  lcd.print(padIdx + 1);

  const char* paramNames[] = {"Thresh:", "Cancel ms:"};
  lcd.setCursor(0, 1);
  lcd.print(paramNames[menuParamIndex]);
  lcd.print(" ");
  if (menuParamIndex == 0) {
    lcd.print(ps.xtalkThreshold);
  } else if (menuParamIndex == 1) {
    lcd.print(ps.xtalkCancelTime);
  }
  if (editing) {
    lcd.setCursor(LCD_COLS -1, 1);
    lcd.print("*");
  }
}

// Отобразить меню подтверждения сброса
void displayConfirmReset() {
  lcd.clear();
  delay(3);
  lcd.setCursor(0, 0);
  lcdPrintCentered("Reset to Defaults?");
  lcd.setCursor(0, 1);
  if (resetYesSelected) {
    lcd.print("> YES    NO ");
  } else {
    lcd.print("  YES    >NO");
  }
}

// Основная логика интерфейса пользователя
void processUI(Settings &deviceSettingsRef, PadStatus padStatusRef[], int8_t &buttonStateRef, bool &debugModeRef,
               bool &editingParamRef, uint8_t &menuParamIndexRef, uint8_t &editPadIndexRef, unsigned long &lastActivityRef,
               UIState &uiStateRef, bool &lcdNeedsUpdateRef) {
  const unsigned long timeout = 15000;
  unsigned long now = millis();

  // Таймауты выхода из редакторов
  if (editingParamRef && (now - lastActivityRef > timeout)) {
    editingParamRef = false;
    uiStateRef = UI_MAIN;
    lcdNeedsUpdateRef = true;
  }
  if (editingHiHatParam && (now - lastActivityRef > timeout)) {
    editingHiHatParam = false;
    uiStateRef = UI_MAIN;
    lcdNeedsUpdateRef = true;
  }
  if (editingXtalk && (now - lastActivityRef > timeout)) {
    editingXtalk = false;
    uiStateRef = UI_MAIN;
    lcdNeedsUpdateRef = true;
  }

  if (buttonStateRef != 0) {
    lastActivityRef = now;
  }

  static int8_t prevButtonState = -1;
  if (buttonStateRef == prevButtonState) return;
  prevButtonState = buttonStateRef;

  // Массив для возможных кривых (используется в изменении параметра)
  static const CurveType allowedCurves[] = {
      CURVE_LINEAR,
      CURVE_EXPONENTIAL,
      CURVE_LOG,
      CURVE_MAX_VELOCITY
  };
  uint8_t curveCount = sizeof(allowedCurves) / sizeof(allowedCurves[0]);

  switch (uiStateRef) {
    case UI_MAIN:
      if (buttonStateRef == 1) { // UP
        if (mainMenuSelection == 0) mainMenuSelection = MENU_ITEMS_COUNT - 1;
        else mainMenuSelection--;
        lcdNeedsUpdateRef = true;
      } else if (buttonStateRef == 3) { // DOWN
        if (mainMenuSelection == MENU_ITEMS_COUNT - 1) mainMenuSelection = 0;
        else mainMenuSelection++;
        lcdNeedsUpdateRef = true;
      } else if (buttonStateRef == 5) { // SELECT
        switch (mainMenuSelection) {
          case MENU_EDIT_PADS:
            uiStateRef = UI_EDIT_PAD;
            editPadIndexRef = 0;
            menuParamIndexRef = 0;
            editingParamRef = false;
            lcdNeedsUpdateRef = true;
            break;
          case MENU_EDIT_HIHAT:
            uiStateRef = UI_EDIT_HIHAT;
            hiHatMenuIndex = 0;
            editingHiHatParam = false;
            lcdNeedsUpdateRef = true;
            break;
          case MENU_EDIT_XTALK:
            uiStateRef = UI_EDIT_XTALK;
            xtalkPadIndex = 0;
            editingXtalk = false;
            lcdNeedsUpdateRef = true;
            break;
          case MENU_RESET_DEFAULTS:
            uiStateRef = UI_CONFIRM_RESET;
            lcdNeedsUpdateRef = true;
            break;
        }
      }
      break;

    case UI_EDIT_PAD: {
      PadSettings &ps = deviceSettingsRef.pads[editPadIndexRef];

      uint8_t currentParamIndices[NUM_TOTAL_PAD_PARAMS];
      uint8_t numAvailableParams = 0;

      currentParamIndices[numAvailableParams++] = PARAM_TYPE;
      currentParamIndices[numAvailableParams++] = PARAM_HEAD_MIDI;
      currentParamIndices[numAvailableParams++] = PARAM_SENSITIVITY;
      currentParamIndices[numAvailableParams++] = PARAM_THRESHOLD;
      currentParamIndices[numAvailableParams++] = PARAM_CURVE;

      if (muxJackMap[editPadIndexRef][1] != -1) {
        currentParamIndices[numAvailableParams++] = PARAM_RIM_MIDI;
        currentParamIndices[numAvailableParams++] = PARAM_RIM_MUTE;
        currentParamIndices[numAvailableParams++] = PARAM_TWO_ZONE_MODE;
        currentParamIndices[numAvailableParams++] = PARAM_MUTE_BY_PIEZO;
      }

      if (menuParamIndexRef >= numAvailableParams && numAvailableParams > 0) {
        menuParamIndexRef = numAvailableParams - 1;
      } else if (numAvailableParams == 0) {
        menuParamIndexRef = 0;
      }

      uint8_t actualParamIndex = currentParamIndices[menuParamIndexRef];

      if (!editingParamRef) {
        if (buttonStateRef == 1) { // UP
          if (editPadIndexRef == 0) editPadIndexRef = NUM_JACKS - 1;
          else editPadIndexRef--;
          menuParamIndexRef = 0;
          lcdNeedsUpdateRef = true;
        } else if (buttonStateRef == 3) { // DOWN
          if (editPadIndexRef == NUM_JACKS - 1) editPadIndexRef = 0;
          else editPadIndexRef++;
          menuParamIndexRef = 0;
          lcdNeedsUpdateRef = true;
        } else if (buttonStateRef == 4) { // RIGHT
          if (menuParamIndexRef == numAvailableParams - 1) menuParamIndexRef = 0;
          else menuParamIndexRef++;
          lcdNeedsUpdateRef = true;
        } else if (buttonStateRef == 2) { // LEFT
          uiStateRef = UI_MAIN;
          lcdNeedsUpdateRef = true;
        } else if (buttonStateRef == 5) { // SELECT
          editingParamRef = true;
          lcdNeedsUpdateRef = true;
        }
      } else {
        if (buttonStateRef == 2 || buttonStateRef == 4) {
          bool increment = (buttonStateRef == 4);

          switch (actualParamIndex) {
            case PARAM_TYPE: {
              PadType allowedTypes[allPadTypesCount];
              uint8_t allowedCount = 0;
              for (uint8_t i = 0; i < allPadTypesCount; i++) {
                if (allPadTypes[i] == PAD_DUAL && muxJackMap[editPadIndexRef][1] == -1) continue;
                allowedTypes[allowedCount++] = allPadTypes[i];
              }
              int8_t curTypeIndex = -1;
              for (uint8_t i = 0; i < allowedCount; i++) {
                if (allowedTypes[i] == ps.type) {
                  curTypeIndex = i;
                  break;
                }
              }
              if (curTypeIndex == -1) curTypeIndex = 0;
              if (increment) {
                curTypeIndex++;
                if (curTypeIndex >= allowedCount) curTypeIndex = 0;
              } else {
                curTypeIndex--;
                if (curTypeIndex < 0) curTypeIndex = allowedCount - 1;
              }
              ps.type = allowedTypes[curTypeIndex];
              if (ps.type != PAD_DUAL && ps.type != PAD_HIHAT && ps.type != PAD_CYMBAL) {
                ps.twoZoneMode = false;
                ps.muteByPiezo = false;
              }
              break;
            }
            case PARAM_HEAD_MIDI:
              if (increment) ps.midiHeadNote = (ps.midiHeadNote == 127) ? 0 : ps.midiHeadNote + 1;
              else ps.midiHeadNote = (ps.midiHeadNote == 0) ? 127 : ps.midiHeadNote - 1;
              break;
            case PARAM_RIM_MIDI:
              if (increment) ps.midiRimNote = (ps.midiRimNote == 127) ? 0 : ps.midiRimNote + 1;
              else ps.midiRimNote = (ps.midiRimNote == 0) ? 127 : ps.midiRimNote - 1;
              break;
            case PARAM_SENSITIVITY:
              if (increment) ps.sensitivity = (ps.sensitivity == 127) ? 0 : ps.sensitivity + 1;
              else ps.sensitivity = (ps.sensitivity == 0) ? 127 : ps.sensitivity - 1;
              break;
            case PARAM_THRESHOLD:
              if (increment) ps.threshold = (ps.threshold == 1023) ? 0 : ps.threshold + 1;
              else ps.threshold = (ps.threshold == 0) ? 1023 : ps.threshold - 1;
              break;
            case PARAM_CURVE: {
              int8_t curCurveIndex = -1;
              for (uint8_t i = 0; i < curveCount; i++) {
                if (ps.curve == allowedCurves[i]) {
                  curCurveIndex = i;
                  break;
                }
              }
              if (curCurveIndex == -1) curCurveIndex = 0;
              if (increment) {
                curCurveIndex++;
                if (curCurveIndex >= curveCount) curCurveIndex = 0;
              } else {
                if (curCurveIndex == 0) curCurveIndex = curveCount - 1;
                else curCurveIndex--;
              }
              ps.curve = allowedCurves[curCurveIndex];
              break;
            }
            case PARAM_RIM_MUTE:
              ps.rimMute = !ps.rimMute;
              break;
            case PARAM_TWO_ZONE_MODE:
              if (muxJackMap[editPadIndexRef][1] != -1) ps.twoZoneMode = !ps.twoZoneMode;
              else ps.twoZoneMode = false;
              break;
            case PARAM_MUTE_BY_PIEZO:
              if (muxJackMap[editPadIndexRef][1] != -1) ps.muteByPiezo = !ps.muteByPiezo;
              else ps.muteByPiezo = false;
              break;
          }
          saveSettings(deviceSettingsRef);
          lcdNeedsUpdateRef = true;
        } else if (buttonStateRef == 5) {
          editingParamRef = false;
          saveSettings(deviceSettingsRef);
          lcdNeedsUpdateRef = true;
        }
      }
    }
    break;

    case UI_EDIT_HIHAT: {
      HiHatSettings &hihat = deviceSettingsRef.hihat;
      if (!editingHiHatParam) {
        if (buttonStateRef == 1) {
          if (hiHatMenuIndex == 0) hiHatMenuIndex = HIHAT_NUM_PARAMS - 1;
          else hiHatMenuIndex--;
          lcdNeedsUpdateRef = true;
        } else if (buttonStateRef == 3) {
          if (hiHatMenuIndex == HIHAT_NUM_PARAMS - 1) hiHatMenuIndex = 0;
          else hiHatMenuIndex++;
          lcdNeedsUpdateRef = true;
        } else if (buttonStateRef == 5) {
          editingHiHatParam = true;
          lcdNeedsUpdateRef = true;
        } else if (buttonStateRef == 2) {
          uiStateRef = UI_MAIN;
          lcdNeedsUpdateRef = true;
        }
      } else {
        if (buttonStateRef == 2 || buttonStateRef == 4) {
          bool increment = (buttonStateRef == 4);
          switch (hiHatMenuIndex) {
            case HIHAT_CC_CLOSED:
              if (increment) hihat.ccClosed = (hihat.ccClosed == 127) ? 0 : hihat.ccClosed + 1;
              else hihat.ccClosed = (hihat.ccClosed == 0) ? 127 : hihat.ccClosed - 1;
              break;
            case HIHAT_CC_OPEN:
              if (increment) hihat.ccOpen = (hihat.ccOpen == 127) ? 0 : hihat.ccOpen + 1;
              else hihat.ccOpen = (hihat.ccOpen == 0) ? 127 : hihat.ccOpen - 1;
              break;
            case HIHAT_CC_STEP:
              if (increment) hihat.ccStep = (hihat.ccStep == 127) ? 0 : hihat.ccStep + 1;
              else hihat.ccStep = (hihat.ccStep == 0) ? 127 : hihat.ccStep - 1;
              break;
            case HIHAT_INVERT:
              if (increment || !increment) hihat.invert = !hihat.invert;
              break;
          }
          saveSettings(deviceSettingsRef);
          lcdNeedsUpdateRef = true;
        } else if (buttonStateRef == 5) {
          editingHiHatParam = false;
          lcdNeedsUpdateRef = true;
        }
      }
    }
    break;

    case UI_EDIT_XTALK: {
      PadSettings &ps = deviceSettingsRef.pads[xtalkPadIndex];
      if (!editingXtalk) {
        if (buttonStateRef == 1) {
          if (xtalkPadIndex == 0) xtalkPadIndex = NUM_JACKS - 1;
          else xtalkPadIndex--;
          lcdNeedsUpdateRef = true;
        } else if (buttonStateRef == 3) {
          if (xtalkPadIndex == NUM_JACKS - 1) xtalkPadIndex = 0;
          else xtalkPadIndex++;
          lcdNeedsUpdateRef = true;
        } else if (buttonStateRef == 5) {
          editingXtalk = true;
          lcdNeedsUpdateRef = true;
        } else if (buttonStateRef == 2) {
          uiStateRef = UI_MAIN;
          lcdNeedsUpdateRef = true;
        }
      } else {
        if (buttonStateRef == 2 || buttonStateRef == 4) {
          bool increment = (buttonStateRef == 4);
          if (xtalkMenuParamIndex == 0) {
            if (increment) {
              if (ps.xtalkThreshold == 127) ps.xtalkThreshold = 0;
              else ps.xtalkThreshold++;
            } else {
              if (ps.xtalkThreshold == 0) ps.xtalkThreshold = 127;
              else ps.xtalkThreshold--;
            }
          } else if (xtalkMenuParamIndex == 1) {
            uint16_t step = 5;
            uint16_t maxTime = 250;
            uint16_t &timeParam = ps.xtalkCancelTime;
            if (increment) {
              timeParam = (timeParam + step > maxTime) ? 0 : timeParam + step;
            } else {
              timeParam = (timeParam < step) ? maxTime : timeParam - step;
            }
          }
          saveSettings(deviceSettingsRef);
          lcdNeedsUpdateRef = true;
        } else if (buttonStateRef == 5) {
          editingXtalk = false;
          saveSettings(deviceSettingsRef);
          lcdNeedsUpdateRef = true;
        }
      }
    }
    break;

    case UI_CONFIRM_RESET:
      if (buttonStateRef == 2 || buttonStateRef == 4) {
        resetYesSelected = !resetYesSelected;
        lcdNeedsUpdateRef = true;
      } else if (buttonStateRef == 5) {
        if (resetYesSelected) {
          initSettings(deviceSettingsRef);
          saveSettings(deviceSettingsRef);
          uiStateRef = UI_MAIN;
          lcdNeedsUpdateRef = true;
        } else {
          uiStateRef = UI_MAIN;
          lcdNeedsUpdateRef = true;
        }
      }
      break;
  }
}
