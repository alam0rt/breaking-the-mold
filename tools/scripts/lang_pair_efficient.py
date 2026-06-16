#!/usr/bin/env python3
"""Efficient pair search: build hash buckets, then for each unique calcHash h_a,
test if h_a XOR rotl(FR_DELTA, r) is in the bucket for any r ∈ 0..31.

This finds ALL pairs in O(N × 32) lookups.
"""

def calc_hash(s):
    h = 0; sh = 0
    for c in s.upper():
        if 'A' <= c <= 'Z':
            sh = (sh + ord(c) - 64) & 31
            h ^= 1 << sh
        elif '0' <= c <= '9':
            sh = (sh + ord(c) + 22 - 64) & 31
            h ^= 1 << sh
    return h

def rotl(x, n):
    return ((x << n) | (x >> (32 - n))) & 0xFFFFFFFF

FR_DELTA = 0x01079563
DE_DELTA = 0x0587801a
FR_ORBIT = [rotl(FR_DELTA, r) for r in range(32)]
DE_ORBIT = [rotl(DE_DELTA, r) for r in range(32)]

import itertools
ALPHA = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"

print("Building hash → shortest string map (alpha L=1..5)...")
h_to_short = {}  # only keep ONE shortest representative per hash
for L in range(1, 6):
    for combo in itertools.product(ALPHA, repeat=L):
        s = "".join(combo)
        h = calc_hash(s)
        if h not in h_to_short or len(s) < len(h_to_short[h]):
            h_to_short[h] = s
        elif len(s) == len(h_to_short[h]) and s < h_to_short[h]:
            h_to_short[h] = s
print(f"  {len(h_to_short):,} unique hashes")

# Now find ALL (h_a, r, h_b) where h_a XOR h_b ∈ FR_ORBIT (with rotation r)
# We pivot on h_a, compute target = h_a XOR rotl(FR_DELTA, r), check if target in h_to_short

print("\n=== FR pair search across ALL unique hashes ===")
fr_pairs = []
for h_a, s_a in h_to_short.items():
    for r in range(32):
        target = h_a ^ FR_ORBIT[r]
        if target in h_to_short and target != h_a:
            s_b = h_to_short[target]
            # Avoid duplicates (a, b) and (b, a)
            if s_a < s_b:
                fr_pairs.append((s_a, s_b, r))

print(f"  {len(fr_pairs):,} unique pairs")

# Filter: only keep pairs where BOTH are ≤4 chars
short_fr = [(a, b, r) for a, b, r in fr_pairs if len(a) <= 4 and len(b) <= 4]
print(f"  {len(short_fr)} pairs where both ≤4 chars")
short_fr.sort(key=lambda x: (len(x[0])+len(x[1]), x[0], x[1]))
for a, b, r in short_fr[:80]:
    print(f"    {a:6s} {b:6s}  rot={r:2d}")

# Pairs ≤5 with high vowel content
print(f"\n=== Pairs (≤5) with at least 1 vowel each ===")
def vc(s): return sum(1 for c in s if c in 'AEIOU')
plausible = [(a,b,r) for a,b,r in fr_pairs if len(a)<=5 and len(b)<=5 and vc(a)>=1 and vc(b)>=1]
plausible.sort(key=lambda x: (len(x[0])+len(x[1]), x[0]))
print(f"  {len(plausible)} pairs")
for a, b, r in plausible[:60]:
    print(f"    {a:6s} {b:6s}  rot={r:2d}")

# Same for DE
print("\n=== DE pair search ===")
de_pairs = []
for h_a, s_a in h_to_short.items():
    for r in range(32):
        target = h_a ^ DE_ORBIT[r]
        if target in h_to_short and target != h_a:
            s_b = h_to_short[target]
            if s_a < s_b:
                de_pairs.append((s_a, s_b, r))
print(f"  {len(de_pairs):,} unique pairs")
short_de = [(a, b, r) for a, b, r in de_pairs if len(a) <= 4 and len(b) <= 4]
print(f"  {len(short_de)} pairs where both ≤4 chars")
for a, b, r in sorted(short_de, key=lambda x: (len(x[0])+len(x[1]), x[0]))[:80]:
    print(f"    {a:6s} {b:6s}  rot={r:2d}")
plausible_de = [(a,b,r) for a,b,r in de_pairs if len(a)<=5 and len(b)<=5 and vc(a)>=1 and vc(b)>=1]
print(f"\n=== DE pairs (≤5) with vowels ===")
print(f"  {len(plausible_de)} pairs")
for a, b, r in sorted(plausible_de, key=lambda x: (len(x[0])+len(x[1]), x[0]))[:60]:
    print(f"    {a:6s} {b:6s}  rot={r:2d}")
