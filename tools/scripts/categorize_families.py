#!/usr/bin/env python3
"""Categorize asset-id families by STRUCTURE + code references (no literal names needed).

For each XOR-sibling family (cluster_families.csv connected component):
  - bit-vote the shared stem hash; classify residuals as sequence-style (members differ
    by a single short token at consecutive bit positions) vs scattered/alias
  - join member ids -> the decompiled functions that reference them (asset_usage_raw.csv)
    and the call type (SetEntitySpriteId / InitEntitySprite = sprite, PlaySoundEffect =
    audio, '== 0x' = discriminator token)
  - infer a category from the referencing function names

Out: docs/analysis/asset-identification/family_categories.csv (+ .md summary)
"""
from __future__ import annotations
import csv, re
from collections import defaultdict, Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
AID = ROOT / "docs/analysis/asset-identification"

# ---- category inference from function names ----
CAT_RULES = [
    ("Player",      r"player|klay|klaymen|orb|jump|fall|walk|run|throw|duck"),
    ("Boss",        r"boss|shriney|glenn?ynt|klogg|joehead|wizz|monkeymage|head"),
    ("Enemy",       r"enemy|patrol|finn|barfo|glid|monkey|rat|bird|guard|worm|gorilla|trooper|shooter"),
    ("Clayball/Projectile", r"clayball|projectile|spawnprojectile|switchblock|catchball"),
    ("Effect/Particle",     r"debris|particle|spawn.*particle|explos|burst|smoke|spark|poof|gore"),
    ("Collectible",         r"collectible|pickup|1970|icon|orbs|gem|coin|life|checkpoint"),
    ("Menu/UI",     r"menu|toggle|createmenu|cursor|option|title|hud|text|font|paused"),
    ("Teleporter/Decor", r"teleport|decor|platform|vehicle|door|gate|switch|warp"),
    ("Cutscene/Level",   r"cutscene|level|stage|runn|intro|outro|movie"),
]

def categorize(funcs: list[str]) -> str:
    blob = " ".join(funcs).lower()
    for cat, pat in CAT_RULES:
        if re.search(pat, blob):
            return cat
    return "Uncategorized"


def calc_const(ids):
    n = len(ids); const = 0
    for b in range(32):
        if sum((i >> b) & 1 for i in ids) * 2 > n:
            const |= (1 << b)
    return const


def main():
    # id -> (functions set, call-types Counter)
    id_funcs = defaultdict(set)
    id_calls = defaultdict(Counter)
    CALLPAT = [("sprite", r"SetEntitySpriteId|InitEntitySprite|AllocSpriteRenderContext|SpriteId"),
               ("audio",  r"PlaySoundEffect"),
               ("token",  r"== ?0x|!= ?0x")]
    with (AID / "asset_usage_raw.csv").open() as f:
        for r in csv.DictReader(f):
            try: idv = int(r["id_hex"], 16)
            except Exception: continue
            id_funcs[idv].add(r["function"])
            ctx = r.get("context", "")
            for kind, pat in CALLPAT:
                if re.search(pat, ctx):
                    id_calls[idv][kind] += 1

    # id -> levels
    id_lv = {}
    with (AID / "cracked_names.csv").open() as f:
        for r in csv.DictReader(f):
            if r["id_hex"].startswith("0x"):
                id_lv[int(r["id_hex"], 16)] = (r["levels"], r["status"])

    rows = []
    with (AID / "cluster_families.csv").open() as f:
        for r in csv.DictReader(f):
            mem = [int(x, 16) for x in r["members"].split(";") if x.startswith("0x")]
            if len(mem) < 3:
                continue
            const = calc_const(mem)
            res = [m ^ const for m in mem]
            pops = sorted(bin(x).count("1") for x in res)
            singles = [x.bit_length() - 1 for x in res if bin(x).count("1") == 1]
            seq = len(singles) >= max(2, len(mem) // 2) and (max(pops) <= 4)
            span = (min(singles), max(singles)) if singles else None
            # referencing functions across members
            funcs = Counter()
            calls = Counter()
            ref_n = 0
            for m in mem:
                if m in id_funcs:
                    ref_n += 1
                    for fn in id_funcs[m]:
                        funcs[fn] += 1
                    calls.update(id_calls[m])
            topfuncs = [fn for fn, _ in funcs.most_common(3)]
            cat = categorize(list(funcs))
            # representative levels
            lvs = Counter()
            for m in mem:
                if m in id_lv:
                    lvs[id_lv[m][0]] += 1
            toplv = lvs.most_common(1)[0][0][:24] if lvs else ""
            rows.append({
                "n_members": len(mem),
                "n_referenced": ref_n,
                "sequence_style": "YES" if seq else "",
                "token_span": f"{span[0]}-{span[1]}" if span else "",
                "stem_hash": f"0x{const:08x}",
                "call_kinds": ";".join(f"{k}:{v}" for k, v in calls.most_common()),
                "category": cat,
                "top_functions": " | ".join(topfuncs),
                "levels": toplv,
                "members": ";".join(f"0x{m:08x}" for m in mem),
            })

    rows.sort(key=lambda r: (r["sequence_style"] != "YES", -r["n_members"]))
    out = AID / "family_categories.csv"
    with out.open("w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        w.writeheader(); w.writerows(rows)

    # markdown summary
    md = ["# Asset-family categories (structure + code refs)\n",
          f"{len(rows)} families (size>=3). `sequence_style=YES` = members differ by one short "
          "sequential token (directional/frame numbering, like the FINN player family).\n",
          "\n## Category counts\n"]
    catc = Counter(r["category"] for r in rows)
    for c, n in catc.most_common():
        md.append(f"- **{c}**: {n} families\n")
    md.append("\n## Sequence-style families (highest value)\n\n")
    md.append("| stem_hash | n | refd | span | category | functions | levels |\n|---|---|---|---|---|---|---|\n")
    for r in rows:
        if r["sequence_style"] == "YES":
            md.append(f"| {r['stem_hash']} | {r['n_members']} | {r['n_referenced']} | {r['token_span']} "
                      f"| {r['category']} | {r['top_functions'][:60]} | {r['levels']} |\n")
    (AID / "family_categories.md").write_text("".join(md))

    print(f"{len(rows)} families -> {out.relative_to(ROOT)} (+ .md)")
    print(f"sequence-style: {sum(1 for r in rows if r['sequence_style']=='YES')}")
    print("\ncategory counts:")
    for c, n in catc.most_common():
        print(f"  {c:24} {n}")
    print("\n=== sequence-style families ===")
    for r in rows:
        if r["sequence_style"] == "YES":
            print(f"  {r['stem_hash']} n={r['n_members']:2} refd={r['n_referenced']:2} span={r['token_span']:6} "
                  f"[{r['category']:20}] {r['top_functions'][:50]}")


if __name__ == "__main__":
    main()
