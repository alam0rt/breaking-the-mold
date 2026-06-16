"""Look at the 1970 icon / hamster shield context.
Two related ids:
  0x408a6461 - regular pickup sound
  0x428a6465 - 'hamster_count == 3' = third hamster (shield activated!)
"""
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
            cracked[hid] = row.get("name","")

def stat(h):
    if h in cracked: return f"CRACKED ({cracked[h]})"
    if h in uncracked:
        t,l,fl = uncracked[h]
        return f"UNCRACKED ({t},[{l}],popc={fl})"
    return ""

# Check sister: 0x408a6461 (regular hamster pickup) vs 0x428a6465 (3rd hamster, shield)
print("=== The 1970 hamster shield pair ===")
print(f"  0x408a6461 (regular hamster pickup): {stat(0x408a6461)}")
print(f"  0x428a6465 (3rd hamster / shield):   {stat(0x428a6465)}")
print()
print(f"  XOR diff: 0x{0x408a6461 ^ 0x428a6465:08x}")
print()

# Try names related to hamsters/1970/shield
candidates = [
    # 1970 icon
    "FX_1970", "FX_1970_GET", "FX_1970_PICKUP", "FX_1970_GRAB",
    "FX_ICON_1970", "FX_ICON_GET",
    "FX_GET_1970", "FX_PICKUP_1970",
    "FX_HAMSTER", "FX_HAMSTER_GET", "FX_HAMSTER_PICKUP", "FX_HAMSTER_GRAB",
    "FX_HAMSTER_1", "FX_HAMSTER_2", "FX_HAMSTER_3",
    "FX_HAMSTER_SHIELD", "FX_HAMSTER_FULL", "FX_HAMSTER_LAST", "FX_HAMSTER_FINAL",
    "FX_HAMSTER_3_OF_3", "FX_HAMSTER_END",
    "FX_SHIELD", "FX_SHIELD_GET", "FX_SHIELD_ON", "FX_SHIELD_ACTIVE",
    "FX_SHIELD_BUILD", "FX_SHIELD_FORM",
    # Generic pickup
    "FX_PICKUP", "FX_PICKUP_GET", "FX_PICKUP_END",
    "FX_GET", "FX_GET_END", "FX_GET_LAST", "FX_GET_FINAL",
    "FX_BONUS", "FX_BONUS_GET", "FX_BONUS_PICKUP",
    "FX_COLLECT", "FX_COLLECT_END",
    "FX_GET_BONUS", "FX_GET_HAMSTER",
    # Power up
    "FX_POWER", "FX_POWER_UP", "FX_POWERUP", "FX_PWRUP",
    "FX_LEVEL_UP", "FX_LIFE_UP", "FX_BOOST",
    "FX_GUARD", "FX_GUARD_GET", "FX_GUARD_FULL",
    "FX_KLAY_HAMSTER", "FX_KLAY_SHIELD", "FX_KLAY_GUARD",
    "FX_KLAY_BONUS", "FX_KLAY_PICKUP", "FX_KLAY_GET",
    "FX_KLAY_HAMSTER_3", "FX_KLAY_3HAMSTER", "FX_KLAY_FULL",
    # Specific 1970 names
    "FX_BONUS_1970","FX_LV_1970","FX_LEVEL_1970","FX_STAGE_1970",
    # The XOR diff suggests one char different — find candidate names that hash to BOTH
]

print("=== Candidate name checks for the pair ===")
for n in candidates:
    h = calcHash(n)
    s = stat(h)
    if s:
        print(f"  *** {n:35s} -> 0x{h:08x}  {s}")
    elif h == 0x408a6461 or h == 0x428a6465:
        print(f"  HIT! {n:35s} -> 0x{h:08x}  (raw match)")

# Brute: find pairs N1,N2 such that hash(N1)=0x408a6461 and hash(N2)=0x428a6465
# with N1,N2 differing by single character substitution
print()
print("=== Smart sister-pair check: try N + suffix pattern ===")
stems = [
    "FX_HAMSTER","FX_1970","FX_PICKUP","FX_GET","FX_BONUS","FX_KLAY",
    "FX_KLAY_HAMSTER","FX_KLAY_1970","FX_KLAY_BONUS","FX_KLAY_PICKUP",
    "FX_KLAY_GET",
    "FX_SHIELD","FX_SHIELD_BUILD","FX_KLAY_SHIELD",
    "FX_GUARD","FX_KLAY_GUARD",
    "FX_BONUS_GET","FX_BONUS_PICKUP","FX_BONUS_HAMSTER",
    "FX_BONUS_END","FX_GET_BONUS","FX_GET_1970",
    "FX_PICKUP_BONUS","FX_PICKUP_1970","FX_PICKUP_HAMSTER",
    "FX_BONUS_LEVEL","FX_BONUS_STAGE","FX_BONUS_AREA",
]
suffs = ["","_1","_2","_3","_4","_GET","_END","_FULL","_LAST","_FINAL","_DONE",
         "_LOOP","_PICKUP","_GRAB","_SM","_LG","_BIG","_END"]
for stem in stems:
    for s in suffs:
        n = stem + s
        h = calcHash(n)
        if h == 0x408a6461 or h == 0x428a6465:
            print(f"    {n:45s} -> 0x{h:08x}")
