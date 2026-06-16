"""Try Skullmonkeys sprite naming conventions WITHOUT prefix.
Verbatim word, ACTOR_VERB, VERB_ACTOR, etc.
Also single-word raw asset names.
"""
import csv

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

unknowns = {}
with open('docs/analysis/asset-identification/cracked_names.csv') as f:
    for row in csv.DictReader(f):
        if row['id_hex'].startswith('0x') and row.get('status')=='uncracked':
            unknowns[int(row['id_hex'],16)] = row

ACTORS = ['KLAY','KLAYMEN','KLAYMAN','KMAN','SKULL','FINN','PHART','JOPS','JOLOPS','GLID',
          'BOSS','GUARD','BUG','BIRD','EGGS','PIG','CLAY','HEAD','MIKE','SHISH',
          'BIT','GUM','MONKEY','PLAYER','EVIL','SHRINEY','GUMBALL','JOE','JOYBOY',
          'EYEBALL','BABY','GUMHEAD','HEADJOE','SOUL','GHOST','SHRINE',
          'CLAYBALL','PROJ','PROJECTILE','MISSILE','ENEMY']
VERBS = ['IDLE','STAND','WALK','RUN','JUMP','FALL','LAND','LANDING','CLIMB','MIRROR','SHADOW',
         'SWIM','DUCK','SLIDE','GRAB','HOLD','PICKUP','THROW','TOSS','SHOOT','FIRE',
         'EAT','BITE','CHOMP','SPIT','LAUGH','SCREAM','CRY','GIGGLE','HURT','HIT','OUCH',
         'DIE','DEATH','SQUISH','FART','BURP','BLINK','SLEEP','SNORE',
         'CARRY','LIFT','DROP','STOMP','KICK','PUNCH','BLOCK','DASH','CHARGE',
         'CHECKPOINT','EXIT','ENTER','START','BEGIN','END','INTRO','OUTRO',
         'SPIN','BOUNCE','BUBBLE','FLY','GLIDE','HOVER','SAIL','FLOAT','RIDE',
         'OPEN','CLOSE','EXPLODE','BREAK','SLAM','SPAWN','RESPAWN','REVIVE',
         'WET','DRY','HOT','COLD','BIG','SMALL','TINY','HUGE','GIANT','BIG_FAT',
         'BURN','MELT','FREEZE','SHRINK','GROW','MORPH','TRANSFORM',
         'WIN','LOSE','VICTORY','DEFEAT',
         'SHADOWED','MIRRORED','REFLECT','GLOW','SHINE',
         'STEP','POSE','FRAME','SWING','TURN','SIT','BEND','LOOK','POINT',
         'WAVE','DANCE','TURNAROUND','TURN_AROUND',
         'STATE','BLOAT','VENT','BURST','EXPAND','BLOWUP','PUFF',
         'BREATHE','BREATH','HUFF','PANT','HUMP','RUMP','HEAD','BUTT','BELLY',
         'GHOST','SOUL','PHANTOM','SHADOW','CARRYING','CARRYBALL']
LEVELS = ['BOIL','BRG1','CAVE','CLOU','CRYS','CSTL','EGGS','EVIL','FINN','FOOD','GLEN','GLID',
          'HEAD','KLOG','MEGA','MENU','MOSS','PHRO','RUNN','SCIE','SEVN','SNOW','SOAR','TMPL','WEED','WIZZ']
MODS = ['','1','2','3','4','5','6','7','8','9','01','02','03','04','05','06','07','08','09','10',
        'A','B','C','D','E','F','G','H',
        'BIG','SM','LG','HI','LO','LEFT','RIGHT','UP','DN','IN','OUT','ON','OFF',
        'LOOP','START','END','MAIN','ALT','EXTRA','BONUS','TOP','BOT','BOTTOM',
        'NORTH','SOUTH','EAST','WEST','LIT','DARK','DIM','BRIGHT']

hits = {}
seen = set()
def try_(s):
    if s in seen: return
    seen.add(s)
    h = calcHash(s)
    if h in unknowns:
        hits.setdefault(h, []).append(s)

# 1. Single actor or verb (raw)
for a in ACTORS: try_(a)
for v in VERBS: try_(v)
for L in LEVELS: try_(L)

# 2. ACTOR_VERB (no prefix)
for a in ACTORS:
    for v in VERBS:
        for sep in ['_','']:
            try_(a + sep + v)
            for m in MODS:
                if m: try_(a + sep + v + sep + m)

# 3. VERB_ACTOR (reversed)
for v in VERBS:
    for a in ACTORS:
        for sep in ['_','']:
            try_(v + sep + a)

# 4. ACTOR_LEVEL (level-specific)
for a in ACTORS:
    for L in LEVELS:
        for sep in ['_','']:
            try_(a + sep + L)
            try_(L + sep + a)

# 5. ACTOR_VERB_LEVEL
for a in ACTORS:
    for v in VERBS:
        for L in LEVELS:
            try_('_'.join([a, v, L]))
            try_('_'.join([L, a, v]))

# 6. VERB_LEVEL (level-specific actions)
for v in VERBS:
    for L in LEVELS:
        for sep in ['_','']:
            try_(v + sep + L)
            try_(L + sep + v)
            for m in MODS:
                if m: try_(L + sep + v + sep + m)

# 7. LEVEL only with mods
for L in LEVELS:
    for v in VERBS:
        for m in MODS:
            if m: try_(L + '_' + v + '_' + m)

print(f'Tested {len(seen):,} unique strings, hits: {len(hits)}')

print()
print('=== Hits ===')
for hh, names in sorted(hits.items(), key=lambda x: (len(x[1]), x[0])):
    row = unknowns[hh]
    print(f"  0x{hh:08x}  popc={bin(hh).count('1')} type={row['type']:12s} levels={row['levels'][:50]} [{len(names)} cands]")
    for n in sorted(set(names), key=lambda s: (len(s), s))[:8]:
        print(f'      -> {n}')
    if len(names) > 8:
        print(f'      ... ({len(names)-8} more)')

# Save
with open('/tmp/sm_no_prefix_hits.csv','w') as f:
    f.write('id_hex,name,type,levels,popc,n_cands\n')
    for hh, names in sorted(hits.items()):
        row = unknowns[hh]
        for n in sorted(set(names)):
            f.write(f"0x{hh:08x},{n},{row['type']},{row['levels'][:60]},{bin(hh).count('1')},{len(set(names))}\n")
