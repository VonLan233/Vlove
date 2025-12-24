"""
Audio Player for Vlove

Handles:
- Piano note playback (synthesized or samples)
- Gesture sound effects
- Pitch bending
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
        self.running = True
        self.lock = threading.Lock()
        self.active_notes = {}  # note -> True (正在播放的音符)
        self.play_thread = None

        if not HAS_AUDIO:
            return

        # Initialize sounddevice
        sd.default.samplerate = sample_rate
        sd.default.channels = 1

        # 启动持续播放线程
        self.play_thread = threading.Thread(target=self._continuous_play_loop)
        self.play_thread.daemon = True
        self.play_thread.start()

    def midi_to_freq(self, midi_note):
        """Convert MIDI note number to frequency"""
        return 440.0 * (2.0 ** ((midi_note - 69) / 12.0))

    def generate_tone(self, frequency, duration=0.5, volume=0.3):
        """Generate a sine wave tone"""
        if not HAS_AUDIO:
            return None

        t = np.linspace(0, duration, int(self.sample_rate * duration), False)

        # Generate sine wave with envelope
        wave = np.sin(2 * np.pi * frequency * t)

        # Add harmonics for richer sound
        wave += 0.5 * np.sin(4 * np.pi * frequency * t)  # 2nd harmonic
        wave += 0.25 * np.sin(6 * np.pi * frequency * t)  # 3rd harmonic

        # Apply ADSR envelope
        attack = int(0.01 * self.sample_rate)
        decay = int(0.1 * self.sample_rate)
        release = int(0.2 * self.sample_rate)

        envelope = np.ones(len(wave))
        # Attack
        envelope[:attack] = np.linspace(0, 1, attack)
        # Decay
        envelope[attack:attack + decay] = np.linspace(1, 0.7, decay)
        # Release
        envelope[-release:] = np.linspace(0.7, 0, release)

        wave = wave * envelope * volume

        return wave.astype(np.float32)

    def _continuous_play_loop(self):
        """持续播放活跃音符的循环"""
        while self.running:
            with self.lock:
                notes = list(self.active_notes.keys())

            if notes and HAS_AUDIO:
                try:
                    # 生成所有活跃音符的混合波形
                    duration = 0.15  # 短音符，循环播放
                    waves = []
                    for note in notes:
                        freq = self.midi_to_freq(note)
                        wave = self.generate_tone(freq, duration, 0.3 / max(len(notes), 1))
                        if wave is not None:
                            waves.append(wave)

                    if waves:
                        # 混合所有波形
                        min_len = min(len(w) for w in waves)
                        mixed = np.zeros(min_len, dtype=np.float32)
                        for w in waves:
                            mixed += w[:min_len]

                        sd.play(mixed, blocking=True)
                except Exception as e:
                    pass
            else:
                time.sleep(0.05)  # 没有音符时休眠

    def play_note(self, midi_note, velocity=100, duration=0.3):
        """开始播放一个音符（会持续播放直到stop_note）"""
        if not HAS_AUDIO:
            return

        with self.lock:
            self.active_notes[midi_note] = True
        print(f"  [Audio] Note {midi_note} ON - active: {list(self.active_notes.keys())}")

    def stop_note(self, midi_note):
        """停止播放一个音符"""
        with self.lock:
            if midi_note in self.active_notes:
                del self.active_notes[midi_note]
        print(f"  [Audio] Note {midi_note} OFF - active: {list(self.active_notes.keys())}")

    def pitch_bend(self, base_note, bend_value):
        """Apply pitch bend to a note
        bend_value: -8192 to 8191, where 0 is no bend
        """
        if not HAS_AUDIO:
            return

        # Convert bend to semitones (standard range is +/- 2 semitones)
        semitones = (bend_value / 8192) * 2
        bent_note = base_note + semitones
        frequency = self.midi_to_freq(bent_note)

        # Play the bent note briefly
        def play_thread():
            try:
                wave = self.generate_tone(frequency, 0.1, 0.2)
                if wave is not None:
                    sd.play(wave)
            except:
                pass

        thread = threading.Thread(target=play_thread)
        thread.daemon = True
        thread.start()

    def play_gesture(self, gesture_name):
        """Play a sound effect for a gesture"""
        if not HAS_AUDIO:
            return

        # Map gestures to sounds
        gesture_sounds = {
            'ThumbsUp': (72, 0.2),    # High C
            'ThumbsDown': (48, 0.3),  # Low C
            'Peace': (67, 0.2),       # G
            'Rock': (60, 0.3),        # C
            'OK': (64, 0.2),          # E
            'Fist': (55, 0.3),        # G
            'OpenHand': (72, 0.2),    # High C
            'Point': (69, 0.2),       # A
            'CallMe': (65, 0.2),      # F
            # Numbers
            '0': (60, 0.15),
            '1': (62, 0.15),
            '2': (64, 0.15),
            '3': (65, 0.15),
            '4': (67, 0.15),
            '5': (69, 0.15),
            '6': (71, 0.15),
            '7': (72, 0.15),
            '8': (74, 0.15),
            '9': (76, 0.15),
        }

        if gesture_name in gesture_sounds:
            note, duration = gesture_sounds[gesture_name]
            self.play_note(note, 100, duration)

    def play_chord(self, notes, velocity=100, duration=0.5):
        """Play multiple notes simultaneously"""
        if not HAS_AUDIO:
            return

        volume = (velocity / 127) * 0.2  # Lower volume for chords

        def play_thread():
            try:
                waves = []
                for note in notes:
                    freq = self.midi_to_freq(note)
                    wave = self.generate_tone(freq, duration, volume / len(notes))
                    if wave is not None:
                        waves.append(wave)

                if waves:
                    # Mix all waves
                    min_len = min(len(w) for w in waves)
                    mixed = np.zeros(min_len, dtype=np.float32)
                    for w in waves:
                        mixed += w[:min_len]

                    sd.play(mixed)
                    sd.wait()
            except Exception as e:
                pass

        thread = threading.Thread(target=play_thread)
        thread.daemon = True
        thread.start()

    def stop(self):
        """Stop all audio"""
        self.running = False
        if HAS_AUDIO:
            try:
                sd.stop()
            except:
                pass


# Simple test
if __name__ == '__main__':
    print("Testing audio player...")

    player = AudioPlayer()

    print("Playing C major scale...")
    notes = [60, 62, 64, 65, 67, 69, 71, 72]
    for note in notes:
        player.play_note(note, 100, 0.3)
        time.sleep(0.35)

    print("Playing C major chord...")
    player.play_chord([60, 64, 67], 100, 1.0)
    time.sleep(1.5)

    print("Done!")
    player.stop()
