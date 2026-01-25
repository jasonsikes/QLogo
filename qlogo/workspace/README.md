# Workspace

This directory contains the supporting components of the QLogo workspace functionality.

## Files

### callframe.cpp

Contains the implementations for `CallFrame` and `Evaluator` structures. These are for managing the QLogo variables and evaluation stack for QLogo procedures.

### exports.cpp

Contains the exported C functions that provide the interface between the compiled QLogo code and the runtime system. These functions handle data type conversions, error handling, control flow, list operations, graphics operations, and other runtime services.

### kernel.cpp

Contains the implementation of the `Kernel` class, which is the main executor of the QLogo language. It maintains the state of execution of the QLogo code, including the read-eval-print loop, file I/O, and special variables.

### library.cpp

Contains the implementation of the `Library` and `Help` classes, which provide access to the QLogo standard library and help system. Both classes use SQLite database connections to store and retrieve library procedure text and help documentation.

### procedures.cpp

Contains the implementation of the `Procedures` class, which is responsible for organizing all procedures in QLogo: primitives, user-defined, and library procedures. It provides methods for creating, defining, copying, and erasing procedures.

### propertylists.cpp

Contains the implementation of the `PropertyLists` class, which manages property lists in QLogo.

### turtle.cpp

Contains the implementation of the `Turtle` class, which maintains the turtle state, including position, orientation, pen state, and visibility. The turtle's position and orientation are represented by a transformation matrix, and the class provides methods for moving, rotating, and drawing with the turtle.

### visited.cpp

Contains the implementation of the `VisitedSet` and `VisitedMap` classes, which are used to track visited nodes during graph traversal to prevent cycles when comparing Datum objects.
