#!/usr/bin/env python3
"""Detect suffix-swap families in the Skullmonkeys asset ID pool.

Mathematical insight
--------------------

The confirmed Skullmonkeys hash is

    asset_id(name) = SEED ^ rotl(calcHash(name), 27),  SEED = 0x28C0E011

and calcHash is *cumulative* over the normalized name:

    calcHash(A || B) = calcHash(A) ^ rotl(calcHash(B), sh_A)

where sh_A is the running shift after processing A. Therefore if two assets
share a common prefix P and differ only in their suffix (S_a vs S_b),

    delta = id_a ^ id_b
          = rotl(calcHash(S_a) ^ calcHash(S_b), 27 + sh_P)
          = rotl(V_swap, 27 + sh_P)

so `rotr(delta, 27)` is in the rotation class of V_swap = calcHash(S_a) ^
calcHash(S_b). The class is invariant of the prefix.

This lets us:

1. Cluster all 216k id-pair deltas by their rotation class. Real suffix swaps
   used by many prefixes produce fat clusters; random collisions produce singletons.
2. Match clusters against curated suffix swaps ("LEFT" vs "RIGHT", "01" vs "02",
   "WALK" vs "RUN", …). Hitting a fat cluster confirms the family.
3. Reverse-engineer the shared prefix: given (id_a, S_a, S_b) we know
   calcHash(P) and sh_P, and we can brute force / dictionary-search P.

Run::

    python3 tools/scripts/asset_pair_attack.py --threshold 5
    python3 tools/scripts/asset_pair_attack.py --emit-prefixes
"""
from __future__ import annotations

import argparse
import collections
import csv
import itertools
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SEEDED_XOR_MASK = 0x28C0E011
SEEDED_OUTPUT_ROT = 27


# ---------------------------------------------------------------------------
# Hash primitives
# ---------------------------------------------------------------------------


def char_value(ch: str):
    o = ord(ch)
    if "a" <= ch <= "z":
        o -= 32
    elif "0" <= ch <= "9":
        o += 22
    if ("A" <= ch <= "Z") or ("0" <= ch <= "9"):
        return (o - 64) & 31
    return None


def calc_hash_and_shift(text: str) -> tuple[int, int]:
    h = 0
    sh = 0
    for c in text:
        v = char_value(c)
        if v is None:
            continue
        sh = (sh + v) & 31
        h ^= 1 << sh
    return h, sh


def rotl(v: int, r: int) -> int:
    r &= 31
    return ((v << r) | (v >> ((32 - r) & 31))) & 0xFFFFFFFF


def rotr(v: int, r: int) -> int:
    return rotl(v, (-r) & 31)


def rotation_class(v: int) -> int:
    """Return the smallest rotl(v, r) over r in [0..31]."""
    return min(rotl(v, r) for r in range(32))


# ---------------------------------------------------------------------------
# Suffix swaps to test
# ---------------------------------------------------------------------------

