#!/usr/bin/env python3
"""Hypothesis: S_EN is empty, so calcHash(S_FR) IN FR_ORBIT directly."""

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

FR_ORBIT = {rotl(FR_DELTA, r): r for r in range(32)}
DE_ORBIT = {rotl(DE_DELTA, r): r for r in range(32)}

import itertools
ALPHA = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
DIGIT = "0123456789"

print("Generating strings & hashing...")
fr_hits = []
de_hits = []

# Length 1-5 alpha (length 5 is 11M strings, takes ~30s in python)
for L in range(1, 6):
    n = 0
    for combo in itertools.product(ALPHA, repeat=L):
        s = "".join(combo)
        h = calc_hash(s)
        if h in FR_ORBIT:
            fr_hits.append((s, FR_ORBIT[h]))
        if h in DE_ORBIT:
            de_hits.append((s, DE_ORBIT[h]))
        n += 1
    print(f"  L={L} (alpha): {n:,} strings, FR={len(fr_hits)}, DE={len(de_hits)}")

print(f"\n=== FR_DELTA orbit hits ({len(fr_hits)}) ===")
# Sort by length and pronounceability
def is_word_like(s):
    vowels = sum(1 for c in s if c in 'AEIOU')
    consonants = len(s) - vowels
    return vowels >= 1 and (vowels >= len(s) // 3)

short = sorted(fr_hits, key=lambda x: (len(x[0]), x[0]))
for s, r in short[:50]:
    flag = "*" if is_word_like(s) else " "
    print(f"  {flag}{s!r:10s} (len={len(s)}, rot={r})")

print(f"\n=== DE_DELTA orbit hits ({len(de_hits)}) ===")
short = sorted(de_hits, key=lambda x: (len(x[0]), x[0]))
for s, r in short[:50]:
    flag = "*" if is_word_like(s) else " "
    print(f"  {flag}{s!r:10s} (len={len(s)}, rot={r})")
