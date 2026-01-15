#!/usr/bin/env python3
"""
Automated decompilation workflow for a single function.

This script:
1. Generates ctx.c context file if needed
2. Ensures function is in symbol_addrs.txt
3. Ensures function is in splat config
4. Runs splat to generate assembly
5. Invokes m2c to decompile
6. Creates/updates source file with decompiled code
7. Optionally runs build verification

Usage:
    # Decompile function by name
    python3 scripts/auto_decompile_function.py GetLevelCount

    # Decompile function by address
    python3 scripts/auto_decompile_function.py 0x8007A9B0

    # Decompile and verify build
    python3 scripts/auto_decompile_function.py GetLevelCount --verify

    # Dry run (show what would happen)
    python3 scripts/auto_decompile_function.py GetLevelCount --dry-run
"""

import argparse
import json
import re
import subprocess
import sys
import shutil
from pathlib import Path
from typing import Optional, Tuple, Dict
from urllib.request import urlopen, Request
from urllib.error import URLError

DEFAULT_PORT = 8192
RAM_BASE = 0x80010000
ROM_BASE = 0x800
SYMBOL_FILE = "symbol_addrs.txt"
CONFIG_FILE = "config/splat.pal.yaml"
CTX_FILE = "ctx.c"
M2C_PATH = "tools/m2c/m2c.py"
ASM_BASE = "asm/pal/nonmatchings"
SRC_BASE = "src"


def ghidra_request(endpoint: str, port: int = DEFAULT_PORT) -> dict:
    """Make a request to the Ghidra MCP bridge."""
    url = f"http://localhost:{port}{endpoint}"
    try:
        with urlopen(url, timeout=30) as response:
            return json.loads(response.read().decode())
    except URLError as e:
        return {"error": str(e)}


def get_function_by_name(name: str, port: int) -> Optional[dict]:
    """Get function info by name."""
    # List all functions and find by name
    result = ghidra_request(f"/functions?limit=10000", port)
    if result.get("success"):
        # MCP API returns functions in 'result', not 'functions'
        functions = result.get("result", result.get("functions", []))
        for func in functions:
            if func.get("name") == name:
                return func
    return None


def get_function_by_address(address: int, port: int) -> Optional[dict]:
    """Get function info by address."""
    result = ghidra_request(f"/functions/{address:08x}", port)
    if result.get("success") or result.get("name"):
        return result.get("result", result)
    return None


def decompile_function(address: int, port: int) -> Optional[str]:
    """Get decompiled C code from Ghidra."""
    result = ghidra_request(f"/functions/{address:08x}/decompile", port)
    if result.get("success"):
        return result.get("code", "")
    return None


def vram_to_rom(vram_addr: int) -> int:
    """Convert VRAM address to ROM offset."""
    return (vram_addr - RAM_BASE) + ROM_BASE


def run_command(cmd: list, description: str, dry_run: bool = False) -> Tuple[bool, str]:
    """Run a shell command and return success status and output."""
    if dry_run:
        print(f"  [DRY RUN] Would run: {' '.join(cmd)}")
        return True, ""
    
    print(f"  {description}...", file=sys.stderr)
    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            check=False
        )
        
        if result.returncode != 0:
            print(f"    ERROR: {result.stderr}", file=sys.stderr)
            return False, result.stderr
        
        return True, result.stdout
    
    except Exception as e:
        print(f"    ERROR: {e}", file=sys.stderr)
        return False, str(e)


def ensure_context_file(dry_run: bool = False) -> bool:
    """Ensure ctx.c exists and is up to date."""
    ctx_path = Path(CTX_FILE)
    common_h = Path("include/common.h")
    
    # Check if regeneration is needed
    if ctx_path.exists():
        if common_h.exists() and ctx_path.stat().st_mtime > common_h.stat().st_mtime:
            print(f"  {CTX_FILE} is up to date", file=sys.stderr)
            return True
    
    print(f"  Generating {CTX_FILE}...", file=sys.stderr)
    
    if dry_run:
        print(f"    [DRY RUN] Would generate {CTX_FILE}", file=sys.stderr)
        return True
    
    # Generate context file
    cmd = [
        "cpp", "-E", "-P",
        "-I", "include",
        "-I", "psyq",
        "-D_LANGUAGE_C",
        "include/common.h"
    ]
    
    success, output = run_command(cmd, f"Generate {CTX_FILE}", dry_run=False)
    
    if success:
        with open(CTX_FILE, 'w') as f:
            f.write(output)
        print(f"    Generated {CTX_FILE} ({len(output)} bytes)", file=sys.stderr)
    
    return success


