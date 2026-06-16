"""Look for FX_SKULL_GRUNT_<NN> sisters in uncracked space."""
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

print(f"Probing FX_SKULL_<ACTION>_<NN> sisters of FX_SKULL_GRUNT_01...")
print(f"FX_SKULL_GRUNT_01 -> 0x{calcHash('FX_SKULL_GRUNT_01'):08x}\n")

# Check related grunt/head/joe action names
ACT = ["GRUNT","HUFF","BREATH","BREATHE","WHEEZE","COUGH","SNORT","SNORE","SIGH",
       "TALK","SHOUT","ROAR","SCREAM","CRY","YELL","BARK","GROWL","CACKLE","LAUGH",
       "MOAN","GROAN","SOB","GASP","PANT","HUFFPUFF","HMM","UMPH",
       "LAND","BOUNCE","HIT","THUD","DROP","FALL","DIE","DEATH",
       "ANGRY","SCARED","HAPPY","SAD","SLEEP","WAKE","HURT",
       "OUCH","OUCHIE","PAIN","WAH",
       "OPEN","CLOSE","SHUT","SPLAT","SQUASH",
       "EAT","GULP","CHEW","SUCK","SPIT","SLURP"]

for a in ACT:
    for n in [f"FX_SKULL_{a}", f"FX_SKULL_{a}_01", f"FX_SKULL_{a}_02", f"FX_SKULL_{a}_03",
              f"FX_HEAD_{a}", f"FX_HEAD_{a}_01", f"FX_HEAD_{a}_02",
              f"FX_JOE_{a}", f"FX_JOE_{a}_01", f"FX_JOE_{a}_02"]:
        h = calcHash(n)
        if h in cracked:
            print(f"  {n:30s} -> 0x{h:08x} CRACKED: {cracked[h]}")
        elif h in uncracked:
            t, lv, fl = uncracked[h]
            # only show HEAD levels (boss head context)
            if "HEAD" in lv:
                print(f"  {n:30s} -> 0x{h:08x} UNCRACKED {t} popc={fl} [{lv}]")
