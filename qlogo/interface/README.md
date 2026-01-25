# Interface

This directory contains the implementation of the interface components for QLogo. The interface is responsible for managing the input and output streams.

Since QLogo can be a console application or a GUI application, the interface classes are implemented in a way that is compatible with both.

## Files

### inputqueue.cpp

Contains the implementation of the `InputQueueThread` and `InputQueue` classes, which create a separate thread to wait for data and subsequently read data from the input queue. This is used by the GUI interface to handle asynchronous user input.

### logointerface.cpp

Contains the implementation of the `LogoInterface` class, which is the base class for all interfaces that handle user interaction. It provides a set of common methods that are used by all interfaces, including text-based input/output, signal handling, and basic graphics operations that can be overridden by subclasses.

### logointerfacegui.cpp

Contains the implementation of the `LogoInterfaceGUI` class, which extends `LogoInterface` and provides additional methods to handle GUI user interactions and graphics input/output. Since the process for user interaction is different for a GUI application, the `LogoInterfaceGUI` class adds a separate thread to wait for user input and a message queue to handle the input events.

### textstream.cpp

Contains the implementation of the `TextStream` class, which is used to read text from any kind of text stream. It provides methods to read lists, lines, words, and characters from the stream, with support for tokenization, comment removal, and special character processing.
