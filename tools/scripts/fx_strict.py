"""Strict FX_ enumeration matching ONLY verified pattern shapes.

Verified pattern: FX_<ACTOR>_<VERB>[_<MOD>] where:
  ACTOR is one word (or BOSS_HEAD, BIG_HEAD, BIG_FAT, etc. — known compounds)
  VERB is exactly one word
  MOD is exactly one short token from a tight list (digits 1-9, 01-09, single letter, or short positional)

Rejects:
  - VERB+MOD combos (DUCK_DOWN, FOOT_F, GALE_20)
  - 2-digit numbers >= 10
  - Multi-word verbs (SHIPMASTER_DONG_TALL, MARTIAN_SPITTING_POOPY)
  - Adjective modifiers (HAPPY, BAD, OK, POOPY, BIGFAT, GREASY)
"""
import csv
import sys
import os
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


# Audio uncracked
audio_unk = {}
with open('docs/analysis/asset-identification/cracked_names.csv') as f:
    for row in csv.DictReader(f):
        if row['id_hex'].startswith('0x') and row.get('status') == 'uncracked' and row['type'] == 'audio':
            audio_unk[int(row['id_hex'], 16)] = row


# Compound actor names known to be real (from verified cracks + Neverhood ScummVM)
COMPOUND_ACTORS = ['BOSS_HEAD', 'BIG_HEAD', 'BIG_GUM', 'BIG_FAT', 'JOE_HEAD', 'HEAD_JOE',
                   'BABY_FINN', 'FINN_BABY', 'KLAY_BABY', 'BABY_KLAY']


# Single-word actors from verified cracks + level-specific characters from CLAUDE.md
ACTORS = sorted(set([
    # PLAYER / KLAYMEN family (verified: FX_KLAY_*)
    'KLAY', 'KLAYMEN', 'KMAN', 'PLAYER', 'KIDLY',
    # SKULL enemy family (verified: FX_SKULL_*)
    'SKULL', 'SKULLBOT', 'SKULLBALL', 'SKULLY',
    # FINN boss (verified: FX_FINN_DIE_4)
    'FINN', 'FINNY',
    # Boss head (verified: FX_BOSS_HEAD_*)
    'BOSS', 'HEAD',
    # Bird (verified: FX_BIRD_FLY_01, FX_BIRD_SQUISH_01)
    'BIRD', 'BIRDIE', 'BIRDY',
    # Rat (verified: FX_RAT_DASH_END)
    'RAT',
    # Gum (verified: FX_GUM_PIERCE_DN)
    'GUM', 'GUMBALL', 'GUMHEAD',
    # Other characters by level (from Skullmonkeys docs/lore)
    'PHART', 'JOPS', 'JOLOPS', 'GLID', 'CHASEY', 'PUFF',
    'GUARD', 'KNIGHT', 'NUN', 'MONK', 'PRIEST', 'WIZZ', 'WIZARD',
    'CLAYBALL', 'CLAY', 'BIT', 'SHRINEY',
    'BUG', 'PIG', 'WORM', 'BAT', 'SPIDER',
    'GHOST', 'SOUL', 'PHANTOM',
    'JOE', 'MIKE', 'SHISH', 'JOYBOY',
    # Level-specific
    'KLOG',  # KLOG level boss/character
    'GLEN', 'MEGA',
    # Generic terms used as actors in verified names
    'BABY',
] + COMPOUND_ACTORS))

