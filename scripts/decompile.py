#!/usr/bin/env python3
"""
Minimal decompilation workflow tool.

Automates the initial steps of PSX decompilation:
1. Look up function in symbol_addrs.txt (requires name and size annotation)
2. Calculate ROM offset from VRAM address
3. Insert ASM segment into splat.pal.yaml
4. Run make check to verify build still matches

This script does NOT require Ghidra - it reads from symbol_addrs.txt directly.
For the full Ghidra-integrated workflow, use old_decompile.py.

Usage:
    # Show what would be done (no changes made)
    python3 scripts/decompile.py FunctionName --dry-run

    # Prepare function for decompilation (update YAML, run splat, verify)
    python3 scripts/decompile.py FunctionName

    # Skip make check
    python3 scripts/decompile.py FunctionName --no-check

Examples:
    python3 scripts/decompile.py GetAssetCount --dry-run
    python3 scripts/decompile.py PlayerTickCallback
"""

import sys
import argparse
import re
import subprocess
from pathlib import Path
from typing import Optional, Tuple

# Configuration
SYMBOL_FILE = Path("symbol_addrs.txt")
CONFIG_FILE = Path("config/splat.pal.yaml")

# Memory layout constants (PAL version)
RAM_BASE = 0x80010000
ROM_BASE = 0x800


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


def vram_to_rom(vram: int) -> int:
    """Convert VRAM address to ROM file offset."""
    return (vram - RAM_BASE) + ROM_BASE


def parse_symbol_addrs(name: str) -> Optional[Tuple[int, int]]:
    """
    Look up function in symbol_addrs.txt.
    
    Returns (address, size) tuple or None if not found.
    
    Expected format:
        FunctionName = 0x80012345; // size:0x88
    """
    if not SYMBOL_FILE.exists():
        error(f"{SYMBOL_FILE} not found")
    
    # Pattern to match: name = 0xADDRESS; // size:0xSIZE
    # Allow optional whitespace and other comments after size
    pattern = re.compile(
        rf'^{re.escape(name)}\s*=\s*0x([0-9A-Fa-f]+)\s*;\s*//\s*size:0x([0-9A-Fa-f]+)',
        re.MULTILINE
    )
    
    with open(SYMBOL_FILE, 'r') as f:
        content = f.read()
    
    match = pattern.search(content)
    if not match:
        return None
    
    address = int(match.group(1), 16)
    size = int(match.group(2), 16)
    return (address, size)


def find_insertion_point(subsegments: list, rom_offset: int) -> Tuple[int, Optional[int]]:
    """
    Find where to insert a new segment in the subsegments list.
    
    Returns (insert_index, next_segment_start) where next_segment_start is the
    ROM offset of the segment after the insertion point (for overlap checking).
    """
    insert_idx = 0
    next_start = None
    
    for i, sub in enumerate(subsegments):
        if not isinstance(sub, list) or len(sub) < 1:
            continue
        
        sub_start_str = str(sub[0])
        sub_start = int(sub_start_str, 16) if sub_start_str.startswith('0x') else int(sub_start_str)
        
        if sub_start < rom_offset:
            insert_idx = i + 1
        elif sub_start == rom_offset:
            # Segment already exists at this offset
            return (i, sub_start)
        else:
            # Found the next segment after our insertion point
            next_start = sub_start
            break
    
    return (insert_idx, next_start)


def get_segment_at_offset(subsegments: list, rom_offset: int) -> Optional[Tuple[int, int, str]]:
    """
    Find which segment contains the given ROM offset.
    
    Returns (start, end, type) or None.
    """
    for i, sub in enumerate(subsegments):
        if not isinstance(sub, list) or len(sub) < 2:
            continue
        
        start_str = str(sub[0])
        start = int(start_str, 16) if start_str.startswith('0x') else int(start_str)
        seg_type = sub[1]
        
        # Find end from next segment
        end = 0x97000  # Default: end of file
        for j in range(i + 1, len(subsegments)):
            next_sub = subsegments[j]
            if isinstance(next_sub, list) and len(next_sub) >= 1:
                end_str = str(next_sub[0])
                end = int(end_str, 16) if end_str.startswith('0x') else int(end_str)
                break
        
        if start <= rom_offset < end:
            return (start, end, seg_type)
    
    return None


