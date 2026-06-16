#!/usr/bin/env python3
"""Per-level entity-type census from BLB Asset 501 records.

Asset 501 entity records (24 bytes) carry entity_type(+0x12) + layer(+0x14), NOT asset
ids. But entity_type maps to an EntityType###_*_Init function whose symbol name gives the
role. So this builds, per level, the roster of entity types/roles present — the role
*context* for that level's assets (esp. the 274 BLB-only assets the code scan can't reach).

Note: +0x12 is the BLB type, remapped to an internal type per-layer by
RemapEntityTypesForLevel (we apply the documented layer-1 pickup remaps for 25/27;
otherwise assume identity, which holds for the layer-2 gameplay entities).

Out: docs/analysis/asset-identification/entity_census.csv (+ .md)
"""
from __future__ import annotations
import re, struct, csv
from collections import Counter, defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
AID = ROOT / "docs/analysis/asset-identification"
EXT = ROOT / "extracted"
SYM = ROOT / "symbol_addrs.txt"

# internal-type -> role label, from EntityType###(_###)*_Role_Init symbol names
def load_type_roles():
    roles = {}
    pat = re.compile(r"EntityType((?:\d+_)*\d+)_([A-Za-z0-9]+?)_Init")
    for line in SYM.read_text(errors="replace").splitlines():
        m = pat.search(line)
        if not m:
            continue
        nums = [int(x) for x in m.group(1).split("_")]
        role = m.group(2)
        for n in nums:
            roles.setdefault(n, role)
    return roles

# documented layer-dependent remaps (BLB type, layer) -> internal type
REMAP = {(25, 1): 0, (25, 2): 79, (27, 1): 4, (27, 2): 97}

ROLE_CAT = [
    ("Boss", r"boss"),
    ("Enemy", r"enemy|flying|floating|cluster|barfo|glider|worm|gorilla"),
    ("Projectile", r"bullet|ammo|projectile|clayball|catchable"),
    ("Collectible", r"pickup|collectible|extralife|greenheart|phart|phoenix|powerup"),
    ("Platform/Decor", r"platform|clock|mechanism|scaled|trigger|directional|grid|gridline"),
    ("Menu/UI", r"menu|randomized"),
]
def role_cat(role):
    rl = role.lower()
    for c, pat in ROLE_CAT:
        if re.search(pat, rl):
            return c
    return "Other"


def main():
    roles = load_type_roles()
    print(f"entity-type roles from symbols: {len(roles)}")

    # per-level: Counter of (internal_type)
    per_level = defaultdict(Counter)
    per_level_layers = defaultdict(Counter)
    for binf in sorted(EXT.glob("*/*/501_entities.bin")):
        level = binf.parts[-3]
        data = binf.read_bytes()
        for i in range(len(data) // 24):
            rec = data[i*24:(i+1)*24]
            blb = struct.unpack_from("<H", rec, 0x12)[0]
            layer = struct.unpack_from("<H", rec, 0x14)[0] & 0xff
            internal = REMAP.get((blb, layer), blb)
            per_level[level][internal] += 1
            per_level_layers[level][layer] += 1

    # write per-level roster
    rows = []
    for level in sorted(per_level):
        cnt = per_level[level]
        total = sum(cnt.values())
        # roster string: role(count)
        roster = []
        catc = Counter()
        for t, c in cnt.most_common():
            role = roles.get(t, f"type{t}")
            roster.append(f"{role}={c}")
            catc[role_cat(role)] += c
        rows.append({
            "level": level, "n_entities": total,
            "n_types": len(cnt),
            "categories": ";".join(f"{k}:{v}" for k, v in catc.most_common()),
            "roster": " ".join(roster[:14]),
        })
    out = AID / "entity_census.csv"
    with out.open("w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0].keys())); w.writeheader(); w.writerows(rows)

    # global type-role table
    glob = Counter()
    for level in per_level:
        glob.update(per_level[level])
    md = ["# Per-level entity census (BLB Asset 501)\n",
          "\nEntity types populating each level, mapped to role via `EntityType###_*_Init` symbols. "
          "Gives the role *context* for each level's assets (the 274 BLB-only assets can't be code-"
          "categorized, but their level's entity roster says what kinds of things live there).\n",
          "\n## Entity types game-wide (internal type → role, total placements)\n\n",
          "| type | role | category | placements |\n|---|---|---|---|\n"]
    for t, c in glob.most_common():
        role = roles.get(t, f"type{t}")
        md.append(f"| {t} | {role} | {role_cat(role)} | {c} |\n")
    md.append("\n## Per-level roster\n\n| level | entities | categories | roster |\n|---|---|---|---|\n")
    for r in rows:
        md.append(f"| {r['level']} | {r['n_entities']} | {r['categories'][:40]} | {r['roster'][:90]} |\n")
    (AID / "entity_census.md").write_text("".join(md))

    print(f"levels: {len(rows)}  -> {out.relative_to(ROOT)} (+ .md)")
    print("\ntop entity types game-wide:")
    for t, c in glob.most_common(15):
        print(f"  type {t:3} {roles.get(t,'?'):28} [{role_cat(roles.get(t,'')):14}] {c}")


if __name__ == "__main__":
    main()
