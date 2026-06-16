"""Find the transformation Skullmonkeys uses for Klaymen-derived assets.

We have ~1929 Neverhood hash literals from ScummVM source.
Skullmonkeys has 658 IDs.
Direct hits: 3 (klaymen.cpp).

Maybe other Skullmonkeys IDs are wrapped via some transformation.
Let's brute-force: for each rotation r in 0..31 and each XOR constant pulled
from the Skullmonkeys binary, count how many (Nev hash) -> (SM id) maps land
on a known SM id.
"""

import re
import csv
import glob
import os
from collections import Counter

def rotl(v, r):
    r &= 31
    return ((v << r) | (v >> ((32 - r) & 31))) & 0xFFFFFFFF

# Load
sm_ids = set()
with open('/home/sam/projects/btm/docs/reference/asset-ids-master.csv') as f:
    next(f)
    for line in f:
        parts = line.split(',')
        if parts[0].startswith('0x'):
            sm_ids.add(int(parts[0], 16))

src_dir = '/tmp/scummvm_neverhood'
hash_re = re.compile(r'0x([0-9A-Fa-f]{8})\b')
nev_hashes = set()
for src in glob.glob(f'{src_dir}/*.cpp'):
    text = open(src).read()
    for hm in hash_re.finditer(text):
        h = int(hm.group(1), 16)
        if h >= 0x00100000 and h != 0xFFFFFFFF:
            nev_hashes.add(h)

print(f"Neverhood hashes: {len(nev_hashes)}, Skullmonkeys IDs: {len(sm_ids)}")

# Try every (rotation, xor) from a sweep of common XOR constants:
#   - 0
#   - the canonical 0x28C0E011
#   - the Skullmonkeys XORs we've seen in the binary
candidate_xors = [0, 0x28C0E011, 0x28C0E000, 0xFFFFFFFF]
# Read additional XOR-style constants from .data section of SLES binary (heuristic).
# Look for 32-bit values that appear repeatedly in the .data section.
try:
    with open('/home/sam/projects/btm/bin/SLES_010.90', 'rb') as f:
        bin_data = f.read()
    # Grab values likely to be hash-system constants: high bits set + scattered
    # bits, from a typical "magic constants" pool.
    import struct
    counts = Counter()
    for i in range(0, len(bin_data) - 4, 4):
        v = struct.unpack('<I', bin_data[i:i+4])[0]
        # Magic-constant heuristic: high bit set or middle-range, popcount diverse
        if 0x10000000 <= v <= 0xF0000000:
            pc = bin(v).count('1')
            if 8 <= pc <= 24:
                counts[v] += 1
    # Add top frequent ones
    for v, c in counts.most_common(30):
        if c >= 3:
            candidate_xors.append(v)
except Exception as e:
    print("scan failed:", e)

candidate_xors = sorted(set(candidate_xors))

print(f"Candidate XORs: {len(candidate_xors)}")

best = []
for r in range(32):
    for x in candidate_xors:
        hits = sum(1 for n in nev_hashes if (rotl(n, r) ^ x) in sm_ids)
        if hits >= 4:
            best.append((hits, r, x))

best.sort(reverse=True)
print(f"\nTop transformations (>= 4 hits) of {len(best)} found:")
for hits, r, x in best[:30]:
    print(f"  hits={hits:4d}  rotl(N, {r:2d}) ^ 0x{x:08x}")
