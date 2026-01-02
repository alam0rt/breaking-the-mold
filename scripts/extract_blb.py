#!/usr/bin/env python3
"""
extract_blb.py - Extract all assets from Skullmonkeys GAME.BLB archives

Creates an organized directory structure matching the BLB header layout:
    extracted/
    ├── <blb_name>/
    │   ├── header.bin                    # Raw 4096-byte header
    │   ├── header.json                   # Parsed header as JSON
    │   ├── levels/
    │   │   ├── 00_MENU/
    │   │   │   ├── metadata.json         # Level metadata
    │   │   │   ├── primary.bin           # Primary level data
    │   │   │   ├── secondary.bin         # Secondary data
    │   │   │   ├── secondary_sub_0.bin   # Secondary sub-blocks
    │   │   │   ├── tertiary_sub_0.bin    # Tertiary sub-blocks
    │   │   │   └── ...
    │   │   └── ...
    │   ├── loading_screens/
    │   │   ├── 00_MENU.bin               # Loading screen data
    │   │   └── ...
    │   ├── movies.json                   # Movie table (references external STR files)
    │   ├── credits.json                  # Credits sequence table
    │   ├── state_data.bin                # State/config region (0xF34-0xFFF)
    │   └── unknown_ed0.bin               # Unknown u32 array at 0xED0

Usage:
    python3 scripts/extract_blb.py disks/blb/GAME.BLB
    python3 scripts/extract_blb.py disks/blb/*.blb --output extracted/
"""

import argparse
import json
import os
import re
import sys
from pathlib import Path

# Add scripts directory to path for imports
sys.path.insert(0, str(Path(__file__).parent))

from blb import BLBFile, BLBHeader, LevelMetadataEntry, SECTOR_SIZE, HEADER_SIZE


def sanitize_filename(name: str) -> str:
    """Sanitize a string for use as a filename."""
    # Replace problematic characters with underscores
    name = re.sub(r'[<>:"/\\|?*\x00-\x1f]', '_', name)
    # Replace spaces and multiple underscores
    name = re.sub(r'\s+', '-', name)
    name = re.sub(r'_+', '_', name)
    name = re.sub(r'-+', '-', name)
    # Strip leading/trailing special chars
    name = name.strip('_- ')
    return name or "unnamed"


def level_to_dict(level: LevelMetadataEntry) -> dict:
    """Convert a level entry to a clean dictionary for JSON output."""
    return {
        "index": level.index,
        "level_id": level.level_id,
        "name": level.name,
        
        # Primary data
        "primary": {
            "sector_offset": level.sector_offset,
            "sector_count": level.sector_count,
            "byte_offset": level.byte_offset,
            "byte_size": level.byte_size,
        },
        
        # Unknown fields
        "unknown_04": level.unknown_04,
        "unknown_06": level.unknown_06,
        "entry1_offset_lo": level.entry1_offset_lo,
        "unknown_0A": level.unknown_0A,
        
        # Level identification
        "level_index": level.level_index,
        "level_flag": level.level_flag,
        
        # Tertiary configuration
        "tert_block_count": level.tert_block_count,
        "tert_data_offsets": level.tert_data_offsets,
        
        # Secondary data
        "secondary": {
            "sector_offset": level.secondary_offset,
            "sector_count": level.secondary_count,
            "byte_offset": level.secondary_offset * SECTOR_SIZE,
            "byte_size": level.secondary_count * SECTOR_SIZE,
            "sub_offsets": level.sec_sub_offsets,
            "sub_counts": level.sec_sub_counts,
        },
        
        # Tertiary sub-blocks
        "tertiary": {
            "sub_offsets": level.tert_sub_offsets,
            "sub_counts": level.tert_sub_counts,
        },
    }


