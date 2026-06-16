"""Combine raw-mode candidates with function-call context to find the most likely names.

For each unknown ID:
  1. Look up its callers in asset_usage_by_function.csv.
  2. Map the function call (PlayEntityPositionSound, InitParticleEntity, etc.) to the asset role.
  3. Score each combine candidate with role-relevant keywords boosted.
"""

import re
import csv
from collections import defaultdict

# Role inference from function names that take asset IDs:
ROLE_RULES = [
    (re.compile(r'PlaySound|PlayEntityPositionSound|StartSound|PlayMusic|PlayBgm|PlaySE', re.I), 'audio'),
    (re.compile(r'SetEntitySpriteId|InitSprite|StartAnimation|SetSpriteId|LoadSprite', re.I), 'sprite'),
    (re.compile(r'InitParticle|EmitParticle|SpawnParticle', re.I), 'particle'),
    (re.compile(r'Background|setBackground|LoadBg|SetBg', re.I), 'background'),
    (re.compile(r'Tile|Tilemap', re.I), 'tile'),
    (re.compile(r'Font', re.I), 'font'),
    (re.compile(r'Palette|Pal\b', re.I), 'palette'),
    (re.compile(r'Music|Bgm', re.I), 'music'),
]

ROLE_KEYWORDS = {
    'audio':     {'SOUND','SFX','SND','VOICE','VO','HUM','BUZZ','BANG','BOOM','SPLAT','SCREAM','LAUGH',
                  'GRUNT','YELL','GIGGLE','CHOMP','POP','GASP','CRY','MOAN','GROAN','HEY','OUCH',
                  'SIGH','PLOP','HOOT','OINK','BARK','MEOW','WOOF','BLEH','BLAH','BAH'},
    'sprite':    {'SPRITE','SPR','IMG','IMAGE','PIC','FRAME','ANIM','CEL'},
    'particle':  {'PARTICLE','PART','EFFECT','FX','DUST','SMOKE','SPARK','DEBRIS','SHARD','BIT','PIECE'},
    'background':{'BG','BACK','BACKGROUND','SKY','SCENE','LAYER','LEVEL','STAGE','ROOM','BACKDROP'},
    'tile':      {'TILE','MAP','TILEMAP','GROUND','FLOOR','WALL'},
    'font':      {'FONT','TEXT','LETTER','GLYPH','CHAR','CHARS'},
    'palette':   {'PAL','PALETTE','COLOR','HUE','TONE','TINT'},
    'music':     {'MUSIC','BGM','THEME','SONG','TRACK','BEAT','LOOP','LP','MUS','TUNE'},
}

# Generic game tokens
GENERIC_TOKENS = {
    'BOIL','BRG1','CAVE','CLOU','CRYS','CSTL','EGGS','EVIL','FINN','FOOD','GLEN','GLID','HEAD',
    'KLOG','MEGA','MENU','MOSS','PHRO','RUNN','SCIE','SEVN','SNOW','SOAR','TMPL','WEED','WIZZ',
    'JOE','KLAY','KLAYMEN','KLAYMAN','PLAYER','MONKEY','SKULL','JOYBOY','MIKE','SHISH','BOSS',
    'WALK','RUN','JUMP','IDLE','BLINK','DEATH','DIE','HIT','HURT','GRAB','SHOOT','FIRE','FALL',
    'CLIMB','PUSH','PULL','OPEN','CLOSE','ENTER','EXIT','PICKUP','THROW','BREATHE','LOOK',
    'TURN','DUCK','SLIDE','STAND','ATTACK','DEFEND','SHIELD','LAND','LANDING','SPIN',
    'WIN','LOSE','LIFE','LIVES','SCORE','POINTS','BONUS','GAME','OVER','START','TITLE',
    'INTRO','OUTRO','LOGO','CREDIT','DEMO','LEVEL','WORLD','STAGE','CHECK','CHECKPOINT',
    'GUM','GUMBALL','STAR','RING','HEART','COIN','GEM','KEY','POWER','POWERUP','EGG','EGGS',
    'OK','CANCEL','BACK','OPTIONS','CONTROLS','SOUND','VIDEO','PASSWORD','PASS','CODE',
    'LOAD','SAVE','PHART','SHRINEY','GUARD','ALIEN','MONSTER','ROBOT',
}

