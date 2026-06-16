#!/usr/bin/env python3
"""Focused 2-token MITM attack against the 8 role-annotated target IDs.

Tokens come from BLB level codes + game vocabulary + role-specific extras.
Output as CSV with provenance to add to the cracked_names.csv ledger.
"""
import csv
from pathlib import Path
import sys
sys.path.insert(0, "tools/scripts")
from compound_hash_attack import calc_hash_and_shift, rotl, rotr

SEED = 0x28C0E011
OUT_ROT = 27

def asset_id(name): h, _ = calc_hash_and_shift(name); return SEED ^ rotl(h, OUT_ROT)

# Verified anchors
KNOWN = {0x29C0E211, 0x2AD0F011, 0x0AD0F813, 0x68C0F413, 0x69C04050, 0x69C8F473}

# Role-annotated targets from manifest.js
TARGETS = {
    0x00e2f188: ("Number glyph 0-9", "MENU"),
    0x33808e1b: ("Menu cursor anim frame", "MENU"),
    0x4a3ea110: ("Menu door reveal monkey", "MENU"),
    0x4aee0118: ("Shriney Guard slamming down", "MEGA"),
    0x4baf8200: ("Shriney Guard yelling", "MEGA"),
    0x61981a0c: ("Plain monkey enemy", "MEGA"),
    0x63848e59: ("Menu cursor", "MENU"),
    0x75189608: ("Plain monkey enemy variant", "MEGA"),
}

# Tokens (large pool to maximize hit chance)
LEVELS = ["MENU", "MEGA", "WIZZ", "EVIL", "KLOG", "BOIL", "BRG1", "CSTL",
          "EGGS", "FOOD", "GLEN", "HEAD", "RUNN", "SCIE", "TMPL", "WEED"]

ROLES = ["CURSOR", "POINTER", "ARROW", "HILIGHT", "HIGHLIGHT", "ACTIVE",
         "IDLE", "LOOP", "ANIM", "FRAME", "CYCLE",
         "BOSS", "ENEMY", "MONKEY", "MONK", "PLAYER",
         "DOOR", "OPEN", "REVEAL", "INTRO", "OUTRO", "END",
         "SCORE", "TIME", "LIVES", "HUD", "STATUS",
         "NUMBER", "NUMBERS", "DIGIT", "DIGITS", "GLYPH", "FONT", "CHAR",
         "TITLE", "LOGO", "FLASH", "BLINK", "SHINE",
         "YELL", "SHOUT", "SLAM", "HIT", "CRUSH", "POUND", "SMASH",
         "ROAR", "SCREAM", "ANGRY", "RAGE", "SCARED", "TAUNT",
         "WALK", "RUN", "JUMP", "FALL", "STAND",
         "DEAD", "DIE", "DEATH", "HURT",
         "BG", "FG", "BACKGROUND", "FOREGROUND",
         "SHRINEY", "GUARD", "ROYAL", "STATUE", "TEMPLE",
         "BIG", "SMALL", "TALL", "SHORT",
         "BANK", "SHEET", "MAIN", "PRIMARY", "SECONDARY", "COMMON", "SHARED",
         "SPRT", "SPR", "SEQ", "TILE", "XTILE", "FNT", "SND", "MUS",
         "SPRITE", "SOUND", "MUSIC", "ICON", "EFFECT",
         "INIT", "INTRO", "OUTRO", "PLAY", "PAUSE",
         ]

EXTRA_NUMBERS = [str(i) for i in range(20)] + ["00", "01", "02", "03", "04", "05",
                                                  "10", "20", "30", "40", "50"]

# Build token pool
TOKENS = list(set(LEVELS + ROLES))
print(f"{len(TOKENS)} tokens, {len(EXTRA_NUMBERS)} numbers")

# Precompute hashes for all tokens
A = []
for t in TOKENS:
    h, sh = calc_hash_and_shift(t)
    A.append((t, h, sh))

# Build B-index for second token
b_by_h = {}
for t, h, sh in A:
    b_by_h.setdefault(h, []).append((t, sh))

