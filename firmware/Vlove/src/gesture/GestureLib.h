#pragma once

#include "GestureTypes.h"
#include "DynamicMatcher.h"
#include <pgmspace.h>

// ============================================
// Static Gesture IDs (matching Vlove Config.h)
// ============================================

// Numbers 0-9
#define GESTURE_NUM_0      1
#define GESTURE_NUM_1      2
#define GESTURE_NUM_2      3
#define GESTURE_NUM_3      4
#define GESTURE_NUM_4      5
#define GESTURE_NUM_5      6
#define GESTURE_NUM_6      7
#define GESTURE_NUM_7      8
#define GESTURE_NUM_8      9
#define GESTURE_NUM_9      10

// Common gestures
#define GESTURE_THUMBS_UP    11
#define GESTURE_PEACE        12
#define GESTURE_ROCK         13
#define GESTURE_OK           14
#define GESTURE_FIST         15
#define GESTURE_OPEN_HAND    16
#define GESTURE_POINT        17
#define GESTURE_CALL         18
#define GESTURE_GUN          19

// ============================================
// Dynamic Gesture IDs
// ============================================
#define GESTURE_WAVE          100
#define GESTURE_FIST_RELEASE  101
#define GESTURE_PINCH_RELEASE 102

// ============================================
// Static Gesture Library
// Priority: 100 = high specificity, 90 = general pose
// ============================================
const StaticGestureDef PROGMEM GESTURE_LIB_STATIC[] = {
    // ----- Numbers -----

    // 0: Fist (all closed)
    { GESTURE_NUM_0, 90,
      { FINGER_CLOSED, FINGER_CLOSED, FINGER_CLOSED, FINGER_CLOSED, FINGER_CLOSED } },

    // 1: Index up, others closed
    { GESTURE_NUM_1, 100,
      { FINGER_CLOSED, FINGER_EXTENDED, FINGER_CLOSED, FINGER_CLOSED, FINGER_CLOSED } },

    // 2: Index and middle up (peace sign)
    { GESTURE_NUM_2, 100,
      { FINGER_CLOSED, FINGER_EXTENDED, FINGER_EXTENDED, FINGER_CLOSED, FINGER_CLOSED } },

    // 3: Index, middle, ring up
    { GESTURE_NUM_3, 100,
      { FINGER_CLOSED, FINGER_EXTENDED, FINGER_EXTENDED, FINGER_EXTENDED, FINGER_CLOSED } },

    // 4: All fingers up except thumb
    { GESTURE_NUM_4, 100,
      { FINGER_CLOSED, FINGER_EXTENDED, FINGER_EXTENDED, FINGER_EXTENDED, FINGER_EXTENDED } },

    // 5: All fingers open (open hand)
    { GESTURE_NUM_5, 90,
      { FINGER_EXTENDED, FINGER_EXTENDED, FINGER_EXTENDED, FINGER_EXTENDED, FINGER_EXTENDED } },

    // 6: Thumb and pinky up (shaka)
    { GESTURE_NUM_6, 100,
      { FINGER_EXTENDED, FINGER_CLOSED, FINGER_CLOSED, FINGER_CLOSED, FINGER_EXTENDED } },

    // 7: Thumb, index, middle up
    { GESTURE_NUM_7, 100,
      { FINGER_EXTENDED, FINGER_EXTENDED, FINGER_EXTENDED, FINGER_CLOSED, FINGER_CLOSED } },

    // 8: Thumb, index, middle, ring up
    { GESTURE_NUM_8, 100,
      { FINGER_EXTENDED, FINGER_EXTENDED, FINGER_EXTENDED, FINGER_EXTENDED, FINGER_CLOSED } },

    // 9: Thumb up only (same as thumbs up)
    { GESTURE_NUM_9, 95,
      { FINGER_EXTENDED, FINGER_CLOSED, FINGER_CLOSED, FINGER_CLOSED, FINGER_CLOSED } },

    // ----- Common Gestures -----

    // Thumbs up: Only thumb up
    { GESTURE_THUMBS_UP, 100,
      { FINGER_EXTENDED, FINGER_CLOSED, FINGER_CLOSED, FINGER_CLOSED, FINGER_CLOSED } },

    // Peace: Index and middle up
    { GESTURE_PEACE, 100,
      { FINGER_CLOSED, FINGER_EXTENDED, FINGER_EXTENDED, FINGER_CLOSED, FINGER_CLOSED } },

    // Rock: Index and pinky up
    { GESTURE_ROCK, 100,
      { FINGER_CLOSED, FINGER_EXTENDED, FINGER_CLOSED, FINGER_CLOSED, FINGER_EXTENDED } },

    // OK: Thumb and index half (touching), others extended
    { GESTURE_OK, 100,
      { FINGER_HALF, FINGER_HALF, FINGER_EXTENDED, FINGER_EXTENDED, FINGER_EXTENDED } },

    // Fist: All closed
    { GESTURE_FIST, 90,
      { FINGER_CLOSED, FINGER_CLOSED, FINGER_CLOSED, FINGER_CLOSED, FINGER_CLOSED } },

    // Open hand: All open
    { GESTURE_OPEN_HAND, 90,
      { FINGER_EXTENDED, FINGER_EXTENDED, FINGER_EXTENDED, FINGER_EXTENDED, FINGER_EXTENDED } },

    // Point: Index extended, thumb half, others closed
    { GESTURE_POINT, 100,
      { FINGER_HALF, FINGER_EXTENDED, FINGER_CLOSED, FINGER_CLOSED, FINGER_CLOSED } },

    // Call me (shaka): Thumb and pinky extended
    { GESTURE_CALL, 100,
      { FINGER_EXTENDED, FINGER_CLOSED, FINGER_CLOSED, FINGER_CLOSED, FINGER_EXTENDED } },

    // Gun: Thumb and index extended
    { GESTURE_GUN, 100,
      { FINGER_EXTENDED, FINGER_EXTENDED, FINGER_CLOSED, FINGER_CLOSED, FINGER_CLOSED } },
};

