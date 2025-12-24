#!/usr/bin/env python3
"""
Vlove Client - Receives data from ESP32 and plays sounds

Usage:
    python vlove_client.py [port] [--bt]

Commands:
    python vlove_client.py                    # Auto-detect serial port
    python vlove_client.py /dev/cu.usbserial-110
    python vlove_client.py --bt               # Use Bluetooth
"""

import serial
import serial.tools.list_ports
import sys
import threading
import time
from audio_player import AudioPlayer

# ============ CONFIGURATION ============
DEFAULT_BAUD = 115200
BT_DEVICE_NAME = "Vlove"

# Gesture names for display
GESTURE_NAMES = {
    0: "None",
    1: "0", 2: "1", 3: "2", 4: "3", 5: "4",
    6: "5", 7: "6", 8: "7", 9: "8", 10: "9",
    11: "👍 Thumbs Up",
    12: "👎 Thumbs Down",
    13: "✌️ Peace",
    14: "🤘 Rock",
    15: "👌 OK",
    16: "✊ Fist",
    17: "🖐️ Open Hand",
    18: "👆 Point",
    19: "🤙 Call Me",
}


class VloveClient:
    def __init__(self, port=None, use_bluetooth=False):
        self.port = port
        self.use_bluetooth = use_bluetooth
        self.serial = None
        self.running = False
        self.audio = AudioPlayer()

    def find_port(self):
        """Auto-detect serial port"""
        ports = serial.tools.list_ports.comports()
        for p in ports:
            # Look for common ESP32 identifiers
            if 'usbserial' in p.device.lower() or 'cp210' in p.description.lower():
                return p.device
            if 'ch340' in p.description.lower():
                return p.device
        return None

    def connect(self):
        """Connect to ESP32"""
        if self.use_bluetooth:
            print("Bluetooth mode not implemented in this version.")
            print("Please use USB serial.")
            return False

        if not self.port:
            self.port = self.find_port()
            if not self.port:
                print("Error: Could not find ESP32. Please specify port.")
                print("Available ports:")
                for p in serial.tools.list_ports.comports():
                    print(f"  {p.device}: {p.description}")
                return False

        try:
            self.serial = serial.Serial(self.port, DEFAULT_BAUD, timeout=0.1)
            print(f"Connected to {self.port}")
            return True
        except Exception as e:
            print(f"Error connecting to {self.port}: {e}")
            return False

    def start(self):
        """Start receiving data"""
        if not self.connect():
            return

        self.running = True

        print()
        print("=" * 50)
        print("   VLOVE CLIENT")
        print("=" * 50)
        print()
        print("Listening for data...")
        print("Press Ctrl+C to exit")
        print()

        # Start input thread for sending commands
        input_thread = threading.Thread(target=self.input_handler)
        input_thread.daemon = True
        input_thread.start()

        # Main loop
        try:
            while self.running:
                self.process_serial()
                time.sleep(0.01)
        except KeyboardInterrupt:
            print("\nExiting...")
        finally:
            self.running = False
            if self.serial:
                self.serial.close()
            self.audio.stop()

    def input_handler(self):
        """Handle keyboard input for sending commands"""
        while self.running:
            try:
                cmd = input()
                if self.serial and self.serial.is_open:
                    self.serial.write((cmd + '\n').encode())
            except:
                pass

    def process_serial(self):
        """Process incoming serial data"""
        if not self.serial or not self.serial.is_open:
            return

        try:
            line = self.serial.readline().decode('utf-8', errors='ignore').strip()
            if not line:
                return

            # Parse data based on prefix
            if line.startswith('G,'):
                self.handle_gesture(line)
            elif line.startswith('P,'):
                self.handle_piano(line)
            elif line.startswith('R,'):
                self.handle_raw(line)
            else:
                # Pass through other messages (calibration, help, etc.)
                print(line)

        except Exception as e:
            pass

    def handle_gesture(self, line):
        """Handle gesture event: G,<id>,<name>"""
        try:
            parts = line.split(',')
            gesture_id = int(parts[1])
            gesture_name = parts[2] if len(parts) > 2 else ""

            # Get display name
            display_name = GESTURE_NAMES.get(gesture_id, gesture_name)

            print(f"[GESTURE] {display_name}")

            # Play gesture sound
            self.audio.play_gesture(gesture_name)

        except Exception as e:
            print(f"Error parsing gesture: {e}")

    def handle_piano(self, line):
        """Handle piano event: P,<type>,<note>,<velocity>,..."""
        try:
            parts = line.split(',')
            event_type = parts[1]

            if event_type == 'ON':
                if parts[2] == 'CHORD':
                    # Chord: P,ON,CHORD,60,64,67
                    notes = [int(n) for n in parts[3:]]
                    print(f"[PIANO] Chord ON: {notes}")
                    for note in notes:
                        self.audio.play_note(note)
                else:
                    # Single note: P,ON,60,100
                    note = int(parts[2])
                    velocity = int(parts[3]) if len(parts) > 3 else 100
                    print(f"[PIANO] Note ON: {note} (vel={velocity})")
                    self.audio.play_note(note, velocity)

            elif event_type == 'OFF':
                if parts[2] == 'CHORD':
                    notes = [int(n) for n in parts[3:]]
                    print(f"[PIANO] Chord OFF: {notes}")
                    for note in notes:
                        self.audio.stop_note(note)
                else:
                    note = int(parts[2])
                    print(f"[PIANO] Note OFF: {note}")
                    self.audio.stop_note(note)

            elif event_type == 'BEND':
                note = int(parts[2])
                bend = int(parts[3])
                print(f"[PIANO] Pitch Bend: {bend}")
                self.audio.pitch_bend(note, bend)

        except Exception as e:
            print(f"Error parsing piano: {e}")

    def handle_raw(self, line):
        """Handle raw data: R,raw0,...,raw4,mapped0,...,mapped4"""
        try:
            parts = line.split(',')
            raw = [int(parts[i]) for i in range(1, 6)]
            mapped = [int(parts[i]) for i in range(6, 11)]

            # Simple visualization
            bar = ""
            for i, val in enumerate(mapped):
                level = int(val / 4095 * 10)
                bar += f" {'█' * level}{'░' * (10 - level)} "

            print(f"\r{bar}", end='')

        except Exception as e:
            pass


def main():
    port = None
    use_bt = False

    # Parse arguments
    for arg in sys.argv[1:]:
        if arg == '--bt' or arg == '-b':
            use_bt = True
        elif not arg.startswith('-'):
            port = arg

    client = VloveClient(port, use_bt)
    client.start()


if __name__ == '__main__':
    main()
