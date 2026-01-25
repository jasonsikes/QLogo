# QLogo Parser

This directory contains the implementations of the parser classes for QLogo.

## Files

### treeifier.cpp

The `Treeifier` class is the component responsible for converting QLogo lists into Abstract Syntax Trees (AST).

### runparser.cpp

The `Runparser` class is the lexical analyzer, responsible for the initial tokenization of QLogo code. It is so named because its functionality is available to the user via the `RUNPARSE` command.

### op_strings.cpp and cmd_strings.cpp

These two files contain string constants used throughout QLogo. `cmd_strings.cpp` is generated using a Python script using data found in `compiler*.cpp` files.
