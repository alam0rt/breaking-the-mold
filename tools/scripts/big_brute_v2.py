"""Massive Skullmonkeys raw-namespace brute. Iterates over many
prefix/actor/action/level/digit combinations and reports unique hits.

Expanded vocab vs big_klaymen_brute.py."""
import csv
import itertools
import sys
import re

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

# Load unknown IDs
ids_meta = {}
unknown_ids = set()
with open('docs/analysis/asset-identification/cracked_names.csv') as f:
    for row in csv.DictReader(f):
        if row['id_hex'].startswith('0x'):
            i = int(row['id_hex'], 16)
            ids_meta[i] = row
            if row.get('status') == 'uncracked':
                unknown_ids.add(i)

print(f"Targeting {len(unknown_ids)} uncracked IDs", file=sys.stderr)

# Vocabulary
PREFIXES_ASSET = ['FX','SFX','SND','MUS','MUSIC','BGM','SPR','SPRITE','IMG','PIC','ANIM','ANI',
                  'BG','BACKGROUND','TILE','FRAME','PRT','PARTICLE','PART','EFFECT','EFX',
                  'PAL','PALETTE','FONT','TXT','TEXT','ICON','OBJ','PROP','ITEM','ENT','ENTITY',
                  'GFX','GRPH','GRAPHIC','HUD','UI','VFX','MDL','MOD','MODEL','VOX']

ACTORS_ALL = ['KLAY','KLAYMEN','KLAYMAN','KMAN','JOE','JOYBOY','MIKE','SHISH','PLAYER','MONKEY',
              'SKULL','SKULLY','BOSS','ENEMY','GUARD',
              'EGG','EGGS','GUM','GUMBALL','PHART','SHRINEY',
              'JOPS','JOLOPS','JALOPS','JOLLIES','JOPSIE','BIT','CLAYBALL','FINN','KLOG',
              'GLID','EVIL','TROMBONE','BUG','WORM','SLUG','BIRD','BEE','FLY','ALIEN','ROBOT',
              'TURTLE','FISH','SNAKE','RAT','SPIDER','CAT','DOG','BAT','OCTOPUS','CRAB',
              # SM specific from level-asset analysis
              'CLOUD','CLOUDS','TREE','TREES','PLANT','VINE','LEAF','BLB','SK','MNK','MNKY',
              'PLAT','PLATFORM','GUMHEAD','HEADJOE','SPHERE','BALL','BLOB','SLIME','GROBOT']

ACTIONS_ALL = ['WALK','RUN','JUMP','IDLE','BLINK','DIE','DEATH','HIT','HURT','GRAB','SHOOT',
               'FIRE','FALL','CLIMB','PUSH','PULL','OPEN','CLOSE','ENTER','EXIT','PICKUP',
               'THROW','BREATHE','LOOK','TURN','DUCK','SLIDE','STAND','ATTACK','DEFEND',
               'LAND','LANDING','SPIN','PIN','PLAT','TRACK','BOUNCE','LAUGH','SCREAM','CRY',
               'YELL','GIGGLE','GASP','COUGH','SNEEZE','FART','BURP','OUCH','BANG','BOOM',
               'POP','PLOP','PLUNK','CHOMP','EAT','BITE','SPIT','BLOW','INHALE','HEADOFF',
               'HEADON','IDLEHEAD','HEADSHOT','HEADBUTT','INTRO','OUTRO','TITLE','CREDIT',
               'DEMO','SAY','SHOUT','GROAN','MOAN','HOWL','RAGE','ROAR','LAUGH','GIGGLE',
               'SHOUT','BARK','HOOT','OINK','BLEH','BLAH','HUH','HEY','YO','YEAH','OK',
               'WAVE','POINT','REACH','GRAB','LIFT','DROP','TOSS','HOIST','THROW','KICK',
               'PUNCH','SLASH','SWING','BLOCK','PARRY','DASH','CHARGE','RECOIL','STAGGER',
               'STUMBLE','DAZE','REVIVE','RESPAWN','HEAL','POISON','BURN','FREEZE','SHOCK',
               'WET','DRY','BURN','GLOW','FADE','BLINK','SPARK','SHINE','SHIMMER',
               # Game-meta
               'WIN','LOSE','GAMEOVER','GAMEWIN','PASS','FAIL','LEVEL','STAGE','BONUS','GET','EARN',
               'COLLECT','POWERUP','POWERDOWN','LEVELUP']