# (a, b) pairs of suffix tokens. Each pair contributes V = calcHash(a) ^
# calcHash(b). Multiple pairs can share the same V (e.g. ("L","R") and
# ("LEFT","RIGHT") have different V's, both worth testing).
SUFFIX_SWAPS = [
    # Direction swaps
    ("LEFT", "RIGHT"), ("L", "R"),
    ("UP", "DOWN"), ("U", "D"),
    ("FRONT", "BACK"), ("F", "B"),
    ("TOP", "BOTTOM"),
    ("NORTH", "SOUTH"),
    ("EAST", "WEST"),
    ("FORWARD", "BACKWARD"),
    ("OPEN", "CLOSE"),
    ("ON", "OFF"),
    ("IN", "OUT"),
    ("UPLEFT", "UPRIGHT"),
    ("DOWNLEFT", "DOWNRIGHT"),
    # State swaps
    ("START", "END"), ("BEGIN", "END"),
    ("INTRO", "OUTRO"),
    ("STAND", "WALK"), ("STAND", "RUN"), ("STAND", "JUMP"), ("STAND", "FALL"),
    ("STAND", "HURT"), ("STAND", "DIE"), ("STAND", "ATTACK"), ("STAND", "IDLE"),
    ("WALK", "RUN"), ("WALK", "JUMP"), ("WALK", "FALL"), ("WALK", "HURT"),
    ("WALK", "DIE"), ("WALK", "ATTACK"),
    ("RUN", "JUMP"), ("RUN", "FALL"), ("RUN", "ATTACK"),
    ("JUMP", "FALL"), ("JUMP", "LAND"), ("JUMP", "ATTACK"),
    ("HURT", "DIE"),
    ("IDLE", "WALK"), ("IDLE", "ATTACK"),
    ("ATTACK", "HURT"), ("ATTACK", "DIE"),
    ("WIN", "LOSE"),
    # Numbered swaps (most common in animation banks)
    ("01", "02"), ("02", "03"), ("03", "04"), ("01", "03"),
    ("1", "2"), ("2", "3"), ("3", "4"), ("1", "3"),
    ("A", "B"), ("B", "C"), ("A", "C"), ("A", "D"),
    # Color/team swaps
    ("RED", "BLUE"), ("RED", "GREEN"), ("BLUE", "GREEN"),
    ("BLACK", "WHITE"), ("GOLD", "SILVER"),
    # Size swaps
    ("BIG", "SMALL"), ("BIG", "LITTLE"), ("LARGE", "SMALL"),
    # Region swaps (PAL localization deltas already confirm regional family)
    ("ENGLISH", "FRENCH"), ("ENGLISH", "GERMAN"), ("FRENCH", "GERMAN"),
    ("EN", "FR"), ("EN", "DE"), ("FR", "DE"),
    # Hit/sound variants
    ("HIT", "MISS"), ("DEAD", "ALIVE"),
    # Player input swaps
    ("PRESS", "RELEASE"),
    # Common art-pipeline swaps
    ("DAY", "NIGHT"), ("LIGHT", "DARK"),
    ("CLOSED", "OPENED"),
    ("INACTIVE", "ACTIVE"),
    ("EMPTY", "FULL"),
]


# ---------------------------------------------------------------------------
# Target loading
# ---------------------------------------------------------------------------


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


# ---------------------------------------------------------------------------
# Main analyses
# ---------------------------------------------------------------------------


def cluster_pair_deltas(target_ids):
    """Return {rotation_class -> list of (id_a, id_b)} for all id pairs."""
    clusters: dict[int, list[tuple[int, int]]] = collections.defaultdict(list)
    ids = sorted(target_ids)
    for i, a in enumerate(ids):
        for b in ids[i + 1:]:
            delta = a ^ b
            base = rotr(delta, SEEDED_OUTPUT_ROT)
            cls = rotation_class(base)
            clusters[cls].append((a, b))
    return clusters


def match_swaps_to_clusters(clusters, swap_v_map):
    """For each swap, compute its rotation class and report any matching cluster."""
    cluster_by_class = clusters  # already keyed by class
    matches = []
    for (a, b), v in swap_v_map.items():
        cls = rotation_class(v)
        hits = cluster_by_class.get(cls, [])
        if hits:
            matches.append(((a, b), v, cls, hits))
    matches.sort(key=lambda m: -len(m[3]))
    return matches


def report_clusters(clusters, threshold, top):
    """Print and return the fattest clusters (excluding the trivial zero class)."""
    sizes = sorted(clusters.items(), key=lambda kv: (-len(kv[1]), kv[0]))
    rows = []
    for cls, pairs in sizes:
        if cls == 0 or len(pairs) < threshold:
            continue
        rows.append((cls, pairs))
        if len(rows) >= top:
            break
    return rows


