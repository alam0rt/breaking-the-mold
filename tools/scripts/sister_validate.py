#!/usr/bin/env python3
"""
Sister validation — for each candidate root, exhaustively try common
modifier patterns and report ALL hits in the master ID set.
A real family should have multiple siblings in the master.
"""
import csv
import os

def calcHash(s):
    h, sh = 0, 0
    for ch in s:
        c = ord(ch)
        if 65 <= c <= 90: pass
        elif 97 <= c <= 122: c -= 32
        elif 48 <= c <= 57: c += 22
        else: continue
        sh = (sh + c - 64) & 31
        h ^= 1 << sh
    return h & 0xFFFFFFFF

REPO = '/home/sam/projects/btm'
allids = {}
with open(os.path.join(REPO, 'docs/analysis/asset-identification/cracked_names.csv')) as f:
    for row in csv.DictReader(f):
        if row['id_hex'].startswith('0x'):
            allids[int(row['id_hex'], 16)] = row

# Wide modifier set
mods = (
    [''] +
    [f'_{i}' for i in range(1, 21)] +
    [f'_{i:02d}' for i in range(1, 21)] +
    ['_A','_B','_C','_D','_E','_F','_G','_H','_I','_J','_K','_L','_M','_N','_O','_P','_Q','_R','_S','_T','_U','_V','_W','_X','_Y','_Z'] +
    ['_LOOP','_END','_START','_MAIN','_BEGIN','_FINISH','_INIT'] +
    ['_LEFT','_RIGHT','_UP','_DOWN','_DN','_TOP','_BOTTOM','_BOT','_FRONT','_BACK','_SIDE','_NEAR','_FAR'] +
    ['_BIG','_SM','_SMALL','_LG','_LARGE','_HI','_HIGH','_LO','_LOW','_FAST','_SLOW','_HARD','_SOFT'] +
    ['_IN','_OUT','_ON','_OFF','_OPEN','_CLOSE','_RUN','_STOP'] +
    ['_OK','_NOK','_NEW','_OLD','_FULL','_HALF','_PART','_DONE'])

ROOTS = [
    # Strict-pattern candidates being validated
    'FX_SKULL_TAP',
    'FX_PUFF_FALL',
    'FX_KLAY_WET',
    # Plus context — verified families to confirm tool works
    'FX_SKULL_FLY',
    'FX_BIRD_FLY',
    'FX_BOSS_HEAD_IDLE',
    'FX_KLAY',
    'FX_SKULL',
    'FX_BIRD',
    'FX_PUFF',
    'FX_RAT',
    'FX_GUM',
]

print(f'mods set: {len(mods)} variants')
print(f'roots: {len(ROOTS)}')
print()

for root in ROOTS:
    found = []
    for m in mods:
        s = root + m
        h = calcHash(s)
        if h in allids:
            row = allids[h]
            mark = 'V' if row['status'] == 'verified' else ' '
            cand = row['status'] == 'candidate'
            mk = 'V' if row['status'] == 'verified' else 'C' if row['status'] == 'candidate' else ' '
            found.append((m, s, h, row['status'], row.get('name','') or '', row.get('levels','')[:30], mk))
    if found:
        print(f'== {root}: {len(found)} hit(s)')
        for m, s, h, st, nm, lv, mk in found:
            print(f'  [{mk}] {s:32s} -> 0x{h:08x}  {st:10s}  {nm:30s} levels={lv}')
        print()
