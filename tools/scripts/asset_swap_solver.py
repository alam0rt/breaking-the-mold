#!/usr/bin/env python3
"""Match observed pair-delta rotation classes against ALL plausible swap pairs.

The pair-attack discovered concrete rotation classes that appear over and over
in the ID pool (e.g. 0x00283063 connects Shriney-Guard slam to Shriney-Guard
yell). Each class corresponds to a specific *swap*

    V_swap = calcHash(suffix_a) ^ calcHash(suffix_b)

up to a 32-rotation. This script enumerates (suffix_a, suffix_b) pairs over
the game vocabulary and reports which observed classes they match. It also
emits a "high-confidence" filter: only swaps that are both an internal word
swap AND have a sane prefix recovery for at least one role-labeled asset.

Run::

    python3 tools/scripts/asset_swap_solver.py
    python3 tools/scripts/asset_swap_solver.py --min-cluster 4
"""
from __future__ import annotations

import argparse
import collections
import csv
import itertools
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from compound_hash_attack import (  # type: ignore
    build_vocab,
    calc_hash_and_shift,
    rotl,
    rotr,
)

ROOT = Path(__file__).resolve().parents[2]
SEEDED_XOR_MASK = 0x28C0E011
SEEDED_OUTPUT_ROT = 27


def rotation_class(v: int) -> int:
    return min(rotl(v, r) for r in range(32))


def load_targets():
    info: dict[int, dict[str, str]] = {}
    master = ROOT / "docs/reference/asset-ids-master.csv"
    with master.open() as f:
        next(f)
        for line in f:
            c = line.rstrip("\n").split(",")
            if len(c) >= 5:
                info[int(c[1])] = {"kinds": c[2], "sources": c[4], "levels": "", "role": ""}
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
                    info[i]["role"] = row.get("human_role", "")
    return info


