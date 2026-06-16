"""Validate 0xe0880448 - is it FX_PICKUP_SHIELD or another shield/halo name?
Look for sister sounds: the halo create branch is parallel to bird create.
"""
import sys, os, csv
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

uncracked = {}
cracked = {}
with open("docs/analysis/asset-identification/cracked_names.csv") as f:
    import csv as c
    r = c.DictReader(f)
    for row in r:
        try: hid = int(row["id_hex"], 16)
        except: continue
        if row["status"] == "uncracked":
            uncracked[hid] = (row["type"], row["levels"], int(row["floor"]))
        else:
            cracked[hid] = row.get("name","")

def mark(h):
    if h == 0xe0880448: return " <-- TARGET"
    if h in cracked: return f" (CRACKED: {cracked[h]})"
    if h in uncracked: return f" (uncracked {uncracked[h][0]}, {len(uncracked[h][1].split(';'))}lvls)"
    return ""

print("=== Halo/Shield naming candidates ===")
for n in ["FX_PICKUP_SHIELD","FX_PICKUP_HALO","FX_HALO_PICKUP","FX_HALO_ON",
         "FX_HALO_CREATE","FX_HALO_SPAWN","FX_HALO_ACTIVATE","FX_HALO_BORN",
         "FX_SHIELD_ON","FX_SHIELD_UP","FX_SHIELD_CREATE","FX_SHIELD_SPAWN",
         "FX_SHIELD_ACTIVATE","FX_SHIELD_ACTIVE","FX_SHIELD_START","FX_SHIELD_BEGIN",
         "FX_KLAY_SHIELD","FX_KLAY_HALO","FX_KLAY_HALO_ON","FX_KLAY_SHIELD_ON",
         "FX_HALO","FX_SHIELD","FX_HALO_FX","FX_SHIELD_FX",
         "FX_POWERUP_HALO","FX_POWERUP_SHIELD","FX_POWER_HALO","FX_POWER_SHIELD",
         "FX_GAIN_HALO","FX_GAIN_SHIELD","FX_GET_HALO","FX_GET_SHIELD",
         "FX_PWRUP_HALO","FX_PWRUP_SHIELD","FX_PWR_HALO","FX_PWR_SHIELD",
         "FX_KLAY_PWRUP","FX_PWRUP","FX_POWERUP","FX_PWR_UP","FX_POWER_UP",
         "FX_HALO_LOOP","FX_SHIELD_LOOP","FX_HALO_HUM","FX_SHIELD_HUM",
         "FX_PICKUP_PWR","FX_PICKUP_POWER","FX_PICKUP_BUDDY",
         "FX_BIRD","FX_BIRD_SPAWN","FX_BIRD_CREATE","FX_BIRD_ON","FX_KLAY_BIRD",
         "FX_YELLOW_BIRD","FX_YELLOWBIRD","FX_BIRDY","FX_BIRDIE","FX_BIRD_FRIEND",
         "FX_HAMSTER_SHIELD","FX_PICKUP_HAMSTER","FX_GAIN_HAMSTER",
         "FX_GAIN_HAMSTERSHIELD","FX_HAMSTERSHIELD","FX_HAMSTER_HALO",
         "FX_KLAY_BUDDY","FX_BUDDY","FX_FRIEND","FX_FRIEND_ON",
         ]:
    h = calcHash(n)
    m = mark(h)
    if m or h == 0xe0880448:
        print(f"  {n:30s} -> 0x{h:08x}{m}")

# Also check FX_PICKUP_<X> variants
print("\n=== FX_PICKUP_<X> variants ===")
for X in ["BIRD","HALO","SHIELD","SHRED","HAMSTER","ICON","MEDAL","PWR","BOOST",
         "SHARD","ITEM","PARTS","GIB","BIT","UNIT","HP","MP","XP","STAR","GEM",
         "TOKEN","TROPHY","GOLD","COIN","KEY","HEART","ARMOR","LIFE","EXTRALIFE",
         "1UP","ONE","TWO","THREE","BUDDY","FRIEND","ANGEL","BLESS","HEAL","CURE",
         "POTION","ELIXIR","CRYSTAL","RUBY","GEM","JADE","JEWEL","BLING","TRINKET"]:
    n = f"FX_PICKUP_{X}"
    h = calcHash(n)
    m = mark(h)
    if m or h == 0xe0880448:
        print(f"  {n:30s} -> 0x{h:08x}{m}")
