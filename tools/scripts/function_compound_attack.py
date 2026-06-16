#!/usr/bin/env python3
"""Function-context attack: function tokens × English wordlist (MITM).

For each named function, attack ITS specific IDs using:
  A token: from function name OR action vocab (boosted)
  B token: from English dictionary (broad)

Use 2-token MITM. This dramatically narrows the search space because
we KNOW the function's role (e.g., ShrineyGuardAttackAnimState) so the
asset names should relate to that role.
"""
from __future__ import annotations
import csv
import re
import sys
import time
from collections import defaultdict
from pathlib import Path

sys.path.insert(0, "tools/scripts")
from compound_hash_attack import calc_hash_and_shift, rotl, rotr

ROOT = Path(__file__).resolve().parents[2]
SEED = 0x28C0E011
OUT_ROT = 27
KNOWN = {0x29C0E211, 0x2AD0F011, 0x0AD0F813, 0x68C0F413, 0x69C04050, 0x69C8F473,
         0x085860D4}


def split_camel(name: str) -> list[str]:
    s = name.replace('_', ' ')
    s = re.sub(r'([a-z0-9])([A-Z])', r'\1 \2', s)
    s = re.sub(r'([A-Z])([A-Z][a-z])', r'\1 \2', s)
    return [p.upper() for p in s.split() if p]


GENERIC = set("INIT TICK CALLBACK STATE EVENT HANDLER SET GET ON OFF UPDATE PROCESS DESTROY WITH AND OR THE SUB FUN".split())

EXTRA_TOKENS = """
WALK RUN JUMP FALL LAND DIE DEATH HURT HIT
ATTACK SHOOT FIRE SLAM BOUNCE OPEN CLOSE START STOP
TURN ROLL SPIN HOVER FLOAT FLY GLIDE SPIT YELL SCREAM ROAR
IDLE STAND SPAWN APPEAR DISAPPEAR ENTRY ENTER EXIT REST
ANIM ANIMATION SPRITE SPR ICON FRAME LOOP CYCLE
SOUND SND SFX FX MUSIC MUS VOICE VOX
LEFT RIGHT UP DOWN L R U D
PARTICLE DEBRIS EXPLODE EXPLOSION
PROJECTILE PROJ BULLET SHOT BEAM
DEAD ALIVE HURTING DAMAGED
COMMON RARE ROYAL UNIQUE
FX1 FX2 FX0 FX01
""".split()


def precompute(words):
    out = []
    seen = set()
    for w in words:
        w = w.upper()
        if w in seen: continue
        seen.add(w)
        h, sh = calc_hash_and_shift(w)
        out.append((w, h, sh))
    return out


def main():
    if len(sys.argv) < 2:
        print("Usage: ... <wordlist>")
        return

    # Load English wordlist
    print(f"Loading wordlist: {sys.argv[1]}")
    words = []
    with open(sys.argv[1]) as f:
        for line in f:
            w = line.strip().upper()
            if 2 <= len(w) <= 8 and w.isalpha():
                words.append(w)
    print(f"  {len(words)} dictionary words")

    # Load asset IDs
    all_ids = set()
    with (ROOT/"docs/reference/asset-ids-master.csv").open() as f:
        rdr = csv.DictReader(f)
        for r in rdr:
            all_ids.add(int(r["id_hex"], 16))

    # Load function context
    func_ctx = {}
    with (ROOT/"docs/analysis/asset-identification/asset_usage_by_function.csv").open() as f:
        rdr = csv.DictReader(f)
        for r in rdr:
            fn = r["function"]
            if re.match(r'(Callback|Sub|FUN|EntityCallback|PlayerCallback)_[0-9a-fA-Fx]+$', fn):
                continue
            ids = [int(x, 16) for x in r["ids"].split(';') if x.strip()]
            func_ctx[fn] = ids
    print(f"\n{len(func_ctx)} named functions")

    # Build dictionary B-index
    B = precompute(words)
    b_by_h = defaultdict(list)
    for w, h, sh in B:
        b_by_h[h].append((w, sh))
    print(f"B-index: {len(b_by_h)} unique hashes ({len(B)} words)")

    # Per-function attack: A = function tokens + extras, B = dictionary
    print("\n=== ATTACKING ===")
    hits = []
    t0 = time.time()
    n_attacked = 0
    for fn, ids in func_ctx.items():
        toks = [t for t in split_camel(fn) if t not in GENERIC]
        if not toks: continue

        # A pool: function tokens + extras
        A = precompute(toks + EXTRA_TOKENS)

        # Targets: this function's IDs (excluding KNOWN)
        target_hashes = {}
        for i in ids:
            if i in KNOWN: continue
            t = rotr(i ^ SEED, OUT_ROT)
            target_hashes[t] = i
        if not target_hashes: continue

        n_attacked += 1
        for a, h_A, sh_A in A:
            # Test A alone
            if h_A in target_hashes:
                aid = target_hashes[h_A]
                hits.append((aid, a, "", a, fn))
            # MITM: A + B
            for h_T, aid in target_hashes.items():
                req_h_B = rotr(h_T ^ h_A, sh_A)
                if req_h_B in b_by_h:
                    for b, _ in b_by_h[req_h_B]:
                        full_bare = a + b
                        h_full, _ = calc_hash_and_shift(full_bare)
                        if h_full != h_T: continue
                        # Score = 1 if A is from fn, 0 otherwise
                        score = 5 if a in toks else 1
                        full = a + "_" + b
                        hits.append((aid, a, b, full, fn))
            # Also B + A (A on right)
            for h_T, aid in target_hashes.items():
                # B + A : h_full = h_B ^ rotl(h_A, sh_B)
                # We iterate over all B hashes; for each, compute required h_A
                # That's the same MITM but with roles swapped. Skip for now —
                # we already have A on left; if real names favor A on right,
                # we'd need this. Accept the limitation.
                pass

    elapsed = time.time() - t0
    print(f"\nAttacked {n_attacked} functions in {elapsed:.1f}s")
    print(f"{len(hits)} raw hits")

    # Dedupe
    seen = set()
    unique = []
    for hit in hits:
        key = (hit[0], hit[3])
        if key in seen: continue
        seen.add(key)
        unique.append(hit)

    by_id = defaultdict(list)
    for hit in unique:
        by_id[hit[0]].append(hit)

    print(f"{len(unique)} unique (id, name) pairs across {len(by_id)} IDs")

    # Print sorted by # of context functions agreeing
    print(f"\n=== TOP HITS (by # function contexts) ===")
    items = []
    for aid, rows in by_id.items():
        fns = set(r[4] for r in rows)
        items.append((len(fns), aid, rows))
    items.sort(reverse=True)

    for nfns, aid, rows in items[:50]:
        if nfns < 1: break
        sample = "; ".join(r[3] for r in rows[:3])
        print(f"  0x{aid:08x}  ({len(rows)} cands, {nfns} fn(s))  {sample[:120]}")

    # Save
    out = ROOT / "docs/analysis/asset-identification/function_compound_hits.csv"
    with out.open("w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["id_hex", "tokenA", "tokenB", "name_underscore", "context_function"])
        for hit in sorted(unique, key=lambda x: (x[0], x[3])):
            w.writerow([f"0x{hit[0]:08x}", hit[1], hit[2], hit[3], hit[4]])
    print(f"\nwrote {out.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
