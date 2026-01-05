#!/usr/bin/env python3
"""
BLB Asset Coverage Analysis

Shows which asset types we understand (can parse/extract) vs unknown.
Uses the AssetID definitions from blb.py to ensure consistency.

Run with: python3 scripts/blb_asset_coverage.py
"""
import struct
from pathlib import Path
from collections import defaultdict

# Add parent to path for imports
import sys
sys.path.insert(0, str(Path(__file__).parent))

from blb import BLBFile, parse_container_toc, HEADER_SIZE, SECTOR_SIZE

# Asset understanding status - aligned with blb.py AssetID and docs/blb-data-format.md
# "parsed" = can fully extract and view (sprites, palettes, tilemaps)
# "partial" = structure known but not all fields understood
# "unknown" = format not understood
ASSET_STATUS = {
    # Secondary Segment - Tile Data
    0x064: ("TileHeader", "partial", "36B, ~20B parsed (counts, colors, dimensions)"),
    0x065: ("TileHeader101", "unknown", "12B sparse, purpose unknown"),
    0x0C8: ("TilemapContainer", "parsed", "TOC + u16 tile index arrays per layer"),
    0x0C9: ("LayerEntries", "partial", "92B per layer, ~30B parsed (dims, scroll, type)"),
    0x12C: ("TilePixels", "parsed", "8bpp indexed pixel data, fully extracted"),
    0x12D: ("PaletteIndices", "parsed", "1 byte per tile palette assignment"),
    0x12E: ("TileFlags", "parsed", "1 byte per tile (SIZE_8X8, SEMI_TRANS, SKIP)"),
    0x12F: ("AnimatedTiles", "partial", "Animation lookup table, format partially known"),
    0x190: ("PaletteContainer", "parsed", "TOC + 512B palettes (256 × 16-bit PSX colors)"),
    0x191: ("PaletteAnim", "partial", "Palette animation timing data"),
    
    # Primary Segment - Level Geometry
    0x258: ("Geometry", "partial", "Level geometry/entity RLE sprites container"),
    0x259: ("Collision", "partial", "Collision/layout data, format partially known"),
    0x25A: ("LevelPalette", "parsed", "15-bit PSX colors, fully extracted"),
    
    # Tertiary Segment - Audio & Sprites
    0x1F4: ("SpriteMetadata", "partial", "Sprite positioning/animation data"),
    0x1F5: ("SpriteRLE", "partial", "Sprite RLE pixel data, format known"),
    0x1F6: ("Audio502", "unknown", "Audio configuration, format unknown"),
    0x1F7: ("Audio503", "unknown", "Audio configuration, format unknown"),
    0x1F8: ("Audio504", "unknown", "Rare (2 occurrences), format unknown"),
    0x2BC: ("SPUSamples", "partial", "ADPCM audio samples for SPU"),
}


def analyze_blb(blb_path: Path) -> dict:
    """Analyze asset coverage in a BLB file."""
    file_size = blb_path.stat().st_size
    asset_bytes = defaultdict(int)
    asset_counts = defaultdict(int)
    coverage_regions = []  # (start, end, status) tuples
    
    with BLBFile(blb_path) as blb:
        f = blb._file
        primary_bytes = 0
        secondary_bytes = 0
        tertiary_bytes = 0
        
        # Header is always parsed
        coverage_regions.append((0, HEADER_SIZE, "parsed"))
        
        for i in range(blb.header.level_count):
            try:
                level = blb.header.get_level_by_index(i)
                
                # Primary segment - partial (geometry/collision)
                if level.sector_count > 0:
                    prim_start = level.sector_offset * SECTOR_SIZE
                    prim_end = prim_start + level.sector_count * SECTOR_SIZE
                    primary_bytes += level.sector_count * SECTOR_SIZE
                    coverage_regions.append((prim_start, prim_end, "partial"))
                
                # Secondary base
                if level.secondary_count > 0:
                    sec_start = level.secondary_offset * SECTOR_SIZE
                    sec_end = sec_start + level.secondary_count * SECTOR_SIZE
                    secondary_bytes += level.secondary_count * SECTOR_SIZE
                    coverage_regions.append((sec_start, sec_end, "parsed"))
                
                # Secondary sub-blocks
                for j in range(5):
                    if j < len(level.sec_sub_counts) and level.sec_sub_counts[j] > 0:
                        sub_start = level.sec_sub_offsets[j] * SECTOR_SIZE
                        sub_end = sub_start + level.sec_sub_counts[j] * SECTOR_SIZE
                        secondary_bytes += level.sec_sub_counts[j] * SECTOR_SIZE
                        coverage_regions.append((sub_start, sub_end, "parsed"))
                
                # Tertiary sub-blocks - partial (sprites/audio)
                for j in range(level.tert_block_count):
                    if level.tert_sub_counts[j] > 0:
                        tert_start = level.tert_sub_offsets[j] * SECTOR_SIZE
                        tert_end = tert_start + level.tert_sub_counts[j] * SECTOR_SIZE
                        tertiary_bytes += level.tert_sub_counts[j] * SECTOR_SIZE
                        coverage_regions.append((tert_start, tert_end, "partial"))
                
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
        "coverage_regions": coverage_regions,
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
        ("parsed", "✓ Parsed (can extract/render)", "\033[92m"),
        ("partial", "◐ Partial (structure known, not all fields)", "\033[93m"),
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
                print(f"      0x{aid:03X} {info[0]:<18}: {size:>10,} bytes  ({count:3d}×)  {info[2]}")
        
        if status == "parsed":
            subtotal = result["parsed_bytes"]
            print(f"      {'':>4} {'(header)':<18}: {HEADER_SIZE:>10,} bytes")
        
        pct = 100 * subtotal / file_size if file_size > 0 else 0
        print(f"      {color}{'Subtotal:':<24} {subtotal:>10,} bytes ({pct:.1f}% of file)\033[0m")
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
    
    # Print visual bitmap
    print_coverage_bitmap(result)


