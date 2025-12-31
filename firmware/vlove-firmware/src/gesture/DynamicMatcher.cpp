#include "DynamicMatcher.h"
#include <Arduino.h>
#include <string.h>

DynamicMatcher::DynamicMatcher()
    : gestureCount(0)
    , lastRecognized(GESTURE_NONE)
    , debounceTimeMs(0) {
    for (uint8_t i = 0; i < MAX_DYNAMIC_GESTURES; i++) {
        gestures[i].active = false;
        resetTracker(i);
    }
}

void DynamicMatcher::resetTracker(uint8_t index) {
    trackers[index].currentPhase = 0xFF;
    trackers[index].phaseTimeMs = 0;
    trackers[index].totalTimeMs = 0;
    trackers[index].active = false;
}

void DynamicMatcher::reset() {
    for (uint8_t i = 0; i < MAX_DYNAMIC_GESTURES; i++) {
        resetTracker(i);
    }
    lastRecognized = GESTURE_NONE;
    debounceTimeMs = 0;
}

int DynamicMatcher::findGestureIndex(GestureId id) const {
    for (uint8_t i = 0; i < MAX_DYNAMIC_GESTURES; i++) {
        if (gestures[i].active && gestures[i].header.id == id) {
            return i;
        }
    }
    return -1;
}

bool DynamicMatcher::registerGesture(const DynamicGestureDef& header, const DynamicPhase* phases) {
    // Check if already registered
    int existing = findGestureIndex(header.id);
    if (existing >= 0) {
        // Update existing
        gestures[existing].header = header;
        memcpy(gestures[existing].phases, phases,
               header.numPhases * sizeof(DynamicPhase));
        resetTracker(existing);
        return true;
    }

    // Find empty slot
    for (uint8_t i = 0; i < MAX_DYNAMIC_GESTURES; i++) {
        if (!gestures[i].active) {
            gestures[i].header = header;
            memcpy(gestures[i].phases, phases,
                   header.numPhases * sizeof(DynamicPhase));
            gestures[i].active = true;
            resetTracker(i);
            gestureCount++;
            return true;
        }
    }

    return false; // No empty slot
}

bool DynamicMatcher::unregisterGesture(GestureId id) {
    int index = findGestureIndex(id);
    if (index >= 0) {
        gestures[index].active = false;
        resetTracker(index);
        gestureCount--;
        return true;
    }
    return false;
}

void DynamicMatcher::clearGestures() {
    for (uint8_t i = 0; i < MAX_DYNAMIC_GESTURES; i++) {
        gestures[i].active = false;
        resetTracker(i);
    }
    gestureCount = 0;
}

bool DynamicMatcher::matchesPhase(const int* fingerPos, const DynamicPhase& phase) {
    for (int i = 0; i < NUM_FINGERS; i++) {
        uint8_t normalizedValue = normalizeFingerPos(fingerPos[i]);
        if (!matchesConstraint(normalizedValue, phase.fingers[i])) {
            return false;
        }
    }
    return true;
}

int8_t DynamicMatcher::getCurrentPhase(GestureId id) const {
    int index = findGestureIndex(id);
    if (index >= 0 && trackers[index].active) {
        return trackers[index].currentPhase;
    }
    return -1;
}

GestureId DynamicMatcher::update(const int* fingerPos, uint16_t deltaTimeMs) {
    GestureId completed = GESTURE_NONE;

    // Handle debounce
    if (debounceTimeMs > 0) {
        if (deltaTimeMs >= debounceTimeMs) {
            debounceTimeMs = 0;
        } else {
            debounceTimeMs -= deltaTimeMs;
        }
    }

    // Update each registered gesture
    for (uint8_t i = 0; i < MAX_DYNAMIC_GESTURES; i++) {
        if (!gestures[i].active) continue;

        const DynamicGestureEntry& gesture = gestures[i];
        GestureTracker& tracker = trackers[i];

        // Update timing
        if (tracker.active) {
            tracker.phaseTimeMs += deltaTimeMs;
            tracker.totalTimeMs += deltaTimeMs;

            // Check for timeout
            if (tracker.totalTimeMs > gesture.header.timeoutMs) {
                resetTracker(i);
                continue;
            }
        }

        // Get current phase (or first phase if not started)
        uint8_t phaseIdx = tracker.active ? tracker.currentPhase : 0;
        if (phaseIdx >= gesture.header.numPhases) {
            resetTracker(i);
            continue;
        }

        const DynamicPhase& phase = gesture.phases[phaseIdx];

        // Check if current finger positions match this phase
        bool matches = matchesPhase(fingerPos, phase);

        if (!tracker.active) {
            // Not tracking yet - start if phase 0 matches
            if (matches) {
                tracker.active = true;
                tracker.currentPhase = 0;
                tracker.phaseTimeMs = 0;
                tracker.totalTimeMs = 0;
            }
        } else if (matches) {
            // Still matching current phase
            // Check if we can advance to next phase
            if (tracker.phaseTimeMs >= phase.minDurationMs) {
                // Check if this is the final phase
                if (phase.nextPhase == 0xFF) {
                    // Gesture complete!
                    if (debounceTimeMs == 0 || lastRecognized != gesture.header.id) {
                        completed = gesture.header.id;
                        lastRecognized = gesture.header.id;
                        debounceTimeMs = gesture.header.debounceMs;
                    }
                    resetTracker(i);
                }
                // Otherwise wait for position to change to advance
            }
        } else {
            // Position changed - check if we should advance or reset
            if (tracker.phaseTimeMs >= phase.minDurationMs) {
                // Met minimum time, try to advance
                uint8_t nextIdx = phase.nextPhase;
                if (nextIdx < gesture.header.numPhases) {
                    // Check if next phase matches
                    if (matchesPhase(fingerPos, gesture.phases[nextIdx])) {
                        tracker.currentPhase = nextIdx;
                        tracker.phaseTimeMs = 0;
                    } else if (phase.maxDurationMs > 0 &&
                               tracker.phaseTimeMs > phase.maxDurationMs) {
                        // Exceeded max time and next phase doesn't match
                        resetTracker(i);
                    }
                } else if (nextIdx == 0xFF) {
                    // This was final phase but position changed after min time
                    // Still count as complete
                    if (debounceTimeMs == 0 || lastRecognized != gesture.header.id) {
                        completed = gesture.header.id;
                        lastRecognized = gesture.header.id;
                        debounceTimeMs = gesture.header.debounceMs;
                    }
                    resetTracker(i);
                }
            } else {
                // Didn't meet minimum time, reset
                resetTracker(i);
            }
        }
    }

    return completed;
}
