// файл Config.h

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// --- Аппаратные константы ---

#define LCD_ADDR 0x27
#define LCD_COLS 16
#define LCD_ROWS 2

#define MUX_S0 5
#define MUX_S1 4
#define MUX_S2 3
#define MUX_S3 2
#define MUX_SIG A7

#define BUTTONS_PIN A0
#define HIHAT_PEDAL_PIN A6

#define MIDI_TX_PIN 1

#define NUM_JACKS 10
#define NUM_MUX_CHANNELS 16

#define EEPROM_MAGIC 0x42
#define SETTINGS_ADDR 0

#define MIDI_BAUD 31250
#define DEBUG_BAUD 115200

#define BUTTON_DEBOUNCE_MS 50

// --- Enums ---

enum PadType { PAD_DISABLED=0, PAD_SINGLE, PAD_DUAL, PAD_HIHAT, PAD_CYMBAL };
enum CurveType { CURVE_LINEAR=0, CURVE_EXPONENTIAL, CURVE_LOG, CURVE_MAX_VELOCITY};

static const char* padTypeNames[] = {
    "DISABLED",
    "SINGLE",
    "DUAL",
    "HIHAT",
    "CYMBAL"
};

static const char* padCurveNames[] = {
    "LIN",
    "EXP",
    "LOG",
    "MAX"
};


enum MainMenuItems {
    MENU_EDIT_PADS = 0,
    MENU_EDIT_HIHAT,
    MENU_EDIT_XTALK,
    MENU_RESET_DEFAULTS, 
    MENU_ITEMS_COUNT  
};

enum UIState {
    UI_MAIN = 0,
    UI_EDIT_PAD,
    UI_EDIT_HIHAT,
    UI_EDIT_XTALK,
    UI_CONFIRM_RESET
};

enum HiHatParam {
  HIHAT_CC_CLOSED = 0,
  HIHAT_CC_OPEN,
  HIHAT_CC_STEP,
  HIHAT_INVERT,
  HIHAT_HIT_NOTE,
  HIHAT_HIT_VELOCITY,
  HIHAT_HIT_THRESHOLD_RAW,
  HIHAT_DEBOUNCE_TIME_MS,
  HIHAT_NUM_PARAMS
};

enum ParamType {
    PARAM_ENUM = 0,
    PARAM_INT,
    PARAM_BOOL
};

enum PadParam {
  PARAM_TYPE = 0,
  PARAM_HEAD_MIDI,
  PARAM_SENSITIVITY,
  PARAM_THRESHOLD,
  PARAM_CURVE,
  PARAM_RIM_MIDI,
  PARAM_RIM_MUTE,
  PARAM_TWO_ZONE_MODE,
  PARAM_MUTE_BY_PIEZO,
  PARAM_SCANTIME,      
  PARAM_MASKTIME, 
  NUM_TOTAL_PAD_PARAMS
};

// Типы пэдов для выбора (константы)
extern const PadType allPadTypes[];
extern const uint8_t allPadTypesCount;
extern const char* allParamNames[NUM_TOTAL_PAD_PARAMS];

extern const char* hiHatParamNames[HIHAT_NUM_PARAMS];

#endif