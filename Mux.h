// файл Mux.h
#ifndef MUX_H
#define MUX_H

#include "Config.h"

// Прототипы

void muxSelect(uint8_t channel);
int muxRead(uint8_t channel);

// Тайловая структура маппинга джека на мультиплексор
extern const int8_t muxJackMap[NUM_JACKS][2]; // leftChannel, rightChannel

#endif