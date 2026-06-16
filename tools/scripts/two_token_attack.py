#!/usr/bin/env python3
"""Bidirectional 2-token meet-in-the-middle attack on asset IDs.

Algorithm
---------
For full_name = A + B:
    calcHash(full) = calcHash(A) XOR rotl(calcHash(B), shift(A))

So given target h_T = rotr(asset_id ^ SEED, 27):
    h_T = h_A XOR rotl(h_B, sh_A)
    => h_B = rotr(h_T XOR h_A, sh_A)

For each A-token with (h_A, sh_A):
  For each target hash h_T:
    required_h_B = rotr(h_T XOR h_A, sh_A)
    look up B-tokens whose calcHash == required_h_B

Complexity: |A| × |targets| × O(1)  →  36M ops for 55k words.

Output: docs/analysis/asset-identification/two_token_hits.csv
"""
from __future__ import annotations
import csv
import sys
import time
from collections import defaultdict
from pathlib import Path

sys.path.insert(0, "tools/scripts")
from compound_hash_attack import calc_hash_and_shift, rotl, rotr  # type: ignore

ROOT = Path(__file__).resolve().parents[2]
SEED = 0x28C0E011
OUT_ROT = 27
CONFIRMED = {0x29C0E211, 0x2AD0F011, 0x0AD0F813, 0x68C0F413, 0x69C04050, 0x69C8F473}


def load_words(path: str, min_len: int, max_len: int) -> list[tuple[str, int, int]]:
    seen = set()
    out = []
    with open(path) as f:
        for line in f:
            w = line.strip().upper()
            if not w or len(w) < min_len or len(w) > max_len:
                continue
            if w in seen:
                continue
            seen.add(w)
            h, sh = calc_hash_and_shift(w)
            out.append((w, h, sh))
    return out


def load_targets() -> dict[int, list[int]]:
    """Return dict: target_calcHash -> list of asset_ids."""
    out: dict[int, list[int]] = defaultdict(list)
    with (ROOT / "docs/reference/asset-ids-master.csv").open() as f:
        rdr = csv.DictReader(f)
        for r in rdr:
            i = int(r["id_hex"], 16)
            t = rotr(i ^ SEED, OUT_ROT)
            out[t].append(i)
    return out


def main():
    if len(sys.argv) < 2:
        print("usage: two_token_attack.py <wordlist> [min] [max] [extra_words_path]", file=sys.stderr)
        sys.exit(2)
    path = sys.argv[1]
    min_len = int(sys.argv[2]) if len(sys.argv) > 2 else 3
    max_len = int(sys.argv[3]) if len(sys.argv) > 3 else 7

    print(f"loading words {min_len}..{max_len}…", flush=True)
    words = load_words(path, min_len, max_len)
    if len(sys.argv) > 4:
        extra = load_words(sys.argv[4], 1, 30)
        existing = {w for w, _, _ in words}
        for w, h, sh in extra:
            if w not in existing:
                words.append((w, h, sh))
        print(f"  + {len(extra)} extra tokens")
    print(f"  total {len(words)} tokens")

    targets = load_targets()
    print(f"  {sum(len(v) for v in targets.values())} ids → {len(targets)} unique calcHashes")

    b_by_h: dict[int, list[str]] = defaultdict(list)
    for w, h, _ in words:
        b_by_h[h].append(w)

    target_hashes = list(targets.keys())
    print(f"\nrunning attack: {len(words)} A × {len(target_hashes)} targets…")

    hits = []
    t0 = time.time()
    for ai, (A, h_A, sh_A) in enumerate(words):
        for h_T in target_hashes:
            req_h_B = rotr(h_T ^ h_A, sh_A)
            if req_h_B in b_by_h:
                for B in b_by_h[req_h_B]:
                    full = A + B
                    h_full, _ = calc_hash_and_shift(full)
                    if h_full != h_T:
                        continue
                    aid = (SEED ^ rotl(h_full, OUT_ROT)) & 0xFFFFFFFF
                    if aid in targets[h_T] and aid not in CONFIRMED:
                        hits.append((aid, A, B, full))
        if (ai + 1) % 2000 == 0:
            elapsed = time.time() - t0
            rate = (ai + 1) / elapsed if elapsed > 0 else 0
            print(f"  {ai+1}/{len(words)}  ({rate:.0f}/s)  raw_hits={len(hits)}", flush=True)

    elapsed = time.time() - t0
    print(f"\nfinished in {elapsed:.1f}s — {len(hits)} raw hits")

    seen = set()
    unique = []
    for aid, A, B, full in hits:
        key = (aid, full)
        if key in seen:
            continue
        seen.add(key)
        unique.append((aid, A, B, full))
    unique.sort(key=lambda x: (x[0], x[3]))

    n_ids = len({h[0] for h in unique})
    print(f"unique (aid, full) hits: {len(unique)}")
    print(f"unique IDs covered: {n_ids}")

    by_id = defaultdict(list)
    for aid, A, B, full in unique:
        by_id[aid].append((A, B, full))
    print("\n=== sample (first 30 IDs) ===")
    for aid in sorted(by_id)[:30]:
        n = len(by_id[aid])
        examples = "; ".join(f"{A}+{B}" for A, B, _ in by_id[aid][:3])
        print(f"  0x{aid:08x}  ({n} hits)  {examples}")

    out = ROOT / "docs/analysis/asset-identification/two_token_hits.csv"
    out.parent.mkdir(parents=True, exist_ok=True)
    with out.open("w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["id_hex", "tokenA", "tokenB", "full", "tokA_len", "tokB_len"])
        for aid, A, B, full in unique:
            w.writerow([f"0x{aid:08x}", A, B, full, len(A), len(B)])
    print(f"\nwrote {out.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
