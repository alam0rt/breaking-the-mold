"""Try actor-first / no-prefix Skullmonkeys-specific patterns vs unknown raw IDs."""
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

ACTORS = ['KLAY','KLAYMEN','KLAYMAN','KMAN','MIKE','SHISH','JOE','JOYBOY',
          'PLAYER','MONKEY','BOY','GUY','HERO','MAN',
          'CLAYBALL','BALL','SKULLBALL','EYEBALL','GUMBALL','HEAD','JOEHEAD','HEADJOE',
          'EVIL','EVILGUY','EVILMONKEY','BAD','VILLAIN','BOSS',
          'BABY','TINY','SHRINKEN','SMALL','BIG','GIANT','HUGE',
          'GUM','SHRINEY','BIT','EGG','SHOW','BIRD','FISH','SLUG','BUG',
          'PHART','JOPS','JOLOPS','GUARD','SKULL','SOUL','GHOST','SPIRIT']
ACTIONS = ['IDLE','STAND','WALK','RUN','JUMP','FALL','LAND','LANDING','CLIMB',
           'SWIM','DUCK','SLIDE','HIDE','PUSH','PULL','GRAB','HOLD','PICKUP','PICK',
           'THROW','TOSS','HURL','SHOOT','SPIT','BLOW','BREATHE','BREATH','LAUGH','SCREAM',
           'CRY','GIGGLE','GASP','HURT','HIT','DIE','DEATH','DEAD','SQUISH',
           'FART','BURP','BUTT','SIT','BEND','TURN','LOOK','POINT','WAVE','DANCE',
           'BLINK','SLEEP','WAKE','SNORE','EAT','EATING','CHEW','CHOMP',
           'SPIN','BOUNCE','BUBBLE','FLY','FLYING','GLIDE','SAIL','FLOAT','RIDE',
           'CARRY','LIFT','DROP','RELEASE','STOMP','KICK','PUNCH',
           'MORPH','TRANSFORM','SHRINK','GROW','EXPAND','MELT','BURN',
           'WIN','LOSE','VICTORY','DEFEAT',
           'SWEAT','FROZEN','COLD','HOT','WET','DRY','SHOCKED',
           'CHECKPOINT','CKPT','EXIT','ENTER','START','BEGIN','END','FINISH',
           'INTRO','OUTRO','TITLE','PAUSE','RESET','RESPAWN','REVIVE','SPAWN',
           'SHADOW','MIRROR','REFLECT','GLOW','SHINE','LIGHT','DARK',
           'PORTAL','GATE','DOOR','EXIT','CHECKPOINT','LOOP','ANIM']

hits = {}
def try_(s):
    h = calcHash(s)
    if h in unknowns:
        hits.setdefault(h, []).append(s)

# Actor only
for a in ACTORS: try_(a)
# Actor + action with multiple separators and modifiers
for a in ACTORS:
    for v in ACTIONS:
        for sep in ['_', '']:
            try_(a + sep + v)
            for n in ['1','2','3','4','5','6','7','01','02','03','A','B','C','D','LOOP','START','END','MAIN','ALT']:
                for sep2 in ['_', '']:
                    try_(a + sep + v + sep2 + n)
        try_(v + '_' + a)
# Just verbs (sometimes assets are just verbs)
for v in ACTIONS:
    try_(v)
    for n in ['1','2','3','01','LOOP','MAIN','ALT']:
        try_(v + '_' + n)
# Level + verb
LEVELS = ['BOIL','BRG1','CAVE','CLOU','CRYS','CSTL','EGGS','EVIL','FINN','FOOD','GLEN','GLID',
          'HEAD','KLOG','MEGA','MENU','MOSS','PHRO','RUNN','SCIE','SEVN','SNOW','SOAR','TMPL','WEED','WIZZ']
for L in LEVELS:
    for v in ACTIONS:
        for sep in ['_','']:
            try_(L + sep + v)
            for n in ['1','2','3','01','A','B']:
                try_(L + sep + v + '_' + n)

print(f'Hits: {len(hits)} ids')
for hh, names in sorted(hits.items()):
    row = unknowns[hh]
    levels = row['levels'][:40] or '-'
    print(f"  0x{hh:08x}  popc={bin(hh).count('1')} type={row['type']:8s} levels={levels}")
    for n in names[:6]:
        print(f'      -> {n}')
    if len(names) > 6:
        print(f'      ... ({len(names)-6} more)')
