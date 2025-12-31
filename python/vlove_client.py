#!/usr/bin/env python3
"""
Vlove Client - Receives data from ESP32 and plays sounds

Usage:
    python vlove_client.py [port] [--bt]

Commands:
    python vlove_client.py                    # Auto-detect serial port
    python vlove_client.py /dev/cu.usbserial-110
    python vlove_client.py --bt               # Use Bluetooth
    python vlove_client.py --list             # List available ports
"""

import serial
import serial.tools.list_ports
import sys
import threading
import time
from audio_player import AudioPlayer

# ============ CONFIGURATION ============
DEFAULT_BAUD = 115200
BT_DEVICE_NAME = "vlove-left"

# Gesture names for display
GESTURE_NAMES = {
    0: "None",
    1: "0", 2: "1", 3: "2", 4: "3", 5: "4",
    6: "5", 7: "6", 8: "7", 9: "8", 10: "9",
    11: "👍 Thumbs Up",
    12: "✌️ Peace",
    13: "🤘 Rock",
    14: "👌 OK",
    15: "✊ Fist",
    16: "🖐️ Open Hand",
    17: "👆 Point",
    18: "🤙 Call Me",
    19: "🔫 Gun",
}


class VloveClient:
    def __init__(self, port=None, use_bluetooth=False):
        self.port = port
        self.use_bluetooth = use_bluetooth
        self.serial = None
        self.running = False
        self.audio = AudioPlayer()

    def find_usb_port(self):
        """Auto-detect USB serial port"""
        ports = serial.tools.list_ports.comports()
        for p in ports:
            desc = p.description.lower()
            device = p.device.lower()
            # Look for common ESP32 USB-UART chips
            if 'cp210' in desc or 'ch340' in desc or 'ftdi' in desc:
                return p.device
            if 'usbserial' in device or 'usbmodem' in device:
                return p.device
        return None

    def find_bluetooth_port(self):
        """Auto-detect Bluetooth serial port"""
        ports = serial.tools.list_ports.comports()
        bt_name = BT_DEVICE_NAME.lower()

        for p in ports:
            device = p.device.lower()
            desc = p.description.lower()

            # macOS: Bluetooth ports appear as /dev/cu.<device-name> or /dev/tty.<device-name>
            # Linux: Usually /dev/rfcomm0
            # Windows: COM ports with Bluetooth in description

            if bt_name in device or bt_name in desc:
                return p.device
            if 'bluetooth' in desc and bt_name in desc:
                return p.device

        # macOS specific: try common Bluetooth port patterns
        import os
        if sys.platform == 'darwin':
            possible_ports = [
                f'/dev/cu.{BT_DEVICE_NAME}',
                f'/dev/tty.{BT_DEVICE_NAME}',
                f'/dev/cu.{BT_DEVICE_NAME}-SerialPort',
                f'/dev/tty.{BT_DEVICE_NAME}-SerialPort',
            ]
            for port in possible_ports:
                if os.path.exists(port):
                    return port

        return None

    def list_ports(self):
        """List all available ports"""
        ports = serial.tools.list_ports.comports()
        print("\nAvailable ports:")
        print("-" * 60)

        usb_ports = []
        bt_ports = []
        other_ports = []

        for p in ports:
            device = p.device.lower()
            desc = p.description.lower()

            # Categorize ports
            if 'bluetooth' in desc or BT_DEVICE_NAME.lower() in device:
                bt_ports.append(p)
            elif 'usbserial' in device or 'cp210' in desc or 'ch340' in desc:
                usb_ports.append(p)
            else:
                other_ports.append(p)

        if usb_ports:
            print("\n[USB Serial]")
            for p in usb_ports:
                print(f"  {p.device}: {p.description}")

        if bt_ports:
            print("\n[Bluetooth]")
            for p in bt_ports:
                print(f"  {p.device}: {p.description}")

        if other_ports:
            print("\n[Other]")
            for p in other_ports:
                print(f"  {p.device}: {p.description}")

        if not ports:
            print("  No ports found")

        print("-" * 60)

    def connect(self):
        """Connect to ESP32 via USB or Bluetooth"""

        # If port is specified, use it directly
        if self.port:
            pass
        elif self.use_bluetooth:
            # Auto-detect Bluetooth port
            print(f"Searching for Bluetooth device '{BT_DEVICE_NAME}'...")
            self.port = self.find_bluetooth_port()
            if not self.port:
                print(f"\nError: Bluetooth device '{BT_DEVICE_NAME}' not found.")
                print("\nPlease ensure:")
                print(f"  1. ESP32 Bluetooth is enabled (send 'BT' command via USB first)")
                print(f"  2. Device '{BT_DEVICE_NAME}' is paired with your computer")
                print(f"  3. On macOS: System Preferences > Bluetooth > Connect")
                print("\nAvailable ports:")
                for p in serial.tools.list_ports.comports():
                    print(f"  {p.device}: {p.description}")
                return False
            print(f"Found Bluetooth port: {self.port}")
        else:
            # Auto-detect USB port
            self.port = self.find_usb_port()
            if not self.port:
                print("Error: Could not find ESP32 USB port.")
                print("Try --bt for Bluetooth or specify port manually.")
                self.list_ports()
                return False

        # Connect to the port
        try:
            self.serial = serial.Serial(self.port, DEFAULT_BAUD, timeout=0.1)
            conn_type = "Bluetooth" if self.use_bluetooth else "USB"
            print(f"Connected via {conn_type}: {self.port}")
            return True
        except serial.SerialException as e:
            print(f"Error connecting to {self.port}: {e}")
            if self.use_bluetooth:
                print("\nBluetooth troubleshooting:")
                print("  - Make sure device is paired and connected")
                print("  - On macOS, click 'Connect' in Bluetooth preferences")
                print("  - Try disconnecting and reconnecting")
            return False

    def start(self):
        """Start receiving data"""
        if not self.connect():
            return

        self.running = True

        print()
        print("=" * 50)
        conn_type = "BLUETOOTH" if self.use_bluetooth else "USB"
        print(f"   VLOVE CLIENT ({conn_type})")
        print("=" * 50)
        print()
        print("Listening for data...")
        print("Type commands and press Enter to send to ESP32")
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
            except EOFError:
                break
            except Exception:
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

        except serial.SerialException:
            print("\nConnection lost. Exiting...")
            self.running = False
        except Exception:
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

            # Gesture sounds disabled - uncomment to enable
            # self.audio.play_gesture(gesture_name)

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

        except Exception:
            pass


