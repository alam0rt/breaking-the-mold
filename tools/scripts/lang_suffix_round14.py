#!/usr/bin/env python3
"""Round 14: search for language suffix pairs more carefully.

The id-space FR delta is 0x01079563 (popcount 12).
In calcHash-space (before the 0x28c0e011 ^ rotl(_, 27) wrapper), this
corresponds to FR_calcHash_delta = rotr(0x01079563, 27) = rotl(0x01079563, 5)
= 0x20F2AC60 (still popcount 12).

Since asset names share the suffix-substitution structure, we need:
   calcHash(S_FR) XOR calcHash(S_EN) ∈ rotations(FR_DELTA_ID)
   i.e., calcHash(S_FR) XOR calcHash(S_EN) ∈ rotations(0x01079563)
(both spaces give the same orbit since rotation distributes over XOR)

Approach: enumerate calcHash for short strings (alpha, alphanumeric,
underscores OK because they're skipped), bucket by hash, and find
pairs in target orbit.
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

# Verify formula
SEED = 0x28c0e011
def asset_id(s):
    return SEED ^ rotl(calc_hash(s), 27)

assert asset_id("NO") == 0x29c0e211, hex(asset_id("NO"))
assert asset_id("YES") == 0x2ad0f011
assert asset_id("PAUSED") == 0x0ad0f813
assert asset_id("QUITGAME") == 0x69c8f473
print("Formula verified")

FR_DELTA_ID = 0x01079563     # in id-space
DE_DELTA_ID_ALIGNED = 0x0587801a  # in id-space, same alignment as FR

# All rotations of FR delta (in id-space)
FR_ORBIT = set()
for r in range(32):
    FR_ORBIT.add(rotl(FR_DELTA_ID, r))
DE_ORBIT = set()
for r in range(32):
    DE_ORBIT.add(rotl(DE_DELTA_ID_ALIGNED, r))

print(f"FR orbit size: {len(FR_ORBIT)}, popcount {bin(FR_DELTA_ID).count('1')}")
print(f"DE orbit size: {len(DE_ORBIT)}, popcount {bin(DE_DELTA_ID_ALIGNED).count('1')}")

# Build hash table of plausible language tag strings
import itertools
ALPHA = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
DIGIT = "0123456789"

# Length 1-4 only, alpha first then alphanumeric
print("\nGenerating short alpha and alphanumeric strings...")
strings = []
for L in range(1, 5):
    for combo in itertools.product(ALPHA, repeat=L):
        strings.append("".join(combo))
print(f"  {len(strings):,} alpha strings (lengths 1-4)")
# Add digit-only and mixed
mixed = []
for L in range(1, 4):
    for combo in itertools.product(ALPHA + DIGIT, repeat=L):
        s = "".join(combo)
        if s not in set(strings):
            mixed.append(s)
print(f"  {len(mixed):,} alphanumeric (lengths 1-3)")
strings.extend(mixed)

# Hash bucket
print("Building hash → strings map...")
h_to_strs = {}
for s in strings:
    h_to_strs.setdefault(calc_hash(s), []).append(s)
print(f"  {len(h_to_strs):,} unique hashes from {len(strings):,} strings")

# For each hash a, for each rotation in orbit, check if a^orbit_val exists in map
# Pick representative pairs that look like language-tag-able
def is_pronouncable(s):
    """Heuristic: contains at least one vowel, no triple consonants."""
    vowels = sum(1 for c in s if c in 'AEIOUY')
    return vowels >= 1

print("\n=== Searching for FR pairs ===")
fr_pairs = []
for h_a, strs_a in h_to_strs.items():
    for delta in FR_ORBIT:
        h_b = h_a ^ delta
        if h_b in h_to_strs:
            for a in strs_a:
                for b in h_to_strs[h_b]:
                    if a < b:  # Avoid duplicate
                        fr_pairs.append((a, b, delta))

# Filter pronouncable
print(f"  {len(fr_pairs):,} total FR pairs found")
prn_fr = [(a,b,d) for a,b,d in fr_pairs if is_pronouncable(a) and is_pronouncable(b)]
print(f"  {len(prn_fr):,} pronouncable pairs")

# Show the most plausible language-tag-looking pairs
SUFFIX_PRIORS = {"US", "FR", "DE", "EN", "GB", "UK", "JP", "JA", "GE",
                 "USA", "FRA", "DEU", "GER", "ENG", "FRE", "FRC",
                 "AME", "AMR", "ENGLISH", "FRENCH", "GERMAN",
                 "UE", "UK", "EU", "JA", "JP",
                 "L1", "L2", "L3", "L4", "L0",
                 "T1", "T2", "T3", "T0",
                 "0", "1", "2", "3", "00", "01", "02", "03",
                 "A", "B", "C", "D", "E", "F", "G", "H",
                 # Translation words
                 "NO", "OUI", "YES", "JA", "NEIN", "NON",
                 "OK", "JAA", "OUI",
                 "QUIT", "EXIT", "RETURN", "BACK",
                 "PAUSE", "PLAY", "STOP", "GO",
                 "WIN", "LOSE", "DEAD", "DIE",
                 "GAME", "JEU", "SPIEL",
                 "OVER", "FIN", "ENDE",
                 "CONTINUE", "RESUME",
                 "OPTIONS", "OPTION",
                 "MENU", "TITLE", "TITRE", "TITEL",
                 "SCORE", "BEST", "TIME",
                 "LIVES", "VIE", "VIES", "LEBEN", "LEBN",
                 "LEVEL", "NIVEAU", "EBENE", "STUFE",
                 "PRESS", "START",
                 "LOAD", "SAVE", "PLAY",
                 "PAUSED", "GAMEOVER",
                 }

# Find pairs where BOTH are in suffix priors
prior_pairs = [(a,b,d) for a,b,d in fr_pairs if a in SUFFIX_PRIORS and b in SUFFIX_PRIORS]
print(f"\nPairs where BOTH strings are in priors: {len(prior_pairs)}")
for a, b, d in prior_pairs:
    print(f"  {a:10s} {b:10s} delta={d:#010x}")

# Find pairs where AT LEAST ONE is in priors
one_prior = [(a,b,d) for a,b,d in fr_pairs if a in SUFFIX_PRIORS or b in SUFFIX_PRIORS]
print(f"\nPairs where AT LEAST ONE is a known prior: {len(one_prior)}")
for a, b, d in one_prior[:50]:
    a_in = "*" if a in SUFFIX_PRIORS else " "
    b_in = "*" if b in SUFFIX_PRIORS else " "
    print(f"  {a_in}{a:10s} {b_in}{b:10s} delta={d:#010x}")

print("\n=== Searching for DE pairs ===")
de_pairs = []
for h_a, strs_a in h_to_strs.items():
    for delta in DE_ORBIT:
        h_b = h_a ^ delta
        if h_b in h_to_strs:
            for a in strs_a:
                for b in h_to_strs[h_b]:
                    if a < b:
                        de_pairs.append((a, b, delta))

print(f"  {len(de_pairs):,} total DE pairs found")
prior_pairs_de = [(a,b,d) for a,b,d in de_pairs if a in SUFFIX_PRIORS and b in SUFFIX_PRIORS]
print(f"\nPairs where BOTH strings are in priors: {len(prior_pairs_de)}")
for a, b, d in prior_pairs_de:
    print(f"  {a:10s} {b:10s} delta={d:#010x}")
one_prior_de = [(a,b,d) for a,b,d in de_pairs if a in SUFFIX_PRIORS or b in SUFFIX_PRIORS]
print(f"\nPairs where AT LEAST ONE is a known prior: {len(one_prior_de)}")
for a, b, d in one_prior_de[:50]:
    a_in = "*" if a in SUFFIX_PRIORS else " "
    b_in = "*" if b in SUFFIX_PRIORS else " "
    print(f"  {a_in}{a:10s} {b_in}{b:10s} delta={d:#010x}")
