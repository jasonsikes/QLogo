#!/usr/bin/env python3
"""
Pipe Protocol Test Program for qlogo

This program tests qlogo by communicating via the pipe protocol used by Psychi.
It reads YAML test scripts that specify messages to send and expected responses.
"""

import argparse
import os
import sys
import subprocess
import struct
import yaml
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

try:
    from PyQt6.QtCore import QByteArray, QDataStream, QIODevice, QPointF
    from PyQt6.QtGui import QColor, QImage, QTransform
    PYQT_VERSION = 6
except ImportError:
    try:
        from PySide6.QtCore import QByteArray, QDataStream, QIODevice, QPointF
        from PySide6.QtGui import QColor, QImage, QTransform
        PYQT_VERSION = 6
    except ImportError:
        print("Error: PyQt6 or PySide6 is required for this test program.")
        print("Install with: pip install PyQt6")
        sys.exit(1)


# Message type constants (from sharedconstants.h)
W_ZERO = 0
W_INITIALIZE = 1
W_CLOSE_PIPE = 2
W_SET_SCREENMODE = 3
W_FILE_DIALOG_GET_PATH = 4

S_SYSTEM = 5
S_TOPLEVEL = 6
S_PAUSE = 7

C_CONSOLE_PRINT_STRING = 8
C_CONSOLE_REQUEST_LINE = 9
C_CONSOLE_REQUEST_CHAR = 10
C_CONSOLE_RAWLINE_READ = 11
C_CONSOLE_CHAR_READ = 12
C_CONSOLE_SET_FONT_NAME = 13
C_CONSOLE_SET_FONT_SIZE = 14
C_CONSOLE_BEGIN_EDIT_TEXT = 15
C_CONSOLE_END_EDIT_TEXT = 16
C_CONSOLE_TEXT_CURSOR_POS = 17
C_CONSOLE_SET_TEXT_CURSOR_POS = 18
C_CONSOLE_SET_CURSOR_MODE = 19
C_CONSOLE_SET_TEXT_COLOR = 20
C_CONSOLE_CLEAR_SCREEN_TEXT = 21

C_CANVAS_UPDATE_TURTLE_POS = 22
C_CANVAS_EMIT_VERTEX = 23
C_CANVAS_SET_FOREGROUND_COLOR = 24
C_CANVAS_SET_BACKGROUND_COLOR = 25
C_CANVAS_SET_BACKGROUND_IMAGE = 26
C_CANVAS_BEGIN_POLYGON = 27
C_CANVAS_END_POLYGON = 28
C_CANVAS_SET_TURTLE_IS_VISIBLE = 29
C_CANVAS_DRAW_LABEL = 30
C_CANVAS_DRAW_ARC = 31
C_CANVAS_CLEAR_SCREEN = 32
C_CANVAS_SETBOUNDS = 33
C_CANVAS_SET_IS_BOUNDED = 34
C_CANVAS_SET_PENSIZE = 35
C_CANVAS_SET_PENUPDOWN = 36
C_CANVAS_SET_FONT_NAME = 37
C_CANVAS_SET_FONT_SIZE = 38
C_CANVAS_GET_IMAGE = 39
C_CANVAS_GET_SVG = 40
C_CANVAS_MOUSE_BUTTON_DOWN = 41
C_CANVAS_MOUSE_MOVED = 42
C_CANVAS_MOUSE_BUTTON_UP = 43
C_CANVAS_SET_PENMODE = 44

# Screen mode constants
INIT_SCREEN_MODE = 0
TEXT_SCREEN_MODE = 1
FULL_SCREEN_MODE = 2
SPLIT_SCREEN_MODE = 3

# Pen mode constants
PEN_MODE_PAINT = 0
PEN_MODE_ERASE = 1
PEN_MODE_REVERSE = 2

