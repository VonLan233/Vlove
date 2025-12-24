#pragma once

#include "Config.h"

class GestureRecognizer {
private:
  int lastGesture = GESTURE_NONE;
  int gestureCount = 0;

  // Finger state helpers
  bool isOpen(int value) {
    return value > FINGER_OPEN_THRESHOLD;
  }

  bool isClosed(int value) {
    return value < FINGER_CLOSED_THRESHOLD;
  }

  bool isHalf(int value) {
    return value >= FINGER_CLOSED_THRESHOLD && value <= FINGER_OPEN_THRESHOLD;
  }

public:
  int recognize(int fingers[5]) {
    int thumb = fingers[0];
    int index = fingers[1];
    int middle = fingers[2];
    int ring = fingers[3];
    int pinky = fingers[4];

    int detected = GESTURE_NONE;

    // ========== NUMBERS ==========

    // 0: Fist (all closed) or OK sign
    if (isClosed(thumb) && isClosed(index) && isClosed(middle) && isClosed(ring) && isClosed(pinky)) {
      detected = GESTURE_NUM_0;
    }
    // 1: Index up, others closed
    else if (isClosed(thumb) && isOpen(index) && isClosed(middle) && isClosed(ring) && isClosed(pinky)) {
      detected = GESTURE_NUM_1;
    }
    // 2: Index and middle up (peace sign)
    else if (isClosed(thumb) && isOpen(index) && isOpen(middle) && isClosed(ring) && isClosed(pinky)) {
      detected = GESTURE_NUM_2;
    }
    // 3: Index, middle, ring up
    else if (isClosed(thumb) && isOpen(index) && isOpen(middle) && isOpen(ring) && isClosed(pinky)) {
      detected = GESTURE_NUM_3;
    }
    // 4: All fingers up except thumb
    else if (isClosed(thumb) && isOpen(index) && isOpen(middle) && isOpen(ring) && isOpen(pinky)) {
      detected = GESTURE_NUM_4;
    }
    // 5: All fingers open (open hand)
    else if (isOpen(thumb) && isOpen(index) && isOpen(middle) && isOpen(ring) && isOpen(pinky)) {
      detected = GESTURE_NUM_5;
    }
    // 6: Thumb and pinky up, others closed
    else if (isOpen(thumb) && isClosed(index) && isClosed(middle) && isClosed(ring) && isOpen(pinky)) {
      detected = GESTURE_NUM_6;
    }
    // 7: Thumb, index, middle up
    else if (isOpen(thumb) && isOpen(index) && isOpen(middle) && isClosed(ring) && isClosed(pinky)) {
      detected = GESTURE_NUM_7;
    }
    // 8: Thumb, index, middle, ring up
    else if (isOpen(thumb) && isOpen(index) && isOpen(middle) && isOpen(ring) && isClosed(pinky)) {
      detected = GESTURE_NUM_8;
    }
    // 9: Thumb up, others in fist (or index curled with thumb touching)
    else if (isOpen(thumb) && isClosed(index) && isClosed(middle) && isClosed(ring) && isClosed(pinky)) {
      detected = GESTURE_NUM_9;
    }

    // ========== COMMON GESTURES ==========

    // Thumbs up: Only thumb up
    else if (isOpen(thumb) && isClosed(index) && isClosed(middle) && isClosed(ring) && isClosed(pinky)) {
      detected = GESTURE_THUMBS_UP;
    }
    // Rock sign: Index and pinky up, others closed
    else if (isClosed(thumb) && isOpen(index) && isClosed(middle) && isClosed(ring) && isOpen(pinky)) {
      detected = GESTURE_ROCK;
    }
    // OK sign: Thumb and index touching (both half), others open
    else if (isHalf(thumb) && isHalf(index) && isOpen(middle) && isOpen(ring) && isOpen(pinky)) {
      detected = GESTURE_OK;
    }
    // Point: Index up, thumb out, others closed
    else if (isHalf(thumb) && isOpen(index) && isClosed(middle) && isClosed(ring) && isClosed(pinky)) {
      detected = GESTURE_POINT;
    }
    // Call me (shaka): Thumb and pinky out
    else if (isOpen(thumb) && isClosed(index) && isClosed(middle) && isClosed(ring) && isOpen(pinky)) {
      detected = GESTURE_CALL;
    }
    // Fist: All closed
    else if (isClosed(thumb) && isClosed(index) && isClosed(middle) && isClosed(ring) && isClosed(pinky)) {
      detected = GESTURE_FIST;
    }
    // Open hand: All open
    else if (isOpen(thumb) && isOpen(index) && isOpen(middle) && isOpen(ring) && isOpen(pinky)) {
      detected = GESTURE_OPEN_HAND;
    }
    // Peace: Index and middle up
    else if (isClosed(thumb) && isOpen(index) && isOpen(middle) && isClosed(ring) && isClosed(pinky)) {
      detected = GESTURE_PEACE;
    }

    // Debounce: require consistent readings
    if (detected == lastGesture) {
      gestureCount++;
    } else {
      lastGesture = detected;
      gestureCount = 1;
    }

    if (gestureCount >= GESTURE_DEBOUNCE_COUNT) {
      return detected;
    }

    return GESTURE_NONE;
  }

  const char* getGestureName(int gesture) {
    switch (gesture) {
      case GESTURE_NONE:       return "None";
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
      case GESTURE_THUMBS_DOWN:return "ThumbsDown";
      case GESTURE_PEACE:      return "Peace";
      case GESTURE_ROCK:       return "Rock";
      case GESTURE_OK:         return "OK";
      case GESTURE_FIST:       return "Fist";
      case GESTURE_OPEN_HAND:  return "OpenHand";
      case GESTURE_POINT:      return "Point";
      case GESTURE_CALL:       return "CallMe";
      default:                 return "Unknown";
    }
  }
};
