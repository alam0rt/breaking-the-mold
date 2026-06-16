#!/usr/bin/env python3
"""Search longer stems with end-shift matching wizard family token convention.

K=18: tokens '1','2','3' produce siblings (most common audio convention)
K=15: tokens '4','5','6'
"""

import itertools

def calcHash_with_endsh(s):
    h, sh = 0, 0
    for ch in s:
        c = ord(ch)
        if 65 <= c <= 90: pass
        elif 97 <= c <= 122: c -= 32
        elif 48 <= c <= 57: c += 22
        else: continue
        sh = (sh + c - 64) & 31
        h ^= 1 << sh
    return h & 0xFFFFFFFF, sh


chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789'

# Search lengths 5..7, K in {18, 15, 17, 24}
# For each match, score by "wordlike" heuristic
import re
def wordlike(s):
    vowels = sum(1 for c in s.upper() if c in 'AEIOUY')
    if vowels == 0:
        return False
    # No 4 consonants in a row
    if re.search(r'[BCDFGHJKLMNPQRSTVWXZ]{4,}', s.upper()):
        return False
    # Not all digits
    if s.isdigit():
        return False
    return True


ks_of_interest = {15, 17, 18, 24}
samples = {k: [] for k in ks_of_interest}

for L in [5, 6]:
    print(f'\nSearching L={L} ({36**L:,} strings)...')
    n_seen = 0
    for combo in itertools.product(chars, repeat=L):
        s = ''.join(combo)
        h, sh = calcHash_with_endsh(s)
        if h == 0x0c041001 and sh in ks_of_interest:
            if wordlike(s):
                samples[sh].append(s)
        n_seen += 1
    print(f'  {n_seen:,} strings tested')

print()
for k in sorted(samples):
    print(f'\nK={k} ({len(samples[k])} wordlike stems):')
    for s in samples[k][:60]:
        print(f'    {s}')
    if len(samples[k]) > 60:
        print(f'    ... {len(samples[k])-60} more')
