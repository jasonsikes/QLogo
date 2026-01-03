# Pipe Protocol Tests

This directory contains tests for the pipe protocol used by qlogo and Psychi.

## Overview

The `test_pipe.py` program tests qlogo by communicating via the same binary pipe protocol that Psychi uses. It reads YAML test scripts that specify:
- Messages to send to qlogo
- Expected messages to receive from qlogo
- Data validation for messages

## Requirements

- Python 3.6+
- PyQt6 or PySide6 (`pip install PyQt6`)
- PyYAML (`pip install pyyaml`)
- qlogo executable (built from the project)

## Usage

```bash
# Run all tests in the tests/ subdirectory
./test_pipe.py

# Run tests from a specific directory
./test_pipe.py tests/

# Specify qlogo executable path
./test_pipe.py --qlogo /path/to/qlogo tests/
```

## Test Script Format

Test scripts are YAML files with the following structure:

```yaml
name: Test Name
messages:
  # Expect the initial prompt request
  - expect:
      message: C_CONSOLE_REQUEST_LINE
      data:
        prompt: "? "  
  - send:
      message: C_CONSOLE_RAWLINE_READ
      data: "print 2 + 2"
  
  - expect:
      message: C_CONSOLE_PRINT_STRING
      data: "4"
  
  - expect:
      message: C_CONSOLE_REQUEST_LINE
      data:
        prompt: "? "
```

### Message Actions

- **`send`**: Send a message to qlogo
  - `message`: Message type name (e.g., `C_CONSOLE_RAWLINE_READ`)
  - `data`: Message data (varies by message type)

- **`expect`**: Expect a message from qlogo
  - `message`: Expected message type name
  - `data`: Expected message data (optional, for validation)
  - `timeout`: Timeout in seconds (default: 5.0)

- **`wait`**: Wait for a specified duration
  - `seconds`: Duration to wait

### Message Types

See `sharedconstants.h` for the complete list of message types. Common ones include:

- `W_INITIALIZE`: Initialization message (sent by qlogo on startup)
- `C_CONSOLE_PRINT_STRING`: Text output from qlogo
- `C_CONSOLE_REQUEST_LINE`: qlogo requests input
- `C_CONSOLE_RAWLINE_READ`: Send input line to qlogo
- `C_CONSOLE_CHAR_READ`: Send single character to qlogo
- `C_CANVAS_*`: Canvas/graphics related messages

### Data Types

Message data can be:
- **String**: `"hello"`
- **Number**: `42` or `3.14`
- **Boolean**: `true` or `false`
- **Color**: `{color: {r: 255, g: 0, b: 0, a: 255}}`
- **Point**: `{point: {x: 10.5, y: 20.3}}`
- **List**: `["item1", "item2"]`

## Example Test

See `tests/example.yaml` for a complete example test.

## Notes

- The test program automatically handles the `W_INITIALIZE` handshake that occurs when qlogo starts
- Tests should clean up by sending `W_CLOSE_PIPE` at the end (handled automatically)
- The program will search for qlogo in common locations if not specified