# Message name to value mapping
MESSAGE_NAMES = {
    'W_ZERO': W_ZERO,
    'W_INITIALIZE': W_INITIALIZE,
    'W_CLOSE_PIPE': W_CLOSE_PIPE,
    'W_SET_SCREENMODE': W_SET_SCREENMODE,
    'W_FILE_DIALOG_GET_PATH': W_FILE_DIALOG_GET_PATH,
    'S_SYSTEM': S_SYSTEM,
    'S_TOPLEVEL': S_TOPLEVEL,
    'S_PAUSE': S_PAUSE,
    'C_CONSOLE_PRINT_STRING': C_CONSOLE_PRINT_STRING,
    'C_CONSOLE_REQUEST_LINE': C_CONSOLE_REQUEST_LINE,
    'C_CONSOLE_REQUEST_CHAR': C_CONSOLE_REQUEST_CHAR,
    'C_CONSOLE_RAWLINE_READ': C_CONSOLE_RAWLINE_READ,
    'C_CONSOLE_CHAR_READ': C_CONSOLE_CHAR_READ,
    'C_CONSOLE_SET_FONT_NAME': C_CONSOLE_SET_FONT_NAME,
    'C_CONSOLE_SET_FONT_SIZE': C_CONSOLE_SET_FONT_SIZE,
    'C_CONSOLE_BEGIN_EDIT_TEXT': C_CONSOLE_BEGIN_EDIT_TEXT,
    'C_CONSOLE_END_EDIT_TEXT': C_CONSOLE_END_EDIT_TEXT,
    'C_CONSOLE_TEXT_CURSOR_POS': C_CONSOLE_TEXT_CURSOR_POS,
    'C_CONSOLE_SET_TEXT_CURSOR_POS': C_CONSOLE_SET_TEXT_CURSOR_POS,
    'C_CONSOLE_SET_CURSOR_MODE': C_CONSOLE_SET_CURSOR_MODE,
    'C_CONSOLE_SET_TEXT_COLOR': C_CONSOLE_SET_TEXT_COLOR,
    'C_CONSOLE_CLEAR_SCREEN_TEXT': C_CONSOLE_CLEAR_SCREEN_TEXT,
    'C_CANVAS_UPDATE_TURTLE_POS': C_CANVAS_UPDATE_TURTLE_POS,
    'C_CANVAS_EMIT_VERTEX': C_CANVAS_EMIT_VERTEX,
    'C_CANVAS_SET_FOREGROUND_COLOR': C_CANVAS_SET_FOREGROUND_COLOR,
    'C_CANVAS_SET_BACKGROUND_COLOR': C_CANVAS_SET_BACKGROUND_COLOR,
    'C_CANVAS_SET_BACKGROUND_IMAGE': C_CANVAS_SET_BACKGROUND_IMAGE,
    'C_CANVAS_BEGIN_POLYGON': C_CANVAS_BEGIN_POLYGON,
    'C_CANVAS_END_POLYGON': C_CANVAS_END_POLYGON,
    'C_CANVAS_SET_TURTLE_IS_VISIBLE': C_CANVAS_SET_TURTLE_IS_VISIBLE,
    'C_CANVAS_DRAW_LABEL': C_CANVAS_DRAW_LABEL,
    'C_CANVAS_DRAW_ARC': C_CANVAS_DRAW_ARC,
    'C_CANVAS_CLEAR_SCREEN': C_CANVAS_CLEAR_SCREEN,
    'C_CANVAS_SETBOUNDS': C_CANVAS_SETBOUNDS,
    'C_CANVAS_SET_IS_BOUNDED': C_CANVAS_SET_IS_BOUNDED,
    'C_CANVAS_SET_PENSIZE': C_CANVAS_SET_PENSIZE,
    'C_CANVAS_SET_PENUPDOWN': C_CANVAS_SET_PENUPDOWN,
    'C_CANVAS_SET_FONT_NAME': C_CANVAS_SET_FONT_NAME,
    'C_CANVAS_SET_FONT_SIZE': C_CANVAS_SET_FONT_SIZE,
    'C_CANVAS_GET_IMAGE': C_CANVAS_GET_IMAGE,
    'C_CANVAS_GET_SVG': C_CANVAS_GET_SVG,
    'C_CANVAS_MOUSE_BUTTON_DOWN': C_CANVAS_MOUSE_BUTTON_DOWN,
    'C_CANVAS_MOUSE_MOVED': C_CANVAS_MOUSE_MOVED,
    'C_CANVAS_MOUSE_BUTTON_UP': C_CANVAS_MOUSE_BUTTON_UP,
    'C_CANVAS_SET_PENMODE': C_CANVAS_SET_PENMODE,
}


