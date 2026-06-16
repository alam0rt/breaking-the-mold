"""Probe the 5 remappable footstep IDs as a sound-effect family."""
import sys, os, csv
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

REMAP_IDS = [0x248e52, 0x246166fa, 0x44d4c8d8, 0x5860c640, 0x70d006d8]
TARGET_SET = set(REMAP_IDS)

# These are 5 sounds that get remapped based on g_SoundRemapMode.
# Hypothesis: they're per-surface/material footsteps for Klaymen.
roots = [
    "FX_KLAY_STEP",
    "FX_KLAY_WALK",
    "FX_KLAY_FOOTSTEP",
    "FX_KMEN_STEP",
    "FX_KMEN_FOOTSTEP",
    "FX_PLAYER_STEP",
    "FX_PLAYER_FOOTSTEP",
    "FX_STEP",
    "FX_FOOTSTEP",
    "FX_FOOT",
    "FX_HERO_STEP",
    "FX_HERO_FOOTSTEP",
    "FX_FINN_STEP",
    "FX_FINN_FOOTSTEP",
    "FX_KLAYMEN_STEP",
    "FX_KLAYMEN_FOOTSTEP",
]

# Per-surface tokens
surfaces = [
    "WOOD","METAL","DIRT","GRASS","WATER","STONE","ROCK","SAND","SNOW","ICE",
    "MUD","LAVA","CLAY","TIN","RUST","TILE","CARPET","CONCRETE","CEMENT",
    "GLASS","PLASTIC","BONE","FLESH","STEEL","COPPER","BRASS","GOLD","SILVER",
    "GRAVEL","COBBLE","BRICK","DUST","SLUDGE","SLIME","FOAM",
    "HARD","SOFT","LOUD","QUIET","NORMAL","DEEP","HOLLOW","SOLID",
    "1","2","3","4","5",
    "A","B","C","D","E",
    "L","R","LEFT","RIGHT",
    "01","02","03","04","05",
    "LO","HI","LOW","HIGH","MID","MED",
    "ECHO","DULL","FLAT","SHARP",
]
mods = [
    "", "_LEFT", "_RIGHT", "_L", "_R",
    "_LOOP", "_LOOPED", "_1", "_2", "_3",
    "_01", "_02", "_03", "_04", "_05",
]

print(f"Checking {len(roots)} roots × {len(surfaces)} surfaces × {len(mods)} mods")
hits = {h: [] for h in REMAP_IDS}
total = 0
for root in roots:
    for surf in surfaces:
        for mod in mods:
            for sep in ["_", ""]:
                name = root + sep + surf + mod
                total += 1
                h = calcHash(name)
                if h in TARGET_SET:
                    hits[h].append(name)
            # Also bare root + mod
            name = root + mod
            total += 1
            h = calcHash(name)
            if h in TARGET_SET:
                hits[h].append(name)

# Bare root alone
for root in roots:
    h = calcHash(root)
    if h in TARGET_SET:
        hits[h].append(root)

print(f"\nTried {total} names")
for h in REMAP_IDS:
    print(f"\n0x{h:08x} popc={bin(h).count('1')}:")
    if hits[h]:
        for n in sorted(set(hits[h]))[:20]:
            print(f"    {n!r}")
    else:
        print("    (no hits)")