#define GESTURE_LIB_STATIC_COUNT (sizeof(GESTURE_LIB_STATIC) / sizeof(StaticGestureDef))

// ============================================
// Dynamic Gesture Definitions
// ============================================

// Wave Gesture: Open -> flex -> open -> flex
const DynamicGestureDef GESTURE_WAVE_DEF = {
    GESTURE_WAVE,   // id
    4,              // numPhases
    1500,           // timeoutMs
    50,             // debounceMs
    0               // reserved
};

const DynamicPhase GESTURE_WAVE_PHASES[] = {
    // Phase 0: All fingers extended
    {
        { FINGER_EXTENDED, FINGER_EXTENDED, FINGER_EXTENDED, FINGER_EXTENDED, FINGER_EXTENDED },
        80, 300, 1
    },
    // Phase 1: Fingers slightly flexed
    {
        { FINGER_HALF, FINGER_HALF, FINGER_HALF, FINGER_HALF, FINGER_HALF },
        50, 200, 2
    },
    // Phase 2: Fingers extended again
    {
        { FINGER_EXTENDED, FINGER_EXTENDED, FINGER_EXTENDED, FINGER_EXTENDED, FINGER_EXTENDED },
        50, 200, 3
    },
    // Phase 3: Fingers flexed again (complete)
    {
        { FINGER_HALF, FINGER_HALF, FINGER_HALF, FINGER_HALF, FINGER_HALF },
        50, 0, 0xFF
    }
};

// Fist Release: Closed fist -> open hand
const DynamicGestureDef GESTURE_FIST_RELEASE_DEF = {
    GESTURE_FIST_RELEASE,   // id
    3,                      // numPhases
    1000,                   // timeoutMs
    40,                     // debounceMs
    0                       // reserved
};

