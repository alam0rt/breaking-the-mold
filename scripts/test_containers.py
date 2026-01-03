#!/usr/bin/env python3
"""
test_containers.py - Validate container structures in BLB files

This script tests that all container structures (segment TOCs and asset sub-TOCs)
are well-formed with no overlapping entries.

Usage:
    python3 scripts/test_containers.py                    # Test all BLBs
    python3 scripts/test_containers.py disks/blb/GAME.BLB # Test specific BLB
    python3 scripts/test_containers.py --level MENU       # Test specific level
    python3 scripts/test_containers.py --verbose          # Show all entries
"""

import argparse
import struct
import sys
from pathlib import Path
from typing import List, Tuple, Optional

# Add scripts directory to path
sys.path.insert(0, str(Path(__file__).parent))

from container import (
    Container, ContainerEntry, ValidationError,
    SegmentTOC, AssetSubTOC,
    is_container_type, validate_segment_with_assets,
    CONTAINER_ASSET_TYPES
)
from blb import BLBHeader, BLBFile, SECTOR_SIZE


def test_primary_segment(blb: BLBFile, level_idx: int, verbose: bool = False) -> List[ValidationError]:
    """
    Test the Primary segment for a specific level.
    
    Returns:
        List of validation errors
    """
    level = blb.header.get_level_by_index(level_idx)
    if level is None:
        return [ValidationError("LEVEL_NOT_FOUND", f"Level {level_idx} not found")]
    
    # Read primary segment data
    if level.sector_offset == 0 or level.sector_count == 0:
        return []  # No primary data
    
    segment_data = blb.read_sectors(level.sector_offset, level.sector_count)
    segment_name = f"Level[{level_idx}]({level.level_id})/Primary"
    
    if verbose:
        print(f"\n  {segment_name}:")
        print(f"    Sector offset: 0x{level.sector_offset:X}, count: {level.sector_count}")
        print(f"    Data size: {len(segment_data):,} bytes")
    
    errors = validate_segment_with_assets(segment_data, segment_name)
    
    if verbose and not errors:
        # Parse and show summary
        try:
            segment = SegmentTOC.from_bytes(segment_data, name=segment_name)
            print(f"    TOC entries: {segment.count}")
            for entry in segment.entries:
                is_container = "CONTAINER" if is_container_type(entry.field_a) else "RAW"
                print(f"      [{entry.index}] type=0x{entry.field_a:03X} size={entry.size:,} "
                      f"offset=0x{entry.offset:X} ({is_container})")
                
                # For container assets, show sub-TOC
                if is_container_type(entry.field_a):
                    asset_data = segment.get_entry_data(entry)
                    try:
                        asset = AssetSubTOC.from_bytes(asset_data, entry.field_a, 
                                                       expected_size=entry.size)
                        print(f"          Sub-entries: {asset.count}")
                    except ValueError as e:
                        print(f"          Parse error: {e}")
        except ValueError as e:
            print(f"    Parse error: {e}")
    
    return errors


def test_secondary_segment(blb: BLBFile, level_idx: int, verbose: bool = False) -> List[ValidationError]:
    """
    Test the Secondary segment for a specific level.
    """
    level = blb.header.get_level_by_index(level_idx)
    if level is None:
        return []
    
    if level.secondary_offset == 0 or level.secondary_count == 0:
        return []  # No secondary data
    
    segment_data = blb.read_sectors(level.secondary_offset, level.secondary_count)
    segment_name = f"Level[{level_idx}]({level.level_id})/Secondary"
    
    if verbose:
        print(f"\n  {segment_name}:")
        print(f"    Sector offset: 0x{level.secondary_offset:X}, count: {level.secondary_count}")
        print(f"    Data size: {len(segment_data):,} bytes")
    
    errors = validate_segment_with_assets(segment_data, segment_name)
    
    if verbose and not errors:
        try:
            segment = SegmentTOC.from_bytes(segment_data, name=segment_name)
            print(f"    TOC entries: {segment.count}")
            for entry in segment.entries:
                is_container = "CONTAINER" if is_container_type(entry.field_a) else "RAW"
                print(f"      [{entry.index}] type=0x{entry.field_a:03X} size={entry.size:,} "
                      f"offset=0x{entry.offset:X} ({is_container})")
        except ValueError as e:
            print(f"    Parse error: {e}")
    
    return errors


