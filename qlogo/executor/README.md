# QLogo Executor

This directory contains the implementation of the Kernel class, which is the executor proper of the QLogo language. The Kernel class is responsible for executing various commands and operations in QLogo. The implementation is divided into several files, each handling different aspects of the execution process.

## File Structure

- `controlstructures.cpp`: Contains the implementations for operations involving control structure commands.
- `kernel.cpp`: Contains the kernel methods that support and maintain the state of QLogo execution.
- `workspacemanagement.cpp`: Contains the implementations for operations that manage the workspace, such as variables, procedures, and property lists.
- `datastructureprimitives.cpp`: Contains the implementations for operations that manipulate data structures.
- `graphics.cpp`: Contains the implementations for operations that manipulate the drawing canvas and the turtle.

## Command Documentation and Interface Structure

Embedded in the source code of each `Kernel::exc*` method is a comment block that contains two things: the documentation for the command or operation and the QLogo interface information, which includes the command name and its arity. The documentation for each method precedes the QLogo command implementation. Two python scripts are provided:

- `generate_help.py`: Generates the help documentation from the source code. The resulting text is accessable to the user via the `HELP` command.
- `generate_command_table.py`: Generates two things. The first is a header file that contains the command table, which is used to map command names to their corresponding QLogo interface information. The second is a source file that contains the method interfaces.

If a new command is created, then the command table and the method interface must be updated by running the `generate_command_table.py` script. If the new command includes help text, then the help documentation must be updated by running the `generate_help.py` script.

As an example, here is the file snippet for the `PRINT` command. There are three parts to the documentation and interface for each command.

1. The documentation, lines between the markers `/***DOC` and `COD***/`.
2. The QLogo command information, lines marked with `// CMD`.
3. The QLogo command implementation.

```cpp
/***DOC PRINT PR
PRINT thing
PR thing
(PRINT thing1 thing2 ...)
(PR thing1 thing2 ...)

    command.  Prints the input or inputs to the current write stream
    (initially the screen).  All the inputs are printed on a single
    line, separated by spaces, ending with a newline.  If an input is a
    list, square brackets are not printed around it, but brackets are
    printed around sublists.  Braces are always printed around arrays.

COD***/
// CMD PRINT 0 1 -1
// CMD PR 0 1 -1
DatumPtr Kernel::excPrint(DatumPtr node)
{
    // Implementation of the PRINT command
}
```

The documentation block begins with the `/***DOC` marker, followed by the command name(s) on the same line. The documentation ends with the `COD***/` marker. In this case, the command name is `PRINT` with a synonym, `PR`. If a command has multiple names, they must appear on the same line, separated by spaces.

Everything between the lines marked with `/***DOC` and `COD***/` is the documentation for the command. That documentation is entered into the SQLite database so it can be accessed via the `HELP` command.

Immediately after the help text block is the QLogo command information. The QLogo command information is marked by lines starting with `// CMD`. The command information lines must contain the command name, and its arity. The arity of QLogo commands is a list of three values (see "Arity", below). This information is used to map the command name to the corresponding QLogo command implementation. If a command has multiple names, they must appear in separate, consecutive lines.

The QLogo command information is used to generate the command table and the method declarations.

The command information (the lines starting with `// CMD`) must appear immediately before the QLogo command implementation. The help documentation can occur anywhere in the kernel source code as long as it is in the `executor` subdirectory, but for convenience, it is usually placed  before the QLogo command information.

## Arity

In QLogo, the arity of a command is a list of three values. The first value is the **minimum** number of arguments the command accepts. The second value is the **default** number of arguments the command accepts. The third value is the **maximum** number of arguments the command accepts.

In the `PRINT` example, the arity is `0 1 -1`. This means that the `PRINT` command can accept any number between 0 and infinity ("infinity" is indicated by the `-1`). The default number of arguments is 1.