#!/usr/bin/env python3
"""Filter klash_combine output for plausible asset names.

Heuristics:
  1. Must contain at least one game-vocab subject token
  2. Must NOT be just digits + filler
  3. Length 5-20 chars in joined form
  4. Score by token quality (subjects + actions = high score)
"""
import csv, re
from pathlib import Path

ROOT = Path("/home/sam/projects/btm")

# Subject tokens: characters/levels that GUARANTEE relevance
SUBJECT_TOKENS = set("""FINN JOE JOEHEAD KLAYMEN KLAY HOBORG KLOGG KLOG GLENN
YNTIS YNT MAGE MONKEYMAGE MONKEY MONK PHARTHEAD PHART PLAYER BOSS ENEMY
SHRINEY GUARD WIZZ RUNN ROBOT WORM WILLIE TROMBONE HAMSTER SHIELD CLAYBALL
BALL CLAY SKULL SKULLMONKEY SAGE WORMWIZARD MENU PHRO SCIE TMPL FINN MEGA
BOIL SNOW FOOD HEAD BRG1 GLID CAVE WEED EGGS GLEN CLOU SEVN SOAR CRYS CSTL
WIZZ RUNN MOSS KLOG EVIL AMAZING BOILER BRAND CASTLE CENTER DEATH DOG DRIVEY
ELEVATED ENGINE GARDEN GATE GLENN GRAVEYARD GUARD HARD INCREDIBLE MAGE MINES
MOSK MOSS MUERTOS OPTIONS PHRO RUSHMORE SCIENCE SHARDS SHRINES SHRINEY
STRUCTURE WIZZ WORM YNT YNTIS PASSWORD CONTINUE QUIT QUITGAME PAUSED YES NO
""".split())

# Action tokens: state/action verbs
ACTION_TOKENS = set("""IDLE STAND REST WAIT WALK RUN MOVE JUMP FALL DROP FLY
FLOAT HOVER SWIM SPLASH ATTACK ATK HIT STRIKE PUNCH KICK BASH SLASH DEATH DIE
KILL DEFEAT DESTROY BREAK SMASH DAMAGE HURT OUCH PAIN HEAL SPAWN INIT BIRTH
CREATE NEW BORN PICKUP GRAB TAKE GET HOLD ROLL SPIN ROTATE EXPLODE BLAST BOOM
BURST SHATTER CRACK DEBRIS PIECE FRAG PART CHUNK BIT PARTICLE FX EFFECT SFX
PFX PROJECTILE BULLET SHOT SHOOT FIRE PORTAL WARP TELEPORT VICTORY WIN WON
TRIUMPH SUCCESS INTRO OUTRO BEGIN START END FINAL FINISH OPEN CLOSE PRESS
RELEASE TOGGLE FLASH BLINK GLOW SHINE TWINKLE SPARKLE BUTTON BTN ICON ICN
IMAGE PIC SPRITE SPR ANIM ANM FRAME LOOP CYCLE STATE SOUND SND VOICE VOX
NOISE MUSIC MUS SAMPLE FONT FNT TEXT TXT STR LBL LABEL HUD BAR METER GAUGE
INDICATOR OPTION ITEM SELECT CHOOSE MAP WORLD STAGE LEVEL ZONE AREA ROOM
DOOR WALL FLOOR CEILING TILE SKY CLOUD STAR BACKGROUND FOREGROUND READY DONE
ACTIVE INACTIVE GEM COIN POWER POWERUP COLLECT BONUS REWARD SPAWN DEFAULT
""".split())

# Generic noise tokens (English words that produce false positives)
NOISE_TOKENS = set("""HAND ARE FACING HARD ARENA AREAS LANDSALE SHINZA HARSALE
LUST ASSE EVENT KLOG CAVE ROLL RIFLERY SMOKABLE COMMON SHIFTER WEAVE INWEAVE
""".split())

def is_plausible(name):
    """Score 0-3 based on token quality."""
    parts = re.split(r'[_\W]', name)
    parts = [p for p in parts if p and not p.isdigit()]
    
    if not parts:
        return False, 0, []
    
    # Filter pure digits / very short
    real_tokens = [p for p in parts if not p.isdigit() and len(p) >= 2]
    if not real_tokens:
        return False, 0, []
    
    has_subject = any(t in SUBJECT_TOKENS for t in real_tokens)
    has_action = any(t in ACTION_TOKENS for t in real_tokens)
    
    # Reject single-letter tokens
    if any(len(t) == 1 for t in real_tokens):
        return False, 0, []
    
    # Score
    score = 0
    if has_subject:
        score += 2
    if has_action:
        score += 1
    
    # Penalize if more than 50% digits in original
    digit_count = sum(c.isdigit() for c in name)
    alpha_count = sum(c.isalpha() for c in name)
    if digit_count > alpha_count // 2:
        score -= 1
    
    return score > 0, score, real_tokens

# Load IDs metadata
ids_dict = {}
with open(ROOT / "docs/reference/asset-ids-master.csv") as f:
    rdr = csv.DictReader(f)
    for r in rdr:
        ids_dict[int(r["id_hex"], 16)] = r

# Load and filter results
hits_by_id = {}
with open("/tmp/round13_focused.txt") as f:
    for line in f:
        parts = line.split()
        if len(parts) < 4:
            continue
        idstr, name = parts[0], parts[1]
        try:
            idval = int(idstr, 16)
        except ValueError:
            continue
        plausible, score, tokens = is_plausible(name)
        if plausible:
            hits_by_id.setdefault(idval, []).append((score, name, tokens))

# Sort by score
print(f"=== Plausible hits across {len(hits_by_id)} IDs ===\n")
shortlist = []
for idval in sorted(hits_by_id, key=lambda i: -max(s for s, _, _ in hits_by_id[i])):
    cands = sorted(hits_by_id[idval], key=lambda x: -x[0])
    info = ids_dict.get(idval, {})
    top = cands[0]
    if top[0] >= 3:
        marker = "✨✨"
    elif top[0] >= 2:
        marker = "✨ "
    else:
        marker = "  "
    print(f"{marker} 0x{idval:08x}  popc={info.get('popcount','?'):>2}  kinds={info.get('kinds','?'):20s}  hits={len(cands):3d}  best='{top[1]}' (s={top[0]})")
    for sc, name, tok in cands[:3]:
        print(f"      [{sc}] {name}")
    shortlist.append((idval, top))

# Save  
with open(ROOT / "docs/analysis/asset-identification/round13_filtered_hits.csv", "w", newline="") as f:
    w = csv.writer(f)
    w.writerow(["id_hex","top_candidate","top_score","n_candidates","all_candidates","popcount","kinds","sources"])
    for idval in sorted(hits_by_id):
        cands = sorted(hits_by_id[idval], key=lambda x: -x[0])
        info = ids_dict.get(idval, {})
        w.writerow([
            f"0x{idval:08x}",
            cands[0][1],
            cands[0][0],
            len(cands),
            ";".join(c[1] for c in cands[:5]),
            info.get('popcount',''),
            info.get('kinds',''),
            info.get('sources','')
        ])

print(f"\nTotal: {len(shortlist)} IDs with plausible hits")
print(f"  Score >=3: {sum(1 for _, t in shortlist if t[0] >= 3)}")
print(f"  Score >=2: {sum(1 for _, t in shortlist if t[0] >= 2)}")
print(f"\nSaved to: docs/analysis/asset-identification/round13_filtered_hits.csv")
