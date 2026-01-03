#!/usr/bin/env python3
"""
show_asset_tree.py - Display BLB asset hierarchy as a directory tree

Shows the asset structure like a filesystem with shared assets displayed
as symbolic links pointing to their canonical location.

Usage:
    python3 scripts/show_asset_tree.py                    # Show all levels
    python3 scripts/show_asset_tree.py --level PHRO       # Show specific level
    python3 scripts/show_asset_tree.py --level MENU       # Show MENU level
"""

import argparse
import sys
from pathlib import Path
from collections import defaultdict
from typing import Dict, List, Tuple, Optional, Set

sys.path.insert(0, str(Path(__file__).parent))

from container import SegmentTOC, AssetSubTOC, is_container_type
from blb import BLBFile

# ANSI colors
class C:
    RESET = "\033[0m"
    BOLD = "\033[1m"
    DIM = "\033[2m"
    
    # Colors
    BLUE = "\033[34m"
    CYAN = "\033[36m"
    GREEN = "\033[32m"
    YELLOW = "\033[33m"
    MAGENTA = "\033[35m"
    RED = "\033[31m"
    WHITE = "\033[37m"
    
    # Bright colors
    BRIGHT_BLUE = "\033[94m"
    BRIGHT_CYAN = "\033[96m"
    BRIGHT_GREEN = "\033[92m"
    BRIGHT_YELLOW = "\033[93m"
    
    @classmethod
    def disable(cls):
        for attr in dir(cls):
            if not attr.startswith('_') and attr != 'disable':
                setattr(cls, attr, "")


# Tree drawing characters
TREE_PIPE = "│   "
TREE_TEE = "├── "
TREE_LAST = "└── "
TREE_SPACE = "    "

# Asset type display info
ASSET_INFO = {
    0x064: ("📋", "SpriteIndex", C.CYAN),
    0x065: ("📋", "SpriteIndex2", C.CYAN),
    0x12C: ("🖼️ ", "SpriteGfx", C.GREEN),
    0x12D: ("📊", "SpriteMeta1", C.DIM),
    0x12E: ("📊", "SpriteMeta2", C.DIM),
    0x190: ("🎬", "Animation", C.MAGENTA),
    0x191: ("⚙️ ", "AnimConfig", C.DIM),
    0x258: ("🏗️ ", "Geometry", C.YELLOW),
    0x259: ("💥", "Collision", C.RED),
    0x25A: ("🎨", "Palette", C.BRIGHT_CYAN),
}


def get_asset_info(asset_type: int) -> Tuple[str, str, str]:
    """Get (icon, name, color) for an asset type."""
    return ASSET_INFO.get(asset_type, ("📦", f"Type_{asset_type:03X}", C.WHITE))


def format_size(size: int) -> str:
    """Format size in human-readable form."""
    if size >= 1024 * 1024:
        return f"{size / 1024 / 1024:.1f}MB"
    elif size >= 1024:
        return f"{size / 1024:.1f}KB"
    else:
        return f"{size}B"


def print_tree_line(prefix: str, connector: str, text: str, color: str = "", suffix: str = ""):
    """Print a line of the tree."""
    print(f"{C.DIM}{prefix}{connector}{C.RESET}{color}{text}{C.RESET}{suffix}")


