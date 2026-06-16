#!/usr/bin/env python3
"""Test prototype-build movie filenames as asset name candidates.

The beta build had visible movie filenames: MVDWI, MVEA, MVEND, MVEVIL, MVEYE,
MVGAS, MVINTRO1, MVINTRO2, MVLOGO, MVRED, MVWIN, MVYAM, MVYNT.

These suggest a 2-3 letter prefix + descriptor naming convention. Let me
try variations of these and similar short codes against the asset table.
"""
import sys, csv
sys.path.insert(0, "tools/scripts")
from compound_hash_attack import asset_id

ROOT = "/home/sam/projects/btm"

ids_dict = {}
with open(f"{ROOT}/docs/reference/asset-ids-master.csv") as f:
    rdr = csv.DictReader(f)
    for r in rdr:
        ids_dict[int(r["id_hex"], 16)] = r
ids = set(ids_dict)

# Movie filenames from beta build
MOVIES = ["DWI","EA","END","EVIL","EYE","GAS","INTRO1","INTRO2","LOGO","RED","WIN","YAM","YNT"]

# Common prefixes/suffixes
PREFIXES = ["MV","MOVIE","MV_","MOVIE_","FMV","FMV_","MOV","MOV_","M","V"]
SUFFIXES = ["",".STR",".TIM",".SPR",".ANM",".ANIM",".SND",".VAB",".XA"]

# Movie + various stages  
print("=== Movie name variants ===")
hits = []
for m in MOVIES:
    for p in PREFIXES:
        for s in SUFFIXES:
            for variant in [p+m+s, p+"_"+m+s, m+s, m+p+s, m+"_"+p+s, m]:
                # Strip non-alnum
                h = asset_id(variant)
                if h in ids:
                    print(f"  HIT  {variant:30s} -> 0x{h:08x}  {ids_dict[h]['kinds']}")
                    hits.append((variant, h))

# Try more aggressive: NES-style codenames
SHORT_CODES = []
# 2-3 letter prefixes with 2-4 letter codes
for p in "ABCDEFGHIJKLMNOPQRSTUVWXYZ":
    for c in MOVIES:
        SHORT_CODES.append(p + c)
        SHORT_CODES.append(c + p)

# Music tracks (M1-M5)
MUSIC_PREFIXES = ["MUS","MUSIC","BGM","BGM_","MUSIC_","M","MUS_"]
print("\n=== Music name variants ===")
for n in range(1, 30):
    for p in MUSIC_PREFIXES:
        for fmt in [f"{p}{n}", f"{p}{n:02d}", f"{p}_{n}", f"{p}_{n:02d}",
                    f"MUSIC{n}", f"BGM{n}", f"M{n}", f"M{n:02d}"]:
            h = asset_id(fmt)
            if h in ids:
                print(f"  HIT  {fmt:30s} -> 0x{h:08x}  {ids_dict[h]['kinds']}")
                hits.append((fmt, h))

# Level + stage (e.g., FINN1, FINN2, MEGAEND)
LEVELS = "BOIL BRG1 CAVE CLOU CRYS CSTL EGGS EVIL FINN FOOD GLEN GLID HEAD KLOG MEGA MENU MOSS PHRO RUNN SCIE SEVN SNOW SOAR TMPL WEED WIZZ".split()
print("\n=== Level + extras ===")
for lvl in LEVELS:
    for ext in ["END","START","BOSS","INTRO","OUTRO","TRANS","BG","SKY","CLOUD","GROUND","ICE","FIRE","WATER","CAVE","ROOM"]:
        for fmt in [lvl+ext, lvl+"_"+ext, ext+lvl, ext+"_"+lvl]:
            h = asset_id(fmt)
            if h in ids:
                print(f"  HIT  {fmt:30s} -> 0x{h:08x}  {ids_dict[h]['kinds']}")
                hits.append((fmt, h))

# Common asset codenames in old PSX games
PSX_ASSET_PATTERNS = [
    # Two-letter + number (common in old games)
    *[f"{p}{i}" for p in "FN AN SQ SP PT IM MS SE EF FX FB SB MD".split() for i in range(0, 100)],
    *[f"{p}{i:02d}" for p in "FN AN SQ SP PT IM MS SE EF FX FB SB MD".split() for i in range(0, 100)],
    # Three-letter + number
    *[f"{p}{i}" for p in "ANM SPR PIC FNT TIM PAL VAB XA SEQ MID SMP SND SFX VOX TXT MAP".split() for i in range(0, 200)],
    *[f"{p}{i:02d}" for p in "ANM SPR PIC FNT TIM PAL VAB XA SEQ MID SMP SND SFX VOX TXT MAP".split() for i in range(0, 200)],
    *[f"{p}{i:03d}" for p in "ANM SPR PIC FNT TIM PAL VAB XA SEQ MID SMP SND SFX VOX TXT MAP".split() for i in range(0, 200)],
]
print(f"\n=== PSX-style codenames ({len(PSX_ASSET_PATTERNS):,} candidates) ===")
for c in PSX_ASSET_PATTERNS:
    h = asset_id(c)
    if h in ids:
        print(f"  HIT  {c:30s} -> 0x{h:08x}  {ids_dict[h]['kinds']}")
        hits.append((c, h))

# Cinematic-style names
CINEMA_NAMES = ["LOGO","INTRO","OUTRO","CREDITS","ENDING","TITLE","DEMO","START","END","NAMCO","KONAMI","ELECTRONIC","ARTS","EA","NEVERHOOD","SKULLMONKEYS","PRESS","BLINK","FLASH"]
print("\n=== Cinematic names ===")
for n in CINEMA_NAMES:
    for p in ["","ANIM_","SCREEN_","BG_","TITLE_","PIC_","LOGO_","SHOT_","FRAME_"]:
        for s in ["","_ANIM","_SCREEN","_BG","_PIC","_LOGO","_SHOT","_FRAME"]:
            v = p+n+s
            h = asset_id(v)
            if h in ids:
                print(f"  HIT  {v:30s} -> 0x{h:08x}  {ids_dict[h]['kinds']}")
                hits.append((v, h))

print(f"\n=== TOTAL: {len(hits)} hits ===")
seen = set()
for v, h in hits:
    key = (v, h)
    if key in seen: continue
    seen.add(key)
    print(f"  0x{h:08x}  {v}")

# Save
with open(f"{ROOT}/docs/analysis/asset-identification/movie_name_hits.csv", "w", newline="") as f:
    w = csv.writer(f)
    w.writerow(["candidate","id_hex","kinds","floor"])
    seen = set()
    for v, h in hits:
        if (v, h) in seen: continue
        seen.add((v, h))
        w.writerow([v, f"0x{h:08x}", ids_dict[h]['kinds'], ids_dict[h]['popcount']])
