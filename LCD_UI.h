#ifndef LCD_UI_H
#define LCD_UI_H

#include "UIAction.h"
#include "Mux.h"



extern void saveSettings(const Settings &);
extern void initSettings(Settings &);

static const PadParamInfo padParams[] = {
    {PARAM_TYPE, "Type", PARAM_ENUM, [](const PadSettings &, uint8_t) { return true; }},
    {PARAM_HEAD_MIDI, "HeadMidi", PARAM_INT, [](const PadSettings &, uint8_t) { return true; }},
    {PARAM_SENSITIVITY, "Sensitivity", PARAM_INT, [](const PadSettings &, uint8_t) { return true; }},
    {PARAM_THRESHOLD, "Threshold", PARAM_INT, [](const PadSettings &, uint8_t) { return true; }},
    {PARAM_CURVE, "Curve", PARAM_ENUM, [](const PadSettings &, uint8_t) { return true; }},
    {PARAM_RIM_MIDI, "RimMidi", PARAM_INT, [](const PadSettings &, uint8_t idx) { return muxJackMap[idx][1] != -1; }},
    {PARAM_RIM_MUTE, "RimMute", PARAM_BOOL, [](const PadSettings &, uint8_t idx) { return muxJackMap[idx][1] != -1; }},
    {PARAM_TWO_ZONE_MODE, "TwoZone", PARAM_BOOL, [](const PadSettings &, uint8_t idx) { return muxJackMap[idx][1] != -1; }},
    {PARAM_MUTE_BY_PIEZO, "MutePiezo", PARAM_BOOL, [](const PadSettings &, uint8_t idx) { return muxJackMap[idx][1] != -1; }},
    {PARAM_SCANTIME, "ScanTime", PARAM_INT, [](const PadSettings &, uint8_t) { return true; }},
    {PARAM_MASKTIME, "MaskTime", PARAM_INT, [](const PadSettings &, uint8_t) { return true; }},
};

static const char* curveNames[] = {"Lin", "Exp", "Log", "MaxV"};
static const CurveType allowedCurves[] = {
    CURVE_LINEAR, CURVE_EXPONENTIAL, CURVE_LOG, CURVE_MAX_VELOCITY
};

// *** MainMenu ***

MainMenu::MainMenu(uint8_t &m, UIState &ui, bool &lcd)
  : MenuBase(ui, lcd), mainMenuSelection_(m) {}

void MainMenu::onActiveItemPrev() {
  if (mainMenuSelection_ == 0)
    mainMenuSelection_ = MENU_ITEMS_COUNT - 1;
  else
    mainMenuSelection_--;
  lcdNeedsUpdate_ = true;
}

void MainMenu::onActiveItemNext() {
  mainMenuSelection_ = (mainMenuSelection_ + 1) % MENU_ITEMS_COUNT;
  lcdNeedsUpdate_ = true;
}

void MainMenu::onSave() {
  switch (mainMenuSelection_) {
    case MENU_EDIT_PADS:
      uiState_ = UI_EDIT_PAD;
      break;
    case MENU_EDIT_HIHAT:
      uiState_ = UI_EDIT_HIHAT;
      break;
    case MENU_EDIT_XTALK:
      uiState_ = UI_EDIT_XTALK;
      break;
    case MENU_RESET_DEFAULTS:
      uiState_ = UI_CONFIRM_RESET;
      break;
  }
  lcdNeedsUpdate_ = true;
}

void MainMenu::render() {
  lcd.clear();
  const char* menuItems[MENU_ITEMS_COUNT] = {"Inputs", "HiHat Pedal", "XTALK", "Reset Defaults"};
  uint8_t displayOffset = 0;
  if (mainMenuSelection_ >= 1 && MENU_ITEMS_COUNT > 2) displayOffset = mainMenuSelection_ - 1;
  if (displayOffset >= MENU_ITEMS_COUNT - 1 && MENU_ITEMS_COUNT > 1) displayOffset = MENU_ITEMS_COUNT - 2;
  if (MENU_ITEMS_COUNT == 1) displayOffset = 0;
  for (uint8_t i = 0; i < 2; i++) {
    uint8_t idx = displayOffset + i;
    lcd.setCursor(0, i);
    if (idx < MENU_ITEMS_COUNT) {
      lcd.print((idx == mainMenuSelection_) ? ">" : " ");
      lcd.print(menuItems[idx]);
    } else {
      lcd.print("                ");
    }
  }
}

bool MainMenu::isActive() const { return uiState_ == UI_MAIN; }

// *** EditPadMenu ***

EditPadMenu::EditPadMenu(Settings &ds, uint8_t &epi, uint8_t &mpi, bool &ep,
  UIState &ui, bool &lcd, LiquidCrystal_I2C &l)
  : MenuBase(ui, lcd), deviceSettings_(ds), editPadIndex_(epi),
    menuParamIndex_(mpi), editingParam_(ep), lcd_(l) {}

