"""Sister-asset hunt: probe near known cracks for related family IDs.
For each verified family root, brute many verb+suffix combos; report only uncracked hits.
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

allids = {}
with open('docs/analysis/asset-identification/cracked_names.csv') as f:
    for row in csv.DictReader(f):
        if row['id_hex'].startswith('0x'):
            allids[int(row['id_hex'],16)] = row

VERBS = ['HIT','TURN','WALK','IDLE','JUMP','LAND','LANDING','FALL','RUN','BLINK','DIE','DEATH','HURT',
         'GRAB','SHOOT','LAUGH','SCREAM','CRY','GASP','GIGGLE','HOWL','GROAN','MOAN','BAWL',
         'FART','BURP','SAY','SHOUT','TALK','EAT','BITE','CHOMP','CHEW','SPIT','BREATH','BREATHE','BLOW','PUFF',
         'SLEEP','WAKE','SNORE','REST','SIT','BEND','LOOK','POINT','SPIN','BOUNCE',
         'DEFEAT','VICTORY','WIN','LOSE','DODGE','BLOCK','DASH','CHARGE','RECOIL','STAGGER','REVIVE',
         'EXPLODE','BURST','POP','BLAST','BOOM','SLAM','THUMP','THUD','TINK','TONK','CLANG',
         'SHAKE','TWIST','SWAY','BOB','ROLL','CLIMB','SWIM','DUCK','SLIDE',
         'STAND','PEEK','HIDE','DROP','RELEASE','CARRY','LIFT','LEAP',
         'GHOST','SOUL','PHANTOM','SHADOW','MIRROR','GLOW','SHINE',
         'PICKUP','PICK','THROW','TOSS','HURL','LAUNCH','FIRE','RECOIL',
         'STOMP','KICK','PUNCH','SLASH','SWING','HUFF','PANT','SIGH',
         'WIN','LOSE','VICTORY','DEFEAT','SUCCESS','FAIL','PASS','FAILURE',
         'PLOP','PLUNK','PLOOSH','SPLOOSH','SPLAT','SPLASH','SQUISH','SQUASH',
         'BANG','SHOOM','WHOOSH','WHIZZ','WOOSH','VROOM',
         'INTRO','OUTRO','START','END','FINISH','BEGIN',
         'OPEN','CLOSE','ENTER','EXIT','EXPLODE','BREAK','SHATTER',
         'STEP','FOOTSTEP','FOOTFALL','PAW','PAWS',
         'SQUEAK','SQUEAL','SQUEEZE','SQUELCH','BLEEP','BEEP','BUZZ','BLIP',
         'PING','PONG','DING','DONG','BELL','CHIRP','TWEET','HOOT','BARK','MEOW','MOO','BAA','OINK',
         'SLURP','GULP','MUNCH','SLAM','SLOSH','CRUNCH','GRIND','GRIT',
         'CRACKLE','SIZZLE','HISS','RUSTLE','RUMBLE','WHIRR','HONK','BLAST','BLARE',
         'TOOT','TOOTLE','HORN','TUBA','TROMBONE',
         'WHACK','WALLOP','BOP','TAP','TICK','TOCK','CLICK','CLUNK',
         'SING','HUM','WHISTLE','WHIMPER','WHINE','MUTTER','MUMBLE',
         'YO','HEY','UH','HUH','OW','OUCH','PRAY','CHANT',
         'STIR','POKE','PROD','TICKLE','SCRATCH',
         'FLAP','FLUTTER','SOAR','DIVE','SWOOP','PERCH',
         'WIND','GUST','BREEZE','GALE','STORM','THUNDER','LIGHTNING',
         'WATER','SPLASH','RIPPLE','WAVE','TIDE','SMACK','SLAP','BUMP','BANG',
         'PRESS','RELEASE','PULL','PULLBACK','READY','AIM','LOAD',
         'CARRY','CARRYBALL','CARRYING','HOLDBALL','HOLD',
         'DROP','RELEASE','TOSS','TOSSDOWN','TOSSUP',
         'WET','SOAKED','MUDDY','SLIMY','GOOEY','OILY','GREASY']

PFXS = [
    # boss head family
    'FX_BOSS_HEAD_','FX_HEAD_BOSS_','FX_HEAD_','FX_BIG_HEAD_','FX_HEADJOE_','FX_JOE_','FX_HEADBOSS_',
    # bird family
    'FX_BIRD_','FX_BIRDIE_','FX_BIRDY_',
    # rat family (BRG1)
    'FX_RAT_','FX_RATTY_',
    # gum family
    'FX_GUM_','FX_GUMBALL_','FX_GUMHEAD_','FX_GUMS_',
    # skull family
    'FX_SKULL_','FX_SKULLBOT_','FX_SKULLBALL_','FX_SKULLY_',
    # klay family
    'FX_KLAY_','FX_KLAYMEN_','FX_KMAN_','FX_PLAYER_',
    # other characters
    'FX_FINN_','FX_PHART_','FX_JOPS_','FX_JOLOPS_','FX_GLID_','FX_BIT_',
    'FX_GHOST_','FX_SOUL_','FX_SHRINE_','FX_SHRINEY_',
    'FX_PUFF_','FX_NUN_','FX_PIG_','FX_GUARD_','FX_BUG_',
]

SUFS = ['','_1','_2','_3','_4','_5','_6','_7','_8','_9',
        '_01','_02','_03','_04','_05','_06','_07','_08','_09','_10',
        '_LOOP','_END','_START','_MAIN','_ALT','_BIG','_SM','_LG','_HI','_LO',
        '_LEFT','_RIGHT','_UP','_DN','_DOWN','_IN','_OUT','_ON','_OFF',
        '_A','_B','_C','_D','_E','_F','_G','_H']

hits = []
seen = set()
for pfx in PFXS:
    for v in VERBS:
        for suf in SUFS:
            s = pfx + v + suf
            if s in seen: continue
            seen.add(s)
            h = calcHash(s)
            if h in allids:
                row = allids[h]
                if row.get('status') == 'uncracked':
                    hits.append((h, s, row))
                    
print(f'Tested {len(seen):,} unique strings, sister hits: {len(hits)}')
print()

# Group by id
from collections import defaultdict
grouped = defaultdict(list)
for h, s, row in hits:
    grouped[h].append((s, row))

# Sort by single-candidate hits first, then cluster
print('=== Sister hits ===')
for h, items in sorted(grouped.items(), key=lambda x: (len(x[1]), x[0])):
    s, row = items[0]
    n = len(items)
    levels = row['levels'][:50]
    typ = row['type']
    print(f"  0x{h:08x}  popc={bin(h).count('1')} type={typ:12s} levels={levels} [{n} cands]")
    for s, _ in items[:6]:
        print(f'      -> {s}')
    if n > 6:
        print(f'      ... ({n-6} more)')
