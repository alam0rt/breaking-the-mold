"""For each L=5 preimage of 0x0c042100, find which of the 36 possible single-char
extensions hashes to 0x0c042102. This identifies real stem-extension chains.

If we find ONE clean English-looking 5-char stem with valid extensions to BOTH
the L=6 and L=7 sister hashes, that's strong signal it's the real name.
"""
import sys, subprocess, os
sys.path.insert(0, 'tools/scripts')
from calchash import calcHash

CS = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
STEP = [0]*36
for d, c in enumerate(CS):
    o = ord(c)
    if c.isdigit():
        o += 22
    STEP[d] = (o - 64) & 31

def hash_with_state(s):
    """Like calcHash but returns (hash, sh) so we can extend."""
    h, sh = 0, 0
    for c in s.upper():
        if c in CS:
            d = CS.index(c)
            sh = (sh + STEP[d]) & 31
            h ^= 1 << sh
    return h, sh

def extend_one_char(s, c):
    """Add one char to s and return new hash (and new sh)."""
    h, sh = hash_with_state(s)
    if c in CS:
        d = CS.index(c)
        sh = (sh + STEP[d]) & 31
        h ^= 1 << sh
    return h, sh

# Get L=5 preimages of duckling target 0x0c042100
P5 = []
with open('/tmp/p5.txt', 'w') as f:
    f.write('0x0c042100\n')
res = subprocess.run(['./tools/kcrack', 'extend', '', '/tmp/p5.txt', '--raw',
                      '--suf', '5', '--overshoot', '0'],
                     cwd='/home/sam/projects/btm', capture_output=True, text=True, timeout=120)
for line in res.stdout.split('\n'):
    parts = line.split('\t')
    if len(parts) >= 2 and parts[0].startswith('0x'):
        P5.append(parts[1].strip())
print(f'P5 (L=5 of 0x0c042100): {len(P5)} candidates')

# Get L=6 preimages of 0x0c042102 and L=7 of 0x0c042103 for the sisters
P6_EXACT = set()
with open('/tmp/p6_excl.txt', 'w') as f:
    f.write('0x0c042102\n')
res = subprocess.run(['./tools/kcrack', 'extend', '', '/tmp/p6_excl.txt', '--raw',
                      '--suf', '6', '--overshoot', '0'],
                     cwd='/home/sam/projects/btm', capture_output=True, text=True, timeout=300)
for line in res.stdout.split('\n'):
    parts = line.split('\t')
    if len(parts) >= 2 and parts[0].startswith('0x'):
        P6_EXACT.add(parts[1].strip())
print(f'P6 (L=6 of 0x0c042102): {len(P6_EXACT)} candidates')

P7_EXACT = set()
with open('/tmp/p7_excl.txt', 'w') as f:
    f.write('0x0c042103\n')
res = subprocess.run(['./tools/kcrack', 'extend', '', '/tmp/p7_excl.txt', '--raw',
                      '--suf', '7', '--overshoot', '0'],
                     cwd='/home/sam/projects/btm', capture_output=True, text=True, timeout=600)
for line in res.stdout.split('\n'):
    parts = line.split('\t')
    if len(parts) >= 2 and parts[0].startswith('0x'):
        P7_EXACT.add(parts[1].strip())
print(f'P7 (L=7 of 0x0c042103): {len(P7_EXACT)} candidates')
print()

# For each P5 stem, find single-char extensions that hit P6_EXACT or P7_EXACT
print('=== Checking 1-char extensions of L=5 stems ===')
extending_56 = []  # stems extending to L=6 sister
extending_57 = []  # stems whose L=6 ext extending to L=7 sister
for stem in P5:
    for c in CS:
        h6, _ = extend_one_char(stem, c)
        if h6 == 0x0c042102 and (stem + c) in P6_EXACT:
            # found stem+c -> sister L=6
            extending_56.append((stem, stem + c))
            # and check 2-char extension
            for c2 in CS:
                h7, _ = extend_one_char(stem + c, c2)
                if h7 == 0x0c042103 and (stem + c + c2) in P7_EXACT:
                    extending_57.append((stem, stem + c, stem + c + c2))

print(f'5->6 chains: {len(extending_56)}')
for stem, ext6 in extending_56[:30]:
    print(f'  {stem} + {ext6[5:]} = {ext6}')
print()
print(f'5->6->7 chains: {len(extending_57)}')
for stem, ext6, ext7 in extending_57[:30]:
    print(f'  {stem} -> {ext6} -> {ext7}')