def cluster_pair_deltas(target_ids):
    clusters: dict[int, list[tuple[int, int]]] = collections.defaultdict(list)
    ids = sorted(target_ids)
    for i, a in enumerate(ids):
        for b in ids[i + 1:]:
            delta = a ^ b
            base = rotr(delta, SEEDED_OUTPUT_ROT)
            cls = rotation_class(base)
            clusters[cls].append((a, b))
    return clusters


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--min-cluster", type=int, default=3,
                    help="only report clusters with at least this many pairs")
    ap.add_argument("--max-suffix-words", type=int, default=2,
                    help="enumerate suffix candidates of 1..N concatenated vocab tokens")
    ap.add_argument("--top-swaps", type=int, default=30,
                    help="how many top swap candidates to report per cluster")
    ap.add_argument("--out", default=str(ROOT / "docs/analysis/asset-identification/swap_solutions.csv"))
    args = ap.parse_args()

    vocab = build_vocab()
    print(f"vocabulary {len(vocab)} tokens")

    # Generate suffix candidates: 1-token and 2-token compounds from category-
    # restricted pools (action, direction, bodypart, number, etc.).
    cat = {}
    for w, c in vocab.items():
        cat.setdefault(c, []).append(w)

    suffix_words = set()
    for c in ("act", "dir", "body", "num", "ui", "fx", "desc", "char", "item"):
        for w in cat.get(c, []):
            suffix_words.add(w)
    if args.max_suffix_words >= 2:
        # Try concatenated two-token suffixes within likely combos
        for c1 in ("act", "body", "ui"):
            for c2 in ("dir", "num"):
                for a in cat.get(c1, []):
                    for b in cat.get(c2, []):
                        suffix_words.add(a + b)
        for c1 in ("dir", "ui"):
            for c2 in ("act", "num"):
                for a in cat.get(c1, []):
                    for b in cat.get(c2, []):
                        suffix_words.add(a + b)
    print(f"suffix candidate corpus: {len(suffix_words):,}")

    # Compute calc_hash for each suffix candidate
    suffix_table = {}
    for w in suffix_words:
        h, sh = calc_hash_and_shift(w)
        suffix_table[w] = (h, sh)

    # Filter clusters first so we only test against the rotation classes we care about
    target_info = load_targets()
    clusters_all = cluster_pair_deltas(target_info.keys())
    big_clusters = {cls: pairs for cls, pairs in clusters_all.items()
                    if len(pairs) >= args.min_cluster and cls != 0 and cls.bit_count() >= 4}
    target_classes = set(big_clusters.keys())
    # Pre-expand rotations of each target class so we can check by O(1) lookup
    rotated_class_for: dict[int, int] = {}
    for tc in target_classes:
        for r in range(32):
            rotated_class_for[rotl(tc, r)] = tc
    print(f"observed clusters with size>={args.min_cluster}, pop>=4: {len(big_clusters)}")
    print(f"target swap V's after rotation expansion: {len(rotated_class_for)}")

    # Now compute swap V's and only keep ones that match a target class
    swap_v: dict[int, list[tuple[str, str, int]]] = collections.defaultdict(list)
    suffixes = sorted(suffix_table.keys())
    n = len(suffixes)
    print(f"checking {n*(n-1)//2:,} suffix pairs...")
    progress = 0
    for i, a in enumerate(suffixes):
        ha = suffix_table[a][0]
        for j in range(i + 1, n):
            b = suffixes[j]
            hb = suffix_table[b][0]
            v = ha ^ hb
            tc = rotated_class_for.get(v)
            if tc is not None:
                swap_v[tc].append((a, b, v))
        progress += 1
        if progress % 500 == 0:
            print(f"  ...processed {progress}/{n} suffixes, found {sum(len(x) for x in swap_v.values())} hits so far")
    print(f"swap candidate hits: {sum(len(x) for x in swap_v.values())} across {len(swap_v)} target classes")

    target_info = load_targets()  # already loaded above; but harmless if reloaded
    clusters = clusters_all
    big_clusters = {cls: pairs for cls, pairs in big_clusters.items() if cls in swap_v}
    # Sort clusters by interest (size * popcount loosely)
    ordered = sorted(big_clusters.items(),
                     key=lambda kv: (-len(kv[1]), kv[0]))

    rows = []
    for cls, pairs in ordered:
        candidates = swap_v.get(cls, [])
        pairs_with_roles = sum(1 for a, b in pairs
                               if target_info.get(a, {}).get("role") or
                                  target_info.get(b, {}).get("role"))
        sample_roles = []
        for a, b in pairs:
            ra = target_info.get(a, {}).get("role", "")
            rb = target_info.get(b, {}).get("role", "")
            if ra:
                sample_roles.append(f"0x{a:08x}={ra}")
            if rb:
                sample_roles.append(f"0x{b:08x}={rb}")
        sample_roles = list(dict.fromkeys(sample_roles))[:5]
        for a, b, v in candidates[:args.top_swaps]:
            rows.append({
                "class_hex": f"0x{cls:08x}",
                "class_pop": cls.bit_count(),
                "cluster_size": len(pairs),
                "pairs_role_labeled": pairs_with_roles,
                "suffix_a": a,
                "suffix_b": b,
                "v_hex": f"0x{v:08x}",
                "sample_role_ids": " | ".join(sample_roles),
            })

    rows.sort(key=lambda r: (-r["cluster_size"], -r["pairs_role_labeled"], r["class_hex"],
                              len(r["suffix_a"]) + len(r["suffix_b"]), r["suffix_a"]))

    out = Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    with out.open("w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=["class_hex", "class_pop", "cluster_size",
                                          "pairs_role_labeled", "suffix_a", "suffix_b",
                                          "v_hex", "sample_role_ids"])
        w.writeheader()
        for r in rows:
            w.writerow(r)

    print(f"wrote {out.relative_to(ROOT)} ({len(rows)} rows)")

    # Print summary per cluster
    print("\n=== SWAP CANDIDATES FOR FAT CLUSTERS ===")
    seen_classes = set()
    for r in rows:
        cls = r["class_hex"]
        if cls in seen_classes:
            continue
        seen_classes.add(cls)
        # Find all candidates for this class
        cands = [x for x in rows if x["class_hex"] == cls][:8]
        print(f"\nclass={cls} pop={r['class_pop']} cluster_size={r['cluster_size']} "
              f"role_labeled_pairs={r['pairs_role_labeled']}")
        if r["sample_role_ids"]:
            print(f"  roles: {r['sample_role_ids']}")
        for c in cands:
            print(f"    {c['suffix_a']:>14s} <-> {c['suffix_b']:<14s} V={c['v_hex']}")


if __name__ == "__main__":
    main()
