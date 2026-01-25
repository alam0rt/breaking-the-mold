#!/usr/bin/env python3
"""
Build a sprite ID mapping file from extracted BLB data.

Creates a JSON file mapping sprite IDs (in hex and decimal) to their
extracted PNG/GIF files, making it easy to correlate Ghidra entity
sprite IDs with actual sprite images.

Usage:
    python tools/scripts/build_sprite_mapping.py /tmp/extracted_blb -o /tmp/sprite_mapping.json
"""

import argparse
import json
import re
from pathlib import Path
from collections import defaultdict


def find_sprite_files(extracted_dir: Path) -> dict:
    """
    Find all sprite files in the extracted directory.
    
    Returns:
        dict: {sprite_id: {"hex": "0x...", "decimal": ..., "files": [...], "levels": [...]}}
    """
    sprites = defaultdict(lambda: {"files": [], "levels": set(), "segments": set()})
    
    # Match patterns:
    # sprite_NNNNNNNN_animXX.png (single frame)
    # sprite_NNNNNNNN_animXX_fYY.png (multi-frame)
    # sprite_NNNNNNNN_animXX.gif (animated)
    pattern = re.compile(r"sprite_(\d+)_anim(\d+)(?:_f(\d+))?\.(png|gif)$")
    
    for path in extracted_dir.rglob("sprite_*.*"):
        match = pattern.match(path.name)
        if not match:
            continue
        
        sprite_id = int(match.group(1))
        anim_num = int(match.group(2))
        frame_num = int(match.group(3)) if match.group(3) else None
        ext = match.group(4)
        
        # Get level/segment from path
        parts = path.parts
        level = None
        segment = None
        for i, p in enumerate(parts):
            # Level codes are 4 uppercase letters
            if len(p) == 4 and p.isupper():
                level = p
                # Segment is next directory
                if i + 1 < len(parts) - 1:
                    segment = parts[i + 1]
                break
        
        sprites[sprite_id]["files"].append(str(path.relative_to(extracted_dir)))
        if level:
            sprites[sprite_id]["levels"].add(level)
        if segment:
            sprites[sprite_id]["segments"].add(segment)
    
    # Convert sets to sorted lists and add hex representation
    result = {}
    for sprite_id, info in sprites.items():
        result[sprite_id] = {
            "hex": f"0x{sprite_id:08x}",
            "decimal": sprite_id,
            "levels": sorted(info["levels"]),
            "segments": sorted(info["segments"]),
            "file_count": len(info["files"]),
            "sample_file": info["files"][0] if info["files"] else None,
            "files": sorted(info["files"])[:10],  # Limit to 10 files for brevity
        }
    
    return result


def build_reverse_mapping(sprites: dict) -> dict:
    """
    Build a hex-to-decimal lookup table.
    
    Returns:
        dict: {"0x12345678": 305419896, ...}
    """
    return {info["hex"]: sprite_id for sprite_id, info in sprites.items()}


def main():
    parser = argparse.ArgumentParser(description="Build sprite ID mapping from extracted BLB")
    parser.add_argument("extracted_dir", type=Path, help="Path to extracted BLB directory")
    parser.add_argument("-o", "--output", type=Path, default=None,
                        help="Output JSON file (default: <extracted_dir>/sprite_mapping.json)")
    args = parser.parse_args()
    
    if not args.extracted_dir.is_dir():
        print(f"Error: {args.extracted_dir} is not a directory")
        return 1
    
    output_path = args.output or (args.extracted_dir / "sprite_mapping.json")
    
    print(f"Scanning {args.extracted_dir} for sprites...")
    sprites = find_sprite_files(args.extracted_dir)
    
    print(f"Found {len(sprites)} unique sprite IDs")
    
    # Build the mapping
    mapping = {
        "description": "Sprite ID mapping for Skullmonkeys (SLES-01090)",
        "note": "Use hex IDs to match Ghidra decompiled code, decimal to match extracted filenames",
        "sprite_count": len(sprites),
        "hex_to_decimal": build_reverse_mapping(sprites),
        "sprites": {str(k): v for k, v in sorted(sprites.items())},
    }
    
    # Write output
    output_path.write_text(json.dumps(mapping, indent=2))
    print(f"Wrote mapping to {output_path}")
    
    # Print some examples
    print("\nSample mappings (first 10):")
    for i, (sprite_id, info) in enumerate(sorted(sprites.items())[:10]):
        print(f"  {info['hex']:>12} = {sprite_id:>10d} ({', '.join(info['levels'][:3])})")
    
    return 0


if __name__ == "__main__":
    exit(main())
