"""Derive collectible/pickup ASSET (sprite) names from verified FX_PICKUP_<X> sound names.

The hypothesis: if FX_PICKUP_ONE_UP exists, the sprite asset is probably named
ONE_UP or PICKUP_ONE_UP - using the same root token.

Known collectible sprite IDs from InitX..Collectible factory chain:
  0x6A351094 = UniverseEnema sprite
  0x08624580 = TriggerCollectible100C sprite (single-frame decor)
  0x8C30008C = ScaleReset sprite (grow)
  0x88A28194 = 1970Icon sprite
  0x80E85EA0 = HamsterShield sprite
  0x902C0002 = SuperWillie sprite
  0xBB781481 = wake-decor sprite (DecorPlaySoundAndAnimate)
  0xBE68D0C6 / 0xB868D0C6 = death/explode debris (0x146ce002 trigger)
  0xB01C25F0 = death-grow animation entity

Also other tagged sprite IDs from collectible Init functions:
  0x0E3889BE = JoeHeadJoe landed/splat
  0x06958 0A0 = JoeHeadJoe ball splat
"""
import sys, os, csv
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

# Roots derived from verified FX_PICKUP/etc names
ROOTS = {
    "ONE_UP": "extra life (FX_PICKUP_ONE_UP)",
    "ONEUP": "extra life alt",
    "1UP": "extra life alt",
    "EXTRA_LIFE": "extra life alt",
    "EXTRALIFE": "extra life alt",
    "LIFE": "extra life short",
    "UNIVERSE_ENEMA": "screen-clear bomb (FX_PICKUP_UNIVERSE_ENEMA)",
    "UNIVERSEENEMA": "screen-clear alt",
    "ENEMA": "screen-clear short",
    "UE": "screen-clear shortest",
    "GLIDEY": "yellow bird (FX_PICKUP_GLIDEY)",
    "YELLOW_BIRD": "yellow bird alt",
    "YELLOWBIRD": "yellow bird alt",
    "BIRD": "yellow bird short",
    "PHOENIX": "phoenix hand (FX_PICKUP_PHOENIX)",
    "PHOENIX_HAND": "phoenix hand full",
    "PHOENIXHAND": "phoenix hand alt",
    "HAND": "phoenix hand short",
    "FARTHEAD": "phart head (FX_PICKUP_FARTHEAD)",
    "PHART_HEAD": "phart head official",
    "PHARTHEAD": "phart head alt",
    "PHART": "phart short",
    "FART_HEAD": "phart head alt",
    "FARTHEAD_ICON": "phart head icon",
    "SUPER_WILLIE": "super willie (FX_PICKUP_SUPER_WILLIE)",
    "SUPERWILLIE": "super willie alt",
    "WILLIE": "willie short",
    "GROW": "scale reset (FX_PICKUP_GROW)",
    "SCALE_RESET": "scale reset official",
    "SCALERESET": "scale reset alt",
    "SHIELD": "halo/shield (FX_PICKUP_SHIELD)",
    "HALO": "halo alt",
    "1970": "1970 icon (FX_PICKUP_1970)",
    "1970_03": "1970 third (FX_PICKUP_1970_03)",
    "HAMSTER": "hamster shield",
    "HAMSTER_SHIELD": "hamster shield full",
    "HAMSTERSHIELD": "hamster shield alt",
    "PUFF": "puffer (FX_PUFF_FALL_3)",
    "PUFFER": "puffer alt",
    "PUFFY": "puffer alt2",
    "SKULL": "skull bot (FX_SKULL_*)",
    "SKULL_BOT": "skull bot alt",
    "SKULLBOT": "skull bot alt",
    "MENU_PASSWORD": "menu password (FX_MENU_PASSWORD)",
    "PASSWORD": "password screen",
    "PWD": "password short",
    # Common variants
    "SWIRLY": "swirly Q",
    "SWIRLY_Q": "swirly Q full",
    "ORB": "clay ball",
    "CLAY_BALL": "clay ball full",
    "CLAYBALL": "clay ball alt",
    "JINGLE": "level-jingle",
    "MA": "ma bird",
    "MA_BIRD": "ma bird full",
    "EGGY": "eggy",
    "EGGSHELL": "egg",
    "GUM": "gum/poop",
    "BIRD_PICKUP": "bird pickup",
    "ICON": "icon",
    "LIFE_ICON": "life icon",
    "1UP_ICON": "1up icon",
    "TROPHY": "trophy",
    "MEDAL": "medal",
    "AWARD": "award",
}

