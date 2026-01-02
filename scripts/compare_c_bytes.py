#!/usr/bin/env python3
"""
Compare compiled C files against original binary across all GCC versions.
Shows byte-level match percentage to identify the best compiler version.
"""

import subprocess
import tempfile
import os
import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).parent.parent
SRC_DIR = PROJECT_ROOT / "src"
ORIGINAL = PROJECT_ROOT / "disks/pal/SLES_010.90"
GCC_BUILDS = PROJECT_ROOT / "tools/gcc-builds"

# Compiler flags
CFLAGS = "-O2 -G8 -fno-builtin -mno-abicalls -mcpu=3000"

GCC_VERSIONS = [
    "gcc-2.5.7-psx",
    "gcc-2.6.0-psx", 
    "gcc-2.6.3-psx",
    "gcc-2.7.2-psx",
    "gcc-2.8.0-psx",
    "gcc-2.8.1-psx",
    "gcc-2.91.66-psx",
    "gcc-2.95.2-psx",
]

# Function locations from symbol_addrs.txt
# Format: func_name -> (vram_addr, size) - will calculate file offset
# File offset = VRAM - 0x80010000 + 0x800
FUNCTIONS = {
    "initPlayerState": (0x800260D0, 0x8C),
    "getLevelName": (0x8007AA08, 0x20),
    "main": (0x800828B0, 0x360),
    "LoadBLBHeader": (0x800208B0, 0xC4),
    "LoadGameAssetLocations": (0x800389DC, 0x1BC),  # estimated size from code
}

def vram_to_offset(vram: int) -> int:
    """Convert VRAM address to file offset."""
    return vram - 0x80010000 + 0x800

# Map source files to functions
FILE_FUNCTIONS = {
    "initPlayerState.c": ["initPlayerState"],
    "getLevelName.c": ["getLevelName"],
    "main.c": ["main"],
    "LoadBLBHeader.c": ["LoadBLBHeader"],
    "LoadGameAssetLocations.c": ["LoadGameAssetLocations"],
}

def read_original_bytes(offset: int, size: int) -> bytes:
    """Read bytes from original binary."""
    with open(ORIGINAL, "rb") as f:
        f.seek(offset)
        return f.read(size)

def compile_file(src_file: Path, gcc_version: str, temp_dir: Path) -> bytes | None:
    """Compile a C file with a specific GCC version and return .text bytes."""
    cc1 = GCC_BUILDS / gcc_version / "cc1"
    temp_s = temp_dir / "temp.s"
    temp_o = temp_dir / "temp.o"
    temp_bin = temp_dir / "temp.bin"
    
    try:
        # Preprocess
        cpp_result = subprocess.run(
            ["cpp", "-I", "include", "-I", "psyq", "-D_LANGUAGE_C", "-DVERSION_pal", str(src_file)],
            capture_output=True, text=True, cwd=PROJECT_ROOT
        )
        if cpp_result.returncode != 0:
            return None
        
        # Compile with cc1
        cc1_result = subprocess.run(
            [str(cc1)] + CFLAGS.split() + ["-o", str(temp_s)],
            input=cpp_result.stdout, capture_output=True, text=True, cwd=PROJECT_ROOT
        )
        if cc1_result.returncode != 0:
            return None
        
        # Assemble with maspsx
        maspsx_result = subprocess.run(
            ["python3", "tools/maspsx/maspsx.py", "--aspsx-version=2.86", 
             "--run-assembler", "--gnu-as-path=mipsel-unknown-linux-gnu-as",
             "-o", str(temp_o), "-march=r3000", "-mtune=r3000", 
             "-no-pad-sections", "-G8", "-Iinclude", str(temp_s)],
            capture_output=True, text=True, cwd=PROJECT_ROOT
        )
        if maspsx_result.returncode != 0:
            return None
        
        # Extract .text section
        subprocess.run(
            ["mipsel-unknown-linux-gnu-objcopy", "-O", "binary", "-j", ".text", 
             str(temp_o), str(temp_bin)],
            capture_output=True, cwd=PROJECT_ROOT
        )
        
        if temp_bin.exists():
            with open(temp_bin, "rb") as f:
                return f.read()
        return None
        
    except Exception as e:
        return None

