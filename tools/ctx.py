#!/usr/bin/env python3
"""
Generate context file for m2c and decomp.me

This script preprocesses your project's headers to create a ctx.c file
that can be used with m2c for better decompilation output.

Usage:
    python3 tools/ctx.py [path/to/file.c]
    
If no argument is given, generates context from include/common.h
Output is saved to ctx.c in the project root.
"""

import os
import sys
import subprocess
from pathlib import Path

# Project directories
SCRIPT_DIR = Path(__file__).parent
ROOT_DIR = SCRIPT_DIR.parent
SRC_DIR = ROOT_DIR / "src"
INCLUDE_DIR = ROOT_DIR / "include"


def get_c_file_for_asm_dir(dirname: str) -> Path | None:
    """Find the C file corresponding to an asm directory name."""
    for c_file in SRC_DIR.rglob("*.c"):
        if c_file.stem == dirname or dirname in str(c_file):
            return c_file
    return None


def preprocess_c_file(c_file: Path) -> str:
    """Run the C preprocessor on a file and return the output."""
    cpp_command = [
        "cpp", "-E", "-P",
        "-Iinclude",
        "-Isrc",
        "-D_LANGUAGE_C",
        "-DVERSION_US",  # Adjust for your version
        str(c_file)
    ]
    
    try:
        result = subprocess.run(
            cpp_command,
            cwd=ROOT_DIR,
            capture_output=True,
            text=True,
            check=True
        )
        return result.stdout
    except subprocess.CalledProcessError as e:
        print(f"Error preprocessing {c_file}:", file=sys.stderr)
        print(e.stderr, file=sys.stderr)
        sys.exit(1)


def clean_preprocessed(text: str) -> str:
    """Clean up preprocessor output for m2c consumption."""
    lines = text.split("\n")
    output = []
    
    for line in lines:
        # Skip __attribute__ lines (m2c doesn't handle them well)
        if "__attribute__" in line:
            continue
        
        # Fix sizeof(long) which m2c doesn't understand
        line = line.replace("sizeof(long)", "4")
        
        output.append(line)
    
    return "\n".join(output)


def main():
    if len(sys.argv) > 1:
        arg = sys.argv[1]
        if arg in ("-h", "--help"):
            print(__doc__)
            sys.exit(0)
        c_file = Path(arg)
        if not c_file.exists():
            c_file = ROOT_DIR / arg
    else:
        # Default to common.h
        c_file = INCLUDE_DIR / "common.h"
    
    if not c_file.exists():
        print(f"Error: File not found: {c_file}", file=sys.stderr)
        sys.exit(1)
    
    print(f"Generating context from: {c_file}")
    
    processed = preprocess_c_file(c_file)
    cleaned = clean_preprocessed(processed)
    
    output_path = ROOT_DIR / "ctx.c"
    with open(output_path, "w", encoding="utf-8") as f:
        f.write(cleaned)
    
    print(f"Context saved to: {output_path}")
    print(f"Size: {len(cleaned)} bytes")


if __name__ == "__main__":
    main()