def emit_prefix_candidates(matches, target_info, vocab_words):
    """For each (swap, hit pair), compute (calc_hash(P), sh_P) and look it up in
    a dictionary of common prefix candidates."""
    prefix_table = {}
    for word in vocab_words:
        h, sh = calc_hash_and_shift(word)
        prefix_table.setdefault((h, sh), []).append(word)

    rows = []
    for (s_a, s_b), v, cls, pairs in matches:
        h_a, sh_a = calc_hash_and_shift(s_a)
        h_b, sh_b = calc_hash_and_shift(s_b)
        for id_a, id_b in pairs:
            # Recover the shift: id_a = SEED ^ rotl(calcHash(P) ^ rotl(h_a, sh_P), 27)
            # rotr(id_a ^ SEED, 27) = calcHash(P) ^ rotl(h_a, sh_P)
            # We don't know sh_P directly, but we can recover it from the delta rotation.
            delta = id_a ^ id_b
            base = rotr(delta, SEEDED_OUTPUT_ROT)  # = rotl(v, sh_P)
            # find sh_P such that rotl(v, sh_P) == base
            sh_p = None
            for r in range(32):
                if rotl(v, r) == base:
                    sh_p = r
                    break
            if sh_p is None:
                continue
            # Now recover calcHash(P):
            h_p_from_a = rotr(id_a ^ SEEDED_XOR_MASK, SEEDED_OUTPUT_ROT) ^ rotl(h_a, sh_p)
            h_p_from_b = rotr(id_b ^ SEEDED_XOR_MASK, SEEDED_OUTPUT_ROT) ^ rotl(h_b, sh_p)
            assert h_p_from_a == h_p_from_b
            prefix_matches = prefix_table.get((h_p_from_a, sh_p), [])
            rows.append({
                "suffix_a": s_a,
                "suffix_b": s_b,
                "id_a": f"0x{id_a:08x}",
                "id_b": f"0x{id_b:08x}",
                "calc_hash_prefix": f"0x{h_p_from_a:08x}",
                "shift_prefix": sh_p,
                "prefix_matches": ",".join(prefix_matches),
                "role_a": target_info.get(id_a, {}).get("role", ""),
                "role_b": target_info.get(id_b, {}).get("role", ""),
                "levels_a": target_info.get(id_a, {}).get("levels", ""),
                "levels_b": target_info.get(id_b, {}).get("levels", ""),
            })
    return rows


