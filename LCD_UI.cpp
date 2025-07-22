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

// Глобальные переменные для состояний меню, которые должны сохраняться между вызовами
// (они должны быть определены в одном .cpp файле, но доступны из других функций LCD_UI.cpp)
static uint8_t mainMenuSelection = 0; // Для выбора в главном меню

static uint8_t hiHatMenuIndex = 0;
static bool editingHiHatParam = false;

static bool editingXtalk = false;
static uint8_t xtalkPadIndex = 0;

static bool resetYesSelected = true; // Для UI_CONFIRM_RESET

// Объявление прототипов функций для использования внутри LCD_UI.cpp
void lcdPrintCentered(const char *str); // Прототип для локальной функции
extern void saveSettings(const Settings &settings); // Прототип для функции из Settings.cpp (из Settings.cpp)
extern void initSettings(Settings &settings); // Прототип для функции из Settings.cpp

// --- Pad Parameters Definition ---
// extern const char* allParamNames[NUM_TOTAL_PAD_PARAMS]; // Объявлен в Config.h, определение в Constants.cpp
// extern const PadType allPadTypes[]; // Объявлен в Config.h, определение в Constants.cpp
// extern const uint8_t allPadTypesCount; // Объявлен в Config.h, определение в Constants.cpp
// extern const char* hiHatParamNames[HIHAT_NUM_PARAMS]; // Объявлен в Config.h, определение в Constants.cpp

// Функция для центрированного вывода на LCD
void lcdPrintCentered(const char *str) {
  int pad = (LCD_COLS - strlen(str)) / 2;
  if (pad < 0) pad = 0;
  lcd.setCursor(pad, 0);
  lcd.print(str);
}

// --- LCD Initialization ---
void lcdInit() {
  lcd.init();
  lcd.backlight();
}

// --- Main Menu Display ---
void displayMainMenu(uint8_t selectedItem) {
  lcd.clear();

  const char* menuItems[MENU_ITEMS_COUNT] = {
    "Toggle Debug Mode",
    "Edit Pads",
    "Edit HiHat Pedal",
    "Edit XTALK"
  };

  // Отображаем меню с прокруткой для 16x2 LCD
  // Показываем текущий выбранный пункт и следующий за ним.
  uint8_t displayOffset = 0; // Смещение для прокрутки

  if (selectedItem >= 1 && MENU_ITEMS_COUNT > 2) {
    displayOffset = selectedItem - 1; // Если выбран не первый, то начинаем показывать с предыдущего
  }

  // Ограничиваем displayOffset, чтобы не выходить за пределы списка
  if (displayOffset >= MENU_ITEMS_COUNT - 1 && MENU_ITEMS_COUNT > 1) {
    displayOffset = MENU_ITEMS_COUNT - 2; // Показываем последние 2 пункта
  }
  if (MENU_ITEMS_COUNT == 1) displayOffset = 0; // Если всего один пункт

  for (uint8_t i = 0; i < 2; ++i) { // Отображаем 2 строки
    uint8_t itemIndex = displayOffset + i;
    if (itemIndex < MENU_ITEMS_COUNT) {
      lcd.setCursor(0, i);
      lcd.print((itemIndex == selectedItem) ? ">" : " ");
      lcd.print(menuItems[itemIndex]);
    } else {
      lcd.setCursor(0, i);
      lcd.print("                "); // Очистить строку если нет пункта
    }
  }
}

