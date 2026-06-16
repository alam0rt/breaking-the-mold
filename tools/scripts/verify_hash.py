"""Verify Python calcHash against known cracks."""
import sys, os
sys.path.insert(0, os.path.dirname(__file__))
from check_klaymen_names import calcHash

# FX_MENU_SELECT was the WRAP namespace, not raw. So let me load
# the cracked names CSV and test raw entries.
known_raw = [
    # These are FUNCTION NAMES not raw asset names; checking raw cracks instead
    ("FX_KLAY_DUCK_DOWN", 0x4428c941),
    ("FX_SKULL_EAT", 0x7000c248),
    ("FX_SKULL_UP", 0x30004240),
    ("FX_BLOW_GALE_SM", None),  # was a known WIZZ one, check
    ("KLAY", None),  # placeholder
    ("BABYHEAD", None),
    ("FINN", None),
    ("KLAYMEN", None),
]

for name, expected in known_raw:
    h = calcHash(name)
    if expected is None:
        print(f"  {name!r}: 0x{h:08x}")
    else:
        ok = "OK" if h == expected else "FAIL"
        print(f"{ok} {name!r}: got 0x{h:08x} expected 0x{expected:08x}")
