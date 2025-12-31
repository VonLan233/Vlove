#pragma once

#include "GestureTypes.h"

// Maximum number of dynamic gestures to track simultaneously
#define MAX_DYNAMIC_GESTURES 4

// Maximum phases per dynamic gesture
#define MAX_GESTURE_PHASES 4

// Dynamic gesture registration structure
struct DynamicGestureEntry {
    DynamicGestureDef header;
    DynamicPhase phases[MAX_GESTURE_PHASES];
    bool active;
};

class DynamicMatcher {
public:
    DynamicMatcher();

    // Register a dynamic gesture
    bool registerGesture(const DynamicGestureDef& header, const DynamicPhase* phases);

    // Unregister a gesture by ID
    bool unregisterGesture(GestureId id);

    // Clear all registered gestures
    void clearGestures();

    // Update state machine with current finger positions
    // Returns completed gesture ID or GESTURE_NONE
    GestureId update(const int* fingerPos, uint16_t deltaTimeMs);

    // Reset all tracking state
    void reset();

    // Get current phase of a specific gesture (for debugging)
    int8_t getCurrentPhase(GestureId id) const;

    // Get number of registered gestures
    uint8_t getGestureCount() const { return gestureCount; }

private:
    // Registered dynamic gestures
    DynamicGestureEntry gestures[MAX_DYNAMIC_GESTURES];
    uint8_t gestureCount;

    // Tracking state for each gesture
    GestureTracker trackers[MAX_DYNAMIC_GESTURES];

    // Debounce after recognition
    GestureId lastRecognized;
    uint16_t debounceTimeMs;

    // Helper methods
    bool matchesPhase(const int* fingerPos, const DynamicPhase& phase);
    int findGestureIndex(GestureId id) const;
    void resetTracker(uint8_t index);
};
