"""Extract prefix tokens from guesses.html."""
import re
text = open('/tmp/guesses.html').read()
spans = re.findall(r'<span class="[wr]">([A-Z0-9_]+)</span>', text)
prefixes = {}
for s in spans:
    if '_' in s:
        p = s.split('_')[0]
        if 1 <= len(p) <= 6:
            prefixes[p] = prefixes.get(p, 0) + 1
print(f'Found {len(prefixes)} distinct prefix tokens, {len(spans)} guesses')
for p, c in sorted(prefixes.items(), key=lambda x: -x[1]):
    print(f'  {c:3d}  {p}')
