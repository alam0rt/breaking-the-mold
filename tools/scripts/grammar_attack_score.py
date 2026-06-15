#!/usr/bin/env python3
"""Score and filter hits from /tmp/asset_grammar (or compatible TSV files).

Input TSV columns (from C grammar attack):
    id_hex, full_name, prepend, prefix, sep, suffix

Output:
    docs/analysis/asset-identification/grammar_attack_hits.csv  (all hits)
    docs/analysis/asset-identification/grammar_attack_top.csv   (best per id)

Scoring heuristics (each adds to a per-row score):

  +40  role label corroborates ANY token (visual ID anchor)
  +25  the suffix token is in the "high-prior" set (LEFT/RIGHT/01/02/...) AND
       the asset_id is in a cluster known to use that swap
  +15  short total name (<= 12 chars)
  +15  asset id occurs in a "fat" suffix-swap cluster and our base/prefix
       appears as prefix in another cluster member's candidate too
  +10  the base word matches a real English word (we mark this via the
       --english-set option, default: seclists Miscellaneous/lang-english.txt)
   +5  bare base (no prepend, no sep, no suffix) - i.e. a 1-word name
  +20  level token in name matches the asset's known level scope
  -10  per character of "junk" extension (numbers attached to non-numeric base)
  -20  base is numeric only

Then we *deduplicate* by (asset_id, score) and pick the highest-scoring
candidate per asset. We also emit a coverage report: how many of the 658
asset IDs have a >=80 candidate.
"""
from __future__ import annotations

import argparse
import collections
import csv
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from compound_hash_attack import (  # type: ignore
    calc_hash_and_shift,
    rotl,
    rotr,
)

ROOT = Path(__file__).resolve().parents[2]
SEEDED_XOR_MASK = 0x28C0E011
SEEDED_OUTPUT_ROT = 27


HIGH_PRIOR_SUFFIXES = {
    "LEFT", "RIGHT", "UP", "DOWN", "L", "R", "U", "D",
    "STAND", "WALK", "RUN", "JUMP", "FALL", "IDLE", "DIE", "ATTACK", "ATK",
    "HURT", "HIT", "SHOOT", "FIRE", "SPIT", "BOUNCE", "SLAM", "ROLL",
    "OPEN", "CLOSE", "START", "END", "INTRO", "OUTRO",
    "01", "02", "03", "04", "05", "1", "2", "3", "4", "5",
    "A", "B", "C", "D",
}

LEVEL_TOKENS = {"MENU", "PHRO", "SCIE", "TMPL", "FINN", "MEGA", "BOIL", "SNOW",
                "FOOD", "HEAD", "BRG1", "GLID", "CAVE", "WEED", "EGGS", "GLEN",
                "CLOU", "SEVN", "SOAR", "CRYS", "CSTL", "WIZZ", "RUNN", "MOSS",
                "KLOG", "EVIL"}


def load_targets():
    info: dict[int, dict[str, str]] = {}
    master = ROOT / "docs/reference/asset-ids-master.csv"
    with master.open() as f:
        next(f)
        for line in f:
            c = line.rstrip("\n").split(",")
            if len(c) >= 5:
                info[int(c[1])] = {
                    "kinds": c[2], "popcount": c[3], "sources": c[4],
                    "levels": "", "segments": "", "role": "",
                }
    ident = ROOT / "docs/analysis/asset-identification/sprite_identification_template.csv"
    if ident.exists():
        with ident.open(newline="") as f:
            for row in csv.DictReader(f):
                try:
                    i = int(row["sprite_decimal"])
                except Exception:
                    continue
                if i in info:
                    info[i]["levels"] = row.get("levels", "")
                    info[i]["segments"] = row.get("segments", "")
                    info[i]["role"] = row.get("human_role", "")
    return info


