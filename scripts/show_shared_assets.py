#!/usr/bin/env python3
"""
show_shared_assets.py - Display shared assets (duplicate references) in BLB files

Shows which sub-TOC entries share the same underlying data, helping understand
the asset reuse patterns in Skullmonkeys levels.

Usage:
    python3 scripts/show_shared_assets.py                    # Show all levels
    python3 scripts/show_shared_assets.py --level PHRO       # Show specific level
    python3 scripts/show_shared_assets.py --blb GAME.BLB     # Show specific BLB
    python3 scripts/show_shared_assets.py --summary          # Just show counts
"""

import argparse
import sys
from pathlib import Path
from collections import defaultdict
from typing import Dict, List, Tuple, Optional

sys.path.insert(0, str(Path(__file__).parent))

from container import SegmentTOC, AssetSubTOC, is_container_type, CONTAINER_ASSET_TYPES
from blb import BLBFile

# Asset type names for display
ASSET_TYPE_NAMES = {
    0x064: "SpriteIndex",
    0x065: "SpriteIndex2",
    0x12C: "SpriteGfx",
    0x12D: "SpriteMeta1",
    0x12E: "SpriteMeta2",
    0x190: "Animation",
    0x191: "AnimConfig",
    0x258: "Geometry",
    0x259: "Collision",
    0x25A: "Palette",
}


def get_asset_name(asset_type: int) -> str:
    """Get human-readable name for an asset type."""
    return ASSET_TYPE_NAMES.get(asset_type, f"Type_{asset_type:03X}")


def find_shared_ranges(subtoc: AssetSubTOC) -> Dict[Tuple[int, int], List[int]]:
    """
    Find entries that share the same data range.
    
    Returns:
        Dict mapping (offset, size) -> list of entry indices
    """
    range_to_entries: Dict[Tuple[int, int], List[int]] = defaultdict(list)
    
    for entry in subtoc.entries:
        key = (entry.offset, entry.size)
        range_to_entries[key].append(entry.index)
    
    # Filter to only shared ranges (more than one entry)
    return {k: v for k, v in range_to_entries.items() if len(v) > 1}


def analyze_level(blb: BLBFile, level_idx: int, verbose: bool = False) -> Dict[str, any]:
    """
    Analyze shared assets in a level's segments.
    
    Returns:
        Dict with analysis results
    """
    level = blb.header.get_level_by_index(level_idx)
    if level is None:
        return {}
    
    result = {
        "level_id": level.level_id,
        "level_name": level.name,
        "segments": {}
    }
    
    # Analyze Primary segment
    if level.sector_offset > 0 and level.sector_count > 0:
        segment_data = blb.read_sectors(level.sector_offset, level.sector_count)
        result["segments"]["Primary"] = analyze_segment(segment_data, "Primary", verbose)
    
    # Analyze Secondary segment
    if level.secondary_offset > 0 and level.secondary_count > 0:
        segment_data = blb.read_sectors(level.secondary_offset, level.secondary_count)
        result["segments"]["Secondary"] = analyze_segment(segment_data, "Secondary", verbose)
    
    return result


def analyze_segment(segment_data: bytes, segment_name: str, verbose: bool = False) -> Dict[str, any]:
    """Analyze shared assets in a segment."""
    try:
        segment = SegmentTOC.from_bytes(segment_data, name=segment_name)
    except ValueError as e:
        return {"error": str(e)}
    
    result = {
        "entry_count": segment.count,
        "assets": {}
    }
    
    for entry in segment.entries:
        if not is_container_type(entry.field_a):
            continue  # Skip raw assets
        
        asset_name = get_asset_name(entry.field_a)
        asset_data = segment.get_entry_data(entry)
        
        try:
            subtoc = AssetSubTOC.from_bytes(asset_data, entry.field_a, expected_size=entry.size)
            shared_ranges = find_shared_ranges(subtoc)
            
            if shared_ranges:
                result["assets"][asset_name] = {
                    "type": f"0x{entry.field_a:03X}",
                    "total_entries": subtoc.count,
                    "shared_groups": len(shared_ranges),
                    "total_shared": sum(len(v) for v in shared_ranges.values()),
                    "groups": {}
                }
                
                for (offset, size), indices in sorted(shared_ranges.items()):
                    group_key = f"0x{offset:X}+{size}"
                    result["assets"][asset_name]["groups"][group_key] = {
                        "offset": offset,
                        "size": size,
                        "entries": indices,
                        "flags": [subtoc.entries[i].field_a for i in indices]
                    }
        except ValueError:
            pass
    
    return result


