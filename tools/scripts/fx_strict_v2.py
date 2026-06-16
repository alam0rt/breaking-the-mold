"""Strict FX_ enumeration matching the verified-name shape only.

Verified raw-audio names follow EXACTLY one of these shapes:
  FX_<ACTOR>_<VERB>           (FX_KLAY_JUMP, FX_KLAY_LAND, FX_SKULL_FLAP)
  FX_<ACTOR>_<VERB>_<DIGIT>   (FX_FINN_DIE_4)
  FX_<ACTOR>_<VERB>_<DD>      (FX_SKULL_FIRE_01, FX_SKULL_LAND_01)
  FX_<ACTOR>_<VERB>_<COMPOUND_VERB>  (FX_SKULL_FOOTSTEP_LEFT)
  FX_<ACTOR>_<VERB>_<COMPOUND_VERB>_<DIGIT>

Modifiers from the broad saturation that DO NOT match a single verified name:
  HAPPY, OILY, GREASY, READY, SAD, MAD, MEH, OK, GOOD, BAD, POOPY, BUMPY,
  WET, DRY, BIGFAT, BREATHIN, ALL letters except A-F, single-letter K/H/I/J,
  TALL, SHORT, EXTRA, BONUS, NORTH/SOUTH/EAST/WEST, LIT/DARK/DIM/BRIGHT, etc.

This script enforces only:
  - empty suffix
  - 1-2 digit suffix
  - exact compound: LEFT/RIGHT/UP/DN/IN/OUT/ON/OFF/LOOP/START/END (already part
    of verb category, not a separator)

The actor/verb dictionary is also restricted to only those that appear in
verified names + ScummVM Klaymen vocabulary + canonical Skullmonkeys characters.
"""
import csv
import os
import sys
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


# ---- Load targets ----
audio_unk = {}
with open('docs/analysis/asset-identification/cracked_names.csv') as f:
    for row in csv.DictReader(f):
        if row['id_hex'].startswith('0x') and row.get('status') == 'uncracked' and row['type'] == 'audio':
            audio_unk[int(row['id_hex'], 16)] = row

# ---- Conservative dictionaries ----
# Canonical actors (verified or strong cohort signal). No "PIGGY" etc. variants.
ACTORS = [
    # Verified
    'KLAY', 'KLAYMEN', 'KMAN',
    'SKULL', 'BIRD', 'RAT', 'GUM', 'BOSS_HEAD', 'BOSSHEAD',
    'FINN', 'PHART', 'JOPS', 'JOLOPS',
    # Common Skullmonkeys characters
    'GLID', 'GLIDER', 'BIT', 'BITS',
    'GUARD', 'NUN', 'MONK', 'PRIEST',
    'BUG', 'BUGS', 'BEE', 'BEETLE', 'WORM', 'SLUG', 'SPIDER',
    'PIG', 'PIGS', 'BACON',
    'BAT', 'MOTH', 'FROG',
    'BOMB', 'BARREL', 'CRATE', 'BOX', 'TNT', 'MINE',
    'SPIKE', 'SAW', 'BLADE', 'TRAP',
    'PROJ', 'PROJECTILE', 'MISSILE', 'BULLET',
    'CHASEY', 'CHASE', 'HUNTER',
    'PUFF', 'PUFFY', 'PUFFER',
    'MIKE', 'SHISH',
    'GHOST', 'SOUL', 'PHANTOM',
    'CLAY', 'CLAYBALL', 'BALL',
    'EYEBALL', 'GUMBALL', 'GUMHEAD', 'JOEHEAD', 'JOE',
    'MEGA', 'BIG_GUM', 'BIGGUM',
    'EVIL', 'BAD', 'JOLOPS', 'JOPSY',
    'KLOG', 'TIMBER', 'AXE',
    'SHRINEY', 'SHRINE',
    'BABY', 'BABY_FINN',
    'ROBOT', 'BOT', 'DROID', 'MECH', 'MACHINE',
    'WIZARD', 'WIZ', 'WIZZ', 'MAGE',
    # Actions-as-actors (e.g. FX_BLOW_GALE)
    'BLOW', 'WIND', 'GUST', 'GAS',
    'JUMP', 'LAND', 'FIRE', 'BLAST',
    # Generic
    'PLAYER', 'ENEMY', 'BOSS',
    'ITEM', 'PICKUP', 'COLLECT', 'PRIZE',
    'STAGE', 'ROOM', 'AREA', 'LEVEL', 'LVL',
    'MENU', 'UI', 'CURSOR', 'SELECT',
    'DOOR', 'GATE', 'PORTAL',
    'SHOT', 'PIERCE', 'STAB',
]

