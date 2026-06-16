"""Decode the WIZZ FX_BLOW_GALE family."""
import sys, os
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

# Verified
print(f"FX_BLOW_GALE_SM            = 0x{calcHash('FX_BLOW_GALE_SM'):08x}")
print(f"FX_BLOW_GALE_MD            = 0x{calcHash('FX_BLOW_GALE_MD'):08x}")
print(f"FX_BLOW_GALE_LG            = 0x{calcHash('FX_BLOW_GALE_LG'):08x}")
print(f"FX_BLOW_GALE_LRG           = 0x{calcHash('FX_BLOW_GALE_LRG'):08x}")
print(f"FX_BLOW_GALE_BIG           = 0x{calcHash('FX_BLOW_GALE_BIG'):08x}")
print(f"FX_BLOW_GALE_S             = 0x{calcHash('FX_BLOW_GALE_S'):08x}")
print(f"FX_BLOW_GALE_M             = 0x{calcHash('FX_BLOW_GALE_M'):08x}")
print(f"FX_BLOW_GALE_L             = 0x{calcHash('FX_BLOW_GALE_L'):08x}")
print(f"FX_BLOW_GALE               = 0x{calcHash('FX_BLOW_GALE'):08x}")

print()
print("WIZZ family ids:")
for hid in [0x04041001, 0x08041001, 0x0c041001, 0x0e041001, 0x08089081,
            0x40815801, 0x48017101, 0xc0099011]:
    print(f"  0x{hid:08x}")

# Try other potential siblings
print()
print("Variants:")
for suffix in ["_SM","_MD","_LG","_S","_M","_L","_LRG","_BIG","_HUGE",
               "_1","_2","_3","_4","_5",
               "_A","_B","_C","_D",
               "_LOW","_HIGH","_HI","_LO",
               "_LOOP","_SHORT","_LONG",
               "_QUIET","_LOUD","_FAR","_NEAR","_UP","_DOWN",
               "", "_X","_XL"]:
    name = "FX_BLOW_GALE" + suffix
    h = calcHash(name)
    print(f"  {name:24s} -> 0x{h:08x}")
