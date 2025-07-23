//файл LCD_UI.cpp
#include <Arduino.h>

#include "Config.h"
#include "LCD_UI.h"
#include "Settings.h"
#include "Mux.h"
#include "Pads.h"

extern LiquidCrystal_I2C lcd;
extern bool editingParam;
extern uint8_t menuParamIndex;
extern uint8_t editPadIndex;
extern UIState uiState;
extern bool lcdNeedsUpdate;
extern bool debugMode;
extern unsigned long lastActivity;
extern int8_t buttonState;
extern bool editingHiHatParam;
extern uint8_t hiHatMenuIndex;
extern bool editingXtalk;
extern uint8_t xtalkPadIndex;
extern uint8_t xtalkMenuParamIndex;
extern uint8_t mainMenuSelection;

static bool resetYesSelected = true;

const PadParamInfo padParams[] = {
    {PARAM_TYPE, "Type", PARAM_ENUM, [](const PadSettings &, uint8_t) { return true; }},
    {PARAM_HEAD_MIDI, "HeadMidi", PARAM_INT, [](const PadSettings &, uint8_t) { return true; }},
    {PARAM_SENSITIVITY, "Sens", PARAM_INT, [](const PadSettings &, uint8_t) { return true; }},
    {PARAM_THRESHOLD, "Thresh", PARAM_INT, [](const PadSettings &, uint8_t) { return true; }},
    {PARAM_CURVE, "Curve", PARAM_ENUM, [](const PadSettings &, uint8_t) { return true; }},
    {PARAM_SCANTIME, "ScanTime", PARAM_INT, [](const PadSettings &, uint8_t) { return true; }},
    {PARAM_MASKTIME, "MaskTime", PARAM_INT, [](const PadSettings &, uint8_t) { return true; }},
    {PARAM_RIM_MUTE, "RimMute", PARAM_BOOL, [](const PadSettings &, uint8_t idx) { return idx>3; }},
    {PARAM_RIM_MIDI, "RimMidi", PARAM_INT, [](const PadSettings &, uint8_t idx) { return idx>3; }},
    {PARAM_ARIM_MIDI, "ARimMidi", PARAM_INT, [](const PadSettings &, uint8_t idx) { return idx>4; }},
    {PARAM_AHEAD_MIDI, "AHdMidi", PARAM_INT, [](const PadSettings &, uint8_t idx) { return idx>4; }},
    {PARAM_CHOKE, "Choke", PARAM_BOOL, [](const PadSettings &, uint8_t idx) { return idx>4; }},
    
};

const char* curveNames[] = {"Lin", "Exp", "Log", "MaxV"};
const CurveType allowedCurves[] = {
    CURVE_LINEAR,
    CURVE_EXPONENTIAL,
    CURVE_LOG,
    CURVE_MAX_VELOCITY
};

// === MainMenu ===

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
    const char *menuItems[MENU_ITEMS_COUNT] = {"Inputs", "HiHat Pedal", "XTALK", "Reset Defaults"};
    uint8_t displayOffset = 0;
    if (mainMenuSelection_ >= 1 && MENU_ITEMS_COUNT > 2)
        displayOffset = mainMenuSelection_ - 1;
    if (displayOffset >= MENU_ITEMS_COUNT - 1 && MENU_ITEMS_COUNT > 1)
        displayOffset = MENU_ITEMS_COUNT - 2;
    if (MENU_ITEMS_COUNT == 1)
        displayOffset = 0;

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

bool MainMenu::isActive() const {
    return uiState_ == UI_MAIN;
}

void MainMenu::handleAction(UIAction action) {
    switch (action) {
    case ACT_MOVE_ACTIVE_ITEM_PREV: onActiveItemPrev(); break;
    case ACT_MOVE_ACTIVE_ITEM_NEXT: onActiveItemNext(); break;
    case ACT_SAVE: onSave(); break;
    case ACT_BACK: break;
    default: break;
    }
}

// === EditPadMenu ===

