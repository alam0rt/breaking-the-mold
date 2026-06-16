"""Look for sisters of FX_PUFF_FALL_3 - other FINN/GLID audio."""
import sys, os, csv
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

uncracked = {}
cracked = {}
with open("docs/analysis/asset-identification/cracked_names.csv") as f:
    r = csv.DictReader(f)
    for row in r:
        try: hid = int(row["id_hex"], 16)
        except: continue
        if row["status"] == "uncracked":
            uncracked[hid] = (row["type"], row["levels"], int(row["floor"]))
        else:
            cracked[hid] = row.get("name","")

def stat(h):
    if h in cracked: return f"CRACKED ({cracked[h]})"
    if h in uncracked:
        t,l,fl = uncracked[h]
        return f"UNCRACKED ({t},[{l}],popc={fl})"
    return ""

# Try every PUFF_<ACTION>_<N> combination
ACTIONS = ["FALL","JUMP","LAND","UP","DOWN","FLOAT","SINK","RISE","DROP",
           "INFLATE","DEFLATE","BURST","BLOW","BREATH","PUFF","CRY",
           "HURT","DIE","HIT","SPIN","TURN","SWIM","DIVE","BOUNCE",
           "PAD","FLAP","HOVER","GLIDE","SOAR","DRIFT",
           "FAST","SLOW","START","STOP","END","BEGIN",
           "LOOP","LOOPED","TICK","WALK","RUN","STEP"]

SUFS = ["","_1","_2","_3","_4","_5","_6","_7","_8","_9","_0",
        "_01","_02","_03","_04","_05",
        "_A","_B","_C","_D",
        "_UP","_DOWN","_SM","_LG","_BIG","_END","_LOOP","_START",
        "_HARD","_SOFT","_LOUD","_QUIET","_LO","_HI"]

PREFIXES = ["FX_","SFX_","FX_PUFF_","FX_PUFFY_","FX_PUFFER_"]
ENTITIES = ["PUFF","PUFFY","PUFFER","KLAY","FINN","BABY"]

hits_by_id = {}
for pre in PREFIXES:
    for ent in ENTITIES:
        for act in ACTIONS:
            for suf in SUFS:
                if pre.endswith("_"):
                    name = pre + act + suf
                else:
                    name = pre + ent + "_" + act + suf
                h = calcHash(name)
                if h in uncracked and uncracked[h][0] == "audio":
                    hits_by_id.setdefault(h, []).append(name)
                elif h in uncracked and uncracked[h][0] == "sprite":
                    # only emit sprite hits if FINN
                    if "FINN" in uncracked[h][1] or "GLID" in uncracked[h][1]:
                        hits_by_id.setdefault(h, []).append(name)

# Filter for FINN/GLID
print("=== FINN/GLID audio with PUFF/Klaymen patterns ===")
finn_glid = [(h, n) for h, n in hits_by_id.items()
             if uncracked[h][0] == "audio" and (
                "FINN" in uncracked[h][1] or "GLID" in uncracked[h][1]
             )]
finn_glid.sort()
for hid, names in finn_glid:
    t, lv, fl = uncracked[hid]
    print(f"\n0x{hid:08x} popc={fl} levels=[{lv}]")
    for n in sorted(set(names))[:6]:
        print(f"    {n!r}")

# Other audio
print("\n=== Other audio hits ===")
other = [(h, n) for h, n in hits_by_id.items()
         if (h, n) not in finn_glid and uncracked[h][0] == "audio"]
other.sort()
for hid, names in other[:20]:
    t, lv, fl = uncracked[hid]
    print(f"\n0x{hid:08x} popc={fl} levels=[{lv}]")
    for n in sorted(set(names))[:6]:
        print(f"    {n!r}")