def extract_levels(blb: BLBFile, output_dir: Path) -> int:
    """Extract all level data (primary, secondary, tertiary sections and sub-blocks)."""
    levels_dir = output_dir / "levels"
    levels_dir.mkdir(parents=True, exist_ok=True)
    
    extracted_count = 0
    
    for level in blb.header.level_entries:
        # Create level directory
        level_name = sanitize_filename(level.name) if level.name else "unnamed"
        level_id = level.level_id or "UNKN"
        dir_name = f"{level.index:02d}_{level_id}"
        level_dir = levels_dir / dir_name
        level_dir.mkdir(exist_ok=True)
        
        print(f"  Level [{level.index:2d}] {level_id}: {level.name}")
        
        # Write level metadata as JSON
        metadata_path = level_dir / "metadata.json"
        with open(metadata_path, 'w') as f:
            json.dump(level_to_dict(level), f, indent=2)
        
        # Extract primary data
        if level.sector_count > 0:
            data = blb.read_sectors(level.sector_offset, level.sector_count)
            primary_path = level_dir / "primary.bin"
            primary_path.write_bytes(data)
            print(f"    - primary.bin: {len(data):,} bytes (sectors {level.sector_offset}-{level.sector_offset + level.sector_count - 1})")
            extracted_count += 1
        
        # Extract secondary data
        if level.secondary_count > 0:
            data = blb.read_sectors(level.secondary_offset, level.secondary_count)
            secondary_path = level_dir / "secondary.bin"
            secondary_path.write_bytes(data)
            print(f"    - secondary.bin: {len(data):,} bytes (sectors {level.secondary_offset}-{level.secondary_offset + level.secondary_count - 1})")
            extracted_count += 1
        
        # Extract secondary sub-blocks
        if level.sec_sub_offsets and level.sec_sub_counts:
            for i, (offset, count) in enumerate(zip(level.sec_sub_offsets, level.sec_sub_counts)):
                if count > 0:
                    data = blb.read_sectors(offset, count)
                    sub_path = level_dir / f"secondary_sub_{i}.bin"
                    sub_path.write_bytes(data)
                    print(f"    - secondary_sub_{i}.bin: {len(data):,} bytes (sectors {offset}-{offset + count - 1})")
                    extracted_count += 1
        
        # Extract tertiary sub-blocks
        if level.tert_sub_offsets and level.tert_sub_counts:
            for i, (offset, count) in enumerate(zip(level.tert_sub_offsets, level.tert_sub_counts)):
                if count > 0:
                    data = blb.read_sectors(offset, count)
                    sub_path = level_dir / f"tertiary_sub_{i}.bin"
                    sub_path.write_bytes(data)
                    print(f"    - tertiary_sub_{i}.bin: {len(data):,} bytes (sectors {offset}-{offset + count - 1})")
                    extracted_count += 1
    
    return extracted_count


def extract_sectors(blb: BLBFile, output_dir: Path) -> int:
    """Extract all sector table entries (loading screens, etc.)."""
    sectors_dir = output_dir / "loading_screens"
    sectors_dir.mkdir(parents=True, exist_ok=True)
    
    extracted_count = 0
    sector_table = []
    
    for entry in blb.header.sector_entries:
        # Build entry info for JSON
        entry_info = {
            "index": entry.index,
            "code": entry.code,
            "short_name": entry.short_name,
            "level_index": entry.level_index,
            "entry_flags": entry.entry_flags,
            "unknown_byte": entry.unknown_byte,
            "sector_offset": entry.sector_offset,
            "sector_count": entry.sector_count,
            "byte_offset": entry.byte_offset,
            "byte_size": entry.byte_size,
        }
        sector_table.append(entry_info)
        
        if entry.sector_count == 0:
            continue
        
        # Build filename based on entry type
        code = entry.code or "UNKN"
        if entry.entry_flags == 0x03:  # Game over
            filename = f"gameover_{entry.index:02d}_{code}.bin"
        elif entry.entry_flags == 0x05:  # Special loading
            filename = f"special_{entry.index:02d}_{code}.bin"
        elif entry.level_index == 0x35:  # Credits
            filename = f"credits_{entry.index:02d}_{code}.bin"
        else:
            filename = f"{entry.index:02d}_{code}.bin"
        
        # Read and write data
        data = blb.read_entry(entry)
        output_path = sectors_dir / filename
        output_path.write_bytes(data)
        
        print(f"  Sector [{entry.index:2d}] {code}: {filename} ({len(data):,} bytes)")
        extracted_count += 1
    
    # Write sector table JSON
    table_path = sectors_dir / "sector_table.json"
    with open(table_path, 'w') as f:
        json.dump(sector_table, f, indent=2)
    
    return extracted_count


