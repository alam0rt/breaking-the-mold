"""Try FX_BLOW_L_N for various N, find which WIZZ ids match."""
import sys, os
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

WIZZ = [0x04041001, 0x08041001, 0x0c041001, 0x0e041001,
        0x08089081, 0x40815801, 0x48017101, 0xc0099011]
WIZZ_SET = set(WIZZ)

# Try FX_BLOW_L_N and FX_BLOW_S_N and FX_BLOW_M_N etc
templates = [
    "FX_BLOW_L_{n}",
    "FX_BLOW_M_{n}",
    "FX_BLOW_S_{n}",
    "FX_BLOW_SM_{n}",
    "FX_BLOW_MD_{n}",
    "FX_BLOW_LG_{n}",
    "FX_BLOW_BIG_{n}",
    "FX_BLOW_HUGE_{n}",
    "FX_BLOW_GALE_SM_{n}",
    "FX_BLOW_GALE_MD_{n}",
    "FX_BLOW_GALE_LG_{n}",
    "FX_BLOW_GALE_S_{n}",
    "FX_BLOW_GALE_M_{n}",
    "FX_BLOW_GALE_L_{n}",
    "FX_BLOW_GALE_{n}",
    "FX_BLOW_GUST_{n}",
    "FX_BLOW_WIND_{n}",
    "FX_BLOW_STORM_{n}",
    "FX_BLOW_{n}",
    "FX_WIND_{n}",
    "FX_GUST_{n}",
    "FX_GALE_{n}",
    "FX_STORM_{n}",
]
nums = list("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789")

hits = {}
total = 0
for tpl in templates:
    for n in nums:
        name = tpl.format(n=n)
        total += 1
        h = calcHash(name)
        if h in WIZZ_SET:
            hits.setdefault(h, []).append(name)

print(f"Tried {total} names")
for h in sorted(WIZZ_SET):
    if h in hits:
        print(f"0x{h:08x} ({len(hits[h])} hits): {hits[h]}")
    else:
        print(f"0x{h:08x}: NO MATCH")