def ensure_in_symbol_addrs(func_name: str, func_address: int, func_size: Optional[int], 
                            dry_run: bool = False) -> bool:
    """Ensure function is in symbol_addrs.txt."""
    symbol_path = Path(SYMBOL_FILE)
    
    if not symbol_path.exists():
        print(f"  ERROR: {SYMBOL_FILE} not found", file=sys.stderr)
        return False
    
    # Check if already present
    with open(symbol_path, 'r') as f:
        content = f.read()
        pattern = rf'^{re.escape(func_name)}\s*=\s*0x{func_address:08X}\s*;'
        if re.search(pattern, content, re.MULTILINE):
            print(f"  {func_name} already in {SYMBOL_FILE}", file=sys.stderr)
            return True
    
    # Add to file
    print(f"  Adding {func_name} to {SYMBOL_FILE}...", file=sys.stderr)
    
    if dry_run:
        print(f"    [DRY RUN] Would add to {SYMBOL_FILE}", file=sys.stderr)
        return True
    
    # Format entry
    entry = f"{func_name} = 0x{func_address:08X};"
    if func_size:
        entry += f" // size:0x{func_size:X}"
    
    # Append to file
    with open(symbol_path, 'a') as f:
        f.write(f"\n{entry}\n")
    
    print(f"    Added to {SYMBOL_FILE}", file=sys.stderr)
    return True


def ensure_in_splat_config(func_name: str, rom_offset: int, dry_run: bool = False) -> bool:
    """Ensure function is in splat config."""
    # Use update_splat_config.py script
    cmd = [
        "python3", "scripts/update_splat_config.py",
        "--filter", func_name
    ]
    
    if dry_run:
        cmd.append("--dry-run")
    
    success, _ = run_command(cmd, f"Update splat config for {func_name}", dry_run=False)
    return success


def run_splat(dry_run: bool = False) -> bool:
    """Run splat to generate assembly files."""
    cmd = ["python3", "-m", "splat", "split", CONFIG_FILE]
    success, _ = run_command(cmd, "Run splat", dry_run)
    return success


def run_m2c(func_name: str, source_file: str, dry_run: bool = False) -> Tuple[bool, str]:
    """Run m2c decompiler on the function's assembly."""
    # Find assembly file
    asm_file = Path(ASM_BASE) / source_file / f"{func_name}.s"
    
    if not asm_file.exists():
        print(f"  ERROR: Assembly file not found: {asm_file}", file=sys.stderr)
        return False, ""
    
    print(f"  Decompiling {asm_file}...", file=sys.stderr)
    
    if dry_run:
        print(f"    [DRY RUN] Would run m2c on {asm_file}", file=sys.stderr)
        return True, "// Decompiled code would appear here"
    
    # Run m2c
    cmd = [
        "python3", M2C_PATH,
        "--context", CTX_FILE,
        "--target", "mipsel-gcc-c",
        str(asm_file)
    ]
    
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, check=False)
        
        if result.returncode != 0:
            print(f"    m2c error: {result.stderr}", file=sys.stderr)
            return False, ""
        
        decompiled = result.stdout
        print(f"    Decompiled {len(decompiled)} bytes", file=sys.stderr)
        return True, decompiled
    
    except Exception as e:
        print(f"    ERROR: {e}", file=sys.stderr)
        return False, ""


def create_or_update_source(func_name: str, source_file: str, decompiled_code: str, 
                             dry_run: bool = False) -> bool:
    """Create or update source file with decompiled code."""
    src_path = Path(SRC_BASE) / f"{source_file}.c"
    
    # Check if file exists
    if src_path.exists():
        print(f"  Updating {src_path}...", file=sys.stderr)
        
        if dry_run:
            print(f"    [DRY RUN] Would replace INCLUDE_ASM in {src_path}", file=sys.stderr)
            return True
        
        # Read existing file
        with open(src_path, 'r') as f:
            content = f.read()
        
        # Find and replace INCLUDE_ASM macro
        include_asm_pattern = rf'INCLUDE_ASM\s*\(\s*["\']([^"\']+)["\']\s*,\s*["\']?{re.escape(func_name)}["\']?\s*\)\s*;?'
        
        if re.search(include_asm_pattern, content):
            # Replace INCLUDE_ASM with decompiled code
            new_content = re.sub(include_asm_pattern, decompiled_code, content)
            
            with open(src_path, 'w') as f:
                f.write(new_content)
            
            print(f"    Replaced INCLUDE_ASM in {src_path}", file=sys.stderr)
            return True
        else:
            print(f"    WARNING: INCLUDE_ASM for {func_name} not found in {src_path}", file=sys.stderr)
            print(f"    You may need to manually add the decompiled code", file=sys.stderr)
            return False
    
    else:
        print(f"  Creating {src_path}...", file=sys.stderr)
        
        if dry_run:
            print(f"    [DRY RUN] Would create {src_path}", file=sys.stderr)
            return True
        
        # Create new file
        src_path.parent.mkdir(parents=True, exist_ok=True)
        
        content = f'''#include "common.h"

{decompiled_code}
'''
        
        with open(src_path, 'w') as f:
            f.write(content)
        
        print(f"    Created {src_path}", file=sys.stderr)
        return True


