"""For each new candidate, validate by sister-checks."""
import sys, os, csv
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

uncracked = {}
cracked = {}
with open("docs/analysis/asset-identification/cracked_names.csv") as f:
    r = csv.DictReader(f)
    for row in r:
        try: hid = int(row["id_hex"], 16)
        except: continue
        if row["status"] == "uncracked":
            uncracked[hid] = (row["type"], row["levels"], int(row["floor"]))
        else:
            cracked[hid] = row.get("name","")

def stat(h):
    if h in cracked: return f"CRACKED ({cracked[h]})"
    if h in uncracked:
        t,l,fl = uncracked[h]
        return f"UNCRACKED ({t},[{l}],popc={fl})"
    return ""

candidates = [
    # CLOU level - soft footstep variants for cloud landing
    ("FX_KLAY_FOOTSTEP_QUIET_SOFT", 0x04dce858),
    ("FX_KLAY_FOOTSTEP_RIGHT_SOFT", 0x2470e0f2),
    ("FX_KLAY_RUN_FAST_SOFT", 0x0434ce72),
    ("FX_KLAY_FOOTSTEP_LEFT_SOFT", 0x401106da),
    ("FX_KMEN_ROLL_LEFT_DOWN", 0x428a6465),
    # 0x50f08207 - "HIT_HEAD" — but Falling context. Try sister actions
    ("FX_KLAY_FALL_LOOP", 0x50f08207),
    ("FX_KLAY_HIT_HEAD", 0x50f08207),
]

# Sisters for CLOU soft set
print("=== Cloud-level KLAY soft variants ===")
for name, h in candidates[:5]:
    print(f"  {name:35s} -> 0x{calcHash(name):08x}  {stat(calcHash(name))}")
print()

print("=== Soft variants of verified KLAY ===")
# FX_KLAY_LAND_SOFT (verified) and sisters
soft_check = [
    "FX_KLAY_LAND_SOFT",      # verified
    "FX_KLAY_FOOTSTEP_SOFT",
    "FX_KLAY_STEP_SOFT",
    "FX_KLAY_WALK_SOFT",
    "FX_KLAY_RUN_SOFT",
    "FX_KLAY_JUMP_SOFT",
    "FX_KLAY_FALL_SOFT",
    "FX_KLAY_HURT_SOFT",
    "FX_KLAY_DIE_SOFT",
    "FX_KLAY_FOOTSTEP_QUIET_SOFT",  # the discovered one
    "FX_KLAY_FOOTSTEP_LOUD_SOFT",
    "FX_KLAY_FOOTSTEP_LEFT_SOFT",
    "FX_KLAY_FOOTSTEP_RIGHT_SOFT",
    "FX_KLAY_FOOTSTEP_HARD_SOFT",
    "FX_KLAY_RUN_FAST_SOFT",
    "FX_KLAY_RUN_SLOW_SOFT",
    "FX_KLAY_WALK_FAST_SOFT",
    "FX_KLAY_WALK_SLOW_SOFT",
]
for n in soft_check:
    h = calcHash(n)
    s = stat(h)
    print(f"  {n:35s} -> 0x{h:08x}  {s}")

# Hit_head & fall sisters
print()
print("=== FALL / HIT_HEAD sisters for 0x50f08207 ===")
fall_check = [
    "FX_KLAY_FALL", "FX_KLAY_FALL_LOOP", "FX_KLAY_FALL_LOOP_01", "FX_KLAY_FALL_LOOP_02",
    "FX_KLAY_FALL_START", "FX_KLAY_FALL_END",
    "FX_KLAY_FALL_LONG", "FX_KLAY_FALL_SHORT",
    "FX_KLAY_FALL_HARD", "FX_KLAY_FALL_SOFT",
    "FX_KLAY_FALL_1", "FX_KLAY_FALL_2", "FX_KLAY_FALL_3",
    "FX_KLAY_FALLING", "FX_KLAY_FALLING_LOOP",
    "FX_KLAY_BOUNCE", "FX_KLAY_BOUNCE_LOOP",
    "FX_KLAY_HIT_HEAD", "FX_KLAY_HEAD_HIT", "FX_KLAY_HEAD_BUMP",
    "FX_KLAY_FALL_HEAD",
]
for n in fall_check:
    h = calcHash(n)
    s = stat(h)
    print(f"  {n:35s} -> 0x{h:08x}  {s}")

# Verify ROLL_LEFT_DOWN: that doesn't grammatically work. Look for ROLL variants
print()
print("=== FX_KMEN / FX_KLAY ROLL variants ===")
for r in ["FX_KMEN_ROLL_LEFT_DOWN","FX_KMEN_ROLL_LEFT","FX_KMEN_ROLL","FX_KMEN_ROLL_DOWN",
          "FX_KLAY_ROLL","FX_KLAY_ROLL_LOOP","FX_KLAY_ROLL_DOWN","FX_KLAY_ROLL_BOUNCE",
          "FX_KLAY_BARREL","FX_KLAY_BARREL_ROLL",
          "FX_KMEN_BARREL_ROLL","FX_KMEN_TUMBLE","FX_KMEN_SPIN",]:
    h = calcHash(r)
    s = stat(h)
    if s:
        print(f"  {r:35s} -> 0x{h:08x}  {s}")
    else:
        print(f"  {r:35s} -> 0x{h:08x}")

# Audit Init1970IconEntity context for 0x428a6465 (1970 icon = bonus item)
print()
print("=== 0x428a6465 sisters: 1970 icon variants ===")
# 1970 = bonus collectible icon. Try icon-related names
for n in [
    "FX_ICON_1970","FX_BONUS_1970","FX_1970_ICON","FX_1970",
    "FX_ICON_BONUS","FX_BONUS_COIN","FX_BONUS_ITEM",
    "FX_1970_PICKUP","FX_1970_GRAB","FX_1970_TAKE","FX_1970_GET",
    "FX_BONUS_GET","FX_BONUS_TAKE","FX_BONUS_PICKUP",
    "FX_BONUS_GRAB","FX_PICKUP_BONUS","FX_GET_BONUS",
    "FX_CHECKPOINT_1970","FX_CHECKPOINT",
    "FX_PICKUP","FX_GRAB","FX_GET","FX_TAKE","FX_COIN",
    "FX_COLLECT","FX_BONUS","FX_HEAD","FX_HEAD_PICKUP","FX_HEAD_GRAB",
    "FX_HEAD_GET","FX_BABY_HEAD","FX_BABY_PICKUP",
    "FX_KLAY_PICKUP","FX_KLAY_GRAB","FX_KLAY_TAKE","FX_KLAY_GET","FX_KLAY_COIN",
    "FX_KLAY_COLLECT","FX_KLAY_BONUS",
    "FX_KLAY_PICK_UP","FX_KLAY_PICKUP_1970","FX_KLAY_PICKUP_HEAD",
    "FX_KLAY_PICKUP_BONUS","FX_KLAY_PICKUP_ITEM","FX_KLAY_PICKUP_COIN",
]:
    h = calcHash(n)
    s = stat(h)
    if s and "UNCRACKED" in s and "0x428a6465" in s:
        print(f"  *** {n:35s} -> 0x{h:08x}  {s}")
    elif s:
        print(f"      {n:35s} -> 0x{h:08x}  {s}")
