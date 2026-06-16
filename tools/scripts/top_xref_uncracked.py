"""List top-xref uncracked audio ids with their xrefs counts."""
import csv, subprocess
from collections import defaultdict

uncracked = []
with open("docs/analysis/asset-identification/cracked_names.csv") as f:
    r = csv.DictReader(f)
    for row in r:
        if row["status"] != "uncracked": continue
        if row["type"] != "audio": continue
        try: hid = int(row["id_hex"], 16)
        except: continue
        uncracked.append((row["id_hex"], int(row["floor"]), row["levels"]))

# Get xref count for each
results = []
for hid, fl, lv in uncracked:
    try:
        out = subprocess.check_output(["grep", "-c", hid, "export/SLES_010.90.c"], text=True).strip()
        xrefs = int(out)
    except:
        xrefs = 0
    if xrefs > 0:
        results.append((xrefs, fl, hid, lv))

results.sort(reverse=True)
print(f"{'ID':12s} {'XRefs':>6s} {'popc':>5s} {'#lvls':>5s} {'LEVELS':50s}")
print("-"*90)
for xr, fl, hid, lv in results[:60]:
    nl = len(lv.split(';')) if lv else 0
    print(f"{hid:12s} {xr:>6d} {fl:>5d} {nl:>5d} [{lv[:60]:60s}]")
