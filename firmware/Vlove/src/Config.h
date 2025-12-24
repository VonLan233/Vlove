#pragma once

#include <Arduino.h>

// ============ FEATURE TOGGLES ============
// Comment out to disable features
// #define ENABLE_IMU           // MPU6050 IMU support (需要外接MPU6050模块)
#define ENABLE_OPENGLOVES       // OpenGloves protocol for SteamVR

// ============ PIN CONFIGURATION ============
// ESP32 DOIT V1 pins

// Finger sensors (ADC pins)
#define PIN_THUMB   36
#define PIN_INDEX   39
#define PIN_MIDDLE  34
#define PIN_RING    35
#define PIN_PINKY   32

// Optional: LED for status
#define PIN_LED     2

// IMU (I2C pins)
#define PIN_IMU_SDA   21
#define PIN_IMU_SCL   22

// ============ COMMUNICATION ============
#define BAUD_RATE         115200
#define BT_DEVICE_NAME    "Vlove"

// Communication modes
#define COMM_SERIAL       0
#define COMM_BLUETOOTH    1

// ============ TIMING ============
#define LOOP_DELAY_MS     10

// ============ ADC ============
#define ANALOG_MAX        4095

// ============ SENSOR INVERSION ============
// Set to true if potentiometer polarity is reversed
#define INVERT_THUMB   false
#define INVERT_INDEX   true
#define INVERT_MIDDLE  false
#define INVERT_RING    true
#define INVERT_PINKY   false

// ============ OPENGLOVES CONFIG ============
// Alpha encoding character mapping
// A-E: finger curl (0-4095 mapped to 0-4095)
// F-J: finger splay (optional)
// Parentheses contain quaternion for hand rotation
#define OPENGLOVES_CURL_MIN    0
#define OPENGLOVES_CURL_MAX    4095

// ============ MODES ============
enum OperationMode {
  MODE_GESTURE = 0,
  MODE_PIANO_SINGLE,
  MODE_PIANO_PITCH,
  MODE_PIANO_CHORD,
  MODE_RAW,
  MODE_OPENGLOVES      // OpenGloves protocol for SteamVR
};

// Current mode (global)
extern OperationMode currentMode;

// ============ GESTURES ============
// Static gesture IDs
enum GestureID {
  GESTURE_NONE = 0,

  // Numbers 0-9
  GESTURE_NUM_0,
  GESTURE_NUM_1,
  GESTURE_NUM_2,
  GESTURE_NUM_3,
  GESTURE_NUM_4,
  GESTURE_NUM_5,
  GESTURE_NUM_6,
  GESTURE_NUM_7,
  GESTURE_NUM_8,
  GESTURE_NUM_9,

  // Common gestures
  GESTURE_THUMBS_UP,    // 👍
  GESTURE_THUMBS_DOWN,  // 👎
  GESTURE_PEACE,        // ✌️
  GESTURE_ROCK,         // 🤘
  GESTURE_OK,           // 👌
  GESTURE_FIST,         // ✊
  GESTURE_OPEN_HAND,    // 🖐️
  GESTURE_POINT,        // 👆
  GESTURE_CALL,         // 🤙

  GESTURE_COUNT
};

// ============ PIANO ============
// Piano event types
enum PianoEventType {
  PIANO_NOTE_OFF = 0,
  PIANO_NOTE_ON,
  PIANO_PITCH_BEND,
  PIANO_CHORD
};

// Piano event structure
struct PianoEvent {
  bool hasEvent;
  PianoEventType type;
  uint8_t note;       // MIDI note number (0-127)
  uint8_t velocity;   // 0-127
  int16_t pitchBend;  // -8192 to 8191
  uint8_t chord[5];   // Up to 5 notes in a chord
  uint8_t chordSize;
};

// Base notes for each finger (C major scale starting from middle C)
#define NOTE_THUMB   60  // C4
#define NOTE_INDEX   62  // D4
#define NOTE_MIDDLE  64  // E4
#define NOTE_RING    65  // F4
#define NOTE_PINKY   67  // G4

// ============ CALIBRATION ============
#define EEPROM_SIZE           512
#define EEPROM_CAL_MAGIC_ADDR 0x00
#define EEPROM_CAL_DATA_ADDR  0x01
#define EEPROM_CAL_MAGIC      0xCA

// ============ GESTURE THRESHOLDS ============
// Finger position thresholds (0-4095 scale after calibration)
#define FINGER_OPEN_THRESHOLD    3000  // Above this = finger open
#define FINGER_CLOSED_THRESHOLD  1000  // Below this = finger closed
#define FINGER_HALF_THRESHOLD    2000  // Middle position

// Gesture stability (number of consistent readings)
#define GESTURE_DEBOUNCE_COUNT   3
