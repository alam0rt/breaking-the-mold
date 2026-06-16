"""Aggressively brute FX_KLAY_*, FX_SKULL_*, FX_FINN_*, FX_PHART_* against unknowns.
The verified pattern is FX_ACTOR_VERB[_NUM]. Try every reasonable verb."""
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

# Verified actors
ACTORS = ['KLAY','KLAYMEN','KMAN','SKULL','FINN','PHART','JOPS','JOLOPS','GLID',
          'BOSS','GUARD','BUG','BIRD','EGGS','PIG','CLAY','HEAD','MIKE','SHISH',
          'BIT','GUM','MONKEY','PLAYER','EVIL','SHRINEY','SHRINE','GUMBALL',
          'JOE','JOYBOY','EYEBALL','BALL','BABY']
# Many verbs (Skullmonkeys gameplay-specific)
VERBS = ['JUMP','LAND','LANDING','FALL','RUN','WALK','IDLE','STAND','BLINK',
         'DIE','DEATH','HIT','HURT','OUCH','PAIN',
         'GRAB','HOLD','PICKUP','PICK','THROW','TOSS','SHOOT','FIRE','LAUNCH',
         'EAT','BITE','CHOMP','CHEW','SPIT','SPITOUT',
         'LAUGH','SCREAM','CRY','YELL','GASP','GIGGLE','HOWL','GROAN','MOAN',
         'FART','BURP','SNEEZE','COUGH','SNORT','BREATH','BREATHE','HUFF',
         'BLOW','PUFF',
         'SWIM','CLIMB','DUCK','SLIDE','SIT','BEND','TURN','LOOK','POINT',
         'WAVE','DANCE','SPIN','BOUNCE','BOB','SHAKE','TWIST','SWAY',
         'SLEEP','WAKE','SNORE','REST',
         'FLY','GLIDE','SAIL','FLOAT','HOVER','DROP','LIFT','CARRY',
         'STOMP','KICK','PUNCH','SLASH','SWING','BLOCK','DODGE','DASH',
         'CHARGE','RECOIL','STAGGER','STUMBLE','DAZE','REVIVE','RESPAWN','HEAL',
         'POISON','BURN','FREEZE','SHOCK','GLOW','FADE','SHINE',
         'WIN','LOSE','SUCCESS','FAIL','VICTORY','DEFEAT',
         'BUMP','POP','PLOP','PLUNK','PLOOSH','SPLOOSH','SPLAT','SPLASH',
         'BANG','BOOM','SHOOM','WHOOSH','WHIZZ','THUD','THUMP','TINK','TONK',
         'OUT','IN','UP','DOWN','LEFT','RIGHT',
         'INTRO','OUTRO','START','END','FINISH','BEGIN',
         'OPEN','CLOSE','ENTER','EXIT','EXPLODE','BREAK',
         'WET','DRY','HOT','COLD','BIG','SMALL',
         'FAT','THIN','TALL','SHORT','BLACK','WHITE','RED','BLUE','GREEN',
         'STEP','FOOT','HEAD','BUTT','BELLY',
         'WET','SLIME','GOO','MUD','OOZE',
         'SAY','SHOUT','TALK','SING','HUM','WHISTLE','WHIMPER',
         'BUMP','RUB','SCRATCH',
         'INFLATE','DEFLATE','BLOAT','SHRINK','GROW',
         'YO','HEY','WHA','UH','HUH','OH']

# Also more obscure / sound-effect-related verbs
EXTRA_VERBS = ['SQUEAK','SQUEAL','SQUEEZE','SQUISH','SQUASH','SQUELCH',
               'BLEEP','BEEP','BUZZ','BLIP','PING','PONG','DING','DONG','BELL',
               'CHIRP','TWEET','HOOT','CROW','BARK','MEOW','MOO','BAA','OINK',
               'SLURP','GULP','BURP','MUNCH','SLAM','SLOSH',
               'CRACKLE','POP','SIZZLE','HISS','RUSTLE','RUMBLE','WHIRR',
               'HONK','BLAST','BLARE','TOOT','TOOTLE','TUBA','TROMBONE',
               'CHURN','GRIND','GRIT','CRUNCH','SNAP',
               'WHISH','WHIRL','WHIP','WHACK','WALLOP','BOP','BUMP',
               'TAP','TICK','TOCK','CLICK','CLUNK','CLANG']
VERBS = list(set(VERBS + EXTRA_VERBS))

# Modifiers
MODS = ['','1','2','3','4','5','6','7','8','9','01','02','03','04','05',
        'A','B','C','D','E','F',
        'BIG','SM','LG','HI','LO','TALL','SHORT',
        'LEFT','RIGHT','UP','DN','IN','OUT','ON','OFF',
        'LOOP','START','END','MAIN','ALT','EXTRA','BONUS']

PREFIXES = ['FX','SFX','SND','MUS','BGM','MUSIC',
            'SPR','SPRITE','IMG','PIC','ANI','ANIM','ANIMA','ANIMATION',
            'PRT','PARTICLE','PART','EFFECT','EFX','VFX',
            'OBJ','PROP','ENT','ENTITY','MDL','MODEL',
            'PAL','PALETTE','TILE','TILES','BG','BACKGROUND',
            'FONT','TEXT','TXT','HUD','UI','MENU','ICON','CURSOR',
            'STATE','POSE','FRAME','FR','SEQ','SEQUENCE','SCENE']

hits = {}
seen = set()
count = 0
for pfx in PREFIXES:
    for actor in ACTORS:
        for verb in VERBS:
            for sep in ['_','']:
                base = sep.join([pfx, actor, verb])
                for m in MODS:
                    s = base + (sep+m if m else '')
                    if s in seen: continue
                    seen.add(s)
                    h = calcHash(s)
                    if h in unknowns:
                        hits.setdefault(h, []).append(s)
                    count += 1
print(f'Tested {count:,} unique strings, hits: {len(hits)}', flush=True)

# Filter: print high-confidence and per-id summaries
import json
print()
print('=== Hit summary ===')
for hh, names in sorted(hits.items(), key=lambda x: (len(x[1]), x[0])):
    row = unknowns[hh]
    print(f"  0x{hh:08x}  popc={bin(hh).count('1')} type={row['type']:8s} levels={row['levels'][:40]} [{len(names)} cands]")
    # print a few representative names sorted by short
    names = sorted(set(names), key=lambda s: (len(s), s))
    for n in names[:8]:
        print(f'      -> {n}')
    if len(names) > 8:
        print(f'      ... ({len(names)-8} more)')

# Save full results
with open('/tmp/sm_actor_brute_hits.csv','w') as f:
    f.write('id_hex,name,type,levels,n_candidates\n')
    for hh, names in sorted(hits.items()):
        row = unknowns[hh]
        for n in sorted(set(names)):
            f.write(f'0x{hh:08x},{n},{row["type"]},{row["levels"][:60]},{len(set(names))}\n')
