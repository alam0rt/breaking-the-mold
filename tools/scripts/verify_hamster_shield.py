"""Verify HAMSTER SHIELD names parallel to 1970-icon family.
- 0x42906465 = normal hamster shield pickup
- 0xc2906565 = 3rd hamster shield pickup
"""
import sys, os
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

T1 = 0x42906465  # normal
T2 = 0xc2906565  # 3rd
print(f"Target 0x42906465 (normal hamster pickup):")
for n in ["FX_PICKUP_HAMSTER","FX_HAMSTER","FX_HAMSTER_PICKUP","FX_PICKUP_FURRY",
          "FX_PICKUP_HAM","FX_HAM","FX_HAMSTER_GET","FX_GET_HAMSTER",
          "FX_PICKUP_HAMSTER_SHIELD","FX_HAMSTER_SHIELD","FX_HAMSTERS",
          "FX_PICKUP_FUR","FX_PICKUP_LIVE",
          "FX_FURBALL","FX_FUR","FX_BUDDY","FX_PET",
          "FX_HALO_GET","FX_PICKUP_HALO","FX_PICKUP_SHIELD_OK",
          "FX_HAM_GRAB","FX_HAM_GET","FX_HAM_PICKUP",
          "FX_KLAY_PICKUP","FX_PICKUP_KLAY","FX_PICKUP_NORMAL",
          "FX_FURRY","FX_FUZZY","FX_HAMSTER_NORM",
          ]:
    h = calcHash(n)
    if h == T1:
        print(f"  {n:35s} -> 0x{h:08x} <-- TARGET")

print(f"\nTarget 0xc2906565 (3rd hamster pickup - 'full' jingle):")
for n in ["FX_PICKUP_HAMSTER_03","FX_HAMSTER_03","FX_PICKUP_HAMSTER_3",
          "FX_PICKUP_HAMSTER_FULL","FX_HAMSTER_FULL","FX_HAMSTER_MAX",
          "FX_HAMSTER_3RD","FX_PICKUP_HAMSTER_3RD","FX_HAM_03","FX_HAM_3",
          "FX_HAMSTER_SHIELD_FULL","FX_HAMSTER_SHIELD_3",
          "FX_HAM_FULL","FX_HAM_MAX","FX_HAM_LAST","FX_HAM_LAST_03",
          "FX_FULL_HAMSTER","FX_HAMSTER_DONE","FX_HAMSTER_END",
          "FX_3RD_HAMSTER","FX_LAST_HAMSTER","FX_FINAL_HAMSTER",
          "FX_HAMSTER_TOP","FX_HAMSTER_PEAK","FX_HAMSTER_HIGH",
          "FX_HAMSTER_LEVEL","FX_HAMSTER_NEW","FX_HAMSTER_GAIN",
          ]:
    h = calcHash(n)
    if h == T2:
        print(f"  {n:35s} -> 0x{h:08x} <-- TARGET")
