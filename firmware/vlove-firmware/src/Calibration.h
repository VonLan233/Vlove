#pragma once

#include <EEPROM.h>
#include "Config.h"

class Calibration {
public:
  // Calibration boundaries
  int minVal[5] = {0, 0, 0, 0, 0};
  int maxVal[5] = {4095, 4095, 4095, 4095, 4095};
  bool isCalibrating = false;
  bool hasValidCalibration = false;

private:
  // Histogram for percentile calculation (256 bins, each represents 16 ADC values)
  static const int HISTOGRAM_BINS = 256;
  static const int BIN_SIZE = 16;  // 4096 / 256
  uint16_t histogram[5][HISTOGRAM_BINS];
  uint32_t totalSamples[5];

  // Debounce buffer
  static const int STABLE_BUFFER_SIZE = 5;
  int stableBuffer[5][STABLE_BUFFER_SIZE];
  int bufferIndex[5];
  bool bufferFilled[5];

  int sampleCount;

  // Configuration
  static const int MARGIN_PERCENT = 5;      // Range margin percentage
  static const int MIN_RANGE = 500;         // Minimum valid range
  static const int PERCENTILE_LOW = 2;      // Use 2nd percentile as min (exclude outlier lows)
  static const int PERCENTILE_HIGH = 98;    // Use 98th percentile as max (exclude outlier highs)
  static const int STABLE_THRESHOLD = 50;   // Debounce threshold: max difference in buffer

public:
  void begin() {
    EEPROM.begin(EEPROM_SIZE);
    loadFromEEPROM();
  }

  void startCalibration() {
    isCalibrating = true;
    sampleCount = 0;

    // Clear histogram
    for (int i = 0; i < 5; i++) {
      for (int j = 0; j < HISTOGRAM_BINS; j++) {
        histogram[i][j] = 0;
      }
      totalSamples[i] = 0;
      bufferIndex[i] = 0;
      bufferFilled[i] = false;
      for (int j = 0; j < STABLE_BUFFER_SIZE; j++) {
        stableBuffer[i][j] = 0;
      }
    }

    Serial.println();
    Serial.println("****************************************");
    Serial.println("*       CALIBRATION MODE ACTIVE        *");
    Serial.println("****************************************");
    Serial.println();
    Serial.println("Improved calibration with:");
    Serial.println("  - Debounce filtering (stable readings only)");
    Serial.println("  - Percentile-based range (excludes outliers)");
    Serial.println();
    Serial.println("Move ALL fingers through full range:");
    Serial.println("  1. Make a tight fist (curl all fingers)");
    Serial.println("  2. Open hand fully (extend all fingers)");
    Serial.println("  3. Hold each position for 1-2 seconds");
    Serial.println("  4. Repeat 3-5 times slowly");
    Serial.println();
    Serial.println("Type 'DONE' or press ENTER when finished.");
    Serial.println();
  }

  void update(int raw[5]) {
    if (!isCalibrating) return;

    sampleCount++;

    for (int i = 0; i < 5; i++) {
      int value = raw[i];

      // Update debounce buffer
      stableBuffer[i][bufferIndex[i]] = value;
      bufferIndex[i] = (bufferIndex[i] + 1) % STABLE_BUFFER_SIZE;
      if (bufferIndex[i] == 0) {
        bufferFilled[i] = true;
      }

      // Only check stability after buffer is filled
      if (!bufferFilled[i]) continue;

      // Check if stable (values in buffer differ less than threshold)
      if (!isStable(i)) continue;

      // When stable, add value to histogram
      int bin = constrain(value / BIN_SIZE, 0, HISTOGRAM_BINS - 1);
      if (histogram[i][bin] < 65535) {  // Prevent overflow
        histogram[i][bin]++;
        totalSamples[i]++;
      }
    }
  }

  bool isStable(int finger) {
    int minV = stableBuffer[finger][0];
    int maxV = stableBuffer[finger][0];
    for (int j = 1; j < STABLE_BUFFER_SIZE; j++) {
      if (stableBuffer[finger][j] < minV) minV = stableBuffer[finger][j];
      if (stableBuffer[finger][j] > maxV) maxV = stableBuffer[finger][j];
    }
    return (maxV - minV) <= STABLE_THRESHOLD;
  }

  // Calculate percentile from histogram
  int getPercentile(int finger, int percentile) {
    if (totalSamples[finger] == 0) return (percentile < 50) ? 0 : 4095;

    uint32_t targetCount = (totalSamples[finger] * percentile) / 100;
    uint32_t cumulative = 0;

    for (int bin = 0; bin < HISTOGRAM_BINS; bin++) {
      cumulative += histogram[finger][bin];
      if (cumulative >= targetCount) {
        return bin * BIN_SIZE + BIN_SIZE / 2;  // Return center value of the bin
      }
    }
    return 4095;
  }

