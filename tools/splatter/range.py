#!/usr/bin/env python3
"""
Range - Add an address range as a named segment.

Adds a segment covering a VRAM address range to the splat configuration.
Does not decompile - just configures splat to extract the range.

Usage:
    # Add an ASM range
    python3 tools/splatter/range.py engine/sprites 0x80010000..0x80030000 --type=asm

    # Add a data range
    python3 tools/splatter/range.py engine/tables 0x800A0000..0x800A5000 --type=data

    # Preview without changes
    python3 tools/splatter/range.py engine/sprites 0x80010000..0x80030000 --dry-run

Examples:
    range.py engine/sprites 0x80010000..0x80030000 --type=asm
    range.py player/states 0x80060000..0x80065000 --type=c
    range.py data/strings 0x800A0000..0x800A2000 --type=rodata
"""

import sys
import argparse
import re
import subprocess
from pathlib import Path

# Add lib to path
sys.path.insert(0, str(Path(__file__).parent))

from lib.splat_config import SplatConfig, vram_to_rom, rom_to_vram


# Configuration
PROJECT_ROOT = Path(__file__).parent.parent.parent
CONFIG_FILE = PROJECT_ROOT / "config" / "splat.pal.yaml"


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


def parse_range(range_str: str) -> tuple:
    """
    Parse a range string like '0x80010000..0x80030000' or '0x80010000-0x80030000'.
    
    Returns (start, end) as integers.
    """
    # Try .. separator
    if '..' in range_str:
        parts = range_str.split('..')
    elif '-' in range_str and not range_str.startswith('-'):
        # Handle single dash, but not negative numbers
        # Find the dash that separates the two addresses
        match = re.match(r'(0x[0-9a-fA-F]+)-(0x[0-9a-fA-F]+)', range_str)
        if match:
            parts = [match.group(1), match.group(2)]
        else:
            error(f"Invalid range format: {range_str}")
            return (0, 0)
    else:
        error(f"Invalid range format: {range_str}\n"
              f"       Use format: 0xSTART..0xEND or 0xSTART-0xEND")
        return (0, 0)
    
    if len(parts) != 2:
        error(f"Invalid range format: {range_str}")
        return (0, 0)
    
    try:
        start = int(parts[0], 16) if parts[0].startswith('0x') else int(parts[0])
        end = int(parts[1], 16) if parts[1].startswith('0x') else int(parts[1])
    except ValueError as e:
        error(f"Invalid address in range: {e}")
        return (0, 0)
    
    if start >= end:
        error(f"Start address (0x{start:X}) must be less than end address (0x{end:X})")
    
    return (start, end)


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
        error("make not found")
    except Exception as e:
        error(f"Failed to run make check: {e}")
    return False


def main():
    parser = argparse.ArgumentParser(
        description="Add an address range as a named segment",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
    %(prog)s engine/sprites 0x80010000..0x80030000 --type=asm
    %(prog)s player/states 0x80060000..0x80065000 --type=c
    %(prog)s data/strings 0x800A0000..0x800A2000 --type=rodata --dry-run
"""
    )
    parser.add_argument("name", help="Segment name (also used as file path, e.g., 'engine/sprites')")
    parser.add_argument("range", help="Address range in format 0xSTART..0xEND")
    parser.add_argument("--type", "-t", default="asm",
                        choices=['asm', 'c', 'data', 'rodata', 'sdata', 'bin'],
                        help="Segment type (default: asm)")
    parser.add_argument("--dry-run", action="store_true",
                        help="Show what would be done without making changes")
    parser.add_argument("--no-check", action="store_true",
                        help="Skip make check verification step")
    
    args = parser.parse_args()
    
    # Parse the range
    start_addr, end_addr = parse_range(args.range)
    
    print(f"\n{'='*60}")
    print(f"Adding range: {args.name}")
    print(f"{'='*60}\n")
    
    info(f"Name:  {args.name}")
    info(f"Type:  {args.type}")
    info(f"Start: 0x{start_addr:08X} (ROM: 0x{vram_to_rom(start_addr):X})")
    info(f"End:   0x{end_addr:08X} (ROM: 0x{vram_to_rom(end_addr):X})")
    info(f"Size:  0x{end_addr - start_addr:X} ({end_addr - start_addr} bytes)")
    
    # Load config
    config = SplatConfig(CONFIG_FILE)
    config.load()
    
    # Check what's currently at this range
    start_rom = vram_to_rom(start_addr)
    end_rom = vram_to_rom(end_addr)
    
    containing = config.find_segment_at(start_rom)
    if containing:
        if containing.seg_type == 'c':
            error(f"Range starts inside existing 'c' segment '{containing.name}' "
                  f"at 0x{containing.vram_start:08X}.\n"
                  f"       Cannot automatically split C segments.")
        info(f"Start is in segment [{containing.seg_type}] '{containing.name or '(unnamed)'}' at 0x{containing.vram_start:08X}")
    
    # Check for overlap with C segments
    overlap = config.check_overlap_with_c(start_rom, end_rom)
    if overlap:
        error(f"Range overlaps with C segment '{overlap.name}' at 0x{overlap.vram_start:08X}")
    
    # Add the segment
    print(f"\n{'='*60}")
    print("Updating splat configuration")
    print(f"{'='*60}\n")
    
    if args.dry_run:
        info(f"Would add segment (dictionary format):")
        print(f"    - name: {args.name}")
        print(f"      type: {args.type}")
        print(f"      start: 0x{start_rom:X}")
        info(f"Would add trailing segment:")
        print(f"    - type: asm")
        print(f"      start: 0x{end_rom:X}")
    else:
        added = config.add_segment(
            start=start_rom,
            seg_type=args.type,
            name=args.name,
            end=end_rom,
            use_dict_format=True
        )
        
        if added:
            config.save()
            success(f"Updated {CONFIG_FILE.name}")
            info(f"Added segment: {args.name}")
        else:
            info(f"Segment already exists at 0x{start_rom:X}")
    
    # Verify build
    if not args.no_check:
        print(f"\n{'='*60}")
        print("Verifying build")
        print(f"{'='*60}\n")
        
        if not run_make_check(args.dry_run):
            error("Build verification failed")
    
    # Done
    print(f"\n{'='*60}")
    print("Done!")
    print(f"{'='*60}")
    
    if args.dry_run:
        print("\n⚠️  Dry run - no changes were made")
    else:
        print(f"\nSegment '{args.name}' added.")
        print(f"Run 'make' to extract the segment to asm/pal/{args.name}.s")
    
    print()


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\nInterrupted by user", file=sys.stderr)
        sys.exit(130)
