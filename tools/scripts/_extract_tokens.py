"""Use cracked-name tokens as vocab + brute combine on low-floor targets.

The cracked names contain jargon specific to this game. Tokenize them,
add common variants, and combine to find unknown names.
"""
import csv, re
import sys; sys.path.insert(0, 'tools/scripts')
from calchash import calcHash

# Extract tokens from all cracked names
tokens = set()
cracked = []
with open('docs/analysis/asset-identification/cracked_names.csv') as f:
    for r in csv.DictReader(f):
        if r['status'] != 'uncracked' and r['name']:
            cracked.append(r['name'])

print(f'Cracked names ({len(cracked)}):')
for n in sorted(cracked):
    print(f'  {n}')
    # Tokenize on _ space etc
    for tok in re.split(r'[\s_]+', n):
        if tok:
            tokens.add(tok.upper())

print()
print(f'Tokens extracted: {len(tokens)}')
for t in sorted(tokens):
    print(f'  {t}')
