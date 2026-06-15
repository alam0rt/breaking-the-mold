"""Quick hit-check of symbol-derived tokens against target id set."""
import sys
sys.path.insert(0, 'tools/scripts')
from compound_hash_attack import calc_hash_and_shift, rotl  # type: ignore

SEED = 0x28C0E011


def asset_id(name):
    h, _ = calc_hash_and_shift(name)
    return SEED ^ rotl(h, 27) & 0xFFFFFFFF


targets = set()
with open('/tmp/asset_targets.txt') as f:
    for L in f:
        s = L.strip()
        if not s or s.startswith('#'):
            continue
        targets.add(int(s, 16) if s.startswith('0x') else int(s))

with open('/tmp/sym_tokens.txt') as f:
    toks = [t.strip() for t in f if t.strip() and not t.strip().startswith('0X')]
toks = [t for t in toks if 3 <= len(t) <= 20]
print(f'tokens: {len(toks)}')

hits = []

# Single tokens
for t in toks:
    if asset_id(t) in targets:
        hits.append((asset_id(t), t, 'single'))

# Type prefixes
PREFIXES = ['S_', 'SPR_', 'SEQ_', 'SND_', 'SFX_', 'FX_', 'ANM_', 'N2_', 'SM_',
            'BG_', 'FG_', 'ITEM_', 'ENT_', 'OBJ_', 'MENU_', 'HUD_', 'BOSS_',
            'ENEMY_', 'LEVEL_', 'PHRO_', 'SCIE_', 'TMPL_', 'MEGA_', 'FINN_',
            'KLAY_', 'JOE_', 'EVIL_', 'BLB_']
for prefix in PREFIXES:
    for t in toks:
        a = asset_id(prefix + t)
        if a in targets:
            hits.append((a, prefix + t, 'prefixed'))

ACTIONS = ['STAND', 'WALK', 'RUN', 'JUMP', 'FALL', 'IDLE', 'DIE', 'ATTACK',
           'HURT', 'HIT', 'SHOOT', 'FIRE', 'SLAM', 'ROLL', 'BOUNCE', 'SCREAM',
           'YELL']
for t in toks:
    for a_word in ACTIONS:
        a = asset_id(t + a_word)
        if a in targets:
            hits.append((a, t + a_word, 'compound2'))
        a = asset_id(t + '_' + a_word)
        if a in targets:
            hits.append((a, t + '_' + a_word, 'compound2'))

print(f'hits: {len(hits)}')
seen = set()
for a, n, kind in sorted(hits):
    if (a, n) in seen:
        continue
    seen.add((a, n))
    print(f'  0x{a:08x}  {n!r:40s} ({kind})')