def verify_build(dry_run: bool = False) -> bool:
    """Run make check to verify the build matches."""
    if dry_run:
        print("  [DRY RUN] Would run 'make check'", file=sys.stderr)
        return True
    
    print("\n  Verifying build...", file=sys.stderr)
    
    # Clean build
    success, _ = run_command(["make", "clean"], "Clean build")
    if not success:
        return False
    
    # Build
    success, _ = run_command(["make"], "Build")
    if not success:
        return False
    
    # Check
    success, output = run_command(["make", "check"], "Verify match")
    
    if success:
        print("\n  ✓ BUILD MATCHES ORIGINAL", file=sys.stderr)
    else:
        print("\n  ✗ BUILD DOES NOT MATCH", file=sys.stderr)
        print("    Decompilation may need manual adjustment", file=sys.stderr)
    
    return success


def main():
    parser = argparse.ArgumentParser(
        description="Automated decompilation workflow for a single function"
    )
    parser.add_argument("function", type=str,
                        help="Function name or address (e.g., GetLevelCount or 0x8007A9B0)")
    parser.add_argument("--port", type=int, default=DEFAULT_PORT,
                        help=f"Ghidra MCP port (default: {DEFAULT_PORT})")
    parser.add_argument("--source-file", type=str, default=None,
                        help="Source file name (default: use function name)")
    parser.add_argument("--verify", action="store_true",
                        help="Run make check after decompilation")
    parser.add_argument("--dry-run", action="store_true",
                        help="Show what would happen without making changes")
    parser.add_argument("--use-ghidra-decompile", action="store_true",
                        help="Use Ghidra's decompiler instead of m2c")
    
    args = parser.parse_args()
    
    try:
        # Parse function identifier
        if args.function.startswith("0x"):
            func_address = int(args.function, 16)
            func = get_function_by_address(func_address, args.port)
        else:
            func = get_function_by_name(args.function, args.port)
            if func:
                func_address = func.get("entryPoint", 0)
                if isinstance(func_address, str):
                    func_address = int(func_address, 16)
        
        if not func:
            print(f"ERROR: Function '{args.function}' not found in Ghidra", file=sys.stderr)
            return 1
        
        func_name = func.get("name", "")
        print(f"\n=== Decompiling {func_name} @ 0x{func_address:08X} ===\n", file=sys.stderr)
        
        # Get function size
        func_size = None
        body = func.get("body", {})
        if body:
            min_addr = body.get("minAddress", "")
            max_addr = body.get("maxAddress", "")
            if min_addr and max_addr:
                if isinstance(min_addr, str):
                    min_addr = int(min_addr, 16)
                if isinstance(max_addr, str):
                    max_addr = int(max_addr, 16)
                func_size = max_addr - min_addr + 1
        
        rom_offset = vram_to_rom(func_address)
        
        # Determine source file name
        source_file = args.source_file or func_name
        
        # Step 1: Ensure context file exists
        if not ensure_context_file(args.dry_run):
            return 1
        
        # Step 2: Ensure in symbol_addrs.txt
        if not ensure_in_symbol_addrs(func_name, func_address, func_size, args.dry_run):
            return 1
        
        # Step 3: Ensure in splat config
        if not ensure_in_splat_config(func_name, rom_offset, args.dry_run):
            return 1
        
        # Step 4: Run splat
        if not run_splat(args.dry_run):
            return 1
        
        # Step 5: Decompile
        if args.use_ghidra_decompile:
            print(f"  Using Ghidra decompiler...", file=sys.stderr)
            decompiled = decompile_function(func_address, args.port)
            if not decompiled:
                print(f"  ERROR: Failed to get decompiled code from Ghidra", file=sys.stderr)
                return 1
        else:
            success, decompiled = run_m2c(func_name, source_file, args.dry_run)
            if not success:
                print(f"\n  Tip: Try --use-ghidra-decompile to use Ghidra's decompiler", file=sys.stderr)
                return 1
        
        # Step 6: Create/update source file
        if not create_or_update_source(func_name, source_file, decompiled, args.dry_run):
            print(f"  WARNING: Could not update source file automatically", file=sys.stderr)
            print(f"  Decompiled code:\n{decompiled}", file=sys.stderr)
        
        # Step 7: Verify build (optional)
        if args.verify and not args.dry_run:
            if not verify_build(args.dry_run):
                print(f"\n  Build verification failed. You may need to manually adjust the decompiled code.", file=sys.stderr)
                return 1
        
        print(f"\n✓ Decompilation complete for {func_name}", file=sys.stderr)
        return 0
    
    except KeyboardInterrupt:
        print("\nInterrupted by user", file=sys.stderr)
        return 130
    except Exception as e:
        print(f"ERROR: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        return 1


if __name__ == "__main__":
    sys.exit(main())