def update_splat_yaml(name: str, rom_offset: int, size: int, dry_run: bool = False) -> bool:
    """
    Update splat.pal.yaml to add new ASM segment for the function.
    
    Uses ruamel.yaml to preserve formatting.
    """
    try:
        import ruamel.yaml
    except ImportError:
        error("ruamel.yaml not installed. Run: pip install ruamel.yaml")
    
    yaml = ruamel.yaml.YAML()
    yaml.preserve_quotes = True
    yaml.default_flow_style = False
    yaml.width = 4096  # Prevent line wrapping
    
    with open(CONFIG_FILE, 'r') as f:
        config = yaml.load(f)
    
    # Find main code segment
    segments = config.get('segments', [])
    main_segment = None
    for segment in segments:
        if isinstance(segment, dict) and segment.get('type') == 'code' and segment.get('name') == 'main':
            main_segment = segment
            break
    
    if not main_segment:
        error("Could not find 'main' code segment in config")
    
    subsegments = main_segment.get('subsegments', [])
    
    # Check what segment currently contains this offset
    containing = get_segment_at_offset(subsegments, rom_offset)
    if containing:
        start, end, seg_type = containing
        if seg_type == 'c':
            error(f"Function is inside an existing 'c' segment [0x{start:X}, 0x{end:X}].\n"
                  f"       Cannot automatically split C segments.\n"
                  f"       You may need to manually edit the YAML or extend the C file.")
        info(f"Function is in segment [0x{start:X}, 0x{end:X}] type={seg_type}")
    
    # Find insertion point
    insert_idx, next_start = find_insertion_point(subsegments, rom_offset)
    
    # Check if segment already exists at this exact offset
    if next_start == rom_offset:
        info(f"Segment already exists at ROM offset 0x{rom_offset:X}")
        return True
    
    # Calculate end offset
    end_offset = rom_offset + size
    
    # Validate no overlap with next segment
    if next_start and end_offset > next_start:
        error(f"Function would overlap with next segment!\n"
              f"       Function: 0x{rom_offset:X} - 0x{end_offset:X} (size 0x{size:X})\n"
              f"       Next segment starts at: 0x{next_start:X}\n"
              f"       Check if the size in symbol_addrs.txt is correct.")
    
    if dry_run:
        info(f"Would insert: [0x{rom_offset:X}, asm, {name}]")
        info(f"Would insert: [0x{end_offset:X}, asm]")
        return True
    
    # Insert new segments using hex strings to match existing YAML style
    from ruamel.yaml.comments import CommentedSeq
    
    # Create hex-formatted integers using ruamel.yaml's HexInt
    class HexInt(int):
        """Integer that formats as hex in YAML."""
        pass
    
    def represent_hex_int(dumper, data):
        return dumper.represent_scalar('tag:yaml.org,2002:int', f'0x{data:X}')
    
    yaml.representer.add_representer(HexInt, represent_hex_int)
    
    # Create flow-style sequences (compact [a, b, c] format)
    new_func_segment = CommentedSeq([HexInt(rom_offset), "asm", name])
    new_func_segment.fa.set_flow_style()
    
    new_end_segment = CommentedSeq([HexInt(end_offset), "asm"])
    new_end_segment.fa.set_flow_style()
    
    # Insert in order (function first, then end marker)
    subsegments.insert(insert_idx, new_func_segment)
    subsegments.insert(insert_idx + 1, new_end_segment)
    
    # Write back
    with open(CONFIG_FILE, 'w') as f:
        yaml.dump(config, f)
    
    success(f"Updated {CONFIG_FILE}")
    info(f"Added: [0x{rom_offset:X}, asm, {name}]")
    info(f"Added: [0x{end_offset:X}, asm]")
    return True


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
            timeout=120
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


