#!/usr/bin/env python3
"""Aggressive function-context attack with MITM 2-token compounds.

For each named function, extract its tokens, then try MITM with each pair
of tokens (cross product) — including action verbs and direction modifiers.
"""
from __future__ import annotations
import csv
import re
import sys
import time
from collections import defaultdict
from itertools import product
from pathlib import Path

sys.path.insert(0, "tools/scripts")
from compound_hash_attack import calc_hash_and_shift, rotl, rotr

ROOT = Path(__file__).resolve().parents[2]
SEED = 0x28C0E011
OUT_ROT = 27

# Already known
KNOWN = {0x29C0E211, 0x2AD0F011, 0x0AD0F813, 0x68C0F413, 0x69C04050, 0x69C8F473,
         0x085860D4}


def split_camel(name: str) -> list[str]:
    s = name.replace('_', ' ')
    s = re.sub(r'([a-z0-9])([A-Z])', r'\1 \2', s)
    s = re.sub(r'([A-Z])([A-Z][a-z])', r'\1 \2', s)
    parts = s.split()
    return [p.upper() for p in parts if p]


# Generic noise tokens to lower priority
GENERIC = set("INIT TICK CALLBACK STATE EVENT HANDLER SET GET ON OFF UPDATE PROCESS DESTROY WITH AND OR THE SUB FUN CALLBACK".split())


# Action / state / direction tokens to combine with extracted ones
EXTRA_TOKENS = """
WALK RUN JUMP FALL LAND DIE DEATH HURT HIT
ATTACK SHOOT FIRE SLAM BOUNCE OPEN CLOSE START STOP
TURN DUCK CROUCH SLIDE ROLL SPIN CLIMB SWIM HOVER FLOAT FLY GLIDE
CELEBRATE WIN VICTORY LOSE TAUNT WAVE WAIT
IDLE STAND SPAWN APPEAR DISAPPEAR ENTRY ENTER EXIT REST
SPIT YELL SCREAM ROAR CRY LAUGH
ANIM ANIMATION SPRITE SPR ICON FRAME LOOP CYCLE
SOUND SND SFX FX MUSIC MUS VOICE VOX
LEFT RIGHT UP DOWN FRONT BACK SIDE TOP BOTTOM L R U D
01 02 03 1 2 3 0 4 5
SPAWN PARTICLE PARTICLES DEBRIS EXPLODE EXPLOSION
PROJECTILE PROJ BULLET SHOT BEAM
DEAD ALIVE HURTING DAMAGED DAMAGE
COMMON RARE ROYAL UNIQUE
RED BLUE GREEN YELLOW
A B C D 0 1 2 3 4 5 6 7 8 9
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
        func_ctx[fn] = (ids, r.get("first_examples", ""))

print(f"Loaded {len(func_ctx)} named functions")
print(f"Loaded {len(all_ids)} asset IDs")

# Collect all tokens from all named functions
all_tokens = set()
fn_tokens = {}
for fn, (ids, _) in func_ctx.items():
    toks = [t for t in split_camel(fn) if t not in GENERIC]
    fn_tokens[fn] = toks
    all_tokens.update(toks)

print(f"\n{len(all_tokens)} unique non-generic tokens extracted from function names")

# Add the extra action tokens
all_tokens.update(EXTRA_TOKENS)
print(f"{len(all_tokens)} after adding actions/states")

# Precompute hash data
words = precompute(list(all_tokens))
b_by_h = defaultdict(list)
for w, h, sh in words:
    b_by_h[h].append((w, sh))

# For each named function, attack ITS specific IDs using MITM with its
# tokens (boosted) and all_tokens (background)
print("\n=== ATTACKING ===")
hits = []

# For each function, get target hashes (from its IDs)
for fn, (ids, examples) in func_ctx.items():
    fn_toks = fn_tokens[fn]
    if not fn_toks:
        continue

    # Compute fn-specific token hashes
    A_words = precompute(fn_toks + list(EXTRA_TOKENS))

    # Targets are this function's IDs
    target_hashes = {}
    for i in ids:
        if i in KNOWN: continue
        t = rotr(i ^ SEED, OUT_ROT)
        target_hashes[t] = i

    if not target_hashes: continue

    # MITM: for each (A, hA, shA), for each h_T, find required h_B in b_by_h
    for a, h_A, sh_A in A_words:
        # Test single-token A first (no compound)
        if h_A in target_hashes:
            aid = target_hashes[h_A]
            hits.append((aid, a, "", a, fn, "single"))
        # Test compound A + B
        for h_T, aid in target_hashes.items():
            req_h_B = rotr(h_T ^ h_A, sh_A)
            if req_h_B in b_by_h:
                for b, _ in b_by_h[req_h_B]:
                    full_bare = a + b
                    h_full, _ = calc_hash_and_shift(full_bare)
                    if h_full != h_T: continue
                    full = a + "_" + b
                    hits.append((aid, a, b, full, fn, "compound"))


print(f"\n{len(hits)} raw hits across {len({h[0] for h in hits})} unique IDs")

# Dedupe
seen = set()
unique = []
for hit in hits:
    key = (hit[0], hit[3])
    if key in seen: continue
    seen.add(key)
    unique.append(hit)

# Group by ID and print
by_id = defaultdict(list)
for hit in unique:
    by_id[hit[0]].append(hit)

print(f"\n=== {len(unique)} unique hits across {len(by_id)} IDs ===\n")
for aid in sorted(by_id):
    rows = by_id[aid]
    n = len(rows)
    fns = set(r[4] for r in rows)
    print(f"\n0x{aid:08x}  ({n} candidates from {len(fns)} fn(s))")
    print(f"  context fns: {', '.join(list(fns)[:3])}")
    for hit in rows[:8]:
        aid2, a, b, full, fn, kind = hit
        print(f"    [{kind:8s}]  {full}")

# Save
out = ROOT / "docs/analysis/asset-identification/function_context_compound_hits.csv"
with out.open("w", newline="") as f:
    w = csv.writer(f)
    w.writerow(["id_hex", "tokenA", "tokenB", "name_underscore", "context_function", "kind"])
    for hit in sorted(unique, key=lambda x: (x[0], x[3])):
        w.writerow([f"0x{hit[0]:08x}", hit[1], hit[2], hit[3], hit[4], hit[5]])
print(f"\nwrote {out.relative_to(ROOT)}")
