#pragma once

#include <EEPROM.h>
#include "Config.h"

class Calibration {
public:
  // 校准边界
  int minVal[5] = {0, 0, 0, 0, 0};
  int maxVal[5] = {4095, 4095, 4095, 4095, 4095};
  bool isCalibrating = false;
  bool hasValidCalibration = false;

private:
  // 校准过程中的临时数据
  int tempMin[5];  // 临时最小值
  int tempMax[5];  // 临时最大值
  int prevValue[5];
  int sampleCount;

  // 配置
  static const int MARGIN_PERCENT = 5;     // 范围边距百分比（扩展范围避免边缘死区）
  static const int SPIKE_THRESHOLD = 300;  // 突变阈值（过滤异常读数）
  static const int MIN_RANGE = 500;        // 最小有效范围

public:
  void begin() {
    EEPROM.begin(EEPROM_SIZE);
    loadFromEEPROM();
  }

  void startCalibration() {
    isCalibrating = true;
    sampleCount = 0;

    // Reset - 初始值设为极端，便于记录实际范围
    for (int i = 0; i < 5; i++) {
      tempMin[i] = 4095;  // 将被实际最小值替换
      tempMax[i] = 0;     // 将被实际最大值替换
      prevValue[i] = -1;
    }

    Serial.println();
    Serial.println("****************************************");
    Serial.println("*       CALIBRATION MODE ACTIVE        *");
    Serial.println("****************************************");
    Serial.println();
    Serial.println("Move ALL fingers through full range:");
    Serial.println("  1. Make a tight fist (curl all fingers)");
    Serial.println("  2. Open hand fully (extend all fingers)");
    Serial.println("  3. Repeat 3-5 times slowly");
    Serial.println();
    Serial.println("Tip: Move fingers slowly for better accuracy!");
    Serial.println();
    Serial.println("Type 'DONE' or press ENTER when finished.");
    Serial.println();
  }

  void update(int raw[5]) {
    if (!isCalibrating) return;

    sampleCount++;

    for (int i = 0; i < 5; i++) {
      int value = raw[i];

      // 过滤异常突变（如果不是首次读取）
      if (prevValue[i] >= 0) {
        int diff = abs(value - prevValue[i]);
        if (diff > SPIKE_THRESHOLD) {
          // 忽略这个读数，可能是噪声
          continue;
        }
      }

      // 更新最小值和最大值
      if (value < tempMin[i]) {
        tempMin[i] = value;
      }
      if (value > tempMax[i]) {
        tempMax[i] = value;
      }

      prevValue[i] = value;
    }
  }

  void printStatus(int raw[5]) {
    const char* names[] = {"T", "I", "M", "R", "P"};
    Serial.print("Samples: ");
    Serial.print(sampleCount);
    Serial.print(" | ");
    for (int i = 0; i < 5; i++) {
      int range = tempMax[i] - tempMin[i];
      Serial.print(names[i]);
      Serial.print(":");
      Serial.print(raw[i]);
      Serial.print("[");
      Serial.print(tempMin[i]);
      Serial.print("-");
      Serial.print(tempMax[i]);
      Serial.print("]");
      // 显示范围状态
      if (range < MIN_RANGE) {
        Serial.print("!");  // 范围不足警告
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
    Serial.println(sampleCount);
    Serial.println();
    Serial.println("Results (min -> max):");

    const char* names[] = {"Thumb ", "Index ", "Middle", "Ring  ", "Pinky "};
    bool allGood = true;

    for (int i = 0; i < 5; i++) {
      int rawRange = tempMax[i] - tempMin[i];

      // 检查是否有有效数据
      if (tempMin[i] >= tempMax[i] || rawRange < 100) {
        Serial.print("  ");
        Serial.print(names[i]);
        Serial.println(": NO DATA - move finger more!");
        // 保持默认值
        minVal[i] = 0;
        maxVal[i] = 4095;
        allGood = false;
        continue;
      }

      // 添加边距扩展（避免边缘死区）
      int margin = rawRange * MARGIN_PERCENT / 100;
      minVal[i] = max(0, tempMin[i] - margin);
      maxVal[i] = min(4095, tempMax[i] + margin);

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
      Serial.println("Try 'CAL' again with more finger movement.");
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
