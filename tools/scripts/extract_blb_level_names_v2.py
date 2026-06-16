#!/usr/bin/env python3
"""Extract FULL level display names from BLB header."""
import sys, struct
from pathlib import Path

ROOT = Path("/home/sam/projects/btm")
blb_path = ROOT / "disks/blb/sles-01090.blb"

with open(blb_path, "rb") as f:
    data = f.read()

print(f"BLB size: {len(data):,}")

# Per blbacc.c:
#   +0x56: 4-byte level_id (e.g. "MENU")
#   +0x5B: char* display name (the field IS the string, not a pointer)
# Each entry is 0x70 (112) bytes
# Level count at 0xF31

level_count = data[0xF31]
print(f"level_count at 0xF31: {level_count}")

print("\n=== Level entries ===")
for i in range(level_count + 5):  # extra to see beyond
    base = i * 0x70
    if base + 0x70 > len(data):
        break
    asset_idx = data[base + 0x0C]
    flag = data[base + 0x0D]
    
    # Read NULL-terminated strings at +0x56 and +0x5B
    name_bytes = data[base + 0x56:base + 0x70]
    end1 = name_bytes.find(b'\x00')
    if end1 == -1: end1 = len(name_bytes)
    name1 = name_bytes[:end1].decode('latin-1', errors='replace')
    
    # Display name at +0x5B (extends until null OR end of entry)
    disp_bytes = data[base + 0x5B:base + 0x70]
    end2 = disp_bytes.find(b'\x00')
    if end2 == -1: end2 = len(disp_bytes)
    disp = disp_bytes[:end2].decode('latin-1', errors='replace')
    
    print(f"  {i:3d} idx={asset_idx:3d} flag=0x{flag:02x}  level_id='{name1[:5]:5s}'  display='{disp}'")

# Better: dump the raw region just to see the structure
print(f"\n=== Raw bytes at offsets 0x50-0x80 of first entry ===")
for off in range(0x40, 0x80, 16):
    chunk = data[off:off+16]
    hex_str = ' '.join(f'{b:02x}' for b in chunk)
    ascii_str = ''.join(chr(b) if 32 <= b < 127 else '.' for b in chunk)
    print(f"  {off:04x}: {hex_str}  |{ascii_str}|")
