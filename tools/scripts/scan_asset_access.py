#!/usr/bin/env python3
"""Comprehensive asset-access scan of the decompiled C.

For every KNOWN asset id (from cracked_names.csv), find every occurrence in
export/SLES_010.90.c, the enclosing function, and the access mechanism:
  init    InitEntitySprite(entity, ID, ...)        - entity gets this sprite
  setspr  SetEntitySpriteId(.., ID, ..)            - sprite swap on an entity
  alloc   AllocSpriteRenderContext(.., ID)         - sprite render alloc
  sound   PlaySoundEffect(ID, ..)                  - audio
  token   == ID / != ID / case ID                  - discriminator (event arg / state)
  table   bare ID in a data/array initializer       - loaded via data table
  other   anything else

Then categorize each id by its enclosing function names, and report per-id access
+ per-category rollups.

Out: docs/analysis/asset-identification/asset_access_catalog.csv
     docs/analysis/asset-identification/asset_access_summary.md
"""
from __future__ import annotations
import csv, re
from collections import defaultdict, Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
AID = ROOT / "docs/analysis/asset-identification"
SRC = ROOT / "export/SLES_010.90.c"

CAT_RULES = [
    ("Boss",        r"boss|shriney|glennynt|glenynt|klogg|joehead|wizz|monkeymage|bigwhite|\bhead\b"),
    ("Player",      r"player|klaymen|\bklay|jumpfrom|playerstate|playercallback|soarstate|soarsub|flightmode"),
    ("Enemy",       r"enemy|patrol|finn|barfo|glid|monkey|\brat|\bbird|guard|worm|gorilla|trooper|shooter|"
                    r"damageeventhandler|deatheventhandler|shadowmirror|hazard|spawnentityortrigger"),
    ("Projectile",  r"projectile|clayball|catchball|switchblock|spawnprojectile|homing|ammo|bullet"),
    ("Effect/Particle", r"debris|particle|explos|burst|smoke|spark|poof|gore|spawncollectibleparticle|destroywitheffect"),
    ("Collectible", r"collectible|pickup|1970|icon|\bgem\b|\bcoin\b|extralife|checkpoint|\borbs?\b|powerup"),
    ("Menu/UI",     r"menu|toggle|cursor|option|title|\bhud\b|\btext\b|font|paused|password|createmenu|loadingscreen"),
    ("Teleport/Decor", r"teleport|decor|platform|vehicle|\bdoor\b|\bgate\b|switchblock|warp|elevator|lift|"
                    r"pathfollow|followpath|movingentity|scaledmoving|directionalposition"),
    ("Cutscene/Level", r"cutscene|setupandstartlevel|loadlevel|\bstage\b|runnevent|intro|outro|movie"),
    ("Sound/Audio", r"playsound|sndefx|voice|music"),
]

FUNC_DEF = re.compile(r"^[A-Za-z_][\w \*]*\b([A-Za-z_]\w+)\s*\(")
CTRL = {"if", "for", "while", "switch", "return", "do", "else", "case"}


def enclosing_functions(path):
    """Yield (lineno, line, current_function)."""
    pending = None
    cur = "<global>"
    prev_nonblank = ""
    with path.open(errors="replace") as f:
        for ln, line in enumerate(f, 1):
            s = line.rstrip("\n")
            st = s.strip()
            if st == "{" and pending:
                cur = pending; pending = None
            elif st.startswith("{") is False:
                m = FUNC_DEF.match(s)
                if m and m.group(1) not in CTRL and not s.rstrip().endswith(";") \
                        and "=" not in s.split("(")[0]:
                    pending = m.group(1)
            yield ln, s, cur


def classify(line, idv):
    hx = f"0x{idv:08x}".lower()
    hx2 = f"0x{idv:x}".lower()           # ghidra often drops leading zeros
    L = line.lower()
    if "initentitysprite" in L: return "init"
    if "setentityspriteid" in L: return "setspr"
    if "allocspriterendercontext" in L: return "alloc"
    if "playsoundeffect" in L or "playsound" in L: return "sound"
    if re.search(r"[=!]=\s*0x", L) or "case 0x" in L: return "token"
    if re.match(r"^\s*0x[0-9a-f]+\s*,?\s*$", L) or "{0x" in L or "= {" in L: return "table"
    return "other"


