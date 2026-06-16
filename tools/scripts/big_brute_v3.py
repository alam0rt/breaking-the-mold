"""Look for hits where the candidate prefix matches the asset's role exactly.

Role-prefix consistency rule:
  audio  -> FX/SFX/SND/MUS/BGM
  sprite -> SPR/IMG/PIC/ANI/EFFECT/PROP/OBJ
  particle -> PRT/PARTICLE/PART/EFFECT
  font   -> FONT
  background -> BG
  tile   -> TILE
  music  -> MUS/BGM/MUSIC

If the asset's known type is 'audio' and one candidate is FX_X_Y, it's much more
likely than candidates with non-audio prefixes.

Also enforce actor-context: KLAY+all-21-levels = player; FINN+BRG1/SOAR = Finn boss; etc.
"""
import csv
import itertools
import sys
import re
from collections import defaultdict

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

# Load metadata
ids_meta = {}
unknown_ids = set()
with open('docs/analysis/asset-identification/cracked_names.csv') as f:
    for row in csv.DictReader(f):
        if row['id_hex'].startswith('0x'):
            i = int(row['id_hex'], 16)
            ids_meta[i] = row
            if row.get('status') == 'uncracked':
                unknown_ids.add(i)

# role -> allowed prefixes
ROLE_PREFIXES = {
    'audio':      {'FX','SFX','SND','MUS','BGM'},
    'sprite':     {'SPR','IMG','PIC','ANI','ANIM','EFFECT','PROP','OBJ','GFX','GRPH','GRAPHIC','HUD','UI','VFX','MDL','MOD','MODEL'},
    'sprite|anim':{'SPR','IMG','PIC','ANI','ANIM','EFFECT','PROP','OBJ','GFX','GRPH','GRAPHIC','HUD','UI','VFX','MDL','MOD','MODEL'},
    'particle':   {'PRT','PARTICLE','PART','EFFECT','EFX','VFX'},
    'font':       {'FONT','TXT','TEXT'},
    'background': {'BG','BACKGROUND'},
    'tile':       {'TILE'},
    'music':      {'MUS','MUSIC','BGM'},
}

# Vocabulary (compact)
PREFIXES = ['FX','SFX','SND','MUS','MUSIC','BGM','SPR','SPRITE','IMG','PIC','ANIM','ANI',
            'BG','BACKGROUND','TILE','FRAME','PRT','PARTICLE','PART','EFFECT','EFX',
            'PAL','PALETTE','FONT','TXT','TEXT','ICON','OBJ','PROP','GFX','HUD','UI','VFX','MDL']
ACTORS = ['KLAY','KLAYMEN','KMAN','JOE','JOYBOY','MIKE','SHISH','PLAYER','MONKEY',
          'SKULL','BOSS','ENEMY','GUARD','EGG','EGGS','GUM','GUMBALL','PHART','SHRINEY',
          'JOPS','JOLOPS','JALOPS','JOLLIES','JOPSIE','BIT','CLAYBALL','FINN','KLOG',
          'GLID','EVIL','TROMBONE','BUG','WORM','SLUG','BIRD','BEE','FLY','ALIEN','ROBOT',
          'CLOUD','TREE','PLAT','PLATFORM','GUMHEAD','HEADJOE','SPHERE','BALL','BLOB','SLIME']
ACTIONS = ['WALK','RUN','JUMP','IDLE','BLINK','DIE','DEATH','HIT','HURT','GRAB','SHOOT',
           'FIRE','FALL','CLIMB','PUSH','PULL','OPEN','CLOSE','ENTER','EXIT','PICKUP',
           'THROW','BREATHE','LOOK','TURN','DUCK','SLIDE','STAND','ATTACK','LAND','LANDING',
           'SPIN','BOUNCE','LAUGH','SCREAM','CRY','YELL','GIGGLE','GASP','COUGH','SNEEZE',
           'FART','BURP','OUCH','BANG','BOOM','POP','PLOP','PLUNK','CHOMP','EAT','BITE',
           'SPIT','BLOW','HEADOFF','HEADON','HEADBUTT','INTRO','OUTRO','TITLE','CREDIT','DEMO',
           'SAY','SHOUT','GROAN','MOAN','HOWL','RAGE','ROAR','LIFT','DROP','TOSS','THROW',
           'KICK','PUNCH','SLASH','SWING','BLOCK','DASH','CHARGE','RECOIL','STAGGER','STUMBLE',
           'DAZE','REVIVE','RESPAWN','HEAL','POISON','BURN','FREEZE','SHOCK','GLOW','FADE','SHINE']
LEVELS = ['BOIL','BRG1','CAVE','CLOU','CRYS','CSTL','EGGS','EVIL','FINN','FOOD','GLID','HEAD',
          'KLOG','MEGA','MENU','MOSS','PHRO','RUNN','SCIE','SEVN','SNOW','SOAR','TMPL','WEED','WIZZ',
          'GLEN','BLB','BOSS','MAP','HUB']
