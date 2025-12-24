#pragma once

#include <Arduino.h>
#include "Config.h"

// 滤波配置
#define OVERSAMPLE_COUNT    4      // 单次读取时的过采样次数
#define FILTER_WINDOW_SIZE  5      // 移动中值滤波窗口大小
#define EMA_ALPHA           0.3f   // 指数移动平均系数 (0.1-0.5, 越大响应越快)
#define DEADZONE            15     // 死区阈值，忽略小于此值的变化

class AnalogFilter {
private:
  // 每个手指的滤波器状态
  int window[5][FILTER_WINDOW_SIZE];  // 中值滤波窗口
  int windowIndex[5];                  // 窗口当前写入位置
  float emaValue[5];                   // EMA 当前值
  int lastOutput[5];                   // 上次输出值（用于死区）
  bool initialized[5];                 // 是否已初始化

  // 引脚映射
  int pins[5];
  bool inverted[5];

public:
  AnalogFilter() {
    // 初始化引脚
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
    // 配置 ADC 分辨率和衰减（ESP32 特有）
    #ifdef ESP32
    analogReadResolution(12);  // 12 位分辨率
    analogSetAttenuation(ADC_11db);  // 全范围 0-3.3V
    #endif

    // 预热：读取几次初始化滤波器
    for (int i = 0; i < FILTER_WINDOW_SIZE * 2; i++) {
      int dummy[5];
      readFiltered(dummy);
      delay(5);
    }
  }

  // 读取单个引脚的原始值（带过采样）
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

  // 中值滤波：返回窗口中的中值
  int getMedian(int finger) {
    // 复制窗口数据并排序
    int sorted[FILTER_WINDOW_SIZE];
    for (int i = 0; i < FILTER_WINDOW_SIZE; i++) {
      sorted[i] = window[finger][i];
    }

    // 简单冒泡排序（窗口小，效率够用）
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

  // 读取所有手指的滤波值
  void readFiltered(int output[5]) {
    for (int i = 0; i < 5; i++) {
      // 1. 过采样读取原始值
      int raw = readRawOversampled(pins[i], inverted[i]);

      // 2. 更新中值滤波窗口
      window[i][windowIndex[i]] = raw;
      windowIndex[i] = (windowIndex[i] + 1) % FILTER_WINDOW_SIZE;

      // 3. 获取中值
      int median = getMedian(i);

      // 4. 应用指数移动平均
      if (!initialized[i]) {
        emaValue[i] = median;
        lastOutput[i] = median;
        initialized[i] = true;
      } else {
        emaValue[i] = EMA_ALPHA * median + (1.0f - EMA_ALPHA) * emaValue[i];
      }

      int filtered = (int)emaValue[i];

      // 5. 应用死区（减少抖动）
      if (abs(filtered - lastOutput[i]) > DEADZONE) {
        lastOutput[i] = filtered;
      }

      output[i] = lastOutput[i];
    }
  }

  // 读取原始值（无滤波，用于调试）
  void readRaw(int output[5]) {
    for (int i = 0; i < 5; i++) {
      output[i] = readRawOversampled(pins[i], inverted[i]);
    }
  }

  // 重置滤波器状态（校准后使用）
  void reset() {
    for (int i = 0; i < 5; i++) {
      initialized[i] = false;
      windowIndex[i] = 0;
    }
  }
};
