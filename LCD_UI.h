#ifndef LCD_UI_H
#define LCD_UI_H

#include "Settings.h"
#include "Config.h"
#include "Pads.h"

extern uint8_t mainMenuSelection;
extern uint8_t hiHatMenuIndex;
extern bool editingHiHatParam;
extern uint8_t xtalkPadIndex;
extern bool editingXtalk;


void lcdInit();
void displayHiHatEditMenu(const HiHatSettings &hihat, uint8_t menuIndex, bool editing, bool &lcdNeedsUpdate);
void displayMainMenu(uint8_t selectedItem);
void displayPadEditMenu(const Settings &deviceSettings, uint8_t padIdx, uint8_t menuParamIndex, bool editingParam, bool &lcdNeedsUpdate);
void displayConfirmReset();
void processUI(Settings &deviceSettingsRef, PadStatus padStatusRef[], int8_t &buttonStateRef, bool &debugModeRef,
               bool &editingParamRef, uint8_t &menuParamIndexRef, uint8_t &editPadIndexRef,
               unsigned long &lastActivityRef, UIState &uiStateRef, bool &lcdNeedsUpdateRef);

#endif