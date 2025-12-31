"""
Audio Player for Vlove - Simplified version
"""

import math
import threading
import time

try:
    import numpy as np
    import sounddevice as sd
    HAS_AUDIO = True
except ImportError:
    HAS_AUDIO = False
    print("Warning: numpy/sounddevice not installed. Audio disabled.")
    print("Install with: pip install numpy sounddevice")


class AudioPlayer:
    def __init__(self, sample_rate=44100):
        self.sample_rate = sample_rate
        self.active_notes = set()
        self.lock = threading.Lock()

        if not HAS_AUDIO:
            return

        # Get device sample rate
        try:
            device_info = sd.query_devices(kind='output')
            self.sample_rate = int(device_info['default_samplerate'])
            print(f"Audio initialized: {self.sample_rate}Hz")
        except:
            print(f"Using default sample rate: {self.sample_rate}Hz")

    def midi_to_freq(self, midi_note):
        """Convert MIDI note number to frequency"""
        return 440.0 * (2.0 ** ((midi_note - 69) / 12.0))

    def generate_tone(self, frequency, duration=0.3, volume=0.3):
        """Generate a piano-like tone"""
        if not HAS_AUDIO:
            return None

        t = np.linspace(0, duration, int(self.sample_rate * duration), False)

        # Generate sine wave with harmonics for richer sound
        wave = np.sin(2 * np.pi * frequency * t)
        wave += 0.5 * np.sin(4 * np.pi * frequency * t)
        wave += 0.25 * np.sin(6 * np.pi * frequency * t)

        # ADSR envelope
        attack = int(0.02 * self.sample_rate)
        decay = int(0.1 * self.sample_rate)
        release = int(0.15 * self.sample_rate)

        envelope = np.ones(len(wave)) * 0.7
        if attack > 0:
            envelope[:attack] = np.linspace(0, 1, attack)
        if decay > 0 and attack + decay < len(envelope):
            envelope[attack:attack + decay] = np.linspace(1, 0.7, decay)
        if release > 0:
            envelope[-release:] = np.linspace(0.7, 0, release)

        wave = wave * envelope * volume
        return wave.astype(np.float32)

    def play_note(self, midi_note, velocity=100, duration=0.5):
        """Play a single note (non-blocking)"""
        if not HAS_AUDIO:
            return

        with self.lock:
            self.active_notes.add(midi_note)

        freq = self.midi_to_freq(midi_note)
        vol = (velocity / 127) * 0.4

        def _play():
            try:
                wave = self.generate_tone(freq, duration, vol)
                if wave is not None:
                    sd.play(wave, self.sample_rate)
                    sd.wait()
            except Exception as e:
                print(f"Play error: {e}")

        thread = threading.Thread(target=_play)
        thread.daemon = True
        thread.start()

        print(f"  [Audio] Note {midi_note} ON")

    def stop_note(self, midi_note):
        """Stop a note"""
        with self.lock:
            self.active_notes.discard(midi_note)
        print(f"  [Audio] Note {midi_note} OFF")

    def play_chord(self, notes, velocity=100, duration=0.8):
        """Play multiple notes as a chord"""
        if not HAS_AUDIO:
            return

        vol = (velocity / 127) * 0.3 / len(notes)

        def _play():
            try:
                waves = []
                for note in notes:
                    freq = self.midi_to_freq(note)
                    wave = self.generate_tone(freq, duration, vol)
                    if wave is not None:
                        waves.append(wave)

                if waves:
                    min_len = min(len(w) for w in waves)
                    mixed = np.zeros(min_len, dtype=np.float32)
                    for w in waves:
                        mixed += w[:min_len]
                    sd.play(mixed, self.sample_rate)
                    sd.wait()
            except Exception as e:
                print(f"Chord error: {e}")

        thread = threading.Thread(target=_play)
        thread.daemon = True
        thread.start()

        print(f"  [Audio] Chord {notes}")

    def play_gesture(self, gesture_name):
        """Play a sound effect for a gesture"""
        gesture_sounds = {
            'ThumbsUp': 72, 'Peace': 67, 'Rock': 60, 'OK': 64,
            'Fist': 55, 'OpenHand': 72, 'Point': 69, 'CallMe': 65,
            '0': 60, '1': 62, '2': 64, '3': 65, '4': 67,
            '5': 69, '6': 71, '7': 72, '8': 74, '9': 76,
        }
        if gesture_name in gesture_sounds:
            self.play_note(gesture_sounds[gesture_name], 100, 0.2)

    def pitch_bend(self, base_note, bend_value):
        """Apply pitch bend"""
        if not HAS_AUDIO:
            return
        semitones = (bend_value / 8192) * 2
        bent_note = base_note + semitones
        freq = self.midi_to_freq(bent_note)

        def _play():
            try:
                wave = self.generate_tone(freq, 0.1, 0.2)
                if wave is not None:
                    sd.play(wave, self.sample_rate)
            except:
                pass

        thread = threading.Thread(target=_play)
        thread.daemon = True
        thread.start()

    def stop(self):
        """Stop all audio"""
        if HAS_AUDIO:
            try:
                sd.stop()
            except:
                pass


# Test
if __name__ == '__main__':
    print("Testing audio player...")
    player = AudioPlayer()
    time.sleep(0.3)

    print("\nPlaying C major scale (C D E F G A B C)...")
    notes = [60, 62, 64, 65, 67, 69, 71, 72]
    for i, note in enumerate(notes):
        print(f"  Note {i+1}/8: {note}")
        player.play_note(note, 100, 0.4)
        time.sleep(0.5)

    time.sleep(0.5)
    print("\nPlaying C major chord (C E G)...")
    player.play_chord([60, 64, 67], 100, 1.0)
    time.sleep(1.5)

    print("\nDone!")
    player.stop()
