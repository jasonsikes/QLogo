# QLogo Parser

This directory contains the implementations of the parser classes for QLogo.

## Files

- `treeifyer.cpp`: Contains the implementation of the `Treeifier` class, which is responsible for treeifying lists into Abstract Syntax Tree (AST) nodes.
- `runparser.cpp`: Implements the `Runparser` class, which is responsible for parsing a QLogo word or list into a list of tokens.

## Key Components

### Treeifier

The `Treeifier` class is the main component responsible for treeifying QLogo lists into an Abstract Syntax Tree (AST). It handles:

- Treeifying expressions
- Treeifying procedures
- Converting lists to AST nodes

### Runparser

The `Runparser` class is the lexical analyzer, responsible for the initial tokenization of QLogo code. It:

- Breaks down words and lists into individual tokens
- Handles special characters and quoted words
- Prepares the input for treeification by the `Treeifier` class

The Runparser class is so named because its functionality is available to the user via the `RUNPARSE` command.

## Processing Process

1. The `Runparser` tokenizes the input, converting words and lists into a list of tokens.
2. The `Treeifier` then takes these tokens and treeifies them into an Abstract Syntax Tree (AST).
3. The AST is used by other parts of QLogo for execution and interpretation of the code.

For more detailed information on the implementation, please refer to the comments in the source files.
