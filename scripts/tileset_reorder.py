#!/usr/bin/env python3
"""
tileset_reorder.py - Reorder tileset rows to find correct visual arrangement

This tool helps discover the correct tile ordering by letting you experiment
with different row arrangements of extracted tilesets.

Usage:
    python3 tileset_reorder.py <tileset.png> [options]

Options:
    --cols N          Tiles per row in source (default: 16)
    --output FILE     Output filename (default: reordered.png)
    --interleave N    Interleave every N rows (e.g., 2 = alternate odd/even)
    --reverse-rows    Reverse row order (bottom to top)
    --block-height N  Group rows into blocks of N, reorder blocks
    --pattern STR     Custom row pattern like "0,2,4,1,3,5" or "even,odd"
    --preview         Show interactive preview (requires pygame)

Examples:
    # Interleave odd and even rows
    python3 tileset_reorder.py output/tilesets/02_SCIE_tileset.png --interleave 2

    # Reverse all rows
    python3 tileset_reorder.py output/tilesets/02_SCIE_tileset.png --reverse-rows

    # Custom pattern - first 4 rows become 0,2,1,3
    python3 tileset_reorder.py output/tilesets/02_SCIE_tileset.png --pattern "0,2,1,3"

    # Group into blocks of 4 rows, interleave within each block
    python3 tileset_reorder.py output/tilesets/02_SCIE_tileset.png --block-height 4 --interleave 2
"""

import sys
from pathlib import Path
from PIL import Image
import argparse


def load_tileset(path: Path, tile_size: int = 16, cols: int = 16):
    """Load tileset and split into rows of tiles."""
    img = Image.open(path)
    width, height = img.size
    
    row_height = tile_size
    num_rows = height // row_height
    
    rows = []
    for r in range(num_rows):
        y = r * row_height
        row_img = img.crop((0, y, width, y + row_height))
        rows.append(row_img)
    
    return rows, img.size


def reorder_interleave(rows, n):
    """Interleave rows - collect every nth row together.
    n=2: all even rows, then all odd rows
    n=3: rows 0,3,6..., then 1,4,7..., then 2,5,8...
    """
    result = []
    for offset in range(n):
        for i in range(offset, len(rows), n):
            result.append(rows[i])
    return result


def reorder_deinterleave(rows, n):
    """Reverse of interleave - spread rows back out.
    Useful if tiles were stored interleaved.
    """
    result = [None] * len(rows)
    idx = 0
    for offset in range(n):
        for i in range(offset, len(rows), n):
            result[i] = rows[idx]
            idx += 1
    return result


def reorder_block_interleave(rows, block_height, interleave_n):
    """Group into blocks of block_height rows, interleave within each block."""
    result = []
    for block_start in range(0, len(rows), block_height):
        block = rows[block_start:block_start + block_height]
        reordered_block = reorder_interleave(block, interleave_n)
        result.extend(reordered_block)
    return result


def reorder_pattern(rows, pattern_str):
    """Apply custom pattern to rows.
    Pattern can be:
    - Comma-separated indices: "0,2,1,3,4,6,5,7"
    - Keywords: "even,odd" or "odd,even"
    - Repeat pattern with *: "0,2,1,3*" repeats for all rows
    """
    if pattern_str == "even,odd":
        return reorder_interleave(rows, 2)
    elif pattern_str == "odd,even":
        evens = rows[::2]
        odds = rows[1::2]
        return odds + evens
    
    # Check for repeat marker
    repeat = pattern_str.endswith("*")
    if repeat:
        pattern_str = pattern_str[:-1]
    
    indices = [int(x.strip()) for x in pattern_str.split(",")]
    
    if repeat:
        # Apply pattern repeatedly
        result = []
        pattern_len = len(indices)
        for block_start in range(0, len(rows), pattern_len):
            for offset in indices:
                idx = block_start + offset
                if idx < len(rows):
                    result.append(rows[idx])
        return result
    else:
        # Apply pattern once
        return [rows[i] for i in indices if i < len(rows)]


