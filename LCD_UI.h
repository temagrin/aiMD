//файл LCD_UI.h
#ifndef LCD_UI_H
#define LCD_UI_H

#include <LiquidCrystal_I2C.h>
#include "Config.h"   // Чтобы использовать UIState, ParamType, константы, enum и др.
#include "UIAction.h"
#include "Mux.h"
#include "Settings.h"
#include "Pads.h"     // Для типа PadStatus

// Внешние функции для сохранения и инициализации настроек
extern void saveSettings(const Settings &settings);
extern void initSettings(Settings &settings);

// Внешняя глобальная переменная отслеживания необходимости обновления дисплея
extern bool lcdNeedsUpdate_;

// Внешний объект LCD — определён в основном модуле (aiMD.ino или main)
extern LiquidCrystal_I2C lcd;

// Информация о параметрах пэда
struct PadParamInfo {
  uint8_t paramIndex;
  const char *name;
  ParamType paramType;
  bool (*isAvailable)(const PadSettings &, uint8_t idx);
};

// Абстрактный базовый класс меню
class MenuBase {
public:
  MenuBase(UIState &ui, bool &lcdNeedsUpdate) : uiState_(ui), lcdNeedsUpdate_(lcdNeedsUpdate) {}
  virtual ~MenuBase() {}

  virtual void render() = 0;
  virtual bool isActive() const = 0;
  virtual void handleAction(UIAction action) = 0;

protected:
  UIState &uiState_;
  bool &lcdNeedsUpdate_;
};


// === MainMenu ===
class MainMenu : public MenuBase {
public:
  MainMenu(uint8_t &mainMenuSelection, UIState &ui, bool &lcdNeedsUpdate);

  void onActiveItemPrev();
  void onActiveItemNext();
  void onSave();

  void render() override;
  bool isActive() const override;
  void handleAction(UIAction action) override;

private:
  uint8_t &mainMenuSelection_;
};


// === EditPadMenu ===
class EditPadMenu : public MenuBase {
public:
  EditPadMenu(Settings &deviceSettings, uint8_t &editPadIndex, uint8_t &menuParamIndex,
              bool &editingParam, UIState &ui, bool &lcdNeedsUpdate, LiquidCrystal_I2C &lcd);

  void onActiveItemPrev();
  void onActiveItemNext();
  void onParamPrev();
  void onParamNext();
  void onParamIncrease();
  void onParamDecrease();
  void onSave();
  void onBack();

  void render() override;
  bool isActive() const override { return uiState_ == UI_EDIT_PAD; }
  void handleAction(UIAction action) override;
  const PadParamInfo* getAvailableParams(uint8_t &count) const;

private:
  void changeParameter(bool increment);
  
  Settings &deviceSettings_;
  uint8_t &editPadIndex_;
  uint8_t &menuParamIndex_;
  bool &editingParam_;
  LiquidCrystal_I2C &lcd_;
};


// === EditHiHatMenu ===
class EditHiHatMenu : public MenuBase {
public:
  EditHiHatMenu(Settings &deviceSettings, bool &editingParam, uint8_t &menuIndex, UIState &ui,
                bool &lcdNeedsUpdate, LiquidCrystal_I2C &lcd);
  virtual ~EditHiHatMenu();
  void onActiveItemPrev();
  void onActiveItemNext();
  void onParamIncrease();
  void onParamDecrease();
  void onSave();
  void onBack();

  void render() override;
  bool isActive() const override { return uiState_ == UI_EDIT_HIHAT; }
  void handleAction(UIAction action) override;

private:
  Settings &deviceSettings_;
  bool &editingHiHatParam_;
  uint8_t &hiHatMenuIndex_;
  LiquidCrystal_I2C &lcd_;
};


// === EditXtalkMenu ===
class EditXtalkMenu : public MenuBase {
public:
  EditXtalkMenu(Settings &deviceSettings, bool &editingXtalk, uint8_t &xtalkPadIndex,
                uint8_t &xtalkMenuParamIndex, UIState &ui, bool &lcdNeedsUpdate, LiquidCrystal_I2C &lcd);
  virtual ~EditXtalkMenu();
  void onActiveItemPrev();
  void onActiveItemNext();
  void onParamIncrease();
  void onParamDecrease();
  void onParamNext();
  void onParamPrev();
  void onSave();
  void onBack();

  void render() override;
  bool isActive() const override { return uiState_ == UI_EDIT_XTALK; }
  void handleAction(UIAction action) override;

private:
  Settings &deviceSettings_;
  bool &editingXtalk_;
  uint8_t &xtalkPadIndex_;
  uint8_t &xtalkMenuParamIndex_;
  LiquidCrystal_I2C &lcd_;
};


// === ConfirmResetMenu ===
class ConfirmResetMenu : public MenuBase {
public:
  ConfirmResetMenu(UIState &ui, bool &lcdNeedsUpdate, bool &resetYesSelected,
                   LiquidCrystal_I2C &lcd, Settings &deviceSettings);
  virtual ~ConfirmResetMenu();
  void onParamIncrease();
  void onParamDecrease();
  void onSave();
  void onBack();

  void render() override;
  bool isActive() const override { return uiState_ == UI_CONFIRM_RESET; }
  void handleAction(UIAction action) override;

private:
  bool &resetYesSelected_;
  LiquidCrystal_I2C &lcd_;
  Settings &deviceSettings_;
};


// Дополнительные функции для отображения меню


void lcdPrintCentered(const char *str);

void displayMainMenu(uint8_t selectedItem);

void displayPadEditMenu(const Settings &deviceSettings, uint8_t padIdx, uint8_t menuParamIdx, bool editingParam, bool &lcdNeedsUpdateRef);

void displayHiHatEditMenu(const HiHatSettings &hihat, uint8_t menuIndex, bool editing, bool &lcdNeedsUpdateRef);

void displayXtalkMenu(const Settings &deviceSettings, uint8_t padIdx, uint8_t menuParamIndex, bool editing);

void displayConfirmReset();

void processUI(Settings &deviceSettings, PadStatus padStatus[], int8_t &buttonState, bool &debugMode,
               bool &editingParam, uint8_t &menuParamIndex, uint8_t &editPadIndex, unsigned long &lastActivity,
               UIState &uiState, bool &lcdNeedsUpdate);

#endif // LCD_UI_H
