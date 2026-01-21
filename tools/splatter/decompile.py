#!/usr/bin/env python3
"""
Decompile - Prepare a function for decompilation.

Automates the initial steps of PSX decompilation:
1. Look up function in symbol_addrs.txt (requires name and size annotation)
2. Calculate ROM offset from VRAM address
3. Insert segment into splat.pal.yaml (dictionary format)
4. Run make check to verify build still matches
5. Run m2c to verify the ASM decompiles correctly

Usage:
    # Show what would be done (no changes made)
    python3 tools/splatter/decompile.py FunctionName --dry-run

    # Prepare function for decompilation
    python3 tools/splatter/decompile.py FunctionName

    # Skip verification steps
    python3 tools/splatter/decompile.py FunctionName --no-check

Examples:
    python3 tools/splatter/decompile.py GetAssetCount --dry-run
    python3 tools/splatter/decompile.py PlayerTickCallback
"""

import sys
import argparse
import subprocess
from pathlib import Path

# Add lib to path
sys.path.insert(0, str(Path(__file__).parent))

from lib.symbols import parse_symbol_addrs, find_symbol
from lib.splat_config import SplatConfig, vram_to_rom


# Configuration
PROJECT_ROOT = Path(__file__).parent.parent.parent
SYMBOL_FILE = PROJECT_ROOT / "symbol_addrs.txt"
CONFIG_FILE = PROJECT_ROOT / "config" / "splat.pal.yaml"
CTX_FILE = PROJECT_ROOT / "ctx.c"


def error(msg: str):
    """Print error and exit."""
    print(f"❌ ERROR: {msg}", file=sys.stderr)
    sys.exit(1)


def success(msg: str):
    """Print success message."""
    print(f"✓ {msg}")


def info(msg: str):
    """Print info message."""
    print(f"  {msg}")


def run_make_check(dry_run: bool = False) -> bool:
    """Run make check to verify the build still matches."""
    if dry_run:
        info("Would run: make check")
        return True
    
    info("Running make check...")
    try:
        result = subprocess.run(
            ["make", "check"],
            capture_output=True,
            text=True,
            timeout=120,
            cwd=PROJECT_ROOT
        )
        if result.returncode == 0:
            success("Build matches original binary!")
            return True
        else:
            print(result.stdout)
            print(result.stderr, file=sys.stderr)
            error(f"make check failed with exit code {result.returncode}")
    except subprocess.TimeoutExpired:
        error("make check timed out after 120 seconds")
    except FileNotFoundError:
        error("make not found. Are you in the project directory?")
    except Exception as e:
        error(f"Failed to run make check: {e}")
    return False


def run_m2c_check(name: str, dry_run: bool = False) -> bool:
    """
    Run m2c on the generated ASM to verify it decompiles to a valid function.
    """
    asm_file = PROJECT_ROOT / "asm" / "pal" / f"{name}.s"
    
    if dry_run:
        info(f"Would run m2c on {asm_file}")
        return True
    
    if not asm_file.exists():
        error(f"ASM file not found: {asm_file}\n"
              f"       Did splat run correctly?")
    
    if not CTX_FILE.exists():
        info("Context file ctx.c not found, generating...")
        try:
            result = subprocess.run(
                ["make", "context"],
                capture_output=True,
                text=True,
                timeout=30,
                cwd=PROJECT_ROOT
            )
            if result.returncode != 0:
                error(f"Failed to generate ctx.c: {result.stderr}")
        except Exception as e:
            error(f"Failed to run make context: {e}")
    
    info(f"Running m2c on {asm_file}...")
    try:
        result = subprocess.run(
            ["python3", "tools/m2c/m2c.py", 
             "--context", str(CTX_FILE),
             "--target", "mipsel-gcc-c",
             str(asm_file)],
            capture_output=True,
            text=True,
            timeout=30,
            cwd=PROJECT_ROOT
        )
        
        output = result.stdout
        
        if result.returncode != 0:
            print(result.stderr, file=sys.stderr)
            error(f"m2c failed with exit code {result.returncode}")
        
        if not output.strip():
            error("m2c produced empty output - ASM may be malformed")
        
        # Check that output looks like a C function
        has_function = False
        for line in output.split('\n'):
            line = line.strip()
            if not line or line.startswith('//') or line.startswith('/*'):
                continue
            if f"{name}(" in line or "(" in line:
                has_function = True
                break
        
        if not has_function:
            print(output)
            error("m2c output doesn't appear to contain a function.\n"
                  f"       The ASM at {asm_file} may be data, not code.")
        
        # Show preview
        lines = output.strip().split('\n')
        preview_lines = lines[:15] if len(lines) > 15 else lines
        
        success("m2c successfully decompiled the function!")
        info("Preview:")
        for line in preview_lines:
            print(f"    {line}")
        if len(lines) > 15:
            print(f"    ... ({len(lines) - 15} more lines)")
        
        return True
        
    except subprocess.TimeoutExpired:
        error("m2c timed out after 30 seconds")
    except FileNotFoundError:
        error("python3 or m2c not found")
    except Exception as e:
        error(f"Failed to run m2c: {e}")
    return False


