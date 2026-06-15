#!/usr/bin/env python3
"""Exhaustive preimage solver for Skullmonkeys asset ids.

For a target id, asset_id = SEED ^ rotl(calcHash(name), 27), so
    calcHash(name) == rotr(id ^ SEED, 27) =: T
and the FLOOR (minimum name length) is popcount(T) (each char toggles one bit).

This enumerates EVERY string of length == floor that hashes to the target
(the complete minimal-length preimage set), then filters/scores that list:
no word-anchoring during generation, so a known word anywhere in the string
is found. Generation is exact, not sampled.

Why minimal length only: the count of length-L preimages of one value grows
like 36^L / 2^32 -- a handful at floor 4, ~hundreds at 8, ~3e7 at 11, ~4e10 at
13. So exhaustive enumeration is feasible exactly for the short names we can
actually recover (menu text); longer floors are reported as "too large".

At floor length there is no bit cancellation, so the toggled positions are a
permutation of T's set bits, and each consecutive forward step (mod 32) must be
1..26 (the reachable per-char step range: A..Z -> 1..26, 0..9 -> 6..15). We DFS
those orderings (heavy gap pruning), then expand each step to its character(s):
step k -> letter chr(64+k), plus a digit when 6<=k<=15.

    python3 tools/scripts/klash_solve.py                 # all ids, floor<=cap
    python3 tools/scripts/klash_solve.py --max-floor 9
    python3 tools/scripts/klash_solve.py --ids 0x29c0e211 0x68c0f413
"""
from __future__ import annotations
import argparse, csv, itertools, random, sys, time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import name_attack as na   # reuse the curated vocabulary + target loader

SEED = 0x28C0E011

def rotl(v, r): r &= 31; return ((v << r) | (v >> ((32 - r) & 31))) & 0xFFFFFFFF
def rotr(v, r): return rotl(v, (32 - (r & 31)) & 31)
def popcount(v): return bin(v & 0xFFFFFFFF).count("1")

def calc(s):
    h = 0; sh = 0
    for ch in s:
        o = ord(ch)
        if 'a' <= ch <= 'z': o -= 32
        elif '0' <= ch <= '9': o += 22
        if ('A' <= ch <= 'Z') or ('0' <= ch <= '9'):
            sh = (sh + (o - 64)) & 31; h ^= 1 << sh
    return h

# step value -> the character(s) that produce it (1..26 letters; 6..15 also a digit)
STEP_CHARS = {}
for k in range(1, 27):
    chars = [chr(64 + k)]                       # A..Z
    if 6 <= k <= 15: chars.append(chr(48 + k - 6))  # 0..9
    STEP_CHARS[k] = chars

# -------------------------------------------------------------- spinner
class Spinner:
    FRAMES = list(".oO0Oo. .·oO0Oo  ˚o0O0o˚ .oO@Oo.".split(" "))
    MSGS = [
        "the evil engine is busy uncovering the secrets of the neverh00d",
        "rotating bits by 27, as one does",
        "xor-ing reality against 0x28c0e011",
        "interrogating the shriney guard for filenames",
        "decompiling monkey screams into ascii",
        "rummaging through klogg's filing cabinet",
        "toggling bits in the clay",
        "asking willie trombone very nicely",
        "reassembling the lost toolx pipeline",
        "counting fart particles",
    ]
    def __init__(self, enabled=True):
        self.enabled = enabled and sys.stderr.isatty()
        self.t0 = time.time(); self.last = 0; self.fi = 0; self.mi = 0
    def tick(self, done, total, found):
        if not self.enabled: return
        now = time.time()
        if now - self.last < 0.08: return
        self.last = now
        # jittery clay ball: usually advance, sometimes stutter/skip
        self.fi = (self.fi + random.choice([1, 1, 1, 0, 2])) % len(self.FRAMES)
        if int(now - self.t0) and int(now) % 4 == 0:
            self.mi = (self.mi + 1) % len(self.MSGS)
        ball = self.FRAMES[self.fi]
        pad = " " * random.choice([0, 0, 1])   # tiny horizontal jitter
        sys.stderr.write(f"\r{pad}{ball}  {self.MSGS[self.mi]}… [{done}/{total}, {found} clean] ")
        sys.stderr.flush()
    def done(self, found):
        if self.enabled:
            sys.stderr.write(f"\r .oO0Oo.  done — {found} clean candidates uncovered." + " " * 30 + "\n")
            sys.stderr.flush()

