"""Find the remaining 2 remap IDs: 0x246166fa (popc=15) and 0x70d006d8 (popc=12).

Hypothesis: Klaymen movement sounds with varied modifiers.
"""
import sys, os, csv
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

TARGETS = {0x246166fa: "popc=15 (long name)", 0x70d006d8: "popc=12 (medium name)"}

# Broader vocabulary
movements = [
    "FOOTSTEP","STEP","WALK","RUN","JUMP","LAND","FALL","STOP","STAND","CROUCH",
    "DUCK","SLIDE","SHUFFLE","CLIMB","CRAWL","ROLL","HOP","SKIP","SCAMPER",
    "TIPTOE","STOMP","TROT","JOG","MARCH","SPRINT","DASH","CHARGE",
    "BOUNCE","SPRING","LEAP","DIVE",
    "DOWN","UP","START","STOP","BEGIN","END",
    "PUSH","PULL","DRAG","KICK","PUNCH",
    "TURN","SPIN","TWIST","PIVOT",
    "GRAB","TAKE","DROP","HOLD","RELEASE","CARRY",
    "FOOTSTEP_QUIET","FOOTSTEP_LOUD","RUN_FAST","RUN_SLOW","STOP_SHORT",
    "WALK_HARD","WALK_SOFT","JUMP_LAND","JUMP_FALL","JUMP_HIGH",
    "FOOT","LEG","HAND","ARM",
    "FOOT_HIT","FOOT_LAND","HAND_GRAB",
    "DRAG_DOWN","FALL_DOWN","FALL_HARD","FALL_SOFT",
    "JUMP_HARD","JUMP_SOFT","LAND_HARD","LAND_SOFT","LAND_END",
    "STEP_LOUD","STEP_QUIET","STEP_HARD","STEP_SOFT",
]
mods = [
    "","_QUIET","_LOUD","_FAST","_SLOW","_HARD","_SOFT","_BIG","_SM","_LG",
    "_LO","_HI","_LOW","_HIGH","_NORM","_MED","_MID","_END","_START",
    "_LOOP","_FADE","_S","_M","_L","_XL","_XS","_NORMAL","_MAIN","_LOOPED",
    "_1","_2","_3","_4","_5","_6","_7","_8","_9","_01","_02","_03",
    "_DEEP","_HOLLOW","_FLAT","_SHARP","_DULL","_THIN","_ECHO",
    "_LEFT","_RIGHT","_DN","_UP","_FR","_BK",
]
prefixes = ["FX_KLAY","FX_KMEN","FX_PLAYER","FX_HERO","FX_FINN","FX_KLAYMEN",
            "FX_KMAN","FX_BOSS","FX","SFX_KLAY","FX_FRIEND","FX_CHAR"]

print("Searching for remaining remap IDs...")
total = 0
hits = {h: [] for h in TARGETS}
for pre in prefixes:
    for mv in movements:
        for mod in mods:
            for sep in ["_",""]:
                name = pre + sep + mv + mod
                total += 1
                h = calcHash(name)
                if h in TARGETS:
                    hits[h].append(name)
            # Also without prefix sep
            name = pre + mv + mod
            total += 1
            h = calcHash(name)
            if h in TARGETS:
                hits[h].append(name)

print(f"Tried {total} names")
for h, label in TARGETS.items():
    print(f"\n0x{h:08x} {label}: {len(hits[h])} hits")
    for n in sorted(set(hits[h]))[:15]:
        print(f"    {n!r}")
