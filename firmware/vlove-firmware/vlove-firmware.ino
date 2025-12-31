/**
 * Vlove - VR Glove with Gesture Recognition and Air Piano
 *
 * Features:
 * - Finger tracking with calibration
 * - Static gesture recognition (numbers 0-9, common gestures)
 * - Air piano (single notes, pitch control, chords)
 * - USB Serial and Bluetooth communication
 * - OpenGloves protocol for SteamVR integration
 * - IMU support for hand orientation tracking
 */

#include "src/Config.h"
#include "src/Calibration.h"
#include "src/GestureRecognizer.h"
#include "src/AirPiano.h"
#include "src/Communication.h"
#include "src/AnalogFilter.h"

#ifdef ENABLE_IMU
#include "src/IMU.h"
#endif

// Global objects
Calibration calibration;
GestureRecognizer gestureRecognizer;
AirPiano airPiano;
Communication comm;
AnalogFilter analogFilter;

#ifdef ENABLE_IMU
IMU imu;
bool imuEnabled = false;
#endif

// Finger data
int rawFingers[5];
int mappedFingers[5];

// Timing
unsigned long lastOutputTime = 0;

void setup() {
  // Initialize serial for debugging (always on)
  Serial.begin(BAUD_RATE);
  delay(1000);

  // Print banner
  Serial.println();
  Serial.println("****************************************");
  Serial.println("*            VLOVE v2.0               *");
  Serial.println("*   Gesture Recognition & Air Piano   *");
  Serial.println("*     + OpenGloves + IMU Support      *");
  Serial.println("****************************************");
  Serial.println();

  // Initialize communication
  comm.begin();

  // Initialize analog filter
  analogFilter.begin();
  Serial.println("Analog filter initialized.");

  // Initialize IMU
  #ifdef ENABLE_IMU
  Serial.println("Initializing IMU...");
  imuEnabled = imu.begin(PIN_IMU_SDA, PIN_IMU_SCL);
  if (imuEnabled) {
    Serial.println("IMU ready. Calibrating gyro...");
    imu.calibrate(200);
  }
  #endif

  // Initialize gesture recognizer
  gestureRecognizer.begin();
  Serial.println("Gesture recognizer initialized (static + dynamic).");

  // Initialize calibration
  calibration.begin();

  // Check for saved calibration
  if (calibration.hasValidCalibration) {
    Serial.println("Loaded calibration from EEPROM.");
    Serial.println();
    printHelp();
  } else {
    Serial.println("No calibration found. Starting calibration...");
    calibration.startCalibration();
  }
}

void loop() {
  // Handle serial commands
  handleCommands();

  // Update IMU
  #ifdef ENABLE_IMU
  if (imuEnabled) {
    imu.update();
  }
  #endif

  // Read finger values with filtering
  analogFilter.readFiltered(rawFingers);

  // Update calibration if active
  if (calibration.isCalibrating) {
    calibration.update(rawFingers);

    // Print calibration status
    static unsigned long lastCalibPrint = 0;
    if (millis() - lastCalibPrint >= 300) {
      lastCalibPrint = millis();
      calibration.printStatus(rawFingers);
    }
    return;  // Don't process gestures during calibration
  }

  // Map finger values using calibration
  for (int i = 0; i < 5; i++) {
    mappedFingers[i] = calibration.mapValue(i, rawFingers[i]);
  }

  // Process based on current mode
  switch (currentMode) {
    case MODE_HOME:
      // Paused - do nothing, wait for commands
      break;
    case MODE_GESTURE:
      processGestureMode();
      break;
    case MODE_PIANO_SINGLE:
    case MODE_PIANO_PITCH:
    case MODE_PIANO_CHORD:
      processPianoMode();
      break;
    case MODE_RAW:
      processRawMode();
      break;
    case MODE_OPENGLOVES:
      processOpenGlovesMode();
      break;
  }

  delay(LOOP_DELAY_MS);
}

// Debug flag for gesture recognition
bool gestureDebug = false;

