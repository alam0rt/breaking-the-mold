#!/usr/bin/env python3
"""Round 11: prefix-anchored MITM using BLB level vocabulary as subjects.

For each target ID, fix the prefix to ALL combinations of:
- 4-letter level codes (BLB)
- Character names (Finn, Klogg, Joe, Glenn, Yntis, etc.)
- Common boss/enemy nouns

Then solve for what the SUFFIX hash must be, and check against an
exhaustive English+specific suffix wordlist.

This is fundamentally different from previous attempts because:
  1. Prefix is restricted to PROVEN game vocabulary (BLB display names)
  2. Suffix is a single-shot hash lookup (O(1) per target)
  3. Target IDs are filtered to high-floor (likely text strings)
"""
import sys, csv
sys.path.insert(0, "tools/scripts")
from compound_hash_attack import asset_id, calc_hash_and_shift, rotl, rotr

ROOT = "/home/sam/projects/btm"
SEED = 0x28C0E011

# Helper: get hash bits + shift for a token  
def hb(s):
    h, sh = calc_hash_and_shift(s)
    return h, sh

# For a TARGET id, the required combined hash is:
def target_h(idval):
    return rotr(idval ^ SEED, 27)

# For prefix A and target T:
#   h(A+B) = h(A) ^ rotl(h(B), sh(A)) = target_h(T)
#   => h(B) = rotr(target_h(T) ^ h(A), sh(A))
def required_h_b(a_h, a_sh, target):
    return rotr(target ^ a_h, a_sh)

# Load IDs
ids_dict = {}
with open(f"{ROOT}/docs/reference/asset-ids-master.csv") as f:
    rdr = csv.DictReader(f)
    for r in rdr:
        ids_dict[int(r["id_hex"], 16)] = r
ids = set(ids_dict)

KNOWN = {asset_id(s) for s in ["NO","YES","PAUSED","QUIT","CONTINUE","QUITGAME"]}

# Subjects: known characters + level codes + display name tokens
SUBJECTS = list(set([
    # Characters
    "FINN", "JOE", "JOEHEAD", "JOEHEADJOE", "KLAYMEN", "KLAY",
    "KLOGG", "KLOG", "GLENN", "GLENNYNTIS", "YNTIS", "YNT",
    "MONKEYMAGE", "MAGE", "MONKEY", "MONK", "MONKEYS",
    "PHARTHEAD", "PHART", "PLAYER", "BOSS", "ENEMY",
    "SHRINEY", "SHRINEYGUARD", "GUARD", "WIZ", "WIZZ",
    "RUNN", "ROBOT", "WORM",
    "WILLIE", "WILLIETROMBONE",
    "TROMBONE", "WIZARD", "MAGICIAN",
    "HAMSTER", "HAMSTERSHIELD",
    
    # Level codes (BLB)
    "MENU", "PHRO", "SCIE", "TMPL", "FINN", "MEGA", "BOIL", "SNOW",
    "FOOD", "HEAD", "BRG1", "GLID", "CAVE", "WEED", "EGGS", "GLEN",
    "CLOU", "SEVN", "SOAR", "CRYS", "CSTL", "WIZZ", "RUNN", "MOSS",
    "KLOG", "EVIL",
    
    # Display name keywords
    "OPTIONS", "OPTION", "SKULLMONKEY", "SKULLMONKEYS",
    "GATE", "SCIENCE", "CENTER", "SHRINES", "SHRINE",
    "AMAZING", "DRIVEY", "SHRINEYGUARD",
    "HARD", "BOILER", "BOIL", "SNO", "SKULLMONKEYBRAND",
    "BRAND", "HOTDOG", "HOT", "DOG",
    "JOEHEADJOE", "ELEVATED", "STRUCTURE", "MOSK",
    "DEATH", "GARDEN", "MINES", "WEEDS",
    "EGGS", "RUSHMORE",
    "SOAR", "SHARDS", "CASTLE", "MUERTOS",
    "MAGE", "INCREDIBLE",
    "GRAVEYARD", "ENGINE",
    
    # Common asset prefixes
    "SPR", "ANM", "ANIM", "FX", "PCL", "PARTICLE", "DEBRIS", "PFX",
    "ICON", "BTN", "BUTTON", "FONT", "TEXT", "BG", "BACK", "FG",
    "TILE", "FLOOR", "WALL", "DOOR", "SKY", "CLOUD",
    "HUD", "BAR", "METER",
    
    # Common verbs / objects
    "ATTACK", "DEFEND", "JUMP", "WALK", "RUN", "FALL", "FLY", "SWIM",
    "DEATH", "DIE", "KILL", "DAMAGE", "HIT", "HURT",
    "IDLE", "STAND", "REST", "WAIT", "SLEEP",
    "SPAWN", "INIT", "BIRTH", "CREATE",
    "DESTROY", "EXPLODE", "BLAST", "BURST", "SHATTER",
    "PICKUP", "GRAB", "GET", "TAKE",
    "SHOT", "SHOOT", "FIRE", "PROJECTILE",
    "BALL", "ROLL", "ROLLING", "SPHERE",
    "WIN", "LOSE", "VICTORY", "DEFEAT",
    "FLAG", "STAR", "GEM", "COIN", "ITEM", "POWERUP",
    
    # Common ending fragments
    "1", "2", "3", "01", "02", "03", "A", "B", "C", "D",
    "INTRO", "OUTRO", "START", "END", "MAIN", "BIG", "SMALL",
    "LEFT", "RIGHT", "UP", "DOWN", "BACK", "FRONT", "TOP", "BOTTOM",
]))

