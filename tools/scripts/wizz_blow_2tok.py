"""Enumerate FX_BLOW_<TOK1>_<TOK2> in full."""
import sys, os
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

WIZZ = {0x04041001: None, 0x08041001: "FX_BLOW_L_2",
        0x0c041001: None, 0x0e041001: "FX_BLOW_GALE_SM",
        0x08089081: None, 0x40815801: None,
        0x48017101: None, 0xc0099011: None}

ABCNUM = list("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789")
PAIRS = [a+b for a in ABCNUM for b in ABCNUM] + ABCNUM  # 2-char and 1-char

# Patterns
hits = {}
total = 0
patterns = [
    "FX_BLOW_{a}_{b}",
    "FX_BLOW_{a}{b}",
    "FX_BLOW_{a}_{b}_{c}",
    "FX_GUST_{a}_{b}",
    "FX_GUST_{a}{b}",
    "FX_WIND_{a}_{b}",
    "FX_WIND_{a}{b}",
    "FX_GALE_{a}_{b}",
    "FX_GALE_{a}{b}",
    "FX_STORM_{a}_{b}",
    "FX_PUFF_{a}_{b}",
    "FX_WHOOSH_{a}_{b}",
    "FX_BREEZE_{a}_{b}",
]

for tpl in patterns:
    if "{c}" in tpl:
        # 3-token pattern - limit
        for a in ABCNUM:
            for b in ABCNUM:
                for c in ABCNUM:
                    name = tpl.format(a=a, b=b, c=c)
                    total += 1
                    h = calcHash(name)
                    if h in WIZZ:
                        hits.setdefault(h, []).append(name)
    else:
        for a in PAIRS:
            for b in PAIRS:
                name = tpl.format(a=a, b=b)
                total += 1
                h = calcHash(name)
                if h in WIZZ:
                    hits.setdefault(h, []).append(name)

print(f"Tried {total} names")
for h in sorted(WIZZ.keys()):
    label = WIZZ[h] or ""
    label_str = f"  ({label})" if label else ""
    if h in hits:
        print(f"0x{h:08x}{label_str}: {len(hits[h])} hits (first 15)")
        for n in hits[h][:15]:
            print(f"    {n!r}")
    else:
        print(f"0x{h:08x}{label_str}: NO MATCH")