def cluster_pair_deltas(target_ids):
    clusters: dict[int, list[tuple[int, int]]] = collections.defaultdict(list)
    ids = sorted(target_ids)
    for i, a in enumerate(ids):
        for b in ids[i + 1:]:
            delta = a ^ b
            base = rotr(delta, SEEDED_OUTPUT_ROT)
            cls = min(rotl(base, r) for r in range(32))
            clusters[cls].append((a, b))
    return clusters


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--hits", type=Path, default=Path("/tmp/grammar_hits.txt"),
                    help="C-attack output TSV")
    ap.add_argument("--english-set", type=Path,
                    default=Path("/nix/store/0yay2lpaas7bf2q3yxddamb8zkii76nf-seclists-2025.3/share/wordlists/seclists/Miscellaneous/lang-english.txt"))
    ap.add_argument("--out-all",
                    default=str(ROOT / "docs/analysis/asset-identification/grammar_attack_hits.csv"))
    ap.add_argument("--out-top",
                    default=str(ROOT / "docs/analysis/asset-identification/grammar_attack_top.csv"))
    ap.add_argument("--min-score", type=float, default=20)
    ap.add_argument("--top", type=int, default=120, help="rows to preview")
    args = ap.parse_args()

    target_info = load_targets()
    clusters = cluster_pair_deltas(target_info.keys())

    # Bucket each id into the rotation classes it participates in (with size)
    id_clusters: dict[int, list[tuple[int, int]]] = collections.defaultdict(list)
    for cls, pairs in clusters.items():
        if len(pairs) < 4 or cls == 0 or cls.bit_count() < 4:
            continue
        for a, b in pairs:
            id_clusters[a].append((cls, len(pairs)))
            id_clusters[b].append((cls, len(pairs)))

    english_set = set()
    if args.english_set.exists():
        with args.english_set.open() as f:
            for line in f:
                w = re.sub(r"[^A-Z0-9]", "", line.strip().upper())
                if 3 <= len(w) <= 18:
                    english_set.add(w)
    print(f"english dictionary: {len(english_set):,} normalized words")

    rows = []
    with args.hits.open() as f:
        for raw in f:
            parts = raw.rstrip("\n").split("\t")
            if len(parts) < 6:
                continue
            id_hex, full, prep, base, sep, suf = parts[:6]
            asset = int(id_hex, 16)
            info = target_info.get(asset, {})
            role = info.get("role", "").upper()
            levels = info.get("levels", "").upper()
            kinds = info.get("kinds", "")

            score = 0.0
            tokens = [prep, base, suf]
            for tok in tokens:
                if not tok:
                    continue
                tok_alnum = re.sub(r"[^A-Z0-9]", "", tok)
                if role and tok_alnum in role:
                    score += 40
                    break

            if suf in HIGH_PRIOR_SUFFIXES and asset in id_clusters:
                score += 25
            if base in english_set:
                score += 10
            if not prep and not sep and not suf:
                score += 5
            if base in LEVEL_TOKENS and levels and base in levels:
                score += 20
            if base.isdigit():
                score -= 20
            if len(full) <= 12:
                score += 15
            elif len(full) > 20:
                score -= (len(full) - 20)

            # Penalize "all-digits" full name
            if full.replace("_", "").replace("-", "").isdigit():
                score -= 30

            rows.append({
                "score": round(score, 1),
                "id_hex": id_hex,
                "full": full,
                "prepend": prep,
                "base": base,
                "sep": sep,
                "suffix": suf,
                "role": info.get("role", ""),
                "levels": info.get("levels", ""),
                "kinds": kinds,
                "sources": info.get("sources", ""),
                "in_swap_cluster": int(asset in id_clusters),
            })

    rows.sort(key=lambda r: (-r["score"], r["id_hex"], r["full"]))

    out_all = Path(args.out_all)
    out_all.parent.mkdir(parents=True, exist_ok=True)
    with out_all.open("w", newline="") as f:
        if rows:
            w = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
            w.writeheader()
            for r in rows:
                w.writerow(r)
    print(f"wrote {out_all.relative_to(ROOT)} ({len(rows)} rows)")

    # Top per id
    by_id: dict[str, dict] = {}
    for r in rows:
        cur = by_id.get(r["id_hex"])
        if cur is None or r["score"] > cur["score"]:
            by_id[r["id_hex"]] = r
    out_top = Path(args.out_top)
    with out_top.open("w", newline="") as f:
        fields = ["id_hex", "score", "full", "role", "levels", "kinds", "in_swap_cluster", "sources"]
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        for r in sorted(by_id.values(), key=lambda r: (-r["score"], r["id_hex"])):
            w.writerow({k: r.get(k, "") for k in fields})
    print(f"wrote {out_top.relative_to(ROOT)} ({len(by_id)} ids)")

    # Print preview
    print(f"\n=== TOP {args.top} CANDIDATES (score >= {args.min_score}) ===")
    shown = 0
    seen_ids = set()
    for r in rows:
        if r["score"] < args.min_score:
            break
        if r["id_hex"] in seen_ids:
            continue
        seen_ids.add(r["id_hex"])
        cluster_mark = "*" if r["in_swap_cluster"] else " "
        print(f"  {r['score']:5.1f} {cluster_mark}  {r['id_hex']}  {r['full']:30}  "
              f"role={r['role'][:35]:35s}  levels={r['levels'][:25]}")
        shown += 1
        if shown >= args.top:
            break
    print(f"\ncoverage: {len(by_id)}/{len(target_info)} ids hit at any score; "
          f"{sum(1 for r in by_id.values() if r['score'] >= 60)} have score >= 60; "
          f"{sum(1 for r in by_id.values() if r['score'] >= 80)} have score >= 80")


if __name__ == "__main__":
    main()
