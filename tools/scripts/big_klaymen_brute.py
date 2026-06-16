"""Massive Klaymen-named brute-force using Skullmonkeys/Neverhood vocabulary.
Builds many FX_/MUSIC_/SPR_/BG_/PRT_ names with various roots."""

import csv
import itertools
import sys

# --- calcHash ---
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

# --- load master ---
ids = set()
ids_meta = {}
with open('docs/analysis/asset-identification/cracked_names.csv') as f:
    for row in csv.DictReader(f):
        if row['id_hex'].startswith('0x'):
            i = int(row['id_hex'], 16)
            ids.add(i)
            ids_meta[i] = row

# --- vocabulary ---
PREFIXES = [
    'FX', 'SFX', 'SND', 'MUS', 'MUSIC', 'BGM',
    'SPR', 'SPRITE', 'IMG', 'PIC', 'ANIM', 'ANI',
    'BG', 'BACKGROUND', 'TILE', 'FRAME',
    'PRT', 'PARTICLE', 'PART', 'EFFECT', 'EFX', 'FX',
    'PAL', 'PALETTE', 'FONT', 'TXT', 'TEXT', 'ICON',
    'OBJ', 'PROP', 'ITEM',
]
ACTORS = [
    'KLAY', 'KLAYMEN', 'KLAYMAN', 'KMAN', 'JOE', 'JOYBOY', 'MIKE', 'SHISH',
    'PLAYER', 'MONKEY', 'SKULL', 'BOSS', 'ENEMY', 'GUARD',
    'EGG', 'EGGS', 'GUM', 'GUMBALL',
    # Skullmonkeys-specific
    'PHART', 'SHRINEY', 'JOPS', 'JOLOPS', 'JALOPS', 'JOLLIES', 'JOPSIE',
    'BIT', 'CLAYBALL', 'FINN', 'KLOG', 'GLID', 'EVIL', 'TROMBONE',
    # Generic creature
    'BUG', 'WORM', 'SLUG', 'BIRD', 'BEE', 'FLY', 'ALIEN', 'ROBOT',
]
ACTIONS = [
    'WALK', 'RUN', 'JUMP', 'IDLE', 'BLINK', 'DIE', 'DEATH', 'HIT', 'HURT',
    'GRAB', 'SHOOT', 'FIRE', 'FALL', 'CLIMB', 'PUSH', 'PULL', 'OPEN', 'CLOSE',
    'ENTER', 'EXIT', 'PICKUP', 'THROW', 'BREATHE', 'LOOK', 'TURN', 'DUCK',
    'SLIDE', 'STAND', 'ATTACK', 'DEFEND', 'LAND', 'LANDING', 'SPIN', 'PIN',
    'PLAT', 'PLATFORM', 'TRACK', 'BOUNCE', 'BOUNCING',
    'LAUGH', 'SCREAM', 'CRY', 'YELL', 'GIGGLE', 'GASP', 'COUGH', 'SNEEZE',
    'FART', 'BURP', 'OUCH', 'BANG', 'BOOM', 'POP', 'PLOP', 'PLUNK',
    'CHOMP', 'EAT', 'BITE', 'SPIT', 'BLOW', 'INHALE',
    'HEADOFF', 'HEADON', 'IDLEHEAD', 'HEADSHOT', 'HEADBUTT',
    'INTRO', 'OUTRO', 'TITLE', 'CREDIT', 'DEMO',
]
LEVELS = [
    'BOIL','BRG1','CAVE','CLOU','CRYS','CSTL','EGGS','EVIL','FINN','FOOD','GLID','HEAD',
    'KLOG','MEGA','MENU','MOSS','PHRO','RUNN','SCIE','SEVN','SNOW','SOAR','TMPL','WEED','WIZZ',
    'GLEN','BLB',
]
MODIFIERS = ['', '1','2','3','4','5','6','7','8','9','01','02','03',
             'A','B','C','D','BIG','SM','SMALL','LG','LARGE',
             'NEW','OLD','HI','LO','UP','DN','LEFT','RIGHT','L','R',
             'IN','OUT','ON','OFF']
SEPS = ['_','']

def gen():
    """Generate candidate strings."""
    # PREFIX_ACTOR_ACTION (KLAY-centric)
    for pfx, act, vrb, mod in itertools.product(PREFIXES, ACTORS, ACTIONS, MODIFIERS):
        for sep in SEPS:
            yield sep.join([pfx, act, vrb] + ([mod] if mod else []))

    # ACTOR_ACTION (no prefix)
    for act, vrb, mod in itertools.product(ACTORS, ACTIONS, MODIFIERS):
        for sep in SEPS:
            yield sep.join([act, vrb] + ([mod] if mod else []))

    # ACTION_ACTOR
    for vrb, act, mod in itertools.product(ACTIONS, ACTORS, MODIFIERS):
        for sep in SEPS:
            yield sep.join([vrb, act] + ([mod] if mod else []))

    # PREFIX_LEVEL_X
    for pfx, lvl, mod in itertools.product(PREFIXES, LEVELS, MODIFIERS):
        for sep in SEPS:
            yield sep.join([pfx, lvl] + ([mod] if mod else []))

    # PREFIX_ACTION_LEVEL
    for pfx, vrb, lvl in itertools.product(PREFIXES, ACTIONS, LEVELS):
        for sep in SEPS:
            yield sep.join([pfx, vrb, lvl])

# --- run ---
hits = {}  # id -> list of names
seen = set()
count = 0
for s in gen():
    if s in seen: continue
    seen.add(s)
    h = calcHash(s)
    if h in ids:
        hits.setdefault(h, []).append(s)
    count += 1
    if count % 5_000_000 == 0:
        print(f"  ...{count:,} tried", file=sys.stderr)

print(f"\nTested {count:,} unique strings.\nHits: {len(hits)} IDs\n")

# Sort hits by status (uncracked first), then number of candidates (1 = unique)
def hit_key(item):
    h, names = item
    meta = ids_meta.get(h, {})
    status = meta.get('status', '')
    return (
        0 if status == 'uncracked' else 1,
        len(names),
        h
    )

for h, names in sorted(hits.items(), key=hit_key):
    meta = ids_meta.get(h, {})
    status = meta.get('status', '?')
    levels = meta.get('levels', '')[:60]
    typ = meta.get('type', '')
    name_first = names[0] if len(names) <= 5 else f"{names[0]} (+{len(names)-1} more)"
    print(f"  0x{h:08x}  type={typ:8s} status={status:14s} levels={levels:<60s}  -> {name_first}")
    if 1 < len(names) <= 8:
        for n in names[1:]:
            print(f"                                                                                                    {n}")
