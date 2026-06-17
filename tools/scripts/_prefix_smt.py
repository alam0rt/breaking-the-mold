"""SAT/SMT encoding of the pickup-PREFIX hash constraint via Z3.

The hash function is linear over GF(2):
  - Each char picks a step value s_i in {1..26} (some shared across alnum)
  - Cumulative shift T_i = (s_1+...+s_i) mod 32
  - Hash = XOR_i (1 << T_i)
  - Final shift = T_n

For Z3:
  - Variables: s_i (Int, 1..26), encoded as char_i (Int, 0..35)
  - Constraint: T_n = TARGET_SH
  - Constraint: each bit of the final XOR equals corresponding TARGET_H bit
  - Optional: substring constraints (e.g., must contain "PICKUP")

This lets us add semantic constraints (substring, char-set, prefix/suffix) that
would be impractical to express with pure meet-in-the-middle filtering.

Usage:
  python _prefix_smt.py 8                     # find any L=8 preimage
  python _prefix_smt.py 10 --contains PICKUP  # L=10 containing PICKUP
  python _prefix_smt.py 9  --contains OBJ
  python _prefix_smt.py 8  --all --max 50     # enumerate up to 50 distinct
  python _prefix_smt.py 8  --letters-only
"""
import argparse
import sys

try:
    from z3 import (Int, IntVal, BitVec, BitVecVal, Solver, And, Or, Implies,
                    Sum, If, sat, unsat, set_param)
except ImportError:
    print('Z3 not installed. Run: pip install z3-solver', file=sys.stderr)
    sys.exit(1)

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

    chars = [Int(f'c_{i}') for i in range(L)]
    char_range = list(range(26 if letters_only else 36))
    for ci in chars:
        s.add(Or([ci == k for k in char_range]))

    # For each char position, the step value: lookup from STEP table.
    # Encode as nested If: step_i = If(c_i == 0, STEP[0], If(c_i == 1, ...))
    def step_expr(ci):
        # binary tree of If for efficiency
        return _build_if_tree(ci, char_range)

    # cumulative shifts T_i in 0..31
    shifts = []
    running = IntVal(0)
    for ci in chars:
        running = running + step_expr(ci)
        # We need running mod 32; introduce an Int variable
        T = Int(f'T_{len(shifts)}')
        s.add(T >= 0, T < 32)
        # T = running mod 32: introduce quotient
        Q = Int(f'Q_{len(shifts)}')
        s.add(Q >= 0)
        s.add(running == 32 * Q + T)
        shifts.append(T)

    # Final shift constraint
    s.add(shifts[-1] == TARGET_SH)

    # Hash constraint: for each bit k in 0..31, parity of (T_i == k) must
    # equal corresponding bit of TARGET_H.
    for k in range(32):
        cnt = Sum([If(T == k, 1, 0) for T in shifts])
        target_bit = (TARGET_H >> k) & 1
        # parity: cnt mod 2 == target_bit
        Pmod = Int(f'P_{k}')
        Pq = Int(f'Pq_{k}')
        s.add(Pmod == target_bit, Pq >= 0, cnt == 2 * Pq + Pmod)

    # Substring constraints: contains "STR" means exists i: chars[i..i+m] = STR
    if contains:
        m = len(contains)
        if m > L:
            s.add(False)  # impossible
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


def _build_if_tree(ci, values):
    """Build nested If expressing STEP[ci] for ci in values."""
    if len(values) == 1:
        return IntVal(STEP[values[0]])
    mid = len(values) // 2
    left, right = values[:mid], values[mid:]
    return If(ci < right[0],
              _build_if_tree(ci, left),
              _build_if_tree(ci, right))


def extract_string(model, chars):
    return ''.join(ALPHABET[model.eval(ci).as_long()] for ci in chars)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('L', type=int)
    ap.add_argument('--contains', default=None)
    ap.add_argument('--starts-with', default=None)
    ap.add_argument('--ends-with', default=None)
    ap.add_argument('--letters-only', action='store_true')
    ap.add_argument('--all', action='store_true', help='Enumerate all (up to --max)')
    ap.add_argument('--max', type=int, default=20)
    ap.add_argument('--timeout', type=int, default=60, help='Per-query timeout (s)')
    args = ap.parse_args()

    s, chars = build_solver(args.L,
                             letters_only=args.letters_only,
                             contains=args.contains,
                             starts_with=args.starts_with,
                             ends_with=args.ends_with)
    s.set('timeout', args.timeout * 1000)

    print(f'Target: h=0x{TARGET_H:08x} sh={TARGET_SH}, L={args.L}', file=sys.stderr)
    if args.contains:
        print(f'Constraint: contains {args.contains!r}', file=sys.stderr)
    if args.starts_with:
        print(f'Constraint: starts with {args.starts_with!r}', file=sys.stderr)
    if args.ends_with:
        print(f'Constraint: ends with {args.ends_with!r}', file=sys.stderr)
    if args.letters_only:
        print('Constraint: letters only', file=sys.stderr)

    found = []
    for i in range(args.max if args.all else 1):
        r = s.check()
        if r == unsat:
            print('unsat', file=sys.stderr)
            break
        if r != sat:
            print(f'unknown ({r})', file=sys.stderr)
            break
        m = s.model()
        sol = extract_string(m, chars)
        h, sh = calc(sol)
        assert h == TARGET_H and sh == TARGET_SH, f'BAD: {sol} -> 0x{h:08x} sh={sh}'
        found.append(sol)
        print(f'  [{i+1}] {sol}')
        if not args.all:
            break
        # Block this exact solution
        s.add(Or([chars[j] != m.eval(chars[j]).as_long() for j in range(args.L)]))

    print(f'\nFound {len(found)} solution(s)')


if __name__ == '__main__':
    main()
