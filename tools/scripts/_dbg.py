import csv
with open('docs/analysis/asset-identification/cracked_names.csv') as f:
    rows = list(csv.DictReader(f))
print(f'Total rows: {len(rows)}')
print(f'First 3 rows:')
for r in rows[:3]:
    print(f'  id_hex={r["id_hex"]!r} status={r["status"]!r}')
print(f'Unique id_hex prefixes:')
from collections import Counter
prefs = Counter(r['id_hex'][:2] for r in rows)
for p, n in prefs.most_common():
    print(f'  {p!r}: {n}')
