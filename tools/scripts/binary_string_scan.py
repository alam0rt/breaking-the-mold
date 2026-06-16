"""Scan all PSX binaries for plain-text strings that hash to known asset IDs."""
import re, os

def calcHash(s):
    h, sh = 0, 0
    for c in s.upper():
        c = ord(c)
        if 65<=c<=90: pass
        elif 48<=c<=57: c += 22
        else: continue
        sh = (sh + c - 64) & 31
        h ^= 1 << sh
    return h
def rotl(v, r):
    r &= 31
    return ((v << r) | (v >> ((32 - r) & 31))) & 0xFFFFFFFF
def asset_id(name):
    return (0x28C0E011 ^ rotl(calcHash(name), 27)) & 0xFFFFFFFF

ids = set()
with open('docs/reference/asset-ids-master.csv') as f:
    next(f)
    for L in f:
        parts = L.split(',')
        if parts[0].startswith('0x'):
            ids.add(int(parts[0],16))
print(f'Total ids: {len(ids)}')

bins = []
for root in ['disks/prototype', 'disks/demo', 'bin']:
    if not os.path.exists(root): continue
    for r,_,files in os.walk(root):
        for fn in files:
            p = os.path.join(r,fn)
            try:
                sz = os.path.getsize(p)
            except OSError:
                continue
            # skip ISOs (too big), keep loose binaries
            if 50_000 < sz < 5_000_000 and not p.lower().endswith(('.iso', '.cue')):
                bins.append(p)

print(f'Binaries to scan: {len(bins)}')

all_hits = {}
for b in bins:
    try:
        data = open(b,'rb').read()
    except OSError:
        continue
    tokens = set()
    for m in re.finditer(rb'[A-Za-z][A-Za-z0-9_]{3,30}', data):
        tokens.add(m.group(0).decode('ascii'))
    for tok in tokens:
        for variant in (tok, tok.upper(), tok.lower(), tok.replace('_','')):
            aid = asset_id(variant)
            if aid in ids:
                all_hits.setdefault(aid, set()).add((b, variant))

print(f'\nUnique IDs hit: {len(all_hits)}')
anchors = {0x29c0e211, 0x2ad0f011, 0x0ad0f813, 0x68c0f413, 0x69c04050, 0x69c8f473}
for aid, hset in sorted(all_hits.items()):
    tag = ' (ANCHOR)' if aid in anchors else ''
    print(f'  0x{aid:08x}{tag}')
    seen = set()
    for b, v in hset:
        if v in seen: continue
        seen.add(v)
        print(f'    {v:25} in {b}')
        if len(seen) >= 5: break
