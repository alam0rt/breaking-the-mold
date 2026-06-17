"""Test the char-to-step mapping isolation."""
from z3 import (BitVec, BitVecVal, Solver, And, Or, sat, unsat, ZeroExt)

ALPHABET = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789'
STEP = [0] * 36
for d, c in enumerate(ALPHABET):
    o = ord(c)
    if c.isdigit():
        o += 22
    STEP[d] = (o - 64) & 31

# Test 1: Just the char->step encoding for one char
print('=== Test 1: forced ci=27 should give si=7 ===')
s = Solver()
ci = BitVec('c', 6)
si = BitVec('s', 5)
s.add(ci == 27)
s.add(ci < 36)
cases = Or([And(ci == k, si == STEP[k]) for k in range(36)])
s.add(cases)
r = s.check()
print(f'check: {r}')
if r == sat:
    m = s.model()
    print(f'  c={m.eval(ci).as_long()}, s={m.eval(si).as_long()} (expected 7)')

# Test 2: same as above but with bigger int
print('\n=== Test 2: same with ci=35 (max), should give si=15 ===')
s = Solver()
ci = BitVec('c', 6)
si = BitVec('s', 5)
s.add(ci == 35)
s.add(ci < 36)
cases = Or([And(ci == k, si == STEP[k]) for k in range(36)])
s.add(cases)
r = s.check()
print(f'check: {r}')
if r == sat:
    m = s.model()
    print(f'  c={m.eval(ci).as_long()}, s={m.eval(si).as_long()} (expected 15)')

# Test 3: ci unconstrained except < 36, look for matching step=7
print('\n=== Test 3: find ci where si=7 (should find ci=6, "G", or ci=27, "1") ===')
s = Solver()
ci = BitVec('c', 6)
si = BitVec('s', 5)
s.add(ci < 36)
cases = Or([And(ci == k, si == STEP[k]) for k in range(36)])
s.add(cases)
s.add(si == 7)
r = s.check()
print(f'check: {r}')
if r == sat:
    m = s.model()
    print(f'  c={m.eval(ci).as_long()} (one of 6 [G] or 27 [1])')
