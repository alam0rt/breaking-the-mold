#!/usr/bin/env python3
"""
PSX Decompilation Progress Tracker
==================================

Calculate and display decompilation progress based on map file analysis.

Usage:
    python3 tools/progress.py build/us/game.map disks/us/SLUS_000.00

This script parses the linker map file to determine which functions
are from C source (decompiled) vs assembly (not yet decompiled).
"""

import argparse
import os
import re
import sys


def parse_map_file(map_path):
    """Parse a GNU ld map file to extract symbol information."""
    
    if not os.path.exists(map_path):
        print(f"Error: Map file not found: {map_path}")
        sys.exit(1)
    
    c_symbols = []      # Symbols from .c files
    asm_symbols = []    # Symbols from .s files
    
    with open(map_path, "r") as f:
        content = f.read()
    
    # Look for symbol definitions with source file information
    # Format varies by linker, but commonly:
    #   0x80012345  function_name  src/file.c
    #   0x80012345  function_name  asm/file.s
    
    # Simple pattern - adjust based on your actual map file format
    pattern = r"0x([0-9a-fA-F]+)\s+(\w+)\s+(\S+\.(c|s))"
    
    for match in re.finditer(pattern, content):
        addr = int(match.group(1), 16)
        name = match.group(2)
        source = match.group(3)
        ext = match.group(4)
        
        if ext == "c":
            c_symbols.append((name, addr, source))
        elif ext == "s":
            asm_symbols.append((name, addr, source))
    
    return c_symbols, asm_symbols


def get_binary_size(binary_path):
    """Get the size of the original binary."""
    
    if not os.path.exists(binary_path):
        print(f"Warning: Binary not found: {binary_path}")
        return 0
    
    return os.path.getsize(binary_path)


def calculate_progress(c_symbols, asm_symbols, total_size):
    """Calculate decompilation progress."""
    
    total_funcs = len(c_symbols) + len(asm_symbols)
    decomp_funcs = len(c_symbols)
    
    if total_funcs == 0:
        return {
            "total_functions": 0,
            "decompiled_functions": 0,
            "function_percent": 0.0,
            "total_bytes": total_size,
            "decompiled_bytes": 0,
            "byte_percent": 0.0,
        }
    
    # Function-based progress
    func_percent = (decomp_funcs / total_funcs) * 100
    
    # For byte-based progress, we'd need to know function sizes
    # This is a simplified estimate
    byte_percent = func_percent  # Rough approximation
    
    return {
        "total_functions": total_funcs,
        "decompiled_functions": decomp_funcs,
        "function_percent": func_percent,
        "total_bytes": total_size,
        "decompiled_bytes": int(total_size * func_percent / 100),
        "byte_percent": byte_percent,
    }


def print_progress(progress):
    """Display progress in a nice format."""
    
    print("")
    print("=" * 50)
    print("  Decompilation Progress")
    print("=" * 50)
    print("")
    print(f"  Functions: {progress['decompiled_functions']:>5} / {progress['total_functions']:<5} ({progress['function_percent']:.1f}%)")
    print(f"  Bytes:     {progress['decompiled_bytes']:>5} / {progress['total_bytes']:<5} ({progress['byte_percent']:.1f}%)")
    print("")
    
    # ASCII progress bar
    bar_width = 40
    filled = int(bar_width * progress['function_percent'] / 100)
    bar = "█" * filled + "░" * (bar_width - filled)
    print(f"  [{bar}] {progress['function_percent']:.1f}%")
    print("")


def main():
    parser = argparse.ArgumentParser(description="Calculate decompilation progress")
    parser.add_argument("map_file", help="Path to linker map file")
    parser.add_argument("binary", help="Path to original binary (for size reference)")
    parser.add_argument("--json", action="store_true", help="Output as JSON")
    
    args = parser.parse_args()
    
    c_symbols, asm_symbols = parse_map_file(args.map_file)
    total_size = get_binary_size(args.binary)
    progress = calculate_progress(c_symbols, asm_symbols, total_size)
    
    if args.json:
        import json
        print(json.dumps(progress, indent=2))
    else:
        print_progress(progress)


if __name__ == "__main__":
    main()
