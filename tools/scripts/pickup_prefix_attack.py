"""Hypothesis: sprite/anim shares the suffix _PICKUP_<NAME> from the verified FX_ sound,
but with a different prefix (SPR_, ICON_, ANIM_, AS_, S_, A_, BLB_, etc.)

Test all prefixes × all known pickup tails. Check every hit against the master uncracked set.
"""
import sys, os
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

# Load all asset IDs (cracked + uncracked) and their status/levels/types
all_ids = {}  # id -> (status, type, levels)
with open('docs/analysis/asset-identification/cracked_names.csv') as f:
    next(f)
    for line in f:
        parts = line.rstrip('\n').split(',')
        if len(parts) < 6: continue
        try:
            hid = int(parts[0], 16)
        except ValueError:
            continue
        all_ids[hid] = (parts[5], parts[3], parts[4])

# Roots from verified FX_PICKUP_<X> + FX_<other> + key items
ROOTS = [
    # Pickups
    "PICKUP_ONE_UP", "ONE_UP", "ONEUP", "1UP",
    "PICKUP_GROW", "GROW", "SCALE_RESET", "SCALERESET",
    "PICKUP_PHOENIX", "PHOENIX", "PHOENIX_HAND", "PHOENIXHAND",
    "PICKUP_FARTHEAD", "FARTHEAD", "FART_HEAD", "PHART_HEAD", "PHARTHEAD", "PHART", "FART",
    "PICKUP_SHIELD", "SHIELD", "HALO", "PICKUP_HALO",
    "PICKUP_GLIDEY", "GLIDEY", "YELLOW_BIRD", "YELLOWBIRD", "BIRD",
    "PICKUP_SUPER_WILLIE", "SUPER_WILLIE", "SUPERWILLIE",
    "PICKUP_UNIVERSE_ENEMA", "UNIVERSE_ENEMA", "UNIVERSEENEMA", "ENEMA",
    "PICKUP_1970", "1970", "HAMSTER", "HAMSTER_SHIELD", "HAMSTERSHIELD",
    # Willie variants (Neverhood character)
    "PICKUP_WILLIE", "WILLIE", "WILLY", "PICKUP_WILLY", "TROMBONE", "WILLIE_TROMBONE",
    "PICKUP_WILLIE_TROMBONE", "PICKUP_TROMBONE",
    # Player + entities
    "KLAYMEN", "KLAY", "FINN", "BIRD", "RAT",
    # Other audio family roots
    "MENU_SELECT", "MENU_PASSWORD",
    "BOSS_HEAD", "SKULL_GRUNT", "SKULL_SCREAM", "SKULL_LAND", "SKULL_FIRE",
    "PUFF_FALL",
    # Generic pickups
    "EXTRAS", "EXTRA", "EXTRA_LIFE", "EXTRALIFE", "LIFE", "ONEUPLIFE",
    "POWER", "POWERUP", "POWER_UP", "BONUS", "ITEM", "DROP",
]

# Prefixes - asset-class indicators common to ToolX / Neverhood
PREFIXES = [
    "", "FX_", "SFX_", "SND_",  # audio (for sanity check verification)
    "SPR_", "SP_", "S_", "SS_",
    "ANIM_", "AN_", "A_", "AS_",
    "ICON_", "ITEM_", "BLB_", "ENT_",
    "BG_", "FG_", "OBJ_", "OB_",
    "HUD_", "GFX_", "MSC_", "MC_",
    "AS", "SS",       # smushed
    "AS_PICKUP_",     # 2-level smush
    "ICON_PICKUP_",
    "SPRITE_",
    "ASS_",           # archaic
    "PLR_", "PLR", "PL_", "PL",
    # ToolX naming class prefixes from Neverhood (sister-game): "asXxx", "ssXxx", "kmXxx", "scXxx", "moXxx"
    "AS", "SS", "KM", "SC", "MO", "MS",
    "ASS", "SSS", "KMS", "SCS", "MOS",
    # Single letters
    "P_", "I_", "E_", "T_", "G_", "M_", "X_", "F_", "C_", "D_",
]

# Suffixes - frame/state markers
SUFFIXES = [
    "", "_01", "_02", "_03", "_1", "_2", "_3",
    "_A", "_B", "_C",
    "_ICON", "_SPR", "_ANIM", "_BLB",
    "_NEW", "_OK", "_GO", "_GET",
    "_FLY", "_PWR", "_PWRUP", "_PWR_UP",
    "_BIG", "_SM",
]

# Generate all combinations + check
hits = []
seen = set()
total = 0
for pre in PREFIXES:
    for root in ROOTS:
        for suf in SUFFIXES:
            name = pre + root + suf
            if name in seen: continue
            seen.add(name)
            total += 1
            h = calcHash(name)
            if h in all_ids:
                status, typ, lvl = all_ids[h]
                hits.append((h, name, status, typ, lvl))

print(f"Tested {total} candidates ({len(seen)} unique strings)")
print(f"Found {len(hits)} matches")
print()

# Sort by status (uncracked first, then verified)
hits.sort(key=lambda x: (0 if x[2] == 'uncracked' else 1, x[0]))

print("=== UNCRACKED hits (new candidates!) ===")
for h, n, status, typ, lvl in hits:
    if status == 'uncracked':
        print(f"  0x{h:08x} [{typ:8s}] {n!r}")
        print(f"             levels: {lvl[:80]}")

print()
print("=== VERIFIED hits (sanity check) ===")
for h, n, status, typ, lvl in hits:
    if status == 'verified':
        print(f"  0x{h:08x} [{typ:8s}] {n!r}")