def run_m2c_check(name: str, dry_run: bool = False) -> bool:
    """
    Run m2c on the generated ASM to verify it decompiles to a valid function.
    
    This catches issues like:
    - Malformed ASM that m2c can't parse
    - Missing context types
    - Functions that are actually data
    """
    asm_file = Path(f"asm/pal/{name}.s")
    ctx_file = Path("ctx.c")
    
    if dry_run:
        info(f"Would run m2c on {asm_file}")
        return True
    
    if not asm_file.exists():
        error(f"ASM file not found: {asm_file}\n"
              f"       Did splat run correctly?")
    
    if not ctx_file.exists():
        info("Context file ctx.c not found, generating...")
        try:
            result = subprocess.run(
                ["make", "context"],
                capture_output=True,
                text=True,
                timeout=30
            )
            if result.returncode != 0:
                error(f"Failed to generate ctx.c: {result.stderr}")
        except Exception as e:
            error(f"Failed to run make context: {e}")
    
    info(f"Running m2c on {asm_file}...")
    try:
        result = subprocess.run(
            ["python3", "tools/m2c/m2c.py", 
             "--context", str(ctx_file),
             "--target", "mipsel-gcc-c",
             str(asm_file)],
            capture_output=True,
            text=True,
            timeout=30
        )
        
        output = result.stdout
        
        # Check for common m2c failure patterns
        if result.returncode != 0:
            print(result.stderr, file=sys.stderr)
            error(f"m2c failed with exit code {result.returncode}")
        
        # Check that output looks like a C function
        if not output.strip():
            error("m2c produced empty output - ASM may be malformed")
        
        # Look for function signature pattern (return type + name + parens)
        # m2c output typically starts with the function like: "? func_name(..."
        has_function = False
        for line in output.split('\n'):
            line = line.strip()
            # Skip empty lines and comments
            if not line or line.startswith('//') or line.startswith('/*'):
                continue
            # Check for function-like pattern: something followed by name(
            if f"{name}(" in line or "(" in line:
                has_function = True
                break
        
        if not has_function:
            print(output)
            error("m2c output doesn't appear to contain a function.\n"
                  f"       The ASM at {asm_file} may be data, not code.")
        
        # Show a preview of the decompiled code
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


def main():
    parser = argparse.ArgumentParser(
        description="Minimal PSX decompilation workflow (no Ghidra required)",
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
                        help="Skip make check verification step")
    
    args = parser.parse_args()
    
    print(f"\n{'='*60}")
    print(f"Preparing: {args.function}")
    print(f"{'='*60}\n")
    
    # Step 1: Look up function in symbol_addrs.txt
    info(f"Looking up '{args.function}' in {SYMBOL_FILE}...")
    result = parse_symbol_addrs(args.function)
    
    if not result:
        error(f"Function '{args.function}' not found in {SYMBOL_FILE}\n"
              f"       Make sure it exists with the format:\n"
              f"       {args.function} = 0xADDRESS; // size:0xSIZE")
    
    address, size = result
    rom_offset = vram_to_rom(address)
    
    success(f"Found: {args.function}")
    info(f"VRAM address: 0x{address:08X}")
    info(f"ROM offset:   0x{rom_offset:X}")
    info(f"Size:         0x{size:X} ({size} bytes)")
    
    if size == 0:
        error(f"Function has size 0 in {SYMBOL_FILE}.\n"
              f"       You must determine and add the correct size first.")
    
    # Step 2: Update splat YAML
    print(f"\n{'='*60}")
    print("Updating splat configuration")
    print(f"{'='*60}\n")
    
    if not update_splat_yaml(args.function, rom_offset, size, args.dry_run):
        error("Failed to update splat configuration")
    
    # Step 3: Run make check
    if not args.no_check:
        print(f"\n{'='*60}")
        print("Verifying build")
        print(f"{'='*60}\n")
        
        if not run_make_check(args.dry_run):
            error("Build verification failed")
        
        # Step 4: Verify m2c can decompile the ASM
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
        print(f"  3. Change segment type from 'asm' to 'c' in {CONFIG_FILE}")
        print(f"  4. Run: make && make check")
    
    print()


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\nInterrupted by user", file=sys.stderr)
        sys.exit(130)