class PipeProtocol:
    """Handles communication with qlogo via the pipe protocol."""
    
    def __init__(self, process: subprocess.Popen):
        self.process = process
        self.read_buffer = bytearray()
        self.read_buffer_len = 0
    
    def send_message(self, message_type: int, *args) -> None:
        """Send a message to qlogo."""
        buffer = QByteArray()
        stream = QDataStream(buffer, QIODevice.OpenModeFlag.WriteOnly)
        stream.writeUInt8(message_type)
        
        # Serialize arguments based on type
        for arg in args:
            if isinstance(arg, str):
                stream.writeQString(arg)
            elif isinstance(arg, int):
                stream.writeInt32(arg)
            elif isinstance(arg, float):
                stream.writeDouble(arg)
            elif isinstance(arg, bool):
                stream.writeBool(arg)
            elif isinstance(arg, dict):
                # Handle QColor
                if 'r' in arg and 'g' in arg and 'b' in arg:
                    color = QColor(arg.get('r', 0), arg.get('g', 0), arg.get('b', 0), arg.get('a', 255))
                    stream << color
                # Handle QPointF
                elif 'x' in arg and 'y' in arg:
                    point = QPointF(arg['x'], arg['y'])
                    stream << point
                else:
                    raise ValueError(f"Unknown dict type: {arg}")
            elif isinstance(arg, list):
                # Handle QStringList
                stream.writeInt32(len(arg))
                for item in arg:
                    stream.writeQString(str(item))
            else:
                raise ValueError(f"Unsupported argument type: {type(arg)}")
        
        # Prepend length (qint64 = 8 bytes)
        data_len = buffer.size()
        length_bytes = struct.pack('<q', data_len)  # Little-endian qint64
        
        # Write length + data
        self.process.stdin.write(length_bytes + buffer.data())
        self.process.stdin.flush()
    
    def read_message(self, timeout: float = 5.0) -> Optional[Tuple[int, QByteArray]]:
        """Read a message from qlogo. Returns (message_type, data_buffer) or None."""
        import select
        
        # Read length prefix (8 bytes for qint64)
        if len(self.read_buffer) == 0:
            # Need to read length
            length_bytes = self._read_bytes(8, timeout)
            if length_bytes is None:
                return None
            self.read_buffer_len = struct.unpack('<q', length_bytes)[0]
            self.read_buffer = bytearray()
        
        # Read message data
        remaining = self.read_buffer_len - len(self.read_buffer)
        if remaining > 0:
            data = self._read_bytes(remaining, timeout)
            if data is None:
                return None
            self.read_buffer.extend(data)
        
        # Check if we have complete message
        if len(self.read_buffer) < self.read_buffer_len:
            return None  # Still waiting for more data
        
        # Parse message
        buffer = QByteArray(bytes(self.read_buffer))
        stream = QDataStream(buffer, QIODevice.OpenModeFlag.ReadOnly)
        message_type = stream.readUInt8()
        
        # Reset for next message
        self.read_buffer = bytearray()
        self.read_buffer_len = 0
        
        return (message_type, buffer)
    
    def _read_bytes(self, count: int, timeout: float) -> Optional[bytes]:
        """Read exactly count bytes from stdout."""
        import select
        import time
        
        data = bytearray()
        start_time = time.time()
        
        while len(data) < count:
            elapsed = time.time() - start_time
            if elapsed > timeout:
                return None
            
            # Check if data is available
            if sys.platform != 'win32':
                remaining_timeout = timeout - elapsed
                ready, _, _ = select.select([self.process.stdout], [], [], remaining_timeout)
                if not ready:
                    return None
            
            chunk = self.process.stdout.read(count - len(data))
            if not chunk:
                if len(data) == 0:
                    return None
                time.sleep(0.01)  # Small delay before retry
                continue
            
            data.extend(chunk)
        
        return bytes(data)


