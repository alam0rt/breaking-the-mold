#!/usr/bin/env python3
"""Round 12: brute-prefix MITM.

For each target, enumerate ALL 1-4 character prefixes (alphanumeric)
and solve for required suffix hash. Check against extensive vocabulary.

Total prefixes: 36 + 36^2 + 36^3 + 36^4 ≈ 1.7M.
Per target: 1 lookup × 1.7M = fast.
Total: 1.7M × 658 IDs = 1.1B operations.

Use itertools.product for prefix generation.
"""
import sys, csv, itertools, time
sys.path.insert(0, "tools/scripts")
from compound_hash_attack import asset_id, calc_hash_and_shift, rotl, rotr

ROOT = "/home/sam/projects/btm"
SEED = 0x28C0E011

ids_dict = {}
with open(f"{ROOT}/docs/reference/asset-ids-master.csv") as f:
    rdr = csv.DictReader(f)
    for r in rdr:
        ids_dict[int(r["id_hex"], 16)] = r
ids = set(ids_dict)
KNOWN = {asset_id(s) for s in ["NO","YES","PAUSED","QUIT","CONTINUE","QUITGAME"]}

def target_h(idval):
    return rotr(idval ^ SEED, 27)

def required_h_b(a_h, a_sh, target):
    return rotr(target ^ a_h, a_sh)

# Build B vocabulary - MORE diverse words including 1-3 letter combinations
import os
B_VOCAB = set()

# English words 3-12 chars
for path in ["/tmp/words_short.txt", "/usr/share/dict/words"]:
    if os.path.exists(path):
        with open(path) as f:
            for line in f:
                w = line.strip().upper()
                if 3 <= len(w) <= 14 and w.isalpha():
                    B_VOCAB.add(w)
        break

# Short suffixes (1-3 char alphanumeric)
ALPHA = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
DIGITS = "0123456789"

for c in ALPHA:
    B_VOCAB.add(c)
for c1 in ALPHA:
    for c2 in ALPHA:
        B_VOCAB.add(c1 + c2)
for c1 in ALPHA:
    for c2 in ALPHA:
        for c3 in ALPHA:
            B_VOCAB.add(c1 + c2 + c3)

# Common game-specific suffixes
B_VOCAB.update([
    "IDLE", "WALK", "JUMP", "FALL", "RUN", "SWIM", "FLY",
    "ATTACK", "ATK", "HIT", "DAMAGE", "DMG", "DIE", "DEATH",
    "SPAWN", "INIT", "BIRTH", "EXIT", "EXIT", "END", "START",
    "FX", "SFX", "PFX", "ANM", "SPR", "FNT", "ICN", "BTN", "PIC",
    "INTRO", "OUTRO", "OPEN", "CLOSE", "HIT", "MISS",
    "FRAME", "LOOP", "SPIN", "PULSE",
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0",
    "01", "02", "03", "04", "05", "10", "20", "30",
    "L", "R", "U", "D", "N", "S", "E", "W",
    "LEFT", "RIGHT", "UP", "DOWN", "TOP", "BOT", "BACK", "FORE", "MID",
    "BIG", "SML", "MID", "LRG", "MED", "TINY", "HUGE", "MASSIVE",
    "RED", "GREEN", "BLUE", "BLACK", "WHITE", "GOLD", "SILVER",
    "FIRE", "ICE", "WATER", "AIR", "EARTH",
    "ON", "OFF", "OPEN", "CLOSED", "ACTIVE", "READY", "DONE",
])

# Add level codes (4-letter)
B_VOCAB.update("MENU PHRO SCIE TMPL FINN MEGA BOIL SNOW FOOD HEAD BRG1 GLID CAVE WEED EGGS GLEN CLOU SEVN SOAR CRYS CSTL WIZZ RUNN MOSS KLOG EVIL".split())

# Index B vocabulary by hash
B_by_h = {}
for w in B_VOCAB:
    h, sh = calc_hash_and_shift(w)
    B_by_h.setdefault(h, []).append(w)

print(f"B vocab: {len(B_VOCAB):,} unique words, {len(B_by_h):,} hash buckets")

# Pre-compute all 1-3 char prefixes (alpha+digit)
SYM = ALPHA + DIGITS
all_prefixes = []
# 1-char
for c in SYM:
    h, sh = calc_hash_and_shift(c)
    all_prefixes.append((c, h, sh))
# 2-char
for a in SYM:
    for b in SYM:
        s = a + b
        h, sh = calc_hash_and_shift(s)
        all_prefixes.append((s, h, sh))
# 3-char
for a in SYM:
    for b in SYM:
        for c in SYM:
            s = a + b + c
            h, sh = calc_hash_and_shift(s)
            all_prefixes.append((s, h, sh))

print(f"Generated {len(all_prefixes):,} prefixes (1-3 chars)")

print("\n=== MITM with brute prefix ===")
hits = {}

t0 = time.time()
for tid in ids:
    if tid in KNOWN:
        continue
    target_h_val = target_h(tid)
    
    for prefix, ph, psh in all_prefixes:
        # Direct
        if ph == target_h_val:
            hits.setdefault(tid, set()).add(prefix)
        
        required = required_h_b(ph, psh, target_h_val)
        if required in B_by_h:
            for b_word in B_by_h[required]:
                cand = prefix + b_word
                if asset_id(cand) == tid:
                    hits.setdefault(tid, set()).add(cand)

elapsed = time.time() - t0
print(f"Searched in {elapsed:.1f}s")

# Filter "real" looking hits — must look like a sensible name
def is_real_looking(s):
    """Heuristic: discard pure dictionary-collisional gibberish."""
    s = s.upper()
    # Discard if dominantly random
    if any(s.startswith(p) for p in ["BG", "AB", "AC", "AD", "AE", "AF", "AG", "AH", "AI", "AJ", "AK", "AL", "AM", "AN", "AO", "AP", "AQ", "AR", "AS", "AT", "AU", "AV", "AW", "AX", "AY", "AZ"]):
        # Check if rest of string is a sensible word
        return False
    # Otherwise OK
    return True

print(f"\n=== Found {len(hits)} unique IDs with hits ===")
shortlist = []
for tid in sorted(hits):
    info = ids_dict[tid]
    cands = sorted(hits[tid])
    # Only show "reasonable looking" candidates
    plausible = [c for c in cands if 4 <= len(c) <= 14 and any(p in c for p in ["FINN", "JOE", "KLOGG", "GLENN", "YNTIS", "MAGE", "CLAYBALL", "PHART", "SHRINEY"])]
    if plausible:
        print(f"  ✨ 0x{tid:08x}  popc={info['popcount']:2}  hits={len(cands)}  plausible={len(plausible)}")
        for c in plausible[:5]:
            print(f"    >> {c}")
        shortlist.append((tid, plausible[:3], info))

# Save all
with open(f"{ROOT}/docs/analysis/asset-identification/round12_brute_prefix.csv", "w", newline="") as f:
    w = csv.writer(f)
    w.writerow(["id_hex","n_candidates","candidates","popcount","kinds"])
    for tid in sorted(hits):
        info = ids_dict[tid]
        cands = sorted(hits[tid])
        w.writerow([f"0x{tid:08x}", len(cands), ";".join(cands[:20]), info['popcount'], info['kinds']])

print(f"\nFull results: docs/analysis/asset-identification/round12_brute_prefix.csv")
print(f"Plausible (subject-matched) hits: {len(shortlist)}")
