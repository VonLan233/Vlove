#pragma once

#include "Config.h"
#include "gesture/GestureTypes.h"
#include "gesture/StaticMatcher.h"
#include "gesture/DynamicMatcher.h"
#include "gesture/GestureLib.h"

class GestureRecognizer {
public:
    GestureRecognizer();

    // Initialize the recognizer with built-in gesture libraries
    void begin();

    // Main recognition function (backward compatible)
    // Returns gesture ID using the legacy recognize() interface
    int recognize(int fingers[5]);

    // Extended recognition function
    // Returns full GestureResult with static, dynamic, and confidence
    GestureResult recognizeEx(int fingers[5], uint16_t deltaTimeMs = 10);

    // Get gesture name (backward compatible)
    const char* getGestureName(int gesture);

    // Access to sub-matchers for advanced usage
    StaticMatcher& getStaticMatcher() { return staticMatcher; }
    DynamicMatcher& getDynamicMatcher() { return dynamicMatcher; }

    // Add custom static gesture
    bool addStaticGesture(const StaticGestureDef& gesture) {
        return staticMatcher.addCustomGesture(gesture);
    }

    // Add custom dynamic gesture
    bool addDynamicGesture(const DynamicGestureDef& header, const DynamicPhase* phases) {
        return dynamicMatcher.registerGesture(header, phases);
    }

    // Reset all tracking state
    void reset();

    // Get last recognized gestures
    GestureId getLastStaticGesture() const { return lastStaticGesture; }
    GestureId getLastDynamicGesture() const { return lastDynamicGesture; }
    uint8_t getLastConfidence() const { return lastConfidence; }

private:
    StaticMatcher staticMatcher;
    DynamicMatcher dynamicMatcher;

    GestureId lastStaticGesture;
    GestureId lastDynamicGesture;
    uint8_t lastConfidence;
    bool initialized;
};
