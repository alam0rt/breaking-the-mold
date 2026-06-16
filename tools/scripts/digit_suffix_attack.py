"""Targeted attack: try [PREFIX]_[NN] and [PREFIX][NN] patterns where NN is a number,
to find digit-suffix interpretations of unknown ids. The point: digits 0-9 are
step-equivalent to letters F-O, so any ID we've cracked with a letter-only string
might have a more readable digit-form interpretation."""
import os
import sys

def calcHash(s):
    h, sh = 0, 0
    for c in s.upper():
        c = ord(c)
        if 65 <= c <= 90:
            pass
        elif 48 <= c <= 57:
            c += 22
        else:
            continue
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
csv = 'docs/reference/asset-ids-master.csv'
with open(csv) as f:
    next(f)
    for line in f:
        parts = line.strip().split(',')
        if len(parts) >= 1 and parts[0].startswith('0x'):
            all_ids.add(int(parts[0], 16))
print(f"Total ids: {len(all_ids)}")

# Verified anchors (skip — already known)
anchors = {0x29c0e211, 0x2ad0f011, 0x0ad0f813, 0x68c0f413, 0x69c04050, 0x69c8f473}
targets = all_ids - anchors

# Vocabulary: prefixes that often pair with numeric suffix
prefixes = [
    # generic UI
    'MENU','LEVEL','STAGE','OPTION','OPT','SELECT','ICON','BTN','BUTTON','LABEL',
    'CARD','PAGE','ITEM','SLOT','SAVE','LOAD','BG','BACK','TITLE','NAME','TEXT',
    'CURSOR','ARROW','MARK','HILITE','HIGHLIGHT','FRAME','BORDER','BAR','BOX',
    'NUM','DIGIT','LETTER','GLYPH','FONT','CHAR','LIVES','LIFE','SCORE','HUD','TIMER',
    'KEY','GEM','COIN','HEART','HEALTH','EXTRA','BONUS','POWER','CONTROLS','DEMO',
    # level codes
    'BOIL','BRG1','CAVE','CLOU','CRYS','CSTL','EGGS','EVIL','FINN','FOOD','GLEN',
    'GLID','HEAD','KLOG','MEGA','MOSS','PHRO','RUNN','SCIE','SEVN','SNOW','SOAR',
    'TMPL','WEED','WIZZ',
    # animations
    'ANIM','ANIMS','FRM','FR','FRAME','SPRT','SPR','SPRITE','TILE','XTILE','TILES',
    'SEQ','SEQS','MASTER','OVERLAY','CLUT','PALETTE','PAL','SAMPLE','SFX','MUSIC',
    'WAVE','VOICE','VAB','SEQ','VOX','SND','SOUND',
    # actions/states
    'WALK','RUN','JUMP','IDLE','HIT','DIE','DEAD','WIN','LOSE','APPEAR','VANISH',
    'SPAWN','EXPLODE','FIRE','SHOOT','OPEN','CLOSE','DROP','RISE','FALL','TURN',
    # body parts
    'HEAD','BODY','ARM','ARMS','LEG','LEGS','FOOT','FEET','HAND','HANDS','EYE','EYES',
    'MOUTH','TAIL','WING','HAIR','TORSO','HIP','HIPS','BUTT','ASS','BELLY',
    # game-specific
    'KLAY','KLAYMAN','SKULL','SKULLY','MONKEY','MONKEYS','MUD','GOIN','GLO','VOG',
    'IDOL','BOSS','ENEMY','PLAYER','CHEW','EAT','SPIT','SLAM','SLAP','PUNCH','KICK',
    # directions
    'UP','DOWN','LEFT','RIGHT','NORTH','SOUTH','EAST','WEST','TOP','BOT','BOTTOM','MID','MIDDLE',
    # qualifiers
    'BIG','SMALL','LARGE','TINY','RED','BLUE','GREEN','YELLOW','BLACK','WHITE',
    # password / menu
    'PWORD','PASSWORD','PASS','CODE','SCRAB','SCRABLINK','SCRABS',
]

# 2-digit and 3-digit suffix counters
nums = []
for n in range(0, 100):
    nums.append(f'{n:02d}')   # zero-padded
    nums.append(f'{n}')
for n in range(100, 256):
    nums.append(f'{n}')

# Try patterns
tries = 0
hits = []
for p in prefixes:
    for n in nums:
        for joiner in ['', '_']:
            for s in (p + joiner + n, n + joiner + p):
                tries += 1
                aid = asset_id(s)
                if aid in targets:
                    hits.append((aid, s, len(s)))

# Also try [PREFIX]_[ROLE]_[NN]
roles = ['IDLE','RUN','WALK','JUMP','HIT','DIE','APPEAR','SPAWN','OPEN','CLOSE',
         'ICON','MARK','BG','TITLE','TEXT','CARD','SELECT','HILITE','GLYPH','FRAME',
         'KEY','GEM','COIN','LIFE','LIVES','HUD','HEAD','BODY','ARM','ARMS','LEG',
         'WING','HAND','EYE','BOSS','BUSH','BLOCK','BRICK','PILLAR','SPIKE','SPIKES',
         'TILE','TILES','SLAM','SHOT','FIRE','EXPLODE','GLOW','SPARK','TRAIL']
print(f"3-token search: {len(prefixes)} x {len(roles)} x {len(nums)} ...")
for p in prefixes:
    for r in roles:
        for n in nums[:200]:  # cap at 0-99 + 0-99 padded
            for joiner in ['_', '']:
                s = p + joiner + r + joiner + n
                tries += 1
                aid = asset_id(s)
                if aid in targets:
                    hits.append((aid, s, len(s)))

print(f"Tried {tries} patterns -> {len(hits)} hits")

# Sort by length (shorter ones more likely real names)
hits.sort(key=lambda x: (x[2], x[1]))
unique_ids = set()
shown = 0
for aid, s, L in hits:
    print(f"  0x{aid:08x}  {s}")
    unique_ids.add(aid)
    shown += 1
    if shown > 200:
        print(f"  ... ({len(hits) - shown} more)")
        break
print(f"\nUnique target ids hit: {len(unique_ids)}")
