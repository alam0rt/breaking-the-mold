#!/usr/bin/env python3
"""
BLB Asset Coverage Analysis

Shows which asset types we understand (can parse/extract) vs unknown.
Run with: python3 scripts/blb_asset_coverage.py
"""
import struct
from pathlib import Path
from collections import defaultdict

# Add parent to path for imports
import sys
sys.path.insert(0, str(Path(__file__).parent))

from blb import BLBFile, parse_container_toc, HEADER_SIZE, SECTOR_SIZE

# Asset understanding status
# "parsed" = can fully extract and view (sprites, palettes)
# "partial" = structure known but not fully verified (tiles, audio)
# "unknown" = format not understood
ASSET_STATUS = {
    100: ("Level Metadata", "parsed", "36-byte config per level"),
    400: ("Sprite Palettes", "parsed", "256-color CLUT container"),
    600: ("RLE Sprites", "parsed", "RLE-encoded sprite container"),
    
    200: ("Background Tiles", "partial", "Large graphics data, likely tiles"),
    300: ("Tile/Collision Data", "partial", "Similar to 200, possibly collision maps"),
    500: ("Audio Data", "partial", "Sound effects or music"),
    601: ("SPU Audio Samples", "partial", "ADPCM samples uploaded to SPU via SpuRead()"),
    602: ("Audio Metadata", "partial", "Audio parameters for 601 samples"),
    
    101: ("Unknown", "unknown", ""),
    201: ("Tile Metadata?", "unknown", "Small, 26 occurrences"),
    301: ("Unknown", "unknown", ""),
    302: ("Unknown", "unknown", ""),
    401: ("Palette Metadata?", "unknown", "20 bytes typically"),
    501: ("Audio Metadata?", "unknown", ""),
    502: ("Unknown", "unknown", ""),
    503: ("Unknown", "unknown", ""),
    504: ("Unknown", "unknown", "Rare, 2 occurrences"),
    700: ("Unknown", "unknown", "Small entries"),
}


def analyze_blb(blb_path: Path) -> dict:
    """Analyze asset coverage in a BLB file."""
    file_size = blb_path.stat().st_size
    asset_bytes = defaultdict(int)
    asset_counts = defaultdict(int)
    
    with BLBFile(blb_path) as blb:
        f = blb._file
        primary_bytes = 0
        secondary_bytes = 0
        tertiary_bytes = 0
        
        for i in range(blb.header.level_count):
            try:
                level = blb.header.get_level_by_index(i)
                primary_bytes += level.sector_count * SECTOR_SIZE
                
                # Secondary base
                secondary_bytes += level.secondary_count * SECTOR_SIZE
                
                # Secondary sub-blocks
                for j in range(5):
                    if j < len(level.sec_sub_counts):
                        secondary_bytes += level.sec_sub_counts[j] * SECTOR_SIZE
                
                # Tertiary sub-blocks
                for j in range(level.tert_block_count):
                    tertiary_bytes += level.tert_sub_counts[j] * SECTOR_SIZE
                
                # Parse secondary container for asset breakdown
                if level.secondary_byte_size > 0:
                    f.seek(level.secondary_byte_offset)
                    sec_data = f.read(level.secondary_byte_size)
                    try:
                        for e in parse_container_toc(sec_data):
                            asset_bytes[e.asset_id] += e.size
                            asset_counts[e.asset_id] += 1
                    except:
                        pass
                
                # Parse tertiary containers
                for j in range(level.tert_block_count):
                    tert_off = level.get_tert_sub_byte_offset(j)
                    tert_size = level.get_tert_sub_byte_size(j)
                    if tert_size > 0:
                        f.seek(tert_off)
                        tert_data = f.read(tert_size)
                        try:
                            for e in parse_container_toc(tert_data):
                                asset_bytes[e.asset_id] += e.size
                                asset_counts[e.asset_id] += 1
                        except:
                            pass
            except:
                pass
    
    # Calculate referenced vs unreferenced
    referenced_bytes = HEADER_SIZE + primary_bytes + secondary_bytes + tertiary_bytes
    unreferenced_bytes = file_size - referenced_bytes
    
    # Calculate totals by status
    parsed = HEADER_SIZE
    partial = 0
    unknown = 0
    
    for aid, size in asset_bytes.items():
        status_info = ASSET_STATUS.get(aid, ("Unknown", "unknown", ""))
        status = status_info[1]
        if status == "parsed":
            parsed += size
        elif status == "partial":
            partial += size
        else:
            unknown += size
    
    total = parsed + partial + unknown
    
    return {
        "file_size": file_size,
        "asset_bytes": dict(asset_bytes),
        "asset_counts": dict(asset_counts),
        "primary_bytes": primary_bytes,
        "secondary_bytes": secondary_bytes,
        "tertiary_bytes": tertiary_bytes,
        "referenced_bytes": referenced_bytes,
        "unreferenced_bytes": unreferenced_bytes,
        "parsed_bytes": parsed,
        "partial_bytes": partial,
        "unknown_bytes": unknown,
        "total_asset_bytes": total,
    }


