#!/usr/bin/env python3
"""Analyse the naming STRUCTURE of a visually-confirmed sprite family.

Given a set of asset IDs that are the *same object in different states/directions*
(so the names share a stem and differ by one short token), this recovers:
  - the shared stem's calcHash (per-bit majority vote across the IDs)
  - each ID's residual = id ^ stemhash = rotl(h(variant_token), stem_end_shift)

If the family really is STEM+TOKEN, residuals come out tiny (low popcount). Then
each residual is a single (or few) toggled bit(s) -> the per-ID variant token.

This proved the FINN player directional family: stemhash=0x10b95810, the 9 cardinal
dirs = digits 1-9 clockwise from up (residual bit = K + val(digit), K=2), the 6
left dirs add a constant 0x0c marker. Recovering the literal stem STRING still
needs inverting calcHash of a ~12-char name (lossy), but the structure groups the
whole family and predicts sibling IDs.

Usage: python3 tools/scripts/family_structure.py 0x10b85810 0x10b91810 ...
"""
import sys

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

def rotr(v, r):
    r &= 31
    return ((v >> r) | (v << ((32 - r) & 31))) & 0xFFFFFFFF


def analyse(ids):
    n = len(ids)
    const = 0
    for b in range(32):
        if sum((i >> b) & 1 for i in ids) * 2 > n:
            const |= (1 << b)
    print(f"stem calcHash (majority vote) = 0x{const:08x}  popcount={bin(const).count('1')}")
    res = [(i, i ^ const) for i in ids]
    pops = sorted(bin(r).count('1') for _, r in res)
    print(f"residual popcounts: {pops}")
    print(f"\n{'id':>12}  {'residual':>10} popc  single-bit pos / token-hash")
    for i, r in res:
        pc = bin(r).count('1')
        note = ""
        if pc == 1:
            note = f"bit {r.bit_length()-1}"
        print(f"0x{i:08x}  0x{r:08x}   {pc:>2}  {note}")
    # interpret single-bit residuals as STEM+char with some end-shift K
    singles = [(i, r.bit_length() - 1) for i, r in res if bin(r).count('1') == 1]
    if len(singles) >= 3:
        positions = sorted(p for _, p in singles)
        span = positions[-1] - positions[0]
        print(f"\n{len(singles)} single-char variants span bit positions "
              f"{positions[0]}..{positions[-1]} (span {span}); "
              f"consecutive => a numeric/sequential token (digit or letter index).")


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("give >=2 hex ids"); sys.exit(1)
    analyse([int(x, 16) for x in sys.argv[1:]])
