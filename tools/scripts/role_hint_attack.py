#!/usr/bin/env python3
"""Precision attack on the 8 unconfirmed role-annotated IDs.

For each ID with a human-written description, try every plausible naming
convention seen in 90s game asset pipelines.
"""
import sys
sys.path.insert(0, 'tools/scripts')
from compound_hash_attack import asset_id

# Targets with their roles (from name-game manifest.js)
TARGETS = {
    0x00e2f188: ("Number glyph frames 0-9", "ALL"),
    0x33808e1b: ("Menu cursor animation frame", "MENU"),
    0x4a3ea110: ("Menu door opens to reveal monkey animation", "MENU"),
    0x4aee0118: ("Shriney Guard boss slamming down animation", "MEGA"),
    0x4baf8200: ("Shriney Guard boss yelling animation", "MEGA"),
    0x61981a0c: ("Plain monkey enemy", "ENEMY"),
    0x63848e59: ("Menu cursor", "MENU"),
    0x75189608: ("Plain monkey enemy variant", "ENEMY"),
}

# Vocabulary buckets per concept
NUMBER_WORDS = ["NUMBER", "NUMBERS", "NUM", "NUMS", "DIGIT", "DIGITS",
                "GLYPH", "GLYPHS", "FONT", "FONTS", "TEXT", "CHARS",
                "NUMGLYPH", "NUMFONT", "FONTNUM", "DIGITGLYPH", "NUMERAL",
                "NUMERALS", "INT", "INTEGER", "COUNTER", "SCORE",
                "NUMFRAMES", "NUMS09", "NUM09", "DIGITS09", "FONT09",
                "HUDFONT", "HUDNUM", "SCOREFONT", "SCORENUM", "SCORENUMS",
                "NUMERIC", "NUMERICALGLYPH", "GAMEFONT", "GAMENUM",
                "TINYNUM", "BIGNUM", "BIGNUMS", "HUDDIGITS", "HUDNUMS",
                "DIGIT0", "ZEROTONINE", "FONT0", "FONT09", "FONTDIGITS"]

CURSOR_WORDS = ["CURSOR", "POINTER", "ARROW", "SELECT", "SELECTOR",
                "HIGHLIGHT", "MENUCURSOR", "CURSOR1", "CURSOR0", "MARKER",
                "MENUSELECT", "MENUARROW", "MENUPOINTER", "MENUMARKER",
                "MENUHIGHLIGHT", "CURSORANIM", "ARROWANIM", "POINTERANIM",
                "MENUSELECTOR", "OPTIONSCURSOR", "OPTIONCURSOR",
                "PAUSECURSOR", "PAUSEARROW", "PAUSEPOINTER",
                "BIGCURSOR", "TINYCURSOR", "CURSOR_IDLE", "CURSOR_BLINK",
                "CURSORBLINK", "CURSORIDLE", "CURSORLOOP", "FRAMECURSOR",
                "CURSORFRAME"]

DOOR_WORDS = ["DOOR", "DOORS", "DOOROPEN", "OPENDOOR", "DOORANIM", "DOORINTRO",
              "MENUDOOR", "DOORLOAD", "OPENING", "REVEAL", "INTRO",
              "INTROANIM", "MENUINTRO", "TITLEDOOR", "DOORREVEAL",
              "MAINDOOR", "MENUDOORS", "OPENMENU", "MAINMENUOPEN",
              "TITLEDOORS", "GAMEDOOR", "MENUOPEN", "MAINMENUDOOR",
              "MENULOAD", "MENULOADANIM"]

SHRINEY_WORDS = ["SHRINEY", "SHRINEYGUARD", "GUARD", "BOSS", "MEGA",
                 "MEGABOSS", "MEGAGUARD", "STONEHEAD", "BIGSTONE",
                 "STONEFACE", "GIANTHEAD", "TIKI", "TIKIBOSS", "STATUE",
                 "STATUEBOSS", "MONKHEAD", "MONKEYHEAD"]

