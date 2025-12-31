#pragma once

#include "Config.h"

class AirPiano {
private:
  // Base MIDI notes for each finger
  const uint8_t baseNotes[5] = {NOTE_THUMB, NOTE_INDEX, NOTE_MIDDLE, NOTE_RING, NOTE_PINKY};

  // Chord definitions (MIDI note offsets from root)
  // Major chord: root, major 3rd (+4), perfect 5th (+7)
  // Minor chord: root, minor 3rd (+3), perfect 5th (+7)

  // Track finger states for note on/off
  bool fingerActive[5] = {false, false, false, false, false};

  // Threshold for triggering a note (high value = finger bent = note on)
  const int NOTE_ON_THRESHOLD = 1500;   // Above this = note on
  const int NOTE_OFF_THRESHOLD = 1000;  // Below this = note off

  // For pitch bend mode
  int lastPitchBend = 0;

  // For chord mode
  uint8_t lastChord[5] = {0, 0, 0, 0, 0};
  uint8_t lastChordSize = 0;

public:
  PianoEvent process(int fingers[5], OperationMode mode) {
    PianoEvent event;
    event.hasEvent = false;
    event.type = PIANO_NOTE_OFF;
    event.note = 0;
    event.velocity = 100;
    event.pitchBend = 0;
    event.chordSize = 0;

    switch (mode) {
      case MODE_PIANO_SINGLE:
        return processSingleNote(fingers);

      case MODE_PIANO_PITCH:
        return processPitchBend(fingers);

      case MODE_PIANO_CHORD:
        return processChord(fingers);

      default:
        return event;
    }
  }

private:
  // Mode 1: Each finger triggers its own note
  PianoEvent processSingleNote(int fingers[5]) {
    PianoEvent event;
    event.hasEvent = false;
    event.type = PIANO_NOTE_OFF;
    event.velocity = 100;

    for (int i = 0; i < 5; i++) {
      bool shouldBeActive = fingers[i] > NOTE_ON_THRESHOLD;    // Finger bent = note on
      bool shouldBeInactive = fingers[i] < NOTE_OFF_THRESHOLD; // Finger extended = note off

      // Note ON: finger just closed
      if (shouldBeActive && !fingerActive[i]) {
        fingerActive[i] = true;
        event.hasEvent = true;
        event.type = PIANO_NOTE_ON;
        event.note = baseNotes[i];
        // Velocity based on how far bent (higher = louder)
        event.velocity = map(fingers[i], NOTE_ON_THRESHOLD, ANALOG_MAX, 64, 127);
        return event;
      }

      // Note OFF: finger just opened
      if (shouldBeInactive && fingerActive[i]) {
        fingerActive[i] = false;
        event.hasEvent = true;
        event.type = PIANO_NOTE_OFF;
        event.note = baseNotes[i];
        event.velocity = 0;
        return event;
      }
    }

    return event;
  }

  // Mode 2: Finger bend controls pitch
  PianoEvent processPitchBend(int fingers[5]) {
    PianoEvent event;
    event.hasEvent = false;

    // Use index finger position to control pitch
    // Map 0-4095 to MIDI pitch bend range (-8192 to 8191)
    int pitchBend = map(fingers[1], 0, ANALOG_MAX, -8192, 8191);

    // Use middle finger to trigger note on/off (bent = active)
    bool noteActive = fingers[2] > NOTE_ON_THRESHOLD;

    // Note state change
    if (noteActive != fingerActive[2]) {
      fingerActive[2] = noteActive;
      event.hasEvent = true;
      event.type = noteActive ? PIANO_NOTE_ON : PIANO_NOTE_OFF;
      event.note = NOTE_MIDDLE;  // Base note C4
      event.velocity = noteActive ? 100 : 0;
      event.pitchBend = pitchBend;
      return event;
    }

    // Pitch bend change (only when note is active)
    if (fingerActive[2] && abs(pitchBend - lastPitchBend) > 100) {
      lastPitchBend = pitchBend;
      event.hasEvent = true;
      event.type = PIANO_PITCH_BEND;
      event.pitchBend = pitchBend;
      event.note = NOTE_MIDDLE;
      return event;
    }

    return event;
  }

  // Mode 3: Finger combinations create chords
  PianoEvent processChord(int fingers[5]) {
    PianoEvent event;
    event.hasEvent = false;
    event.type = PIANO_CHORD;
    event.chordSize = 0;

    // Determine which fingers are active (bent = active)
    bool active[5];
    for (int i = 0; i < 5; i++) {
      active[i] = fingers[i] > NOTE_ON_THRESHOLD;
    }

    // Build chord based on active fingers
    uint8_t chord[5];
    uint8_t chordSize = 0;

    // Each finger adds its note to the chord
    for (int i = 0; i < 5; i++) {
      if (active[i]) {
        chord[chordSize++] = baseNotes[i];
      }
    }

    // Check if chord changed
    bool chordChanged = (chordSize != lastChordSize);
    if (!chordChanged) {
      for (int i = 0; i < chordSize; i++) {
        if (chord[i] != lastChord[i]) {
          chordChanged = true;
          break;
        }
      }
    }

    if (chordChanged) {
      // Send note off for old chord
      if (lastChordSize > 0) {
        event.hasEvent = true;
        event.type = PIANO_NOTE_OFF;
        memcpy(event.chord, lastChord, lastChordSize);
        event.chordSize = lastChordSize;
      }

      // Update last chord
      memcpy(lastChord, chord, chordSize);
      lastChordSize = chordSize;

      // Send note on for new chord
      if (chordSize > 0) {
        event.hasEvent = true;
        event.type = PIANO_NOTE_ON;
        memcpy(event.chord, chord, chordSize);
        event.chordSize = chordSize;
        event.velocity = 100;
      }
    }

    return event;
  }

public:
  // Get note name for display
  const char* getNoteName(uint8_t note) {
    static const char* names[] = {
      "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };
    int octave = (note / 12) - 1;
    int noteIndex = note % 12;

    static char buffer[8];
    sprintf(buffer, "%s%d", names[noteIndex], octave);
    return buffer;
  }
};
