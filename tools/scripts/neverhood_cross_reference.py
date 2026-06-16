"""COMPREHENSIVE Neverhood-Skullmonkeys hash cross-reference.

CONFIRMED MODEL: Skullmonkeys uses raw calcHash() (no wrap) for assets reused
from Neverhood. The 0x28C0E011 ^ rotl(h, 27) wrap is a SECOND namespace,
applied to menu/UI strings (NO/YES/QUIT/PAUSED/CONTINUE/QUITGAME).

So the strategy is:
1. Mine every 32-bit hex literal from ScummVM Neverhood source.
2. Cross-check raw against Skullmonkeys unknown IDs.
3. For each hit, emit ScummVM context (function name, surrounding code)
   so we can guess the original asset name string.
"""

import os
import re
import glob
import csv
from collections import defaultdict

def rotl(v, r):
    r &= 31
    return ((v << r) | (v >> ((32 - r) & 31))) & 0xFFFFFFFF

# Load Skullmonkeys ID metadata
sm_ids = {}
with open('/home/sam/projects/btm/docs/reference/asset-ids-master.csv') as f:
    reader = csv.DictReader(f)
    for row in reader:
        h = row.get('id_hex') or row.get('id') or ''
        if h.startswith('0x'):
            sm_ids[int(h, 16)] = row

print(f"Skullmonkeys IDs in master: {len(sm_ids)}")

level_info = {}
with open('/home/sam/projects/btm/docs/analysis/asset-identification/cracked_names.csv') as f:
    reader = csv.DictReader(f)
    for row in reader:
        h = row['id_hex']
        if h.startswith('0x'):
            level_info[int(h, 16)] = row

src_dir = '/tmp/scummvm_neverhood'
hash_re = re.compile(r'0x([0-9A-Fa-f]{8})\b')
nev_hash_to_context = defaultdict(list)

for src in sorted(glob.glob(f'{src_dir}/*.cpp')):
    fn = os.path.basename(src)
    text = open(src).read()
    lines = text.split('\n')
    cur_func = None
    func_re = re.compile(r'^(?:[A-Za-z_][\w:&*\s]*\s+)?([A-Za-z_][\w:]*)\s*\(')
    for i, line in enumerate(lines):
        m = func_re.match(line.lstrip())
        if m and ('::' in m.group(1) or line.lstrip().startswith(('void ', 'bool ', 'int ', 'uint32 ', 'static '))):
            cur_func = m.group(1)
        for hm in hash_re.finditer(line):
            h = int(hm.group(1), 16)
            if h < 0x00100000 or h == 0xFFFFFFFF:
                continue
            nev_hash_to_context[h].append((fn, cur_func or '?', i+1, line.strip()[:140]))

print(f"Unique Neverhood hash literals (raw): {len(nev_hash_to_context)}")

direct_hits = []
for N, contexts in nev_hash_to_context.items():
    if N in sm_ids:
        direct_hits.append((N, contexts))

wrapped_hits = []
for N, contexts in nev_hash_to_context.items():
    M = (0x28C0E011 ^ rotl(N, 27)) & 0xFFFFFFFF
    if M in sm_ids and M != N:
        wrapped_hits.append((M, N, contexts))

print(f"\n{'='*80}")
print(f"DIRECT MATCHES (Neverhood hash == Skullmonkeys ID): {len(direct_hits)}")
print(f"{'='*80}")

direct_hits.sort(key=lambda x: -len(x[1]))

for N, contexts in direct_hits:
    sm_meta = sm_ids.get(N, {})
    levels = level_info.get(N, {}).get('levels', '')
    asset_type = sm_meta.get('asset_type') or sm_meta.get('type') or ''
    print(f"\n  0x{N:08x}  type={asset_type:12s}  levels[:80]={levels[:80]}")
    for fn, func, ln, line in contexts[:6]:
        print(f"    {fn}:{ln} :: {func}")
        print(f"      {line}")

print(f"\n{'='*80}")
print(f"WRAPPED MATCHES: {len(wrapped_hits)}")
print(f"{'='*80}")
for M, N, contexts in wrapped_hits[:10]:
    print(f"  Skullmonkeys 0x{M:08x}  =  Wrap(Neverhood 0x{N:08x})")
    for fn, func, ln, line in contexts[:2]:
        print(f"    {fn}:{ln} :: {func}  [{line}]")

out = '/home/sam/projects/btm/docs/analysis/asset-identification/neverhood_direct_hits.csv'
with open(out, 'w') as f:
    f.write('skullmonkeys_id,scummvm_file,scummvm_function,scummvm_line_no,scummvm_code,sm_levels,sm_floor\n')
    for N, contexts in direct_hits:
        levels = level_info.get(N, {}).get('levels', '')
        floor = level_info.get(N, {}).get('floor', '')
        for fn, func, ln, line in contexts:
            line_clean = line.replace(',', ';').replace('"', "'")
            func_clean = (func or '').replace(',', ';')
            f.write(f'0x{N:08x},{fn},{func_clean},{ln},"{line_clean}",{levels},{floor}\n')

print(f"\nWritten: {out}")
print(f"Direct hits: {len(direct_hits)} unique IDs, {sum(len(c) for _,c in direct_hits)} total contexts")
