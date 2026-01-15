# QLogo Interface

This directory contains the implementation of the interface components for QLogo. The interface is responsible for managing the input and output streams, as well as handling the main control flow of the application.

Since QLogo can be a console application or a GUI application, the interface classes are implemented in a way that is compatible with both.

The base class `LogoInterface` provides the main control flow and functionality for simple text input/output.

For a GUI application, the `LogoInterfaceGUI` class extends `LogoInterface` and provides additional methods to handle GUI user interactions and graphics input/output.

Since the process for user interaction is different for a GUI application, the `LogoInterfaceGUI` class uses a separate thread to wait for user input and a message queue to handle the input events.

The `TextStream` class is used to read text from any kind of text stream. It provides methods to read lists, lines, words, and characters from the stream, as well as handling special cases like escaped characters and vertical bars.
