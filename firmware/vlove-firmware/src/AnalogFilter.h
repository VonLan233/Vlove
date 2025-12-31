#pragma once

#include <Arduino.h>
#include "Config.h"

// Filter configuration
#define OVERSAMPLE_COUNT    4      // Number of oversampling reads per measurement
#define FILTER_WINDOW_SIZE  5      // Median filter sliding window size
#define EMA_ALPHA           0.3f   // Exponential moving average coefficient (0.1-0.5, higher = faster response)
#define DEADZONE            15     // Deadzone threshold, ignore changes smaller than this value

class AnalogFilter {
private:
  // Filter state for each finger
  int window[5][FILTER_WINDOW_SIZE];  // Median filter window
  int windowIndex[5];                  // Current write position in window
  float emaValue[5];                   // Current EMA value
  int lastOutput[5];                   // Last output value (for deadzone)
  bool initialized[5];                 // Whether initialized

  // Pin mapping
  int pins[5];
  bool inverted[5];

public:
  AnalogFilter() {
    // Initialize pins
    pins[0] = PIN_THUMB;
    pins[1] = PIN_INDEX;
    pins[2] = PIN_MIDDLE;
    pins[3] = PIN_RING;
    pins[4] = PIN_PINKY;

    inverted[0] = INVERT_THUMB;
    inverted[1] = INVERT_INDEX;
    inverted[2] = INVERT_MIDDLE;
    inverted[3] = INVERT_RING;
    inverted[4] = INVERT_PINKY;

    for (int i = 0; i < 5; i++) {
      windowIndex[i] = 0;
      emaValue[i] = 0;
      lastOutput[i] = 0;
      initialized[i] = false;
      for (int j = 0; j < FILTER_WINDOW_SIZE; j++) {
        window[i][j] = 0;
      }
    }
  }

  void begin() {
    // Configure ADC resolution and attenuation (ESP32 specific)
    #ifdef ESP32
    analogReadResolution(12);  // 12-bit resolution
    analogSetAttenuation(ADC_11db);  // Full range 0-3.3V
    #endif

    // Warm-up: read a few times to initialize the filter
    for (int i = 0; i < FILTER_WINDOW_SIZE * 2; i++) {
      int dummy[5];
      readFiltered(dummy);
      delay(5);
    }
  }

  // Read raw value from a single pin (with oversampling)
  int readRawOversampled(int pin, bool invert) {
    long sum = 0;
    for (int i = 0; i < OVERSAMPLE_COUNT; i++) {
      sum += analogRead(pin);
    }
    int value = sum / OVERSAMPLE_COUNT;

    if (invert) {
      value = ANALOG_MAX - value;
    }
    return value;
  }

  // Median filter: return the median value in the window
  int getMedian(int finger) {
    // Copy window data and sort
    int sorted[FILTER_WINDOW_SIZE];
    for (int i = 0; i < FILTER_WINDOW_SIZE; i++) {
      sorted[i] = window[finger][i];
    }

    // Simple bubble sort (window is small, efficiency is sufficient)
    for (int i = 0; i < FILTER_WINDOW_SIZE - 1; i++) {
      for (int j = 0; j < FILTER_WINDOW_SIZE - i - 1; j++) {
        if (sorted[j] > sorted[j + 1]) {
          int temp = sorted[j];
          sorted[j] = sorted[j + 1];
          sorted[j + 1] = temp;
        }
      }
    }

    return sorted[FILTER_WINDOW_SIZE / 2];
  }

  // Read filtered values for all fingers
  void readFiltered(int output[5]) {
    for (int i = 0; i < 5; i++) {
      // 1. Read raw value with oversampling
      int raw = readRawOversampled(pins[i], inverted[i]);

      // 2. Thumb offset correction (poor contact causes high baseline)
      if (i == 0) {
        raw = max(0, raw - 200 * ANALOG_MAX / 255);  // Subtract ~200 (on 0-255 scale)
      }

      // 3. Update median filter window
      window[i][windowIndex[i]] = raw;
      windowIndex[i] = (windowIndex[i] + 1) % FILTER_WINDOW_SIZE;

      // 4. Get median value
      int median = getMedian(i);

      // 5. Apply exponential moving average
      if (!initialized[i]) {
        emaValue[i] = median;
        lastOutput[i] = median;
        initialized[i] = true;
      } else {
        emaValue[i] = EMA_ALPHA * median + (1.0f - EMA_ALPHA) * emaValue[i];
      }

      int filtered = (int)emaValue[i];

      // 6. Apply deadzone (reduce jitter)
      if (abs(filtered - lastOutput[i]) > DEADZONE) {
        lastOutput[i] = filtered;
      }

      output[i] = lastOutput[i];
    }
  }

  // Read raw values (no filtering, for debugging)
  void readRaw(int output[5]) {
    for (int i = 0; i < 5; i++) {
      output[i] = readRawOversampled(pins[i], inverted[i]);
    }
  }

  // Reset filter state (use after calibration)
  void reset() {
    for (int i = 0; i < 5; i++) {
      initialized[i] = false;
      windowIndex[i] = 0;
    }
  }
};
