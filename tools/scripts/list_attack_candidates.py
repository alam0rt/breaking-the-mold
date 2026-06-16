#!/usr/bin/env python3
"""Show attack candidates: uncracked ids at low popcount with single-level
(context-rich) attribution."""

import csv

with open('docs/analysis/asset-identification/cracked_names.csv') as f:
    rows = list(csv.DictReader(f))

print('=== UNCRACKED IDS at floor<=8 (potential attack candidates) ===\n')
low = [r for r in rows
       if r['status']=='uncracked'
       and r['id_hex'].startswith('0x')
       and bin(int(r['id_hex'],16)).count('1') <= 8]

# Sort by floor then type then levels-narrowness
def levels_narrow(r):
    return r['levels'].count(';') if r['levels'] else -1

for r in sorted(low, key=lambda r: (bin(int(r['id_hex'],16)).count('1'), levels_narrow(r), r['type'], r['id_hex'])):
    pc = bin(int(r['id_hex'],16)).count('1')
    levels = r['levels'][:60]
    print(f'  {r["id_hex"]} popc={pc:2d} {r["type"]:8s} {levels}')

print(f'\nTOTAL: {len(low)} IDs at floor<=8\n')

# Single-level audio/sprite (most context-rich)
print('=== Single-level attack targets (highest context value) ===')
single = [r for r in low if ';' not in r['levels'] and r['levels'] and r['type'] in ('audio','sprite')]
for r in sorted(single, key=lambda r: (bin(int(r['id_hex'],16)).count('1'), r['levels'])):
    pc = bin(int(r['id_hex'],16)).count('1')
    print(f'  {r["id_hex"]}  popc={pc}  {r["type"]:8s}  level={r["levels"]}')
