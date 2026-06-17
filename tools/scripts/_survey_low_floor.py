"""Survey uncracked targets by floor (popcount). Low floor = brute-feasible.

For each uncracked target, floor is the minimum alnum length. Real length is
floor + 2k for k internal collisions, but most names will have k=0 or 1, so
the real length is most likely floor or floor+2.

Brute feasibility:
  L=4: 1.7M cands, instant
  L=5: 60M, instant
  L=6: 2.2B, 7 sec
  L=7: 78B, 4 min
  L=8: 2.8T, 2.6 hr
  L=9: 100T, 4 days
  L=10: 3.7P, 141 days
"""
import csv
from collections import Counter, defaultdict

by_floor = defaultdict(list)
with open('docs/analysis/asset-identification/cracked_names.csv') as f:
    for r in csv.DictReader(f):
        if r['status'] != 'uncracked':
            continue
        if not r['id_hex'].startswith('0x'):
            continue
        idv = int(r['id_hex'], 16)
        if idv == 0:
            continue
        fl = int(r['floor'])
        by_floor[fl].append({
            'id': r['id_hex'],
            'type': r['type'],
            'levels': r['levels'][:60],
            'alts': r['alts'][:80],
        })

print('Floor distribution of uncracked targets:')
print(f'{"floor":<6} {"count":<6} sprite/audio/other')
print('-' * 60)
for fl in sorted(by_floor):
    rows = by_floor[fl]
    types = Counter(r['type'] for r in rows)
    typestr = ' '.join(f'{k}={v}' for k, v in sorted(types.items()))
    print(f'{fl:<6} {len(rows):<6} {typestr}')

# Show actual targets at low floors
print()
print('=' * 70)
print('LOW-FLOOR TARGETS (brute-able):')
print('=' * 70)
for fl in sorted(by_floor):
    if fl > 8:
        break
    rows = by_floor[fl]
    print(f'\n--- floor={fl} ({len(rows)} targets) ---')
    for r in rows[:30]:
        levels = r['levels'] or '-'
        alts = r['alts'] or '-'
        print(f'  {r["id"]} {r["type"]:<7s} lv={levels:<25s} alts={alts}')
    if len(rows) > 30:
        print(f'  ... +{len(rows) - 30} more')