def print_level_analysis(analysis: Dict, summary_only: bool = False):
    """Print analysis results for a level."""
    level_id = analysis.get("level_id", "???")
    level_name = analysis.get("level_name", "Unknown")
    
    # Count total shared
    total_shared = 0
    total_groups = 0
    for seg_name, seg_data in analysis.get("segments", {}).items():
        for asset_name, asset_data in seg_data.get("assets", {}).items():
            total_shared += asset_data.get("total_shared", 0)
            total_groups += asset_data.get("shared_groups", 0)
    
    if summary_only:
        if total_shared > 0:
            print(f"  {level_id:4} {level_name[:25]:25} {total_groups:3} groups, {total_shared:4} shared entries")
        else:
            print(f"  {level_id:4} {level_name[:25]:25} (no shared assets)")
        return
    
    # Detailed output
    print(f"\n{'='*70}")
    print(f"Level: {level_id} - {level_name}")
    print(f"{'='*70}")
    
    if total_shared == 0:
        print("  No shared assets in this level.")
        return
    
    for seg_name, seg_data in analysis.get("segments", {}).items():
        assets = seg_data.get("assets", {})
        if not assets:
            continue
        
        print(f"\n  {seg_name} Segment:")
        print(f"  {'-'*50}")
        
        for asset_name, asset_data in assets.items():
            asset_type = asset_data["type"]
            total_entries = asset_data["total_entries"]
            shared_groups = asset_data["shared_groups"]
            total_shared = asset_data["total_shared"]
            
            print(f"\n    {asset_name} ({asset_type})")
            print(f"    Total entries: {total_entries}, Shared: {total_shared} in {shared_groups} groups")
            
            for group_key, group_data in asset_data["groups"].items():
                offset = group_data["offset"]
                size = group_data["size"]
                entries = group_data["entries"]
                flags = group_data["flags"]
                
                # Format entry list nicely
                if len(entries) <= 8:
                    entry_str = ", ".join(str(e) for e in entries)
                else:
                    entry_str = f"{entries[0]}, {entries[1]}, ... {entries[-2]}, {entries[-1]} ({len(entries)} entries)"
                
                # Check if flags are all the same or different
                unique_flags = set(flags)
                if len(unique_flags) == 1:
                    flag_str = f"flags=0x{flags[0]:08X}"
                else:
                    flag_str = f"flags={len(unique_flags)} different"
                
                print(f"      [{offset:5X}-{offset+size:5X}] ({size:6,} bytes) → entries: [{entry_str}] {flag_str}")


def main():
    parser = argparse.ArgumentParser(description="Display shared assets in BLB files")
    parser.add_argument("--blb", help="Specific BLB file to analyze")
    parser.add_argument("--level", help="Specific level ID to analyze (e.g., PHRO)")
    parser.add_argument("--summary", "-s", action="store_true", help="Show summary only")
    parser.add_argument("--verbose", "-v", action="store_true", help="Show more detail")
    args = parser.parse_args()
    
    project_root = Path(__file__).parent.parent
    blb_dir = project_root / "disks" / "blb"
    
    # Find BLB files
    if args.blb:
        blb_path = Path(args.blb)
        if not blb_path.exists():
            blb_path = blb_dir / args.blb
        if not blb_path.exists():
            print(f"Error: BLB file not found: {args.blb}", file=sys.stderr)
            sys.exit(1)
        blb_files = [blb_path]
    else:
        # Default to the main PAL BLB
        default_blb = blb_dir / "sles-01090.blb"
        if default_blb.exists():
            blb_files = [default_blb]
        else:
            blb_files = sorted(blb_dir.glob("*.blb")) + sorted(blb_dir.glob("*.BLB"))
            if blb_files:
                blb_files = [blb_files[0]]  # Just use first one
    
    if not blb_files:
        print("Error: No BLB files found", file=sys.stderr)
        sys.exit(1)
    
    for blb_path in blb_files:
        print(f"\n{'#'*70}")
        print(f"# BLB: {blb_path.name}")
        print(f"{'#'*70}")
        
        with BLBFile(blb_path) as blb:
            for level_idx in range(blb.header.level_count):
                level = blb.header.get_level_by_index(level_idx)
                if level is None:
                    continue
                
                # Apply level filter
                if args.level and level.level_id != args.level:
                    continue
                
                analysis = analyze_level(blb, level_idx, args.verbose)
                print_level_analysis(analysis, summary_only=args.summary)
    
    print()


if __name__ == "__main__":
    main()