// Gesture display interval (ms)
#define GESTURE_DISPLAY_INTERVAL 200

void processGestureMode() {
  static GestureId lastSentGesture = GESTURE_NONE;
  static unsigned long lastDisplayTime = 0;

  // Use extended recognition for static + dynamic gestures
  GestureResult result = gestureRecognizer.recognizeEx(mappedFingers, LOOP_DELAY_MS);

  // Display gesture every GESTURE_DISPLAY_INTERVAL ms
  if (millis() - lastDisplayTime >= GESTURE_DISPLAY_INTERVAL) {
    lastDisplayTime = millis();

    // Debug: show finger values
    if (gestureDebug) {
      Serial.print("Fingers[0-255]: ");
      for (int i = 0; i < 5; i++) {
        uint8_t normalized = (uint8_t)((long)mappedFingers[i] * 255 / 4095);
        Serial.print(normalized);
        Serial.print(" ");
      }
      Serial.print(" -> ");
    }

    // Always show current gesture
    if (result.staticGesture != GESTURE_NONE) {
      Serial.print("Gesture: ");
      Serial.print(gestureRecognizer.getGestureName(result.staticGesture));
      Serial.print(" (");
      Serial.print(result.confidence);
      Serial.println("%)");

      // Send to comm if changed
      if (result.staticGesture != lastSentGesture) {
        comm.sendGesture(result.staticGesture, gestureRecognizer.getGestureName(result.staticGesture));
        lastSentGesture = result.staticGesture;
      }
    } else {
      Serial.println("Gesture: None");
    }
  }

  // Handle dynamic gestures (always report when detected)
  if (result.isNewDynamic && result.dynamicGesture != GESTURE_NONE) {
    comm.sendGesture(result.dynamicGesture, gestureRecognizer.getGestureName(result.dynamicGesture));
    Serial.print("Dynamic: ");
    Serial.println(gestureRecognizer.getGestureName(result.dynamicGesture));
  }
}

void processPianoMode() {
  PianoEvent event = airPiano.process(mappedFingers, currentMode);

  if (event.hasEvent) {
    comm.sendPianoEvent(event);

    Serial.print("Piano: ");
    if (event.type == PIANO_NOTE_ON) {
      Serial.print("ON  ");
    } else {
      Serial.print("OFF ");
    }
    Serial.print("note=");
    Serial.print(event.note);
    Serial.print(" vel=");
    Serial.println(event.velocity);
  }
}

void processRawMode() {
  static unsigned long lastRawPrint = 0;
  if (millis() - lastRawPrint >= 100) {
    lastRawPrint = millis();
    comm.sendRawData(rawFingers, mappedFingers);
  }
}

void processOpenGlovesMode() {
  // Send at ~100Hz for smooth tracking
  #ifdef ENABLE_IMU
  if (imuEnabled) {
    Quaternion q = imu.getQuaternion();
    comm.sendOpenGlovesWithIMU(mappedFingers, q.w, q.x, q.y, q.z);
  } else {
    comm.sendOpenGloves(mappedFingers);
  }
  #else
  comm.sendOpenGloves(mappedFingers);
  #endif
}

void handleCommands() {
  static String cmdBuffer = "";

  while (Serial.available()) {
    char c = Serial.read();

    if (c == '\n' || c == '\r') {
      if (cmdBuffer.length() > 0) {
        processCommand(cmdBuffer);
        cmdBuffer = "";
      }
    } else {
      cmdBuffer += c;
    }
  }
}

