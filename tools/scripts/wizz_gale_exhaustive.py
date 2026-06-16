"""Rigorous validation of FX_BLOW_GALE_SM by checking
whether the FX_BLOW_GALE prefix has more sisters in WIZZ uncracked."""
import sys, os, csv
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

# Load uncracked WIZZ ids
wizz_uncracked = set()
all_uncracked = set()
with open("docs/analysis/asset-identification/cracked_names.csv") as f:
    r = csv.DictReader(f)
    for row in r:
        if row["status"] == "uncracked":
            try:
                hid = int(row["id_hex"], 16)
                all_uncracked.add(hid)
                if "WIZZ" in row["levels"]:
                    wizz_uncracked.add(hid)
            except Exception:
                pass

# Check ALL combinations of FX_BLOW_GALE_<X> where X is 1-3 chars from alphanumeric
print("=== FX_BLOW_GALE_<X> 1-3 char suffix exhaustive ===")
CHARS = list("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789")
hits_wizz = []
hits_other = []
for n_chars in range(1, 4):
    def gen(n):
        if n == 0:
            yield ""
            return
        for c in CHARS:
            for rest in gen(n - 1):
                yield c + rest
    for s in gen(n_chars):
        name = f"FX_BLOW_GALE_{s}"
        h = calcHash(name)
        if h in wizz_uncracked:
            hits_wizz.append((name, h))
        elif h in all_uncracked:
            hits_other.append((name, h))

print(f"WIZZ uncracked hits: {len(hits_wizz)}")
for name, h in hits_wizz:
    print(f"  {name:25s} -> 0x{h:08x}")
print(f"Other uncracked hits: {len(hits_other)}")
for name, h in hits_other[:30]:
    print(f"  {name:25s} -> 0x{h:08x}")