def show_level_tree(blb: BLBFile, level_idx: int, no_color: bool = False):
    """Display a level's assets as a tree structure."""
    if no_color:
        C.disable()
    
    level = blb.header.get_level_by_index(level_idx)
    if level is None:
        return
    
    # Level header
    print(f"\n{C.BOLD}{C.BRIGHT_YELLOW}📁 {level.level_id}{C.RESET} - {level.name}")
    
    segments = []
    
    # Collect Primary segment
    if level.sector_offset > 0 and level.sector_count > 0:
        segment_data = blb.read_sectors(level.sector_offset, level.sector_count)
        segments.append(("Primary", segment_data, len(segment_data)))
    
    # Collect Secondary segment  
    if level.secondary_offset > 0 and level.secondary_count > 0:
        segment_data = blb.read_sectors(level.secondary_offset, level.secondary_count)
        segments.append(("Secondary", segment_data, len(segment_data)))
    
    # TODO: Tertiary segments
    
    for seg_idx, (seg_name, seg_data, seg_size) in enumerate(segments):
        is_last_segment = (seg_idx == len(segments) - 1)
        seg_connector = TREE_LAST if is_last_segment else TREE_TEE
        seg_prefix = TREE_SPACE if is_last_segment else TREE_PIPE
        
        print_tree_line("", seg_connector, f"📂 {seg_name}/", C.BOLD + C.BLUE, 
                       f" {C.DIM}({format_size(seg_size)}){C.RESET}")
        
        try:
            segment = SegmentTOC.from_bytes(seg_data, name=seg_name)
        except ValueError as e:
            print_tree_line(seg_prefix, TREE_LAST, f"⚠️  Parse error: {e}", C.RED)
            continue
        
        # Process each asset in the segment
        for asset_idx, entry in enumerate(segment.entries):
            is_last_asset = (asset_idx == len(segment.entries) - 1)
            asset_connector = TREE_LAST if is_last_asset else TREE_TEE
            asset_prefix = seg_prefix + (TREE_SPACE if is_last_asset else TREE_PIPE)
            
            icon, name, color = get_asset_info(entry.field_a)
            size_str = format_size(entry.size)
            
            if is_container_type(entry.field_a):
                # Container asset - show as directory with contents
                print_tree_line(seg_prefix, asset_connector, 
                               f"{icon} {name}/", C.BOLD + color,
                               f" {C.DIM}(0x{entry.field_a:03X}, {size_str}){C.RESET}")
                
                # Parse sub-TOC and show entries
                try:
                    asset_data = segment.get_entry_data(entry)
                    subtoc = AssetSubTOC.from_bytes(asset_data, entry.field_a, 
                                                    expected_size=entry.size)
                    
                    # Find shared ranges to identify "symlinks"
                    range_to_first: Dict[Tuple[int, int], int] = {}
                    shared_targets: Set[Tuple[int, int]] = set()
                    
                    for sub_entry in subtoc.entries:
                        key = (sub_entry.offset, sub_entry.size)
                        if key in range_to_first:
                            shared_targets.add(key)
                        else:
                            range_to_first[key] = sub_entry.index
                    
                    # Display sub-entries
                    for sub_idx, sub_entry in enumerate(subtoc.entries):
                        is_last_sub = (sub_idx == len(subtoc.entries) - 1)
                        sub_connector = TREE_LAST if is_last_sub else TREE_TEE
                        
                        key = (sub_entry.offset, sub_entry.size)
                        sub_size_str = format_size(sub_entry.size)
                        flags_str = f"0x{sub_entry.field_a:08X}"
                        
                        if key in shared_targets and range_to_first[key] != sub_entry.index:
                            # This is a "symlink" to another entry
                            target_idx = range_to_first[key]
                            print_tree_line(asset_prefix, sub_connector,
                                           f"[{sub_idx:2}]", C.CYAN,
                                           f" {C.DIM}→ [{target_idx}] (shared, {sub_size_str}){C.RESET}")
                        else:
                            # Regular entry (or first occurrence of shared data)
                            marker = ""
                            if key in shared_targets:
                                marker = f" {C.BRIGHT_GREEN}★{C.RESET}"  # Mark canonical entries
                            
                            print_tree_line(asset_prefix, sub_connector,
                                           f"[{sub_idx:2}]", color,
                                           f" {C.DIM}{sub_size_str} @ 0x{sub_entry.offset:X} flags={flags_str}{C.RESET}{marker}")
                    
                except ValueError as e:
                    print_tree_line(asset_prefix, TREE_LAST, f"⚠️  Sub-TOC error: {e}", C.RED)
            else:
                # Raw asset - show as file
                print_tree_line(seg_prefix, asset_connector,
                               f"{icon} {name}", color,
                               f" {C.DIM}(0x{entry.field_a:03X}, {size_str}){C.RESET}")


def main():
    parser = argparse.ArgumentParser(description="Display BLB assets as a tree structure")
    parser.add_argument("--blb", help="Specific BLB file to analyze")
    parser.add_argument("--level", "-l", help="Specific level ID (e.g., PHRO, MENU)")
    parser.add_argument("--no-color", action="store_true", help="Disable colored output")
    parser.add_argument("--all", "-a", action="store_true", help="Show all levels")
    args = parser.parse_args()
    
    project_root = Path(__file__).parent.parent
    blb_dir = project_root / "disks" / "blb"
    
    # Find BLB file
    if args.blb:
        blb_path = Path(args.blb)
        if not blb_path.exists():
            blb_path = blb_dir / args.blb
    else:
        blb_path = blb_dir / "sles-01090.blb"
        if not blb_path.exists():
            blb_files = sorted(blb_dir.glob("*.blb")) + sorted(blb_dir.glob("*.BLB"))
            if blb_files:
                blb_path = blb_files[0]
    
    if not blb_path.exists():
        print(f"Error: BLB file not found: {blb_path}", file=sys.stderr)
        sys.exit(1)
    
    print(f"{C.BOLD}🎮 {blb_path.name}{C.RESET}")
    print(f"{C.DIM}{'─' * 60}{C.RESET}")
    print(f"{C.DIM}Legend: ★ = canonical (shared by others), → = link to shared{C.RESET}")
    
    with BLBFile(blb_path) as blb:
        if args.level:
            # Show specific level
            for level_idx in range(blb.header.level_count):
                level = blb.header.get_level_by_index(level_idx)
                if level and level.level_id == args.level:
                    show_level_tree(blb, level_idx, args.no_color)
                    break
            else:
                print(f"Error: Level '{args.level}' not found", file=sys.stderr)
                sys.exit(1)
        elif args.all:
            # Show all levels
            for level_idx in range(blb.header.level_count):
                show_level_tree(blb, level_idx, args.no_color)
        else:
            # Show just first few levels as example
            print(f"\n{C.DIM}Showing first 3 levels. Use --level XXXX or --all for more.{C.RESET}")
            for level_idx in range(min(3, blb.header.level_count)):
                show_level_tree(blb, level_idx, args.no_color)
    
    print()


if __name__ == "__main__":
    main()