EditPadMenu::EditPadMenu(Settings &ds, uint8_t &epi, uint8_t &mpi, bool &ep,
                         UIState &ui, bool &lcd, LiquidCrystal_I2C &l)
    : MenuBase(ui, lcd), deviceSettings_(ds), editPadIndex_(epi),
      menuParamIndex_(mpi), editingParam_(ep), lcd_(l) {}

const PadParamInfo* EditPadMenu::getAvailableParams(uint8_t &count) const {
    static PadParamInfo avail[NUM_TOTAL_PAD_PARAMS];
    count = 0;
    auto &ps = deviceSettings_.pads[editPadIndex_];
    for (auto &p : padParams) { // padParams теперь виден
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
        
    }
}
void EditPadMenu::onParamDecrease() {
    if (editingParam_) {
        changeParameter(false);
    }
}
void EditPadMenu::onSave() {
    if (editingParam_) { // Если сейчас редактируем параметр
        editingParam_ = false; // Выйти из режима редактирования
        saveSettings(deviceSettings_); // Сохранить настройки
    } else { // Если не редактируем, а выбрали пункт меню (Select)
        editingParam_ = true; // Войти в режим редактирования (параметр уже выбран)
    }
}
void EditPadMenu::onBack() {
    if (editingParam_) { // Если сейчас редактируем параметр
        editingParam_ = false; // Выйти из режима редактирования (отмена)
    } else { // Если просто в меню пэдов
        uiState_ = UI_MAIN; // Выйти в главное меню
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
        if (ps.type != PAD_HIHAT && ps.type != PAD_CYMBAL) {
            ps.choke = false;
        }
        break;
    }
    case PARAM_HEAD_MIDI:
        ps.midiHeadNote = increment ? (ps.midiHeadNote + 1) % 128 : (ps.midiHeadNote == 0 ? 127 : ps.midiHeadNote - 1);
        break;
    case PARAM_RIM_MIDI:
        ps.midiRimNote = increment ? (ps.midiRimNote + 1) % 128 : (ps.midiRimNote == 0 ? 127 : ps.midiRimNote - 1);
        break;
    case PARAM_AHEAD_MIDI:
        ps.altHeadNote = increment ? (ps.altHeadNote + 1) % 128 : (ps.altHeadNote == 0 ? 127 : ps.altHeadNote - 1);
        break;
    case PARAM_ARIM_MIDI:
        ps.altRimNote = increment ? (ps.altRimNote + 1) % 128 : (ps.altRimNote == 0 ? 127 : ps.altRimNote - 1);
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
    case PARAM_CHOKE:
        ps.choke = muxJackMap[editPadIndex_][1] != -1 ? !ps.choke : false;
        break;
    }
}

void EditPadMenu::render() {
    displayPadEditMenu(deviceSettings_, editPadIndex_, menuParamIndex_, editingParam_, lcdNeedsUpdate_);
}

void EditPadMenu::handleAction(UIAction action) {
    switch (action) {
        case ACT_MOVE_ACTIVE_ITEM_PREV: onActiveItemPrev(); break;
        case ACT_MOVE_ACTIVE_ITEM_NEXT: onActiveItemNext(); break;
        case ACT_MOVE_PARAM_PREV: onParamPrev(); break;
        case ACT_MOVE_PARAM_NEXT: onParamNext(); break;
        case ACT_EDIT_PARAM_INCREASE: onParamIncrease(); break;
        case ACT_EDIT_PARAM_DECREASE: onParamDecrease(); break;
        case ACT_SAVE: onSave(); break;
        case ACT_BACK: onBack(); break;
        default: break;
    }
    lcdNeedsUpdate_ = true;
}

// === EditHiHatMenu ===

EditHiHatMenu::EditHiHatMenu(Settings &deviceSettings, bool &editingHiHatParam, uint8_t &hiHatMenuIndex,
                             UIState &ui, bool &lcdNeedsUpdate, LiquidCrystal_I2C &lcd)
    : MenuBase(ui, lcdNeedsUpdate),
      deviceSettings_(deviceSettings),
      editingHiHatParam_(editingHiHatParam),
      hiHatMenuIndex_(hiHatMenuIndex),
      lcd_(lcd) {}

