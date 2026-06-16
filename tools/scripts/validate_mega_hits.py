"""Validate top candidates from mega attack."""
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

def show_family(stem, sufs):
    print(f"\n=== {stem} family ===")
    for suf in [""] + sufs:
        name = stem + suf if not suf or suf.startswith("_") else stem + "_" + suf
        h = calcHash(name)
        s = stat(h)
        if s:
            print(f"  {name:35s} -> 0x{h:08x}  {s}")

# Validate FX_SKULL_GRUNT - HEAD level, popc=15
print("=" * 60)
print("FX_SKULL_GRUNT family (HEAD audio)")
print("=" * 60)
show_family("FX_SKULL_GRUNT",
    ["_01","_02","_03","_04","_05","_06","_07","_08","_09",
     "_1","_2","_3","_4","_5","_6","_7","_8","_9",
     "_UP","_DN","_DOWN","_BIG","_SM","_LG","_LONG","_SHORT","_LOOP","_END","_START"])

# Validate FX_SKULL_RIP/ZAP_12 collision  
print("\n" + "=" * 60)
print("FX_SKULL_RIP / FX_SKULL_ZAP family")
print("=" * 60)
show_family("FX_SKULL_RIP", ["_01","_02","_03","_04","_05","_1","_2","_3","_UP","_DN","_END","_LOOP","_START"])
show_family("FX_SKULL_ZAP", ["_01","_02","_03","_04","_05","_06","_07","_08","_09","_10","_11","_12","_13","_1","_2","_3"])

# Validate FX_KLAY_FOOTSTEP family
print("\n" + "=" * 60)
print("FX_KLAY_FOOTSTEP family")
print("=" * 60)
show_family("FX_KLAY_FOOTSTEP",
    ["_QUIET","_LOUD","_LO","_HI","_SOFT","_HARD",
     "_GRASS","_WOOD","_METAL","_STONE","_WATER",
     "_LEFT","_RIGHT","_L","_R","_DN","_UP",
     "_01","_02","_03"])

# Sister check JINGLE_LAND
print("\n" + "=" * 60)
print("SFX_JINGLE_LAND family")
print("=" * 60)
show_family("SFX_JINGLE_LAND",
    ["_1","_2","_3","_4","_5","_6","_7","_8","_9",
     "_01","_02","_03",
     "_A","_B","_C","_D","_S","_M","_L","_SM","_LG"])
# Also try FX_
show_family("FX_JINGLE_LAND",
    ["_1","_2","_3","_4","_5","_6","_7","_8","_9","_S","_L"])

# Sister check EGGY_BLAST (WIZZ)
print("\n" + "=" * 60)
print("SFX_EGGY_BLAST / FX_EGGY_BLAST family (WIZZ sprite)")
print("=" * 60)
show_family("SFX_EGGY_BLAST", ["_SM","_MD","_LG","_S","_M","_L","_1","_2","_3"])
show_family("FX_EGGY_BLAST", ["_SM","_MD","_LG","_S","_M","_L","_1","_2","_3"])

# Sister check MA_CLAP
print("\n" + "=" * 60)
print("FX_MA_CLAP family (KLOG sprite)")
print("=" * 60)
show_family("FX_MA_CLAP",
    ["_1","_2","_3","_4","_5","_6","_7","_8","_9","_10","_11","_12",
     "_01","_02","_03"])
