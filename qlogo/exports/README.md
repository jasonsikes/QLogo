# Exports

This directory contains the exported C functions that provide the interface between compiled QLogo code and the runtime system. Compiled Logo calls these functions across the C ABI to reach Qt and the rest of the QLogo runtime.

The files here are split along the same UCBLogo primitive categories as the compiler sources in `qlogo/compiler/`. The corresponding declarations are in `include/workspace/exports.h`. That header is generated from `EXPORTC` lines in this directory by `util/update_exports_header.py`.

## Files

### exports.cpp

Contains runtime services used by the compiler core: converting between Logo datums and C types, looking up and assigning variables, constructing error objects, handling bad inputs when `ERRACT` is set, and allocating temporary memory.

### exports_arithmetic.cpp

Contains the implementations for `RANDOM`, `RERANDOM`, and `FORM`. Other arithmetic is compiled directly to LLVM and C math library calls.

### exports_communication.cpp

Contains the implementations for writing datums to the standard output device, used by `PRINT`, `TYPE`, and `SHOW`.

### exports_controlstructures.cpp

Contains the implementations for evaluation-stack control, procedure entry, `OUTPUT`/`STOP`, `GOTO`, `CATCH`/`THROW`, `PAUSE`/`CONTINUE`, `RUNRESULT`, `TEST`, and `APPLY`.

### exports_datastructureprimitives.cpp

Contains the implementations for constructing, selecting, mutating, and querying words, lists, and arrays, including `EQUALP`, `MEMBER`, `PARSE`, and `RUNPARSE`.

### exports_graphics.cpp

Contains the implementations for turtle motion, pen and screen control, mouse input, and saving or loading canvas images.

### exports_specialvariables.cpp

Contains the implementation for reading the `ERRACT` special variable.

### exports_workspacemanagement.cpp

Contains the implementations for defining a procedure from the read stream (`TO`) and marking a variable as local (`LOCAL`).
