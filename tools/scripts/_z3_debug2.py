"""Minimal-as-possible Z3 encoding to isolate the bug."""
from z3 import (BitVec, BitVecVal, Solver, And, Or, sat, unsat, ZeroExt, Extract)

# Try a totally direct encoding for 1XV0
# steps = [7, 24, 22, 6]
# T = [7, 31, 21, 27]
# hash = (1<<7) | (1<<31) | (1<<21) | (1<<27) = 0x88200080

# Encode with concrete steps and just check Z3's BV shift semantics
s = Solver()
T1 = BitVecVal(7, 5)
T2 = BitVecVal(31, 5)
T3 = BitVecVal(21, 5)
T4 = BitVecVal(27, 5)
one32 = BitVecVal(1, 32)
b1 = one32 << ZeroExt(27, T1)
b2 = one32 << ZeroExt(27, T2)
b3 = one32 << ZeroExt(27, T3)
b4 = one32 << ZeroExt(27, T4)
total = b1 ^ b2 ^ b3 ^ b4
s.add(total == BitVecVal(0x88200080, 32))
r = s.check()
print(f'Concrete shift test: {r}')

# Make a model and check each bit
s2 = Solver()
T_var = BitVec('T', 5)
s2.add(T_var == 7)
shifted = BitVecVal(1, 32) << ZeroExt(27, T_var)
target = BitVecVal(0x80, 32)
s2.add(shifted == target)
r = s2.check()
print(f'1<<7 test: {r}')

# Now try the chain accumulation
s3 = Solver()
si_vals = [7, 24, 22, 6]
running = BitVecVal(0, 5)
shifts = []
for v in si_vals:
    running = running + BitVecVal(v, 5)
    shifts.append(running)
contributions = [BitVecVal(1, 32) << ZeroExt(27, T) for T in shifts]
total_hash = contributions[0]
for c in contributions[1:]:
    total_hash = total_hash ^ c
s3.add(total_hash == BitVecVal(0x88200080, 32))
# also check final shift
s3.add(shifts[-1] == 27)
r = s3.check()
print(f'Accumulated chain test: {r}')
if r == sat:
    m = s3.model()
    for i, T in enumerate(shifts):
        print(f'  T_{i+1} = {m.eval(T).as_long()}')

# Verify with explicit BV evaluation by checking what `running` value Z3 thinks each is
s4 = Solver()
running = BitVecVal(0, 5)
for i, v in enumerate(si_vals):
    running = running + BitVecVal(v, 5)
    T = BitVec(f'T_{i+1}', 5)
    s4.add(T == running)
expected_T = [7, 31, 21, 27]
for i in range(4):
    name = f'T_{i+1}'
    found_eq = s4.check(BitVec(name, 5) == expected_T[i])
    print(f'  Expecting {name}={expected_T[i]}: {found_eq}')