void processCommand(String cmd) {
  cmd.trim();
  cmd.toUpperCase();

  if (cmd == "CAL") {
    calibration.startCalibration();
  }
  else if (cmd == "STOP" || cmd == "DONE") {
    if (calibration.isCalibrating) {
      calibration.stopCalibration();
      printHelp();
    }
  }
  else if (cmd == "CLEAR") {
    calibration.clearEEPROM();
    Serial.println("EEPROM cleared.");
    calibration.startCalibration();
  }
  else if (cmd == "DEBUG" || cmd == "D") {
    gestureDebug = !gestureDebug;
    Serial.print("Gesture debug: ");
    Serial.println(gestureDebug ? "ON" : "OFF");
  }
  else if (cmd == "HOME" || cmd == "MENU" || cmd == "M") {
    currentMode = MODE_HOME;
    Serial.println("Mode: HOME (Paused)");
    printHelp();
  }
  else if (cmd == "GESTURE" || cmd == "G") {
    currentMode = MODE_GESTURE;
    Serial.println("Mode: GESTURE RECOGNITION");
  }
  else if (cmd == "PIANO1" || cmd == "P1") {
    currentMode = MODE_PIANO_SINGLE;
    Serial.println("Mode: PIANO - Single Notes (one finger = one note)");
  }
  else if (cmd == "PIANO2" || cmd == "P2") {
    currentMode = MODE_PIANO_PITCH;
    Serial.println("Mode: PIANO - Pitch Control (bend = pitch)");
  }
  else if (cmd == "PIANO3" || cmd == "P3") {
    currentMode = MODE_PIANO_CHORD;
    Serial.println("Mode: PIANO - Chord Mode");
  }
  else if (cmd == "RAW" || cmd == "R") {
    currentMode = MODE_RAW;
    Serial.println("Mode: RAW DATA");
  }
  else if (cmd == "VR" || cmd == "OPENGLOVES" || cmd == "OG") {
    currentMode = MODE_OPENGLOVES;
    Serial.println("Mode: OPENGLOVES (SteamVR)");
    #ifdef ENABLE_IMU
    if (imuEnabled) {
      Serial.println("  IMU: Enabled - sending orientation data");
    } else {
      Serial.println("  IMU: Not available - fingers only");
    }
    #else
    Serial.println("  IMU: Disabled in Config.h");
    #endif
  }
  else if (cmd == "IMU") {
    #ifdef ENABLE_IMU
    if (imuEnabled) {
      imu.printData();
    } else {
      Serial.println("IMU not initialized. Check wiring.");
    }
    #else
    Serial.println("IMU disabled in Config.h");
    #endif
  }
  else if (cmd == "IMUCAL") {
    #ifdef ENABLE_IMU
    if (imuEnabled) {
      Serial.println("Calibrating IMU... Keep device still!");
      imu.calibrate(500);
    }
    #endif
  }
  else if (cmd == "BT") {
    comm.toggleBluetooth();
  }
  else if (cmd == "HELP" || cmd == "H" || cmd == "?") {
    printHelp();
  }
  else if (calibration.isCalibrating) {
    // Any other input stops calibration
    calibration.stopCalibration();
    printHelp();
  }
  else {
    Serial.print("Unknown command: ");
    Serial.println(cmd);
    Serial.println("Type 'HELP' for commands.");
  }
}

void printHelp() {
  Serial.println();
  Serial.println("============ COMMANDS ============");
  Serial.println("CAL      - Start calibration");
  Serial.println("CLEAR    - Clear EEPROM & recalibrate");
  Serial.println();
  Serial.println("--- Modes ---");
  Serial.println("M/HOME   - Main menu (pause)");
  Serial.println("G        - Gesture recognition mode");
  Serial.println("P1       - Piano: Single notes");
  Serial.println("P2       - Piano: Pitch control");
  Serial.println("P3       - Piano: Chord mode");
  Serial.println("R        - Raw data mode");
  Serial.println("VR       - OpenGloves mode (SteamVR)");
  Serial.println();
  Serial.println("--- Hardware ---");
  Serial.println("BT       - Toggle Bluetooth");
  #ifdef ENABLE_IMU
  Serial.println("IMU      - Show IMU data");
  Serial.println("IMUCAL   - Calibrate IMU");
  Serial.print("IMU Status: ");
  Serial.println(imuEnabled ? "OK" : "Not found");
  #endif
  Serial.println();
  Serial.println("HELP     - Show this help");
  Serial.println("==================================");
  Serial.println();
}
