"""SAT/SMT encoding of the pickup-PREFIX hash via Z3 BitVecs.

Faster encoding: shifts are BitVec(5), hash is BitVec(32), XOR is native.
Each char index picks a step value from a small lookup table.

The key trick: for the per-char hash contribution `1 << T_i`, we encode it as
`BitVecVal(1, 32) << ZeroExt(27, T_i)`. Z3's bit-blaster handles this well.

Usage:
  python _prefix_smt2.py 8                       # find any L=8 preimage
  python _prefix_smt2.py 10 --contains PICKUP    # L=10 containing PICKUP
  python _prefix_smt2.py 8  --all --max 50       # enumerate up to 50 distinct
  python _prefix_smt2.py 8  --letters-only --contains OBJ
"""
import argparse
import sys
import time

from z3 import (BitVec, BitVecVal, Solver, And, Or, sat, unsat, ZeroExt,
                Extract, LShR, ULT, ULE, set_param)

ALPHABET = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789'  # 36
STEP = [0] * 36
for d, c in enumerate(ALPHABET):
    o = ord(c)
    if c.isdigit():
        o += 22
    STEP[d] = (o - 64) & 31

TARGET_H = 0x88200080
TARGET_SH = 27


def calc(s):
    h, sh = 0, 0
    for c in s.upper():
        i = ALPHABET.find(c)
        if i < 0:
            continue
        sh = (sh + STEP[i]) & 31
        h ^= (1 << sh)
    return h & 0xFFFFFFFF, sh


def build_solver(L, letters_only=False, contains=None, starts_with=None,
                 ends_with=None):
    s = Solver()
    set_param('parallel.enable', True)
    set_param('parallel.threads.max', 12)

    # Each char is a BitVec(7) holding the alphabet index 0..35 (need 7 bits to
    # avoid signed-comparison surprises at BV6).
    chars = [BitVec(f'c_{i}', 7) for i in range(L)]
    n_alpha = 26 if letters_only else 36
    for ci in chars:
        s.add(ULT(ci, n_alpha))

    # Step per char: si = STEP[ci]. Encode via Or over (char_value, step_value).
    steps = []
    for i, ci in enumerate(chars):
        si = BitVec(f's_{i}', 5)  # step in 1..26, fits in BV5 (max 31)
        cases = Or([And(ci == BitVecVal(k, 7), si == BitVecVal(STEP[k], 5))
                    for k in range(n_alpha)])
        s.add(cases)
        steps.append(si)

    # Cumulative shifts T_i as BitVec(5) — natural mod 32 arithmetic
    shifts = []
    running = BitVecVal(0, 5)
    for si in steps:
        running = running + si  # BV5 arithmetic auto-mods 32
        T = BitVec(f'T_{len(shifts)}', 5)
        s.add(T == running)
        shifts.append(T)

    # Final shift
    s.add(shifts[-1] == TARGET_SH)

    # Hash: XOR over i of (BitVec(1<<T_i, 32))
    # Express 1<<T as BV: (BitVecVal(1, 32) << ZeroExt(27, T))
    one32 = BitVecVal(1, 32)
    contributions = [one32 << ZeroExt(27, T) for T in shifts]
    total_hash = contributions[0]
    for c in contributions[1:]:
        total_hash = total_hash ^ c
    s.add(total_hash == BitVecVal(TARGET_H, 32))

    # Substring contains
    if contains:
        m = len(contains)
        if m > L:
            s.add(False)
        else:
            disj = []
            for i in range(L - m + 1):
                conj = []
                for j, c in enumerate(contains.upper()):
                    idx = ALPHABET.find(c)
                    if idx < 0:
                        conj.append(False)
                    else:
                        conj.append(chars[i + j] == idx)
                disj.append(And(conj))
            s.add(Or(disj))

    if starts_with:
        for j, c in enumerate(starts_with.upper()):
            idx = ALPHABET.find(c)
            if idx >= 0 and j < L:
                s.add(chars[j] == idx)

    if ends_with:
        m = len(ends_with)
        for j, c in enumerate(ends_with.upper()):
            idx = ALPHABET.find(c)
            if idx >= 0:
                s.add(chars[L - m + j] == idx)

    return s, chars


def extract_string(model, chars):
    return ''.join(ALPHABET[model.eval(c).as_long()] for c in chars)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('L', type=int)
    ap.add_argument('--contains', default=None)
    ap.add_argument('--starts-with', default=None)
    ap.add_argument('--ends-with', default=None)
    ap.add_argument('--letters-only', action='store_true')
    ap.add_argument('--all', action='store_true')
    ap.add_argument('--max', type=int, default=20)
    ap.add_argument('--timeout', type=int, default=120)
    args = ap.parse_args()

    s, chars = build_solver(args.L,
                             letters_only=args.letters_only,
                             contains=args.contains,
                             starts_with=args.starts_with,
                             ends_with=args.ends_with)
    s.set('timeout', args.timeout * 1000)

    print(f'Target: h=0x{TARGET_H:08x} sh={TARGET_SH}, L={args.L}', file=sys.stderr)
    if args.contains: print(f'Constraint: contains {args.contains!r}', file=sys.stderr)
    if args.starts_with: print(f'Constraint: starts with {args.starts_with!r}', file=sys.stderr)
    if args.ends_with: print(f'Constraint: ends with {args.ends_with!r}', file=sys.stderr)
    if args.letters_only: print('Constraint: letters only', file=sys.stderr)

    found = []
    t0 = time.time()
    for i in range(args.max if args.all else 1):
        r = s.check()
        if r == unsat:
            print(f'unsat after {i} solutions, {time.time()-t0:.1f}s', file=sys.stderr)
            break
        if r != sat:
            print(f'unknown ({r}) after {i} solutions, {time.time()-t0:.1f}s', file=sys.stderr)
            break
        m = s.model()
        sol = extract_string(m, chars)
        h, sh = calc(sol)
        assert h == TARGET_H and sh == TARGET_SH, f'BAD: {sol} -> 0x{h:08x} sh={sh}'
        found.append(sol)
        print(f'  [{i+1}] {sol}')
        if not args.all:
            break
        s.add(Or([chars[j] != m.eval(chars[j]).as_long() for j in range(args.L)]))

    print(f'\nFound {len(found)} solution(s) in {time.time()-t0:.1f}s')


if __name__ == '__main__':
    main()
