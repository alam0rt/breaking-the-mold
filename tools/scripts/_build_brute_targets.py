"""Build target ID files for kcrack brute force, split by namespace.

Outputs:
  /tmp/targets_raw.txt    — RAW-namespace IDs (audio + ~all sprites)
  /tmp/targets_wrap.txt   — WRAP-namespace targets (for menu text)
                             Plus inverse-WRAP IDs for each unknown sprite so a
                             RAW-mode brute would also find them.

Note on WRAP attack:
  If id_wrap = SEED ^ rotl(calcHash(name), 27)
  then calcHash(name) = rotr(id_wrap ^ SEED, 27)
  So to brute a WRAP target with --raw mode, write rotr(id_wrap ^ SEED, 27).
"""
import csv

SEED = 0x28C0E011

def rotr(v, r):
    r &= 31
    return ((v >> r) | (v << ((32 - r) & 31))) & 0xFFFFFFFF

def unwrap(id_wrap):
    """Convert WRAP id to its underlying calcHash value."""
    return rotr(id_wrap ^ SEED, 27)

# Load all uncracked IDs
all_raw = set()
all_wrap = set()

with open('docs/analysis/asset-identification/cracked_names.csv') as f:
    for r in csv.DictReader(f):
        if not r['id_hex'].startswith('0x'):
            continue
        if r['status'] != 'uncracked':
            continue
        idv = int(r['id_hex'], 16)
        # RAW: all audio + sprite + anim + type500/700 (everything except 6 menu anchors)
        all_raw.add(idv)
        # WRAP: also try (uncracked sprites might use WRAP for text rendering)
        all_wrap.add(unwrap(idv))

# Save
with open('/tmp/targets_raw.txt', 'w') as f:
    for i in sorted(all_raw):
        f.write(f'0x{i:08x}\n')

with open('/tmp/targets_unwrap.txt', 'w') as f:
    # The unwrapped-equivalent calcHash values; brute kcrack in --raw mode against these
    # If any string X has calcHash(X) == val, then wrap(calcHash(X)) == original_id
    for i in sorted(all_wrap):
        f.write(f'0x{i:08x}\n')

# COMBINED: brute one set covering both namespaces simultaneously
combined = all_raw | all_wrap
with open('/tmp/targets_combined.txt', 'w') as f:
    for i in sorted(combined):
        f.write(f'0x{i:08x}\n')

print(f'RAW targets:        {len(all_raw):>4}  -> /tmp/targets_raw.txt')
print(f'WRAP-unwrapped:     {len(all_wrap):>4}  -> /tmp/targets_unwrap.txt')
print(f'Combined (one brute): {len(combined):>4}  -> /tmp/targets_combined.txt')
print()
print('Use:')
print('  kcrack extend "" /tmp/targets_combined.txt --raw --suf 8 > /tmp/preimages.txt')
print('Then grep /tmp/preimages.txt for substrings of interest (1970, KLAY, ...)')
