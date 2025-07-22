#include "Pads.h"
#include "Mux.h"
#include "MIDISender.h"
#include <Arduino.h>

extern const int8_t muxJackMap[NUM_JACKS][2];
extern PadStatus padStatus[NUM_JACKS]; // глобальный статус всех пэдов
extern Settings deviceSettings;         // глобальные настройки с XTALK параметрами

static uint8_t applyCurve(CurveType curve, uint16_t input, uint8_t sensitivity) {
    float norm = constrain(input / 1023.0f, 0.0f, 1.0f);
    norm *= (sensitivity / 127.0f);
    float outV = 0;
    switch (curve) {
    case CURVE_LINEAR:      outV = norm; break;
    case CURVE_EXPONENTIAL: outV = pow(norm, 2.0f); break;
    case CURVE_LOG:         outV = (norm > 0) ? (log10(norm * 9.0f + 1) / log10(10)) : 0; break;
    case CURVE_CUSTOM:      outV = norm; break;
    }
    return (uint8_t)(constrain(outV * 127.0f, 1, 127));
}


void scanPad(const PadSettings &ps, PadStatus &pst, uint8_t jackIndex) {
    if (ps.type == PAD_DISABLED) return;

    unsigned long now = millis();

    int leftRaw = (muxJackMap[jackIndex][0] != -1) ? muxRead(muxJackMap[jackIndex][0]) : 0;
    int rightRaw = (muxJackMap[jackIndex][1] != -1) ? muxRead(muxJackMap[jackIndex][1]) : 0;

    // Функция проверки XTALK для конкретного сигнала и позиции (head или rim)
    auto checkXTALK = [&](int rawValue, uint8_t currentPadIndex) -> bool {
        for (uint8_t i = 0; i < NUM_JACKS; ++i) {
            if (i == currentPadIndex) continue;
            PadSettings &otherPad = deviceSettings.pads[i];
            PadStatus &otherStatus = padStatus[i];

            if (otherStatus.activeTrigger && (now - otherStatus.lastTriggerTime < otherPad.xtalkCancelTime)) {
                // Если сигнал меньше XTALK порога пэда i, то ванильно подавляем
                if (rawValue < otherPad.xtalkThreshold) {
                    return true; // XTALK detected — подавляем
                }
            }
        }
        return false;
    };

    // --- Обработка головы ---
    if (ps.type == PAD_SINGLE || ps.type == PAD_DUAL || ps.type == PAD_HIHAT || ps.type == PAD_CYMBAL) {
        if (leftRaw > ps.threshold && !pst.headPressed && (now - pst.lastHitTimeHead > ps.masktime)) {
            // Проверяем XTALK
            if (!checkXTALK(leftRaw, jackIndex)) {
                uint8_t velocity = applyCurve(ps.curve, leftRaw, ps.sensitivity);
                midiSendNoteOn(9, ps.midiHeadNote, velocity);
                pst.headPressed = true;
                pst.lastHitTimeHead = now;
                pst.lastTriggerTime = now;
                pst.activeTrigger = true;
            }
        } else if (leftRaw <= ps.threshold && pst.headPressed) {
            midiSendNoteOff(9, ps.midiHeadNote, 0);
            pst.headPressed = false;
            // Сбрасывать activeTrigger позже — после xtalkCancelTime
        }
    }

    // --- Обработка рима для Dual ---
    if (ps.type == PAD_DUAL || ps.type == PAD_CYMBAL) {
        if (rightRaw > ps.threshold && !pst.rimPressed && (now - pst.lastHitTimeRim > ps.masktime)) {
            // Проверяем XTALK для рима
            if (!(ps.rimMute && pst.headPressed) && !checkXTALK(rightRaw, jackIndex)) {
                uint8_t velocity = applyCurve(ps.curve, rightRaw, ps.sensitivity);
                midiSendNoteOn(9, ps.midiRimNote, velocity);
                pst.rimPressed = true;
                pst.lastHitTimeRim = now;
                pst.lastTriggerTime = now;
                pst.activeTrigger = true;
            }
        } else if (rightRaw <= ps.threshold && pst.rimPressed) {
            midiSendNoteOff(9, ps.midiRimNote, 0);
            pst.rimPressed = false;
        }
    }

    // --- Сброс activeTrigger, если время вышло ---
    if (pst.activeTrigger && (now - pst.lastTriggerTime > ps.xtalkCancelTime)) {
        pst.activeTrigger = false;
    }
}
