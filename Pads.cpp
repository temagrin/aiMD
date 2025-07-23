// файл Pads.cpp
#include "Pads.h"
#include "Mux.h"
#include "MIDISender.h"
#include <Arduino.h>

extern const int8_t muxJackMap[NUM_JACKS][2];
extern PadStatus padStatus[NUM_JACKS]; 
extern Settings deviceSettings;         

static uint8_t applyCurve(CurveType curve, uint16_t input, uint8_t sensitivity) {
    float norm = constrain(input / 1023.0f, 0.0f, 1.0f);

    switch (curve) {
    case CURVE_LINEAR:      
        norm *= (sensitivity / 127.0f);
        return (uint8_t)(constrain(norm * 127.0f, 1, 127));

    case CURVE_EXPONENTIAL:
        norm = pow(norm, 2.0f) * (sensitivity / 127.0f);
        return (uint8_t)(constrain(norm * 127.0f, 1, 127));

    case CURVE_LOG:
        norm = (norm > 0) ? (log10(norm * 9.0f + 1) / log10(10)) : 0;
        norm *= (sensitivity / 127.0f);
        return (uint8_t)(constrain(norm * 127.0f, 1, 127));


    case CURVE_MAX_VELOCITY:
        return 127;

    default:
        norm *= (sensitivity / 127.0f);
        return (uint8_t)(constrain(norm * 127.0f, 1, 127));
    }
}


void scanPad(const PadSettings &ps, PadStatus &pst, uint8_t jackIndex) {
    if (ps.type == PAD_DISABLED) return;

    unsigned long now = millis();

    int leftRaw = (muxJackMap[jackIndex][0] != -1) ? muxRead(muxJackMap[jackIndex][0]) : 0;
    int rightRaw = (muxJackMap[jackIndex][1] != -1) ? muxRead(muxJackMap[jackIndex][1]) : 0;

    auto checkXTALK = [&](int rawValue, uint8_t currentPadIndex) -> bool {
        for (uint8_t i = 0; i < NUM_JACKS; ++i) {
            if (i == currentPadIndex) continue;
            PadSettings &otherPad = deviceSettings.pads[i];
            PadStatus &otherStatus = padStatus[i];
            if (otherStatus.activeTrigger && (now - otherStatus.lastTriggerTime < otherPad.xtalkCancelTime)) {
                if (rawValue < otherPad.xtalkThreshold) {
                    return true; // подавляем из-за XTALK
                }
            }
        }
        return false;
    };

    bool headTrigger = false, rimTrigger = false;
    uint8_t headVelocity = 0, rimVelocity = 0;

    // Вычисляем велосити головы
    if ((ps.type == PAD_SINGLE || ps.type == PAD_DUAL || ps.type == PAD_HIHAT || ps.type == PAD_CYMBAL)
        && leftRaw > ps.threshold && (now - pst.lastHitTimeHead > ps.masktime) && !pst.headPressed
        && !checkXTALK(leftRaw, jackIndex)) 
    {
        headVelocity = applyCurve(ps.curve, leftRaw, ps.sensitivity);
        headTrigger = true;
    }

    // Вычисляем велосити рима если тип с римом, и rimMute не блокирует
    bool rimBlocked = (ps.type == PAD_DUAL || ps.type == PAD_CYMBAL) && ps.rimMute && pst.headPressed;
    if ((ps.type == PAD_DUAL || ps.type == PAD_CYMBAL)
        && rightRaw > ps.threshold && (now - pst.lastHitTimeRim > ps.masktime) && !pst.rimPressed
        && !rimBlocked && !checkXTALK(rightRaw, jackIndex)) 
    {
        rimVelocity = applyCurve(ps.curve, rightRaw, ps.sensitivity);
        rimTrigger = true;
    }

    // Приоритет — более высокая велосити
    if (headTrigger && rimTrigger) {
        if (headVelocity >= rimVelocity) {
            rimTrigger = false;
        } else {
            headTrigger = false;
        }
    }

    // Отправляем MIDI сигналы согласно приоритету
    if (headTrigger) {
        midiSendNoteOn(9, ps.midiHeadNote, headVelocity);
        pst.headPressed = true;
        pst.lastHitTimeHead = now;
        pst.lastTriggerTime = now;
        pst.activeTrigger = true;
    } else if (leftRaw <= ps.threshold && pst.headPressed) {
        midiSendNoteOff(9, ps.midiHeadNote, 0);
        pst.headPressed = false;
    }

    if (rimTrigger) {
        midiSendNoteOn(9, ps.midiRimNote, rimVelocity);
        pst.rimPressed = true;
        pst.lastHitTimeRim = now;
        pst.lastTriggerTime = now;
        pst.activeTrigger = true;
    } else if (rightRaw <= ps.threshold && pst.rimPressed) {
        midiSendNoteOff(9, ps.midiRimNote, 0);
        pst.rimPressed = false;
    }

    if (pst.activeTrigger && (now - pst.lastTriggerTime > ps.xtalkCancelTime)) {
        pst.activeTrigger = false;
    }
}