print(f"Subjects: {len(SUBJECTS)}")

# Pre-compute hash + shift for each subject (and its underscore variants)
prefix_data = []
for s in SUBJECTS:
    h, sh = calc_hash_and_shift(s)
    prefix_data.append((s, h, sh))

# Build B vocabulary - English wordlist + game-specific + all subjects
B_VOCAB = set(SUBJECTS)
# Add English words
import os
for path in ["/tmp/words_short.txt", "/usr/share/dict/words", "/tmp/words_36.txt"]:
    if os.path.exists(path):
        with open(path) as f:
            for line in f:
                w = line.strip().upper()
                if 2 <= len(w) <= 12 and w.isalpha():
                    B_VOCAB.add(w)
        break

# Add common 2-letter and 3-letter combos that are real game terms
B_VOCAB.update([
    "BBP", "FX", "AN", "SP", "PT", "FN", "SF", "BG", "FG", "MF", "MS",
    "MAG", "MGC", "GMR", "GMP", "PRM", "SND", "MUS", "PIC", "BMP",
    "L1", "L2", "L3", "S1", "S2", "S3", "M1", "M2", "M3", "F1", "F2", "F3",
])

# Pre-compute per-B hash bits  
B_by_h = {}
for w in B_VOCAB:
    h, sh = calc_hash_and_shift(w)
    B_by_h.setdefault(h, []).append(w)

print(f"B vocab unique: {len(B_VOCAB):,}, unique hash buckets: {len(B_by_h):,}")

# For each target ID, try every subject, solve for required B hash
print("\n=== MITM 2-token search ===")
hits = {}
for tid in ids:
    if tid in KNOWN:
        continue
    target_h_val = target_h(tid)
    
    for prefix, ph, psh in prefix_data:
        # Check direct match (just prefix alone)
        if ph == target_h_val:
            hits.setdefault(tid, []).append(prefix)
            continue
        
        required = required_h_b(ph, psh, target_h_val)
        if required in B_by_h:
            for b_word in B_by_h[required]:
                # Try concatenated and underscore-separated (same hash)
                candidate = prefix + b_word
                if asset_id(candidate) == tid:
                    hits.setdefault(tid, []).append(candidate)

# Filter to high-quality hits (avoid pure dictionary-noise)
print(f"\n=== Found {sum(len(v) for v in hits.values())} hits across {len(hits)} unique IDs ===")
print("\nShowing distinct IDs:")
for tid in sorted(hits):
    info = ids_dict[tid]
    candidates = sorted(set(hits[tid]))
    print(f"  0x{tid:08x}  popc={info['popcount']:2}  kinds={info['kinds']:25s}  hits={len(candidates)}")
    for c in candidates[:5]:
        print(f"    {c}")

# Save
with open(f"{ROOT}/docs/analysis/asset-identification/round11_mitm_blb_subjects.csv", "w", newline="") as f:
    w = csv.writer(f)
    w.writerow(["id_hex","candidate","popcount","kinds","sources"])
    for tid in sorted(hits):
        info = ids_dict[tid]
        for c in sorted(set(hits[tid])):
            w.writerow([f"0x{tid:08x}", c, info['popcount'], info['kinds'], info['sources']])
