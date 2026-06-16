#!/usr/bin/env python3
"""Game-vocabulary biased 2-token attack.

Hits with at least one game-vocab token are scored higher and reported in detail.
Pure-dictionary hits are dumped but flagged as low-confidence.

Usage:
    python3 tools/scripts/game_two_token_attack.py
    python3 tools/scripts/game_two_token_attack.py --extra /tmp/words_36.txt
"""
from __future__ import annotations
import argparse
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


def load_vocab(path: Path) -> set[str]:
    out = set()
    for line in path.read_text().splitlines():
        line = line.strip().split("#", 1)[0].strip().upper()
        if line:
            out.add(line)
    return out


def precompute(words: set[str]) -> list[tuple[str, int, int]]:
    out = []
    for w in sorted(words):
        h, sh = calc_hash_and_shift(w)
        out.append((w, h, sh))
    return out


def load_targets() -> dict[int, list[int]]:
    out: dict[int, list[int]] = defaultdict(list)
    with (ROOT / "docs/reference/asset-ids-master.csv").open() as f:
        rdr = csv.DictReader(f)
        for r in rdr:
            i = int(r["id_hex"], 16)
            t = rotr(i ^ SEED, OUT_ROT)
            out[t].append(i)
    return out


def load_visual_labels() -> dict[int, str]:
    """ID -> visual label from sprite_identification_template.csv if confirmed."""
    out = {}
    p = ROOT / "docs/analysis/asset-identification/sprite_identification_template.csv"
    if not p.exists():
        return out
    with p.open() as f:
        rdr = csv.DictReader(f)
        for r in rdr:
            try:
                aid = int(r["id_hex"], 16)
            except (KeyError, ValueError):
                continue
            label = (r.get("visual_role") or r.get("notes") or "").strip()
            if label:
                out[aid] = label
    return out


def attack(words: list[tuple[str, int, int]],
           targets: dict[int, list[int]],
           gamevocab: set[str]) -> list[tuple]:
    b_by_h: dict[int, list[str]] = defaultdict(list)
    for w, h, _ in words:
        b_by_h[h].append(w)

    target_hashes = list(targets.keys())
    print(f"  attack: {len(words)} A × {len(target_hashes)} targets…")

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
                    if aid not in targets[h_T] or aid in CONFIRMED:
                        continue
                    score = 0
                    if A in gamevocab:
                        score += 5
                    if B in gamevocab:
                        score += 5
                    if A in gamevocab and B in gamevocab:
                        score += 5  # bonus for double-game
                    hits.append((aid, A, B, full, score))
        if (ai + 1) % 1000 == 0:
            elapsed = time.time() - t0
            rate = (ai + 1) / elapsed if elapsed > 0 else 0
            print(f"    {ai+1}/{len(words)}  ({rate:.0f}/s)  hits={len(hits)}",
                  flush=True)

    elapsed = time.time() - t0
    print(f"  finished in {elapsed:.1f}s")
    return hits


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--extra", help="extra wordlist (one word per line)")
    ap.add_argument("--min", type=int, default=2, dest="min_len")
    ap.add_argument("--max", type=int, default=12, dest="max_len")
    args = ap.parse_args()

    vocab = load_vocab(ROOT / "tools/data/game_vocab.txt")
    print(f"game vocab: {len(vocab)} tokens")

    extra = set()
    if args.extra:
        with open(args.extra) as f:
            for line in f:
                w = line.strip().upper()
                if args.min_len <= len(w) <= args.max_len and w.isalnum():
                    extra.add(w)
        print(f"extra vocab: {len(extra)} tokens")

    all_words = vocab | extra
    words = precompute(all_words)
    print(f"total words: {len(words)}")

    targets = load_targets()
    print(f"{sum(len(v) for v in targets.values())} ids → {len(targets)} unique calcHashes")

    hits = attack(words, targets, vocab)

    # dedupe & sort
    seen = set()
    unique = []
    for aid, A, B, full, score in hits:
        key = (aid, full)
        if key in seen:
            continue
        seen.add(key)
        unique.append((aid, A, B, full, score))

    print(f"\n{len(unique)} unique hits across {len({h[0] for h in unique})} IDs")

    # split high-score (game vocab present) vs noise
    high = [h for h in unique if h[4] >= 5]
    pure = [h for h in unique if h[4] == 0]
    double = [h for h in unique if h[4] >= 10]
    print(f"  pure-dictionary (no game vocab):  {len(pure)} hits  "
          f"({len({h[0] for h in pure})} ids)")
    print(f"  ≥1 game-vocab token:              {len(high)} hits  "
          f"({len({h[0] for h in high})} ids)")
    print(f"  BOTH tokens in game vocab:        {len(double)} hits  "
          f"({len({h[0] for h in double})} ids)")

    visual = load_visual_labels()

    # show double-game hits in detail
    if double:
        print(f"\n=== DOUBLE GAME-VOCAB HITS (high confidence) ===")
        by_id = defaultdict(list)
        for aid, A, B, full, score in double:
            by_id[aid].append((A, B, full, score))
        for aid in sorted(by_id, key=lambda x: -len(by_id[x])):
            label = f"  [VISUAL: {visual[aid]}]" if aid in visual else ""
            print(f"\n  0x{aid:08x}{label}")
            for A, B, full, score in by_id[aid][:10]:
                print(f"      {full:30}  ({A} + {B})")

    # show single-game hits
    single = [h for h in high if h[4] < 10]
    if single:
        print(f"\n=== SINGLE-GAME-VOCAB HITS (medium confidence) ===")
        by_id = defaultdict(list)
        for aid, A, B, full, score in single:
            by_id[aid].append((A, B, full, score))
        # sort by visual label first then by id
        sorted_ids = sorted(by_id, key=lambda x: (x not in visual, x))
        for aid in sorted_ids[:50]:
            label = f"  [VISUAL: {visual[aid]}]" if aid in visual else ""
            n = len(by_id[aid])
            sample = "; ".join(f"{full}" for _, _, full, _ in by_id[aid][:5])
            print(f"  0x{aid:08x}{label}  ({n})  {sample[:100]}")

    out = ROOT / "docs/analysis/asset-identification/game_two_token_hits.csv"
    out.parent.mkdir(parents=True, exist_ok=True)
    with out.open("w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["id_hex", "tokenA", "tokenB", "full", "score", "visual_label"])
        for aid, A, B, full, score in sorted(unique, key=lambda x: (-x[4], x[0], x[3])):
            w.writerow([f"0x{aid:08x}", A, B, full, score, visual.get(aid, "")])
    print(f"\nwrote {out.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
