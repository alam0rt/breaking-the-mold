"""Sister-validate the FX_<ENTITY>_<ACTION>_<NN> hits from convention attack."""
import sys, os, csv
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

uncracked = {}
cracked = {}
with open("docs/analysis/asset-identification/cracked_names.csv") as f:
    r = csv.DictReader(f)
    for row in r:
        try:
            hid = int(row["id_hex"], 16)
        except Exception:
            continue
        if row["status"] == "uncracked":
            uncracked[hid] = (row["type"], row["levels"], int(row["floor"]))
        elif row["status"] in ("verified", "candidate", "high_conf"):
            cracked[hid] = (row["name"], row["type"], row["levels"])

def status_of(h):
    if h in cracked:
        n, t, l = cracked[h]
        return f"CRACKED ({n})"
    if h in uncracked:
        t, l, fl = uncracked[h]
        return f"UNCRACKED ({t}, [{l}], popc={fl})"
    return "(not in asset set)"

candidates = [
    ("FX_PUFF_FALL", "_1 _2 _3 _4 _5 _6 _01 _02 _03 _04"),
    ("FX_EGGSHELL_ZOOM", "_SM _MD _LG _S _M _L _1 _2 _3 _01 _02 _03"),
    ("FX_SKULL_SPRING", "_01 _02 _03 _04 _05 _1 _2 _3 _4 _SM _LG _UP _DOWN"),
    ("FX_SKULL_ZAP", "_01 _02 _03 _04 _05 _06 _07 _08 _09 _10 _11 _12 _13"),
    ("FX_SKULL_RIP", "_01 _02 _03 _1 _2 _3 _UP _DN"),
    ("FX_KLAY_FOOTSTEP", "_LEFT _RIGHT _01 _02 _QUIET _LOUD _LO _HI _UP _DOWN _DN"),
]

for stem, sufs_str in candidates:
    print(f"\n=== {stem} family ===")
    name = stem
    h = calcHash(name)
    print(f"  {name:35s} -> 0x{h:08x}  {status_of(h)}")
    for suf in sufs_str.split():
        name = stem + suf
        h = calcHash(name)
        print(f"  {name:35s} -> 0x{h:08x}  {status_of(h)}")