const DynamicPhase GESTURE_FIST_RELEASE_PHASES[] = {
    // Phase 0: Closed fist
    {
        { FINGER_CLOSED, FINGER_CLOSED, FINGER_CLOSED, FINGER_CLOSED, FINGER_CLOSED },
        100, 500, 1
    },
    // Phase 1: Open hand
    {
        { FINGER_EXTENDED, FINGER_EXTENDED, FINGER_EXTENDED, FINGER_EXTENDED, FINGER_EXTENDED },
        80, 400, 2
    },
    // Phase 2: Hold (complete)
    {
        { FINGER_ANY, FINGER_ANY, FINGER_ANY, FINGER_ANY, FINGER_ANY },
        50, 0, 0xFF
    }
};

// Pinch Release: Thumb + index pinched -> release
const DynamicGestureDef GESTURE_PINCH_RELEASE_DEF = {
    GESTURE_PINCH_RELEASE,  // id
    3,                      // numPhases
    800,                    // timeoutMs
    30,                     // debounceMs
    0                       // reserved
};

const DynamicPhase GESTURE_PINCH_RELEASE_PHASES[] = {
    // Phase 0: Thumb and index closed (pinching)
    {
        { FINGER_CLOSED, FINGER_CLOSED, FINGER_ANY, FINGER_ANY, FINGER_ANY },
        80, 400, 1
    },
    // Phase 1: Thumb and index open (released)
    {
        { FINGER_EXTENDED, FINGER_EXTENDED, FINGER_ANY, FINGER_ANY, FINGER_ANY },
        50, 300, 2
    },
    // Phase 2: Hold (complete)
    {
        { FINGER_ANY, FINGER_ANY, FINGER_ANY, FINGER_ANY, FINGER_ANY },
        30, 0, 0xFF
    }
};

// ============================================
// Helper Functions
// ============================================

// Register all built-in dynamic gestures
inline void registerBuiltinDynamicGestures(DynamicMatcher& matcher) {
    matcher.registerGesture(GESTURE_WAVE_DEF, GESTURE_WAVE_PHASES);
    matcher.registerGesture(GESTURE_FIST_RELEASE_DEF, GESTURE_FIST_RELEASE_PHASES);
    matcher.registerGesture(GESTURE_PINCH_RELEASE_DEF, GESTURE_PINCH_RELEASE_PHASES);
}

// Static gesture name lookup
inline const char* getStaticGestureName(GestureId id) {
    switch (id) {
        case GESTURE_NUM_0:      return "0";
        case GESTURE_NUM_1:      return "1";
        case GESTURE_NUM_2:      return "2";
        case GESTURE_NUM_3:      return "3";
        case GESTURE_NUM_4:      return "4";
        case GESTURE_NUM_5:      return "5";
        case GESTURE_NUM_6:      return "6";
        case GESTURE_NUM_7:      return "7";
        case GESTURE_NUM_8:      return "8";
        case GESTURE_NUM_9:      return "9";
        case GESTURE_THUMBS_UP:  return "ThumbsUp";
        case GESTURE_PEACE:      return "Peace";
        case GESTURE_ROCK:       return "Rock";
        case GESTURE_OK:         return "OK";
        case GESTURE_FIST:       return "Fist";
        case GESTURE_OPEN_HAND:  return "OpenHand";
        case GESTURE_POINT:      return "Point";
        case GESTURE_CALL:       return "CallMe";
        case GESTURE_GUN:        return "Gun";
        case GESTURE_NONE:       return "None";
        default:                 return "Unknown";
    }
}

// Dynamic gesture name lookup
inline const char* getDynamicGestureName(GestureId id) {
    switch (id) {
        case GESTURE_WAVE:          return "Wave";
        case GESTURE_FIST_RELEASE:  return "FistRelease";
        case GESTURE_PINCH_RELEASE: return "PinchRelease";
        case GESTURE_NONE:          return "None";
        default:                    return "Unknown";
    }
}
