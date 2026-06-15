#!/usr/bin/env python3
"""High-throughput grammar attack on the Skullmonkeys asset hash.

Strategy
--------

A real asset name almost always looks like

    <PREFIX_TOKEN>[_<MIDDLE_TOKEN>]_<SUFFIX_TOKEN>

where SUFFIX_TOKEN is one of a small, predictable set ("LEFT", "01",
"WALK", "DIE", ...). We exploit two facts:

1.  calcHash is cumulative: hash(A || B) = hash(A) XOR rotl(hash(B), sh_A).
    So if we precompute (hash, shift) for every token in the *prefix* corpus,
    extending each prefix with each suffix is just one rotation + one xor + one
    table lookup. We can blast through prefix*suffix*sep combinations at
    several million per second in pure Python.

2.  We can use a large prefix corpus (rockyou, English) and a small but very
    targeted suffix corpus (~200 tokens that we know are real animation/
    direction/numbering conventions).

The result table is dumped per (hit_id, candidate). To distinguish real names
from coincidental collisions, the *same prefix* should hit *several IDs* under
different suffixes that form a known swap family — that is the strong signal.

Run::

    /tmp/build_inputs   # see Makefile or inline below
    python3 tools/scripts/grammar_attack.py \
        --prefixes /nix/store/.../rockyou.txt \
        --english /nix/store/.../lang-english.txt \
        --out docs/analysis/asset-identification/grammar_hits.csv

If only --prefixes is given, the script uses only that wordlist; otherwise it
concatenates English + the game vocabulary as additional prefix sources.
"""
from __future__ import annotations

import argparse
import collections
import csv
import re
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from compound_hash_attack import (  # type: ignore
    build_vocab,
    calc_hash_and_shift,
    rotl,
)

ROOT = Path(__file__).resolve().parents[2]
SEEDED_XOR_MASK = 0x28C0E011
SEEDED_OUTPUT_ROT = 27

# Words we treat as plausible last tokens of an asset name. Each entry's hash &
# shift are tabulated once; we then test prefix * separator * suffix combinations.
SUFFIX_TOKENS = (
    [""]
    # action/state tokens
    + ["STAND", "WALK", "RUN", "JUMP", "FALL", "IDLE", "DIE", "DEATH", "HURT",
       "HIT", "SHOOT", "FIRE", "SPIT", "BOUNCE", "ATTACK", "ATK", "LAND",
       "SLIDE", "TURN", "TAUNT", "SCREAM", "YELL", "SLAM", "ROLL", "FLOAT",
       "FLY", "GLIDE", "SPIN", "CLIMB", "SWIM", "PUSH", "PULL", "KICK",
       "PUNCH", "OPEN", "CLOSE", "OPENING", "CLOSING", "START", "END",
       "INTRO", "OUTRO", "STOMP", "STEP", "WAKE", "SLEEP", "WAIT", "REST",
       "EAT", "DANCE", "EJECT", "SPAWN", "ENTER", "EXIT"]
    # direction tokens
    + ["LEFT", "RIGHT", "UP", "DOWN", "L", "R", "U", "D", "FRONT", "BACK",
       "NORTH", "SOUTH", "EAST", "WEST", "TOP", "BOTTOM", "SIDE", "REAR",
       "FACE", "INNER", "OUTER", "ABOVE", "BELOW", "UPLEFT", "UPRIGHT",
       "DOWNLEFT", "DOWNRIGHT", "TOLEFT", "TORIGHT", "FORWARD", "BACKWARD"]
    # numerics
    + [str(i) for i in range(10)]
    + [f"{i:02d}" for i in range(40)]
    + ["A", "B", "C", "D", "E", "F", "G", "H"]
    # body parts
    + ["HEAD", "BODY", "TAIL", "WING", "WINGS", "HORN", "HORNS", "EYE",
       "EYES", "MOUTH", "TEETH", "NOSE", "EAR", "EARS", "ARM", "ARMS",
       "LEG", "LEGS", "FOOT", "FEET", "HAND", "HANDS", "FINGER", "HAIR",
       "STOMACH", "BELLY", "CHEST", "TORSO", "WAIST", "NECK", "PART",
       "PARTS", "PIECE", "LIMB"]
    # type prefixes / suffixes
    + ["SPR", "SPRT", "SEQ", "ANM", "ANIM", "SND", "SFX", "EFX", "FX", "TIM",
       "VAG", "DAT", "BIN", "BG", "FG", "EFFECT"]
    # menu / ui tokens
    + ["YES", "NO", "OK", "QUIT", "PAUSE", "MENU", "ICON", "BUTTON", "BAR",
       "BG", "NUM", "DIGIT", "FONT", "TEXT", "GLYPH"]
    # frame numbers with F prefix
    + [f"F{i:02d}" for i in range(20)]
    + [f"FRAME{i}" for i in range(10)]
)

