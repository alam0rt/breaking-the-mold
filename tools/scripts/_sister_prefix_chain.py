"""Find sister-prefix relationships in the duckling family.

Hypothesis: 0x0c042100, 0x0c042102, 0x0c042103 (raw inverse-WRAP IDs) are sisters
where the names share a stem.

Specifically:
  P5 = preimages of 0x0c042100 at L=5 (the stem name)
  P7 = preimages of 0x0c042103 at L=7

If the L=7 names are stem+2 chars (extending P5), we can find them by enumerating
all 7-char preimages of 0x0c042103 whose first 5 chars hash to 0x0c042100.

Same for P6 = preimages of 0x0c042102 at L=6 (stem+1 char).
"""
import sys, subprocess
sys.path.insert(0, 'tools/scripts')
from calchash import calcHash

# Get all L=5 preimages of 0x0c042100 (the stem floor=5 target)
P5_TARGET = 0x0c042100
P6_TARGET = 0x0c042102
P7_TARGET = 0x0c042103

# Run kcrack to enumerate L=5 preimages
print('Enumerating L=5 preimages of 0x0c042100...')
result = subprocess.run(
    ['./tools/kcrack', 'extend', '', '/tmp/p5.txt', '--raw', '--suf', '5', '--overshoot', '0'],
    cwd='/home/sam/projects/btm',
    capture_output=True, text=True, timeout=60
)
# Need to write target file first
import os
with open('/tmp/p5.txt', 'w') as f:
    f.write(f'0x{P5_TARGET:08x}\n')

result = subprocess.run(
    ['./tools/kcrack', 'extend', '', '/tmp/p5.txt', '--raw', '--suf', '5', '--overshoot', '0'],
    cwd='/home/sam/projects/btm',
    capture_output=True, text=True, timeout=60
)
P5 = set()
for line in result.stdout.split('\n'):
    parts = line.split('\t')
    if len(parts) >= 2 and parts[0].startswith('0x'):
        P5.add(parts[1].strip())
print(f'  |P5| = {len(P5)}')

# L=6 preimages of 0x0c042102
with open('/tmp/p6.txt', 'w') as f:
    f.write(f'0x{P6_TARGET:08x}\n')
result = subprocess.run(
    ['./tools/kcrack', 'extend', '', '/tmp/p6.txt', '--raw', '--suf', '6', '--overshoot', '0'],
    cwd='/home/sam/projects/btm',
    capture_output=True, text=True, timeout=120
)
P6 = set()
for line in result.stdout.split('\n'):
    parts = line.split('\t')
    if len(parts) >= 2 and parts[0].startswith('0x'):
        P6.add(parts[1].strip())
print(f'  |P6| = {len(P6)}')

# L=7 preimages of 0x0c042103
with open('/tmp/p7.txt', 'w') as f:
    f.write(f'0x{P7_TARGET:08x}\n')
result = subprocess.run(
    ['./tools/kcrack', 'extend', '', '/tmp/p7.txt', '--raw', '--suf', '7', '--overshoot', '0'],
    cwd='/home/sam/projects/btm',
    capture_output=True, text=True, timeout=300
)
P7 = set()
for line in result.stdout.split('\n'):
    parts = line.split('\t')
    if len(parts) >= 2 and parts[0].startswith('0x'):
        P7.add(parts[1].strip())
print(f'  |P7| = {len(P7)}')
print()

# Find prefix relationships
print('=== L=6 names whose first 5 chars are a P5 stem ===')
matches_56 = []
for n6 in P6:
    if n6[:5] in P5:
        matches_56.append((n6[:5], n6))
print(f'  {len(matches_56)} matches')
for stem, full in matches_56[:30]:
    print(f'  stem={stem}  ext={full}')

print()
print('=== L=7 names whose first 5 chars are a P5 stem ===')
matches_57 = []
for n7 in P7:
    if n7[:5] in P5:
        matches_57.append((n7[:5], n7))
print(f'  {len(matches_57)} matches')
for stem, full in matches_57[:30]:
    print(f'  stem={stem}  ext={full}')

print()
print('=== L=7 names whose first 6 chars are a P6 (stem+1) ===')
matches_67 = []
for n7 in P7:
    if n7[:6] in P6:
        matches_67.append((n7[:6], n7))
print(f'  {len(matches_67)} matches')
for stem, full in matches_67[:30]:
    print(f'  stem={stem}  ext={full}')

print()
print('=== TRIPLE matches (5->6->7 prefix chain) ===')
triples = []
for n7 in P7:
    p5 = n7[:5]
    p6 = n7[:6]
    if p5 in P5 and p6 in P6:
        triples.append((p5, p6, n7))
print(f'  {len(triples)} triples')
for p5, p6, n7 in triples[:30]:
    print(f'  {p5} -> {p6} -> {n7}')
