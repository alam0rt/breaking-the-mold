#!/usr/bin/env python3
"""Function-name-context attack on asset IDs.

For each function name that uses asset IDs, decompose the function name
into tokens and try those tokens as candidate asset names — including
combinations with action verbs.

Examples of useful function names from asset_usage_by_function.csv:
- ShrineyGuardAttackAnimState → 0x085860d4 (and 4 others)
- KloggDeathEventHandler → 0x800da102 (and 5 others)
- JoeHeadJoeMoveAndCheckAttack → 0x0b290ba2 (and 3 others)
- InitMonkeyMageBoss → 0x0244655d (and 2 others)
- GlennYntisDeathEventHandler → 0x18e88310 (and 3 others)
- GlennYntisDamageEventHandler → 0x8b603980 (and 2 others)
- ShrineyGuardSoundUpdateTick → 0x086e44c0 (and 3 others)
- TeleporterActivate → 0x0bbb70ac (and 2 others)
- ClayballState_DestroyWithDebris → 0x462c6040 (and 4 others)
- BossState_DefeatedWithParticles → 0x424d1840 (and 1 other)
- CollectibleHamsterShieldTickCallback → 0x42906465 (and 1 other)
- InitPhartHeadCollectible → 0x8c510186 (and 1 other)
- Collectible1970IconTickCallback → 0x408a6461 (and 1 other)

Strategy: extract camelCase tokens from function name, build candidate
strings (UPPER, with/without underscores, with action suffixes).
"""
from __future__ import annotations
import csv
import re
import sys
from pathlib import Path
from itertools import product

sys.path.insert(0, "tools/scripts")
from compound_hash_attack import asset_id

ROOT = Path(__file__).resolve().parents[2]


def split_camel(name: str) -> list[str]:
    """Split CamelCase / snake_case into uppercase tokens."""
    # Replace underscores with spaces, then split CamelCase
    s = name.replace('_', ' ')
    # Insert space before each capital letter that follows a lowercase
    s = re.sub(r'([a-z0-9])([A-Z])', r'\1 \2', s)
    # Insert space between consecutive caps followed by lowercase: HTMLParser → HTML Parser
    s = re.sub(r'([A-Z])([A-Z][a-z])', r'\1 \2', s)
    parts = s.split()
    return [p.upper() for p in parts if p]


# Skip these — they're generic noise tokens
SKIP = set("INIT TICK CALLBACK STATE EVENT HANDLER SET GET ON OFF UPDATE PROCESS DESTROY WITH AND OR THE A AN OF FOR FROM TO NOT IS HAS BE TYPE FN FUNCTION FUNC POINTER PTR SUB ROUTINE".split())


def variants(tokens: list[str]) -> set[str]:
    """Generate name variants from a list of tokens."""
    out = set()
    # Single tokens
    for t in tokens:
        if t in SKIP: continue
        out.add(t)
    # Joined all
    out.add("".join(tokens))
    out.add("_".join(tokens))
    # Pairs
    for i in range(len(tokens)):
        for j in range(len(tokens)):
            if i == j: continue
            if tokens[i] in SKIP or tokens[j] in SKIP: continue
            out.add(tokens[i] + tokens[j])
            out.add(tokens[i] + "_" + tokens[j])
    # Triples (consecutive)
    for i in range(len(tokens) - 2):
        a, b, c = tokens[i], tokens[i+1], tokens[i+2]
        if a in SKIP or b in SKIP or c in SKIP: continue
        out.add(a + b + c)
        out.add(a + "_" + b + "_" + c)
    return out


# Load all asset IDs
all_ids = {}
with (ROOT/"docs/reference/asset-ids-master.csv").open() as f:
    rdr = csv.DictReader(f)
    for r in rdr:
        all_ids[int(r["id_hex"], 16)] = r

# Already known
KNOWN = {0x29C0E211: "NO", 0x2AD0F011: "YES", 0x0AD0F813: "PAUSED",
         0x68C0F413: "QUIT", 0x69C04050: "CONTINUE", 0x69C8F473: "QUITGAME",
         0x085860D4: "GEMCOMMON"}

# Load function context
func_ctx = {}  # function_name -> list of asset_ids
with (ROOT/"docs/analysis/asset-identification/asset_usage_by_function.csv").open() as f:
    rdr = csv.DictReader(f)
    for r in rdr:
        fn = r["function"]
        # Skip generic Callback_/Sub_ funcs
        if re.match(r'(Callback|Sub|FUN|EntityCallback|PlayerCallback)_[0-9a-fA-Fx]+$', fn):
            continue
        ids = [int(x, 16) for x in r["ids"].split(';') if x.strip()]
        func_ctx[fn] = ids

print(f"Loaded {len(func_ctx)} named functions with asset references")
print(f"Loaded {len(all_ids)} total asset IDs")


# For each named function, generate candidate names from its token decomposition
# and test against ITS OWN asset IDs.
hits = []
for fn, ids in func_ctx.items():
    tokens = split_camel(fn)
    cands = variants(tokens)

    # Also try common action/state suffixes
    base_cands = list(cands)
    for c in base_cands:
        for suffix in ["", "ANIM", "ICON", "SPR", "FX", "SFX", "SND"]:
            if suffix:
                cands.add(c + suffix)
                cands.add(c + "_" + suffix)
        for n in range(0, 20):
            cands.add(c + str(n))
            cands.add(c + f"_{n}")

    # Test all candidates against this function's IDs
    for c in cands:
        h = asset_id(c)
        if h in ids and h not in KNOWN:
            hits.append((h, c, fn, len(c)))


# Dedupe
seen = set()
unique = []
for h, c, fn, l in hits:
    if (h, c) in seen: continue
    seen.add((h, c))
    unique.append((h, c, fn, l))

print(f"\n=== {len(unique)} hits ===")
for h, c, fn, l in sorted(unique, key=lambda x: x[0]):
    print(f"  0x{h:08x}  {c:30s}  ({fn})")

out = ROOT / "docs/analysis/asset-identification/function_context_hits.csv"
with out.open("w", newline="") as f:
    w = csv.writer(f)
    w.writerow(["id_hex", "candidate_name", "context_function", "len"])
    for h, c, fn, l in sorted(unique, key=lambda x: x[0]):
        w.writerow([f"0x{h:08x}", c, fn, l])
print(f"\nwrote {out.relative_to(ROOT)}")
