"""Aggressive game-context brute test for low-floor targets.

Tests names commonly used in 90s game asset naming:
  - Single English word (5-8 chars)
  - Numbered variants (e.g., NAME1, NAME2)
  - Concatenations (KLAYBABY, BABYKING)
  - Skullmonkeys-specific terminology
"""
import sys, itertools
sys.path.insert(0, 'tools/scripts')
from calchash import calcHash

def rotr(x, n): return ((x >> n) | (x << (32-n))) & 0xFFFFFFFF
SEED = 0x28C0E011

# Each entry: (id_wrap_or_raw, namespace, popcount_floor, label)
TARGETS = [
    (0x28a0c119, 'wrap', 5, 'duckling sprite MENU'),
    (0x38a0c119, 'wrap', 6, 'duckling sister MENU'),
    (0x30a0c119, 'wrap', 7, 'duckling 76-frame MENU'),
    (0x68c01218, 'wrap', 8, 'MENU sprite'),
    (0x088c5011, 'wrap', 7, 'FINN sprite'),
    (0x28c080df, 'wrap', 7, 'DEMO text stamp global'),
    (0x21842018, 'wrap', 8, 'Klaymen base FINN'),
    (0x10882010, 'wrap', 8, 'EVIL sprite'),
    (0x2c182010, 'wrap', 8, 'MEGA boss body'),
    (0x0c01c014, 'wrap', 8, 'GLEN sprite'),
    (0x04084011, 'wrap', 8, 'global sprite (12 levels)'),
    (0x29808408, 'raw',  8, 'global audio (22 levels)'),
    (0x1847c001, 'raw',  8, 'type700 unknown ns'),
    (0x1847c001, 'wrap', 8, 'type700 unknown ns (wrap try)'),
]

# Massive game-vocab generator
roots = [
    # Skullmonkeys terms
    'KLAY','KLAYMEN','KLAYMAN','SKULL','SKULLY','MONKEY','MONK','APE','GORILLA',
    'JOE','EVO','EVIL','PUFF','GUM','GLEN','GLENN','YNTI','RAT','BIRD',
    'WORM','PHRO','FROG','HAMSTER','BAT','BUG','BEE','FLY',
    'NEVERHOOD','SKULLBOT','KLAYMEN3','KLAY3D','KLAY3',
    # Body parts / actions
    'BABY','BABE','KID','HERO','GUY','MAN','BOY','GIRL','DUDE','CHICK','TOT',
    'KING','PRINCE','PRIN','REGAL','CROWN','THRONE','RULE','LORD','LADY',
    'DUCK','GOOSE','HEN','OWL','BIRD','CHIRP','PEEP','DOVE','WREN','FOWL',
    'EYE','EYES','MOUTH','NOSE','EAR','HAIR','HAND','FOOT','HEAD','ARM','LEG',
    # Game terms
    'BOSS','MINI','LEVEL','STAGE','WORLD','ZONE','ROOM','SCENE','AREA',
    'WIN','LOSE','LOSS','GAIN','LIFE','LIVES','DIE','DEAD','ALIVE','REVIVE',
    'JUMP','RUN','HOP','SKIP','LEAP','DIVE','FLY','GLIDE','WALK','CRAWL',
    'HIT','HURT','OUCH','PAIN','STAB','BLOW','SHOT','SHOOT','BLAST','SPLAT',
    # Level codes
    'BOIL','BRG1','CAVE','CLOU','CRYS','CSTL','EGGS','FINN','FOOD','GLID',
    'HEAD','MEGA','MOSS','PHRO','RUNN','SCIE','SEVN','SNOW','SOAR','TMPL',
    'WEED','WIZZ','KLOG',
    # UI
    'MENU','TITLE','INTRO','OUTRO','LOGO','BANNER','SPLASH','SCREEN','SHOW',
    'GAME','OVER','READY','START','PRESS','PAUSE','PAUSED','QUIT','CONTINUE',
    'OPTION','SOUND','MUSIC','VOLUME','CONFIG','HELP','CTRL','BTN','BUTTON',
    'OK','YES','NO','BACK','NEXT','PREV','HOME','EXIT','BEGIN','END','LOAD','SAVE',
    'CHOOSE','PICK','SELECT','TARGET','CHOICE',
    # Misc
    'EARTH','EARTHWORM','WORM','JIM','EWJ','BOOM','BLAST','POW','WHAM','BAM',
    'SLAM','SMASH','CRASH','BUST','BURST','POP','PIT','HOLE','PAD',
    'POO','POOP','PEE','SPIT','DROOL','BARF','PUKE','VOMIT','SLIME','GOO',
    'SCRY','MAGIC','SPELL','POTION','POWER','LIGHT','DARK','SHINE','GLOW',
    'WIRE','PHONE','DEMO','BETA','TEST','DEV','OLD','NEW','REV',
    # Numbers as words
    'ONE','TWO','THREE','FOUR','FIVE','SIX','SEVEN','EIGHT','NINE','TEN',
    'ZERO','TRIPLE','TRIO','TWINS','PAIR','DUO',
]

# Suffixes / number variants
suffixes = ['','1','2','3','4','5','6','7','8','9','01','02','03','11','12','22','S','M','L',
            'A','B','C','X','Y','Z','SM','LG','EX','OK','BIG','TINY']
prefixes = ['','BIG','SUPER','MEGA','MINI','HYPER','3','2','1','S','SF','M','SM']

# Build candidate set
cands = set()
for r in roots:
    R = r.upper()
    for s in suffixes:
        for p in prefixes:
            cands.add(p + R + s)
    cands.add(R)

# Add concatenations of pairs from a small core
core = ['KLAY','SKULL','JOE','EVO','BABY','HERO','MENU','TITLE','GAME','BOSS',
        'KING','EARTH','WORM','EWJ','3D','LOGO','DEMO']
for a, b in itertools.product(core, core):
    if a != b:
        cands.add(a+b)
        cands.add(a+'_'+b)
        cands.add(a+'1')
        cands.add(a+'01')

# Try all of them
print(f'Testing {len(cands)} candidates...')
print()
hits_found = 0
for tid, ns, fl, label in TARGETS:
    target = rotr(tid ^ SEED, 27) if ns == 'wrap' else tid
    matches = [c for c in cands if calcHash(c) == target]
    if matches:
        hits_found += 1
        print(f'>>> 0x{tid:08x} {ns:<5s} fl={fl} {label}')
        for m in matches:
            print(f'      {m}')
    else:
        pass

if hits_found == 0:
    print('No hits. Real names use vocab outside our test set.')
