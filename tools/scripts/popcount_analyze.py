#!/usr/bin/env python3
"""Analyze popcount distribution for asset IDs."""
import csv, sys
from collections import Counter
sys.path.insert(0, "tools/scripts")
from compound_hash_attack import asset_id, calc_hash_and_shift, rotl

ids = []
with open("/home/sam/projects/btm/docs/reference/asset-ids-master.csv") as f:
    rdr = csv.DictReader(f)
    for r in rdr:
        ids.append((int(r["id_hex"], 16), int(r["popcount"]), r))

def derotated_h(idval):
    return rotl(idval ^ 0x28C0E011, 5)

popc_dist = Counter(p for _, p, _ in ids)
print("Popcount distribution:")
for p in sorted(popc_dist):
    print(f"  popc={p:2d}: {popc_dist[p]:4d} ids")
print()

# Lowest popcount IDs (most reachable by short brute force)
print("Lowest popcount IDs (best brute targets):")
sorted_ids = sorted(ids, key=lambda x: x[1])
for v, p, r in sorted_ids[:30]:
    print(f"  popc={p:2d}  0x{v:08x}  sources={r['sources']}  kinds={r['kinds']}")