EditHiHatMenu::~EditHiHatMenu() = default;

void EditHiHatMenu::onActiveItemPrev() { // Переключение параметра
    if (hiHatMenuIndex_ == 0)
        hiHatMenuIndex_ = HIHAT_NUM_PARAMS - 1;
    else
        hiHatMenuIndex_--;
    lcdNeedsUpdate_ = true;
}
void EditHiHatMenu::onActiveItemNext() { // Переключение параметра
    hiHatMenuIndex_ = (hiHatMenuIndex_ + 1) % HIHAT_NUM_PARAMS;
    lcdNeedsUpdate_ = true;
}
void EditHiHatMenu::onParamIncrease() {
    if (editingHiHatParam_) {
        switch(hiHatMenuIndex_) {
            case HIHAT_CC_CLOSED: deviceSettings_.hihat.ccClosed = (deviceSettings_.hihat.ccClosed + 1) % 128; break;
            case HIHAT_CC_OPEN: deviceSettings_.hihat.ccOpen = (deviceSettings_.hihat.ccOpen + 1) % 128; break;
            case HIHAT_CC_STEP: deviceSettings_.hihat.ccStep = (deviceSettings_.hihat.ccStep + 1) % 128; break;
            case HIHAT_INVERT: deviceSettings_.hihat.invert = !deviceSettings_.hihat.invert; break;
            case HIHAT_HIT_NOTE: deviceSettings_.hihat.hitNote = (deviceSettings_.hihat.hitNote + 1) % 128; break; // Ноты 0-127
            case HIHAT_HIT_VELOCITY: deviceSettings_.hihat.hitVelocity = (deviceSettings_.hihat.hitVelocity + 1) % 128; break; // Скорость 0-127
            case HIHAT_HIT_THRESHOLD_RAW: deviceSettings_.hihat.hitThresholdRaw = (deviceSettings_.hihat.hitThresholdRaw + 10 > 1023) ? 0 : deviceSettings_.hihat.hitThresholdRaw + 10; break; // Порог 0-1023, шаг 10
            case HIHAT_DEBOUNCE_TIME_MS: deviceSettings_.hihat.debounceTimeMs = (deviceSettings_.hihat.debounceTimeMs + 5 > 500) ? 0 : deviceSettings_.hihat.debounceTimeMs + 5; break; // Время 0-500мс, шаг 5
        }
        lcdNeedsUpdate_ = true;
    }
}
void EditHiHatMenu::onParamDecrease() {
    if (editingHiHatParam_) {
        switch(hiHatMenuIndex_) {
            case HIHAT_CC_CLOSED: deviceSettings_.hihat.ccClosed = (deviceSettings_.hihat.ccClosed == 0) ? 127 : deviceSettings_.hihat.ccClosed - 1; break;
            case HIHAT_CC_OPEN: deviceSettings_.hihat.ccOpen = (deviceSettings_.hihat.ccOpen == 0) ? 127 : deviceSettings_.hihat.ccOpen - 1; break;
            case HIHAT_CC_STEP: deviceSettings_.hihat.ccStep = (deviceSettings_.hihat.ccStep == 0) ? 127 : deviceSettings_.hihat.ccStep - 1; break;
            case HIHAT_INVERT: deviceSettings_.hihat.invert = !deviceSettings_.hihat.invert; break;
            // Новые параметры
            case HIHAT_HIT_NOTE: deviceSettings_.hihat.hitNote = (deviceSettings_.hihat.hitNote == 0) ? 127 : deviceSettings_.hihat.hitNote - 1; break;
            case HIHAT_HIT_VELOCITY: deviceSettings_.hihat.hitVelocity = (deviceSettings_.hihat.hitVelocity == 0) ? 127 : deviceSettings_.hihat.hitVelocity - 1; break;
            case HIHAT_HIT_THRESHOLD_RAW: deviceSettings_.hihat.hitThresholdRaw = (deviceSettings_.hihat.hitThresholdRaw < 10) ? 1023 : deviceSettings_.hihat.hitThresholdRaw - 10; break;
            case HIHAT_DEBOUNCE_TIME_MS: deviceSettings_.hihat.debounceTimeMs = (deviceSettings_.hihat.debounceTimeMs < 5) ? 500 : deviceSettings_.hihat.debounceTimeMs - 5; break;
        }
        lcdNeedsUpdate_ = true;
    }
}


