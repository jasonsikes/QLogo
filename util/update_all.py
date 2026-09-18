#! /usr/bin/env python3

"""
Run all of the update scripts in the util directory. Use this after making changes to the
command strings, primitives, compiler, or workspace. This will update the header files
in the include and qlogo directories.
"""

import os
import subprocess
import sys

scripts = [
    "generate_string_constants.py",
    "generate_primitive_mappings.py",
    "update_exports_header.py",
]

# cd into the util directory
os.chdir(os.path.dirname(os.path.abspath(__file__)))

for script in scripts:
    print(f"\n{'='*60}")
    print(f"Running {script}")
    print(f"{'='*60}\n")

    result = subprocess.run([sys.executable, script])
    if result.returncode != 0:
        print(f"Error: {script} failed with exit code {result.returncode}")
        sys.exit(result.returncode)

    print(f"{'='*60}\n")

print(f"\n{'='*60}")
print("All scripts completed successfully!")
print(f"{'='*60}\n")