const PadParamInfo* EditPadMenu::getAvailableParams(uint8_t &count) const {
  static PadParamInfo avail[NUM_TOTAL_PAD_PARAMS];
  count = 0;
  auto &ps = deviceSettings_.pads[editPadIndex_];
  for (auto &p : padParams) {
    if (p.isAvailable(ps, editPadIndex_)) {
      avail[count++] = p;
    }
  }
  return avail;
}

void EditPadMenu::onActiveItemPrev() {
  if (!editingParam_) {
    editPadIndex_ = (editPadIndex_ == 0) ? NUM_JACKS - 1 : editPadIndex_ - 1;
    menuParamIndex_ = 0;
    lcdNeedsUpdate_ = true;
  } else {
    changeParameter(false);
  }
}

void EditPadMenu::onActiveItemNext() {
  if (!editingParam_) {
    editPadIndex_ = (editPadIndex_ + 1) % NUM_JACKS;
    menuParamIndex_ = 0;
    lcdNeedsUpdate_ = true;
  } else {
    changeParameter(true);
  }
}

void EditPadMenu::onParamPrev() {
  if (!editingParam_) {
    uint8_t count = 0;
    getAvailableParams(count);
    if (count > 0) {
      menuParamIndex_ = (menuParamIndex_ == 0) ? count - 1 : menuParamIndex_ - 1;
      lcdNeedsUpdate_ = true;
    }
  } else {
    changeParameter(false);
  }
}

void EditPadMenu::onParamNext() {
  if (!editingParam_) {
    uint8_t count = 0;
    getAvailableParams(count);
    if (count > 0) {
      menuParamIndex_ = (menuParamIndex_ + 1) % count;
      lcdNeedsUpdate_ = true;
    }
  } else {
    changeParameter(true);
  }
}

void EditPadMenu::onParamIncrease() {
  if (editingParam_) {
    changeParameter(true);
  } else {
    editingParam_ = true;
    lcdNeedsUpdate_ = true;
  }
}

void EditPadMenu::onParamDecrease() {
  if (editingParam_) {
    changeParameter(false);
  }
}

void EditPadMenu::onSave() {
  if (editingParam_) {
    editingParam_ = false;
    saveSettings(deviceSettings_);
    lcdNeedsUpdate_ = true;
  }
}

void EditPadMenu::onBack() {
  if (editingParam_) {
    editingParam_ = false;
    lcdNeedsUpdate_ = true;
  } else {
    uiState_ = UI_MAIN;
    lcdNeedsUpdate_ = true;
  }
}