# Conservative verb set: only verbs that match obvious sound-effect actions.
VERBS = [
    # Movement
    'JUMP', 'LAND', 'FALL', 'RUN', 'WALK', 'IDLE', 'STAND', 'BLINK',
    'CLIMB', 'SWIM', 'DUCK', 'SLIDE', 'TURN', 'LOOK',
    'BOUNCE', 'SHAKE', 'ROLL', 'SPIN', 'STEP', 'FOOTSTEP',
    'STOMP', 'KICK', 'PUNCH', 'SLASH', 'SWING', 'DASH',
    'CHARGE', 'RECOIL', 'DODGE',
    # Damage
    'DIE', 'DEATH', 'HIT', 'HURT', 'OUCH',
    'BURN', 'FREEZE', 'SHOCK', 'SQUISH',
    'EXPLODE', 'BURST', 'BREAK', 'SHATTER',
    # Item
    'GRAB', 'HOLD', 'PICKUP', 'THROW', 'TOSS', 'SHOOT', 'FIRE', 'LAUNCH',
    'CARRY', 'LIFT', 'DROP',
    # Vocal
    'LAUGH', 'SCREAM', 'CRY', 'YELL', 'GASP', 'GIGGLE', 'HOWL', 'GROAN', 'MOAN',
    'FART', 'BURP', 'SNEEZE', 'COUGH', 'SNORT',
    'BREATH', 'BREATHE', 'BLOW', 'PUFF',
    'SAY', 'SHOUT', 'TALK', 'SING', 'WHISTLE',
    'ROAR', 'BELLOW', 'GROWL', 'SNARL',
    # Action sounds
    'EAT', 'BITE', 'CHOMP', 'CHEW', 'SPIT',
    'SLURP', 'GULP', 'MUNCH',
    # Mechanical
    'BUMP', 'POP', 'PLOP', 'PLUNK', 'SPLAT', 'SPLASH',
    'BANG', 'BOOM', 'BLAST', 'THUD', 'THUMP', 'SLAM',
    'WHOOSH', 'WHIZZ',
    'SQUEAK', 'BUZZ', 'BEEP', 'PING', 'DING', 'BELL',
    'CHIRP', 'TWEET', 'HOOT',
    'CRACKLE', 'SIZZLE', 'HISS', 'RUMBLE',
    'HONK', 'BLARE', 'TOOT', 'HORN',
    'CRUNCH', 'SNAP', 'TAP', 'TICK', 'CLICK', 'CLUNK',
    'FLAP', 'SOAR', 'DIVE', 'SWOOP',
    'GUST', 'GALE', 'STORM', 'THUNDER',
    'PIERCE', 'STAB',
    # Game state
    'INTRO', 'OUTRO', 'START', 'END', 'BEGIN',
    'OPEN', 'CLOSE', 'ENTER', 'EXIT',
    'WIN', 'LOSE',
    'CHECKPOINT', 'SAVE', 'LOAD', 'SELECT', 'CONFIRM', 'CANCEL', 'PAUSE',
    'TRANSFORM', 'MORPH',
    # Verified compound verbs
    'FOOTSTEP', 'PIERCE', 'FLAP',
    # Direction-as-verb (used as suffix in verified names)
    'LEFT', 'RIGHT', 'UP', 'DN', 'DOWN', 'IN', 'OUT', 'ON', 'OFF',
]

