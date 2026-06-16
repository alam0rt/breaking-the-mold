"""Focused FX_BLOW / FX_WIND attack on WIZZ family."""
import sys, os
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

WIZZ_TARGETS = [0x04041001, 0x08041001, 0x0c041001, 0x0e041001,
                0x08089081, 0x40815801, 0x48017101, 0xc0099011]
WIZZ_SET = set(WIZZ_TARGETS)
LABELS = {0x0e041001: "FX_BLOW_GALE_SM"}

# Core blow roots
ROOTS = [
    "BLOW","WIND","GALE","GUST","BREEZE","STORM","WHOOSH","PUFF","WHIRL",
    "AIR","CYCLONE","TORNADO","BLAST","HUFF","HOWL","SWOOSH","WHISH","WHIRR",
    "BLOW_GALE","BLOW_WIND","WIND_BLOW","WIND_GUST","WIND_GALE","WIND_STORM",
    "GUST_BLOW","GALE_BLOW","STORM_BLOW","STORM_WIND",
    "WIZ_BLOW","WIZ_WIND","WIZ_GALE","WIZ_GUST","WIZ_SPELL","WIZ_CAST",
    "MAGIC_BLOW","MAGIC_WIND","MAGIC_BLAST","MAGIC_PUFF",
    "SPELL_CAST","SPELL_BLAST","SPELL_PUFF","SPELL_WIND",
    "SHIELD","BARRIER","FORCE","SPARK","PULSE",
    "DISAPPEAR","APPEAR","TELEPORT","WARP","FADE",
    "BLOW_SM","BLOW_MD","BLOW_LG","BLOW_HUGE",
    "BLOW_S","BLOW_M","BLOW_L","BLOW_XL",
    "BLOW1","BLOW2","BLOW3","BLOW_1","BLOW_2","BLOW_3","BLOW_4","BLOW_5",
    "WIND1","WIND2","WIND3","WIND_1","WIND_2","WIND_3",
    "GALE_SM","GALE_MD","GALE_LG","GALE_HUGE",
    "GALE_S","GALE_M","GALE_L","GALE_XL",
]

# Prefixes
PREFIXES = ["FX_", "FX", "SFX_", "SND_", ""]

# Suffixes (lots)
SUFFIXES = [
    "","_1","_2","_3","_4","_5","_6","_7","_8","_9","_0",
    "_SM","_MED","_MD","_M","_LG","_L","_LRG","_BIG","_HUGE","_S",
    "_XS","_XL","_TINY","_LARGE","_SMALL",
    "_A","_B","_C","_D","_E","_F",
    "_LO","_HI","_LOW","_HIGH","_MID",
    "_LOOP","_FADE","_END","_STOP","_START",
    "_QUIET","_LOUD","_SOFT","_HARD",
    "_NEAR","_FAR","_UP","_DOWN","_IN","_OUT","_ON","_OFF",
    "_SHORT","_LONG",
    "_SM_1","_SM_2","_MD_1","_MD_2","_LG_1","_LG_2",
    "_BLOW","_WIND","_GUST","_SPELL","_CAST","_MAGIC",
    "_HIT","_MISS","_PASS","_DIE","_DEATH","_SPAWN","_KILL",
    "_LEFT","_RIGHT","_FRONT","_BACK","_OPEN","_CLOSE",
    "_BOSS","_WIZ","_WIZZ","_WIZARD",
    "_KMEN","_KLAY","_KLAYMEN",
    "_GUST_LG","_GUST_SM","_GUST_MD",
]

hits = {}
total = 0
for pre in PREFIXES:
    for root in ROOTS:
        for suf in SUFFIXES:
            name = pre + root + suf
            total += 1
            h = calcHash(name)
            if h in WIZZ_SET:
                if h not in hits:
                    hits[h] = []
                if len(hits[h]) < 30:
                    hits[h].append(name)

print(f"Tried {total} names")
for h in sorted(WIZZ_SET):
    label = f"  ({LABELS[h]})" if h in LABELS else ""
    if h in hits:
        print(f"0x{h:08x}{label}: {len(hits[h])} hits")
        for n in hits[h][:20]:
            print(f"    {n!r}")
    else:
        print(f"0x{h:08x}{label}: NO MATCH")
