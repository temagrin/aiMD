// файл Config.cpp
#include "Config.h" // Обязательно включите, чтобы extern совпал с определением

// Определения массивов
const char* hiHatParamNames[HIHAT_NUM_PARAMS] = {
  "CC Closed", "CC Open", "CC Step", "Invert"
};

const char* allParamNames[NUM_TOTAL_PAD_PARAMS] = {
  "Type", "HeadMidi", "Sens", "Thresh", "Curve", "RimMidi", "RimMute", "TwoZone", "MutePiezo"
};

const PadType allPadTypes[] = { PAD_DISABLED, PAD_SINGLE, PAD_DUAL, PAD_HIHAT, PAD_CYMBAL };
const uint8_t allPadTypesCount = sizeof(allPadTypes) / sizeof(allPadTypes[0]);