ATTACK_WORDS = ["SLAM", "SLAMDOWN", "ATTACK", "ATTACKDOWN", "POUND",
                "STOMP", "CRUSH", "STRIKE", "SMASH", "HIT", "PUMMEL",
                "PUNCH", "BASH", "FALL", "CRASH", "DESCEND", "PRESS",
                "SLAMANIM", "ATTACKANIM", "SMASHDOWN"]

YELL_WORDS = ["YELL", "SCREAM", "SHOUT", "ROAR", "ROARING", "YELLING",
              "SCREAMING", "SHOUTING", "TAUNT", "TAUNTING", "ANGRY",
              "MAD", "RAGE", "RAGING", "GROWL", "GROWLING", "BARK",
              "BARKING", "YELLANIM", "ROARANIM", "SCREAMANIM",
              "TAUNTANIM", "OPEN", "OPENMOUTH", "MOUTHOPEN"]

MONKEY_WORDS = ["MONKEY", "MONK", "MONKE", "MUTON", "APE", "PRIMATE",
                "MONKEYENEMY", "MONKENEMY", "MONKEYWALK", "MONKWALK",
                "BASICMONKEY", "PLAINMONKEY", "PLAINMONK", "GENERICMONKEY",
                "GRUNT", "GRUNTMONKEY", "MOOK", "BADMONKEY", "EVILMONKEY",
                "MONKEYBOT", "MONKBOT", "MUTON", "MUTONS",
                "DUMBMONKEY", "MONKMOOK", "WALKMONKEY", "WALKMONK"]

VARIANT_WORDS = ["BIG", "SMALL", "TINY", "GIANT", "TALL", "FAT", "SLIM",
                 "RED", "BLUE", "GREEN", "YELLOW", "PURPLE", "WHITE",
                 "BLACK", "DARK", "LIGHT", "FAST", "SLOW", "STRONG",
                 "WEAK", "JUMPING", "JUMPER", "FLYING", "FLY",
                 "ARMORED", "BOSS", "ELITE", "MEAN", "TOUGH",
                 "VARIANT", "TYPE2", "ALT", "VAR", "SECOND", "OTHER",
                 "ENEMY2", "MONKEY2", "MONK2"]

# Build per-target candidates
def gen_candidates(role, level):
    cands = set()
    if "Number" in role or "glyph" in role:
        for w in NUMBER_WORDS:
            cands.add(w)
    if "cursor" in role.lower() or "Menu" in role:
        for w in CURSOR_WORDS:
            cands.add(w)
        for w in CURSOR_WORDS:
            cands.add("MENU_" + w)
            cands.add("MAIN_" + w)
    if "door" in role.lower():
        for w in DOOR_WORDS:
            cands.add(w)
        for w in DOOR_WORDS:
            cands.add("MENU_" + w)
            cands.add("MAIN_" + w)
            cands.add("TITLE_" + w)
    if "Shriney" in role:
        for s in SHRINEY_WORDS:
            for a in ATTACK_WORDS if "slam" in role.lower() else YELL_WORDS:
                cands.add(s + a)
                cands.add(s + "_" + a)
                cands.add(a + s)
                cands.add(a + "_" + s)
                cands.add(s)
                cands.add(a)
    if "monkey enemy" in role.lower():
        for m in MONKEY_WORDS:
            cands.add(m)
            for a in ["IDLE", "WALK", "RUN", "ATTACK", "DEATH", "JUMP",
                     "STAND", "TURN"]:
                cands.add(m + a)
                cands.add(m + "_" + a)
                cands.add(a + m)
                cands.add(a + "_" + m)
    if "variant" in role.lower():
        for m in MONKEY_WORDS:
            for v in VARIANT_WORDS:
                cands.add(m + v)
                cands.add(v + m)
                cands.add(m + "_" + v)
                cands.add(v + "_" + m)
    return cands

# Run
total = 0
for tid, (role, level) in TARGETS.items():
    cands = gen_candidates(role, level)
    print(f"\n=== 0x{tid:08x}  {role}  ({level})  -- {len(cands)} candidates ===")
    total += len(cands)
    found = []
    for c in cands:
        if asset_id(c) == tid:
            found.append(c)
    if found:
        print("  *** HITS ***")
        for h in found:
            print(f"    {h}")
    else:
        print("  (no hit)")

print(f"\nTotal candidates tested: {total}")