void EditHiHatMenu::onSave() { // Вход в режим редактирования параметра или сохранение
    if (editingHiHatParam_) { // Если уже редактируем
        editingHiHatParam_ = false; // Выйти из режима редактирования
        saveSettings(deviceSettings_); // Сохранить настройки
    } else { // Если просто в меню выбора параметра
        editingHiHatParam_ = true; // Войти в режим редактирования
    }
    lcdNeedsUpdate_ = true;
}
void EditHiHatMenu::onBack() { // Выход из режима редактирования или выход в главное меню
    if (editingHiHatParam_) { // Если уже редактируем
        editingHiHatParam_ = false; // Отменить редактирование
    } else { // Если просто в меню выбора параметра
        uiState_ = UI_MAIN; // Выйти в главное меню
    }
    lcdNeedsUpdate_ = true;
}

void EditHiHatMenu::render() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("HiHat Pedal"); // Изменил на более короткое название
    if (editingHiHatParam_) {
        lcd.setCursor(15, 0);
        lcd.print("*");
    } 
    char buf[17];
    const char *paramName = hiHatParamNames[hiHatMenuIndex_];
    char valStr[10]; // Буфер для строкового представления значения

    // Получаем значение параметра
    switch (hiHatMenuIndex_) {
        case HIHAT_CC_CLOSED: snprintf(valStr, sizeof(valStr), "%d", deviceSettings_.hihat.ccClosed); break;
        case HIHAT_CC_OPEN: snprintf(valStr, sizeof(valStr), "%d", deviceSettings_.hihat.ccOpen); break;
        case HIHAT_CC_STEP: snprintf(valStr, sizeof(valStr), "%d", deviceSettings_.hihat.ccStep); break;
        case HIHAT_INVERT: snprintf(valStr, sizeof(valStr), "%s", deviceSettings_.hihat.invert ? "Yes" : "No"); break;
        // Новые параметры
        case HIHAT_HIT_NOTE: snprintf(valStr, sizeof(valStr), "%d", deviceSettings_.hihat.hitNote); break;
        case HIHAT_HIT_VELOCITY: snprintf(valStr, sizeof(valStr), "%d", deviceSettings_.hihat.hitVelocity); break;
        case HIHAT_HIT_THRESHOLD_RAW: snprintf(valStr, sizeof(valStr), "%d", deviceSettings_.hihat.hitThresholdRaw); break;
        case HIHAT_DEBOUNCE_TIME_MS: snprintf(valStr, sizeof(valStr), "%d", deviceSettings_.hihat.debounceTimeMs); break;
        default: snprintf(valStr, sizeof(valStr), "N/A"); break;
    }

    snprintf(buf, sizeof(buf), "%-10s: %4s", paramName, valStr);
    lcd.setCursor(0, 1);
    lcd.print(buf);

    lcdNeedsUpdate_ = false;
}

void EditHiHatMenu::handleAction(UIAction action) {
    switch (action) {
    case ACT_MOVE_ACTIVE_ITEM_PREV: onActiveItemPrev(); break;
    case ACT_MOVE_ACTIVE_ITEM_NEXT: onActiveItemNext(); break;
    case ACT_EDIT_PARAM_INCREASE: onParamIncrease(); break;
    case ACT_EDIT_PARAM_DECREASE: onParamDecrease(); break;
    case ACT_SAVE: onSave(); break;
    case ACT_BACK: onBack(); uiState_ = UI_MAIN; break;
    default: break;
    }
    lcdNeedsUpdate_ = true;
}

