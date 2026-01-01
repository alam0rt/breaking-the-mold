#!/usr/bin/env python3
"""
analyze_menu.py - Analyze MENU "Options" level data structure

This script examines the MENU level (index 0) data from the Skullmonkeys
BLB archive to understand how the Options menu is structured.

The MENU level has three data sections:
- Primary: Contains the main menu assets (sprites, layouts, etc.)
- Secondary: Contains additional assets
- Tertiary: Small additional data

Data Structure (Primary):
=========================
The primary data starts with a type 0x03 header followed by asset blocks.

Header (0x00-0x147):
- u32 type = 0x03
- u32 size = 0x258 (600)
- Table of contents entries (0x08-0x147)

Asset Blocks (0x14C onwards):
- Each block starts with magic 0x00180001
- Structure: magic(u32), dimensions(u32), size(u32), flags(u32)

Usage:
    python3 scripts/analyze_menu.py [--extract OUTPUT_DIR]
"""

import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))

from blb import BLBFile, SECTOR_SIZE


def parse_asset_header(data: bytes, offset: int) -> dict:
    """Parse an asset block header at the given offset."""
    if offset + 16 > len(data):
        return None
    
    magic, val1, val2, val3 = struct.unpack('<IIII', data[offset:offset+16])
    
    return {
        'offset': offset,
        'magic': magic,
        'val1': val1,  # Dimensions or type
        'size': val2,  # Size of asset data
        'flags': val3,  # Flags or palette data
    }


def find_asset_blocks(data: bytes) -> list:
    """Find all asset blocks with magic 0x00180001."""
    pattern = struct.pack('<I', 0x00180001)
    assets = []
    
    i = 0
    while i < len(data) - 16:
        if data[i:i+4] == pattern:
            asset = parse_asset_header(data, i)
            if asset:
                assets.append(asset)
                # Skip past this header to avoid re-detecting
                i += 16
                continue
        i += 1
    
    return assets


def analyze_toc(data: bytes, toc_start: int, toc_end: int) -> list:
    """Analyze the table of contents region."""
    entries = []
    offset = toc_start
    
    while offset < toc_end:
        if offset + 8 > len(data):
            break
        
        val1, val2 = struct.unpack('<II', data[offset:offset+8])
        
        # Skip empty entries
        if val1 != 0 or val2 != 0:
            entries.append({
                'toc_offset': offset,
                'value1': val1,
                'value2': val2,
            })
        
        offset += 8
    
    return entries


def analyze_menu_primary(data: bytes, verbose: bool = False) -> dict:
    """Analyze the MENU primary data structure."""
    result = {
        'size': len(data),
        'header': {},
        'toc': [],
        'assets': [],
    }
    
    # Parse header
    if len(data) < 8:
        return result
    
    type_val, size_val = struct.unpack('<II', data[0:8])
    result['header'] = {
        'type': type_val,
        'size': size_val,
    }
    
    # Parse TOC (0x08 to 0x148)
    result['toc'] = analyze_toc(data, 0x08, 0x148)
    
    # Find asset blocks
    result['assets'] = find_asset_blocks(data)
    
    return result