def main():
    # known ids -> dec string and hex variants for matching
    ids = {}
    with (AID / "cracked_names.csv").open() as f:
        for r in csv.DictReader(f):
            if r["id_hex"].startswith("0x"):
                ids[int(r["id_hex"], 16)] = r
    # build a lookup of hex-string -> id (ghidra prints 0x<min-width>)
    hexmap = {}
    for i in ids:
        hexmap[f"0x{i:x}"] = i
        hexmap[f"0x{i:08x}"] = i
    HEXRE = re.compile(r"0x[0-9a-fA-F]{5,8}")

    occ = defaultdict(list)   # id -> [(func, kind, lineno)]
    for ln, line, func in enclosing_functions(SRC):
        if "0x" not in line: continue
        for h in HEXRE.findall(line):
            hl = h.lower()
            if hl in hexmap:
                idv = hexmap[hl]
                occ[idv].append((func, classify(line, idv), ln))

    # per-id rollup
    rows = []
    for idv, info in ids.items():
        recs = occ.get(idv, [])
        funcs = Counter(f for f, _, _ in recs)
        kinds = Counter(k for _, k, _ in recs)
        cat = "Uncategorized"
        blob = " ".join(funcs).lower()
        for c, pat in CAT_RULES:
            if re.search(pat, blob): cat = c; break
        if cat == "Uncategorized" and kinds.get("sound"): cat = "Sound/Audio"
        rows.append({
            "id_hex": f"0x{idv:08x}", "type": info["type"], "status": info["status"],
            "n_refs": len(recs), "access_kinds": ";".join(f"{k}:{v}" for k, v in kinds.most_common()),
            "category": cat, "top_functions": " | ".join(f for f, _ in funcs.most_common(3)),
            "levels": info["levels"][:30],
        })
    # ids with no code reference are loaded via data tables — mark honestly
    for r in rows:
        if r["n_refs"] == 0 and r["category"] == "Uncategorized":
            r["category"] = "Table-only"

    rows.sort(key=lambda r: (r["n_refs"] == 0, r["category"], r["id_hex"]))
    out = AID / "asset_access_catalog.csv"
    with out.open("w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0].keys())); w.writeheader(); w.writerows(rows)

    # rollups
    refd = [r for r in rows if r["n_refs"] > 0]
    catc = Counter(r["category"] for r in rows)
    catc_refd = Counter(r["category"] for r in refd)
    kindc = Counter()
    for r in rows:
        for kv in r["access_kinds"].split(";"):
            if kv: kindc[kv.split(":")[0]] += int(kv.split(":")[1])

    md = ["# Asset access catalog (how every known id is accessed)\n",
          f"\nScanned `export/SLES_010.90.c`. {len(ids)} known ids; **{len(refd)} referenced in code**, "
          f"{len(ids)-len(refd)} loaded via data tables (no direct code ref).\n",
          "\n## Access mechanisms (total occurrences)\n"]
    for k, n in kindc.most_common():
        md.append(f"- `{k}`: {n}\n")
    md.append("\n## Category breakdown (all ids / referenced-only)\n\n| category | all | referenced |\n|---|---|---|\n")
    for c, n in catc.most_common():
        md.append(f"| {c} | {n} | {catc_refd.get(c,0)} |\n")
    (AID / "asset_access_summary.md").write_text("".join(md))

    print(f"{len(ids)} ids; referenced={len(refd)} table-only={len(ids)-len(refd)}")
    print("\naccess mechanisms:")
    for k, n in kindc.most_common(): print(f"  {k:8} {n}")
    print("\ncategory (all / referenced):")
    for c, n in catc.most_common(): print(f"  {c:20} {n:4} / {catc_refd.get(c,0)}")
    print(f"\n-> {out.relative_to(ROOT)} , asset_access_summary.md")


if __name__ == "__main__":
    main()