# Strict modifiers — what ACTUALLY appears in verified raw audio names:
#   01-99 (digits with leading zero), 1-9 (bare digit)
#   LOOP, MAIN, ALT (none verified yet, but plausible)
DIGITS = (['1','2','3','4','5','6','7','8','9']
          + [f'{i:02d}' for i in range(1, 21)])
# Compound suffixes - keep VERY restricted to verified shapes
COMPOUND_SUF = ['LEFT','RIGHT','UP','DN','DOWN','IN','OUT','ON','OFF','LOOP','MAIN','ALT','BIG','SM','LO','HI']

# ---- Brute ----
hits = defaultdict(list)
seen = set()


def try_(s):
    if s in seen:
        return
    seen.add(s)
    h = calcHash(s)
    if h in audio_unk:
        hits[h].append(s)


for actor in ACTORS:
    for verb in VERBS:
        # Bare: FX_ACTOR_VERB  (covers FX_KLAY_JUMP)
        try_(f'FX_{actor}_{verb}')
        # Numeric suffix: FX_ACTOR_VERB_NN
        for d in DIGITS:
            try_(f'FX_{actor}_{verb}_{d}')
        # Compound suffix: FX_ACTOR_VERB_LEFT etc.
        for sfx in COMPOUND_SUF:
            try_(f'FX_{actor}_{verb}_{sfx}')
            # Plus digit: FX_ACTOR_VERB_LEFT_1
            # NOTE: not a single verified raw audio name uses
            # FX_..._<COMPOUND>_<DIGIT>; skipping to suppress noise.


print(f'Tested {len(seen):,} unique strings, hits: {len(hits)} IDs', file=sys.stderr)


# ---- Group by alpha-only form ----
def alpha_only(s):
    return ''.join(c for c in s.upper() if c.isalnum())


sem = {}
for h, names in hits.items():
    by_form = defaultdict(list)
    for n in names:
        by_form[alpha_only(n)].append(n)
    sem[h] = by_form


single = []
multi = []
for h, forms in sem.items():
    if len(forms) == 1:
        form = next(iter(forms))
        best = sorted(forms[form], key=lambda s: (-s.count('_'), len(s)))[0]
        single.append((h, best, audio_unk[h]))
    else:
        cands = []
        for form, variants in forms.items():
            best = sorted(variants, key=lambda s: (-s.count('_'), len(s)))[0]
            cands.append(best)
        multi.append((h, cands, audio_unk[h]))


print()
print(f'--- {len(single)} SINGLE-SEMANTIC hits ---')
for h, name, row in sorted(single, key=lambda x: x[0]):
    levels = row['levels'][:60]
    popc = bin(h).count('1')
    print(f'  0x{h:08x}  popc={popc:2d}  levels={levels}')
    print(f'      -> {name}')

print()
print(f'--- {len(multi)} MULTI-SEMANTIC hits ---')
for h, names, row in sorted(multi, key=lambda x: (len(x[1]), x[0])):
    levels = row['levels'][:60]
    popc = bin(h).count('1')
    print(f'  0x{h:08x}  popc={popc:2d}  levels={levels} [{len(names)} cands]')
    for s in sorted(names, key=lambda x: (len(x), x))[:8]:
        print(f'      -> {s}')


# Save
with open('docs/analysis/asset-identification/fx_strict_hits.csv', 'w') as f:
    f.write('id_hex,name,popc,n_sem_cands,levels\n')
    for h, name, row in single:
        popc = bin(h).count('1')
        f.write(f'0x{h:08x},{name},{popc},1,{row["levels"][:60]}\n')
    for h, names, row in multi:
        for n in names:
            popc = bin(h).count('1')
            f.write(f'0x{h:08x},{n},{popc},{len(names)},{row["levels"][:60]}\n')

print(f'\nSaved to docs/analysis/asset-identification/fx_strict_hits.csv', file=sys.stderr)
