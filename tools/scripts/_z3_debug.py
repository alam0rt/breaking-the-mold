"""Debug Z3 encoding by hardcoding a known solution and checking."""
from z3 import (BitVec, BitVecVal, Solver, And, Or, sat, unsat, ZeroExt)

ALPHABET = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789'
STEP = [0] * 36
for d, c in enumerate(ALPHABET):
    o = ord(c)
    if c.isdigit():
        o += 22
    STEP[d] = (o - 64) & 31

print('STEP:', STEP)
print()

TARGET_H = 0x88200080
TARGET_SH = 27

# Hardcoded solution: "1XV0"
known = '1XV0'
indices = [ALPHABET.find(c) for c in known]
print(f'Known {known}: indices={indices}, steps={[STEP[i] for i in indices]}')

# Build minimal Z3 model
L = 4
s = Solver()
chars = [BitVec(f'c_{i}', 6) for i in range(L)]

# FIX char values to the known solution and see if we get sat
for j, idx in enumerate(indices):
    s.add(chars[j] == idx)

# Constrain chars < 36
for ci in chars:
    s.add(ci < 36)

# Now build the rest of the encoding
steps = []
for i, ci in enumerate(chars):
    si = BitVec(f's_{i}', 5)
    cases = Or([And(ci == k, si == STEP[k]) for k in range(36)])
    s.add(cases)
    steps.append(si)

shifts = []
running = BitVecVal(0, 5)
for si in steps:
    running = running + si
    T = BitVec(f'T_{len(shifts)}', 5)
    s.add(T == running)
    shifts.append(T)

# Final shift constraint
s.add(shifts[-1] == TARGET_SH)

# Hash constraint
one32 = BitVecVal(1, 32)
contributions = [one32 << ZeroExt(27, T) for T in shifts]
total_hash = contributions[0]
for c in contributions[1:]:
    total_hash = total_hash ^ c
s.add(total_hash == BitVecVal(TARGET_H, 32))

r = s.check()
print(f'\nWith hardcoded {known!r}: {r}')
if r == sat:
    m = s.model()
    print('  steps:', [m.eval(steps[i]).as_long() for i in range(L)])
    print('  shifts:', [m.eval(shifts[i]).as_long() for i in range(L)])
    print('  total hash:', f'0x{m.eval(total_hash).as_long():08x}')
elif r == unsat:
    # Try to get unsat core
    s2 = Solver()
    s2.set(unsat_core=True)
    # Re-build with named constraints to find which one fails
    print('  Investigating which constraint fails:')
    # Check final shift on its own
    sh_check = sum(STEP[i] for i in indices) % 32
    print(f'    Expected T_3 (cumulative): {sh_check}')
    # Compute hash manually
    h = 0
    sh = 0
    for i in indices:
        sh = (sh + STEP[i]) & 31
        h ^= 1 << sh
    print(f'    Expected h: 0x{h:08x}')