# Possible prefixes that might wrap the root
PREFIXES = [
    "",            # bare
    "SPR_", "SPRITE_",
    "ICON_",
    "PICKUP_", "PWR_", "POWER_", "POWERUP_", "PWRUP_",
    "ITEM_", "BONUS_",
    "ASSET_", "AS_",
    "COLLECT_", "COLLECTIBLE_",
    "PWR_PICKUP_", "POWER_PICKUP_",
    "ENT_", "ENTITY_",
    "PLR_", "PLAYER_",
    "HUD_",
    "ANIM_", "AN_",
    "BLB_", "BLB_PICKUP_",
    "FX_PICKUP_",  # to confirm the FX names we already cracked map back
]

# Also try with suffixes
SUFFIXES = [
    "",
    "_ICON", "_SPR", "_SPRITE", "_PWR", "_POWER", "_PICKUP",
    "_01", "_02", "_03", "_1", "_2", "_3",
    "_FX", "_ANIM", "_BLB", "_BLBED",
    "_NEW", "_GAINED", "_GET", "_GRAB", "_TAKE",
    "_OK", "_GO", "_WIN",
    "_LIVE", "_ACTIVE", "_ON", "_OFF",
    "_HUD", "_HEAD", "_BODY",
    "_RING", "_HALO", "_AURA",
]

# Load uncracked + cracked from CSV
uncracked = {}
cracked = {}
with open("docs/analysis/asset-identification/cracked_names.csv") as f:
    r = csv.DictReader(f)
    for row in r:
        try:
            hid = int(row["id_hex"], 16)
        except Exception:
            continue
        if row["status"] == "uncracked":
            uncracked[hid] = (row["type"], row["levels"], int(row["floor"]))
        else:
            cracked[hid] = row.get("name", "")

# Also load master asset table to see sprite IDs
master = {}
master_path = "docs/reference/asset-ids-master.csv"
if os.path.exists(master_path):
    with open(master_path) as f:
        r = csv.DictReader(f)
        for row in r:
            try:
                hid = int(row.get("id_hex", row.get("id", "")), 16)
                master[hid] = row
            except Exception:
                pass

# Generate candidates
hits = {}
total = 0
seen = set()
for root, label in ROOTS.items():
    for pre in PREFIXES:
        for suf in SUFFIXES:
            n = pre + root + suf
            if n in seen:
                continue
            seen.add(n)
            total += 1
            h = calcHash(n)
            if h in uncracked:
                hits.setdefault(h, []).append((n, label))
            elif h in cracked:
                # report only if it confirms something interesting
                if any(c in n for c in ("PICKUP", "ENEMA", "WILLIE", "GLIDEY", "PHOENIX", "FARTHEAD", "GROW", "PASSWORD", "1970")):
                    pass  # already known mapping

print(f"Tried {total} unique candidates against {len(uncracked)} uncracked + {len(cracked)} cracked")
print(f"Found {len(hits)} uncracked-id hits\n")

# Print hits sorted by id type and level count
def stat(h):
    t, lv, fl = uncracked[h]
    nl = len(lv.split(";")) if lv else 0
    return t, nl, fl

def lvls(s):
    return len(s.split(';')) if s else 99

sorted_hits = sorted(hits.items(), key=lambda x: stat(x[0]))
for hid, names in sorted_hits:
    t, lv, fl = uncracked[hid]
    print(f"0x{hid:08x} {t:7s} popc={fl:2d} levels({lvls(lv):2d})=[{lv[:60]:60s}]")
    seen_n = set()
    for n, label in names:
        if n in seen_n:
            continue
        seen_n.add(n)
        print(f"  {n:35s}   ({label})")
    print()
