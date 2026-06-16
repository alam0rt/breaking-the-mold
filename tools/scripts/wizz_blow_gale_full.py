"""Confirm FX_BLOW_GALE_SM and search the family thoroughly."""
import sys, os
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

TARGETS = {
    0x04041001: None,
    0x08041001: None,
    0x0c041001: None,  # stem
    0x0e041001: "FX_BLOW_GALE_SM verified",
    0x08089081: None,
    0x40815801: None,
    0x48017101: None,
    0xc0099011: None,
}

# Prefixes
prefixes = ["FX_", "FX", "SFX_", "SND_", "SOUND_", "AUDIO_", "WAV_", ""]
# Roots
roots = ["BLOW_GALE", "BLOWGALE", "BLOW", "GALE",
         "WIND_BLOW", "WIND_GALE", "WIND_GUST", "WIND",
         "GUST", "GUST_BLOW",
         "BREEZE", "BREEZE_BLOW",
         "STORM", "STORM_WIND",
         "AIR", "AIR_BLOW", "AIR_GUST",
         "PUFF", "WHIFF", "WHOOSH", "BLOWN", "BLOWING",
         "WIZARD", "WIZ", "WIZZ", "WIZ_BLOW", "WIZ_WIND",
         "SPELL", "MAGIC", "CAST",
         "CYCLONE", "TORNADO", "FAN",
         "BLOW1", "BLOW2", "BLOW3",
         "WIND1", "WIND2", "WIND3",
         "FX_BLOW_GALE", "FX_WIND_BLOW", "FX_WIND_GALE"]
# Suffixes (size, ordinal, modifier)
suffixes = ["_SM","_MED","_MD","_M","_LG","_L","_LRG","_LRGE","_BIG","_HUGE","_S","",
            "_1","_2","_3","_4","_5","_6","_7","_8","_9",
            "_A","_B","_C","_D",
            "_LO","_HI","_LOW","_HIGH","_MID",
            "_SHORT","_LONG","_LOOP",
            "_LOOPED","_FADE","_END","_STOP","_START",
            "_QUIET","_LOUD","_SOFT","_HARD",
            "_NEAR","_FAR","_UP","_DOWN","_IN","_OUT",
            "_FRONT","_BACK","_SIDE",
            "_X","_XL","_XS","_XXL","_XXS",
            "_HIT","_MISS","_PASS","_DIE","_DEATH","_SPAWN","_HIT1","_HIT2"]

hits = []
total = 0
seen = set()
for pre in prefixes:
    for root in roots:
        for suf in suffixes:
            name = pre + root + suf
            if name in seen:
                continue
            seen.add(name)
            h = calcHash(name)
            total += 1
            if h in TARGETS:
                hits.append((h, name))

print(f"Tried {total} names")
for h, n in sorted(hits):
    label = TARGETS.get(h)
    print(f"  HIT 0x{h:08x}  {n!r}" + (f"  ({label})" if label else ""))

# Also confirm
print()
print(f"calcHash('FX_BLOW_GALE_SM') = 0x{calcHash('FX_BLOW_GALE_SM'):08x}  (target 0x0e041001)")
