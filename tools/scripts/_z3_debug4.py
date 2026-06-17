"""Try If-based encoding instead of Or-And."""
from z3 import (BitVec, BitVecVal, Solver, And, Or, sat, unsat, Implies, If)

ALPHABET = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789'
STEP = [0] * 36
for d, c in enumerate(ALPHABET):
    o = ord(c)
    if c.isdigit():
        o += 22
    STEP[d] = (o - 64) & 31

def if_tree(ci, values):
    if len(values) == 1:
        return BitVecVal(STEP[values[0]], 5)
    mid = len(values) // 2
    return If(ci < values[mid], if_tree(ci, values[:mid]), if_tree(ci, values[mid:]))

# Test: ci=27 -> si=STEP[27]=7
print('=== If-tree encoding ===')
s = Solver()
ci = BitVec('c', 6)
si = BitVec('s', 5)
s.add(ci == 27)
s.add(ci < 36)
s.add(si == if_tree(ci, list(range(36))))
r = s.check()
print(f'  ci=27 expecting si=7: {r}')
if r == sat:
    m = s.model()
    print(f'  got s={m.eval(si).as_long()}')

# Test: same with Implies-based encoding
print('\n=== Implies encoding ===')
s = Solver()
ci = BitVec('c', 6)
si = BitVec('s', 5)
s.add(ci == 27)
s.add(ci < 36)
for k in range(36):
    s.add(Implies(ci == k, si == STEP[k]))
r = s.check()
print(f'  ci=27 expecting si=7: {r}')
if r == sat:
    m = s.model()
    print(f'  got s={m.eval(si).as_long()}')

# Now check: maybe my original was just wrong — let me re-verify Or-And in isolation
print('\n=== Or-And encoding (with explicit comparison) ===')
s = Solver()
ci = BitVec('c', 6)
si = BitVec('s', 5)
s.add(ci == BitVecVal(27, 6))  # explicit BV val
s.add(ci < 36)
clauses = []
for k in range(36):
    clauses.append(And(ci == BitVecVal(k, 6), si == BitVecVal(STEP[k], 5)))
s.add(Or(clauses))
r = s.check()
print(f'  ci=27 expecting si=7: {r}')
if r == sat:
    m = s.model()
    print(f'  got s={m.eval(si).as_long()}')
