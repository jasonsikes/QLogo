# Workspace

This directory contains the Supporting components of the QLogo workspace functionality.

## Files

### workspace.cpp

Defines the `Workspace` class, which is the superclass for classes that need QLogo language workspace functionality (Variables, PropertyLists, Procedures).
 It provides methods for burying, stepping, and tracing workspace items.

### callframe.h

Contains the definitions for `CallFrame` and `Evaluator` structures. These are for managing the execution stack and evaluation stack of QLogo procedures. The CallFrame class maintains the state of variables and their scope, and therefore is a subclass of Workspace.

### procedures.h

Defines the `Procedures` class, which is responsible for organizing all procedures in QLogo: primitives, user-defined, and library procedures. It provides methods for creating, defining, copying, and erasing procedures.

### propertylists.h

Defines the `PropertyLists` class, which manages property lists in QLogo.
