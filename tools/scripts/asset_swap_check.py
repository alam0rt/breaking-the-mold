#!/usr/bin/env python3
"""Directly check whether a (suffix_a, suffix_b) word pair could explain a
known role-labeled cluster, by computing its V = calcHash(a) ^ calcHash(b)
and reporting which cluster's rotation class it falls into.

Usage::

    python3 tools/scripts/asset_swap_check.py SLAM YELL
    python3 tools/scripts/asset_swap_check.py SLAMDOWN YELL
    python3 tools/scripts/asset_swap_check.py --pair SLAM YELL --pair STAND WALK

Also reports which observed cluster (if any) the pair lands in, and prints
all role-labeled IDs in that cluster.
"""
from __future__ import annotations

import argparse
import collections
import csv
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


def check_pair(a: str, b: str, clusters, target_info):
    ha, sha = calc_hash_and_shift(a)
    hb, shb = calc_hash_and_shift(b)
    v = ha ^ hb
    cls = rotation_class(v)
    rotations = [(r, rotl(v, r)) for r in range(32)]
    print(f"\n[{a!r} vs {b!r}]")
    print(f"  calc_hash({a}) = 0x{ha:08x}  sh={sha}")
    print(f"  calc_hash({b}) = 0x{hb:08x}  sh={shb}")
    print(f"  V = 0x{v:08x}, rotation class = 0x{cls:08x} (pop {cls.bit_count()})")

    if cls in clusters:
        pairs = clusters[cls]
        print(f"  CLUSTER MATCH: this swap is in observed cluster of {len(pairs)} pairs.")
        # Find what id pairs would correspond to this swap
        # delta = rotl(V, sh_P + 27)
        for id_a, id_b in pairs:
            delta = id_a ^ id_b
            base = rotr(delta, SEEDED_OUTPUT_ROT)  # = rotl(V, sh_P)
            sh_p = None
            for r in range(32):
                if rotl(v, r) == base:
                    sh_p = r
                    break
            if sh_p is None:
                continue
            # Recover calc_hash(P) from id_a:
            # rotr(id_a ^ SEED, 27) = calc_hash(P) ^ rotl(calc_hash(suff_a), sh_P)
            ch_p = rotr(id_a ^ SEEDED_XOR_MASK, SEEDED_OUTPUT_ROT) ^ rotl(ha, sh_p)
            role_a = target_info.get(id_a, {}).get("role", "")
            role_b = target_info.get(id_b, {}).get("role", "")
            lv_a = target_info.get(id_a, {}).get("levels", "")
            lv_b = target_info.get(id_b, {}).get("levels", "")
            print(f"    pair 0x{id_a:08x} <-> 0x{id_b:08x}: "
                  f"prefix calc_hash=0x{ch_p:08x} sh_P={sh_p} "
                  f"| {role_a[:30]} / {role_b[:30]}")
            if lv_a or lv_b:
                print(f"      levels: {lv_a} / {lv_b}")
    else:
        print("  no observed cluster matches this V class")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("words", nargs="*", help="positional: A B [C D] [E F] ...")
    ap.add_argument("--pair", nargs=2, action="append", metavar=("A", "B"),
                    help="explicit pair to check (can be repeated)")
    args = ap.parse_args()

    pairs = []
    if args.words:
        if len(args.words) % 2 != 0:
            sys.exit("Provide positional pairs (an even number of args).")
        for i in range(0, len(args.words), 2):
            pairs.append((args.words[i], args.words[i + 1]))
    if args.pair:
        pairs.extend(args.pair)
    if not pairs:
        # Default test set: the user's example HEAD_JOE_ATTACK_LEFT/RIGHT plus
        # a handful of plausible action swaps.
        pairs = [
            ("LEFT", "RIGHT"),
            ("L", "R"),
            ("UP", "DOWN"),
            ("U", "D"),
            ("SLAM", "YELL"),
            ("SLAMDOWN", "YELLUP"),
            ("HOCK", "SLAM"),
            ("STAND", "WALK"),
            ("STAND", "RUN"),
            ("STAND", "ATTACK"),
            ("WALK", "JUMP"),
            ("JUMP", "FALL"),
            ("HURT", "DIE"),
            ("IDLE", "ATTACK"),
            ("HOCKA", "HOCKB"),
            ("INTRO", "OUTRO"),
            ("OPEN", "CLOSE"),
            ("OPENING", "CLOSING"),
        ]

    target_info = load_targets()
    clusters = cluster_pair_deltas(target_info.keys())
    for a, b in pairs:
        check_pair(a.upper(), b.upper(), clusters, target_info)


if __name__ == "__main__":
    main()