def print_help():
    """Print usage help"""
    print("""
Vlove Client - ESP32 Glove Data Receiver

Usage:
    python vlove_client.py [options] [port]

Options:
    --bt, -b        Connect via Bluetooth
    --list, -l      List available ports
    --help, -h      Show this help

Examples:
    python vlove_client.py                     # Auto-detect USB
    python vlove_client.py --bt                # Auto-detect Bluetooth
    python vlove_client.py /dev/cu.usbserial-110
    python vlove_client.py /dev/cu.vlove-left  # Specific BT port

Bluetooth Setup:
    1. Connect to ESP32 via USB first
    2. Send 'BT' command to enable Bluetooth
    3. Pair 'vlove-left' in your computer's Bluetooth settings
    4. Run: python vlove_client.py --bt
""")


def main():
    port = None
    use_bt = False
    list_only = False

    # Parse arguments
    for arg in sys.argv[1:]:
        if arg in ('--bt', '-b'):
            use_bt = True
        elif arg in ('--list', '-l'):
            list_only = True
        elif arg in ('--help', '-h'):
            print_help()
            return
        elif not arg.startswith('-'):
            port = arg

    # List ports only
    if list_only:
        client = VloveClient()
        client.list_ports()
        return

    # Start client
    client = VloveClient(port, use_bt)
    client.start()


if __name__ == '__main__':
    main()
