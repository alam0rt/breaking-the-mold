"""Brute around FX_BLOW pattern: try FX_BLOW_<SIZE>_<NUM>."""
import sys, os
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

WIZZ_TARGETS = {0x04041001, 0x08041001, 0x0c041001, 0x0e041001,
                0x08089081, 0x40815801, 0x48017101, 0xc0099011}
LABELS = {0x0e041001: "FX_BLOW_GALE_SM (verified)",
          0x08041001: "FX_BLOW_L_2 (candidate)"}

# Size/modifier tokens (broad)
SIZE_TOKENS = ["SM","MD","LG","S","M","L","XS","XL","TINY","HUGE","BIG","SMALL","LARGE","MED",
               "LO","HI","LOW","HIGH","MID","QUIET","LOUD","SOFT","HARD","NEAR","FAR","CLOSE",
               "SHORT","LONG","DEEP","THIN","GENTLE","HARSH",
               "GALE","WIND","GUST","BLAST","HUFF","HOWL","WHOOSH","BREEZE","STORM",
               "PUFF","WHISH","WHIRR","WHIRL","SWOOSH","CYCLONE",
               "SPELL","MAGIC","CAST","HEX","WIZ","WIZZ",
               "BOSS","EVIL","DARK","LIGHT",
               "A","B","C","D","E","F","G","H","I","J","K","N","O","P","Q","R","T","U","V","W","X","Y","Z",
               "0","1","2","3","4","5","6","7","8","9"]
NUM_TOKENS = ["","_0","_1","_2","_3","_4","_5","_6","_7","_8","_9",
              "_01","_02","_03","_04","_05",
              "_A","_B","_C","_D","_E","_F",
              "_LOOP","_FADE","_END","_HIT","_DIE"]

PREFIXES = ["FX_", "SFX_", "FX", ""]

# FX_BLOW_<TOK>_<NUM>
hits = {}
total = 0
for pre in PREFIXES:
    for t in SIZE_TOKENS:
        for n in NUM_TOKENS:
            name = pre + "BLOW_" + t + n
            total += 1
            h = calcHash(name)
            if h in WIZZ_TARGETS:
                hits.setdefault(h, []).append(name)
            # Also try FX_<TOK>_BLOW
            name2 = pre + t + "_BLOW" + n
            total += 1
            h2 = calcHash(name2)
            if h2 in WIZZ_TARGETS:
                hits.setdefault(h2, []).append(name2)
            # Try FX_BLOW<TOK><NUM>
            name3 = pre + "BLOW" + t + n
            total += 1
            h3 = calcHash(name3)
            if h3 in WIZZ_TARGETS:
                hits.setdefault(h3, []).append(name3)

# FX_WIND_<TOK>_<NUM>
for pre in PREFIXES:
    for t in SIZE_TOKENS:
        for n in NUM_TOKENS:
            for root in ["WIND_","GUST_","GALE_","STORM_","WHOOSH_","BREEZE_"]:
                name = pre + root + t + n
                total += 1
                h = calcHash(name)
                if h in WIZZ_TARGETS:
                    hits.setdefault(h, []).append(name)

print(f"Tried {total} names")
for h in sorted(WIZZ_TARGETS):
    label = LABELS.get(h, "")
    label_str = f"  ({label})" if label else ""
    if h in hits:
        print(f"0x{h:08x}{label_str}: {len(hits[h])} hits")
        for n in hits[h][:30]:
            print(f"    {n!r}")
    else:
        print(f"0x{h:08x}{label_str}: NO MATCH")
