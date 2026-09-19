# Exports

This directory contains the exported C functions that provide the interface between compiled QLogo code and the runtime system. Compiled Logo calls these functions across the C ABI to reach Qt and the rest of the QLogo runtime.

The files here are split along the same UCBLogo primitive categories as the compiler sources in `qlogo/compiler/`. The corresponding declarations are in `include/workspace/exports.h`. That header is generated from `EXPORTC` lines in this directory by `util/update_exports_header.py`.

## Files

### exports.cpp

Contains runtime services shared by all compiled code: converting between Logo values and C types, looking up and storing variables, constructing errors, and allocating temporary memory.

### exports_arithmetic.cpp

Contains arithmetic operations, including random number generation and formatting numbers as Logo words.

### exports_communication.cpp

Contains input and output to the terminal and to files, including converting Logo values to text and writing them to the current output stream.

### exports_controlstructures.cpp

Contains program flow control: running lists, entering procedures, applying templates, catching errors, and non-local exits.

### exports_datastructureprimitives.cpp

Contains constructors, selectors, mutators, and predicates for words, lists, and arrays, including comparison and parsing text into Logo values.

### exports_graphics.cpp

Contains turtle graphics and canvas operations: motion and heading, pen and screen, pointing-device input, and picture files.

### exports_specialvariables.cpp

Contains access to Logo special variables, which are names owned by the interpreter rather than by a procedure.

### exports_workspacemanagement.cpp

Contains workspace operations: defining procedures and controlling the scope of names.