def extract_movies(blb: BLBFile, output_dir: Path) -> int:
    """Export movie metadata (movies are external .STR files on CD)."""
    movie_list = []
    
    for movie in blb.header.movie_entries:
        movie_info = {
            "index": movie.index,
            "movie_id": movie.movie_id,
            "short_name": movie.short_name,
            "filename": movie.filename,
            "sector_count": movie.sector_count,
            "estimated_size_bytes": movie.sector_count * SECTOR_SIZE,
            "unknown_00": movie.unknown_00,
        }
        movie_list.append(movie_info)
        print(f"  Movie [{movie.index:2d}] {movie.movie_id}: {movie.filename} ({movie.sector_count} sectors)")
    
    # Write movie index
    index_path = output_dir / "movies.json"
    with open(index_path, 'w') as f:
        json.dump({
            "description": "Movie entries from BLB header - actual movies are .STR files on CD",
            "count": len(movie_list),
            "movies": movie_list
        }, f, indent=2)
    
    print(f"  Wrote movies.json with {len(movie_list)} entries")
    return len(movie_list)


def extract_credits(blb: BLBFile, output_dir: Path) -> int:
    """Export credits sequence metadata."""
    credits_list = []
    
    for entry in blb.header.credits_entries:
        credits_info = {
            "index": entry.index,
            "code": entry.code,
            "param_a": entry.param_a,
            "param_b": entry.param_b,
        }
        credits_list.append(credits_info)
        print(f"  Credits [{entry.index}] {entry.code or '(empty)'}: param_a={entry.param_a}, param_b={entry.param_b}")
    
    # Write credits JSON
    credits_path = output_dir / "credits.json"
    with open(credits_path, 'w') as f:
        json.dump({
            "description": "Credits sequence table from BLB header",
            "count": len(credits_list),
            "entries": credits_list
        }, f, indent=2)
    
    print(f"  Wrote credits.json with {len(credits_list)} entries")
    return len(credits_list)


def extract_header_data(blb: BLBFile, output_dir: Path):
    """Extract raw header data and metadata."""
    header = blb.header
    
    # Raw header binary
    header_bin = output_dir / "header.bin"
    header_bin.write_bytes(header.raw_data)
    print(f"  Wrote header.bin ({len(header.raw_data):,} bytes)")
    
    # Unknown ED0 region
    if header.unknown_ed0:
        ed0_bin = output_dir / "unknown_ed0.bin"
        ed0_bin.write_bytes(header.unknown_ed0)
        print(f"  Wrote unknown_ed0.bin ({len(header.unknown_ed0):,} bytes)")
    
    # State data region
    if header.state_data and header.state_data.raw_data:
        state_bin = output_dir / "state_data.bin"
        state_bin.write_bytes(header.state_data.raw_data)
        print(f"  Wrote state_data.bin ({len(header.state_data.raw_data):,} bytes)")
    
    # Full header as JSON
    header_json = output_dir / "header.json"
    header_info = {
        "counts": {
            "level_count": header.level_count,
            "asset_count": header.asset_count,
            "sector_table_count": header.sector_table_count,
        },
        "levels": [level_to_dict(lvl) for lvl in header.level_entries],
        "state_data": {
            "game_mode": header.state_data.game_mode if header.state_data else None,
            "asset_index": header.state_data.asset_index if header.state_data else None,
        },
        "unknown_ed0_hex": header.unknown_ed0.hex() if header.unknown_ed0 else None,
    }
    with open(header_json, 'w') as f:
        json.dump(header_info, f, indent=2)
    print(f"  Wrote header.json")