// === EditXtalkMenu ===

EditXtalkMenu::EditXtalkMenu(Settings &deviceSettings, bool &editingXtalk, uint8_t &xtalkPadIndex,
                             uint8_t &xtalkMenuParamIndex, UIState &ui, bool &lcdNeedsUpdate, LiquidCrystal_I2C &lcd)
    : MenuBase(ui, lcdNeedsUpdate),
      deviceSettings_(deviceSettings),
      editingXtalk_(editingXtalk),
      xtalkPadIndex_(xtalkPadIndex),
      xtalkMenuParamIndex_(xtalkMenuParamIndex),
      lcd_(lcd) {}

EditXtalkMenu::~EditXtalkMenu() = default;

void EditXtalkMenu::onActiveItemPrev() { // Переключение пэда
    if (!editingXtalk_) {
        xtalkPadIndex_ = (xtalkPadIndex_ == 0) ? NUM_JACKS - 1 : xtalkPadIndex_ - 1;
        lcdNeedsUpdate_ = true; 
    }
}
void EditXtalkMenu::onActiveItemNext() { // Переключение пэда
    if (!editingXtalk_) {
        xtalkPadIndex_ = (xtalkPadIndex_ + 1) % NUM_JACKS;
        lcdNeedsUpdate_ = true; 
    }
}
void EditXtalkMenu::onParamIncrease() { // Увеличение значения параметра
    if (editingXtalk_) {
        PadSettings &ps = deviceSettings_.pads[xtalkPadIndex_];
        switch(xtalkMenuParamIndex_) {
            case 0: ps.xtalkThreshold = (ps.xtalkThreshold == 127) ? 0 : ps.xtalkThreshold + 1; break;
            case 1: ps.xtalkCancelTime = (ps.xtalkCancelTime >= 500) ? 0 : ps.xtalkCancelTime + 10; break;
        }
        
    }
}
void EditXtalkMenu::onParamDecrease() { // Уменьшение значения параметра
    if (editingXtalk_) {
        PadSettings &ps = deviceSettings_.pads[xtalkPadIndex_];
        switch(xtalkMenuParamIndex_) {
            case 0: ps.xtalkThreshold = (ps.xtalkThreshold == 0) ? 127 : ps.xtalkThreshold - 1; break;
            case 1: ps.xtalkCancelTime = (ps.xtalkCancelTime == 0) ? 500 : ps.xtalkCancelTime - 10; break;
        }
        
    }
}
void EditXtalkMenu::onSave() { // Вход в режим редактирования параметра или сохранение
    if (editingXtalk_) { // Если уже редактируем
        editingXtalk_ = false; // Выйти из режима редактирования
        saveSettings(deviceSettings_); // Сохранить настройки
    } else { // Если просто в меню выбора параметра
        editingXtalk_ = true; // Войти в режим редактирования
    }
    
}
void EditXtalkMenu::onBack() { // Выход из режима редактирования или выход в главное меню
    if (editingXtalk_) { // Если уже редактируем
        editingXtalk_ = false; // Отменить редактирование
    } else { // Если просто в меню выбора параметра
        uiState_ = UI_MAIN; // Выйти в главное меню
    }
    
}
void EditXtalkMenu::onParamNext() { // Переключение параметра XTALK (Threshold/CancelTime)
    xtalkMenuParamIndex_ = (xtalkMenuParamIndex_ + 1) % 2; // Только 2 параметра
    
}
void EditXtalkMenu::onParamPrev() { // Переключение параметра XTALK
    xtalkMenuParamIndex_ = (xtalkMenuParamIndex_ == 0) ? 1 : xtalkMenuParamIndex_ - 1;
    
}

