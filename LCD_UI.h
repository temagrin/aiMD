#ifndef LCD_UI_H
#define LCD_UI_H

#include <Arduino.h>
#include "Settings.h"
#include "Config.h"
#include "Pads.h"

#include "LCD_UI.h"
#include <LiquidCrystal_I2C.h>
#include "Settings.h"
#include "Config.h"
#include "Mux.h"
#include "Pads.h"

extern LiquidCrystal_I2C lcd;

// Внешние глобальные переменные
extern uint8_t mainMenuSelection;
extern uint8_t hiHatMenuIndex;
extern bool editingHiHatParam;
extern bool editingXtalk;
extern uint8_t xtalkPadIndex;
extern uint8_t xtalkMenuParamIndex;

extern bool editingParam;
extern uint8_t menuParamIndex;
extern uint8_t editPadIndex;
extern UIState uiState;
extern bool lcdNeedsUpdate;
extern bool debugMode;
extern unsigned long lastActivity;
extern int8_t buttonState;
void lcdInit();

void displayMainMenu(uint8_t selectedItem);
void displayPadEditMenu(const Settings &deviceSettings, uint8_t padIdx, uint8_t menuParamIndex, bool editingParam, bool &lcdNeedsUpdate);
void displayHiHatEditMenu(const HiHatSettings &hihat, uint8_t menuIndex, bool editing, bool &lcdNeedsUpdate);
void displayXtalkMenu(const Settings &deviceSettings, uint8_t padIdx, uint8_t menuParamIdx, bool editingXtalk);
void displayConfirmReset();

void processUI(Settings &deviceSettingsRef, PadStatus padStatusRef[], int8_t &buttonStateRef, bool &debugModeRef,
               bool &editingParamRef, uint8_t &menuParamIndexRef, uint8_t &editPadIndexRef,
               unsigned long &lastActivityRef, UIState &uiStateRef, bool &lcdNeedsUpdateRef);

#endif