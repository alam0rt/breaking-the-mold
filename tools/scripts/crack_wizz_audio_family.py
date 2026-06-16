#!/usr/bin/env python3
"""Brute wizard-boss-themed names for the WIZZ audio family.

Game context: WIZZ = Wizard boss fight. Spinning platforms, red shield,
spell casting, platform destruction. 5 hits to defeat.

Targets (3-id family with sequential tokens):
  0x04041001  popc=4  WIZZ-only
  0x08041001  popc=4  WIZZ-only
  0x0e041001  popc=6  WIZZ-only
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

words = [
    'SPELL','CAST','HEX','JINX','CURSE','CHARM','RUNE','GLYPH','WARD','BOLT',
    'ZAP','BAM','POW','BANG','BOOM','POOF','PIFF','SIZZ','FIZZ',
    'ZAPP','ZAPS',
    'MAGIC','MAJIK','MYSTIC','OCCULT','ARCANE','MANA','POWER',
    'WIZ','WIZZ','WIZE','WIZD','WIZRD','MAGE','SAGE','SEER','MERLIN',
    'WAND','STAFF','HOOD','CAPE','ROBE',
    'SHLD','SHIELD','SLD','GUARD','BLOCK','BARR','FORCE','FIELD','BUBL','DOME',
    'BOSS','BSS','MUNGO','PANG','EVIL','LORD','KING','GOD','DEMI',
    'HIT','HITS','HT','OUCH','OW','OOF','UGH','GAH',
    'PLAT','SPIN','TWIRL','TURN','REV','REVS','ORBIT','RING','GEAR',
    'BREAK','SMASH','CRACK','POP','SHATTER','BURST','BUST','RUMBL',
    'FALL','DROP','CRUMB','TUMBL','RUIN',
    'WIN','LOS','LOSE','WON',
    'WIND','GALE','GUST','BLOW','HOWL','PUFF','WHIZ','WHEW','HISS','WHIP',
    'WUSH','WHSH','SWSH','WHIRL','WHIRR','SWIRL',
    'FX','SFX','SND','SE','S','M','MX','MFX',
    'AMB','LOOP','MAIN','THM','ALT','ECHO','TONE','HUM','BUZZ',
    'WAH','WOW','WAA','WHA','WOO','LOW','HIGH',
]

prefixes = ['', 'FX_', 'SFX_', 'SND_', 'AUDIO_', 'A_', 'M_', 'S_',
            'WIZ_', 'WIZZ_', 'BOSS_', 'EVIL_', 'MAGE_',
            'SPELL_', 'CAST_', 'HEX_', 'POW_', 'MAGIC_']
suffixes = [''] + [str(i) for i in range(10)] + [
    '_1','_2','_3','_4','_5','_6','_7','_8','_9','_0','_01','_02','_03',
    'A','B','C','D','LO','HI','SM','BG','UP','DN','IN','OUT','LOOP','END','MAIN','ALT',
    'ON','OFF','WAH','WIN','LOSE','HIT','MISS',
    '1A','1B','2A','2B','3A','3B',
]

candidates = {t: [] for t in targets}
seen = set()

for w in words:
    for pfx in prefixes:
        for sfx in suffixes:
            if sfx:
                for sep in ['','_']:
                    s = pfx + w + sep + sfx
                    if s in seen: continue
                    seen.add(s)
                    if calcHash(s) in candidates:
                        candidates[calcHash(s)].append(s)
            else:
                s = pfx + w
                if s in seen: continue
                seen.add(s)
                if calcHash(s) in candidates:
                    candidates[calcHash(s)].append(s)

# Two-word combos
for w1 in words[:80]:
    for w2 in ['HIT','BLAST','POW','ZAP','BANG','BOOM','SPELL','CAST','CRY',
              'SHIELD','SHLD','BUBBLE','POOF','RING','MISS','LAND','HEAL']:
        for sep in ['','_']:
            s = w1 + sep + w2
            if s in seen: continue
            seen.add(s)
            if calcHash(s) in candidates:
                candidates[calcHash(s)].append(s)

print(f'Tested {len(seen):,} strings')
print()
for tid in sorted(targets):
    print(f'0x{tid:08x}:')
    pool = sorted(set(candidates[tid]), key=lambda x: (len(x), x))
    for s in pool[:40]:
        print(f'    {s}')
    if len(pool) > 40:
        print(f'    ... ({len(pool)-40} more)')
    print()
#!/usr/bin/env python3
"""Brute candidate names for WIZZ low-floor audio family.

Targets:
  0x04041001  popc=4  WIZZ
  0x08041001  popc=4  WIZZ
  0x0e041001  popc=6  WIZZ  (was a strict-fx hit: FX_BLOW_GALE_SM)

Family analysis confirmed all three share stem `calcHash=0x0c041001` (popc=5),
with single-char tokens at consecutive bit positions 25..27.
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

# Wind/wizard-themed 3-7 char dictionary
words = [
    'WIND','GALE','BLOW','HOWL','GUST','PUFF','WAFT','HISS','WHIZ','WHEW','WUSH','WHSH','SWSH',
    'AIR','BREZ','BRTH','BREE','BRZE','GULP','SUCK','FAN','FANS',
    'WIN','BLO','BL','HOW','GAL','GST','GLE','GAS','GIZ','BLW',
    'RAIN','SNOW','HAIL','MIST','FOG','SMOG','HAZE','MOON','STAR','SUN',
    'BLOW','BLEW','GLOW','GROW','SHOO','BUFF','HUFF','CHUF','POOF','POOM',
    'HOOO','OOO','OOOH','AHHH','UHHH','YEAH',
    'SHRP','SOFT','HARD','DEEP','HIGH','LOW','BIG','SMAL','SML','LRG',
    'YEL','YELL','EE','OOO','HOO','HOOA','OOAA',
    # Wizard
    'WIZ','WIZ1','WIZ2','WIZ3','WIZRD','MAGE','SPELL','POOF','ZAP',
    'CAST','SUMN','SUMM','POWER','MOTM',
    # Spell sounds
    'BANG','BING','BONG','BOOM','PIFF','WHIP','SWAP','POOF','TONE','TUNE',
    # Common audio category prefixes
    'BG','BGM','MX','MAR','AMB','LOOP','MAIN','THM','THEME','MUS',
    # Generic
    'IN','OUT','UP','DN','HI','LO',
    'FX','SFX','SND','SD','S','M',
]

# Candidates can be: just word, with prefix, with suffix
prefixes = ['', 'FX_', 'SFX_', 'SND_', 'A_', 'M_', 'S_']
suffixes = [''] + [str(i) for i in range(10)] + [
    '_1','_2','_3','_4','_5','_6','_7','_8','_9',
    'LO','HI','SM','BG','LG','UP','DN','IN','OUT','LOOP','END','MAIN','ALT','ON','OFF',
    'A','B','C','D','E','F','G',
]

candidates = {t: [] for t in targets}
seen = set()
for w in words:
    for pfx in prefixes:
        for sfx in suffixes:
            for sep in ['', '_']:
                s = pfx + w + sep + sfx if sfx else pfx + w
                if s in seen:
                    continue
                seen.add(s)
                h = calcHash(s)
                if h in candidates:
                    candidates[h].append(s)

print(f'Tested {len(seen):,} strings')
print()
for tid in sorted(targets):
    print(f'0x{tid:08x}:')
    for s in sorted(set(candidates[tid]), key=lambda x: (len(x), x))[:30]:
        print(f'    {s}')
    print()
