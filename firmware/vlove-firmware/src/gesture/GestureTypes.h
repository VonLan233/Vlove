#pragma once

#include <stdint.h>

// Define ANALOG_MAX if not already defined
#ifndef ANALOG_MAX
  #if defined(ESP32)
    #define ANALOG_MAX 4095
  #else
    #define ANALOG_MAX 1023
  #endif
#endif

// Type definitions
typedef uint8_t FingerPos;   // 0=extended, 255=fully closed
typedef uint8_t GestureId;

// Gesture ID ranges
#define GESTURE_NONE           0
#define GESTURE_STATIC_START   1    // Static gestures 1-99
#define GESTURE_DYNAMIC_START  100  // Dynamic gestures 100-199
#define GESTURE_CUSTOM_START   200  // User-defined 200-254

// Finger indices
#define F_THUMB  0
#define F_INDEX  1
#define F_MIDDLE 2
#define F_RING   3
#define F_PINKY  4
#define NUM_FINGERS 5

// Comparison modes for finger constraints
enum CompareMode : uint8_t {
    CMP_RANGE,   // Value must be within [min, max]
    CMP_ABOVE,   // Value must be >= min
    CMP_BELOW,   // Value must be <= max
    CMP_ANY      // Any value (wildcard)
};

// Single finger constraint (3 bytes)
struct FingerConstraint {
    uint8_t mode;   // CompareMode
    uint8_t min;    // Minimum threshold (0-255 scale)
    uint8_t max;    // Maximum threshold (0-255 scale)
};

// Static gesture definition (17 bytes)
struct StaticGestureDef {
    GestureId id;                      // Gesture ID
    uint8_t priority;                  // Higher = checked first
    FingerConstraint fingers[5];       // Constraints for each finger
};

// Dynamic gesture phase (18 bytes)
struct DynamicPhase {
    FingerConstraint fingers[5];       // Finger constraints for this phase
    uint16_t minDurationMs;            // Minimum time in this phase
    uint16_t maxDurationMs;            // Maximum time (0 = no limit)
    uint8_t nextPhase;                 // Next phase index (0xFF = complete)
};

// Dynamic gesture definition header (6 bytes)
struct DynamicGestureDef {
    GestureId id;                      // Gesture ID
    uint8_t numPhases;                 // Number of phases (max 4)
    uint16_t timeoutMs;                // Total gesture timeout
    uint8_t debounceMs;                // Cooldown after recognition
    uint8_t reserved;                  // Padding for alignment
};

// Gesture recognition result
struct GestureResult {
    GestureId staticGesture;           // Currently matched static gesture
    GestureId dynamicGesture;          // Just-completed dynamic gesture
    uint8_t confidence;                // Match confidence 0-100
    bool isNewStatic;                  // True if static gesture just changed
    bool isNewDynamic;                 // True if dynamic gesture just completed
};

// Gesture tracker state for dynamic gestures
struct GestureTracker {
    uint8_t currentPhase;              // Current phase index (0xFF = not started)
    uint16_t phaseTimeMs;              // Time spent in current phase
    uint16_t totalTimeMs;              // Total time since gesture start
    bool active;                       // Is this gesture being tracked?
};

// Position constants (0-255 scale)
#define POS_EXTENDED    0
#define POS_SLIGHTLY   64
#define POS_HALF      128
#define POS_MOSTLY    192
#define POS_CLOSED    255
#define POS_TOLERANCE  40

// Helper macros for gesture definitions
// Adjusted thresholds based on Vlove sensor data
#define FINGER_EXTENDED  {CMP_BELOW, 0, 120}    // 0-120 = extended (relaxed)
#define FINGER_HALF      {CMP_RANGE, 100, 180}  // 100-180 = half bent
#define FINGER_CLOSED    {CMP_ABOVE, 150, 255}  // 150-255 = closed (relaxed)
#define FINGER_ANY       {CMP_ANY, 0, 255}

// Utility function to normalize ANALOG_MAX to 0-255 scale
inline uint8_t normalizeFingerPos(int rawValue) {
    if (rawValue <= 0) return 0;
    if (rawValue >= ANALOG_MAX) return 255;
    return (uint8_t)((long)rawValue * 255 / ANALOG_MAX);
}

// Check if a value matches a constraint
inline bool matchesConstraint(uint8_t value, const FingerConstraint& constraint) {
    switch (constraint.mode) {
        case CMP_RANGE:
            return (value >= constraint.min && value <= constraint.max);
        case CMP_ABOVE:
            return (value >= constraint.min);
        case CMP_BELOW:
            return (value <= constraint.max);
        case CMP_ANY:
            return true;
        default:
            return false;
    }
}
