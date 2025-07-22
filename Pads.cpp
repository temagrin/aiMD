#include "Pads.h"
#include "Mux.h"
#include "MIDISender.h"
#include <Arduino.h>

// Внешний маппинг для доступности правого канала
extern const int8_t muxJackMap[NUM_JACKS][2];

// Пример применения кривой — пример функции (вынесите/расширьте по потребности)
static uint8_t applyCurve(CurveType curve, uint16_t input, uint8_t sensitivity) {
    float norm = constrain(input / 1023.0f, 0.0f, 1.0f);
    norm *= (sensitivity / 127.0f);
    float outV = 0;
    switch (curve) {
    case CURVE_LINEAR:
        outV = norm;
        break;
    case CURVE_EXPONENTIAL:
        outV = pow(norm, 2.0f);
        break;
    case CURVE_LOG:
        outV = (norm > 0) ? (log10(norm * 9.0f + 1) / log10(10)) : 0;
        break;
    case CURVE_CUSTOM:
        outV = norm;
        break;
    }
    uint8_t midiVal = (uint8_t)(constrain(outV * 127.0f, 1, 127));
    return midiVal;
}

void scanPad(const PadSettings &ps, PadStatus &pst, uint8_t jackIndex) {
    if (ps.type == PAD_DISABLED) return;

    unsigned long now = millis();

    int leftRaw = (muxJackMap[jackIndex][0] != -1) ? muxRead(muxJackMap[jackIndex][0]) : 0;
    int rightRaw = (muxJackMap[jackIndex][1] != -1) ? muxRead(muxJackMap[jackIndex][1]) : 0;

    // Обработка моношки
    if (ps.type == PAD_SINGLE) {
        if (leftRaw > ps.threshold && !pst.headPressed && (now - pst.lastHitTimeHead > ps.masktime)) {
            uint8_t velocity = applyCurve(ps.curve, leftRaw, ps.sensitivity);
            midiSendNoteOn(9, ps.midiHeadNote, velocity);
            pst.headPressed = true;
            pst.lastHitTimeHead = now;
        } else if (leftRaw <= ps.threshold && pst.headPressed) {
            midiSendNoteOff(9, ps.midiHeadNote, 0);
            pst.headPressed = false;
        }
    }
    // Обработка Dual
    else if (ps.type == PAD_DUAL) {
        if (leftRaw > ps.threshold && !pst.headPressed && (now - pst.lastHitTimeHead > ps.masktime)) {
            uint8_t velocity = applyCurve(ps.curve, leftRaw, ps.sensitivity);
            midiSendNoteOn(9, ps.midiHeadNote, velocity);
            pst.headPressed = true;
            pst.lastHitTimeHead = now;
            if (ps.rimMute && pst.rimPressed) {
                midiSendNoteOff(9, ps.midiRimNote, 0);
                pst.rimPressed = false;
            }
        } else if (leftRaw <= ps.threshold && pst.headPressed) {
            midiSendNoteOff(9, ps.midiHeadNote, 0);
            pst.headPressed = false;
        }

        if (rightRaw > ps.threshold && !pst.rimPressed && (now - pst.lastHitTimeRim > ps.masktime)) {
            uint8_t velocity = applyCurve(ps.curve, rightRaw, ps.sensitivity);
            if (!(ps.rimMute && pst.headPressed)) {
                midiSendNoteOn(9, ps.midiRimNote, velocity);
                pst.rimPressed = true;
                pst.lastHitTimeRim = now;
            }
        } else if (rightRaw <= ps.threshold && pst.rimPressed) {
            midiSendNoteOff(9, ps.midiRimNote, 0);
            pst.rimPressed = false;
        }
    }
    // Аналогично для PAD_HIHAT и PAD_CYMBAL - добавьте при необходимости
}