SEPARATORS = ["", "_", "-"]

PREFIX_TOKENS_PREPEND = ["", "S_", "SPR_", "SEQ_", "SND_", "SFX_", "FX_",
                          "OBJ_", "ENT_", "ITEM_", "MENU_"]

PUNCT = re.compile(r"[^A-Z0-9]")


def normalize(text: str) -> str:
    return PUNCT.sub("", text.upper())


def load_wordlist(path: Path, max_words: int | None = None,
                  min_alnum: int = 3, max_alnum: int = 20) -> list[str]:
    out = []
    seen = set()
    with path.open("rb") as f:
        for raw in f:
            try:
                line = raw.decode("utf-8", "ignore").strip()
            except Exception:
                continue
            normalized = normalize(line)
            if not (min_alnum <= len(normalized) <= max_alnum):
                continue
            if normalized in seen:
                continue
            seen.add(normalized)
            out.append(normalized)
            if max_words and len(out) >= max_words:
                break
    return out


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


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--prefixes", type=Path,
                    default=Path("/nix/store/6q8vxs8wigcxkkbij9kz5hb5b77vkm8j-rockyou-2025.3/share/wordlists/rockyou.txt"),
                    help="primary wordlist (rockyou by default)")
    ap.add_argument("--english", type=Path,
                    default=Path("/nix/store/0yay2lpaas7bf2q3yxddamb8zkii76nf-seclists-2025.3/share/wordlists/seclists/Miscellaneous/lang-english.txt"),
                    help="English wordlist (default: seclists Miscellaneous/lang-english.txt)")
    ap.add_argument("--max-prefix-words", type=int, default=400_000)
    ap.add_argument("--out", type=Path,
                    default=ROOT / "docs/analysis/asset-identification/grammar_hits.csv")
    args = ap.parse_args()

    target_info = load_targets()
    target_set = set(target_info.keys())
    print(f"loaded {len(target_set)} target ids")

    # Build prefix corpus
    prefixes: list[str] = []
    seen = set()

    if args.english and args.english.exists():
        eng = load_wordlist(args.english, min_alnum=3, max_alnum=18)
        for w in eng:
            if w not in seen:
                seen.add(w)
                prefixes.append(w)
        print(f"  +english: {len(prefixes)}")

    vocab = build_vocab()
    for w in vocab:
        nw = normalize(w)
        if nw and nw not in seen:
            seen.add(nw)
            prefixes.append(nw)
    print(f"  +game vocab: {len(prefixes)}")

    if args.prefixes and args.prefixes.exists():
        rock = load_wordlist(args.prefixes, max_words=args.max_prefix_words,
                             min_alnum=3, max_alnum=18)
        for w in rock:
            if w not in seen:
                seen.add(w)
                prefixes.append(w)
        print(f"  +rockyou (capped {args.max_prefix_words}): {len(prefixes)}")

    print(f"final prefix corpus: {len(prefixes):,}")

    # Precompute (calc_hash, shift) per prefix and per suffix token
    prefix_hs = [(p, *calc_hash_and_shift(p)) for p in prefixes]
    suffix_hs = [(s, *calc_hash_and_shift(s)) for s in SUFFIX_TOKENS]
    print(f"suffix tokens: {len(suffix_hs)}")

    # Precompute (prepend_hash, prepend_shift) per prepend prefix
    prepend_hs = [(p, *calc_hash_and_shift(p)) for p in PREFIX_TOKENS_PREPEND]

    t0 = time.time()
    hits: dict[int, list[tuple[str, str, str, str]]] = collections.defaultdict(list)
    scanned = 0

    # Inner loop: prepend + base + separator + suffix
    for prep, hp, shp in prepend_hs:
        for base, hb, shb in prefix_hs:
            # hash(prep || base)
            h_pb = hp ^ rotl(hb, shp)
            sh_pb = (shp + shb) & 31
            for sep in SEPARATORS:
                # sep is a non-alnum string in our case → identity shift
                if sep:
                    sep_h, sep_sh = calc_hash_and_shift(sep)  # zero / 0 (separators are non-alnum)
                else:
                    sep_h, sep_sh = 0, 0
                h_pbs = h_pb ^ rotl(sep_h, sh_pb)
                sh_pbs = (sh_pb + sep_sh) & 31
                for suf, hs, shs in suffix_hs:
                    full_h = h_pbs ^ rotl(hs, sh_pbs)
                    asset = SEEDED_XOR_MASK ^ rotl(full_h, SEEDED_OUTPUT_ROT)
                    scanned += 1
                    if asset in target_set:
                        hits[asset].append((prep, base, sep, suf))
        elapsed = time.time() - t0
        print(f"  ...prepend={prep!r:10s} scanned~{scanned:,} hits={sum(len(v) for v in hits.values())} elapsed={elapsed:.1f}s")

    elapsed = time.time() - t0
    print(f"\ntotal scanned: {scanned:,} in {elapsed:.1f}s "
          f"({scanned / elapsed:,.0f}/s)")
    print(f"unique target ids hit: {len(hits)}")

    # Compress and score
    rows = []
    for asset, candidates in hits.items():
        info = target_info[asset]
        role = info["role"]
        levels = info["levels"]
        for prep, base, sep, suf in candidates:
            joined = prep + base + (sep if (sep and suf) else "") + suf
            # Score: prefer (1) role corroboration, (2) short, (3) dictionary base
            score = 0.0
            if role and (base in role.upper() or suf in role.upper()):
                score += 80
            if len(joined) <= 12:
                score += 30
            if len(joined) <= 8:
                score += 20
            if not prep and not sep and not suf:
                score += 40   # bare base word
            if suf in {"LEFT", "RIGHT", "UP", "DOWN", "STAND", "WALK", "RUN",
                       "JUMP", "FALL", "IDLE", "DIE", "ATTACK", "ATK"}:
                score += 10
            if base in vocab:
                score += 5
            rows.append({
                "score": round(score, 1),
                "id_hex": f"0x{asset:08x}",
                "candidate": joined,
                "prepend": prep,
                "base": base,
                "sep": sep,
                "suffix": suf,
                "role": role,
                "levels": levels,
                "kinds": info["kinds"],
                "sources": info["sources"],
            })

    rows.sort(key=lambda r: (-r["score"], r["id_hex"], r["candidate"]))

    out = args.out
    out.parent.mkdir(parents=True, exist_ok=True)
    with out.open("w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0].keys()) if rows else
                           ["score", "id_hex", "candidate", "prepend", "base",
                            "sep", "suffix", "role", "levels", "kinds", "sources"])
        w.writeheader()
        for r in rows:
            w.writerow(r)

    print(f"wrote {out.relative_to(ROOT)} ({len(rows)} rows)")

    # Per-target winner
    per_target: dict[int, dict] = {}
    for r in rows:
        idv = int(r["id_hex"], 16)
        cur = per_target.get(idv)
        if cur is None or r["score"] > cur["score"]:
            per_target[idv] = r

    by_target_out = out.with_name(out.stem + "_by_target.csv")
    with by_target_out.open("w", newline="") as f:
        if per_target:
            fields = ["id_hex", "score", "candidate", "role", "levels", "kinds", "sources"]
            w = csv.DictWriter(f, fieldnames=fields)
            w.writeheader()
            for idv in sorted(per_target):
                r = per_target[idv]
                w.writerow({k: r.get(k, "") for k in fields})
    print(f"wrote {by_target_out.relative_to(ROOT)} ({len(per_target)} ids)")

    # Print top candidates
    print("\n=== TOP CANDIDATES (score >= 60) ===")
    shown = 0
    for r in rows:
        if r["score"] < 60:
            break
        print(f"  {r['score']:5.1f}  {r['id_hex']}  {r['candidate']:28}  "
              f"role={r['role'][:40]:40s}  levels={r['levels'][:30]}")
        shown += 1
        if shown >= 80:
            break


if __name__ == "__main__":
    main()
