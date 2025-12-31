#pragma once

#include "Config.h"
#include <BluetoothSerial.h>

// Global mode variable
OperationMode currentMode = MODE_HOME;

class Communication {
private:
  BluetoothSerial btSerial;
  bool btEnabled = false;
  bool btConnected = false;

public:
  void begin() {
    // Bluetooth is optional, start when requested
    Serial.println("Communication initialized (Serial)");
    Serial.println("Type 'BT' to enable Bluetooth");
  }

  void toggleBluetooth() {
    if (!btEnabled) {
      Serial.println("Starting Bluetooth...");
      btSerial.begin(BT_DEVICE_NAME);
      btEnabled = true;
      Serial.print("Bluetooth enabled: ");
      Serial.println(BT_DEVICE_NAME);
    } else {
      Serial.println("Stopping Bluetooth...");
      btSerial.end();
      btEnabled = false;
      Serial.println("Bluetooth disabled");
    }
  }

  bool isBluetoothConnected() {
    if (!btEnabled) return false;
    return btSerial.hasClient();
  }

  // Send to active output (Serial always, BT if connected)
  void send(const char* data) {
    Serial.print(data);
    if (btEnabled && btSerial.hasClient()) {
      btSerial.print(data);
    }
  }

  void sendLine(const char* data) {
    Serial.println(data);
    if (btEnabled && btSerial.hasClient()) {
      btSerial.println(data);
    }
  }

  // Send gesture event
  // Format: G,<gesture_id>,<gesture_name>
  void sendGesture(int gestureId, const char* gestureName) {
    char buffer[64];
    sprintf(buffer, "G,%d,%s", gestureId, gestureName);
    sendLine(buffer);
  }

  // Send piano event
  // Format: P,<type>,<note>,<velocity>,<pitchbend>,<chord_notes...>
  void sendPianoEvent(PianoEvent& event) {
    char buffer[64];

    switch (event.type) {
      case PIANO_NOTE_ON:
        if (event.chordSize > 0) {
          // Chord
          sprintf(buffer, "P,ON,CHORD,");
          for (int i = 0; i < event.chordSize; i++) {
            char noteStr[8];
            sprintf(noteStr, "%d", event.chord[i]);
            strcat(buffer, noteStr);
            if (i < event.chordSize - 1) strcat(buffer, ",");
          }
        } else {
          // Single note
          sprintf(buffer, "P,ON,%d,%d", event.note, event.velocity);
        }
        break;

      case PIANO_NOTE_OFF:
        if (event.chordSize > 0) {
          sprintf(buffer, "P,OFF,CHORD,");
          for (int i = 0; i < event.chordSize; i++) {
            char noteStr[8];
            sprintf(noteStr, "%d", event.chord[i]);
            strcat(buffer, noteStr);
            if (i < event.chordSize - 1) strcat(buffer, ",");
          }
        } else {
          sprintf(buffer, "P,OFF,%d", event.note);
        }
        break;

      case PIANO_PITCH_BEND:
        sprintf(buffer, "P,BEND,%d,%d", event.note, event.pitchBend);
        break;

      default:
        return;
    }

    sendLine(buffer);
  }

  // Send raw data
  // Format: R,<raw0>,<raw1>,...,<mapped0>,<mapped1>,...
  void sendRawData(int raw[5], int mapped[5]) {
    char buffer[128];
    sprintf(buffer, "R,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
            raw[0], raw[1], raw[2], raw[3], raw[4],
            mapped[0], mapped[1], mapped[2], mapped[3], mapped[4]);
    sendLine(buffer);
  }

  // ============ OPENGLOVES PROTOCOL ============
  // Alpha encoding format for SteamVR OpenGloves driver
  // Format: A<thumb>B<index>C<middle>D<ring>E<pinky>\n
  // With IMU: A<thumb>B<index>C<middle>D<ring>E<pinky>(w|x|y|z)\n

  // Send finger data only (no IMU)
  void sendOpenGloves(int fingers[5]) {
    char buffer[64];
    sprintf(buffer, "A%dB%dC%dD%dE%d",
            fingers[0], fingers[1], fingers[2], fingers[3], fingers[4]);
    sendLine(buffer);
  }

  // Send finger data with IMU quaternion
  void sendOpenGlovesWithIMU(int fingers[5], float qw, float qx, float qy, float qz) {
    char buffer[128];
    // OpenGloves expects quaternion in parentheses, pipe-separated
    // Format: A<thumb>B<index>C<middle>D<ring>E<pinky>(qw|qx|qy|qz)
    sprintf(buffer, "A%dB%dC%dD%dE%d(%.4f|%.4f|%.4f|%.4f)",
            fingers[0], fingers[1], fingers[2], fingers[3], fingers[4],
            qw, qx, qy, qz);
    sendLine(buffer);
  }

  // Send full OpenGloves data with splay (optional)
  // F-J are splay values for each finger
  void sendOpenGlovesFull(int curl[5], int splay[5], float qw, float qx, float qy, float qz) {
    char buffer[160];
    sprintf(buffer, "A%dB%dC%dD%dE%dF%dG%dH%dI%dJ%d(%.4f|%.4f|%.4f|%.4f)",
            curl[0], curl[1], curl[2], curl[3], curl[4],
            splay[0], splay[1], splay[2], splay[3], splay[4],
            qw, qx, qy, qz);
    sendLine(buffer);
  }
};