def _common_prefix_vocab():
    # Reuse the curated game vocabulary from the compound attack.
    from compound_hash_attack import build_vocab  # type: ignore
    vocab = build_vocab()
    # Also try multi-part prefixes built from the vocab (2-token & 3-token).
    seen = set(vocab)
    tokens = list(vocab)
    out = list(tokens)
    for a, b in itertools.product(tokens, tokens):
        s = a + b
        if s not in seen:
            seen.add(s)
            out.append(s)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--threshold", type=int, default=3,
                    help="minimum cluster size to report")
    ap.add_argument("--top", type=int, default=40,
                    help="how many top clusters to print")
    ap.add_argument("--emit-prefixes", action="store_true",
                    help="for each swap-matched cluster, attempt to identify the shared prefix")
    ap.add_argument("--out-clusters", default=str(ROOT / "docs/analysis/asset-identification/pair_clusters.csv"))
    ap.add_argument("--out-matches", default=str(ROOT / "docs/analysis/asset-identification/pair_swap_matches.csv"))
    ap.add_argument("--out-prefixes", default=str(ROOT / "docs/analysis/asset-identification/pair_prefix_candidates.csv"))
    args = ap.parse_args()

    target_info = load_targets()
    target_ids = list(target_info.keys())
    print(f"loaded {len(target_ids)} target ids")

    clusters = cluster_pair_deltas(target_ids)
    cluster_sizes = collections.Counter(len(v) for v in clusters.values())
    print(f"clustered into {len(clusters)} rotation classes; size histogram:")
    for size, count in sorted(cluster_sizes.items(), reverse=True)[:8]:
        print(f"  class-size {size}: {count} classes")

    fat = report_clusters(clusters, args.threshold, args.top)
    print(f"\n=== TOP {len(fat)} CLUSTERS (size >= {args.threshold}) ===")
    for cls, pairs in fat:
        print(f"  class=0x{cls:08x} pop={cls.bit_count():2d} size={len(pairs)}")
        for a, b in pairs[:3]:
            role_a = target_info.get(a, {}).get("role", "")
            role_b = target_info.get(b, {}).get("role", "")
            print(f"    0x{a:08x} <-> 0x{b:08x}  delta=0x{(a ^ b):08x}  {role_a[:25]} / {role_b[:25]}")

    swap_v_map = {(a, b): calc_hash_and_shift(a)[0] ^ calc_hash_and_shift(b)[0]
                  for a, b in SUFFIX_SWAPS}
    matches = match_swaps_to_clusters(clusters, swap_v_map)
    matches_above = [m for m in matches if len(m[3]) >= args.threshold]
    print(f"\n=== SWAP MATCHES (size >= {args.threshold}) ===")
    for (a, b), v, cls, hits in matches_above[:args.top]:
        print(f"  {a:>10s} <-> {b:<10s}  V=0x{v:08x}  class=0x{cls:08x}  cluster_size={len(hits)}")
        for id_a, id_b in hits[:3]:
            role_a = target_info.get(id_a, {}).get("role", "")
            role_b = target_info.get(id_b, {}).get("role", "")
            print(f"      0x{id_a:08x} <-> 0x{id_b:08x}  {role_a[:25]} / {role_b[:25]}")

    # Dump clusters CSV
    out_clusters = Path(args.out_clusters)
    out_clusters.parent.mkdir(parents=True, exist_ok=True)
    with out_clusters.open("w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["class_hex", "popcount", "cluster_size", "id_a", "id_b", "role_a", "role_b"])
        for cls, pairs in fat:
            for a, b in pairs:
                w.writerow([
                    f"0x{cls:08x}", cls.bit_count(), len(pairs),
                    f"0x{a:08x}", f"0x{b:08x}",
                    target_info.get(a, {}).get("role", ""),
                    target_info.get(b, {}).get("role", ""),
                ])

    # Dump swap-match CSV
    out_matches = Path(args.out_matches)
    out_matches.parent.mkdir(parents=True, exist_ok=True)
    with out_matches.open("w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["suffix_a", "suffix_b", "v_hex", "class_hex", "cluster_size",
                    "id_a", "id_b", "role_a", "role_b", "levels_a", "levels_b"])
        for (a, b), v, cls, hits in matches:
            for id_a, id_b in hits:
                w.writerow([
                    a, b, f"0x{v:08x}", f"0x{cls:08x}", len(hits),
                    f"0x{id_a:08x}", f"0x{id_b:08x}",
                    target_info.get(id_a, {}).get("role", ""),
                    target_info.get(id_b, {}).get("role", ""),
                    target_info.get(id_a, {}).get("levels", ""),
                    target_info.get(id_b, {}).get("levels", ""),
                ])

    if args.emit_prefixes:
        prefix_vocab = _common_prefix_vocab()
        print(f"\nbuilding prefix table from {len(prefix_vocab):,} candidate words...")
        rows = emit_prefix_candidates(matches_above, target_info, prefix_vocab)
        rows.sort(key=lambda r: (0 if r["prefix_matches"] else 1, r["suffix_a"], r["id_a"]))
        out_prefixes = Path(args.out_prefixes)
        out_prefixes.parent.mkdir(parents=True, exist_ok=True)
        with out_prefixes.open("w", newline="") as f:
            if rows:
                w = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
                w.writeheader()
                w.writerows(rows)
        hit_rows = [r for r in rows if r["prefix_matches"]]
        print(f"wrote {out_prefixes.relative_to(ROOT)} ({len(rows)} rows, {len(hit_rows)} with named prefix)")
        for r in hit_rows[:30]:
            print(f"  {r['suffix_a']:>8s} <-> {r['suffix_b']:<8s} | prefix={r['prefix_matches']:<20s} | "
                  f"{r['id_a']} vs {r['id_b']} | {r['role_a'][:24]} / {r['role_b'][:24]}")

    print(f"\nwrote {out_clusters.relative_to(ROOT)}")
    print(f"wrote {out_matches.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
