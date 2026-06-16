"""Verify FX_BLOW_L_2 with sister names. If it's the right family,
nearby variants like FX_BLOW_L_1 / FX_BLOW_L_3 should also be in the WIZZ uncracked set."""
import sys, os, csv
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

# Load ALL uncracked WIZZ
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

print(f"WIZZ uncracked: {len(wizz_uncracked)}")
print(f"All uncracked: {len(all_uncracked)}")
print()

# Sister-validation: FX_BLOW_L_1 .. FX_BLOW_L_9
print("=== FX_BLOW_L_<N> sister check ===")
hits_count = 0
for n in "123456789":
    name = f"FX_BLOW_L_{n}"
    h = calcHash(name)
    in_wizz = h in wizz_uncracked
    in_unc = h in all_uncracked
    if in_wizz:
        marker = "WIZZ-UNCRACKED"
        hits_count += 1
    elif in_unc:
        marker = "uncracked (other level)"
    else:
        marker = "NOT in uncracked set"
    print(f"  {name} -> 0x{h:08x}  [{marker}]")

print(f"\n{hits_count} of 9 are in WIZZ-uncracked")

# Sister sizes
print()
print("=== FX_BLOW_<SZ>_2 sister check ===")
for sz in ["S","M","L","SM","MD","LG","XS","XL","BIG","HUGE","TINY","MED","SMALL","LARGE"]:
    name = f"FX_BLOW_{sz}_2"
    h = calcHash(name)
    in_wizz = h in wizz_uncracked
    in_unc = h in all_uncracked
    if in_wizz:
        marker = "WIZZ-UNCRACKED ***"
    elif in_unc:
        marker = "uncracked elsewhere"
    else:
        marker = ""
    print(f"  {name:20s} -> 0x{h:08x}  {marker}")

# Now FX_BLOW_GALE_<SZ> family
print()
print("=== FX_BLOW_GALE_<SZ> ===")
for sz in ["S","M","L","SM","MD","LG","XS","XL","BIG","HUGE","TINY","MED","SMALL","LARGE","SMA"]:
    name = f"FX_BLOW_GALE_{sz}"
    h = calcHash(name)
    in_wizz = h in wizz_uncracked
    in_unc = h in all_uncracked
    marker = "WIZZ-UNCRACKED ***" if in_wizz else ("uncracked elsewhere" if in_unc else "")
    print(f"  {name:25s} -> 0x{h:08x}  {marker}")
