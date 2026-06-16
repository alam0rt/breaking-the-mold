"""Validate KLAY family expansion + remap-table hypothesis."""
import sys, os, csv
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

uncracked = {}
cracked = {}
with open("docs/analysis/asset-identification/cracked_names.csv") as f:
    r = csv.DictReader(f)
    for row in r:
        try: hid = int(row["id_hex"], 16)
        except: continue
        if row["status"] == "uncracked":
            uncracked[hid] = (row["type"], row["levels"], int(row["floor"]))
        else:
            cracked[hid] = (row.get("name",""), row.get("type",""), row.get("levels",""))

def stat(h):
    if h in cracked: return f"CRACKED ({cracked[h][0]})"
    if h in uncracked:
        t,l,fl = uncracked[h]
        return f"UNCRACKED ({t},[{l}],popc={fl})"
    return ""

REMAP_IDS = [0x248e52, 0x246166fa, 0x44d4c8d8, 0x5860c640, 0x70d006d8]

print("=== REMAP family hypothesis: FX_KLAY_<MOVEMENT>_<MODIFIER> ===")
print("Test all combinations to find what hits the 5 remap IDs.\n")

CHARS_movement = ["FOOTSTEP","RUN","WALK","STEP","JOG","TROT","MARCH","JUMP","LAND","JOG","BOUNCE","STOMP"]
MODS = ["","_QUIET","_LOUD","_FAST","_SLOW","_HARD","_SOFT",
        "_LO","_HI","_LOW","_HIGH","_NORM","_NORMAL","_MED","_MID",
        "_LEFT","_RIGHT","_L","_R","_DN","_UP",
        "_1","_2","_3","_4","_5","_01","_02","_03","_04","_05",
        "_WOOD","_METAL","_STONE","_GRASS","_WATER","_DIRT","_SAND","_ICE","_SNOW",
        "_CLAY","_TILE","_CONCRETE","_GLASS","_PLASTIC","_RUBBER",
        "_DEEP","_HOLLOW","_THIN","_FAT","_FLAT","_SHARP","_DULL",
        "_ECHO","_LOOP",
        "_BIG","_SM","_LG","_HUGE","_TINY",
        "_S","_M","_L","_XL","_XS",]

# Check what hits each remap ID
print("Hits for each remap ID:")
for hid in REMAP_IDS:
    matches = []
    for mv in CHARS_movement:
        for mod in MODS:
            for pre in ["FX_KLAY","FX_KMEN","FX_PLAYER","FX_HERO","FX_FINN","FX_KLAYMEN"]:
                name = pre + "_" + mv + mod
                if calcHash(name) == hid:
                    matches.append(name)
    print(f"\n  0x{hid:08x} popc={bin(hid).count('1')}: {len(matches)} hits")
    for m in sorted(set(matches))[:8]:
        print(f"    {m!r}")

# Validate FX_KLAY_LAND_SOFT (CLOU) by sister
print("\n\n=== FX_KLAY_LAND family ===")
for suf in ["_SOFT","_HARD","_BIG","_SM","_LOUD","_QUIET","_LO","_HI","_LOW","_HIGH",
            "_01","_02","_03","_04","_05","_1","_2","_3","_4","_5",
            "_LEFT","_RIGHT","_DOWN","_UP","_END","_LOOP","_FADE","",
            "_DEEP","_FLAT","_FAST","_SLOW","_NORM"]:
    n = "FX_KLAY_LAND" + suf
    h = calcHash(n)
    s = stat(h)
    if s:
        print(f"  {n:30s} -> 0x{h:08x}  {s}")

# Already verified: FX_KLAY_LAND = 0x...
print("\n  (FX_KLAY_LAND verified hash for comparison)")
print(f"  FX_KLAY_LAND -> 0x{calcHash('FX_KLAY_LAND'):08x}")

# Sister-check FX_KLAY_DIE_FALL  
print("\n\n=== FX_KLAY_DIE family ===")
for suf in ["_FALL","_HIT","_BIG","_SM","_LOUD","_QUIET","_END","_LONG","_SHORT",
            "_01","_02","_03","_1","_2","_3","_BURN","_FRY","_SQUISH","_GIB",
            "_LEFT","_RIGHT","_DOWN","_UP","",]:
    n = "FX_KLAY_DIE" + suf
    h = calcHash(n)
    s = stat(h)
    if s:
        print(f"  {n:30s} -> 0x{h:08x}  {s}")