def count_matching_bytes(bytes1: bytes, bytes2: bytes) -> tuple[int, int, int]:
    """Count matching bytes. Returns (matches, total, differing_positions)."""
    min_len = min(len(bytes1), len(bytes2))
    matches = sum(1 for i in range(min_len) if bytes1[i] == bytes2[i])
    
    # Find first differing position
    first_diff = -1
    for i in range(min_len):
        if bytes1[i] != bytes2[i]:
            first_diff = i
            break
    
    return matches, min_len, first_diff

def main():
    print("=" * 70)
    print("Comparing C files against original binary (all GCC versions)")
    print("=" * 70)
    print()
    
    c_files = sorted(SRC_DIR.glob("*.c"))
    
    with tempfile.TemporaryDirectory() as temp_dir:
        temp_path = Path(temp_dir)
        
        for src_file in c_files:
            filename = src_file.name
            
            # Check if this file has known functions
            if filename not in FILE_FUNCTIONS:
                # Try to compile anyway and show size comparison
                has_include_asm = "INCLUDE_ASM" in src_file.read_text()
                if has_include_asm:
                    print(f"[{filename}] Uses INCLUDE_ASM - skipping byte comparison")
                    print()
                    continue
            
            print("=" * 70)
            print(f"File: {filename}")
            print("=" * 70)
            
            # Get function info if available
            func_names = FILE_FUNCTIONS.get(filename, [])
            if func_names:
                for func_name in func_names:
                    if func_name in FUNCTIONS:
                        vram_addr, size = FUNCTIONS[func_name]
                        offset = vram_to_offset(vram_addr)
                        original_bytes = read_original_bytes(offset, size)
                        
                        print(f"\nFunction: {func_name}")
                        print(f"  Original: VRAM=0x{vram_addr:X}, offset=0x{offset:X}, size=0x{size:X} ({size} bytes)")
                        print()
                        
                        print(f"{'GCC Version':<20} {'Size':>8} {'Match':>8} {'Percent':>10} {'1st Diff':>10}")
                        print("-" * 60)
                        
                        best_version = None
                        best_match = 0
                        
                        for gcc_ver in GCC_VERSIONS:
                            compiled = compile_file(src_file, gcc_ver, temp_path)
                            
                            if compiled is None:
                                print(f"{gcc_ver:<20} {'FAIL':>8}")
                                continue
                            
                            matches, total, first_diff = count_matching_bytes(original_bytes, compiled)
                            percent = (matches / total * 100) if total > 0 else 0
                            
                            diff_str = f"0x{first_diff:X}" if first_diff >= 0 else "N/A"
                            size_match = "✓" if len(compiled) == size else f"+{len(compiled)-size}"
                            
                            print(f"{gcc_ver:<20} {len(compiled):>7}{'✓' if len(compiled)==size else ''} "
                                  f"{matches:>7}/{total} {percent:>9.1f}% {diff_str:>10}")
                            
                            if matches > best_match:
                                best_match = matches
                                best_version = gcc_ver
                        
                        if best_version:
                            print()
                            print(f"  Best match: {best_version} ({best_match}/{len(original_bytes)} bytes = "
                                  f"{best_match/len(original_bytes)*100:.1f}%)")
            else:
                # Just show sizes for files without known function mappings
                print(f"\n(No function offset mapping - showing code sizes only)")
                print()
                print(f"{'GCC Version':<20} {'Size':>8}")
                print("-" * 30)
                
                for gcc_ver in GCC_VERSIONS:
                    compiled = compile_file(src_file, gcc_ver, temp_path)
                    if compiled:
                        print(f"{gcc_ver:<20} {len(compiled):>8} (0x{len(compiled):X})")
                    else:
                        print(f"{gcc_ver:<20} {'FAIL':>8}")
            
            print()
    
    print("=" * 70)
    print("Done!")
    print("=" * 70)

if __name__ == "__main__":
    main()