void EditXtalkMenu::render() {
    lcd.clear();
    
    char line1[17];
    snprintf(line1, sizeof(line1), "Pad %d XTALK", xtalkPadIndex_ + 1);
    lcd.setCursor(0, 0);
    lcd.print(line1);
    
    if (editingXtalk_) {
        lcd.setCursor(15, 0);
        lcd.print("*");
    }
    
    char buf[17];
    PadSettings &ps = deviceSettings_.pads[xtalkPadIndex_];
    const char *paramName = nullptr;
    int val = 0;

    switch (xtalkMenuParamIndex_) {
    case 0:
        paramName = "Threshold";
        val = ps.xtalkThreshold;
        break;
    case 1:
        paramName = "CancelTime";
        val = ps.xtalkCancelTime;
        break;
    default:
        paramName = "Unknown";
        val = 0;
        break;
    }

    snprintf(buf, sizeof(buf), "%-10s: %4d", paramName, val);
    
    lcd.setCursor(0, 1);
    lcd.print(buf);

    lcdNeedsUpdate_ = false;
}

void EditXtalkMenu::handleAction(UIAction action) {
    switch (action) {
        case ACT_MOVE_ACTIVE_ITEM_PREV: onActiveItemPrev(); break;
        case ACT_MOVE_ACTIVE_ITEM_NEXT: onActiveItemNext(); break;
        case ACT_MOVE_PARAM_PREV: onParamPrev(); break; // Добавлено для кнопки "влево" вне режима редактирования
        case ACT_MOVE_PARAM_NEXT: onParamNext(); break; // Добавлено для кнопки "вправо" вне режима редактирования
        case ACT_EDIT_PARAM_INCREASE: onParamIncrease(); break;
        case ACT_EDIT_PARAM_DECREASE: onParamDecrease(); break;
        case ACT_SAVE: onSave(); break;
        case ACT_BACK: onBack(); uiState_ = UI_MAIN; break;
        default: break;
    }
    lcdNeedsUpdate_ = true;
}

// === ConfirmResetMenu ===

ConfirmResetMenu::ConfirmResetMenu(UIState &ui, bool &lcdNeedsUpdate, bool &resetYesSelected,
                                   LiquidCrystal_I2C &lcd, Settings &deviceSettings)
    : MenuBase(ui, lcdNeedsUpdate),
      resetYesSelected_(resetYesSelected),
      lcd_(lcd),
      deviceSettings_(deviceSettings) {}

ConfirmResetMenu::~ConfirmResetMenu() = default;

void ConfirmResetMenu::onParamIncrease() { // Используем для переключения выбора YES/NO
    resetYesSelected_ = !resetYesSelected_;
    
}
void ConfirmResetMenu::onParamDecrease() { // Используем для переключения выбора YES/NO
    resetYesSelected_ = !resetYesSelected_;
    
}
void ConfirmResetMenu::onSave() {
    if (resetYesSelected_) { // Если выбрано YES
        initSettings(deviceSettings_); // Сбросить настройки на дефолтные
        saveSettings(deviceSettings_); // Сохранить
    }
    uiState_ = UI_MAIN; // Всегда возвращаемся в главное меню после выбора
    
}
void ConfirmResetMenu::onBack() {
    uiState_ = UI_MAIN; // Отмена сброса, возвращаемся в главное меню
    
}

void ConfirmResetMenu::render() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Reset Defaults?");
    lcd.setCursor(0, 1);
    lcd.print(resetYesSelected_ ? "> YES        NO" : "  YES      >NO");
    lcdNeedsUpdate_ = false;
}

void ConfirmResetMenu::handleAction(UIAction action) {
    switch (action) {
    case ACT_MOVE_ACTIVE_ITEM_PREV: onParamIncrease(); break; // Используем для переключения (Up)
    case ACT_MOVE_ACTIVE_ITEM_NEXT: onParamDecrease(); break; // Используем для переключения (Down)
    case ACT_SAVE: onSave(); break;
    case ACT_BACK: onBack(); uiState_ = UI_MAIN; break;
    default: break;
    }
    lcdNeedsUpdate_ = true;
}

// --- utils ---

