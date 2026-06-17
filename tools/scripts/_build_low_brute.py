"""Build correct brute targets: WRAP-inverse for sprites, RAW for audio.

Sprite IDs use WRAP namespace: id_wrap = (calcHash(name) << 27) | (... rotation)
                                            ^ 0x28C0E011
Inverse: calcHash(name) = rotr(id_wrap ^ 0x28C0E011, 27)

So for kcrack (which runs in RAW mode), feed the inverse value.
"""
import csv

def rotr(x, n):
    return ((x >> n) | (x << (32 - n))) & 0xFFFFFFFF

SEED = 0x28C0E011

# Read all uncracked low-floor targets
targets = []
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
        if fl > 8:
            continue
        # Sprite -> WRAP namespace -> need inverse for RAW brute
        # Audio -> RAW namespace -> use as-is
        # type500/type700 -> unknown, try both
        if r['type'] == 'sprite':
            brute_id = rotr(idv ^ SEED, 27)
            ns = 'wrap'
        elif r['type'] == 'audio':
            brute_id = idv
            ns = 'raw'
        else:
            brute_id = idv  # try raw first
            ns = 'unknown'
        targets.append({
            'orig': idv,
            'brute': brute_id,
            'type': r['type'],
            'ns': ns,
            'floor': fl,
            'levels': r['levels'][:50],
            'alts': r['alts'][:80],
        })

# Sort by floor then orig
targets.sort(key=lambda x: (x['floor'], x['orig']))

print(f'{"orig_id":<12s} {"brute_id":<12s} {"ns":<8s} {"type":<8s} {"floor":<6s} levels/alts')
print('-' * 110)
for t in targets:
    print(f'0x{t["orig"]:08x}  0x{t["brute"]:08x}  {t["ns"]:<8s} {t["type"]:<8s} {t["floor"]:<6d} {t["levels"]}/{t["alts"]}')

# Write target file (only the brute IDs)
with open('/tmp/targets_low_brute.txt', 'w') as f:
    for t in targets:
        f.write(f'0x{t["brute"]:08x}\n')

# Also a mapping file
with open('/tmp/targets_low_map.txt', 'w') as f:
    for t in targets:
        f.write(f'0x{t["brute"]:08x}\t0x{t["orig"]:08x}\t{t["ns"]}\t{t["type"]}\tfloor={t["floor"]}\tlevels={t["levels"]}\n')

print(f'\nWrote {len(targets)} targets to /tmp/targets_low_brute.txt')
