"""Show all L=4 preimages from the page."""
import re
with open('/tmp/prefix.html') as f:
    page = f.read()
m = re.search(r'var ALL=\[(.+?)\];', page)
preimages = re.findall(r'"([^"]+)"', m.group(1))
print(f'Total preimages: {len(preimages)}')

by_len = {}
for p in preimages:
    L = len(p)
    by_len.setdefault(L, []).append(p)

for L in sorted(by_len):
    print(f'\nL={L}: {len(by_len[L])} preimages')

print('\n=== All L=4 preimages ===')
for p in sorted(by_len[4]):
    print(f'  {p}')

# Show some L=6 with no digits at all
print('\n=== All L=6 letter-only preimages ===')
letter_only = [p for p in by_len[6] if p.isalpha()]
for p in sorted(letter_only):
    print(f'  {p}')
print(f'Total letter-only L=6: {len(letter_only)}')