DIGIT_3 = {f'{n:03d}' for n in range(1000)}
DIGIT_2 = {f'{n:02d}' for n in range(100)}

def parts_of(name):
    return [p.upper() for p in name.split('_')]

def score_candidate(parts, role):
    """Higher = more likely the real name."""
    s = 0
    role_kw = ROLE_KEYWORDS.get(role, set())
    for p in parts:
        if p in role_kw:
            s += 10
        elif p in GENERIC_TOKENS:
            s += 5
        elif p in DIGIT_3 or p in DIGIT_2 or p.isdigit():
            s += 3
        elif len(p) >= 4 and any(v in p for v in 'AEIOU'):
            s += 1
    if len(parts) == 2 and parts[0] in role_kw and parts[1] in (GENERIC_TOKENS | DIGIT_3 | DIGIT_2):
        s += 5
    return s

# Load candidates from raw_combine_full.txt
hit_re = re.compile(r'^(0x[0-9a-fA-F]+)\s+(\S+)\s+floor=(\d+)\s+len=(\d+)$')
candidates = defaultdict(list)
with open('/tmp/raw_combine_full.txt') as f:
    for line in f:
        m = hit_re.match(line.strip())
        if not m: continue
        idx, name, floor, ln = m.group(1), m.group(2), int(m.group(3)), int(m.group(4))
        if floor == 0 or ln == 0:
            continue
        candidates[idx].append((name, floor, ln))

# Load function context
context = defaultdict(list)
with open('/home/sam/projects/btm/docs/analysis/asset-identification/asset_usage_by_function.csv') as f:
    reader = csv.DictReader(f)
    for row in reader:
        funcname = row.get('function', '')
        ids = (row.get('ids', '') or '').split(';')
        snippets = row.get('first_examples', '')
        for idx in ids:
            idx = idx.strip().lower()
            if idx.startswith('0x'):
                context[idx].append((funcname, snippets))

def infer_role(funcs_and_snippets):
    """Infer asset role from function name and code snippet."""
    text = ' '.join(f"{f} {s}" for f, s in funcs_and_snippets)
    for pat, role in ROLE_RULES:
        if pat.search(text):
            return role
    return None

# Build the report
print(f"Candidates loaded: {sum(len(v) for v in candidates.values())} across {len(candidates)} IDs")
print(f"Context loaded:    {len(context)} IDs")
print()

results = []
for idx, cands in candidates.items():
    ctx = context.get(idx, [])
    role = infer_role(ctx)
    scored = [(score_candidate(parts_of(name), role), name, floor, ln) for (name, floor, ln) in cands]
    scored.sort(key=lambda x: -x[0])
    if not scored:
        continue
    best_score = scored[0][0]
    if best_score < 5:
        continue
    funcname = ctx[0][0] if ctx else ''
    results.append((idx, role or '?', best_score, scored, funcname))

# Sort by best score
results.sort(key=lambda x: -x[2])

print(f"{'ID':<12} {'role':<10} {'best_cand':<28} {'score':<5} {'#cands':<6} {'func':<40}")
print('-' * 120)
for idx, role, best_score, scored, funcname in results[:50]:
    best_name = scored[0][1]
    cnt = len(scored)
    runner = scored[1] if len(scored) > 1 else None
    runner_str = f"({runner[1]} s={runner[0]})" if runner and runner[0] >= 5 else ''
    print(f"{idx:<12} {role:<10} {best_name:<28} {best_score:<5} {cnt:<6} {funcname[:38]:<40} {runner_str}")

# Save
out = '/home/sam/projects/btm/docs/analysis/asset-identification/raw_role_aware_candidates.txt'
with open(out, 'w') as f:
    f.write(f"# Role-aware candidates from raw-mode combine attack + function-context\n")
    f.write(f"# id\trole\tbest_score\tbest_name\tnum_candidates\tfunction\trunnerup_name\trunnerup_score\n")
    for idx, role, best_score, scored, funcname in results:
        best_name = scored[0][1]
        cnt = len(scored)
        runner = scored[1] if len(scored) > 1 else (0, '', 0, 0)
        f.write(f"{idx}\t{role}\t{best_score}\t{best_name}\t{cnt}\t{funcname}\t{runner[1] if len(scored)>1 else ''}\t{runner[0] if len(scored)>1 else 0}\n")

print(f"\n{len(results)} role-aware strong candidates written to {out}")
