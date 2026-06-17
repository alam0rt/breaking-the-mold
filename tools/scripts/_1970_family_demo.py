"""Demonstrate the brute-force-with-substring approach for 1970-family hashes.

The user's key idea: "if we know the floor / length is N chars, brute every
combination". The problem is that:
  - Hash space is 32 bits -> infinite preimages
  - Real names are ~14 chars (alphanumeric) -> 36^14 = 6e21 candidates
  - We can ONLY brute up to ~8 alphanumeric chars

But there's still hope: BRUTE THE CONTEXT around a known substring.
For target audio that we suspect is a 1970-family pickup variant, we can
brute names of form 'FX_PICKUP_1970' + suffix where suffix is short.

This script shows what 1970-family names hash to known/unknown audio targets.
"""
import sys
sys.path.insert(0, 'tools/scripts')
from calchash import calcHash

# Known FX_PICKUP_1970 family
KNOWN = {
    'FX_PICKUP_1970':       0x408a6461,  # normal hamster
    'FX_PICKUP_1970_03':    0x428a6465,  # 3rd hamster (full shield)
}

print('Known FX_PICKUP_1970 family:')
for n, t in KNOWN.items():
    h = calcHash(n)
    ok = 'YES' if h == t else 'no'
    print(f'  {n:<30s} target=0x{t:08x}  raw=0x{h:08x}  match={ok}')

# Generate variants (continuous extension of family)
print()
print('Hashing common 1970 variants:')
variants = [
    # Numeric variants (1, 2, 3, ...)
    *[f'FX_PICKUP_1970_{i:02d}' for i in range(0, 10)],
    *[f'FX_PICKUP_1970_{i}' for i in range(0, 10)],
    # Action variants
    'FX_PICKUP_1970_HAMSTER','FX_PICKUP_1970_FULL','FX_PICKUP_1970_SHIELD',
    'FX_PICKUP_1970_NORMAL','FX_PICKUP_1970_END','FX_PICKUP_1970_START',
    'FX_PICKUP_1970_LOOP','FX_PICKUP_1970_OUT','FX_PICKUP_1970_IN',
    'FX_PICKUP_1970_OFF','FX_PICKUP_1970_ON',
    # Letter suffixes
    *[f'FX_PICKUP_1970_{c}' for c in 'ABCDEFGH'],
]

# Load the unknown audio targets to spot any hits
import csv
unknowns = {}
with open('docs/analysis/asset-identification/cracked_names.csv') as f:
    for r in csv.DictReader(f):
        if r['status'] == 'uncracked' and r['type'] == 'audio':
            unknowns[int(r['id_hex'], 16)] = (r['levels'], int(r['floor']))

print(f'Loaded {len(unknowns)} uncracked audio targets')
print()

hits = []
for n in variants:
    h = calcHash(n)
    if h in unknowns:
        lv, fl = unknowns[h]
        hits.append((n, h, lv))
    elif h in {0x408a6461, 0x428a6465}:
        pass  # already-cracked
    # else: just print the value
    
if hits:
    print(f'Found {len(hits)} hits from {len(variants)} variants:')
    for n, h, lv in hits:
        print(f'  HIT: {n:<35s} -> 0x{h:08x}  levels={lv[:50]}')
else:
    print(f'No hits in {len(variants)} candidate variants.')

# Now try sister-pair brute: kcrack extend on FX_PICKUP_1970_ + 4-char suffix
print()
print('To brute every "FX_PICKUP_1970_<suffix>" with suffix up to 4 alphanumeric chars,')
print('run:')
print('  ./tools/kcrack extend FX_PICKUP_1970_ /tmp/targets_audio.txt --raw --suf 4')
print()
print('That brutes 36 + 36^2 + 36^3 + 36^4 = 1.7M candidates instantly,')
print('and reports any of the 174 uncracked audio targets that any candidate hits.')
