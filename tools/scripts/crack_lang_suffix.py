#!/usr/bin/env python3
"""Crack the EN→FR and EN→DE suffix pairs from regional delta masks.

From docs/reference/asset-hash-ids.md Round 6:
  FR base delta in calcHash space: 0x01079563 (popcount 12)
  DE base delta in calcHash space: 0x00340b0f (popcount 10)

Each EN→FR / EN→DE pair gives:
  calcHash(EN_suffix) ^ calcHash(FR_suffix) ∈ rotations(0x01079563)

So we search ALL pairs (EN_str, FR_str) of short alphanumeric strings such
that calcHash(EN) ^ calcHash(FR) ∈ rotations(0x01079563) and keep only those
that look like real language tags.
"""
import itertools
import sys

def calc_hash(s):
    h = 0
    sh = 0
    for c in s.upper():
        if c.isalnum():
            if c.isdigit():
                co = ord(c) + 22
            else:
                co = ord(c)
            sh += co - 64
            sh &= 31
            h ^= 1 << sh
    return h

def rotations(mask):
    return {((mask << r) | (mask >> (32 - r))) & 0xFFFFFFFF for r in range(32)}

FR_DELTA_BASE = 0x01079563
DE_DELTA_BASE = 0x00340b0f

FR_ROTS = rotations(FR_DELTA_BASE)
DE_ROTS = rotations(DE_DELTA_BASE)

# Verify NO/YES anchors with our hash function
def asset_id(s):
    h = calc_hash(s)
    return 0x28C0E011 ^ ((h << 27) | (h >> 5)) & 0xFFFFFFFF

assert asset_id("NO")  == 0x29c0e211, f"NO failed: {asset_id('NO'):#x}"
assert asset_id("YES") == 0x2ad0f011, f"YES failed: {asset_id('YES'):#x}"
assert asset_id("PAUSED") == 0x0ad0f813, f"PAUSED failed: {asset_id('PAUSED'):#x}"
assert asset_id("QUITGAME") == 0x69c8f473, f"QUITGAME failed: {asset_id('QUITGAME'):#x}"
print("Hash function verified.")

# Generate all short alphanumeric strings up to length L
ALPHA = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
DIGIT = "0123456789"
ALNUM = ALPHA + DIGIT

# Build a hash table: hash -> list of strings
def all_strings_up_to(L, allow_digits=False, max_count=200000):
    """Yield (str, calcHash) for all strings up to length L."""
    chars = ALNUM if allow_digits else ALPHA
    for n in range(1, L + 1):
        for combo in itertools.product(chars, repeat=n):
            s = ''.join(combo)
            yield s, calc_hash(s)

# Build hash -> str list for length up to 5 (alphabetical only first)
print("Building string hash table for lengths 1-5 (alpha only)...")
hash_to_strs = {}
count = 0
for s, h in all_strings_up_to(5, allow_digits=False):
    hash_to_strs.setdefault(h, []).append(s)
    count += 1
print(f"  {count:,} strings, {len(hash_to_strs):,} unique hashes")

# Also include digit-allowed strings up to length 4
print("Adding strings with digits up to length 4...")
for s, h in all_strings_up_to(4, allow_digits=True):
    if any(c.isdigit() for c in s):
        hash_to_strs.setdefault(h, []).append(s)
        count += 1
print(f"  total: {count:,} strings, {len(hash_to_strs):,} unique hashes")

# Now search for pairs (a, b) such that calcHash(a) ^ calcHash(b) is in rotations(MASK)
def find_pairs(target_rots, max_print=100):
    found = []
    hashes = list(hash_to_strs.keys())
    seen_pairs = set()
    for h_a in hashes:
        for r in range(32):
            tgt = ((FR_DELTA_BASE << r) | (FR_DELTA_BASE >> (32 - r))) & 0xFFFFFFFF
            if tgt not in target_rots:
                continue
            h_b = h_a ^ tgt
            if h_b in hash_to_strs:
                for a in hash_to_strs[h_a][:3]:
                    for b in hash_to_strs[h_b][:3]:
                        key = tuple(sorted([a, b]))
                        if key in seen_pairs:
                            continue
                        seen_pairs.add(key)
                        found.append((a, b, r))
                        if len(found) >= max_print:
                            return found
    return found

print(f"\n=== Searching for EN→FR pairs (delta = rotation of {FR_DELTA_BASE:#010x}) ===")
fr_pairs = []
for h_a, strs_a in hash_to_strs.items():
    for tgt in FR_ROTS:
        h_b = h_a ^ tgt
        if h_b in hash_to_strs:
            for a in strs_a[:1]:
                for b in hash_to_strs[h_b][:1]:
                    if a == b: continue
                    # filter to plausible language tags
                    fr_pairs.append((a, b, tgt))
print(f"Total raw pairs: {len(fr_pairs):,}")

# Filter for plausible language tags
LANG_HINTS_EN = {"E", "EN", "ENG", "ENGLISH", "US", "UK", "GB", "USA", "GBR",
                "AM", "AME", "AMERICAN", "BRT", "BRIT", "BRITISH"}
LANG_HINTS_FR = {"F", "FR", "FRA", "FRC", "FRE", "FRENCH", "FRANCE", "FRANCAIS"}
LANG_HINTS_DE = {"D", "DE", "GE", "GER", "GR", "GERMAN", "GERMANY", "DEUTSCH", "DEU"}

# Pairs where one is an EN hint and other is FR hint
print("\n=== Plausible EN/FR language tag pairs ===")
for a, b, tgt in fr_pairs:
    a_up, b_up = a.upper(), b.upper()
    if (a_up in LANG_HINTS_EN and b_up in LANG_HINTS_FR) or \
       (b_up in LANG_HINTS_EN and a_up in LANG_HINTS_FR):
        print(f"  {a:8s}  {b:8s}  delta={tgt:#010x}")

# Also try DE
print(f"\n=== Searching for EN→DE pairs (delta = rotation of {DE_DELTA_BASE:#010x}) ===")
de_pairs = []
for h_a, strs_a in hash_to_strs.items():
    for tgt in DE_ROTS:
        h_b = h_a ^ tgt
        if h_b in hash_to_strs:
            for a in strs_a[:1]:
                for b in hash_to_strs[h_b][:1]:
                    if a == b: continue
                    de_pairs.append((a, b, tgt))
print(f"Total raw pairs: {len(de_pairs):,}")
print("\n=== Plausible EN/DE language tag pairs ===")
for a, b, tgt in de_pairs:
    a_up, b_up = a.upper(), b.upper()
    if (a_up in LANG_HINTS_EN and b_up in LANG_HINTS_DE) or \
       (b_up in LANG_HINTS_EN and a_up in LANG_HINTS_DE):
        print(f"  {a:8s}  {b:8s}  delta={tgt:#010x}")
