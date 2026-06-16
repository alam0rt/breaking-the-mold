#!/usr/bin/env python3
"""Test level names, role-vocab combos, and Neverhood-prefix combinations
against ALL 658 unknown asset IDs.
"""

def calc_hash(s):
    h = 0; sh = 0
    for c in s.upper():
        if 'A' <= c <= 'Z':
            sh = (sh + ord(c) - 64) & 31
            h ^= 1 << sh
        elif '0' <= c <= '9':
            sh = (sh + ord(c) + 22 - 64) & 31
            h ^= 1 << sh
    return h

def rotl(x, n):
    return ((x << n) | (x >> (32 - n))) & 0xFFFFFFFF

SEED = 0x28c0e011
def asset_id(s):
    return SEED ^ rotl(calc_hash(s), 27)

# Load all unknown IDs
with open('/tmp/all_ids.txt') as f:
    ALL_IDS = set(int(line.strip(), 16) for line in f if line.strip())

# Load known IDs
KNOWN = {
    "NO": 0x29c0e211,
    "YES": 0x2ad0f011,
    "PAUSED": 0x0ad0f813,
    "QUIT": 0x68c0f413,
    "CONTINUE": 0x69c04050,
    "QUITGAME": 0x69c8f473,
}
for n, id in KNOWN.items():
    assert asset_id(n) == id, f"Hash mismatch for {n}"

print(f"Targets: {len(ALL_IDS)} unknown IDs")

# Level names (4-char BLB codes)
LEVELS = ["BOIL","BRG1","CAVE","CLOU","CRYS","CSTL","EGGS","EVIL","FINN","FOOD",
          "GLEN","GLID","HEAD","KLOG","MEGA","MENU","MOSS","PHRO","RUNN","SCIE",
          "SEVN","SNOW","SOAR","TMPL","WEED","WIZZ"]

# Long-form level names from beta  
LEVEL_NAMES = [
    "Options", "SkullmonkeyGate", "Skullmonkey", "Gate",
    "ScienceCenter", "Science", "Center",
    "MonkeyShrines", "Shrines", "Shrine",
    "AmazingDriveyFloat", "AmazingDrivey", "Drivey", "AmazingFloat",
    "ShrineyGuard", "Shriney", "Guard",
    "HardBoiler", "Hard", "Boiler",
    "SkullmonkeyBrand", "Brand",
    "SkullmonkeyBrandHotpants", "Hotpants",
    "JoeHeadJoe", "Joe", "Head",
    "ElevatedStructure", "Elevated", "Structure",
    "YntDeathGarden", "Ynt", "Death", "Garden",
    "YntMines", "Mines",
    "YntWeeds", "Weeds",
    "YntEggs", "Eggs",
    "GlennYntis", "Glenn", "Yntis",
    "MonkRushmore", "Rushmore",
    "Nineties", "Seventies", "1970s",
    "SoarHead", "Soar",
    "Shards",
    "CastleDeLosMuertos", "Castle", "Muertos", "DeLosMuertos",
    "MonkeyMage", "Mage",
    "IncredibleDrivey", "Incredible",
    "WormGraveyard", "Worm", "Graveyard",
    "Klogg",
    "EvilEngine9", "Evil", "Engine", "EvilEngine",
]

# Symbols/roles  
ROLES = [
    "Cursor", "MenuCursor", "MainMenuCursor", "Pointer", "Arrow",
    "Number", "Numbers", "Digits", "Glyph", "Glyphs", "ScoreFont", "HudFont",
    "MainMenu", "TitleScreen", "Title",
    "Door", "Open", "Reveal", "RevealMonkey",
    "Monkey", "Monk", "Ape",
    "Boss", "Mega", "Wizz",
    "Level", "Stage", "World",
    "Sound", "Music", "Track",
    "Sprite", "Anim", "Sequence",
    "Master", "Bank", "Sheet",
    "Background", "Foreground", "BG", "FG",
]

# Internal vocab from prototype debug strings
INTERNAL = [
    "sprt", "sprite", "tile", "xtile", "f3", "f4", "g3", "g4", "ft4",
    "seq", "sequence", "image", "clut", "ram",
    "overlay", "master", "level", "sublevel", "sublevels",
    "bank", "atlas", "font", "fnt",
    "anim", "animation",
]

# Neverhood-style prefixes
PREFIXES = ["", "as", "ss", "Ss", "As", "sq", "me", "Mo", "Sc", "Km",
            "spr", "sprt", "snd", "snd_", "sprt_", "tile", "tile_",
            "anim", "anim_", "seq", "seq_", "fnt", "fnt_", "fnt_main",
            "ent", "ent_", "obj", "obj_", "ico", "ico_",
            "sk", "sm", "tlx", "ToolX",
            "Sk", "Sm", "BtM", "btm",
            "primary", "primary_", "prim_", "shared", "shared_",
            "lev_", "stage_", "world_",
            "ui_", "ui", "menu_", "menu",
            ]

# Build candidate list
all_words = LEVELS + LEVEL_NAMES + ROLES + INTERNAL + [
    # Translations
    "Cursor", "Curseur", "Cursor",
    # Common hash words from Neverhood asset metadata
    "asRecFont", "sqDefault", "meNumRows", "meFirstChar", 
    "meCharWidth", "meCharHeight", "meTracking",
    # Prefixed common
    "spr_cursor", "spr_monkey", "spr_door", "spr_score",
    "fnt_main", "fnt_score", "fnt_hud", "fnt_menu", "fnt_digit",
    "anim_intro", "anim_door", "anim_cursor", "anim_open",
    "main_menu", "menu_main", "title_screen", "title_main",
    # Sound names
    "snd_jump", "snd_hit", "snd_die", "snd_pickup",
    # Per-level bank vocabulary
    "primary", "secondary", "common", "shared", "global",
]
all_words = list(set(all_words))
print(f"\n{len(all_words)} base words")

# Test all combinations
found = []
total_tested = 0
for prefix in PREFIXES:
    for w in all_words:
        for sep in ["", "_", "-"]:
            for suffix in ["", "0", "1", "2", "01", "02", "_us", "_eu",
                           "Idle", "Active", "Loop", "Anim",
                           "_Idle", "_Active", "_Loop", "_Anim"]:
                cand = prefix + (sep if prefix and sep else "") + w + suffix
                if not cand: continue
                total_tested += 1
                aid = asset_id(cand)
                if aid in ALL_IDS:
                    found.append((cand, aid))

print(f"\nTested {total_tested:,} candidates")
print(f"Hits: {len(found)}")
for name, aid in found:
    if aid not in KNOWN.values():
        print(f"  0x{aid:08x}  {name!r}")

# Show known anchor confirmations too
print("\n== Known anchors confirmed ==")
for n, id in KNOWN.items():
    print(f"  0x{id:08x}  {n}")
