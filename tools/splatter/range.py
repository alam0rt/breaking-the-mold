#!/usr/bin/env python3
"""
Range - Add named segments to splat configuration.

Splits the monolithic asm segment into named subsegments for better organization.
Similar to how soul-re organizes their splat config by module.

Usage:
    # Split at a boundary (names the chunk from START to the next segment)
    python3 tools/splatter/range.py core/graphics 0x80013268

    # Split with explicit end address  
    python3 tools/splatter/range.py core/graphics 0x80013268..0x80020000

    # Use list format instead of dict format
    python3 tools/splatter/range.py core/graphics 0x80013268 --list-format

    # Preview without changes
    python3 tools/splatter/range.py core/graphics 0x80013268 --dry-run

Examples:
    range.py core/graphics 0x80013268           # Split at start of graphics code
    range.py entity/core 0x8001A000             # Split at entity system
    range.py player/main 0x80059000             # Split at player code
    range.py psyq/libcd 0x80083000..0x8008F000  # PSY-Q library range
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


def parse_address_or_range(addr_str: str) -> tuple:
    """
    Parse an address or range string.
    
    Formats:
        0x80010000          -> (0x80010000, None)
        0x80010000..0x80020000 -> (0x80010000, 0x80020000)
        0x80010000-0x80020000  -> (0x80010000, 0x80020000)
    
    Returns (start, end) where end may be None.
    """
    # Check for range separator
    if '..' in addr_str:
        parts = addr_str.split('..')
    elif '-' in addr_str and not addr_str.startswith('-'):
        match = re.match(r'(0x[0-9a-fA-F]+)-(0x[0-9a-fA-F]+)', addr_str)
        if match:
            parts = [match.group(1), match.group(2)]
        else:
            # Single address with no range
            parts = [addr_str]
    else:
        parts = [addr_str]
    
    try:
        start = int(parts[0], 16) if parts[0].startswith('0x') else int(parts[0])
        end = None
        if len(parts) > 1:
            end = int(parts[1], 16) if parts[1].startswith('0x') else int(parts[1])
            if start >= end:
                error(f"Start address (0x{start:X}) must be less than end address (0x{end:X})")
    except ValueError as e:
        error(f"Invalid address: {e}")
        return (0, None)
    
    return (start, end)


def run_make_check(dry_run: bool = False) -> bool:
    """Run make clean && make check to verify the build still matches."""
    if dry_run:
        info("Would run: make clean && make check")
        return True
    
    info("Running make clean && make check...")
    try:
        result = subprocess.run(
            ["make", "clean"],
            capture_output=True,
            text=True,
            timeout=30,
            cwd=PROJECT_ROOT
        )
        if result.returncode != 0:
            print(result.stderr, file=sys.stderr)
            error("make clean failed")
        
        result = subprocess.run(
            ["make", "check"],
            capture_output=True,
            text=True,
            timeout=180,
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
        error("make check timed out after 180 seconds")
    except FileNotFoundError:
        error("make not found")
    except Exception as e:
        error(f"Failed to run make check: {e}")
    return False


def main():
    parser = argparse.ArgumentParser(
        description="Add named segments to splat configuration",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
This tool splits the monolithic 'asm' segment into named subsegments.

Address Format:
    0x80013268           Single address - names from here to next segment
    0x80013268..0x80020000  Range - explicit start and end

Examples:
    %(prog)s core/graphics 0x80013268           # Name graphics code section
    %(prog)s entity/core 0x8001A000..0x80030000 # Name entity system with range
    %(prog)s psyq/libcd 0x80083000              # Name PSY-Q library section
    %(prog)s --dry-run player/main 0x80059000  # Preview changes
"""
    )
    parser.add_argument("name", help="Segment name (also used as file path, e.g., 'core/graphics')")
    parser.add_argument("address", help="Start address (0x80XXXXXX) or range (0xSTART..0xEND)")
    parser.add_argument("--type", "-t", default="asm",
                        choices=['asm', 'c', 'data', 'rodata', 'sdata', 'bin', 'hasm'],
                        help="Segment type (default: asm)")
    parser.add_argument("--list-format", action="store_true",
                        help="Use list format [offset, type, name] instead of dict format")
    parser.add_argument("--dry-run", action="store_true",
                        help="Show what would be done without making changes")
    parser.add_argument("--no-check", action="store_true",
                        help="Skip make check verification step")
    
    args = parser.parse_args()
    
    # Parse the address/range
    start_addr, end_addr = parse_address_or_range(args.address)
    
    # Convert to ROM offsets
    start_rom = vram_to_rom(start_addr)
    end_rom = vram_to_rom(end_addr) if end_addr else None
    
    print(f"\n{'='*60}")
    print(f"Adding segment: {args.name}")
    print(f"{'='*60}\n")
    
    info(f"Name:  {args.name}")
    info(f"Type:  {args.type}")
    info(f"Start: 0x{start_addr:08X} (ROM: 0x{start_rom:X})")
    if end_addr:
        info(f"End:   0x{end_addr:08X} (ROM: 0x{end_rom:X})")
        info(f"Size:  0x{end_addr - start_addr:X} ({end_addr - start_addr} bytes)")
    else:
        info(f"End:   (next segment)")
    
    # Load config
    config = SplatConfig(CONFIG_FILE)
    config.load()
    
    # Check what's currently at this location
    containing = config.find_segment_at(start_rom)
    if containing:
        if containing.seg_type == 'c':
            error(f"Cannot split existing 'c' segment '{containing.name}' at 0x{containing.vram_start:08X}")
        info(f"Currently in: [{containing.seg_type}] '{containing.name or '(unnamed)'}' at 0x{containing.vram_start:08X}")
    
    # Check if segment already exists
    if config.segment_exists_at(start_rom):
        error(f"Segment already exists at ROM offset 0x{start_rom:X}")
    
    # Show what we'll do
    print(f"\n{'='*60}")
    print("Updating splat configuration")
    print(f"{'='*60}\n")
    
    use_dict = not args.list_format
    
    if args.dry_run:
        if use_dict:
            info(f"Would add segment (dict format):")
            print(f"  - name: {args.name}")
            print(f"    type: {args.type}")
            print(f"    start: 0x{start_rom:X}")
        else:
            info(f"Would add segment (list format):")
            print(f"  - [0x{start_rom:X}, {args.type}, {args.name}]")
        
        if end_rom:
            info(f"Would add continuation segment at 0x{end_rom:X}")
    else:
        # Add the segment
        subsegments = config.get_subsegments()
        if subsegments is None:
            error("No subsegments found in main segment")
        
        # Find insertion point
        insert_idx = config.find_insertion_index(start_rom)
        
        # Create and insert the new segment
        if use_dict:
            new_seg = config.create_dict_segment(start_rom, args.type, args.name)
        else:
            new_seg = config.create_list_segment(start_rom, args.type, args.name)
        
        subsegments.insert(insert_idx, new_seg)
        info(f"Inserted segment at index {insert_idx}")
        
        # If explicit end was given, add continuation segment  
        if end_rom and not config.segment_exists_at(end_rom):
            end_idx = config.find_insertion_index(end_rom)
            if use_dict:
                end_seg = config.create_dict_segment(end_rom, 'asm')
            else:
                end_seg = config.create_list_segment(end_rom, 'asm')
            subsegments.insert(end_idx, end_seg)
            info(f"Inserted continuation segment at index {end_idx}")
        
        config.save()
        success(f"Updated {CONFIG_FILE.name}")
    
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
        print(f"Run 'make' to regenerate asm/pal/{args.name}.s")
    
    print()


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\nInterrupted by user", file=sys.stderr)
        sys.exit(130)
