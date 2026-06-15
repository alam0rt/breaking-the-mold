#!/usr/bin/env python3
"""Cluster co-occurrence analyzer.

Goal: find prefix candidates P such that P + several different suffixes
produces multiple target IDs that form pairs in a known swap cluster.

This is the killer test - random collisions don't co-occur across clusters,
but a real prefix will produce TWO ids that we've already observed to be
related by a known suffix swap.

Input: /tmp/grammar_hits.txt  (TSV: id, full, prepend, prefix, sep, suffix)
       docs/reference/asset-ids-master.csv

Output: docs/analysis/asset-identification/cluster_prefix_corroboration.csv

For every (prepend, prefix, sep) triplet that yields >=2 different target IDs,
compute the rotation class of every pairwise XOR between those IDs and compare
to the master pair-cluster list. If two pairs share a fat (size>=4) class
that's the same as the class computed from the actual hash of the two suffixes,
that's a confirmed candidate.
"""
from __future__ import annotations

import collections
import csv
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from compound_hash_attack import calc_hash_and_shift, rotl, rotr  # type: ignore

ROOT = Path(__file__).resolve().parents[2]
SEED = 0x28C0E011
OUT_ROT = 27


def main():
    hits_path = Path("/tmp/grammar_hits.txt")
    out_path = ROOT / "docs/analysis/asset-identification/cluster_prefix_corroboration.csv"

    # Group hits by (prepend, prefix, sep) → list of (suffix, asset_id)
    groups: dict[tuple[str, str, str], list[tuple[str, int]]] = collections.defaultdict(list)
    with hits_path.open() as f:
        for line in f:
            parts = line.rstrip("\n").split("\t")
            if len(parts) < 6:
                continue
            id_hex, full, prep, base, sep, suf = parts[:6]
            asset = int(id_hex, 16)
            groups[(prep, base, sep)].append((suf, asset))

    print(f"total prefix groups: {len(groups):,}")
    interesting = [(k, v) for k, v in groups.items() if len(set(x[1] for x in v)) >= 2]
    print(f"prefix groups producing >=2 unique ids: {len(interesting):,}")

    # Build cluster classes from observed pairs
    targets = sorted(set(asset for vs in groups.values() for _, asset in vs))
    target_set = set(targets)

    # Compute pair clusters of the FULL target master set (more correct than
    # restricting to grammar-hit ids)
    master_ids = []
    with (ROOT / "docs/reference/asset-ids-master.csv").open() as f:
        next(f)
        for line in f:
            c = line.split(",")
            if len(c) >= 2:
                master_ids.append(int(c[1]))

    clusters: dict[int, list[tuple[int, int]]] = collections.defaultdict(list)
    ms = sorted(set(master_ids))
    for i, a in enumerate(ms):
        for b in ms[i + 1:]:
            d = a ^ b
            base = rotr(d, OUT_ROT)
            cls = min(rotl(base, r) for r in range(32))
            clusters[cls].append((a, b))

    pair_class: dict[tuple[int, int], int] = {}
    for cls, pairs in clusters.items():
        if len(pairs) < 4 or cls == 0 or cls.bit_count() < 3:
            continue
        for a, b in pairs:
            pair_class[(min(a, b), max(a, b))] = cls
    print(f"fat-cluster pair edges available: {len(pair_class):,}")

    # Now score each interesting prefix group: count pairs of ids that
    # belong to a fat cluster edge.
    out_rows = []
    for (prep, base, sep), hits in interesting:
        ids = sorted(set(x[1] for x in hits))
        if len(ids) < 2:
            continue
        edges = 0
        edge_cls_counts: collections.Counter[int] = collections.Counter()
        edge_examples = []
        for i, a in enumerate(ids):
            for b in ids[i + 1:]:
                cls = pair_class.get((a, b))
                if cls is not None:
                    edges += 1
                    edge_cls_counts[cls] += 1
                    if len(edge_examples) < 4:
                        # find suffixes that produced a, b for example
                        suf_a = next(s for s, x in hits if x == a)
                        suf_b = next(s for s, x in hits if x == b)
                        edge_examples.append((a, b, cls, suf_a, suf_b))
        if edges == 0:
            continue

        # Verify the swap math: for the example pair (a, b) with suffixes
        # (suf_a, suf_b), the prefix P's calcHash combined with these suffix
        # hashes should equal a, b.
        prefix_full = f"{prep}{base}{sep}"
        h_pref, sh_pref = calc_hash_and_shift(prefix_full)
        verified = 0
        for a, b, cls, sa, sb in edge_examples:
            ha, _ = calc_hash_and_shift(sa)
            hb, _ = calc_hash_and_shift(sb)
            id_a_calc = SEED ^ rotl(h_pref ^ rotl(ha, sh_pref), OUT_ROT)
            id_b_calc = SEED ^ rotl(h_pref ^ rotl(hb, sh_pref), OUT_ROT)
            if id_a_calc == a and id_b_calc == b:
                verified += 1

        out_rows.append({
            "prepend": prep,
            "prefix": base,
            "sep": sep,
            "full_prefix": prefix_full,
            "n_unique_ids": len(ids),
            "n_cluster_edges": edges,
            "verified_edges": verified,
            "top_cluster_class": max(edge_cls_counts, key=edge_cls_counts.get) if edge_cls_counts else 0,
            "top_cluster_count": max(edge_cls_counts.values()) if edge_cls_counts else 0,
            "example_pairs": "; ".join(
                f"0x{a:08x}+{sa}↔0x{b:08x}+{sb} cls=0x{cls:08x}"
                for a, b, cls, sa, sb in edge_examples
            ),
        })

    out_rows.sort(key=lambda r: (-r["n_cluster_edges"], -r["n_unique_ids"]))
    out_path.parent.mkdir(parents=True, exist_ok=True)
    with out_path.open("w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(out_rows[0].keys()) if out_rows else
                                          ["prepend", "prefix", "sep", "full_prefix",
                                           "n_unique_ids", "n_cluster_edges",
                                           "verified_edges", "top_cluster_class",
                                           "top_cluster_count", "example_pairs"])
        w.writeheader()
        for r in out_rows:
            w.writerow(r)
    print(f"wrote {out_path.relative_to(ROOT)} ({len(out_rows)} candidate prefixes)")

    print("\n=== TOP 40 PREFIXES BY CLUSTER EDGE COUNT ===")
    for r in out_rows[:40]:
        print(f"  edges={r['n_cluster_edges']:2d}  ids={r['n_unique_ids']:2d}  "
              f"prefix={r['full_prefix']:25s}  example={r['example_pairs'][:120]}")


if __name__ == "__main__":
    main()
