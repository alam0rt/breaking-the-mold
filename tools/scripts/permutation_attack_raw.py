#!/usr/bin/env python3
"""Permutation attack on raw-namespace ids whose popcount equals length.

For an id with popcount P and a name of length L=P (no cancellations), the
hash sets bits at sh_1, sh_2, ..., sh_P where sh_i = sum of STEP(c_j) for j<=i
mod 32. So the sequence of sh values is some permutation of the bits set in id,
and consecutive deltas (sh_i - sh_{i-1}) mod 32 give STEP values.

For each ordering of the P bits, derive the P STEP deltas. If all deltas are
in {1..26} (the valid STEP range), we have a valid candidate; for each STEP,
we know which chars produce it.

Output: every candidate string (alpha-numeric only, no separators) for
each target id. Pipe to klash_match_raw or filter for readability.
"""
import sys
from itertools import permutations

# STEP value -> list of chars
STEP_TO_CHARS = {}
for c in 'ABCDEFGHIJKLMNOPQRSTUVWXYZ':
    s = (ord(c) - 64) & 31
    STEP_TO_CHARS.setdefault(s, []).append(c)
for d in '0123456789':
    s = ((ord(d) + 22) - 64) & 31
    STEP_TO_CHARS.setdefault(s, []).append(d)

# Sanity: STEP values 1..26 should each have at least 1 char
# Digits 0-9 -> step (54-64)&31 = (-10)&31 = 22? Let me check.
# ord('0')=48, +22=70, -64=6. So STEP=6 maps to F or '0'.
# ord('9')=57, +22=79, -64=15. So STEP=15 maps to O or '9'.
# So digits cover STEP 6..15, letters cover STEP 1..26.

def bits(v):
    out = []
    for i in range(32):
        if v & (1 << i):
            out.append(i)
    return out

def candidates_for_id(target):
    bits_set = bits(target)
    L = len(bits_set)
    if L > 11:
        return  # 11! = 40M, slow
    seen = set()
    for perm in permutations(bits_set):
        # First STEP from sh=0 to perm[0]
        sh_prev = 0
        steps = []
        for sh in perm:
            d = (sh - sh_prev) & 31
            if d == 0 or d > 26:
                break
            if d not in STEP_TO_CHARS:
                break
            steps.append(d)
            sh_prev = sh
        if len(steps) != L:
            continue
        # Generate all char combinations
        char_lists = [STEP_TO_CHARS[s] for s in steps]
        # Up to 2^L combinations
        n = 1
        for cl in char_lists:
            n *= len(cl)
        if n > 4096:
            continue  # skip pathological orderings
        from itertools import product
        for combo in product(*char_lists):
            s = ''.join(combo)
            if s not in seen:
                seen.add(s)
                yield s

if __name__ == '__main__':
    targets = []
    for line in sys.stdin:
        line = line.strip()
        if not line: continue
        targets.append(int(line, 16))

    total = 0
    for t in targets:
        n_per = 0
        for cand in candidates_for_id(t):
            print(cand)
            n_per += 1
            total += 1
        print(f"# 0x{t:08x}: {n_per} candidates", file=sys.stderr)
    print(f"# total: {total}", file=sys.stderr)
