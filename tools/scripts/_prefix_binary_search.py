"""Search binary files for strings that hash to 0x88200080."""
import os
import re

CS = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789'
STEP = [0]*36
for d, c in enumerate(CS):
    o = ord(c)
    if c.isdigit(): o += 22
    STEP[d] = (o - 64) & 31

def calc(s):
    h, sh = 0, 0
    for c in s.upper():
        if c in CS:
            d = CS.index(c)
            sh = (sh + STEP[d]) & 31
            h ^= 1 << sh
    return h & 0xFFFFFFFF, sh

T, TS = 0x88200080, 27

# Walk all extracted assets and binary files
paths_to_scan = [
    'bin/SLES_010.90',
    'extracted',
    'disks',
]

candidates_found = set()
files_scanned = 0
strings_tested = 0

def scan_file(path):
    global files_scanned, strings_tested
    try:
        with open(path, 'rb') as f:
            data = f.read()
    except (OSError, MemoryError):
        return
    files_scanned += 1
    # Extract printable ASCII strings of length >= 4
    for m in re.finditer(rb'[\x20-\x7e]{4,40}', data):
        s = m.group().decode('ascii', errors='ignore')
        # Try the string itself, plus underscore-stripped, etc.
        for variant in [s, s.upper(), s.replace(' ', ''), s.replace('-', '_')]:
            strings_tested += 1
            h, sh = calc(variant)
            if h == T and sh == TS:
                candidates_found.add(variant)
                print(f'  *** MATCH from {path}: {variant!r}')

for root_path in paths_to_scan:
    if not os.path.exists(root_path):
        continue
    if os.path.isfile(root_path):
        scan_file(root_path)
    else:
        for dirpath, dirs, files in os.walk(root_path):
            for fn in files:
                p = os.path.join(dirpath, fn)
                scan_file(p)

print(f'\nScanned {files_scanned} files, tested {strings_tested} strings')
print(f'Found {len(candidates_found)} matches:')
for c in sorted(candidates_found):
    print(f'  {c!r}')