// --- Pad Edit Menu Display ---
void displayPadEditMenu(const Settings &deviceSettings, uint8_t padIdx, uint8_t menuParamIdx, bool editingParam, bool &lcdNeedsUpdateRef) {
  const PadSettings &ps = deviceSettings.pads[padIdx];
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Pad ");
  lcd.print(padIdx + 1);

  uint8_t currentParamIndices[NUM_TOTAL_PAD_PARAMS]; // NUM_TOTAL_PAD_PARAMS из Config.h
  uint8_t numAvailableParams = 0;

  currentParamIndices[numAvailableParams++] = PARAM_TYPE;
  currentParamIndices[numAvailableParams++] = PARAM_HEAD_MIDI;
  currentParamIndices[numAvailableParams++] = PARAM_SENSITIVITY;
  currentParamIndices[numAvailableParams++] = PARAM_THRESHOLD;

  if (muxJackMap[padIdx][1] != -1) { // muxJackMap из Mux.h
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
  lcd.print(allParamNames[actualParamIndex]); // allParamNames из Constants.cpp
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

void displayHiHatEditMenu(const HiHatSettings &hihat, uint8_t menuIndex, bool editing, bool &lcdNeedsUpdateRef) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("HiHat Setting");
  lcd.setCursor(0, 1);
  lcd.print(hiHatParamNames[menuIndex]); // hiHatParamNames из Constants.cpp
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

// --- XTALK Edit Menu Display ---
void displayXtalkMenu(const Settings &deviceSettings, uint8_t padIdx, bool editingXtalk) {
  const PadSettings &ps = deviceSettings.pads[padIdx];
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("XTALK Pad ");
  lcd.print(padIdx + 1);
  lcd.setCursor(0, 1);
  lcd.print("Value: ");
  lcd.print(ps.xtalk);
  if (editingXtalk) {
    lcd.setCursor(LCD_COLS - 1, 1);
    lcd.print("*");
  }
  // lcdNeedsUpdateRef = false; // Нет ссылки на lcdNeedsUpdateRef, поэтому не сбрасываем здесь.
}


// --- Confirm Reset Display ---
void displayConfirmReset() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcdPrintCentered("Reset to Defaults?");
  lcd.setCursor(0, 1);
  lcd.print("> YES    NO");
}


// --- Process UI Logic ---
void processUI(Settings &deviceSettingsRef, PadStatus padStatusRef[], int8_t &buttonStateRef, bool &debugModeRef,
               bool &editingParamRef, uint8_t &menuParamIndexRef, uint8_t &editPadIndexRef, unsigned long &lastActivityRef,
               UIState &uiStateRef, bool &lcdNeedsUpdateRef) {

  const unsigned long timeout = 15000;
  unsigned long now = millis();

  // Таймаут выхода из редактирования при бездействии
  if (editingParamRef && (now - lastActivityRef > timeout)) {
    editingParamRef = false;
    uiStateRef = UI_MAIN;
    lcdNeedsUpdateRef = true;
  }
  // Для HiHat и XTalk тоже:
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


  switch (uiStateRef) {
    case UI_MAIN:
      if (buttonStateRef == 1) { // UP
        if (mainMenuSelection == 0) mainMenuSelection = MENU_ITEMS_COUNT - 1;
        else mainMenuSelection--;
        lcdNeedsUpdateRef = true;
      } else if (buttonStateRef == 3) { // DOWN
        if (mainMenuSelection == MENU_ITEMS_COUNT - 1) mainMenuSelection = 0;
        else mainMenuSelection = (mainMenuSelection + 1) % MENU_ITEMS_COUNT; // Используем % для циклического переключения
        lcdNeedsUpdateRef = true;
      } else if (buttonStateRef == 5) { // SELECT
        switch(mainMenuSelection) {
          case MENU_DEBUG_TOGGLE:
            debugModeRef = !debugModeRef;
            if (debugModeRef) Serial.begin(DEBUG_BAUD);
            else Serial.begin(MIDI_BAUD);
            lcdNeedsUpdateRef = true;
            break;
          case MENU_EDIT_PADS:
            uiStateRef = UI_EDIT_PAD;
            editPadIndexRef = 0;
            menuParamIndexRef = 0;
            editingParamRef = false;
            lcdNeedsUpdateRef = true;
            break;
          case MENU_EDIT_HIHAT:
            uiStateRef = UI_EDIT_HIHAT;
            hiHatMenuIndex = 0; // Сброс индекса меню HiHat при входе
            editingHiHatParam = false; // Убедиться, что не в режиме редактирования
            lcdNeedsUpdateRef = true;
            break;
          case MENU_EDIT_XTALK:
            uiStateRef = UI_EDIT_XTALK;
            xtalkPadIndex = 0; // Сброс индекса пэда для XTALK при входе
            editingXtalk = false; // Убедиться, что не в режиме редактирования
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
          if (menuParamIndexRef == 0) menuParamIndexRef = numAvailableParams - 1;
          else menuParamIndexRef--;
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
              for(uint8_t i=0; i < allPadTypesCount; i++) {
                if(allPadTypes[i] == PAD_DUAL && muxJackMap[editPadIndexRef][1] == -1) continue;
                allowedTypes[allowedCount++] = allPadTypes[i];
              }

              int8_t curTypeIndex = -1;
              for(uint8_t i = 0; i < allowedCount; i++) {
                if(allowedTypes[i] == ps.type) {
                  curTypeIndex = i;
                  break;
                }
              }
              if(curTypeIndex == -1) curTypeIndex = 0;

              if(increment) {
                curTypeIndex++;
                if(curTypeIndex >= allowedCount) curTypeIndex = 0;
              } else {
                curTypeIndex--;
                if(curTypeIndex < 0) curTypeIndex = allowedCount - 1;
              }
              ps.type = allowedTypes[curTypeIndex];

              if(ps.type != PAD_DUAL && ps.type != PAD_HIHAT && ps.type != PAD_CYMBAL) {
                ps.twoZoneMode = false;
                ps.muteByPiezo = false;
              }
              break;
            }
            case PARAM_HEAD_MIDI:
              if (increment) { if (ps.midiHeadNote == 127) ps.midiHeadNote = 0; else ps.midiHeadNote++; }
              else { if (ps.midiHeadNote == 0) ps.midiHeadNote = 127; else ps.midiHeadNote--; }
              break;
            case PARAM_RIM_MIDI:
              if (increment) { if (ps.midiRimNote == 127) ps.midiRimNote = 0; else ps.midiRimNote++; }
              else { if (ps.midiRimNote == 0) ps.midiRimNote = 127; else ps.midiRimNote--; }
              break;
            case PARAM_SENSITIVITY:
              if (increment) { if (ps.sensitivity == 127) ps.sensitivity = 0; else ps.sensitivity++; }
              else { if (ps.sensitivity == 0) ps.sensitivity = 127; else ps.sensitivity--; }
              break;
            case PARAM_THRESHOLD:
              if (increment) { if (ps.threshold == 1023) ps.threshold = 0; else ps.threshold++; }
              else { if (ps.threshold == 0) ps.threshold = 1023; else ps.threshold--; }
              break;
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

      if(!editingHiHatParam) {
        if(buttonStateRef == 1) { // UP
          if(hiHatMenuIndex == 0) hiHatMenuIndex = HIHAT_NUM_PARAMS - 1;
          else hiHatMenuIndex--;
          lcdNeedsUpdateRef = true;
        } else if(buttonStateRef == 3) { // DOWN
          if(hiHatMenuIndex == HIHAT_NUM_PARAMS - 1) hiHatMenuIndex = 0;
          else hiHatMenuIndex++;
          lcdNeedsUpdateRef = true;
        } else if(buttonStateRef == 5) { // SELECT
          editingHiHatParam = true;
          lcdNeedsUpdateRef = true;
        } else if(buttonStateRef == 2) { // LEFT
          uiStateRef = UI_MAIN;
          lcdNeedsUpdateRef = true;
        }
      } else {
        if(buttonStateRef == 2 || buttonStateRef == 4) {
          bool increment = (buttonStateRef == 4);
          switch(hiHatMenuIndex) {
            case HIHAT_CC_CLOSED:
              if(increment) hihat.ccClosed = (hihat.ccClosed == 127) ? 0 : hihat.ccClosed + 1;
              else hihat.ccClosed = (hihat.ccClosed == 0) ? 127 : hihat.ccClosed - 1;
              break;
            case HIHAT_CC_OPEN:
              if(increment) hihat.ccOpen = (hihat.ccOpen == 127) ? 0 : hihat.ccOpen + 1;
              else hihat.ccOpen = (hihat.ccOpen == 0) ? 127 : hihat.ccOpen - 1;
              break;
            case HIHAT_CC_STEP:
              if(increment) hihat.ccStep = (hihat.ccStep == 127) ? 0 : hihat.ccStep + 1;
              else hihat.ccStep = (hihat.ccStep == 0) ? 127 : hihat.ccStep - 1;
              break;
            case HIHAT_INVERT:
              if(increment || !increment) hihat.invert = !hihat.invert;
              break;
          }
          saveSettings(deviceSettingsRef);
          lcdNeedsUpdateRef = true;
        } else if(buttonStateRef == 5) {
          editingHiHatParam = false;
          lcdNeedsUpdateRef = true;
        }
      }
    }
    break;

    case UI_EDIT_XTALK: {
      PadSettings &ps = deviceSettingsRef.pads[xtalkPadIndex]; // Получаем ссылку на настройки текущего пэда

      if (!editingXtalk) {
        // Навигация по пэдам для настройки XTALK
        if (buttonStateRef == 1) { // UP - предыдущий пэд
          if (xtalkPadIndex == 0) xtalkPadIndex = NUM_JACKS - 1;
          else xtalkPadIndex--;
          lcdNeedsUpdateRef = true;
        } else if (buttonStateRef == 3) { // DOWN - следующий пэд
          if (xtalkPadIndex == NUM_JACKS - 1) xtalkPadIndex = 0;
          else xtalkPadIndex++;
          lcdNeedsUpdateRef = true;
        } else if (buttonStateRef == 5) { // SELECT - начать редактировать параметр xtalk пэда
          editingXtalk = true;
          lcdNeedsUpdateRef = true;
        } else if (buttonStateRef == 2) { // LEFT - выход в главное меню
          uiStateRef = UI_MAIN;
          lcdNeedsUpdateRef = true;
        }
      } else {
        // Редактирование значения xtalk
        if (buttonStateRef == 2 || buttonStateRef == 4) { // LEFT/RIGHT для уменьшения/увеличения
          if (buttonStateRef == 4) { // RIGHT увеличиваем, с циклом по 0-127
            if (ps.xtalk == 127) ps.xtalk = 0;
            else ps.xtalk++;
          } else {  // LEFT уменьшаем
            if (ps.xtalk == 0) ps.xtalk = 127;
            else ps.xtalk--;
          }
          saveSettings(deviceSettingsRef);
          lcdNeedsUpdateRef = true;
        } else if (buttonStateRef == 5) { // SELECT - сохранить и выйти из редактирования
          editingXtalk = false;
          saveSettings(deviceSettingsRef);
          lcdNeedsUpdateRef = true;
        }
      }
    }
    break;

    case UI_CONFIRM_RESET:
      // static bool resetYesSelected = true; // Перенесено в глобальные статические переменные
      if(buttonStateRef == 2 || buttonStateRef == 4) { // LEFT/RIGHT переключаем вариант выбора
        resetYesSelected = !resetYesSelected; // Переключаем
        lcdNeedsUpdateRef = true; // Обновить экран для отображения ">"
      } else if(buttonStateRef == 5) { // SELECT подтверждаем или отменяем
        if(resetYesSelected) {
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

