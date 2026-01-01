#!/usr/bin/env python3
"""
extract_blb.py - Extract all assets from a Skullmonkeys GAME.BLB archive

Extracts:
- Level data (primary, secondary, tertiary sections)
- Sector table entries (loading screens, etc.)
- Movie metadata (movies are external .STR files, so just metadata)

Usage:
    python3 scripts/extract_blb.py [BLB_PATH] [OUTPUT_DIR]
    
Defaults:
    BLB_PATH: disks/blb/GAME.BLB
    OUTPUT_DIR: extracted/
"""

import argparse
import json
import os
import re
import sys
from pathlib import Path

# Add scripts directory to path for imports
sys.path.insert(0, str(Path(__file__).parent))

from blb import BLBFile, BLBHeader, SECTOR_SIZE, HEADER_SIZE


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


def extract_levels(blb: BLBFile, output_dir: Path) -> int:
    """Extract all level data (primary, secondary, tertiary sections)."""
    levels_dir = output_dir / "levels"
    levels_dir.mkdir(parents=True, exist_ok=True)
    
    extracted_count = 0
    
    for level in blb.header.level_entries:
        # Create level directory
        level_name = sanitize_filename(level.name) if level.name else "unnamed"
        level_id = level.level_id or "UNKN"
        dir_name = f"{level.index:02d}_{level_id}_{level_name}"
        level_dir = levels_dir / dir_name
        level_dir.mkdir(exist_ok=True)
        
        print(f"  Level [{level.index:2d}] {level_id}: {level.name}")
        
        # Extract primary data
        if level.sector_count > 0:
            data = blb.read_sectors(level.sector_offset, level.sector_count)
            primary_path = level_dir / "primary.bin"
            primary_path.write_bytes(data)
            print(f"    - primary.bin: {len(data):,} bytes (sectors {level.sector_offset}-{level.sector_offset + level.sector_count - 1})")
            extracted_count += 1
        else:
            print(f"    - primary: skipped (0 sectors)")
        
        # Extract secondary data
        if level.secondary_count > 0:
            data = blb.read_sectors(level.secondary_offset, level.secondary_count)
            secondary_path = level_dir / "secondary.bin"
            secondary_path.write_bytes(data)
            print(f"    - secondary.bin: {len(data):,} bytes (sectors {level.secondary_offset}-{level.secondary_offset + level.secondary_count - 1})")
            extracted_count += 1
        else:
            print(f"    - secondary: skipped (0 sectors)")
        
        # Extract tertiary data
        if level.tertiary_count > 0:
            data = blb.read_sectors(level.tertiary_offset, level.tertiary_count)
            tertiary_path = level_dir / "tertiary.bin"
            tertiary_path.write_bytes(data)
            print(f"    - tertiary.bin: {len(data):,} bytes (sectors {level.tertiary_offset}-{level.tertiary_offset + level.tertiary_count - 1})")
            extracted_count += 1
        else:
            print(f"    - tertiary: skipped (0 sectors)")
    
    return extracted_count


def extract_sectors(blb: BLBFile, output_dir: Path) -> int:
    """Extract all sector table entries."""
    sectors_dir = output_dir / "sectors"
    sectors_dir.mkdir(parents=True, exist_ok=True)
    
    extracted_count = 0
    
    for entry in blb.header.sector_entries:
        if entry.sector_count == 0:
            print(f"  Sector [{entry.index:2d}] {entry.code}: skipped (0 sectors)")
            continue
        
        # Build filename
        code = entry.code or "UNKN"
        short = sanitize_filename(entry.short_name) if entry.short_name else ""
        if short:
            filename = f"{entry.index:02d}_{code}_{short}.bin"
        else:
            filename = f"{entry.index:02d}_{code}.bin"
        
        # Read and write data
        data = blb.read_entry(entry)
        output_path = sectors_dir / filename
        output_path.write_bytes(data)
        
        print(f"  Sector [{entry.index:2d}] {code}: {filename} ({len(data):,} bytes)")
        extracted_count += 1
    
    return extracted_count


def extract_movies(blb: BLBFile, output_dir: Path) -> int:
    """Export movie metadata (movies are external .STR files on CD)."""
    movies_dir = output_dir / "movies"
    movies_dir.mkdir(parents=True, exist_ok=True)
    
    movie_list = []
    
    for movie in blb.header.movie_entries:
        movie_info = {
            "index": movie.index,
            "movie_id": movie.movie_id,
            "short_name": movie.short_name,
            "filename": movie.filename,
            "sector_count": movie.sector_count,
            "estimated_size_bytes": movie.sector_count * SECTOR_SIZE,
        }
        movie_list.append(movie_info)
        print(f"  Movie [{movie.index:2d}] {movie.movie_id}: {movie.filename} ({movie.sector_count} sectors)")
    
    # Write movie index
    index_path = movies_dir / "movie_index.json"
    with open(index_path, 'w') as f:
        json.dump({
            "description": "Movie entries from BLB header - actual movies are .STR files on CD",
            "movies": movie_list
        }, f, indent=2)
    
    print(f"  Wrote movie_index.json with {len(movie_list)} entries")
    return len(movie_list)


def main():
    parser = argparse.ArgumentParser(
        description="Extract assets from a Skullmonkeys GAME.BLB archive"
    )
    parser.add_argument(
        "blb_path",
        nargs="?",
        default=None,
        help="Path to GAME.BLB file (default: disks/blb/GAME.BLB)"
    )
    parser.add_argument(
        "output_dir",
        nargs="?",
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
    
    if args.blb_path:
        blb_path = Path(args.blb_path)
    else:
        blb_path = project_root / "disks" / "blb" / "GAME.BLB"
    
    if args.output_dir:
        output_dir = Path(args.output_dir)
    else:
        output_dir = project_root / "extracted"
    
    # Check input file exists
    if not blb_path.exists():
        print(f"Error: BLB file not found: {blb_path}", file=sys.stderr)
        sys.exit(1)
    
    print(f"Extracting: {blb_path}")
    print(f"Output to:  {output_dir}")
    print()
    
    # Create output directory
    output_dir.mkdir(parents=True, exist_ok=True)
    
    # Open BLB file and extract
    with BLBFile(blb_path) as blb:
        header = blb.header
        
        print(f"Header info:")
        print(f"  Level count:        {header.level_count}")
        print(f"  Asset/movie count:  {header.asset_count}")
        print(f"  Sector table count: {header.sector_table_count}")
        print()
        
        # Extract levels
        print("=" * 60)
        print("Extracting Levels")
        print("=" * 60)
        level_count = extract_levels(blb, output_dir)
        print()
        
        # Extract sector table entries
        print("=" * 60)
        print("Extracting Sector Table Entries")
        print("=" * 60)
        sector_count = extract_sectors(blb, output_dir)
        print()
        
        # Export movie metadata
        print("=" * 60)
        print("Exporting Movie Metadata")
        print("=" * 60)
        movie_count = extract_movies(blb, output_dir)
        print()
    
    # Summary
    print("=" * 60)
    print("Extraction Complete")
    print("=" * 60)
    print(f"  Level files:   {level_count}")
    print(f"  Sector files:  {sector_count}")
    print(f"  Movie entries: {movie_count}")
    print(f"\nOutput directory: {output_dir}")


if __name__ == "__main__":
    main()