void lcdPrintCentered(const char *str) {
    int pad = (16 - strlen(str)) / 2;
    if (pad < 0)
        pad = 0;
    lcd.setCursor(pad, 0);
    lcd.print(str);
}

void displayPadEditMenu(const Settings &deviceSettings, uint8_t padIdx, uint8_t menuParamIdx, bool editingParam,
                       bool &lcdNeedsUpdateRef) {
    lcd.clear();
    char line[17];

    const PadSettings &ps = deviceSettings.pads[padIdx];

    // Первая строка: номер пэда
    snprintf(line, sizeof(line), "Input %d", padIdx + 1);
    lcd.setCursor(0, 0);
    lcd.print(line);

    const char *paramName = "";
    char valueStr[10] = "";  // для текстового или числового значения
    int valueNum = 0;
    uint8_t count = 0;

    // Получаем доступные параметры
    EditPadMenu tempEditPadMenu(const_cast<Settings&>(deviceSettings), editPadIndex, menuParamIndex, editingParam, uiState, lcdNeedsUpdateRef, lcd);
    const PadParamInfo* availableParams = tempEditPadMenu.getAvailableParams(count);

    if (count > 0 && menuParamIdx < count) {
        const PadParamInfo &paramInfo = availableParams[menuParamIdx];
        paramName = paramInfo.name;

        if (paramInfo.paramIndex == PARAM_TYPE) {
            // Отобразить текстовое имя типа пэда
            if ((uint8_t)ps.type < sizeof(padTypeNames)/sizeof(padTypeNames[0])) {
                snprintf(valueStr, sizeof(valueStr), "%s", padTypeNames[(uint8_t)ps.type]);
            } else {
                snprintf(valueStr, sizeof(valueStr), "Unknown");
            }
        }
        else if (paramInfo.paramIndex == PARAM_CURVE) {
            // Отобразить текстовое имя кривой
            // Поиск индекса кривой
            int crvIdx = -1;
            for (uint8_t i = 0; i < sizeof(padCurveNames)/sizeof(padCurveNames[0]); i++) {
                if (ps.curve == allowedCurves[i]) {
                    crvIdx = i;
                    break;
                }
            }
            if (crvIdx >= 0 && crvIdx < (int)(sizeof(padCurveNames)/sizeof(padCurveNames[0]))) {
                snprintf(valueStr, sizeof(valueStr), "%s", padCurveNames[crvIdx]);
            } else {
                snprintf(valueStr, sizeof(valueStr), "Unknown");
            }
        } else {
            // Числовое значение параметра
            switch (paramInfo.paramIndex) {
                case PARAM_HEAD_MIDI: valueNum = ps.midiHeadNote; break;
                case PARAM_AHEAD_MIDI: valueNum = ps.altHeadNote; break;
                case PARAM_RIM_MIDI: valueNum = ps.midiRimNote; break;
                case PARAM_ARIM_MIDI: valueNum = ps.altRimNote; break;
                case PARAM_SENSITIVITY: valueNum = ps.sensitivity; break;
                case PARAM_THRESHOLD: valueNum = ps.threshold; break;
                case PARAM_RIM_MUTE: valueNum = ps.rimMute ? 1 : 0; break;
                case PARAM_CHOKE: valueNum = ps.choke ? 1 : 0; break;
                case PARAM_SCANTIME: valueNum = ps.scantime; break;
                case PARAM_MASKTIME: valueNum = ps.masktime; break;
                default: valueNum = 0; break;
            }
            snprintf(valueStr, sizeof(valueStr), "%d", valueNum);
        }
    } else {
        snprintf(valueStr, sizeof(valueStr), "--");
    }
    if (editingParam) {
        lcd.setCursor(15, 0);
        lcd.print("*");
    }
    // Формируем и выводим вторую строку
    snprintf(line, sizeof(line), "%-9s: %5s", paramName, valueStr);
    lcd.setCursor(0, 1);
    lcd.print(line);

    lcdNeedsUpdateRef = false;
}

