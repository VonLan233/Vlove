#include "StaticMatcher.h"
#include <Arduino.h>
#include <string.h>

StaticMatcher::StaticMatcher()
    : builtinGestures(nullptr)
    , builtinCount(0)
    , customCount(0)
    , lastGesture(GESTURE_NONE)
    , stableCount(0) {
}

void StaticMatcher::begin(const StaticGestureDef* gestures, uint8_t count) {
    builtinGestures = gestures;
    builtinCount = count;
    reset();
}

void StaticMatcher::reset() {
    lastGesture = GESTURE_NONE;
    stableCount = 0;
}

uint8_t StaticMatcher::calculateConfidence(const int* fingerPos, const StaticGestureDef& gesture) {
    uint16_t totalDist = 0;
    uint8_t checkedFingers = 0;

    for (int i = 0; i < NUM_FINGERS; i++) {
        if (gesture.fingers[i].mode == CMP_ANY) continue;

        checkedFingers++;
        uint8_t value = normalizeFingerPos(fingerPos[i]);

        // Calculate distance from target center
        uint8_t target = (gesture.fingers[i].min + gesture.fingers[i].max) / 2;
        uint8_t dist = abs((int)value - (int)target);
        totalDist += dist;
    }

    if (checkedFingers == 0) return 100;

    // Convert average distance to confidence (0-100)
    uint8_t avgDist = totalDist / checkedFingers;
    if (avgDist > 127) return 0;
    return 100 - (avgDist * 100 / 127);
}

bool StaticMatcher::matchesGesture(const int* fingerPos, const StaticGestureDef& gesture) {
    for (int i = 0; i < NUM_FINGERS; i++) {
        uint8_t normalizedValue = normalizeFingerPos(fingerPos[i]);
        if (!matchesConstraint(normalizedValue, gesture.fingers[i])) {
            return false;
        }
    }
    return true;
}

GestureId StaticMatcher::match(const int* fingerPos, uint8_t* confidence) {
    GestureId bestMatch = GESTURE_NONE;
    uint8_t bestConfidence = 0;
    uint8_t bestPriority = 0;

    // Check built-in gestures (from PROGMEM)
    for (uint8_t i = 0; i < builtinCount; i++) {
        StaticGestureDef gesture;
        memcpy_P(&gesture, &builtinGestures[i], sizeof(StaticGestureDef));

        if (matchesGesture(fingerPos, gesture)) {
            uint8_t conf = calculateConfidence(fingerPos, gesture);
            if (gesture.priority > bestPriority ||
                (gesture.priority == bestPriority && conf > bestConfidence)) {
                bestMatch = gesture.id;
                bestConfidence = conf;
                bestPriority = gesture.priority;
            }
        }
    }

    // Check custom gestures (from RAM)
    for (uint8_t i = 0; i < customCount; i++) {
        const StaticGestureDef& gesture = customGestures[i];

        if (matchesGesture(fingerPos, gesture)) {
            uint8_t conf = calculateConfidence(fingerPos, gesture);
            if (gesture.priority > bestPriority ||
                (gesture.priority == bestPriority && conf > bestConfidence)) {
                bestMatch = gesture.id;
                bestConfidence = conf;
                bestPriority = gesture.priority;
            }
        }
    }

    // Simple debounce: require stable match for DEBOUNCE_FRAMES
    if (bestMatch == lastGesture) {
        if (stableCount < 255) stableCount++;
    } else {
        stableCount = 1;
        lastGesture = bestMatch;
    }

    if (confidence) *confidence = bestConfidence;

    // Return gesture if stable enough
    return (stableCount >= DEBOUNCE_FRAMES) ? bestMatch : GESTURE_NONE;
}

bool StaticMatcher::checkGesture(GestureId id, const int* fingerPos) {
    // Check built-in gestures
    for (uint8_t i = 0; i < builtinCount; i++) {
        StaticGestureDef gesture;
        memcpy_P(&gesture, &builtinGestures[i], sizeof(StaticGestureDef));
        if (gesture.id == id) {
            return matchesGesture(fingerPos, gesture);
        }
    }

    // Check custom gestures
    for (uint8_t i = 0; i < customCount; i++) {
        if (customGestures[i].id == id) {
            return matchesGesture(fingerPos, customGestures[i]);
        }
    }

    return false;
}

bool StaticMatcher::addCustomGesture(const StaticGestureDef& gesture) {
    if (customCount >= MAX_CUSTOM_STATIC_GESTURES) {
        return false;
    }

    // Check for duplicate ID - update if exists
    for (uint8_t i = 0; i < customCount; i++) {
        if (customGestures[i].id == gesture.id) {
            customGestures[i] = gesture;
            return true;
        }
    }

    // Add new gesture
    customGestures[customCount++] = gesture;
    return true;
}

bool StaticMatcher::removeCustomGesture(GestureId id) {
    for (uint8_t i = 0; i < customCount; i++) {
        if (customGestures[i].id == id) {
            // Shift remaining gestures down
            for (uint8_t j = i; j < customCount - 1; j++) {
                customGestures[j] = customGestures[j + 1];
            }
            customCount--;
            return true;
        }
    }
    return false;
}

void StaticMatcher::clearCustomGestures() {
    customCount = 0;
}
