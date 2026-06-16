#!/usr/bin/env python3
"""Try Klaymen-themed candidate names against known FINN sprite ids."""
import sys, os
sys.path.insert(0, os.path.dirname(__file__))

def calcHash(s: str) -> int:
    """Match the C calcHash: case-insensitive, ignore non-alnum,
    cumulative-shift XOR bit-toggle."""
    h = 0
    shift = 0
    for c in s.upper():
        if not (c.isalnum()):
            continue
        # Map digits 0..9 -> 0..9, letters A..Z -> 10..35
        if c.isdigit():
            v = ord(c) - ord('0')
        else:
            v = ord(c) - ord('A') + 10
        h ^= (v << shift)
        shift = (shift + 5) & 31
    return h & 0xFFFFFFFF

# Known FINN ids
TARGETS = {
    0x21842018: "InitPlayerEntity (FINN base player sprite)",
    0x040a2110: "FINN sprite (popc=6)",
    0x10b95810: "FINN family stem (proven, K=2)",
    0x10b85810: "FINN dir sprite (e.g. up)",
}

# Vocabulary set
CANDIDATES = [
    "KLAYMEN", "KLAYMAN", "KLAY", "KLAY_MAN", "KLAYM",
    "PLAYER", "PLAYR", "PLAY",
    "FINN", "FINNBASE", "FINNCORE", "FINNMAIN",
    "FINNGUY", "FINNBOY", "FINNHERO", "FINNKID",
    "MAIN", "HERO", "BASE", "CORE", "MAINCH",
    "BASEMAN", "MAINMAN",
    "PROTO", "PROTAG",
    "BUDDY", "DUDE",
    "BOSS",
    "KLN", "KLN_BASE", "KLN_MAIN",
    "FIN", "FINBASE", "FINMAIN",
    "FNN", "FNNBASE",
    "EGGBASE", "BABYHEAD", "EGGMAN", "EGGBOY",
    "BIRD", "FLY", "FLYER",
    "FALL", "FALLER",
    "JUMP", "JUMPER",
    "WALK", "WALKER",
    "RUN", "RUNNER",
    "PROP", "ACTOR", "ENTITY",
]

# Permutations with prefixes/suffixes
PREFIX = ["", "FINN", "FN", "KMEN", "KLN", "K_", "B_", "P_"]
SUFFIX = ["", "BASE", "MAIN", "SPR", "S", "0", "1", "2", "ID"]

def gen():
    seen = set()
    for c in CANDIDATES:
        seen.add(c)
    for p in PREFIX:
        for c in CANDIDATES:
            for s in SUFFIX:
                seen.add(p + c + s)
                seen.add(p + "_" + c + "_" + s)
    return seen

words = gen()
print(f"Trying {len(words)} candidates against {len(TARGETS)} targets...")
hits = 0
for w in words:
    h = calcHash(w)
    if h in TARGETS:
        print(f"!! HIT: {w!r} -> 0x{h:08x}  ({TARGETS[h]})")
        hits += 1
print(f"Done. {hits} hits.")
