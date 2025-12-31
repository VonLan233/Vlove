#pragma once

#include "GestureTypes.h"

// Maximum number of custom gestures
#define MAX_CUSTOM_STATIC_GESTURES 16

class StaticMatcher {
public:
    StaticMatcher();

    // Initialize with built-in gesture library
    void begin(const StaticGestureDef* gestures, uint8_t count);

    // Match current finger positions against all gestures
    // Returns gesture ID and sets confidence (0-100)
    GestureId match(const int* fingerPos, uint8_t* confidence = nullptr);

    // Check if a specific gesture matches
    bool checkGesture(GestureId id, const int* fingerPos);

    // Add custom gesture at runtime
    bool addCustomGesture(const StaticGestureDef& gesture);

    // Remove custom gesture by ID
    bool removeCustomGesture(GestureId id);

    // Clear all custom gestures
    void clearCustomGestures();

    // Get number of registered gestures
    uint8_t getBuiltinCount() const { return builtinCount; }
    uint8_t getCustomCount() const { return customCount; }

    // Reset debounce state
    void reset();

private:
    // Built-in gestures (stored in PROGMEM)
    const StaticGestureDef* builtinGestures;
    uint8_t builtinCount;

    // Custom gestures (stored in RAM)
    StaticGestureDef customGestures[MAX_CUSTOM_STATIC_GESTURES];
    uint8_t customCount;

    // Debouncing state
    GestureId lastGesture;
    uint8_t stableCount;
    static const uint8_t DEBOUNCE_FRAMES = 2;  // Minimal debounce

    // Internal matching functions
    uint8_t calculateConfidence(const int* fingerPos, const StaticGestureDef& gesture);
    bool matchesGesture(const int* fingerPos, const StaticGestureDef& gesture);
};