# Level codes
LEVELS = ['BOIL','BRG1','CAVE','CLOU','CRYS','CSTL','EGGS','EVIL','FINN','FOOD','GLID','HEAD',
          'KLOG','MEGA','MENU','MOSS','PHRO','RUNN','SCIE','SEVN','SNOW','SOAR','TMPL','WEED',
          'WIZZ','GLEN','BLB','BOSS','MAP','HUB']

MOD = ['','1','2','3','4','5','6','7','8','9','01','02','03','04','05','A','B','C','D','E',
       'BIG','SM','LG','HI','LO','UP','DN','L','R','F','B','IN','OUT','ON','OFF','LEFT',
       'RIGHT','LOOP','START','STOP','MAIN','SUB','EXTRA','ALT','REV']

SEPS = ['_', '']

def gen():
    """Generate strings to test."""
    # PREFIX_ACTOR_ACTION (+modifier)
    for pfx, act, vrb in itertools.product(PREFIXES_ASSET, ACTORS_ALL, ACTIONS_ALL):
        for sep in SEPS:
            yield sep.join([pfx, act, vrb])
            for m in MOD[1:]:
                yield sep.join([pfx, act, vrb, m])

    # PREFIX_ACTION_ACTOR
    for pfx, vrb, act in itertools.product(PREFIXES_ASSET, ACTIONS_ALL, ACTORS_ALL):
        for sep in SEPS:
            yield sep.join([pfx, vrb, act])

    # PREFIX_LEVEL (+ digit/mod)
    for pfx, lvl in itertools.product(PREFIXES_ASSET, LEVELS):
        for sep in SEPS:
            yield sep.join([pfx, lvl])
            for m in MOD[1:]:
                yield sep.join([pfx, lvl, m])

    # PREFIX_LEVEL_ACTION
    for pfx, lvl, vrb in itertools.product(PREFIXES_ASSET, LEVELS, ACTIONS_ALL):
        for sep in SEPS:
            yield sep.join([pfx, lvl, vrb])

    # ACTOR_ACTION (no prefix)
    for act, vrb in itertools.product(ACTORS_ALL, ACTIONS_ALL):
        for sep in SEPS:
            yield sep.join([act, vrb])
            for m in MOD[1:]:
                yield sep.join([act, vrb, m])

    # PREFIX_LEVEL_ACTOR
    for pfx, lvl, act in itertools.product(PREFIXES_ASSET, LEVELS, ACTORS_ALL):
        for sep in SEPS:
            yield sep.join([pfx, lvl, act])

# Run
hits = {}
seen = set()
count = 0
for s in gen():
    if s in seen: continue
    seen.add(s)
    h = calcHash(s)
    if h in unknown_ids:
        hits.setdefault(h, []).append(s)
    count += 1
    if count % 5_000_000 == 0:
        print(f"  ...{count:,} tried, {len(hits)} ids hit", file=sys.stderr)

print(f"\nTested {count:,} unique strings.", file=sys.stderr)
print(f"Hits: {len(hits)} IDs (in unknown set)", file=sys.stderr)

# Sort hits: unique-candidate first
def hit_key(item):
    h, names = item
    return (len(names), h)

print()
print("=== Hits (uncracked IDs only) ===")
for h, names in sorted(hits.items(), key=hit_key):
    meta = ids_meta.get(h, {})
    levels = meta.get('levels', '')[:60]
    typ = meta.get('type', '')
    name_first = names[0] if len(names) <= 3 else f"{names[0]} (+{len(names)-1} more)"
    print(f"  0x{h:08x}  type={typ:8s}  levels={levels:<60s}")
    print(f"    -> {names[0]}")
    if 1 < len(names) <= 8:
        for n in names[1:]:
            print(f"       {n}")
    elif len(names) > 8:
        for n in names[1:6]:
            print(f"       {n}")
        print(f"       ... ({len(names)-6} more)")
    print()

# Save unique-hit cases (where exactly one candidate generated)
out = '/home/sam/projects/btm/docs/analysis/asset-identification/raw_brute_unique_hits.csv'
with open(out, 'w') as f:
    f.write('id_hex,name,type,levels,n_candidates\n')
    for h, names in hits.items():
        if len(names) == 1:
            meta = ids_meta.get(h, {})
            f.write(f"0x{h:08x},{names[0]},{meta.get('type','')},{meta.get('levels','')},1\n")
print(f"\nUnique-candidate hits written to {out}", file=sys.stderr)
