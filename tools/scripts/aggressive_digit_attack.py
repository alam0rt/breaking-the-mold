"""Aggressive [WORD][NUM] and [WORD][NUM][WORD] attack with broad prefix vocab."""
import os, sys, itertools

def calcHash(s):
    h, sh = 0, 0
    for c in s.upper():
        c = ord(c)
        if 65 <= c <= 90: pass
        elif 48 <= c <= 57: c += 22
        else: continue
        sh = (sh + c - 64) & 31
        h ^= 1 << sh
    return h

def rotl(v, r):
    r &= 31
    return ((v << r) | (v >> ((32 - r) & 31))) & 0xFFFFFFFF

def asset_id(name):
    return (0x28C0E011 ^ rotl(calcHash(name), 27)) & 0xFFFFFFFF

# Load all unknown ids
all_ids = set()
with open('docs/reference/asset-ids-master.csv') as f:
    next(f)
    for line in f:
        parts = line.strip().split(',')
        if len(parts) >= 1 and parts[0].startswith('0x'):
            all_ids.add(int(parts[0], 16))
print(f"Total ids: {len(all_ids)}")

anchors = {0x29c0e211, 0x2ad0f011, 0x0ad0f813, 0x68c0f413, 0x69c04050, 0x69c8f473}
targets = all_ids - anchors

# ULTRA-broad prefix list: all words from cracking_wordlist that are 3-7 letters
prefixes = set()
with open('tools/data/cracking_wordlist.txt') as f:
    for line in f:
        w = line.strip().upper()
        if 3 <= len(w) <= 7 and w.isalpha():
            prefixes.add(w)
prefixes = sorted(prefixes)
print(f"Prefixes from cracking_wordlist (3-7 char alpha): {len(prefixes)}")

# Also include tokens from ALL existing function-context hits & compound candidates
extra = set()
for path in [
    'docs/analysis/asset-identification/function_compound_hits.csv',
    'docs/analysis/asset-identification/compound_d3.csv',
    'docs/analysis/asset-identification/three_token_hits.csv',
    'docs/analysis/asset-identification/two_token_hits.csv',
]:
    if not os.path.exists(path): continue
    with open(path) as f:
        for line in f:
            for tok in line.upper().split(','):
                tok = ''.join(c for c in tok if c.isalpha())
                if 3 <= len(tok) <= 9:
                    extra.add(tok)
prefixes = list(prefixes) + sorted(extra - set(prefixes))
print(f"After adding context tokens: {len(prefixes)}")

# 1-3 digit suffixes 0-999
nums = []
for n in range(0, 1000):
    nums.append(str(n))
    if n < 100:
        nums.append(f'{n:02d}')
    if n < 10:
        nums.append(f'{n:03d}')

# Pattern 1: [PREFIX][NUM]
print("Pattern 1: [PREFIX][NUM]...")
hits = []
for p in prefixes:
    for n in nums:
        for s in (p + n,):
            aid = asset_id(s)
            if aid in targets:
                hits.append((aid, s, len(s)))

print(f"  -> {len(hits)} hits, {len(set(h[0] for h in hits))} unique ids")
hits.sort(key=lambda x: (x[2], x[1]))
unique_seen = set()
shown = 0
for aid, s, L in hits:
    if aid in unique_seen: continue
    unique_seen.add(aid)
    print(f"  0x{aid:08x}  {s}")
    shown += 1
    if shown > 50: break
