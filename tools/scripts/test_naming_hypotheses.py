#!/usr/bin/env python3
"""Test unusual / non-English naming hypotheses.

Hypotheses:
1. Sequential numeric codes (S0001, M1701)
2. Module-level naming (MODULE1700SND01)
3. Reversed words (RUSROC, MENU reversed)
4. Single number/letter strings (0123456789)
5. Internal codename (development numbers)
"""
import sys, csv
sys.path.insert(0, "tools/scripts")
from compound_hash_attack import asset_id

ROOT = "/home/sam/projects/btm"

ids = set()
with open(f"{ROOT}/docs/reference/asset-ids-master.csv") as f:
    next(f)
    for line in f:
        ids.add(int(line.split(',')[0], 16))

LEVELS = ["BOIL","BRG1","CAVE","CLOU","CRYS","CSTL","EGGS","EVIL","FINN","FOOD","GLEN","GLID","HEAD","KLOG","MEGA","MENU","MOSS","PHRO","RUNN","SCIE","SEVN","SNOW","SOAR","TMPL","WEED","WIZZ"]

def test(name):
    h = asset_id(name)
    return h in ids, h

hits = []

# H1: Numeric digit string
for s in ["0123456789", "0123", "0987654321", "9876543210"]:
    ok, h = test(s)
    if ok: hits.append((s, h))

# H2: Module names from ScummVM-like pattern
for module in range(100, 6000, 100):
    for prefix in ["MODULE", "M", "MOD"]:
        for suffix in ["SOUND", "SND", "MUSIC", "MUS", "SFX", "ANIM", "SPR"]:
            for idx in range(0, 30):
                name = f"{prefix}{module}{suffix}{idx:02d}"
                ok, h = test(name)
                if ok: hits.append((name, h))
                name = f"{prefix}{module}{suffix}{idx}"
                ok, h = test(name)
                if ok: hits.append((name, h))

print(f"after H2: {len(hits)} hits")

# H3: Sequential codes S0001-S9999
for prefix in "SMFAGBVHDET":
    for n in range(0, 10000, 1):
        name = f"{prefix}{n:04d}"
        ok, h = test(name)
        if ok: hits.append((name, h))

print(f"after H3: {len(hits)} hits")

# H4: Reversed common words
words = ["MENU", "PAUSE", "TITLE", "CURSOR", "OPTIONS", "PASSWORD",
         "MUSIC", "SOUND", "SPRITE", "ANIM", "DEMO", "INTRO", "OUTRO",
         "ENDING", "GAME", "OVER", "PRESS", "START"]
for w in words:
    rev = w[::-1]
    ok, h = test(rev)
    if ok: hits.append((rev, h))

# H5: Level + numeric
for lvl in LEVELS:
    for n in range(0, 1000):
        for sep in ["", "_"]:
            name = f"{lvl}{sep}{n}"
            ok, h = test(name)
            if ok: hits.append((name, h))
            name = f"{lvl}{sep}{n:03d}"
            ok, h = test(name)
            if ok: hits.append((name, h))
            name = f"{n}{sep}{lvl}"
            ok, h = test(name)
            if ok: hits.append((name, h))

print(f"after H5: {len(hits)} hits")

# H6: Common short technical names
for n in ["FNT", "FNT0", "FNT1", "FNT00", "FNT01",
          "PAL", "PAL0", "PAL1",
          "TIM", "TIM0", "TIM1", "TIM00",
          "VAB", "VAB0", "SEQ", "SEQ0", "SEQ1",
          "BG", "BG0", "BG1", "BG00", "BG01",
          "IMG", "IMG0", "IMG1",
          "SPR", "SPR0", "SPR1",
          "PIC", "PIC0", "PIC1",
          "TXT", "TXT0", "TXT1",
          "VRAM", "PSXVRAM",
          "NULL", "BLANK", "EMPTY", "DEFAULT", "DUMMY",
          "TEST", "DEBUG", "TEMP", "TMP",
          "HEAD0", "HEAD1", "TAIL", "BACK", "FRONT",
          "RAM", "ROM", "DISC", "DISK"]:
    ok, h = test(n)
    if ok: hits.append((n, h))

# H7: Programmer-style: 3-letter + index
for prefix in ["FNT","SPR","SEQ","ANM","BG","PIC","IMG","PAL","TEX","TIM","SND","MUS","MID","VAB","XA","STR","SFX","FX","MAP","TBL","DAT","BIN"]:
    for idx in range(0, 1000):
        for fmt in [f"{idx}", f"{idx:02d}", f"{idx:03d}", f"_{idx}", f"_{idx:02d}", f"_{idx:03d}"]:
            name = f"{prefix}{fmt}"
            ok, h = test(name)
            if ok: hits.append((name, h))

print(f"after H7: {len(hits)} hits")

# H8: SOUND_NN / MUSIC_NN
for prefix in ["SOUND", "MUSIC", "SOUND_", "MUSIC_", "MOVIE", "MOVIE_",
               "MUS_", "SND_", "SFX_", "VOX_", "VOICE_", "VOICE",
               "BGM", "BGM_", "BGS", "BGS_"]:
    for idx in range(0, 200):
        for fmt in [f"{idx}", f"{idx:02d}", f"{idx:03d}"]:
            name = f"{prefix}{fmt}"
            ok, h = test(name)
            if ok: hits.append((name, h))

print(f"after H8: {len(hits)} hits")

# H9: Klaymen actions (canonical animation names in many games)
KLAYMEN = ["KLAY", "KLAYMEN", "KMAN", "KM"]
ACTIONS = ["IDLE","WALK","RUN","JUMP","FALL","LAND","HURT","DIE","DEATH",
           "SHOOT","FIRE","ATTACK","SLAM","BOUNCE","TURN","DUCK","CROUCH",
           "SLIDE","ROLL","SPIN","CLIMB","SWIM","FLOAT","HOVER","FLY",
           "CELEBRATE","WIN","VICTORY","LOSE","TAUNT","WAVE","WAIT"]
for k in KLAYMEN:
    for a in ACTIONS:
        for sep in ["", "_"]:
            for tail in ["", "0", "1", "2", "L", "R", "_L", "_R", "ANIM", "_ANIM"]:
                name = f"{k}{sep}{a}{tail}"
                ok, h = test(name)
                if ok: hits.append((name, h))

print(f"after H9: {len(hits)} hits")

# H10: Try sample format: short + number with leading 0
# Many PSX games use names like "L00", "L01", or "T01_KLAY"
for letter in "ABCDEFGHIJKLMNOPQRSTUVWXYZ":
    for n in range(0, 100):
        for fmt in [f"{letter}{n}", f"{letter}{n:02d}", f"{letter}{n:03d}",
                    f"{letter}_{n}", f"{letter}_{n:02d}",
                    f"{letter}{letter}{n}", f"{letter}{letter}{n:02d}"]:
            ok, h = test(fmt)
            if ok: hits.append((fmt, h))

print(f"\n=== {len(hits)} HITS ===")
for name, h in hits:
    print(f"  0x{h:08x}  {name}")
