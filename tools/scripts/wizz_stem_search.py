#!/usr/bin/env python3
"""WIZZ audio family stem analysis.

Stem hash 0x0c041001 (popc=5) with siblings 0x04041001 / 0x08041001 / 0x0e041001
(siblings differ by single-bit residual at positions 27, 26, 25 -- a 3-token
sequence).

For end-shift K=15, next-char vals are 10,11,12 -> chars J/K/L or digits 4/5/6.

Try wind/wizz/audio themed stems.
"""

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

# Stem hash analysis: bits set
v = 0x0c041001
bits = [i for i in range(32) if v & (1 << i)]
print(f'stem 0x0c041001 bits set: {bits}')

# Search 4-5 char stems hashing to 0x0c041001 with esh=15
import itertools
chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789'

print('\nSearching 4-5 char stems with hash=0x0c041001 esh=15...')
for L in [4, 5, 6]:
    cnt = 0
    samples = []
    for combo in itertools.product(chars, repeat=L):
        s = ''.join(combo)
        h, esh = calcHash_with_endsh(s)
        if h == 0x0c041001 and esh == 15:
            cnt += 1
            if cnt <= 30:
                samples.append(s)
    print(f'  L={L}: {cnt} stems')
    for s in samples[:30]:
        print(f'    {s}')
