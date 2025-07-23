#include "Pads.h"
#include "Mux.h"
#include "MIDISender.h"
#include <Arduino.h>
extern bool isHiHatClosed;
extern const int8_t muxJackMap[NUM_JACKS][2];
extern PadStatus padStatus[NUM_JACKS];
extern Settings deviceSettings;

// MIDI Channel (можно вынести в настройки)
const byte midiChannel = 9; // Используем 9, как у вас было в примере

// Функция преобразования нормализованного значения с заданной кривой в velocity (1..127)
static uint8_t applyCurve(CurveType curve, float norm) {
    norm = constrain(norm, 0.0f, 1.0f);

    switch (curve) {
        case CURVE_LINEAR:
            return (uint8_t)(constrain(norm * 127.0f, 1, 127));
        case CURVE_EXPONENTIAL:
            norm = pow(norm, 2.0f);
            return (uint8_t)(constrain(norm * 127.0f, 1, 127));
        case CURVE_LOG:
            norm = (norm > 0) ? (log10(norm * 9.0f + 1) / log10(10)) : 0;
            return (uint8_t)(constrain(norm * 127.0f, 1, 127));
        case CURVE_MAX_VELOCITY:
            return 127;
        default:
            return (uint8_t)(constrain(norm * 127.0f, 1, 127));
    }
}