def print_report(result: dict) -> None:
    """Print formatted coverage report."""
    print("=" * 70)
    print("GAME.BLB Asset Coverage Report")
    print("=" * 70)
    file_size = result['file_size']
    print(f"File size: {file_size:,} bytes ({file_size/1024/1024:.1f} MB)")
    print()
    
    total = result["total_asset_bytes"]
    
    # Group by status
    for status, label, color in [
        ("parsed", "✓ Parsed (can extract/view)", "\033[92m"),
        ("partial", "◐ Partial (structure known)", "\033[93m"),
        ("unknown", "✗ Unknown (format unclear)", "\033[91m"),
    ]:
        print(f"  {color}{label}:\033[0m")
        
        subtotal = 0
        for aid in sorted(result["asset_bytes"].keys()):
            info = ASSET_STATUS.get(aid, ("Unknown", "unknown", ""))
            if info[1] == status:
                size = result["asset_bytes"][aid]
                count = result["asset_counts"][aid]
                subtotal += size
                print(f"      Asset {aid:3d} ({info[0][:12]:<12}): {size:>10,} bytes ({count} occurrences)")
        
        if status == "parsed":
            subtotal = result["parsed_bytes"]
            print(f"      (includes {HEADER_SIZE:,} byte header)")
        
        pct = 100 * subtotal / file_size if file_size > 0 else 0
        print(f"      {color}Subtotal: {subtotal:>10,} bytes ({pct:.1f}% of file)\033[0m")
        print()
    
    # Calculate what we've accounted for
    primary = result['primary_bytes']
    secondary = result['secondary_bytes']
    tertiary = result['tertiary_bytes']
    referenced = result['referenced_bytes']
    unreferenced = result['unreferenced_bytes']
    
    print("-" * 70)
    print("DATA REGIONS:")
    print(f"  Header:          {HEADER_SIZE:>12,} bytes ({100*HEADER_SIZE/file_size:.1f}%)")
    print(f"  Primary:         {primary:>12,} bytes ({100*primary/file_size:.1f}%)")
    print(f"  Secondary:       {secondary:>12,} bytes ({100*secondary/file_size:.1f}%)")
    print(f"  Tertiary:        {tertiary:>12,} bytes ({100*tertiary/file_size:.1f}%)")
    print(f"  Unreferenced:    {unreferenced:>12,} bytes ({100*unreferenced/file_size:.1f}%)")
    print()
    
    print("=" * 70)
    print("SUMMARY (vs full file)")
    print("=" * 70)
    
    print(f"  \033[92mReferenced data:   {100*referenced/file_size:5.1f}% ({referenced/1024/1024:.1f} MB)\033[0m")
    print(f"  \033[91mUnreferenced data: {100*unreferenced/file_size:5.1f}% ({unreferenced/1024/1024:.1f} MB)\033[0m")
    print()
    print("  Unreferenced includes: title screen graphics, bonus room containers")


def main():
    blb_path = Path(__file__).parent.parent / "disks" / "blb" / "GAME.BLB"
    
    if len(sys.argv) > 1:
        blb_path = Path(sys.argv[1])
    
    if not blb_path.exists():
        print(f"Error: {blb_path} not found")
        sys.exit(1)
    
    result = analyze_blb(blb_path)
    print_report(result)


if __name__ == "__main__":
    main()
