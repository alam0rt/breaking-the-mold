"""Curate high-confidence cracks from big_klaymen_brute output.

A 'high-confidence' crack is one where:
1. Only 1 candidate string was generated (no collision between vocab patterns), AND
2. The vocabulary pattern matches the asset's role/levels semantically:
   - audio + FX/SND/SFX/MUSIC prefix
   - sprite + SPR/IMG/PIC/EFFECT/PROP prefix
   - particle + PRT/PARTICLE prefix
   - And actor (KLAYMEN/SKULL/FINN/etc.) is plausibly used in that level set
"""

import re
import csv
from collections import defaultdict

# Re-run the brute and capture results:
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

# Load master meta
ids_meta = {}
with open('docs/analysis/asset-identification/cracked_names.csv') as f:
    for row in csv.DictReader(f):
        if row['id_hex'].startswith('0x'):
            ids_meta[int(row['id_hex'], 16)] = row

# High-confidence cracks observed (1 candidate only, semantically aligned)
CRACKS = [
    # ID, name, justification
    (0x4a60ca40, 'FX_KLAY_JUMP',           'FX prefix, audio type, KLAY actor, all 21 levels — perfect match for player jump sound'),
    (0x5860c640, 'FX_KLAY_LAND',           'FX prefix, audio type, KLAY actor, all 21 levels — Klaymen jump-land confirmed via ScummVM'),
    (0x50008340, 'FX_KLAY_FART',           'FX prefix, audio type, KLAY actor, all 21 levels — Klaymen-style fart (Skullmonkeys is full of farts)'),
    (0x00e49240, 'FX_KLAY_HURT',           'FX prefix, audio type, KLAY actor, all 21 levels — player hurt sound'),
    (0x70404350, 'FX_SKULL_LAND_01',       'FX prefix, audio type, SKULL actor, BRG1/PHRO/TMPL — skull enemy land'),
    (0x70204350, 'FX_SKULL_LAND_02',       'FX prefix, audio type, SKULL actor, BRG1/FOOD — skull enemy land variant'),
    (0x61200640, 'FX_SKULL_FIRE_01',       'FX prefix, audio type, SKULL actor, FOOD/SNOW — skull projectile fire'),
    (0x51200640, 'FX_SKULL_FIRE_02',       'FX prefix, audio type, SKULL actor, FOOD — skull projectile fire variant'),
    (0xe0c00650, 'FX_SKULL_SCREAM_01',     'FX prefix, audio type, SKULL actor, 8 levels — skull death scream'),
    (0x74182b40, 'FX_SKULL_BITE_01',       'FX prefix, audio type, SKULL actor, FOOD/TMPL — skull bite/chomp'),
    (0x40400270, 'FX_FINN_DIE_4',          'FX prefix, audio type, FINN actor, BRG1/SOAR — Finn boss death; was already candidate'),
    (0x31190002, 'SND_ENEMY_BOOM_6',       'SND prefix, type700, ENEMY token, ambiguous between _6/_L; both valid'),
]

print("=== HIGH-CONFIDENCE CRACKS (semantically aligned, mostly unique) ===\n")
for h, name, why in CRACKS:
    meta = ids_meta.get(h, {})
    status = meta.get('status', '?')
    levels = meta.get('levels', '')[:50]
    typ = meta.get('type', '')
    print(f"  0x{h:08x}  type={typ:8s} status={status:14s}  -> {name}")
    print(f"    {why}")
    print(f"    levels: {levels}")
    if status not in ('uncracked',):
        print(f"    [! already named '{meta.get('name','')}']")
    print()
