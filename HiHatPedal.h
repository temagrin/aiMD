// файл HiHatPedal.h

#ifndef HIHATPEDAL_H
#define HIHATPEDAL_H

#include "Settings.h"
#include <Arduino.h>

uint8_t readHiHatValueRaw();
void processHiHatPedal(const HiHatSettings &hihatSettings);

#endif