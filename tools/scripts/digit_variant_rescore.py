"""Take every existing 'candidate' name in cracked_names.csv and produce its
digit-letter equivalence class. Score each variant by:
- digit count (more digits = likely an enumerated asset)
- token boundary plausibility (digits at end is good)
- mixed letter+digit pattern: PREFIX + DIGITS at end is highly plausible
"""
import csv
import itertools
import re

# letters F-O step-equivalent to digits 0-9
L2D = {'F':'0','G':'1','H':'2','I':'3','J':'4','K':'5','L':'6','M':'7','N':'8','O':'9'}
D2L = {v:k for k,v in L2D.items()}

def variants(s):
    pos = []
    for c in s.upper():
        opts = [c]
        if c in L2D:
            opts.append(L2D[c])
        elif c in D2L:
            opts.append(D2L[c])
        pos.append(opts)
    out = set()
    for combo in itertools.product(*pos):
        out.add(''.join(combo))
    return out

def score(s):
    """Higher = more like a real asset name."""
    s = s.upper()
    score = 0
    # trailing digits = +good (numbered asset)
    m = re.match(r'^([A-Z]+)(\d+)$', s)
    if m:
        score += 50
        if len(m.group(2)) == 2:  # 2-digit suffix is most common
            score += 20
    # all alpha = +good
    elif s.isalpha():
        score += 30
    # interior digit blocks (PREFIX_07_SUFFIX) = +meh
    if re.search(r'[A-Z]\d+[A-Z]', s):
        score += 10
    # leading digit = -bad
    if s and s[0].isdigit():
        score -= 30
    # too many digit transitions = -bad
    transitions = sum(1 for i in range(1, len(s)) if s[i].isdigit() != s[i-1].isdigit())
    score -= transitions * 5
    return score

# Read the candidate ledger
csv_path = 'docs/analysis/asset-identification/cracked_names.csv'
candidates_by_id = {}
with open(csv_path) as f:
    reader = csv.DictReader(f)
    for row in reader:
        if row.get('status') in ('candidate', 'verified'):
            aid = row['id_hex']
            name = row.get('name', '')
            if name:
                candidates_by_id.setdefault(aid, []).append(name)
            # also alts
            alts = row.get('alts', '')
            if alts:
                for alt in alts.split(';'):
                    alt = alt.strip()
                    if alt:
                        candidates_by_id.setdefault(aid, []).append(alt)

print(f"Candidates with names: {len(candidates_by_id)}")
print()
print("=" * 70)
print("Best digit-equivalent variant per candidate (sorted by readability score)")
print("=" * 70)

results = []
for aid, names in candidates_by_id.items():
    best_var, best_score, best_orig = None, -999, None
    for name in names:
        for v in variants(name):
            sc = score(v)
            if sc > best_score:
                best_score, best_var, best_orig = sc, v, name
    if best_score > 0:  # show interesting variants
        results.append((best_score, aid, best_orig, best_var))

results.sort(reverse=True)
for sc, aid, orig, var in results[:60]:
    if orig.upper() != var:
        print(f"  score={sc:3d}  {aid}  {orig:25} -> {var}")
    else:
        print(f"  score={sc:3d}  {aid}  {orig}")