# Also include numbers
for n in EXTRA_NUMBERS:
    h, sh = calc_hash_and_shift(n)
    b_by_h.setdefault(h, []).append((n, sh))

print(f"B-index: {len(b_by_h)} unique hashes")

# Attack each target
hits_by_id = {tid: [] for tid in TARGETS}
for tid, (role, level) in TARGETS.items():
    target_h = rotr(tid ^ SEED, OUT_ROT)
    
    # Single token
    for a, h_A, sh_A in A:
        if h_A == target_h:
            hits_by_id[tid].append(("1tok", a, ""))
    
    # 2-token MITM (A, then B)
    for a, h_A, sh_A in A:
        req_h_B = rotr(target_h ^ h_A, sh_A)
        if req_h_B in b_by_h:
            for b, _ in b_by_h[req_h_B]:
                # Verify
                full = a + "_" + b
                if asset_id(full) == tid:
                    hits_by_id[tid].append(("2tok", a, b))

# Print results
print("\n=== HITS PER TARGET ===")
for tid, (role, level) in TARGETS.items():
    print(f"\n0x{tid:08x} [{level}] {role}:")
    hits = hits_by_id[tid]
    if not hits:
        print("  (no hits)")
        continue
    for kind, a, b in hits[:30]:
        if b:
            print(f"  {kind}: {a}_{b}")
        else:
            print(f"  {kind}: {a}")

# 3-token MITM with single base from levels
print("\n=== 3-token MITM (base + middle + end) ===")
ALPHA_NUM = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
short_tokens = [t for t, h, sh in A]  # just the strings

# For each target, iterate 2 priors (a, b) and compute required c hash
# That's len(A)^2 ≈ 250K iterations × len(short_tokens) lookups = 60M, slow
# Instead, build a map of (a + b) -> hash, then lookup for each c
print(f"  building (a, b) → hash map from {len(A)} × {len(A)} pairs...")
ab_by_h = {}
for a, h_A, sh_A in A:
    for b, h_B, sh_B in A:
        h_ab = h_A ^ rotl(h_B, sh_A)
        sh_ab = (sh_A + sh_B) & 31
        ab_by_h.setdefault(h_ab, []).append((a, b, sh_ab))
print(f"  {len(ab_by_h)} unique hashes from {sum(len(v) for v in ab_by_h.values()):,} pairs")

# Now iterate target × (c is each token) -> check (a, b) exists
for tid, (role, level) in TARGETS.items():
    target_h = rotr(tid ^ SEED, OUT_ROT)
    for c, h_C, sh_C in A:
        # need: h_AB ^ rotl(h_C, sh_AB) = target_h
        # So: h_AB = target_h ^ rotl(h_C, sh_AB), and sh_AB depends on AB, not C.
        # We don't know sh_AB in advance, but we have a map by h_AB.
        # Instead: for each (h_AB, list of (a, b, sh_ab)), check target_h ^ rotl(h_C, sh_ab) == h_AB
        # That's 13K iterations per target per C — slow.
        # Alternative: precompute a map indexed by (h_AB, sh_ab) pairs and look up
        # But we have ~700 tokens and 250K (a,b) pairs, so 32 sh values × 700 = 22400 lookups per c.
        # Better to just iterate ab_by_h once per target and per c.
        for h_ab, abs_list in ab_by_h.items():
            for a, b, sh_ab in abs_list:
                if h_ab ^ rotl(h_C, sh_ab) == target_h:
                    full = a + "_" + b + "_" + c
                    if asset_id(full) == tid:
                        hits_by_id[tid].append(("3tok", a, b + "_" + c))

print("\n=== ALL HITS (incl 3tok) PER TARGET ===")
for tid, (role, level) in TARGETS.items():
    print(f"\n0x{tid:08x} [{level}] {role}:")
    hits = hits_by_id[tid]
    if not hits:
        print("  (no hits)")
        continue
    seen = set()
    for kind, a, b in hits:
        full = (a + "_" + b) if b else a
        if full in seen: continue
        seen.add(full)
        print(f"  {kind}: {full}")