def print_analysis(analysis: dict, verbose: bool = False):
    """Print the analysis results."""
    print('=' * 70)
    print('MENU OPTIONS Data Analysis')
    print('=' * 70)
    print()
    
    print(f"Total size: {analysis['size']:,} bytes (0x{analysis['size']:X})")
    print()
    
    header = analysis['header']
    print('Header:')
    print(f"  Type: 0x{header.get('type', 0):X}")
    print(f"  Size/Count: 0x{header.get('size', 0):X} ({header.get('size', 0)})")
    print()
    
    toc = analysis['toc']
    print(f'Table of Contents: {len(toc)} entries')
    if verbose:
        for entry in toc[:20]:
            print(f"  @0x{entry['toc_offset']:04X}: 0x{entry['value1']:08X} 0x{entry['value2']:08X}")
        if len(toc) > 20:
            print(f"  ... and {len(toc) - 20} more entries")
    print()
    
    assets = analysis['assets']
    print(f'Asset Blocks: {len(assets)} found')
    print()
    print('Off      Magic    Val1     Size     Flags')
    print('-------- -------- -------- -------- --------')
    for asset in assets:
        print(f"0x{asset['offset']:04X}   "
              f"0x{asset['magic']:08X} "
              f"0x{asset['val1']:04X}     "
              f"0x{asset['size']:04X}     "
              f"0x{asset['flags']:08X}")
    
    print()
    print('Asset Type Analysis:')
    print()
    
    # Analyze val1 (dimensions/type)
    val1_counts = {}
    for asset in assets:
        v = asset['val1']
        val1_counts[v] = val1_counts.get(v, 0) + 1
    
    print('  Val1 (Dimensions/Type) distribution:')
    for v, count in sorted(val1_counts.items()):
        # Try to interpret as dimensions
        if v == 0x180:
            desc = "384 (possibly 384-byte header or 24x16?)"
        elif v == 0x3C:
            desc = "60 (possibly small sprite)"
        elif v == 0x528:
            desc = "1320 (possibly 22x60?)"
        elif v == 0x114:
            desc = "276 (possibly 23x12?)"
        elif v == 0x138:
            desc = "312 (possibly 26x12?)"
        elif v == 0xF0:
            desc = "240 (possibly 16x15 or 20x12?)"
        else:
            desc = f"{v} bytes"
        print(f"    0x{v:04X}: {count:2d} occurrences - {desc}")


def extract_assets(data: bytes, assets: list, output_dir: Path):
    """Extract individual asset blocks to files."""
    output_dir.mkdir(parents=True, exist_ok=True)
    
    for i, asset in enumerate(assets):
        # Calculate the data region for this asset
        offset = asset['offset']
        header_size = 16
        data_size = asset['size']
        
        # Extract the full asset (header + data)
        end = offset + header_size + data_size
        if end > len(data):
            end = len(data)
        
        asset_data = data[offset:end]
        
        filename = f"asset_{i:02d}_0x{offset:04X}.bin"
        (output_dir / filename).write_bytes(asset_data)
        print(f"  Extracted: {filename} ({len(asset_data)} bytes)")


def main():
    parser = argparse.ArgumentParser(
        description="Analyze MENU Options level data structure"
    )
    parser.add_argument(
        "--blb-path",
        default=None,
        help="Path to GAME.BLB file (default: disks/blb/GAME.BLB)"
    )
    parser.add_argument(
        "--extract",
        metavar="OUTPUT_DIR",
        help="Extract individual assets to the specified directory"
    )
    parser.add_argument(
        "-v", "--verbose",
        action="store_true",
        help="Verbose output"
    )
    
    args = parser.parse_args()
    
    # Find the BLB file
    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    
    if args.blb_path:
        blb_path = Path(args.blb_path)
    else:
        blb_path = project_root / "disks" / "blb" / "GAME.BLB"
    
    if not blb_path.exists():
        print(f"Error: BLB file not found: {blb_path}", file=sys.stderr)
        sys.exit(1)
    
    print(f"Analyzing: {blb_path}")
    print()
    
    # Open the BLB and read MENU level data
    with BLBFile(blb_path) as blb:
        # Get MENU level entry (index 0)
        menu_level = blb.header.level_entries[0]
        
        print(f"MENU Level Entry:")
        print(f"  Level ID: {menu_level.level_id!r}")
        print(f"  Name: {menu_level.name!r}")
        print()
        
        print(f"Data Locations:")
        print(f"  Primary:   0x{menu_level.byte_offset:08X} ({menu_level.byte_size:,} bytes)")
        print(f"  Secondary: 0x{menu_level.secondary_byte_offset:08X} ({menu_level.secondary_byte_size:,} bytes)")
        print(f"  Tertiary:  0x{menu_level.tertiary_byte_offset:08X} ({menu_level.tertiary_byte_size:,} bytes)")
        print()
        
        # Read primary data
        primary_data = blb.read_sectors(menu_level.sector_offset, menu_level.sector_count)
        
        # Analyze the primary data
        analysis = analyze_menu_primary(primary_data, args.verbose)
        print_analysis(analysis, args.verbose)
        
        # Extract assets if requested
        if args.extract:
            output_dir = Path(args.extract)
            print()
            print(f"Extracting assets to: {output_dir}")
            extract_assets(primary_data, analysis['assets'], output_dir)
    
    print()
    print("Analysis complete.")


if __name__ == "__main__":
    main()