def assemble_rows(rows, width):
    """Combine row images into single tileset."""
    if not rows:
        return None
    
    row_height = rows[0].size[1]
    total_height = row_height * len(rows)
    
    result = Image.new('RGBA', (width, total_height), (0, 0, 0, 0))
    
    for i, row in enumerate(rows):
        result.paste(row, (0, i * row_height))
    
    return result


def generate_all_combinations(rows, output_dir: Path):
    """Generate common reordering combinations for analysis."""
    width = rows[0].size[0]
    output_dir.mkdir(parents=True, exist_ok=True)
    
    # Use ALL rows - no truncation
    combinations = [
        ("original", rows),
        ("interleave_2", reorder_interleave(rows, 2)),
        ("interleave_4", reorder_interleave(rows, 4)),
        ("interleave_8", reorder_interleave(rows, 8)),
        ("deinterleave_2", reorder_deinterleave(rows, 2)),
        ("deinterleave_4", reorder_deinterleave(rows, 4)),
        ("reversed", list(reversed(rows))),
        ("odd_even", reorder_pattern(rows, "odd,even")),
    ]
    
    print(f"Generating {len(combinations)} combinations ({len(rows)} rows each)...")
    for name, reordered in combinations:
        img = assemble_rows(reordered, width)
        path = output_dir / f"{name}.png"
        img.save(path)
        print(f"  {name}: {path}")
    
    return combinations


def main():
    parser = argparse.ArgumentParser(description="Reorder tileset rows")
    parser.add_argument("tileset", type=Path, help="Input tileset PNG")
    parser.add_argument("--cols", type=int, default=16, help="Tiles per row")
    parser.add_argument("--output", "-o", type=Path, help="Output filename")
    parser.add_argument("--interleave", type=int, help="Interleave every N rows")
    parser.add_argument("--deinterleave", type=int, help="De-interleave (reverse of interleave)")
    parser.add_argument("--reverse-rows", action="store_true", help="Reverse row order")
    parser.add_argument("--block-height", type=int, help="Process in blocks of N rows")
    parser.add_argument("--pattern", type=str, help="Custom row pattern")
    parser.add_argument("--all", action="store_true", help="Generate all common combinations")
    parser.add_argument("--tile-size", type=int, default=16, help="Tile size in pixels")
    
    args = parser.parse_args()
    
    if not args.tileset.exists():
        print(f"Error: File not found: {args.tileset}")
        sys.exit(1)
    
    print(f"Loading: {args.tileset}")
    rows, (width, height) = load_tileset(args.tileset, args.tile_size, args.cols)
    print(f"  Size: {width}x{height}, {len(rows)} rows")
    
    if args.all:
        # Generate all combinations
        output_dir = args.output or args.tileset.parent / f"{args.tileset.stem}_reordered"
        generate_all_combinations(rows, output_dir)
        return
    
    # Apply transformations
    result_rows = rows
    
    if args.pattern:
        result_rows = reorder_pattern(result_rows, args.pattern)
        print(f"  Applied pattern: {args.pattern}")
    
    if args.interleave:
        if args.block_height:
            result_rows = reorder_block_interleave(result_rows, args.block_height, args.interleave)
            print(f"  Applied block interleave: block={args.block_height}, n={args.interleave}")
        else:
            result_rows = reorder_interleave(result_rows, args.interleave)
            print(f"  Applied interleave: {args.interleave}")
    
    if args.deinterleave:
        result_rows = reorder_deinterleave(result_rows, args.deinterleave)
        print(f"  Applied de-interleave: {args.deinterleave}")
    
    if args.reverse_rows:
        result_rows = list(reversed(result_rows))
        print("  Reversed rows")
    
    # Assemble and save
    result = assemble_rows(result_rows, width)
    
    output_path = args.output or args.tileset.parent / f"{args.tileset.stem}_reordered.png"
    result.save(output_path)
    print(f"Saved: {output_path}")


if __name__ == "__main__":
    main()
