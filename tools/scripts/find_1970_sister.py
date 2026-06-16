"""Now that FX_PICKUP_1970 = 0x408a6461, find sister with diff 0x02000004."""
import sys, os, csv
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

# Target: 0x428a6465 = 0x408a6461 XOR 0x02000004
# That diff is 2 bits at positions 2 and 25.

# Try FX_PICKUP_1970_<X> permutations
sufs = []
ABCNUM = list("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789")
for c in ABCNUM:
    sufs.append("_"+c)
for c1 in ABCNUM:
    for c2 in ABCNUM:
        sufs.append("_"+c1+c2)
sufs.extend(["","_FULL","_LAST","_END","_DONE","_3","_FINAL",
             "_SHIELD","_3OF3","_3_OF_3","_LOOP","_X"])

# Also try prefix replacements
stems = [
    "FX_PICKUP_1970","FX_1970_PICKUP","FX_GET_1970","FX_1970_GET","FX_GRAB_1970",
    "FX_GIVEN_1970","FX_1970_GIVE","FX_PICK_1970","FX_PICK_UP_1970",
    "FX_HAMSTER_1970","FX_1970_HAMSTER",
    "FX_BONUS_1970","FX_1970_BONUS",
    "FX_SHIELD_1970","FX_1970_SHIELD",
    "FX_LAST_1970","FX_1970_LAST",
]

target1 = 0x408a6461  # FX_PICKUP_1970 - regular
target2 = 0x428a6465  # 3rd / shield

print(f"Verify FX_PICKUP_1970 -> 0x{calcHash('FX_PICKUP_1970'):08x}  (target1=0x{target1:08x})")
print()

# Find any name hashing to target2
print("Searching for sister:")
hits2 = []
for stem in stems:
    for s in sufs:
        n = stem + s
        if calcHash(n) == target2:
            hits2.append(n)
        if calcHash(n) == target1:
            if n != "FX_PICKUP_1970":
                pass  # other matches for sanity

print(f"\nHits for 0x{target2:08x}: {len(hits2)}")
for n in sorted(set(hits2))[:30]:
    print(f"  {n!r}")