void EditPadMenu::changeParameter(bool increment) {
  uint8_t count = 0;
  const PadParamInfo* available = getAvailableParams(count);
  if (count == 0) return;

  const PadParamInfo &paramInfo = available[menuParamIndex_];
  PadSettings &ps = deviceSettings_.pads[editPadIndex_];

  switch (paramInfo.paramIndex) {
    case PARAM_TYPE: {
      PadType allowedTypes[allPadTypesCount];
      uint8_t allowedCount = 0;
      for (uint8_t i=0; i<allPadTypesCount; i++) {
        if (allPadTypes[i] == PAD_DUAL && muxJackMap[editPadIndex_][1] == -1) continue;
        allowedTypes[allowedCount++] = allPadTypes[i];
      }
      int8_t curTypeIndex = -1;
      for (uint8_t i=0; i<allowedCount; i++) {
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
      ps.midiHeadNote = increment ? (ps.midiHeadNote + 1) % 128 : (ps.midiHeadNote == 0 ? 127 : ps.midiHeadNote - 1);
      break;
    case PARAM_RIM_MIDI:
      ps.midiRimNote = increment ? (ps.midiRimNote + 1) % 128 : (ps.midiRimNote == 0 ? 127 : ps.midiRimNote - 1);
      break;
    case PARAM_SENSITIVITY:
      ps.sensitivity = increment ? (ps.sensitivity + 1) % 128 : (ps.sensitivity == 0 ? 127 : ps.sensitivity - 1);
      break;
    case PARAM_THRESHOLD:
      ps.threshold = increment ? (ps.threshold + 1) % 1024 : (ps.threshold == 0 ? 1023 : ps.threshold - 1);
      break;
    case PARAM_CURVE: {
      int8_t idx = -1;
      uint8_t len = sizeof(allowedCurves) / sizeof(allowedCurves[0]);
      for (uint8_t i = 0; i< len; i++) {
        if (ps.curve == allowedCurves[i]) {
          idx = i;
          break;
        }
      }
      if (idx == -1) idx = 0;
      if (increment) idx++;
      else idx--;
      if ((uint8_t)idx >= len) idx = 0;
      if (idx < 0) idx = len - 1;
      ps.curve = allowedCurves[idx];
      break;
    }
    case PARAM_SCANTIME:
      ps.scantime = increment ? (ps.scantime == 255 ? 0 : ps.scantime + 1) : (ps.scantime == 0 ? 255 : ps.scantime - 1);
      break;
    case PARAM_MASKTIME:
      ps.masktime = increment ? (ps.masktime == 255 ? 0 : ps.masktime + 1) : (ps.masktime == 0 ? 255 : ps.masktime - 1);
      break;
    case PARAM_RIM_MUTE:
      ps.rimMute = !ps.rimMute;
      break;
    case PARAM_TWO_ZONE_MODE:
      ps.twoZoneMode = muxJackMap[editPadIndex_][1] != -1 ? !ps.twoZoneMode : false;
      break;
    case PARAM_MUTE_BY_PIEZO:
      ps.muteByPiezo = muxJackMap[editPadIndex_][1] != -1 ? !ps.muteByPiezo : false;
      break;
  }
}

void EditPadMenu::render() {
  displayPadEditMenu(deviceSettings_, editPadIndex_, menuParamIndex_, editingParam_, lcdNeedsUpdate_);
}

// *** EditHiHatMenu ***

EditHiHatMenu::EditHiHatMenu(Settings &ds, bool &editingParam, uint8_t &menuIndex,
  UIState &ui, bool &lcd, LiquidCrystal_I2C &l)
  : MenuBase(ui, lcd), deviceSettings_(ds), editingHiHatParam_(editingParam),
    hiHatMenuIndex_(menuIndex), lcd_(l) {}

void EditHiHatMenu::onActiveItemPrev() {
  if (!editingHiHatParam_) {
    hiHatMenuIndex_ = (hiHatMenuIndex_ == 0) ? HIHAT_NUM_PARAMS - 1 : hiHatMenuIndex_ - 1;
    lcdNeedsUpdate_ = true;
  }
}

void EditHiHatMenu::onActiveItemNext() {
  if (!editingHiHatParam_) {
    hiHatMenuIndex_ = (hiHatMenuIndex_ + 1) % HIHAT_NUM_PARAMS;
    lcdNeedsUpdate_ = true;
  }
}

void EditHiHatMenu::onParamIncrease() {
  if (editingHiHatParam_) {
    HiHatSettings &hihat = deviceSettings_.hihat;
    switch (hiHatMenuIndex_) {
      case HIHAT_CC_CLOSED:
        hihat.ccClosed = (hihat.ccClosed == 127) ? 0 : hihat.ccClosed + 1;
        break;
      case HIHAT_CC_OPEN:
        hihat.ccOpen = (hihat.ccOpen == 127) ? 0 : hihat.ccOpen + 1;
        break;
      case HIHAT_CC_STEP:
        hihat.ccStep = (hihat.ccStep == 127) ? 0 : hihat.ccStep + 1;
        break;
      case HIHAT_INVERT:
        hihat.invert = !hihat.invert;
        break;
    }
    saveSettings(deviceSettings_);
    lcdNeedsUpdate_ = true;
  }
}

void EditHiHatMenu::onParamDecrease() {
  if (editingHiHatParam_) {
    HiHatSettings &hihat = deviceSettings_.hihat;
    switch (hiHatMenuIndex_) {
      case HIHAT_CC_CLOSED:
        hihat.ccClosed = (hihat.ccClosed == 0) ? 127 : hihat.ccClosed - 1;
        break;
      case HIHAT_CC_OPEN:
        hihat.ccOpen = (hihat.ccOpen == 0) ? 127 : hihat.ccOpen - 1;
        break;
      case HIHAT_CC_STEP:
        hihat.ccStep = (hihat.ccStep == 0) ? 127 : hihat.ccStep - 1;
        break;
      case HIHAT_INVERT:
        hihat.invert = !hihat.invert;
        break;
    }
    saveSettings(deviceSettings_);
    lcdNeedsUpdate_ = true;
  }
}

void EditHiHatMenu::onSave() {
  editingHiHatParam_ = !editingHiHatParam_;
  lcdNeedsUpdate_ = true;
}

void EditHiHatMenu::onBack() {
  if (!editingHiHatParam_) {
    uiState_ = UI_MAIN;
    lcdNeedsUpdate_ = true;
  } else {
    editingHiHatParam_ = false;
    lcdNeedsUpdate_ = true;
  }
}

void EditHiHatMenu::render() {
  displayHiHatEditMenu(deviceSettings_.hihat, hiHatMenuIndex_, editingHiHatParam_, lcdNeedsUpdate_);
}

bool EditHiHatMenu::isActive() const { return uiState_ == UI_EDIT_HIHAT; }

// *** EditXtalkMenu ***

EditXtalkMenu::EditXtalkMenu(Settings &ds, bool &editXtalk, uint8_t &padIndex,
  uint8_t &paramIndex, UIState &ui, bool &lcd, LiquidCrystal_I2C &l)
  : MenuBase(ui, lcd), deviceSettings_(ds), editingXtalk_(editXtalk),
    xtalkPadIndex_(padIndex), xtalkMenuParamIndex_(paramIndex), lcd_(l) {}

void EditXtalkMenu::onActiveItemPrev() {
  if (!editingXtalk_) {
    xtalkPadIndex_ = (xtalkPadIndex_ == 0) ? NUM_JACKS - 1 : xtalkPadIndex_ - 1;
    lcdNeedsUpdate_ = true;
  }
}

void EditXtalkMenu::onActiveItemNext() {
  if (!editingXtalk_) {
    xtalkPadIndex_ = (xtalkPadIndex_ + 1) % NUM_JACKS;
    lcdNeedsUpdate_ = true;
  }
}

void EditXtalkMenu::onParamIncrease() {
  if (editingXtalk_) {
    PadSettings &ps = deviceSettings_.pads[xtalkPadIndex_];
    if (xtalkMenuParamIndex_ == 0) {
      ps.xtalkThreshold = (ps.xtalkThreshold == 127) ? 0 : ps.xtalkThreshold + 1;
    } else if (xtalkMenuParamIndex_ == 1) {
      uint16_t step = 5, maxTime = 250;
      ps.xtalkCancelTime = (ps.xtalkCancelTime + step > maxTime) ? 0 : ps.xtalkCancelTime + step;
    }
    saveSettings(deviceSettings_);
    lcdNeedsUpdate_ = true;
  }
}

void EditXtalkMenu::onParamDecrease() {
  if (editingXtalk_) {
    PadSettings &ps = deviceSettings_.pads[xtalkPadIndex_];
    if (xtalkMenuParamIndex_ == 0) {
      ps.xtalkThreshold = (ps.xtalkThreshold == 0) ? 127 : ps.xtalkThreshold - 1;
    } else if (xtalkMenuParamIndex_ == 1) {
      uint16_t step = 5, maxTime = 250;
      ps.xtalkCancelTime = (ps.xtalkCancelTime < step) ? maxTime : ps.xtalkCancelTime - step;
    }
    saveSettings(deviceSettings_);
    lcdNeedsUpdate_ = true;
  }
}

void EditXtalkMenu::onSave() {
  editingXtalk_ = !editingXtalk_;
  if (!editingXtalk_) saveSettings(deviceSettings_);
  lcdNeedsUpdate_ = true;
}

void EditXtalkMenu::onBack() {
  if (!editingXtalk_) {
    uiState_ = UI_MAIN;
    lcdNeedsUpdate_ = true;
  } else {
    editingXtalk_ = false;
    lcdNeedsUpdate_ = true;
  }
}

void EditXtalkMenu::render() {
  displayXtalkMenu(deviceSettings_, xtalkPadIndex_, xtalkMenuParamIndex_, editingXtalk_);
}

bool EditXtalkMenu::isActive() const { return uiState_ == UI_EDIT_XTALK; }


// *** ConfirmResetMenu ***

ConfirmResetMenu::ConfirmResetMenu(UIState &ui, bool &lcd, bool &resetYesSelected,
  LiquidCrystal_I2C &l, Settings &ds)
  : MenuBase(ui, lcd), resetYesSelected_(resetYesSelected), lcd_(l), deviceSettings_(ds) {}

void ConfirmResetMenu::onParamIncrease() {
  resetYesSelected_ = !resetYesSelected_;
  lcdNeedsUpdate_ = true;
}

void ConfirmResetMenu::onParamDecrease() {
  resetYesSelected_ = !resetYesSelected_;
  lcdNeedsUpdate_ = true;
}

void ConfirmResetMenu::onSave() {
  if (resetYesSelected_) {
    initSettings(deviceSettings_);
    saveSettings(deviceSettings_);
  }
  uiState_ = UI_MAIN;
  lcdNeedsUpdate_ = true;
}

void ConfirmResetMenu::onBack() {
  uiState_ = UI_MAIN;
  lcdNeedsUpdate_ = true;
}

void ConfirmResetMenu::render() {
  lcd.clear();
  lcdPrintCentered("Reset to Defaults?");
  lcd.setCursor(0, 1);
  if (resetYesSelected_) {
    lcd.print("> YES    NO ");
  } else {
    lcd.print("  YES    >NO");
  }
}

bool ConfirmResetMenu::isActive() const { return uiState_ == UI_CONFIRM_RESET; }

#endif // LCD_UI_H
