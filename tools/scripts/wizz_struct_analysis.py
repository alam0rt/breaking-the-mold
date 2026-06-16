"""Analyze WIZZ family structure by computing per-suffix increment."""
import sys, os
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

# Prefix hashes
prefixes = ["FX_BLOW_L_", "FX_BLOW_", "FX_BLOW_GALE_", "FX_BLOW_M_", "FX_BLOW_S_",
            "FX_BLOW_SM_", "FX_BLOW_MD_", "FX_BLOW_LG_",
            "FX_WIND_", "FX_GALE_", "FX_GUST_", "FX_STORM_",
            "FX_BLOW", "FX_WIND", "FX_GALE",
            "FX_BLOW_L", "FX_BLOW_M", "FX_BLOW_S",
            "FX_BLOW_LARGE_", "FX_BLOW_SMALL_", "FX_BLOW_MED_",
            "FX_BLOW_BIG_", "FX_BLOW_HUGE_",
            "FX_BLOW_GALE", "FX_BLOW_WIND", "FX_BLOW_GUST",
            "FX_BLOW_GALE_S", "FX_BLOW_GALE_M", "FX_BLOW_GALE_L",
            "FX_BLOW_GUST_", "FX_BLOW_GUST_S", "FX_BLOW_GUST_M", "FX_BLOW_GUST_L",
            "FX_BLOW_GUST_SM",
            ]

for p in prefixes:
    print(f"  '{p:30s}' -> 0x{calcHash(p):08x}")

# WIZZ family
print()
print("WIZZ family:")
WIZZ = [0x04041001, 0x08041001, 0x0c041001, 0x0e041001,
        0x08089081, 0x40815801, 0x48017101, 0xc0099011]
for h in WIZZ:
    print(f"  0x{h:08x}  popc={bin(h).count('1')}")

# What's between FX_BLOW_GALE_SM (0x0e041001) and 0x04041001?
print()
diff = 0x0e041001 ^ 0x04041001
print(f"FX_BLOW_GALE_SM XOR 0x04041001 = 0x{diff:08x}  (bits: {[i for i in range(32) if diff & (1<<i)]})")

# Try suffix replacements
for suf in ["SM","MD","LG","XS","XL","BIG","HUGE","TINY","S","M","L","XS","XL",
            "LARGE","SMALL","MED","HUGE","BIG","TINY",
            "1","2","3","4","5","6","7","8","9","0",
            "A","B","C","D","E","F","G","H",
            "LO","HI","LOW","HIGH","MID","DEEP","HOLLOW","CLEAR",
            "QUIET","LOUD","SOFT","HARD","GENTLE","FIERCE",
            "FX","SM","MD"]:
    name = "FX_BLOW_GALE_" + suf
    h = calcHash(name)
    if h in {0x04041001, 0x08041001, 0x0c041001, 0x08089081, 0x40815801, 0x48017101, 0xc0099011}:
        print(f"  MATCH: '{name}' -> 0x{h:08x}")
