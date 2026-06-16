"""For each "verified anchor" (NO/YES/PAUSED/etc.), brute-force the SHORT
strings whose raw calcHash equals the anchor ID. If "NO" itself comes out,
then the wrap formula is what BLB uses. If different short words come out,
then BLB uses raw calcHash and the wrap-formula-derived anchors are wrong.
"""

import string
import itertools

ALPHA = string.ascii_uppercase + string.digits  # 36 chars

def calcHash(s):
    h = 0; sh = 0
    for ch in s:
        c = ord(ch)
        if 65 <= c <= 90: pass
        elif 97 <= c <= 122: c -= 32
        elif 48 <= c <= 57: c += 22
        else: continue
        sh = (sh + c - 64) & 31
        h ^= 1 << sh
    return h & 0xFFFFFFFF

targets = {
    0x29c0e211: 'NO',
    0x2ad0f011: 'YES',
    0x0ad0f813: 'PAUSED',
    0x68c0f413: 'QUIT',
    0x69c04050: 'CONTINUE',
    0x69c8f473: 'QUITGAME',

    # And the 3 Klaymen hits:
    0x5860c640: 'KLM-jumpland',
    0x9d406340: 'KLM-idlehead',
    0x5900c41e: 'KLM-idleblink',
}

print(f"Brute-forcing 1..6 char alpha strings against {len(targets)} targets...")

# 1..5 char first
hits = {h: [] for h in targets}
total = 0
for n in range(1, 6):
    for combo in itertools.product(ALPHA, repeat=n):
        s = ''.join(combo)
        ch = calcHash(s)
        if ch in hits:
            hits[ch].append(s)
        total += 1

print(f"Searched {total:,} strings (1..5 chars).")
for h, label in targets.items():
    matches = hits[h]
    print(f"\n  0x{h:08x}  ({label})  -> {len(matches)} hits")
    for s in matches[:6]:
        print(f"    {s}")
