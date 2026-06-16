#!/usr/bin/env python3
"""Enumerate STEM strings for the FINN directional family.

The family is STEM + <digit 1-9> (clockwise from up). Every valid stem satisfies
    calcHash(STEM) == 0x10b95810  AND  end_shift(STEM) == 2
(those two facts are exactly equivalent to reproducing the 9 cardinal IDs).

We meet-in-the-middle over up-to-4 vocab tokens (calcHash ignores separators, so an
output 'FINNAIMDIR' can be read FINN_AIM_DIR etc.). Every hit is written out with the
resulting cardinal names for human review.

Out: docs/analysis/asset-identification/finn_stem_candidates.csv  (+ .txt list)
"""
from __future__ import annotations
import csv, time
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
AID = ROOT / "docs/analysis/asset-identification"
ROCK = "/nix/store/6q8vxs8wigcxkkbij9kz5hb5b77vkm8j-rockyou-2025.3/share/wordlists/rockyou.txt"
TARGET = 0x10b95810
END_SHIFT = 2
UP_ID = 0x10b95a10  # sanity: STEM+"1" must equal this

def chs(s):
    h = sh = 0
    for ch in s:
        c = ord(ch)
        if 65 <= c <= 90: pass
        elif 97 <= c <= 122: c -= 32
        elif 48 <= c <= 57: c += 22
        else: continue
        sh = (sh + c - 64) & 31
        h ^= 1 << sh
    return h, sh

def rotl(v, r):
    r &= 31
    return ((v << r) | (v >> ((32 - r) & 31))) & 0xFFFFFFFF

def rotr(v, r):
    r &= 31
    return ((v >> r) | (v << ((32 - r) & 31))) & 0xFFFFFFFF


# ---- vocabulary: game/sprite terms + most-common rockyou words ----
GAME = ["", "FINN", "FIN", "FINNS", "FINNY", "PLAYER", "PLYR", "PLR", "P1", "P", "SHIP", "FLYER", "FLY",
 "FLYING", "JET", "JETPACK", "CRAFT", "POD", "AIM", "AIMING", "AIMER", "TURRET", "GUN", "CANNON", "FACE",
 "FACING", "BODY", "ROCKET", "SAUCER", "UFO", "BIRD", "HEAD", "DUDE", "GUY", "KLAY", "KLAYMEN", "WILLIE",
 "DIR", "DIRECTION", "SPIN", "ROT", "ROTATE", "ANGLE", "ANGLES", "SPRITE", "SPR", "SEQ", "ANM", "ANIM",
 "MAIN", "SUB", "DIVER", "SWIM", "FISH", "PILOT", "HOVER", "WING", "ARM", "MOVE", "MOVING", "WALK", "RUN",
 "SHOOT", "SHOOTING", "ATTACK", "CHAR", "CHARACTER", "OBJ", "OBJECT", "ENT", "ENTITY", "ACTOR", "HERO",
 "AVATAR", "MAN", "BOY", "NPC", "ENEMY", "BOSS", "LEVEL", "STAGE", "GAME", "STATE", "FRAME", "FRAMES",
 "CEL", "CELS", "TILE", "GFX", "IMG", "8WAY", "16WAY", "EIGHT", "SIXTEEN", "WAY", "COMPASS", "CLOCK",
 "VEHICLE", "MACHINE", "FLOAT", "SWING", "AERO", "PLANE", "GLIDE", "GLIDER", "BLIMP", "BALLOON", "WHEEL",
 "OF", "THE", "AND", "TO", "IN", "ON", "UP", "DOWN", "LEFT", "RIGHT", "NEW", "OLD", "BIG", "SM", "LG",
 "A", "B", "C", "D", "E", "S", "X", "Y", "Z", "T", "R", "L", "U", "N"]

def load_vocab(extra_words=1500):
    vocab = list(dict.fromkeys(GAME))
    try:
        seen = set(w for w in vocab)
        added = 0
        with open(ROCK, encoding="latin-1") as f:
            for line in f:
                w = line.strip().upper()
                if 3 <= len(w) <= 9 and w.isalpha() and w not in seen:
                    vocab.append(w); seen.add(w); added += 1
                    if added >= extra_words: break
    except FileNotFoundError:
        pass
    return vocab


def main():
    t0 = time.time()
    vocab = load_vocab()
    toks = [(w, *chs(w)) for w in vocab]
    print(f"vocab tokens: {len(toks)} (incl empty)")

    # 2-token combos (with empties => covers 0,1,2 tokens)
    combos = []  # (str, h, sh)
    for w1, h1, s1 in toks:
        for w2, h2, s2 in toks:
            h = (h1 ^ rotl(h2, s1)) & 0xFFFFFFFF
            s = (s1 + s2) & 31
            combos.append((w1 + w2, h, s))
    print(f"2-token combos: {len(combos):,}  ({time.time()-t0:.0f}s)")

    # index RIGHT combos by (h, sh)
    ridx = defaultdict(list)
    for w, h, s in combos:
        ridx[(h, s)].append(w)
    print(f"indexed right side ({time.time()-t0:.0f}s)")

    # for each LEFT combo, need RIGHT with h_R=rotr(TARGET^h_L,s_L), s_R=(END_SHIFT-s_L)&31
    hits = set()
    for wl, hl, sl in combos:
        needh = rotr(TARGET ^ hl, sl)
        needs = (END_SHIFT - sl) & 31
        for wr in ridx.get((needh, needs), []):
            full = wl + wr
            if full:
                hits.add(full)
    print(f"raw stem preimages: {len(hits)}  ({time.time()-t0:.0f}s)")

    # verify + rank (prefer FINN-ish, shorter, fewer non-dict chars)
    rows = []
    for st in hits:
        h, sh = chs(st)
        assert h == TARGET and sh == END_SHIFT
        assert chs(st + "1")[0] == UP_ID
        score = 0
        if st.startswith("FIN"): score += 5
        if "AIM" in st or "DIR" in st or "FACE" in st or "FLY" in st or "SHIP" in st: score += 3
        if "PLAYER" in st or "FINN" in st: score += 3
        score -= abs(len(st) - 12) * 0.2
        rows.append((score, st))
    rows.sort(reverse=True)

    out = AID / "finn_stem_candidates.csv"
    with out.open("w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["stem", "len", "up(+1)", "right(+5)", "down(+8)", "score"])
        for score, st in rows:
            w.writerow([st, len(st), st + "1", st + "5", st + "8", f"{score:.1f}"])
    (AID / "finn_stem_candidates.txt").write_text(
        "\n".join(f"{st}   (up={st}1 right={st}5 down={st}8)" for _, st in rows))
    print(f"\nwrote {len(rows)} candidates -> {out.relative_to(ROOT)} (+ .txt)")
    print("top 25 by plausibility heuristic:")
    for score, st in rows[:25]:
        print(f"  {st:20} len={len(st)}  up={st}1 right={st}5 down={st}8")


if __name__ == "__main__":
    main()
