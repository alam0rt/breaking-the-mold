"""Check FX_BLOW_<SZ>_<NUM> for all SZ x NUM, see what hits each WIZZ id."""
import sys, os
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

WIZZ = {0x04041001: None, 0x08041001: "FX_BLOW_L_2",
        0x0c041001: None, 0x0e041001: "FX_BLOW_GALE_SM",
        0x08089081: None, 0x40815801: None,
        0x48017101: None, 0xc0099011: None}

# Sizes alone (1-2 chars common in this game): S, M, L, SM, MD, LG
# Numbers: 1-9
SIZES = ["S", "M", "L", "SM", "MD", "LG", "XS", "XL"]
NUMS = ["1","2","3","4","5","6","7","8","9","0"]

hits = {}
total = 0
for sz in SIZES:
    for n in NUMS:
        for tpl in [f"FX_BLOW_{sz}_{n}",
                    f"FX_BLOW_GALE_{sz}_{n}",
                    f"FX_BLOW_WIND_{sz}_{n}",
                    f"FX_BLOW_GUST_{sz}_{n}",
                    f"FX_WIND_{sz}_{n}",
                    f"FX_GUST_{sz}_{n}",
                    f"FX_GALE_{sz}_{n}",
                    f"FX_STORM_{sz}_{n}",
                    f"FX_PUFF_{sz}_{n}",
                    f"FX_WHOOSH_{sz}_{n}",
                    f"FX_BREEZE_{sz}_{n}",]:
            total += 1
            h = calcHash(tpl)
            if h in WIZZ:
                hits.setdefault(h, []).append(tpl)

for h in sorted(WIZZ.keys()):
    label = WIZZ[h] or ""
    label_str = f"  ({label})" if label else ""
    if h in hits:
        print(f"0x{h:08x}{label_str}: {hits[h]}")
    else:
        print(f"0x{h:08x}{label_str}: NO MATCH")

# Also try _<NUM> alone
print()
print("=== FX_BLOW_<SZ>_<NUM> with extended num set ===")
for sz in SIZES + ["XS", "XL", "TINY", "HUGE", "BIG"]:
    for n in NUMS + ["A","B","C","D","E","F"]:
        for tpl in [f"FX_BLOW_{sz}_{n}", f"FX_BLOW_GALE_{sz}_{n}"]:
            h = calcHash(tpl)
            if h in WIZZ:
                print(f"  HIT 0x{h:08x}  {tpl!r}")
