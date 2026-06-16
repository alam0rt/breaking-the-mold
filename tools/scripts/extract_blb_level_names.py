#!/usr/bin/env python3
"""Extract level display names from the BLB header."""
import sys, struct
from pathlib import Path

ROOT = Path("/home/sam/projects/btm")

# BLB header structure (per blbacc.c):
#   level_count at offset 0xF31 (u8)
#   levels start at offset 0
#   each level entry: 0x70 = 112 bytes
#   per-entry:
#     +0x0C: GetLevelAssetIndex (u8)
#     +0x0D: GetLevelFlagByIndex (u8)
#     +0x56: getLevelName (char*) - display name string
#     +0x5B: 4-letter level_id (char[4])

# But wait — the levels are inside the first 0x1000 bytes. 
# 0xF31 is for level_count. So the level array fits in [0, 0xF31).
# Each entry 0x70 → max ~52 entries, but only 26 levels.
# Let's just dump 26 entries × 0x70 starting from 0.

import sys
blb_path = ROOT / "disks/blb/sles-01090.blb"
print(f"Reading {blb_path}: {blb_path.stat().st_size} bytes")

with open(blb_path, "rb") as f:
    data = f.read()

# Try to find the BLB header structure. BLB files might have a header before the level table.
# Let's first look at offsets to find printable strings.
# But level_count at 0xF31. So levels in [0, 0x1024).
print(f"\nByte at 0xF31: {data[0xF31]} (level_count)")
print(f"Byte at 0xF36 (first entry kind): {data[0xF36]}")

# 26 entries × 0x70 = 0xB60 = 2912 bytes
# So levels likely start at 0x0
# Check first entry strings
def show_entry(offset, idx):
    base = offset
    asset_idx = data[base + 0x0C]
    flag = data[base + 0x0D]
    name_at_56 = data[base + 0x56:base + 0x70]
    # Find first null
    end = name_at_56.find(b'\x00')
    if end == -1:
        end = len(name_at_56)
    name_str = name_at_56[:end].decode('latin-1', errors='replace').rstrip()
    
    # Also check the +0x5B field
    sub = data[base + 0x5B:base + 0x60]
    sub_str = sub.decode('latin-1', errors='replace').rstrip('\x00')
    
    print(f"  Entry {idx:2d} @ 0x{base:04x}: asset_idx={asset_idx:3d} flag=0x{flag:02x} name='{name_str}' code='{sub_str}'")

print("\n=== Level entries (assuming start at 0) ===")
for i in range(30):
    base = i * 0x70
    if base + 0x70 > len(data):
        break
    show_entry(base, i)