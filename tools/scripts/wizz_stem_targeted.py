#!/usr/bin/env python3
"""Find a real readable stem whose calcHash == 0x0c041001 and end_shift makes
sequential token chars hit the 3 family ids.

For each candidate base name + suffix combo, check:
  1. hash == 0x0c041001
  2. end_shift K such that K+val('1'), K+val('2'), K+val('3') match
     residual bit positions {25, 26, 27} OR
     other sequential mappings.

Try many short audio-name shapes.
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


# Sequential token shapes we want supported:
# A=1, B=2, C=3 -> vals 1,2,3 -> bits K+1, K+2, K+3
# 1,2,3 -> vals 7,8,9 -> bits K+7, K+8, K+9
# Targets are bits {25, 26, 27}. So K+val1=25, K+val2=26, K+val3=27.
# val series must be (v, v+1, v+2). 
# For digits 1,2,3 (vals 7,8,9): K=18. Then ids would be from STEM+'1' bit 25, STEM+'2' bit 26, STEM+'3' bit 27.
# For digits 4,5,6 (vals 10,11,12): K=15. Then STEM+'4' bit 25, STEM+'5' bit 26, STEM+'6' bit 27.
# For letters A,B,C (vals 1,2,3): K=24. Then STEM+'A' bit 25, STEM+'B' bit 26, STEM+'C' bit 27.

# Also: ids order: 0x04041001 has bit 27 NOT set (popc 4), 0x08041001 bit 26 NOT set, 0x0e041001 has bits 25,26,27 set.
# Actually 0x04041001 = bits 0,12,18,26
# 0x08041001 = bits 0,12,18,27
# 0x0e041001 = bits 0,12,18,25,26,27
# So:
#   stem ^ tokenA -> 0x04041001
#   stem ^ tokenB -> 0x08041001
#   stem ^ tokenC -> 0x0e041001
# stem is bit-majority: bits 0,12,18 (all present in all 3) and bits 26,27 (in 2/3).
#   So stem = 0x0c041001 (bits 0,12,18,26,27 -- popc 5).
# Token hashes (after rotl by K=end_shift):
#   tokenA hash = stem ^ id_A = 0x08000000 = bit 27 -> rotr by K must give 1 << val
#   tokenB hash = stem ^ id_B = 0x04000000 = bit 26
#   tokenC hash = stem ^ id_C = 0x02000000 = bit 25

# For single-char tokens, the resulting hash bit is at position (K + val) % 32.
# So K + val_A = 27, K + val_B = 26, K + val_C = 25 (modulo 32).
# This means val_A > val_B > val_C in the sequential order.
# E.g. (val_A, val_B, val_C) = (12, 11, 10) -> K = 15.  Chars: '6','5','4' or 'L','K','J'.
# Or (10, 9, 8) -> K = 17. Chars: 'J','I','H' or '4','3','2'.
# Or (9, 8, 7) -> K = 18. Chars: 'I','H','G' or '3','2','1'.
# Or (3, 2, 1) -> K = 24. Chars: 'C','B','A'.

# We want SHORT STEMS. Search 3-6 char alnum strings.

import itertools
chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789'

print('Searching 3-5 char stems whose hash=0x0c041001 with end_shift in {15,17,18,24}...')
print()
print('K=15 -> tokens 4,5,6 (or J,K,L) for ids 0x0e/08/04 respectively')
print('K=17 -> tokens 2,3,4 (or H,I,J)')
print('K=18 -> tokens 1,2,3 (or G,H,I)')
print('K=24 -> tokens A,B,C')
print()

ks_of_interest = {15, 17, 18, 24}
hit_count = 0
samples = {k: [] for k in ks_of_interest}

for L in [3, 4, 5]:
    for combo in itertools.product(chars, repeat=L):
        s = ''.join(combo)
        h, sh = calcHash_with_endsh(s)
        if h == 0x0c041001 and sh in ks_of_interest:
            samples[sh].append(s)
            hit_count += 1
    print(f'after L={L}: {hit_count} total')

print()
for k in sorted(samples):
    print(f'\nK={k} ({len(samples[k])} stems):')
    for s in samples[k][:30]:
        print(f'    {s}')
    if len(samples[k]) > 30:
        print(f'    ... {len(samples[k])-30} more')
