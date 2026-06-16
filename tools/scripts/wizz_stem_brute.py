#!/usr/bin/env python3
"""Search wizard-themed words to see if any directly hashes to the WIZZ
family stem hash 0x0c041001."""

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


words = [
    'WIZARD','WIZRD','WIZZARD','WIZBOSS','WIZHIT','WIZZHIT','WIZZSPELL',
    'BOSS','BOSSWIZ','MAGE','MAGES','MAGEWIZ',
    'SPELL','SPELLA','SPELLS','SPELLZ','SPELLY','SPELLE',
    'CAST','CASTS','CASTZ','CASTE','CASTEY','CASTING',
    'HEX','HEXY','HEXES','HEXED',
    'ZAP','ZAPP','ZAPS','ZAPPY','ZAPZ',
    'BLAST','BLASTS','BLASTY','BLASTZ',
    'SHIELD','SHIELDY','SHIELDA','SHIELDS','SHLD','SHLDS',
    'GUARD','GUARDS','GUARDIAN',
    'BUBBLE','BUBBLES','BUBLE','BUBLES',
    'DOME','DOMES','DOMED','DOMEY',
    'ATTACK','ATAK','ATTAK','HIT','HITS','HITTS','HITTY',
    'BANG','BANGS','BANGY','BAM','BAMS',
    'BOOM','BOOMS','BOOMY',
    'WIN','WINS','WIND','WINDS',
    'GALE','GALES','GUST','GUSTS',
    'POW','POWS','POWA','POWER','POWERS',
    'EVIL','EVL','EVILY','MUNGO','MUNGY',
    'POOF','POOFS','POOFY',
    'WHIRL','WHIRLS','WHIRLY','TWIRL','TWIRLS',
    'SPIN','SPINS','SPINNY','SPNS',
    'PLAT','PLATS','PLATFORM',
    'CRACK','CRACKS','CRACKY','BREAK','BREAKS','BREAKY',
    'POP','POPS','POPPY','POPSI',
    'FLY','FLYS','FLOAT',
    # WIZZ specific from level lore
    'WIZZBANG','WIZZBANG1','WIZZBANG2','WIZZBANG3','WIZZSHIELD',
    'WIZZHIT1','WIZZSPELL1','WIZZBOSS1','WIZZBOSS2','WIZZBOSS3',
]

prefixes = ['', 'FX_','SFX_','SND_','MUS_','BGM_','MX_','M_','S_','A_','UI_']

print('STEM (hash 0x0c041001) candidates:')
hits = []
for pfx in prefixes:
    for w in words:
        s = pfx + w
        h, sh = calcHash_with_endsh(s)
        if h == 0x0c041001:
            hits.append((s, sh))

if hits:
    for s, sh in hits:
        print(f'  {s:30s}  end_sh={sh}')
else:
    print('  no stem matches')

# Now check: do any of these words appear as base of FX_BLOW_GALE_S+M sibling style?
# The siblings 0x0e041001 came from FX_BLOW_GALE_SM. Try different last-2 chars:
print()
print('FX_BLOW_GALE_S? family check (target ids 0x04041001/0x08041001/0x0e041001):')
for last in 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789':
    s = 'FX_BLOW_GALE_S' + last
    h, _ = calcHash_with_endsh(s)
    if h in {0x04041001, 0x08041001, 0x0e041001}:
        print(f'  {s} -> 0x{h:08x}')
print()
print('FX_BLOW_GALE_? (single last) check:')
for last in 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789':
    s = 'FX_BLOW_GALE_' + last
    h, _ = calcHash_with_endsh(s)
    if h in {0x04041001, 0x08041001, 0x0e041001}:
        print(f'  {s} -> 0x{h:08x}')
