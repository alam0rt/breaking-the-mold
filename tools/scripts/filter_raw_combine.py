"""Filter raw_combine_full.txt for plausible game-context hits.

Strategy: rank each ID by its candidates' Skullmonkeys/Neverhood relevance.
- Rule out junk like 'BRIDLE_LAW' (pairs of random English words).
- Boost matches where a word is a known game token (level codes, character names,
  sprite/anim/audio terms).
"""

import re
from collections import defaultdict

GAME_TOKENS = {
    # Level codes
    'BOIL', 'BRG', 'BRG1', 'CAVE', 'CLOU', 'CRYS', 'CSTL', 'EGGS', 'EVIL',
    'FINN', 'FOOD', 'GLEN', 'GLID', 'HEAD', 'KLOG', 'MEGA', 'MENU', 'MOSS',
    'PHRO', 'RUNN', 'SCIE', 'SEVN', 'SNOW', 'SOAR', 'TMPL', 'WEED', 'WIZZ',
    # Character names
    'JOE', 'KLAY', 'KLAYMEN', 'KLAYMAN', 'PLAYER', 'MONKEY', 'SKULL',
    'JOYBOY', 'MIKE', 'SHISH',
    # Asset categories
    'STAGE', 'SCENE', 'SPRITE', 'ANIM', 'SOUND', 'AUDIO', 'MUSIC', 'BG',
    'BACKGROUND', 'TILE', 'FONT', 'ICON', 'TEXT', 'PALETTE', 'PAL', 'FRAME',
    'IMG', 'IMAGE', 'PIC', 'OBJ', 'OBJECT', 'PROP',
    # Player actions
    'WALK', 'RUN', 'JUMP', 'IDLE', 'BLINK', 'DEATH', 'DIE', 'HIT', 'HURT',
    'GRAB', 'SHOOT', 'FIRE', 'FALL', 'CLIMB', 'PUSH', 'PULL', 'OPEN', 'CLOSE',
    'ENTER', 'EXIT', 'PICKUP', 'THROW', 'BREATHE', 'LOOK', 'TURN', 'DUCK',
    'SLIDE', 'DUCK', 'STAND', 'ATTACK', 'DEFEND', 'SHIELD',
    # Game loop
    'WIN', 'LOSE', 'LIFE', 'LIVES', 'SCORE', 'POINTS', 'BONUS', 'GAME',
    'OVER', 'START', 'TITLE', 'INTRO', 'OUTRO', 'LOGO', 'CREDIT', 'DEMO',
    'BOSS', 'LEVEL', 'WORLD', 'STAGE', 'CHECK', 'CHECKPOINT',
    # Items/collectibles
    'GUM', 'GUMBALL', 'STAR', 'RING', 'HEART', 'COIN', 'GEM', 'KEY', 'POWER',
    'POWERUP', 'EGG', 'EGGS',
    # UI
    'MENU', 'OK', 'CANCEL', 'BACK', 'OPTIONS', 'CONTROLS', 'SOUND', 'VIDEO',
    'PASSWORD', 'PASS', 'CODE', 'LOAD', 'SAVE',
    # Audio types
    'BGM', 'THEME', 'SONG', 'BEAT', 'LOOP', 'AMB', 'AMBIENT', 'CHOMP',
    'SPLAT', 'BOOM', 'POP', 'SCREAM', 'LAUGH', 'GRUNT', 'YELL', 'GIGGLE',
    'FART', 'BURP', 'SFX',
    # Generic
    'NEW', 'OLD', 'BIG', 'SMALL', 'LARGE', 'TINY', 'TOP', 'BOTTOM', 'LEFT',
    'RIGHT', 'UP', 'DOWN', 'FRONT', 'BACK', 'INSIDE', 'OUTSIDE',
    # Numeric digits as a single token marker
}

DIGIT_TOKENS = {str(d) for d in range(10)} | {f'{d:02d}' for d in range(20)} | {f'{d:03d}' for d in range(100)}

# parse the combine output
hit_re = re.compile(r'^(0x[0-9a-fA-F]+)\s+(\S+)\s+floor=(\d+)\s+len=(\d+)$')
candidates = defaultdict(list)
with open('/tmp/raw_combine_full.txt') as f:
    for line in f:
        m = hit_re.match(line.strip())
        if not m:
            continue
        idx, name, floor, ln = m.group(1), m.group(2), int(m.group(3)), int(m.group(4))
        if floor == 0:
            continue   # pseudo "no name" matches
        if ln == 0:
            continue
        # break the combo on '_' (kcrack's separator)
        parts = name.split('_')
        score = 0
        for p in parts:
            up = p.upper()
            if up in GAME_TOKENS:
                score += 5
            elif up in DIGIT_TOKENS or up.isdigit():
                score += 2
            elif len(up) >= 4 and any(v in up for v in 'AEIOU'):
                score += 1   # pronounceable word
        candidates[idx].append((score, name, floor, ln, parts))

# sort each ID's candidates by score and pick the top
print(f"Distinct IDs hit: {len(candidates)}")
print()

# Show IDs with at least ONE high-score candidate
strong = []
for idx, cands in candidates.items():
    cands.sort(key=lambda x: -x[0])
    best = cands[0]
    if best[0] >= 5:
        strong.append((idx, cands))

print(f"IDs with >=1 game-token candidate (score >= 5): {len(strong)}")
print()

strong.sort(key=lambda x: -x[1][0][0])

# Show top 80 strong hits
for idx, cands in strong[:80]:
    best = cands[0]
    score, name, floor, ln, parts = best
    runner_up = cands[1] if len(cands) > 1 else None
    runner_score = runner_up[0] if runner_up else 0
    runner_name = runner_up[1] if runner_up else ''
    print(f"  {idx}  {name:30s} score={score:2d} floor={floor:2d} len={ln:2d}  "
          f"({len(cands)} cands, 2nd: '{runner_name}' s={runner_score})")

# Save full output
with open('/home/sam/projects/btm/docs/analysis/asset-identification/raw_combine_strong.txt', 'w') as f:
    f.write(f"# Strong (score >= 5) hits from raw-mode combine attack\n")
    f.write(f"# Format: id  best_name  score floor len  (cand_count, 2nd_best)\n")
    for idx, cands in strong:
        best = cands[0]
        score, name, floor, ln, parts = best
        runner_up = cands[1] if len(cands) > 1 else None
        runner_score = runner_up[0] if runner_up else 0
        runner_name = runner_up[1] if runner_up else ''
        f.write(f"{idx}\t{name}\t{score}\t{floor}\t{ln}\t{len(cands)}\t{runner_name}\t{runner_score}\n")
print(f"\nWritten to docs/analysis/asset-identification/raw_combine_strong.txt")
