"""0x42906465/0xc2906565 sister pair has +0x80000100 XOR.
Note: This XOR is identical to FX_PICKUP_1970 -> FX_PICKUP_1970_03 (0x408a6461 -> 0x428a6465).
So the 'full' suffix transforms similarly. Try 'FULL', '_03', '_HIGH', etc.
"""
import sys, os
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

T1 = 0x42906465  # hamster regular
T2 = 0xc2906565  # hamster full

# Check the diff signature
print(f"  Hamster XOR: 0x{T1 ^ T2:08x}")
print(f"  1970    XOR: 0x{0x408a6461 ^ 0x428a6465:08x}")
print(f"  These differ - different suffix patterns")
print()

# Hamster regular: 0x42906465 = popc 12
# Try names like FX_PICKUP_HAMSTERSHIELD, etc.
NAMES = []
for r in ["HAMSTERSHIELD","HAMSTER_SHIELD","HAMSTER","HAMSTER_ICON","HAMSTERICON",
          "HSHIELD","H_SHIELD","HAMSHIELD","HAMS_SHIELD","HAMS","HAM_SHIELD",
          "HAMSTER_ORB","HAMSTERORB","FUR","FURBALL","FUR_BALL","FUZZBALL","FUZZ_BALL"]:
    for pre in ["FX_PICKUP_","FX_HAM_","FX_HAMSTER_","FX_PICKUP_HAMSTER_",""]:
        for suf in ["","_HIT","_GET","_OK","_NEW","_FX","_PWR","_PWRUP","_CHIME",
                    "_DING","_BEEP","_BLB","_PICKUP","_GAIN","_GRAB","_TAKE",
                    "_FIRST","_ONE","_ONE_OF_3","_OFTHREE","_OF_THREE","_FRIEND","_BUDDY",
                    "_PROTECTOR","_GUARD","_GUARDIAN","_AURA","_HALO","_ORBIT"]:
            n = pre + r + suf
            h = calcHash(n)
            if h == T1:
                NAMES.append((n, T1))
            if h == T2:
                NAMES.append((n, T2))

print(f"Found {len(NAMES)} hits:")
for n, t in NAMES:
    print(f"  {n:40s} -> 0x{t:08x}")
