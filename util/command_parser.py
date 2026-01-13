#! /usr/bin/env python3

"""
Shared parsing module for extracting command definitions from C++ source files.

This module provides functions to parse "// CMD" comments from compiler source files
and extract command metadata (name, arity, return types, method names).
"""

import os
import re
import sys


# ============================================================================
# Configuration
# ============================================================================

CPP_SOURCE_DIR = "qlogo/compiler"


# ============================================================================
# Parsing Functions
# ============================================================================

def is_command_entry(line):
    """Return True if the given line is a command entry."""
    return line.lstrip().startswith("// CMD")


def extract_command_elements(line):
    """
    Extract the elements of a command entry line.
    
    Returns: [NAME, min_params, default_params, max_params, return_type]
    The first two elements of the line ("// CMD") are discarded.
    """
    return line.strip().split()[2:]


def find_next_command(file):
    """Find the next command entry line in the file."""
    while True:
        line = file.readline()
        if not line:
            return ''
        if is_command_entry(line):
            return line


def is_compiler_define(line):
    """Return True if the given line is a Compiler method definition."""
    return line.lstrip().startswith("Value *Compiler::")


def method_name(line):
    """Return the name of the Compiler method given by line."""
    # Remove the part before "::"
    sans_prefix = line.strip().split("::")[1]
    # Return the part before the "("
    return sans_prefix.split("(")[0]


def parse_source_file(source_path):
    """
    Parse a source file and extract all command entries.
    
    Args:
        source_path: Path to the source .cpp file
    
    Returns:
        List of tuples: (entries, method_name)
        where entries is a list of [name, min, default, max, return_type] lists
        and method_name is the Compiler method name
    
    Raises:
        SystemExit: If a command entry is not followed by a method definition
    """
    results = []
    
    with open(source_path, 'r') as file:
        while True:
            line = find_next_command(file)
            if line == '':
                break
            
            # Collect all contiguous command entries
            entries = []
            while True:
                entries.append(extract_command_elements(line))
                line = file.readline()
                if not is_command_entry(line):
                    break
            
            # The current line should be the Compiler method definition
            if is_compiler_define(line):
                method = method_name(line)
                results.append((entries, method))
            else:
                print("ERROR!")
                print(f"Expected Compiler method definition after command entries.")
                print(f"File: {source_path}")
                print(f"Line: {line}")
                sys.exit(1)
    
    return results


def get_source_files(project_root):
    """Get sorted list of .cpp source files from the source directory."""
    source_dir = os.path.join(project_root, CPP_SOURCE_DIR)
    sources = [f for f in os.listdir(source_dir) if re.match(r'.*\.cpp', f)]
    sources.sort()
    return [os.path.join(source_dir, f) for f in sources]


def parse_all_sources(project_root):
    """
    Parse all source files and return all command definitions.
    
    Args:
        project_root: Root directory of the project
    
    Returns:
        List of tuples: (entries, method_name) from all source files
    """
    all_results = []
    source_files = get_source_files(project_root)
    
    for source_path in source_files:
        print(f"Reading '{source_path}'.")
        results = parse_source_file(source_path)
        print(f"Found {len(results)} entries")
        all_results.extend(results)
    
    return all_results


# ============================================================================
# Utility Functions
# ============================================================================

def get_project_root():
    """Get the project root directory (parent of util/)."""
    script_dir = os.path.dirname(os.path.abspath(__file__))
    return os.path.dirname(script_dir)


def string_to_enum(return_type_str):
    """Return the CPP enum representation of the output_type string."""
    retval = 'RequestReturn'
    s = return_type_str.upper()
    for c in 'RDBN':
        if c in s:
            retval += c
    return retval


def name_to_cpp_identifier(name):
    """Return the CPP identifier representation of the command name."""
    replacements = {
        '?': '_Q',
        '.': '_dot_',
        '-': '_m_',
        '+': '_p_',
        '*': '_star_',
        '/': '_slash_',
        '<': '_lt_',
        '>': '_gt_',
        '=': '_eq_'
    }
    for old, new in replacements.items():
        name = name.replace(old, new)
    return "cmdStr" + name