def test_blb_file(blb_path: Path, level_filter: Optional[str] = None, 
                  verbose: bool = False) -> Tuple[int, int, int, List[str]]:
    """
    Test all levels in a BLB file.
    
    Returns:
        (levels_tested, errors_found, duplicates_found, error_messages)
    """
    print(f"\nTesting: {blb_path.name}")
    print("=" * 60)
    
    all_errors = []
    all_duplicates = 0
    levels_tested = 0
    
    try:
        with BLBFile(blb_path) as blb:
            for level_idx in range(blb.header.level_count):
                level = blb.header.get_level_by_index(level_idx)
                if level is None:
                    continue
                
                # Apply level filter if specified
                if level_filter and level.level_id != level_filter:
                    continue
                
                levels_tested += 1
                level_errors = []
                level_duplicates = 0
                
                if verbose:
                    print(f"\nLevel {level_idx}: {level.level_id} - {level.name}")
                
                # Test Primary segment
                primary_errors = test_primary_segment(blb, level_idx, verbose)
                for err in primary_errors:
                    if err.error_type == "DUPLICATE":
                        level_duplicates += 1
                    else:
                        level_errors.append(err)
                
                # Test Secondary segment
                secondary_errors = test_secondary_segment(blb, level_idx, verbose)
                for err in secondary_errors:
                    if err.error_type == "DUPLICATE":
                        level_duplicates += 1
                    else:
                        level_errors.append(err)
                
                # TODO: Test Tertiary segments (more complex due to sub-blocks)
                
                all_duplicates += level_duplicates
                
                if level_errors:
                    for error in level_errors:
                        all_errors.append(f"  {level.level_id}: {error}")
                elif not verbose:
                    # Show brief status
                    dup_str = f" ({level_duplicates} dups)" if level_duplicates > 0 else ""
                    print(f"  [{level_idx:2}] {level.level_id:4} {level.name[:20]:20} ✓{dup_str}")
                else:
                    dup_str = f" ({level_duplicates} duplicate references)" if level_duplicates > 0 else ""
                    print(f"    Validation: OK{dup_str}")
    
    except Exception as e:
        all_errors.append(f"  File error: {e}")
    
    return levels_tested, len(all_errors), all_duplicates, all_errors


def find_blb_files(base_path: Path) -> List[Path]:
    """Find all BLB files in the disks directory."""
    blb_dir = base_path / "disks" / "blb"
    if not blb_dir.exists():
        return []
    
    return sorted(blb_dir.glob("*.blb")) + sorted(blb_dir.glob("*.BLB"))


def main():
    parser = argparse.ArgumentParser(
        description="Validate container structures in BLB files"
    )
    parser.add_argument(
        "blb_path",
        nargs="?",
        help="Path to BLB file (default: test all in disks/blb/)"
    )
    parser.add_argument(
        "--level",
        help="Test only specific level by ID (e.g., MENU, PHRO)"
    )
    parser.add_argument(
        "-v", "--verbose",
        action="store_true",
        help="Show detailed output"
    )
    
    args = parser.parse_args()
    
    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    
    total_levels = 0
    total_errors = 0
    total_duplicates = 0
    all_messages = []
    
    if args.blb_path:
        # Test specific file
        blb_path = Path(args.blb_path)
        if not blb_path.exists():
            print(f"Error: File not found: {blb_path}", file=sys.stderr)
            sys.exit(1)
        
        levels, errors, duplicates, messages = test_blb_file(blb_path, args.level, args.verbose)
        total_levels += levels
        total_errors += errors
        total_duplicates += duplicates
        all_messages.extend(messages)
    else:
        # Test all BLB files
        blb_files = find_blb_files(project_root)
        
        if not blb_files:
            print("No BLB files found in disks/blb/", file=sys.stderr)
            sys.exit(1)
        
        print(f"Found {len(blb_files)} BLB file(s)")
        
        for blb_path in blb_files:
            levels, errors, duplicates, messages = test_blb_file(blb_path, args.level, args.verbose)
            total_levels += levels
            total_errors += errors
            total_duplicates += duplicates
            all_messages.extend(messages)
    
    # Summary
    print("\n" + "=" * 60)
    print("SUMMARY")
    print("=" * 60)
    print(f"Levels tested:        {total_levels}")
    print(f"Errors found:         {total_errors}")
    print(f"Duplicate references: {total_duplicates} (intentional asset sharing)")
    
    if all_messages:
        print("\nErrors:")
        for msg in all_messages:
            print(msg)
        sys.exit(1)
    else:
        print("\nAll container structures are well-formed! ✓")
        sys.exit(0)


if __name__ == "__main__":
    main()