def print_coverage_bitmap(result: dict) -> None:
    """Print ASCII bitmap showing file coverage."""
    print()
    print("=" * 70)
    print("FILE COVERAGE BITMAP")
    print("=" * 70)
    print()
    print("  Legend: \033[92m█\033[0m=Parsed  \033[93m▓\033[0m=Partial  \033[91m░\033[0m=Unknown  \033[90m·\033[0m=Unreferenced")
    print()
    
    file_size = result['file_size']
    
    # Build coverage map from regions
    # Each character represents ~1MB (or adjust for file size)
    width = 64  # characters per row
    bytes_per_char = file_size // (width * 16)  # 16 rows
    if bytes_per_char < 1024:
        bytes_per_char = 1024  # Minimum 1KB per char
    
    total_chars = (file_size + bytes_per_char - 1) // bytes_per_char
    rows = (total_chars + width - 1) // width
    
    # Get the coverage regions from result
    coverage_regions = result.get('coverage_regions', [])
    
    # Print header with MB markers
    mb_per_char = bytes_per_char / (1024 * 1024)
    chars_per_10mb = int(10 / mb_per_char) if mb_per_char > 0 else 10
    
    # Print scale
    print("  ", end="")
    for i in range(0, width, 8):
        mb = (i * bytes_per_char) / (1024 * 1024)
        print(f"{mb:>7.0f}MB", end=" ")
    print()
    
    # Print bitmap rows
    for row in range(rows):
        print("  ", end="")
        for col in range(width):
            char_idx = row * width + col
            byte_start = char_idx * bytes_per_char
            byte_end = min(byte_start + bytes_per_char, file_size)
            
            if byte_start >= file_size:
                print(" ", end="")
                continue
            
            # Determine what's in this region
            status = classify_region(byte_start, byte_end, coverage_regions)
            
            if status == "parsed":
                print("\033[92m█\033[0m", end="")
            elif status == "partial":
                print("\033[93m▓\033[0m", end="")
            elif status == "unknown":
                print("\033[91m░\033[0m", end="")
            else:  # unreferenced
                print("\033[90m·\033[0m", end="")
        
        # Row label
        row_start_mb = (row * width * bytes_per_char) / (1024 * 1024)
        print(f"  {row_start_mb:>5.1f} MB")
    
    print()
    print(f"  Each character = {bytes_per_char/1024:.0f} KB")
    print()


def classify_region(byte_start: int, byte_end: int, regions: list) -> str:
    """Classify a byte range based on coverage regions."""
    # Check what percentage of this range is covered by each type
    parsed = 0
    partial = 0
    unknown = 0
    
    for region in regions:
        r_start, r_end, r_status = region
        # Calculate overlap
        overlap_start = max(byte_start, r_start)
        overlap_end = min(byte_end, r_end)
        if overlap_start < overlap_end:
            overlap = overlap_end - overlap_start
            if r_status == "parsed":
                parsed += overlap
            elif r_status == "partial":
                partial += overlap
            elif r_status == "unknown":
                unknown += overlap
    
    total = byte_end - byte_start
    covered = parsed + partial + unknown
    
    if covered == 0:
        return "unreferenced"
    
    # Return dominant type
    if parsed >= partial and parsed >= unknown:
        return "parsed"
    elif partial >= unknown:
        return "partial"
    else:
        return "unknown"


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
