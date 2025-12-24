#include "GestureRecognizer.h"

GestureRecognizer::GestureRecognizer()
    : lastStaticGesture(GESTURE_NONE)
    , lastDynamicGesture(GESTURE_NONE)
    , lastConfidence(0)
    , initialized(false) {
}

void GestureRecognizer::begin() {
    if (initialized) return;

    // Initialize static matcher with built-in gestures
    staticMatcher.begin(GESTURE_LIB_STATIC, GESTURE_LIB_STATIC_COUNT);

    // Register built-in dynamic gestures
    registerBuiltinDynamicGestures(dynamicMatcher);

    initialized = true;
}

void GestureRecognizer::reset() {
    staticMatcher.reset();
    dynamicMatcher.reset();
    lastStaticGesture = GESTURE_NONE;
    lastDynamicGesture = GESTURE_NONE;
    lastConfidence = 0;
}

GestureResult GestureRecognizer::recognizeEx(int fingers[5], uint16_t deltaTimeMs) {
    GestureResult result;
    result.staticGesture = GESTURE_NONE;
    result.dynamicGesture = GESTURE_NONE;
    result.confidence = 0;
    result.isNewStatic = false;
    result.isNewDynamic = false;

    if (!initialized) {
        begin();
    }

    // Match static gestures
    uint8_t confidence = 0;
    GestureId staticGesture = staticMatcher.match(fingers, &confidence);

    if (staticGesture != GESTURE_NONE) {
        result.staticGesture = staticGesture;
        result.confidence = confidence;
        result.isNewStatic = (staticGesture != lastStaticGesture);
        lastStaticGesture = staticGesture;
        lastConfidence = confidence;
    } else if (lastStaticGesture != GESTURE_NONE) {
        // Gesture ended
        result.isNewStatic = true;
        lastStaticGesture = GESTURE_NONE;
        lastConfidence = 0;
    }

    // Update dynamic gesture matcher
    GestureId dynamicGesture = dynamicMatcher.update(fingers, deltaTimeMs);

    if (dynamicGesture != GESTURE_NONE) {
        result.dynamicGesture = dynamicGesture;
        result.isNewDynamic = true;
        lastDynamicGesture = dynamicGesture;
    }

    return result;
}

int GestureRecognizer::recognize(int fingers[5]) {
    // Backward compatible interface - just return static gesture
    GestureResult result = recognizeEx(fingers, 10);
    return result.staticGesture;
}

const char* GestureRecognizer::getGestureName(int gesture) {
    // Check if it's a dynamic gesture (100+)
    if (gesture >= GESTURE_DYNAMIC_START) {
        return getDynamicGestureName((GestureId)gesture);
    }
    // Static gesture
    return getStaticGestureName((GestureId)gesture);
}