  void printStatus(int raw[5]) {
    const char* names[] = {"T", "I", "M", "R", "P"};
    Serial.print("Samples: ");
    Serial.print(sampleCount);
    Serial.print(" (stable: ");

    uint32_t totalStable = 0;
    for (int i = 0; i < 5; i++) {
      totalStable += totalSamples[i];
    }
    Serial.print(totalStable / 5);
    Serial.print(") | ");

    for (int i = 0; i < 5; i++) {
      int p2 = getPercentile(i, PERCENTILE_LOW);
      int p98 = getPercentile(i, PERCENTILE_HIGH);
      int range = p98 - p2;

      Serial.print(names[i]);
      Serial.print(":");
      Serial.print(raw[i]);

      // Display stability status
      if (isStable(i)) {
        Serial.print("*");  // Stable marker
      } else {
        Serial.print(" ");
      }

      Serial.print("[");
      Serial.print(p2);
      Serial.print("-");
      Serial.print(p98);
      Serial.print("]");

      if (range < MIN_RANGE) {
        Serial.print("!");  // Insufficient range warning
      }
      Serial.print(" ");
    }
    Serial.println();
  }

  void stopCalibration() {
    isCalibrating = false;

    Serial.println();
    Serial.println("****************************************");
    Serial.println("*       CALIBRATION COMPLETE!          *");
    Serial.println("****************************************");
    Serial.println();
    Serial.print("Total samples: ");
    Serial.print(sampleCount);
    Serial.print(" (stable samples per finger: ~");
    uint32_t avgStable = 0;
    for (int i = 0; i < 5; i++) {
      avgStable += totalSamples[i];
    }
    Serial.print(avgStable / 5);
    Serial.println(")");
    Serial.println();
    Serial.println("Results (2nd - 98th percentile):");

    const char* names[] = {"Thumb ", "Index ", "Middle", "Ring  ", "Pinky "};
    bool allGood = true;

    for (int i = 0; i < 5; i++) {
      int p2 = getPercentile(i, PERCENTILE_LOW);
      int p98 = getPercentile(i, PERCENTILE_HIGH);
      int rawRange = p98 - p2;

      // Check if there is valid data
      if (totalSamples[i] < 100 || rawRange < 100) {
        Serial.print("  ");
        Serial.print(names[i]);
        Serial.print(": NO DATA (");
        Serial.print(totalSamples[i]);
        Serial.println(" samples) - move finger more slowly!");
        minVal[i] = 0;
        maxVal[i] = 4095;
        allGood = false;
        continue;
      }

      // Add margin expansion
      int margin = rawRange * MARGIN_PERCENT / 100;
      minVal[i] = max(0, p2 - margin);
      maxVal[i] = min(4095, p98 + margin);

      int finalRange = maxVal[i] - minVal[i];

      Serial.print("  ");
      Serial.print(names[i]);
      Serial.print(": ");
      Serial.print(minVal[i]);
      Serial.print(" -> ");
      Serial.print(maxVal[i]);
      Serial.print("  (range: ");
      Serial.print(finalRange);

      if (rawRange < MIN_RANGE) {
        Serial.println(" WARNING: low range!)");
        allGood = false;
      } else {
        Serial.println(" OK)");
      }
    }

    Serial.println();
    if (allGood) {
      Serial.println("Calibration OK! All fingers have good range.");
    } else {
      Serial.println("WARNING: Some fingers have limited range.");
      Serial.println("Try 'CAL' again, hold positions longer.");
    }

    hasValidCalibration = true;
    saveToEEPROM();
    Serial.println("Saved to EEPROM.");
    Serial.println("****************************************");
    Serial.println();
  }

  // Map raw value to 0-ANALOG_MAX using calibration
  int mapValue(int finger, int rawValue) {
    if (finger < 0 || finger >= 5) return rawValue;
    if (maxVal[finger] <= minVal[finger]) return rawValue;

    int mapped = map(rawValue, minVal[finger], maxVal[finger], 0, ANALOG_MAX);
    return constrain(mapped, 0, ANALOG_MAX);
  }

  void saveToEEPROM() {
    EEPROM.write(EEPROM_CAL_MAGIC_ADDR, EEPROM_CAL_MAGIC);

    int addr = EEPROM_CAL_DATA_ADDR;
    for (int i = 0; i < 5; i++) {
      EEPROM.put(addr, minVal[i]);
      addr += sizeof(int);
      EEPROM.put(addr, maxVal[i]);
      addr += sizeof(int);
    }
    EEPROM.commit();
  }

  void loadFromEEPROM() {
    if (EEPROM.read(EEPROM_CAL_MAGIC_ADDR) != EEPROM_CAL_MAGIC) {
      hasValidCalibration = false;
      return;
    }

    int addr = EEPROM_CAL_DATA_ADDR;
    for (int i = 0; i < 5; i++) {
      EEPROM.get(addr, minVal[i]);
      addr += sizeof(int);
      EEPROM.get(addr, maxVal[i]);
      addr += sizeof(int);
    }
    hasValidCalibration = true;
  }

  void clearEEPROM() {
    EEPROM.write(EEPROM_CAL_MAGIC_ADDR, 0);
    EEPROM.commit();
    hasValidCalibration = false;
  }
};
