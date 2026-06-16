#!/usr/bin/env python3
"""Search: prefix + 'BLOW_GALE_' + 2 chars = sibling of FX_BLOW_GALE_SM.

We know FX_BLOW_GALE_SM hashes to 0x0e041001 (one of three WIZZ family ids).
Try replacing the last 2 chars to see if any combination hits the other 2 ids:
0x04041001 or 0x08041001.
"""

def calcHash(s):
    h, sh = 0, 0
    for ch in s:
        c = ord(ch)
        if 65 <= c <= 90: pass
        elif 97 <= c <= 122: c -= 32
        elif 48 <= c <= 57: c += 22
        else: continue
        sh = (sh + c - 64) & 31
        h ^= 1 << sh
    return h & 0xFFFFFFFF

targets = {0x04041001, 0x08041001, 0x0e041001}

chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789'

bases = [
    'FX_BLOW_GALE_',
    'FX_BLOW_GUST_',
    'FX_BLOW_GAL_',
    'FX_BLOW_GST_',
    'FX_GALE_',
    'FX_GUST_',
    'FX_GALE_BIG_',
    'FX_GALE_BG_',
    'FX_GALE_LG_',
    'FX_GALE_SM_',
    'FX_GALE_LO_',
    'FX_GALE_HI_',
    'FX_WIZZ_GALE_',
    'FX_WIZZ_BLOW_',
    'FX_WIZZ_WIND_',
    'FX_WIZZ_BOSS_',
    'FX_WIZZ_SPELL_',
    'FX_WIZZ_CAST_',
    'FX_WIZZ_HEX_',
    'FX_WIZZ_HIT_',
    'FX_BOSS_WIZZ_',
    'FX_BOSS_HIT_',
    'FX_BOSS_SPELL_',
    'FX_BOSS_CAST_',
    'FX_BOSS_BANG_',
    'FX_BOSS_HEX_',
    'FX_BOSS_ZAP_',
    'FX_SPELL_',
    'FX_CAST_',
    'FX_HEX_',
    'FX_ZAP_',
    'FX_BLAST_',
    'FX_POOF_',
    'FX_BANG_',
    'FX_BOOM_',
    'FX_WIN_',
    'FX_WIND_',
    'FX_HISS_',
    'FX_WHIP_',
    'FX_PUFF_',
    'FX_HOWL_',
    'FX_WHIRL_',
    'FX_SPIN_',
    'FX_BREAK_',
    'FX_CRACK_',
    'FX_POP_',
]

hits = []
for base in bases:
    for ch1 in chars:
        for ch2 in chars:
            s = base + ch1 + ch2
            h = calcHash(s)
            if h in targets:
                hits.append((s, h))
    # 1 char  
    for ch1 in chars:
        s = base + ch1
        h = calcHash(s)
        if h in targets:
            hits.append((s, h))
    # 3 chars (digits)
    for ch1 in chars[:36]:
        for ch2 in chars[:36]:
            for ch3 in chars[:36]:
                s = base + ch1 + ch2 + ch3
                h = calcHash(s)
                if h in targets:
                    hits.append((s, h))

print(f'Found {len(hits)} hits')
# Sort and group
from collections import defaultdict
by_target = defaultdict(list)
for s, h in hits:
    by_target[h].append(s)

for t in sorted(by_target):
    print(f'\n0x{t:08x}:')
    for s in sorted(by_target[t], key=lambda x: (len(x), x))[:30]:
        print(f'    {s}')
    if len(by_target[t]) > 30:
        print(f'    ... ({len(by_target[t])-30} more)')