void scanPad(const PadSettings &ps, PadStatus &pst, uint8_t jackIndex) {
    if (ps.type == PAD_DISABLED) return;

    unsigned long now = millis();

    int leftRaw  = (muxJackMap[jackIndex][0] != -1) ? muxRead(muxJackMap[jackIndex][0]) : 0;
    int rightRaw = (muxJackMap[jackIndex][1] != -1) ? muxRead(muxJackMap[jackIndex][1]) : 0;

    const float minGain = 0.1f;
    const float maxGain = 4.0f;
    float gain = minGain + ((float)ps.sensitivity / 127.0f) * (maxGain - minGain);

    float effectiveThreshold = (float)ps.threshold / gain;

    auto checkXTALK = [&](int rawValue, uint8_t currentPadIndex) -> bool {
        for (uint8_t i = 0; i < NUM_JACKS; ++i) {
            if (i == currentPadIndex) continue;
            PadSettings &otherPad = deviceSettings.pads[i];
            PadStatus &otherStatus = padStatus[i];
            if (otherStatus.activeTrigger && (now - otherStatus.lastTriggerTime < otherPad.xtalkCancelTime)) {
                if (rawValue < otherPad.xtalkThreshold) return true;
            }
        }
        return false;
    };

    bool headTrigger = false, rimTrigger = false;
    uint8_t headVelocity = 0, rimVelocity = 0;

    bool isHiHat = (ps.type == PAD_HIHAT);
    bool isCymbal = (ps.type == PAD_CYMBAL);
    bool isDual = (ps.type == PAD_DUAL);
    bool isDualPadType = (isDual || isCymbal || isHiHat);

    // Для тарелок рим подтянут к +5В, уровень высокой подтяжки примерно 1000 (из 1023)
    const int rimPulledUpMin = 1000;

    byte currentMidiHeadNote = ps.midiHeadNote;
    byte currentMidiRimNote = ps.midiRimNote;

    if (isHiHat && isHiHatClosed) {
        // При нажатой педали HiHat используются альтернативные ноты
        currentMidiHeadNote = ps.altHeadNote;
        currentMidiRimNote = ps.altRimNote;
    }

    if (isDualPadType) {
        // --- Обработка головы ---
        if (leftRaw > effectiveThreshold && (now - pst.lastHitTimeHead > ps.masktime) && !pst.headPressed && !checkXTALK(leftRaw, jackIndex)) {
            float normHead = constrain((float)leftRaw / 1023.0f * gain, 0.0f, 1.0f);
            headVelocity = applyCurve(ps.curve, normHead);
            headTrigger = true;
        }

        // --- Обработка рима ---
        bool rimMutedByHead = (ps.rimMute && pst.headPressed);

        bool rimClosedPhysical = false;
        if (isCymbal || isHiHat) {
            // Для тарелок проверяем подтяжку
            rimClosedPhysical = rightRaw > rimPulledUpMin;
        } else if (isDual) {
            // Для PAD_DUAL подтяжки рима нет, значит римClosedPhysical всегда false
            rimClosedPhysical = false;
        }

        if (rightRaw > effectiveThreshold && (now - pst.lastHitTimeRim > ps.masktime) && !pst.rimPressed && !rimMutedByHead && !rimClosedPhysical && !checkXTALK(rightRaw, jackIndex)) {
            float normRim = constrain((float)rightRaw / 1023.0f * gain, 0.0f, 1.0f);
            rimVelocity = applyCurve(ps.curve, normRim);
            rimTrigger = true;
        }

        // Приоритет и выбор между хедом и римом
        if (headTrigger && rimTrigger) {
            if (ps.rimMute) {
                // При rimMute отправляем только head
                rimTrigger = false;
            } else {
                // Иначе выбираем по максимальному velocity
                if (headVelocity >= rimVelocity) rimTrigger = false;
                else headTrigger = false;
            }
        }

        if (headTrigger) {
            midiSendNoteOn(9, currentMidiHeadNote, headVelocity);
            pst.headPressed = true;
            pst.lastHitTimeHead = now;
            pst.lastTriggerTime = now;
            pst.activeTrigger = true;
        } else if (leftRaw <= effectiveThreshold && pst.headPressed) {
            midiSendNoteOff(9, currentMidiHeadNote, 0);
            pst.headPressed = false;
        }

        if (rimTrigger) {
            midiSendNoteOn(9, currentMidiRimNote, rimVelocity);
            pst.rimPressed = true;
            pst.lastHitTimeRim = now;
            pst.lastTriggerTime = now;
            pst.activeTrigger = true;
        } else if (rightRaw <= effectiveThreshold && pst.rimPressed) {
            midiSendNoteOff(9, currentMidiRimNote, 0);
            pst.rimPressed = false;
        }

        // Обработка choke через polyphonic aftertouch (если включено)
        static bool lastChokeState = false;

        if ((isHiHat || isCymbal) && ps.choke) {
            if (rimClosedPhysical && !lastChokeState) {
                midiSendPolyAftertouch(MIDI_CHANEL, ps.midiHeadNote, 127);
                lastChokeState = true;
            } else if (!rimClosedPhysical && lastChokeState) {
                // При необходимости можно сбросить choke
                midiSendPolyAftertouch(MIDI_CHANEL, ps.midiHeadNote, 0);
                lastChokeState = false;
            }
        } else {
            lastChokeState = false;
        }

    } else {
        // Логика для одиночных пэдов (PAD_SINGLE)
        if (ps.type == PAD_SINGLE) {
            if (leftRaw > effectiveThreshold && (now - pst.lastHitTimeHead > ps.masktime) && !pst.headPressed && !checkXTALK(leftRaw, jackIndex)) {
                float normHead = constrain((float)leftRaw / 1023.0f * gain, 0.0f, 1.0f);
                headVelocity = applyCurve(ps.curve, normHead);
                headTrigger = true;
            }
            if (headTrigger) {
                midiSendNoteOn(MIDI_CHANEL, ps.midiHeadNote, headVelocity);
                pst.headPressed = true;
                pst.lastHitTimeHead = now;
                pst.lastTriggerTime = now;
                pst.activeTrigger = true;
            } else if (leftRaw <= effectiveThreshold && pst.headPressed) {
                midiSendNoteOff(MIDI_CHANEL, ps.midiHeadNote, 0);
                pst.headPressed = false;
            }
        }
    }

    if (pst.activeTrigger && (now - pst.lastTriggerTime > ps.xtalkCancelTime)) {
        pst.activeTrigger = false;
    }
}
