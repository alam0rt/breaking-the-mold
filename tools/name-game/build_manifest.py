#!/usr/bin/env python3
"""Build the name-game manifest: pick one representative PNG per sprite id,
copy it into assets/, and emit manifest.js (id -> image + hints).

Run from repo root:  python3 tools/name-game/build_manifest.py
"""
import csv, json, re, shutil
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
SITE = ROOT / "tools/name-game"
ASSETS = SITE / "assets"
EXTRACT = ROOT / "extracted"

# 1. scan extracted PNGs: id_dec -> best representative path
rx = re.compile(r"sprite_(\d+)_.*\.png$")
def rank(p: Path):
    n = p.name
    return (0 if "anim00_f00" in n else 1 if "anim00" in n else 2, n)
best = {}
for p in EXTRACT.rglob("sprite_*.png"):
    m = rx.search(p.name)
    if not m: continue
    i = int(m.group(1))
    if i not in best or rank(p) < rank(best[i]):
        best[i] = p
# direct-blb fallbacks
for p in (ROOT / "docs/analysis/asset-identification/direct_blb_thumbnails").glob("sprite_*_direct.png"):
    m = re.search(r"sprite_(\d+)_direct", p.name)
    if m:
        best.setdefault(int(m.group(1)), p)

# 2. level hints from the identification CSV (id_dec -> levels, human_role)
hints = {}
csv_path = ROOT / "docs/analysis/asset-identification/sprite_identification_template.csv"
if csv_path.exists():
    with csv_path.open(newline="") as f:
        for row in csv.DictReader(f):
            try: i = int(row["sprite_decimal"])
            except Exception: continue
            hints[i] = {"levels": row.get("levels",""), "role": row.get("human_role","")}

# 3. which ids are real sprite ids (from the master extract)
sprite_ids = set()
master = ROOT / "docs/reference/asset-ids-master.csv"
if master.exists():
    with master.open() as f:
        next(f)
        for line in f:
            c = line.split(",")
            if len(c) >= 3 and "sprite" in c[2]:
                sprite_ids.add(int(c[1]))

# 4. copy images + build manifest
ASSETS.mkdir(exist_ok=True)
for old in ASSETS.glob("*.png"): old.unlink()
entries = []
for i in sorted(best):
    if sprite_ids and i not in sprite_ids:
        continue
    hexid = f"0x{i & 0xffffffff:08x}"
    dst = ASSETS / f"{hexid}.png"
    shutil.copyfile(best[i], dst)
    h = hints.get(i, {})
    entries.append({
        "id": i, "hex": hexid, "img": f"assets/{hexid}.png",
        "levels": h.get("levels",""), "role": h.get("role",""),
    })

(SITE / "manifest.js").write_text("window.MANIFEST = " + json.dumps(entries) + ";\n")
print(f"manifest: {len(entries)} sprites with images -> {SITE/'manifest.js'}")