MOD = ['','1','2','3','4','5','6','7','01','02','A','B','C','BIG','SM','LG','HI','LO','LEFT','RIGHT',
       'UP','DN','IN','OUT','ON','OFF','LOOP','START','STOP','MAIN','ALT']
SEPS = ['_','']

def gen():
    for pfx in PREFIXES:
        for act in ACTORS:
            for vrb in ACTIONS:
                for sep in SEPS:
                    yield (pfx, act, vrb, sep.join([pfx,act,vrb]))
                    for m in MOD[1:]:
                        yield (pfx, act, vrb, sep.join([pfx,act,vrb,m]))
        for lvl in LEVELS:
            for vrb in ACTIONS:
                for sep in SEPS:
                    yield (pfx, lvl, vrb, sep.join([pfx,lvl,vrb]))
                    for m in MOD[1:]:
                        yield (pfx, lvl, vrb, sep.join([pfx,lvl,vrb,m]))
            for sep in SEPS:
                yield (pfx, lvl, '', sep.join([pfx,lvl]))
                for m in MOD[1:]:
                    yield (pfx, lvl, '', sep.join([pfx,lvl,m]))

# Run with role-prefix filter
hits = defaultdict(list)
seen = set()
count = 0
for pfx, scope, vrb, s in gen():
    if s in seen: continue
    seen.add(s)
    h = calcHash(s)
    if h in unknown_ids:
        meta = ids_meta.get(h, {})
        role = meta.get('type', '')
        allowed = ROLE_PREFIXES.get(role, set())
        if not allowed or pfx in allowed:
            hits[h].append((pfx, scope, vrb, s))
    count += 1
    if count % 5_000_000 == 0:
        print(f"  ...{count:,} tried, {len(hits)} ids", file=sys.stderr)

print(f"\nTested {count:,} unique strings.", file=sys.stderr)
print(f"Hits filtered by role-prefix: {len(hits)} IDs", file=sys.stderr)

# Sort: unique hits first
def hit_key(item):
    h, names = item
    return (len(names), h)

# Show only hits with reasonable agreement
print()
print("=== Role-consistent hits ===")
unique_with_actor_match = []
for h, cands in sorted(hits.items(), key=hit_key):
    meta = ids_meta.get(h, {})
    levels = meta.get('levels', '')
    typ = meta.get('type', '')
    
    # Score each candidate by level/actor consistency
    scored = []
    for pfx, scope, vrb, s in cands:
        score = 0
        # Actor specifically tied to level coverage
        all_21 = sum(1 for L in LEVELS if L in levels) >= 19  # almost-all-levels
        if scope == 'KLAY' and all_21: score += 5  # player asset
        if scope == 'KMAN' and all_21: score += 5
        if scope == 'KLAYMEN' and all_21: score += 5
        if scope == 'PLAYER' and all_21: score += 5
        if scope == 'FINN' and ('BRG1' in levels and 'SOAR' in levels) and len(levels.split(';')) <= 4:
            score += 5
        # Level token in name matches level set
        if scope in LEVELS and scope in levels:
            score += 3
        scored.append((score, pfx, scope, vrb, s))
    scored.sort(key=lambda x: -x[0])
    best = scored[0]
    if len(cands) == 1 and best[0] >= 3:
        unique_with_actor_match.append((h, best, cands))
    elif len(cands) <= 3 and best[0] >= 5:
        unique_with_actor_match.append((h, best, cands))

# Print high-confidence
for h, best, cands in unique_with_actor_match:
    meta = ids_meta.get(h, {})
    levels = meta.get('levels', '')[:50]
    typ = meta.get('type', '')
    score, pfx, scope, vrb, s = best
    print(f"  0x{h:08x}  type={typ:8s} levels={levels:<50s}")
    print(f"     -> {s}  (score {score}, {len(cands)} cand{'s' if len(cands)>1 else ''})")

print(f"\nFound {len(unique_with_actor_match)} high-confidence single-candidate hits", file=sys.stderr)

# ALSO show the 17 role-consistent hits regardless of actor-match score
print()
print("=== ALL role-consistent hits ===")
for h, cands in sorted(hits.items(), key=lambda x: (len(x[1]), x[0])):
    meta = ids_meta.get(h, {})
    levels = meta.get('levels', '')[:50]
    typ = meta.get('type', '')
    print(f"  0x{h:08x}  type={typ:8s} levels={levels}")
    for pfx, scope, vrb, s in cands[:8]:
        print(f"     -> {s}")
    if len(cands) > 8:
        print(f"     ... ({len(cands)-8} more)")

# Save
with open('/home/sam/projects/btm/docs/analysis/asset-identification/raw_high_conf_hits.csv', 'w') as f:
    f.write('id_hex,name,score,type,levels,n_candidates\n')
    for h, best, cands in unique_with_actor_match:
        meta = ids_meta.get(h, {})
        score, pfx, scope, vrb, s = best
        f.write(f"0x{h:08x},{s},{score},{meta.get('type','')},{meta.get('levels','')[:80]},{len(cands)}\n")