def main():
    parser = argparse.ArgumentParser(
        description="Extract assets from Skullmonkeys BLB archives",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
    python3 scripts/extract_blb.py disks/blb/GAME.BLB
    python3 scripts/extract_blb.py disks/blb/*.blb --output extracted/
    python3 scripts/extract_blb.py disks/blb/sles-01090.blb -o out/pal/
        """
    )
    parser.add_argument(
        "blb_files",
        nargs="*",
        default=None,
        help="BLB file(s) to extract (default: disks/blb/GAME.BLB)"
    )
    parser.add_argument(
        "-o", "--output",
        type=Path,
        default=None,
        help="Output directory (default: extracted/)"
    )
    parser.add_argument(
        "-v", "--verbose",
        action="store_true",
        help="Verbose output"
    )
    
    args = parser.parse_args()
    
    # Resolve paths
    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    
    # Get BLB files to process
    if args.blb_files:
        blb_paths = [Path(p) for p in args.blb_files]
    else:
        blb_paths = [project_root / "disks" / "blb" / "GAME.BLB"]
    
    # Get output directory
    if args.output:
        base_output_dir = args.output
    else:
        base_output_dir = project_root / "extracted"
    
    # Process each BLB file
    for blb_path in blb_paths:
        if not blb_path.exists():
            print(f"Error: BLB file not found: {blb_path}", file=sys.stderr)
            continue
        
        # Create output directory named after the BLB file
        blb_name = blb_path.stem
        output_dir = base_output_dir / blb_name
        
        print(f"\n{'=' * 70}")
        print(f"Extracting: {blb_path}")
        print(f"Output to:  {output_dir}")
        print(f"{'=' * 70}")
        
        # Create output directory
        output_dir.mkdir(parents=True, exist_ok=True)
        
        try:
            # Open BLB file and extract
            with BLBFile(blb_path) as blb:
                header = blb.header
                
                print(f"\nHeader info:")
                print(f"  Level count:        {header.level_count}")
                print(f"  Asset/movie count:  {header.asset_count}")
                print(f"  Sector table count: {header.sector_table_count}")
                
                # Extract header data
                print(f"\n{'=' * 40}")
                print("Extracting Header Data")
                print(f"{'=' * 40}")
                extract_header_data(blb, output_dir)
                
                # Extract levels
                print(f"\n{'=' * 40}")
                print("Extracting Levels")
                print(f"{'=' * 40}")
                level_count = extract_levels(blb, output_dir)
                
                # Extract sector table entries (loading screens)
                print(f"\n{'=' * 40}")
                print("Extracting Loading Screens")
                print(f"{'=' * 40}")
                sector_count = extract_sectors(blb, output_dir)
                
                # Export movie metadata
                print(f"\n{'=' * 40}")
                print("Exporting Movie Metadata")
                print(f"{'=' * 40}")
                movie_count = extract_movies(blb, output_dir)
                
                # Export credits metadata
                print(f"\n{'=' * 40}")
                print("Exporting Credits Metadata")
                print(f"{'=' * 40}")
                credits_count = extract_credits(blb, output_dir)
                
                # Summary
                print(f"\n{'=' * 40}")
                print("Extraction Complete")
                print(f"{'=' * 40}")
                print(f"  Level data files:     {level_count}")
                print(f"  Loading screen files: {sector_count}")
                print(f"  Movie entries:        {movie_count}")
                print(f"  Credits entries:      {credits_count}")
                print(f"\n  Output: {output_dir}")
        
        except Exception as e:
            print(f"Error extracting {blb_path}: {e}", file=sys.stderr)
            if args.verbose:
                import traceback
                traceback.print_exc()
    
    return 0


if __name__ == "__main__":
    main()
