# Exports

This directory contains the exported C functions that provide the interface between compiled QLogo code and the runtime system. These functions handle data type conversions, error handling, control flow, list operations, graphics operations, and other runtime services.

The corresponding declarations are in `include/workspace/exports.h`. That header is generated from files in this directory by `util/update_exports_header.py`.

## Files

### exports.cpp

Contains the C library routines used by the QLogo compiler. Compiled Logo calls these functions across the C ABI to reach Qt and the rest of the QLogo runtime.
