// файл Config.cpp
#include "Config.h" // Обязательно включите, чтобы extern совпал с определением

// Определения массивов
static const char* hiHatParamNames[HIHAT_NUM_PARAMS] = {
  "CC Closed",
    "CC Open",
    "CC Step",
    "Invert",
    "Hit Note",
    "Hit Vel",
    "Hit Thresh",
    "Debounce"
};


static const char* allParamNames[NUM_TOTAL_PAD_PARAMS] = {
  "Type", 
  "HdMidi", 
  "Gain", 
  "Thresh", 
  "Curve",
  "ScanTime", 
  "MaskTime",
  "MuteRim",
  "RmMidi",
  "aHdMidi"
  "aRmMidi"
  "Choke"
};

static const PadType allPadTypes[] = { PAD_DISABLED, PAD_SINGLE, PAD_DUAL, PAD_HIHAT, PAD_CYMBAL };
static const uint8_t allPadTypesCount = sizeof(allPadTypes) / sizeof(allPadTypes[0]);