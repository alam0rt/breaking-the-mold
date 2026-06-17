import csv
from collections import Counter

print('=== cracked_names.csv ===')
with open('docs/analysis/asset-identification/cracked_names.csv') as f:
    rows = list(csv.DictReader(f))
print(f'Total rows: {len(rows)}')
status_by_type = Counter()
for r in rows:
    status_by_type[(r['type'], r['status'])] += 1
for (t, s), n in sorted(status_by_type.items()):
    print(f'  type={t:<10s} status={s:<20s} count={n}')

visual_count = sum(1 for r in rows if r['status'] == 'uncracked' and 'VISUAL' in (r.get('alts') or ''))
print()
print(f'Uncracked rows with VISUAL labels: {visual_count}')

print()
print('=== sprite_identification_template.csv ===')
with open('docs/analysis/asset-identification/sprite_identification_template.csv') as f:
    rows = list(csv.DictReader(f))
print(f'Total rows: {len(rows)}')
with_role = sum(1 for r in rows if r.get('human_role'))
print(f'With human_role: {with_role}')
conf_count = Counter(r.get('confidence', '') for r in rows if r.get('human_role'))
for c, n in conf_count.most_common():
    print(f'  confidence={c!r:<20}: {n}')

loc_count = sum(1 for r in rows if 'LOCALIZED_23' in (r.get('notes') or ''))
print()
print(f'LOCALIZED_23 sprites marked: {loc_count}')
