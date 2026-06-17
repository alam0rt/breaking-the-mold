"""Find cracked-sister-pair examples to understand naming conventions.

Look for pairs of cracked names whose IDs differ by small XOR (1-3 bits)
to understand how related names look.
"""
import csv
import sys; sys.path.insert(0, 'tools/scripts')
from calchash import calcHash

cracked = {}
with open('docs/analysis/asset-identification/cracked_names.csv') as f:
    for r in csv.DictReader(f):
        if r['status'] == 'uncracked':
            continue
        if not r['id_hex'].startswith('0x'):
            continue
        if not r['name']:
            continue
        idv = int(r['id_hex'], 16)
        cracked[idv] = (r['name'], r['type'], int(r['floor']), r['levels'])

# Find pairs with small XOR distance
print(f'Loaded {len(cracked)} cracked names')
print()

# Find pairs by RAW XOR distance
def rotr(x, n): return ((x>>n)|(x<<(32-n)))&0xFFFFFFFF
SEED = 0x28C0E011

# Convert each ID to its RAW hash form for proper XOR comparison
raw_hashes = {}
for idv, (name, typ, fl, lv) in cracked.items():
    if typ == 'audio':
        raw_hashes[idv] = idv
    else:  # sprite uses WRAP
        raw_hashes[idv] = rotr(idv ^ SEED, 27)

# Sort by raw hash and find tight clusters
items = sorted(raw_hashes.items(), key=lambda x: x[1])
pairs = []
ids = list(items)
for i, (idv1, raw1) in enumerate(ids):
    for j in range(i+1, min(i+20, len(ids))):  # nearest neighbors
        idv2, raw2 = ids[j]
        diff = raw1 ^ raw2
        bd = bin(diff).count('1')
        if bd <= 4:
            pairs.append((bd, idv1, idv2, raw1, raw2, cracked[idv1], cracked[idv2]))

pairs.sort(key=lambda x: x[0])

print(f'Found {len(pairs)} small-XOR pairs (<=4 bits)')
print()
for bd, id1, id2, r1, r2, info1, info2 in pairs[:50]:
    n1, t1, f1, l1 = info1
    n2, t2, f2, l2 = info2
    h1 = calcHash(n1)
    h2 = calcHash(n2)
    actual_diff = bin(h1 ^ h2).count('1')
    print(f'  bd={bd}/{actual_diff}  raw={r1:08x}^{r2:08x}={r1^r2:08x}')
    print(f'    {n1!r} ({t1}, fl={f1})')
    print(f'    {n2!r} ({t2}, fl={f2})')
