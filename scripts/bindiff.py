#!/usr/bin/env python3
"""
Binary diff tool for PSX decompilation.
Compares built binary against original, showing byte-level differences.

Usage:
    python3 scripts/bindiff.py build/pal/game.bin disks/pal/SLES_010.90
    python3 scripts/bindiff.py build/pal/game.bin disks/pal/SLES_010.90 --context 16
"""

import sys
import argparse
from pathlib import Path


def read_binary(path: Path) -> bytes:
    with open(path, 'rb') as f:
        return f.read()


def format_hex_line(offset: int, data: bytes, highlight_indices: set = None) -> str:
    """Format a line of hex dump with optional highlighting."""
    hex_parts = []
    ascii_parts = []
    
    for i, b in enumerate(data):
        if highlight_indices and i in highlight_indices:
            hex_parts.append(f"\033[91m{b:02X}\033[0m")  # Red
        else:
            hex_parts.append(f"{b:02X}")
        
        if 32 <= b < 127:
            char = chr(b)
        else:
            char = '.'
        
        if highlight_indices and i in highlight_indices:
            ascii_parts.append(f"\033[91m{char}\033[0m")
        else:
            ascii_parts.append(char)
    
    hex_str = ' '.join(hex_parts)
    ascii_str = ''.join(ascii_parts)
    
    return f"{offset:08X}  {hex_str:<48}  |{ascii_str}|"


def find_differences(data1: bytes, data2: bytes) -> list:
    """Find all byte positions where the two binaries differ."""
    diffs = []
    min_len = min(len(data1), len(data2))
    
    for i in range(min_len):
        if data1[i] != data2[i]:
            diffs.append(i)
    
    return diffs


def show_diff_region(data1: bytes, data2: bytes, offset: int, context: int = 16):
    """Show a diff region with context."""
    start = max(0, (offset // 16) * 16 - context)
    end = min(min(len(data1), len(data2)), (offset // 16 + 1) * 16 + context)
    
    print(f"\n\033[1m--- Difference at 0x{offset:08X} ---\033[0m")
    
    for line_offset in range(start, end, 16):
        chunk1 = data1[line_offset:line_offset + 16]
        chunk2 = data2[line_offset:line_offset + 16]
        
        # Find differing bytes in this line
        diff_indices = set()
        for i in range(min(len(chunk1), len(chunk2))):
            if chunk1[i] != chunk2[i]:
                diff_indices.add(i)
        
        if diff_indices:
            print(f"\033[33mBuild:\033[0m    {format_hex_line(line_offset, chunk1, diff_indices)}")
            print(f"\033[36mOriginal:\033[0m {format_hex_line(line_offset, chunk2, diff_indices)}")
        else:
            print(f"          {format_hex_line(line_offset, chunk1)}")


def main():
    parser = argparse.ArgumentParser(description='Compare two binary files')
    parser.add_argument('build', help='Built binary path')
    parser.add_argument('original', help='Original binary path')
    parser.add_argument('--context', '-c', type=int, default=32, 
                        help='Bytes of context around differences (default: 32)')
    parser.add_argument('--max-diffs', '-m', type=int, default=10,
                        help='Maximum number of diff regions to show (default: 10)')
    parser.add_argument('--all', '-a', action='store_true',
                        help='Show all differences (no limit)')
    parser.add_argument('--summary', '-s', action='store_true',
                        help='Only show summary, no details')
    args = parser.parse_args()
    
    build_path = Path(args.build)
    orig_path = Path(args.original)
    
    if not build_path.exists():
        print(f"Error: Build file not found: {build_path}")
        sys.exit(1)
    
    if not orig_path.exists():
        print(f"Error: Original file not found: {orig_path}")
        sys.exit(1)
    
    build_data = read_binary(build_path)
    orig_data = read_binary(orig_path)
    
    # Header
    print("=" * 70)
    print("Binary Comparison")
    print("=" * 70)
    print(f"Build:    {build_path} ({len(build_data):,} bytes)")
    print(f"Original: {orig_path} ({len(orig_data):,} bytes)")
    
    size_diff = len(build_data) - len(orig_data)
    if size_diff != 0:
        print(f"Size difference: {size_diff:+,} bytes")
    
    # Find differences
    diffs = find_differences(build_data, orig_data)
    
    # Check for size mismatch
    if len(build_data) != len(orig_data):
        extra_start = min(len(build_data), len(orig_data))
        extra_end = max(len(build_data), len(orig_data))
        if len(build_data) > len(orig_data):
            print(f"\nBuild has {extra_end - extra_start} extra bytes at end (0x{extra_start:X} - 0x{extra_end:X})")
        else:
            print(f"\nOriginal has {extra_end - extra_start} extra bytes at end (0x{extra_start:X} - 0x{extra_end:X})")
    
    if not diffs and len(build_data) == len(orig_data):
        print("\n\033[92m✓ Files are identical!\033[0m")
        sys.exit(0)
    
    print(f"\nFound {len(diffs):,} differing bytes in shared region")
    
    if args.summary:
        if diffs:
            print(f"First diff at: 0x{diffs[0]:08X}")
            print(f"Last diff at:  0x{diffs[-1]:08X}")
        sys.exit(1 if diffs else 0)
    
    # Group consecutive diffs into regions
    regions = []
    if diffs:
        region_start = diffs[0]
        region_end = diffs[0]
        
        for diff in diffs[1:]:
            if diff <= region_end + 32:  # Merge if within 32 bytes
                region_end = diff
            else:
                regions.append((region_start, region_end))
                region_start = diff
                region_end = diff
        
        regions.append((region_start, region_end))
    
    print(f"Grouped into {len(regions)} diff region(s)")
    
    # Show diff regions
    max_to_show = len(regions) if args.all else min(args.max_diffs, len(regions))
    
    for i, (start, end) in enumerate(regions[:max_to_show]):
        show_diff_region(build_data, orig_data, start, args.context)
    
    if not args.all and len(regions) > args.max_diffs:
        print(f"\n... and {len(regions) - args.max_diffs} more diff region(s)")
        print(f"Use --all to show all, or --max-diffs N to show more")
    
    print("\n" + "=" * 70)
    if diffs:
        print(f"\033[91m✗ Files differ: {len(diffs):,} bytes in {len(regions)} region(s)\033[0m")
        sys.exit(1)
    else:
        print(f"\033[93m⚠ Files have different sizes but shared content matches\033[0m")
        sys.exit(1)


if __name__ == '__main__':
    main()