class TestRunner:
    """Runs tests from YAML files."""
    
    def __init__(self, qlogo_path: str):
        self.qlogo_path = qlogo_path
        self.failed_tests = []
        self.passed_tests = []
    
    def find_qlogo(self) -> Optional[str]:
        """Find the qlogo executable."""
        # Try multiple locations
        candidates = [
            self.qlogo_path,
            os.path.join(os.path.dirname(__file__), '..', '..', 'qlogo', 'qlogo'),
            os.path.join(os.path.dirname(__file__), '..', '..', 'build', 'qlogo', 'qlogo'),
            'qlogo',  # In PATH
        ]
        
        for candidate in candidates:
            if os.path.isfile(candidate) and os.access(candidate, os.X_OK):
                return candidate
        
        return None
    
    def parse_data(self, data: Any, message_type: int) -> List[Any]:
        """Parse YAML data into arguments for the message."""
        if data is None:
            return []
        
        if isinstance(data, (str, int, float, bool)):
            return [data]
        
        if isinstance(data, list):
            return data
        
        if isinstance(data, dict):
            # Handle special types
            if 'color' in data:
                color = data['color']
                return [{'r': color.get('r', 0), 'g': color.get('g', 0), 
                        'b': color.get('b', 0), 'a': color.get('a', 255)}]
            elif 'point' in data:
                point = data['point']
                return [{'x': point.get('x', 0), 'y': point.get('y', 0)}]
            else:
                # Assume it's a direct mapping
                return [data]
        
        return [data]
    
    def compare_message_data(self, expected: Any, actual_buffer: QByteArray, message_type: int) -> Tuple[bool, str]:
        """Compare expected data with actual message data."""
        if expected is None:
            return (True, "")
        
        stream = QDataStream(actual_buffer, QIODevice.OpenModeFlag.ReadOnly)
        stream.readUInt8()  # Skip header
        
        # Parse based on message type
        try:
            if message_type == W_INITIALIZE:
                # W_INITIALIZE from qlogo has no data (it's a request)
                # W_INITIALIZE response has QStringList, QString, double
                if stream.atEnd():
                    # Request message (no data)
                    return (True, "")
                else:
                    # Response message
                    font_count = stream.readInt32()
                    font_names = [stream.readQString() for _ in range(font_count)]
                    text_font_name = stream.readQString()
                    text_font_size = stream.readDouble()
                    # Compare with expected
                    if isinstance(expected, dict):
                        if expected.get('text_font_name') and expected.get('text_font_name') != text_font_name:
                            return (False, f"Font name mismatch: expected {expected.get('text_font_name')}, got {text_font_name}")
                        if expected.get('text_font_size') and abs(expected.get('text_font_size') - text_font_size) > 0.001:
                            return (False, f"Font size mismatch: expected {expected.get('text_font_size')}, got {text_font_size}")
            
            elif message_type == C_CONSOLE_PRINT_STRING:
                text = stream.readQString()
                if isinstance(expected, str):
                    # Strip trailing newlines from actual text for comparison
                    # (qlogo typically includes a newline at the end of print output)
                    text_stripped = text.rstrip('\n\r')
                    expected_stripped = expected.rstrip('\n\r')
                    if expected_stripped != text_stripped:
                        return (False, f"String mismatch: expected '{expected}' (stripped: '{expected_stripped}'), got '{text}' (stripped: '{text_stripped}')")
            
            elif message_type == C_CONSOLE_REQUEST_LINE:
                prompt = stream.readQString()
                if isinstance(expected, dict) and expected.get('prompt') and expected.get('prompt') != prompt:
                    return (False, f"Prompt mismatch: expected '{expected.get('prompt')}', got '{prompt}'")
            
            elif message_type == C_CONSOLE_TEXT_CURSOR_POS:
                row = stream.readInt32()
                col = stream.readInt32()
                if isinstance(expected, dict):
                    if expected.get('row') is not None and expected.get('row') != row:
                        return (False, f"Row mismatch: expected {expected.get('row')}, got {row}")
                    if expected.get('col') is not None and expected.get('col') != col:
                        return (False, f"Col mismatch: expected {expected.get('col')}, got {col}")
            
            elif message_type == C_CONSOLE_CHAR_READ:
                char = stream.readQChar()
                if isinstance(expected, str) and len(expected) > 0 and expected[0] != char:
                    return (False, f"Char mismatch: expected '{expected[0]}', got '{char}'")
            
            # Add more message type handlers as needed
            
        except Exception as e:
            return (False, f"Error parsing message: {e}")
        
        return (True, "")
    
    def run_test(self, test_file: Path) -> bool:
        """Run a single test file. Returns True if passed."""
        print(f"Running: {test_file.name}")
        
        with open(test_file, 'r') as f:
            test_data = yaml.safe_load(f)
        
        test_name = test_data.get('name', test_file.stem)
        messages = test_data.get('messages', [])
        
        # Find and start qlogo
        qlogo_exe = self.find_qlogo()
        if not qlogo_exe:
            print(f"  ERROR: Could not find qlogo executable")
            return False
        
        try:
            # Start qlogo process
            process = subprocess.Popen(
                [qlogo_exe, '--Psychi'],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                bufsize=0  # Unbuffered
            )
            
            protocol = PipeProtocol(process)
            
            # Handle initialization
            # qlogo sends W_INITIALIZE first (but with no data - it's a request)
            init_msg = protocol.read_message(timeout=2.0)
            if init_msg is None:
                print(f"  ERROR: Timeout waiting for initialization")
                process.terminate()
                return False
            
            msg_type, msg_data = init_msg
            if msg_type != W_INITIALIZE:
                print(f"  ERROR: Expected W_INITIALIZE, got {msg_type}")
                process.terminate()
                return False
            
            # Send initialization response with font information
            # Get system fonts (simplified - use a default font)
            import platform
            if platform.system() == 'Linux':
                default_font = 'Monospace'
            elif platform.system() == 'Darwin':
                default_font = 'Monaco'
            else:
                default_font = 'Courier New'
            
            # Create a simple font list (in real Psychi, this comes from QFontDatabase)
            font_names = [default_font, 'Arial', 'Times New Roman']
            text_font_name = default_font
            text_font_size = 12.0
            
            # Send response (QStringList, QString, double)
            protocol.send_message(W_INITIALIZE, font_names, text_font_name, text_font_size)
            
            # Process test messages
            for i, msg in enumerate(messages):
                action = list(msg.keys())[0]
                msg_spec = msg[action]
                
                if action == 'send':
                    msg_name = msg_spec.get('message')
                    if msg_name not in MESSAGE_NAMES:
                        print(f"  ERROR: Unknown message type: {msg_name}")
                        process.terminate()
                        return False
                    
                    msg_type = MESSAGE_NAMES[msg_name]
                    data = msg_spec.get('data')
                    args = self.parse_data(data, msg_type)
                    protocol.send_message(msg_type, *args)
                
                elif action == 'expect':
                    msg_name = msg_spec.get('message')
                    if msg_name not in MESSAGE_NAMES:
                        print(f"  ERROR: Unknown message type: {msg_name}")
                        process.terminate()
                        return False
                    
                    expected_type = MESSAGE_NAMES[msg_name]
                    expected_data = msg_spec.get('data')
                    timeout = msg_spec.get('timeout', 5.0)
                    
                    # Read message
                    received = protocol.read_message(timeout=timeout)
                    if received is None:
                        print(f"  ERROR: Timeout waiting for {msg_name}")
                        process.terminate()
                        return False
                    
                    actual_type, actual_data = received
                    if actual_type != expected_type:
                        # Get message name for error reporting
                        type_to_name = {v: k for k, v in MESSAGE_NAMES.items()}
                        got_name = type_to_name.get(actual_type, f"unknown({actual_type})")
                        print(f"  ERROR: Expected {msg_name} ({expected_type}), got {got_name} ({actual_type})")
                        process.terminate()
                        return False
                    
                    # Compare data if specified
                    if expected_data is not None:
                        match, error = self.compare_message_data(expected_data, actual_data, actual_type)
                        if not match:
                            print(f"  ERROR: {error}")
                            process.terminate()
                            return False
                
                elif action == 'wait':
                    # Wait for a specified time
                    import time
                    time.sleep(msg_spec.get('seconds', 0.1))
                
                elif action == 'close_write_pipe':
                    # Close the write pipe (stdin)
                    process.stdin.close()
                
                else:
                    print(f"  ERROR: Unknown action: {action}")
                    process.terminate()
                    return False
            
            # Wait for process to terminate
            process.wait(timeout=2.0)
            
            print(f"  PASSED")
            return True
            
        except subprocess.TimeoutExpired:
            print(f"  ERROR: Process did not terminate in time")
            process.kill()
            return False
        except Exception as e:
            print(f"  ERROR: {e}")
            if 'process' in locals():
                process.terminate()
            return False
    
    def run_tests(self, test_dir: Path) -> None:
        """Run all tests in a directory."""
        yaml_files = sorted(test_dir.glob('*.yaml')) + sorted(test_dir.glob('*.yml'))
        
        if not yaml_files:
            print(f"No YAML test files found in {test_dir}")
            return
        
        print(f"Found {len(yaml_files)} test file(s)")
        print()
        
        for test_file in yaml_files:
            if self.run_test(test_file):
                self.passed_tests.append(test_file)
            else:
                self.failed_tests.append(test_file)
            print()
        
        # Print summary
        print("=" * 60)
        print(f"Total: {len(yaml_files)} tests")
        print(f"Passed: {len(self.passed_tests)}")
        print(f"Failed: {len(self.failed_tests)}")
        
        if self.failed_tests:
            print("\nFailed tests:")
            for test in self.failed_tests:
                print(f"  - {test.name}")
            print("=" * 60)
            sys.exit(1)
        else:
            print("=" * 60)
            sys.exit(0)


def main():
    parser = argparse.ArgumentParser(description='Test qlogo pipe protocol')
    parser.add_argument('--qlogo', type=str, default='',
                       help='Path to qlogo executable (default: auto-detect)')
    parser.add_argument('tests', nargs='?', type=str, default='tests',
                       help='Directory containing YAML test files (default: tests/)')
    
    args = parser.parse_args()
    
    test_dir = Path(__file__).parent / args.tests
    if not test_dir.exists():
        test_dir = Path(args.tests)
    
    if not test_dir.exists():
        print(f"Error: Test directory not found: {test_dir}")
        sys.exit(1)
    
    runner = TestRunner(args.qlogo)
    runner.run_tests(test_dir)


if __name__ == '__main__':
    main()