# Compact, plausible verb list
VERBS = sorted(set([
    # Movement
    'JUMP', 'LAND', 'FALL', 'RUN', 'WALK', 'IDLE', 'STAND', 'BLINK',
    'CLIMB', 'SWIM', 'DUCK', 'SLIDE', 'TURN', 'SPIN', 'BOUNCE', 'BOB',
    'STEP', 'FOOTSTEP', 'STOMP', 'KICK', 'PUNCH', 'DASH', 'CHARGE',
    'GLIDE', 'FLY', 'HOVER', 'CRAWL',
    # Damage
    'DIE', 'DEATH', 'HIT', 'HURT', 'OUCH', 'PAIN',
    'BURN', 'FREEZE', 'SHOCK', 'SQUISH',
    'EXPLODE', 'BREAK', 'SHATTER',
    # Item
    'GRAB', 'HOLD', 'PICKUP', 'THROW', 'TOSS', 'SHOOT', 'FIRE', 'LAUNCH',
    'CARRY', 'LIFT', 'DROP', 'COLLECT',
    # Vocal
    'LAUGH', 'SCREAM', 'CRY', 'YELL', 'GASP', 'GIGGLE', 'HOWL', 'GROAN', 'MOAN',
    'FART', 'BURP', 'SNEEZE', 'COUGH', 'SNORT',
    'BREATH', 'HUFF', 'BLOW', 'PUFF', 'SIGH',
    'SAY', 'SHOUT', 'TALK', 'SING', 'HUM', 'WHISTLE', 'WHIMPER',
    'ROAR', 'BELLOW', 'GROWL', 'SNARL', 'CHANT',
    # Action
    'EAT', 'BITE', 'CHOMP', 'CHEW', 'SPIT', 'GULP', 'SLURP', 'MUNCH',
    # Mechanical / environmental
    'POP', 'BANG', 'BOOM', 'BLAST', 'THUD', 'THUMP', 'CLANG',
    'SLAM', 'SMACK', 'WHOOSH', 'WHIZZ',
    'SQUEAK', 'SQUEAL', 'BUZZ', 'BLIP', 'PING', 'DING',
    'CHIRP', 'TWEET', 'HOOT', 'BARK',
    'SLOSH', 'SIZZLE', 'HISS', 'RUMBLE', 'WHIRR',
    'HONK', 'BLARE', 'TOOT',
    'CRUNCH', 'SNAP', 'TAP', 'TICK', 'CLICK',
    'FLAP', 'FLUTTER', 'SOAR', 'DIVE',
    'PIERCE', 'STAB', 'SLASH', 'SWING',
    # Game state
    'INTRO', 'OUTRO', 'START', 'END', 'BEGIN', 'WIN', 'LOSE',
    'OPEN', 'CLOSE', 'ENTER', 'EXIT',
    'TRANSFORM', 'MORPH', 'APPEAR', 'VANISH',
    # State / vocal exclamations from verified/likely
    'WET', 'DRY', 'BUMP', 'PEEK', 'HIDE',
]))

# STRICT mod list - only single-token, short
MODS = ['1', '2', '3', '4', '5', '6', '7', '8', '9',
        '01', '02', '03', '04', '05', '06', '07', '08', '09',
        'A', 'B', 'C', 'D',
        'LOOP', 'END', 'START', 'MAIN',
        'LEFT', 'RIGHT', 'UP', 'DN', 'IN', 'OUT', 'ON', 'OFF',
        'BIG', 'SM', 'LG', 'HI', 'LO', 'TOP', 'BOT']


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


# Pattern: FX_<ACTOR>_<VERB>[_<MOD>] (with sep variants for compound actors)
for actor in ACTORS:
    for verb in VERBS:
        # Pattern with underscore separator everywhere (canonical)
        base = f'FX_{actor}_{verb}'
        try_(base)
        for mod in MODS:
            try_(f'{base}_{mod}')

print(f'Tested {len(seen):,} unique strings, hits: {len(hits)}', file=sys.stderr)


def alpha_only(s):
    return ''.join(c for c in s.upper() if c.isalnum())


# Group multi-cand by alpha-stripped form
print('\n=== Strict-pattern audio FX_ hits ===')
single = []
multi = []
for h in sorted(hits, key=lambda x: (len(set(map(alpha_only, hits[x]))), x)):
    by_form = defaultdict(list)
    for n in hits[h]:
        by_form[alpha_only(n)].append(n)
    n_sem = len(by_form)
    row = audio_unk[h]
    if n_sem == 1:
        # Pick canonical underscore form
        form = next(iter(by_form))
        best = sorted(by_form[form], key=lambda s: (-s.count('_'), len(s), s))[0]
        single.append((h, best, row))
    else:
        cands = []
        for form, variants in by_form.items():
            best = sorted(variants, key=lambda s: (-s.count('_'), len(s), s))[0]
            cands.append(best)
        multi.append((h, cands, row))


print(f'\n--- {len(single)} SEMANTICALLY-UNIQUE hits ---')
for h, name, row in single:
    levels = row['levels'][:60]
    popc = bin(h).count('1')
    print(f'  0x{h:08x}  popc={popc:2d}  levels={levels}')
    print(f'      -> {name}')


print(f'\n--- {len(multi)} MULTI-CANDIDATE hits (semantically distinct) ---')
for h, names, row in multi[:40]:
    levels = row['levels'][:60]
    popc = bin(h).count('1')
    print(f'  0x{h:08x}  popc={popc:2d}  levels={levels} [{len(names)} cands]')
    for s in sorted(names, key=lambda x: (len(x), x))[:8]:
        print(f'      -> {s}')
    if len(names) > 8:
        print(f'      ... ({len(names)-8} more)')


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
