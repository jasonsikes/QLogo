#! /usr/bin/env python3

"""
Generate cpp header and implementation files for the QLogo command string constants
and primitive mappings.

This is a convenience wrapper that calls both:
- generate_string_constants.py
- generate_primitive_mappings.py

Usage:
    generate_command_table.py
"""

import os
import sys
import subprocess


def main():
    """Run both generation scripts."""
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    scripts = [
        'generate_string_constants.py',
        'generate_primitive_mappings.py'
    ]
    
    for script in scripts:
        script_path = f"{script_dir}/{script}"
        print(f"\n{'='*60}")
        print(f"Running {script}")
        print(f"{'='*60}\n")
        
        result = subprocess.run([sys.executable, script_path], cwd=script_dir)
        
        if result.returncode != 0:
            print(f"\nError: {script} failed with exit code {result.returncode}")
            sys.exit(result.returncode)
    
    print(f"\n{'='*60}")
    print("All generation scripts completed successfully!")
    print(f"{'='*60}\n")


if __name__ == "__main__":
    main()
