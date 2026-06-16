#!/usr/bin/env python3
"""Combine level codes + role vocab + internal asset terms.

Try many naming patterns to find plausible asset names. We'll print
ALL hits then manually filter for plausibility.
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

with open('/tmp/all_ids.txt') as f:
    ALL_IDS = set(int(line.strip(), 16) for line in f if line.strip())

print(f"Targets: {len(ALL_IDS)} unknown IDs")

# Roles per level (from extracted/<LEVEL>/ contents and game knowledge)
LEVELS = {
    "MEGA": ["Skullmonkey", "Brand", "Hotpants", "Shriney", "Guard", "Slam", "Yell", "Boss", "Crush", "Statue"],
    "WIZZ": ["Wizz", "Mage", "MonkeyMage", "Wiz", "Wizard"],
    "EVIL": ["Evil", "Engine", "Engine9", "Boss"],
    "KLOG": ["Klogg", "Worm", "WormBoss", "Krohnk"],
    "BOIL": ["HardBoiler", "Boiler", "Boil", "Hot"],
    "BRG1": ["Bridge", "Brigade", "Br1", "Br2"],
    "CSTL": ["Castle", "Muertos", "Castillo"],
    "EGGS": ["Eggs", "Egg", "YntEggs", "Hatch"],
    "WEED": ["Weeds", "Weed", "YntWeeds", "Garden"],
    "MENU": ["Menu", "MainMenu", "Title", "Options"],
    "SCIE": ["Science", "ScienceCenter", "Sci"],
    "TMPL": ["Temple", "Shrine", "MonkeyShrines"],
    "RUNN": ["Run", "Running", "Drivey", "AmazingDrivey"],
    "FOOD": ["Food", "Cafe", "Diner"],
    "SOAR": ["Soar", "SoarHead", "Fly"],
    "SNOW": ["Snow", "Ice", "Cold"],
    "GLEN": ["Glenn", "Glen", "Yntis", "GlennYntis"],
    "GLID": ["Glide", "Float"],
    "FINN": ["Finn", "Fin"],
    "HEAD": ["JoeHeadJoe", "Joe", "Head"],
    "MOSS": ["MonkRushmore", "Rushmore", "Moss"],
    "PHRO": ["Phro", "Frog"],
    "SEVN": ["Seven", "Seventies", "Sevn"],
    "CAVE": ["Cave", "Caves", "Tunnel"],
    "CLOU": ["Cloud", "Clouds", "Sky"],
    "CRYS": ["Crystal", "Crys", "Gem"],
    "FOOD": ["Food", "Cafe"],
}

# Asset role vocab
ROLES = ["Cursor", "Pointer", "Arrow", "Hilight", "Hilite", "Highlight", "Active",
         "Idle", "Loop", "Anim", "Frame", "Cycle",
         "Boss", "Enemy", "Monkey", "Monk", "Player",
         "Door", "Open", "Reveal", "Intro", "Outro", "End",
         "Score", "Time", "Lives", "HUD", "Status",
         "Number", "Numbers", "Digit", "Digits", "Glyph", "Font", "Char",
         "Title", "Logo", "Flash",
         "Yell", "Shout", "Slam", "Hit", "Crush",
         "Walk", "Run", "Jump", "Fall", "Stand",
         "Dead", "Die", "Death", "Hurt", "Hurt",
         "Bg", "Background", "Fg", "Foreground",
         "Sky", "Cloud", "Ground", "Floor",
         "Door", "Window", "Wall",
         "Big", "Small", "Tall", "Short",
         ]

# Internal/tooling vocab from prototype debug strings  
INTERNAL = ["sprt", "spr", "seq", "tile", "xtile", "fnt", "image", "clut", "anim",
            "snd", "sfx", "obj", "ent",
            "f3", "f4", "g3", "g4", "ft4", "ft3", "ft", "sprite",
            "primary", "secondary", "common", "shared",
            "Master", "Sub", "Bank",
            ]

# Prefix conventions (Neverhood-derived)
PREFIXES = ["", "as", "ss", "ks", "Sc", "Mo", "Km", "sq", "me",
            "spr", "spr_", "sprt", "sprt_", "tile_", "xtile_",
            "fnt_", "fnt", "anim_", "anim", "seq_", "seq",
            "obj_", "obj", "ent_", "ent",
            "snd_", "snd",
            ]

# Build candidate set with multiple patterns
def gen_candidates():
    cands = set()
    
    # Pattern 1: just role names
    for r in ROLES:
        cands.add(r)
    
    # Pattern 2: level + role
    for lvl, role_list in LEVELS.items():
        for r in role_list + ROLES:
            cands.add(lvl + r)
            cands.add(lvl + "_" + r)
            cands.add(r + lvl)
            cands.add(r + "_" + lvl)
    
    # Pattern 3: prefix + level + role
    for p in PREFIXES:
        for lvl in LEVELS:
            for r in ROLES:
                cands.add(p + lvl + r)
                cands.add(p + r + lvl)
                if p:
                    cands.add(p + "_" + lvl + "_" + r)
                    cands.add(p + lvl + "_" + r)
    
    # Pattern 4: prefix + role + numerals
    for p in PREFIXES:
        for r in ROLES:
            for n in range(10):
                cands.add(p + r + str(n))
                if p:
                    cands.add(p + "_" + r + str(n))
            for n in range(10, 20):
                cands.add(p + r + str(n))
    
    # Pattern 5: internal vocab as base
    for i in INTERNAL:
        for r in ROLES:
            cands.add(i + r)
            cands.add(i + "_" + r)
            cands.add(r + i)
        for lvl in LEVELS:
            cands.add(i + lvl)
            cands.add(i + "_" + lvl)
            cands.add(lvl + i)
    
    return cands

cands = gen_candidates()
print(f"\n{len(cands):,} unique candidates")

# Test
hits = {}  # id -> list of names
for c in cands:
    if not c: continue
    aid = asset_id(c)
    if aid in ALL_IDS:
        hits.setdefault(aid, []).append(c)

print(f"\nIDs hit: {len(hits)}")

# Sort by ID for stable output, prefer plausible names
def plausibility(name):
    """Return higher for more plausible. Looks like real asset name."""
    score = 0
    # Lowercase prefix is good (Neverhood pattern)
    if name and name[0].islower(): score += 1
    # Has at least one underscore or camelCase 
    has_separator = "_" in name or any(c.isupper() for c in name[1:])
    if has_separator: score += 1
    # Length 6-20 is good
    if 6 <= len(name) <= 20: score += 1
    # Contains common vocab
    if any(v.upper() in name.upper() for v in ["BOSS", "CURSOR", "MENU", "BG", "FG"]):
        score += 1
    return score

print("\n=== Hits per ID (with plausibility scores) ===")
for aid in sorted(hits.keys()):
    candidates = hits[aid]
    candidates.sort(key=lambda x: -plausibility(x))
    print(f"\n0x{aid:08x}: {len(candidates)} candidates")
    for c in candidates[:5]:
        print(f"  [{plausibility(c)}] {c!r}")
