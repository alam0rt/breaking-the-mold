"""Sister-validate FX_PUFF_FALL_3 against other FINN audio ids."""
import sys, os, csv
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

# All uncracked + cracked
uncracked = {}
cracked = {}
with open("docs/analysis/asset-identification/cracked_names.csv") as f:
    r = csv.DictReader(f)
    for row in r:
        try:
            hid = int(row["id_hex"], 16)
        except: continue
        if row["status"] == "uncracked":
            uncracked[hid] = (row["type"], row["levels"], int(row["floor"]))
        else:
            cracked[hid] = row.get("name","")

def stat(h):
    if h in cracked: return f"CRACKED ({cracked[h]})"
    if h in uncracked:
        t,l,fl = uncracked[h]
        return f"UNCRACKED ({t},[{l}],popc={fl})"
    return "(unknown)"

# Verify variants for PUFF_FALL with sister _N or _01-style
print("=== FX_PUFF_FALL_N siblings ===")
for n in "123456789":
    name = f"FX_PUFF_FALL_{n}"
    h = calcHash(name)
    print(f"  {name:30s} -> 0x{h:08x}  {stat(h)}")
for nn in ["01","02","03","04","05","06"]:
    name = f"FX_PUFF_FALL_{nn}"
    h = calcHash(name)
    print(f"  {name:30s} -> 0x{h:08x}  {stat(h)}")

# Maybe other PUFF actions
print("\n=== FX_PUFF_<ACTION> siblings ===")
for act in ["FALL","JUMP","LAND","WALK","DIE","HURT","SWIM","DIVE","BREATH","SPIN","BOUNCE","ROLL","UP","DOWN"]:
    for suf in ["","_3","_1","_01","_02","_03"]:
        name = f"FX_PUFF_{act}{suf}"
        h = calcHash(name)
        s = stat(h)
        if "UNCRACKED" in s or "CRACKED" in s:
            print(f"  {name:30s} -> 0x{h:08x}  {s}")

# Check other FINN ids
finn_ids = [0x02a48626, 0x40e0824c, 0x6ae1a244, 0xd0b04444]
print("\n=== Try common FINN actions for the OTHER FINN audio ids ===")
ents = ["PUFF","FINN","KLAY","BABY","FLYER","SWIM","SWIMMER","WATER","FLY","SPLASH","BUBBLE"]
acts = ["FALL","JUMP","LAND","WALK","DIE","HURT","SWIM","DIVE","BREATH","SPIN","BOUNCE","ROLL",
        "UP","DOWN","KICK","FLOAT","SINK","STAND","CRY","YELL","SCREAM","SPLASH","SQUISH","GIB"]
sufs = ["","_1","_2","_3","_4","_5","_01","_02","_03","_04","_SM","_MD","_LG","_S","_M","_L"]

for fid in finn_ids:
    found = []
    for e in ents:
        for a in acts:
            for s in sufs:
                name = f"FX_{e}_{a}{s}"
                if calcHash(name) == fid:
                    found.append(name)
    print(f"\n  0x{fid:08x} hits: {len(found)}")
    for n in found[:8]:
        print(f"    {n!r}")