def main():
    parser = argparse.ArgumentParser(
        description="Prepare a function for decompilation",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
    %(prog)s GetAssetCount --dry-run   # Preview changes
    %(prog)s PlayerTickCallback        # Prepare function for decompilation
    %(prog)s LoadBLBHeader --no-check  # Skip verification step
"""
    )
    parser.add_argument("function", help="Function name (must exist in symbol_addrs.txt with size annotation)")
    parser.add_argument("--dry-run", action="store_true",
                        help="Show what would be done without making changes")
    parser.add_argument("--no-check", action="store_true",
                        help="Skip make check and m2c verification steps")
    
    args = parser.parse_args()
    
    print(f"\n{'='*60}")
    print(f"Preparing: {args.function}")
    print(f"{'='*60}\n")
    
    # Step 1: Look up function in symbol_addrs.txt
    info(f"Looking up '{args.function}' in {SYMBOL_FILE.name}...")
    
    if not SYMBOL_FILE.exists():
        error(f"{SYMBOL_FILE} not found")
    
    symbols = parse_symbol_addrs(SYMBOL_FILE)
    symbol = find_symbol(symbols, args.function)
    
    if not symbol:
        error(f"Function '{args.function}' not found in {SYMBOL_FILE.name}\n"
              f"       Make sure it exists with the format:\n"
              f"       {args.function} = 0xADDRESS; // size:0xSIZE")
    
    if symbol.size is None or symbol.size == 0:
        error(f"Function '{args.function}' has no size annotation.\n"
              f"       Add size:0xXX to the symbol definition.")
    
    rom_offset = vram_to_rom(symbol.address)
    
    success(f"Found: {args.function}")
    info(f"VRAM address: 0x{symbol.address:08X}")
    info(f"ROM offset:   0x{rom_offset:X}")
    info(f"Size:         0x{symbol.size:X} ({symbol.size} bytes)")
    
    # Step 2: Update splat YAML
    print(f"\n{'='*60}")
    print("Updating splat configuration")
    print(f"{'='*60}\n")
    
    config = SplatConfig(CONFIG_FILE)
    config.load()
    
    # Check what segment currently contains this offset
    containing = config.find_segment_at(rom_offset)
    if containing:
        if containing.seg_type == 'c':
            error(f"Function is inside an existing 'c' segment '{containing.name}' "
                  f"[0x{containing.start:X}].\n"
                  f"       Cannot automatically split C segments.")
        info(f"Function is in segment [{containing.seg_type}] at 0x{containing.start:X}")
    
    # Check for overlap
    end_rom = rom_offset + symbol.size
    overlap = config.check_overlap_with_c(rom_offset, end_rom)
    if overlap:
        error(f"Function would overlap with C segment '{overlap.name}' at 0x{overlap.start:X}")
    
    if args.dry_run:
        info(f"Would add segment:")
        info(f"  - name: {args.function}")
        info(f"    type: asm")
        info(f"    start: 0x{rom_offset:X}")
        info(f"Would add trailing segment:")
        info(f"  - type: asm")
        info(f"    start: 0x{end_rom:X}")
    else:
        added = config.add_segment(
            start=rom_offset,
            seg_type='asm',
            name=args.function,
            end=end_rom,
            use_dict_format=True
        )
        
        if added:
            config.save()
            success(f"Updated {CONFIG_FILE.name}")
            info(f"Added segment: {args.function} at 0x{rom_offset:X}")
        else:
            info(f"Segment already exists at 0x{rom_offset:X}")
    
    # Step 3: Run make check
    if not args.no_check:
        print(f"\n{'='*60}")
        print("Verifying build")
        print(f"{'='*60}\n")
        
        if not run_make_check(args.dry_run):
            error("Build verification failed")
        
        # Step 4: Verify m2c can decompile
        print(f"\n{'='*60}")
        print("Verifying decompilation")
        print(f"{'='*60}\n")
        
        if not run_m2c_check(args.function, args.dry_run):
            error("Decompilation verification failed")
    
    # Done
    print(f"\n{'='*60}")
    print("Done!")
    print(f"{'='*60}")
    
    if args.dry_run:
        print("\n⚠️  Dry run - no changes were made")
    else:
        print(f"\nNext steps:")
        print(f"  1. Create src/{args.function}.c with the decompiled code shown above")
        print(f"  2. Fix types and variable names based on analysis")
        print(f"  3. Change segment type from 'asm' to 'c' in {CONFIG_FILE.name}")
        print(f"  4. Run: make && make check")
    
    print()


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\nInterrupted by user", file=sys.stderr)
        sys.exit(130)
