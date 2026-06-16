#!/usr/bin/env python3
"""Analyze the distribution of asset IDs by value range."""
import csv

ranges = {}
with open("/home/sam/projects/btm/docs/reference/asset-ids-master.csv") as f:
    rdr = csv.DictReader(f)
    for r in rdr:
        v = int(r["id_hex"], 16)
        bucket = (v >> 28) & 0xF
        ranges.setdefault(bucket, []).append((v, r))

print("ID distribution by top nibble:")
for k in sorted(ranges):
    print(f"  0x{k:01x}xxxxxxx: {len(ranges[k])} ids")

# IDs in PSX RAM range (0x8000_0000 to 0x8020_0000)
ram_ids = []
with open("/home/sam/projects/btm/docs/reference/asset-ids-master.csv") as f:
    rdr = csv.DictReader(f)
    for r in rdr:
        v = int(r["id_hex"], 16)
        if 0x80000000 <= v < 0x80200000:
            ram_ids.append((v, r))

print(f"\nIDs in PSX RAM range (0x80000000-0x80200000): {len(ram_ids)}")
for v, r in sorted(ram_ids):
    print(f"  0x{v:08x}  popc={r['popcount']}  sources={r['sources']}")