# ------------------------------------------------- exhaustive enumeration
def enum_floor_preimages(T, cap):
    """Yield every string of length popcount(T) with calcHash == T (up to cap)."""
    bits = [i for i in range(32) if (T >> i) & 1]
    n = len(bits)
    if n == 0:
        yield ""; return
    out = 0
    stack = [(0, 0, [])]   # (prev_pos, used_mask, step_list)
    # iterative DFS to avoid recursion overhead
    def rec(prev, used, steps):
        nonlocal out
        if out >= cap: return
        if len(steps) == n:
            for combo in itertools.product(*[STEP_CHARS[s] for s in steps]):
                yield "".join(combo)
                out += 1
                if out >= cap: return
            return
        for idx, b in enumerate(bits):
            if used & (1 << idx): continue
            gap = (b - prev) % 32
            if 1 <= gap <= 26:
                yield from rec(b, used | (1 << idx), steps + [gap])
                if out >= cap: return
    yield from rec(0, 0, [])

def count_orderings(T):
    """Cheap upper-bound-ish count of orderings (no char expansion) to gauge feasibility."""
    bits = [i for i in range(32) if (T >> i) & 1]
    n = len(bits)
    if n <= 1: return 1
    total = 0
    def rec(prev, used, depth):
        nonlocal total
        if total > 5_000_000: return
        if depth == n: total += 1; return
        for idx, b in enumerate(bits):
            if used & (1 << idx): continue
            if 1 <= (b - prev) % 32 <= 26:
                rec(b, used | (1 << idx), depth + 1)
    rec(0, 0, 0)
    return total

# ------------------------------------------------------------- scoring
def best_word(s, vocab):
    """Longest vocab word that is a substring of s; returns (word, start) or None."""
    best = None
    for i in range(len(s)):
        for j in range(len(s), i, -1):
            w = s[i:j]
            if w in vocab and (best is None or len(w) > len(best[0])):
                best = (w, i)
                break
    return best

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--max-floor", type=int, default=9, help="skip ids whose floor exceeds this")
    ap.add_argument("--cap", type=int, default=400_000, help="max preimages enumerated per id")
    ap.add_argument("--ids", nargs="*", help="specific ids (hex/dec) instead of the whole pool")
    ap.add_argument("--words", nargs="*", default=[])
    ap.add_argument("--out", default=str(Path(na.ROOT) / "docs/analysis/asset-identification/preimage_candidates.csv"))
    args = ap.parse_args()

    vocab = na.build_vocab(args.words)
    targets = na.load_targets()
    if args.ids:
        want = [int(x, 0) for x in args.ids]
    else:
        want = sorted(targets)

    # which ids are in feasible-floor range
    todo = []
    for idv in want:
        T = rotr(idv ^ SEED, 27)
        fl = popcount(T)
        if fl <= args.max_floor:
            todo.append((idv, T, fl))
    sp = Spinner()
    print(f"{len(todo)} ids with floor <= {args.max_floor} (of {len(want)})", file=sys.stderr)

    rows = []
    pattern_counts = {}   # filler-string -> how many ids it appears in (recurring affix = real)
    found = 0
    for n_done, (idv, T, fl) in enumerate(todo, 1):
        sp.tick(n_done, len(todo), found)
        # feasibility guard
        if count_orderings(T) > 2_000_000:
            continue
        info = targets.get(idv, {})
        role = (info.get("role") or "").upper()
        for s in enum_floor_preimages(T, args.cap):
            bw = best_word(s, vocab)
            if not bw:
                continue
            word, start = bw
            filler = s[:start] + s[start + len(word):]
            coverage = len(word) / len(s)
            corrob = word in role
            score = round(len(word) * 12 - len(filler) * 9 + (60 if corrob else 0) + coverage * 20, 1)
            rows.append([score, int(corrob), f"0x{idv:08x}", s, word, filler, fl,
                         info.get("role", ""), info.get("levels", "")])
            pattern_counts[filler] = pattern_counts.get(filler, 0) + 1
            found += 1
    sp.done(found)

    # pattern reuse: boost candidates whose filler recurs across multiple ids
    for r in rows:
        filler = r[5]
        if filler and pattern_counts.get(filler, 0) >= 2:
            r[0] = round(r[0] + 8 * min(pattern_counts[filler], 5), 1)

    rows.sort(key=lambda r: (-r[0], r[2]))
    # keep best few per id for the file
    out = Path(args.out)
    with out.open("w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["score", "role_corroborated", "id_hex", "candidate", "word", "filler", "floor", "role", "levels"])
        w.writerows(rows)
    print(f"\nwrote {len(rows)} scored candidates -> {out.relative_to(na.ROOT)}\n")
    print("=== top candidates ===")
    seen_ids = set()
    for r in rows:
        if r[2] in seen_ids:
            continue
        seen_ids.add(r[2])
        star = "★" if r[1] else " "
        tag = f"role:{r[7]}" if r[7] else f"levels:{r[8][:24]}"
        print(f"  {star} {r[2]}  {r[3]:14s} = [{r[4]}]+[{r[5]}]  score={r[0]:6} {tag}")
        if len(seen_ids) >= 40:
            break

if __name__ == "__main__":
    main()
