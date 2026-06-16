#!/usr/bin/env python3
"""Targeted attack on visual-labeled high-priority asset IDs.

For each labeled ID, generate a focused vocabulary based on the visual role
(menu cursor, Shriney boss, Klaymen running, etc.) and test all 1-3 token
compounds against THAT specific ID's hash.

This is more effective than dictionary attacks because we're using
domain-specific knowledge from sprite identifications.

Output: docs/analysis/asset-identification/targeted_hits.csv
"""
from __future__ import annotations
import csv
import sys
import time
from collections import defaultdict
from itertools import product
from pathlib import Path

sys.path.insert(0, "tools/scripts")
from compound_hash_attack import asset_id, calc_hash_and_shift, rotl, rotr

ROOT = Path(__file__).resolve().parents[2]
SEED = 0x28C0E011
OUT_ROT = 27

# ============================================================
# Targets: high-priority IDs with visual labels
# Format: (id_hex, floor, level, role_keywords, candidate_tokens)
# ============================================================

# Common modifiers
ANIMATION_SUFFIXES = "ANIM ANIMATION FRAME FRAMES LOOP CYCLE SEQ SEQUENCE BLINK FLASH WAIT IDLE".split()
DIRECTION_SUFFIXES = "L R U D LEFT RIGHT UP DOWN LR UD".split()
NUMERIC_SUFFIXES = ["", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
                    "00", "01", "02", "03", "04", "05", "06", "07", "08", "09", "10"]
ASSET_TYPE_SUFFIXES = "ICON SPRITE SPR PIC IMAGE IMG PNG TEX BG FX SFX ANIM SEQ".split()
GENERIC_SUFFIXES = "MAIN BASE NEW OLD ALT VARIANT BIG SMALL".split()

# State suffixes (verbs/states)
STATE_SUFFIXES = """
WALK RUN JUMP FALL LAND DIE DEATH HURT HIT
ATTACK SHOOT FIRE SLAM BOUNCE OPEN CLOSE START STOP
TURN EAT SLEEP ROLL SLIDE FLY GLIDE SPIN SPIT YELL SCREAM
TAUNT VICTORY DANCE IDLE REST WAIT STAND
WALKING RUNNING JUMPING FALLING LANDING DYING SHOOTING SHOT
ATTACKING SLAMMING BOUNCING TURNING ROLLING SLIDING FLYING GLIDING
SPINNING SPITTING YELLING SCREAMING HOVERING FLOATING
SPAWN SPAWNS ENTRY ENTER EXIT
""".split()

# ============================================================
# Visual-labeled targets (from attack_priorities.csv + sprite_identification_template)
# ============================================================
TARGETS = [
    # ID, floor, level, role, focused_vocab
    ("0x33808e1b", 12, "MENU", "menu cursor",
     ["CURSOR", "MENUCURSOR", "MENU_CURSOR", "POINTER", "ARROW", "SELECTOR",
      "CURSORANIM", "CURSORLOOP", "CURSORFRAME", "CURSORBLINK", "CURSORFLASH",
      "CURSORWAIT", "CURSORIDLE", "CURSORHILITE", "CURSORHIGHLIGHT",
      "MENUARROW", "MENUPOINTER", "ARROWANIM", "POINTERANIM",
      "PASSWORDCURSOR", "OPTIONSCURSOR", "PAUSECURSOR",
      "MENUSELECT", "MENUSELECTOR", "MENUACTIVE", "MENUACTIVECURSOR",
      "OPTIONSARROW", "PAUSEARROW", "PASSWORDARROW",
      "CURSOR_FLASH", "FLASHCURSOR", "BLINKCURSOR",
      "MENU_FONT", "MENUFONT", "FONT_CURSOR", "FONTCURSOR",
      "PRESSSTART", "PRESS_START", "STARTBUTTON", "STARTBLINK",
      "ANIMCURSOR", "SPRITECURSOR", "SPRCURSOR", "ICONCURSOR",
      ]),

    ("0x63848e59", 13, "MENU", "menu cursor variant",
     ["MENUCURSOR", "CURSORMENU", "PAUSEMENUCURSOR", "OPTIONSMENUCURSOR",
      "PASSWORDMENUCURSOR", "ARROWCURSOR", "POINTERCURSOR",
      "CURSORHILITE", "CURSORHIGHLIGHT", "MENUSELECTANIM", "MENUACTIVECURSOR",
      "MENUCURSORANIM", "MENUCURSORLOOP", "MENUCURSORFRAME",
      "MENUCURSORFLASH", "MENUCURSORBLINK", "MENUCURSORFRAMES",
      "OPTIONSCURSOR", "PASSWORDCURSOR", "PAUSECURSOR",
      "OPTIONS_CURSOR", "PASSWORD_CURSOR", "PAUSE_CURSOR",
      "MENU_CURSOR_ANIM", "MENU_CURSOR_LOOP", "MENU_CURSOR_FRAME",
      "MENU_ACTIVE_CURSOR", "MENU_HIGHLIGHT", "MENU_SELECT_CURSOR",
      "PRESSSTARTBLINK", "PRESS_START_BLINK", "TITLECURSOR", "TITLE_CURSOR",
      ]),

    ("0x68c01218", 8, "MENU", "menu sprite font",
     ["FONT", "MENUFONT", "MENU_FONT", "TEXTFONT", "TEXT_FONT",
      "DIGITS", "DIGIT", "GLYPH", "GLYPHS", "NUMBER", "NUMBERS", "NUM",
      "FONT0", "FONT1", "FONT00", "FONT01",
      "FONTSML", "FONTBIG", "FONTSMALL", "FONTLARGE",
      "FLASHFONT", "FLASH_FONT", "BLINKFONT", "BLINK_FONT",
      "MENUTEXT", "MENU_TEXT", "MENUDIGIT", "MENU_DIGIT",
      "FONTA", "FONTB", "FONTC", "FONTD",
      "TEXTA", "TEXTB", "TEXTC",
      "TITLE", "TITLEFONT", "OPTIONSFONT", "PAUSEFONT", "PAUSE_FONT",
      ]),

    # MEGA Shriney Guard boss
    ("0x4aee0118", 13, "MEGA", "shriney guard slamming",
     ["SHRINEY", "SHRINEYGUARD", "SHRINEY_GUARD", "GUARD", "MEGAGUARD",
      "SHRINEYSLAM", "SHRINEYSLAMMING", "SHRINEYATTACK", "SHRINEYDOWN",
      "SHRINEY_SLAM", "SHRINEY_ATTACK", "SHRINEY_DOWN",
      "SHRINEYGUARDSLAM", "SHRINEYGUARDATTACK",
      "GUARDSLAM", "GUARD_SLAM", "GUARDATTACK", "GUARD_ATTACK",
      "BOSSSLAM", "BOSS_SLAM", "BOSSDOWN", "BOSS_DOWN",
      "MEGABOSS", "MEGA_BOSS", "MEGABOSSSLAM",
      "GROUNDPOUND", "POUNDGROUND", "JUMPSLAM", "JUMP_SLAM",
      "SHRINEY_GUARD_SLAM", "SHRINEY_GUARD_ATTACK",
      "SHRINEY_LAND", "SHRINEY_LANDED", "SHRINEY_FALL",
      "MEGAEND", "MEGAEND_BOSS", "ENDBOSS", "END_BOSS",
      ]),

    ("0x4baf8200", 15, "MEGA", "shriney guard yelling",
     ["SHRINEYYELL", "SHRINEY_YELL", "SHRINEYSCREAM", "SHRINEY_SCREAM",
      "SHRINEYGUARDYELL", "SHRINEY_GUARD_YELL", "SHRINEY_GUARD_SCREAM",
      "GUARDYELL", "GUARD_YELL", "GUARDSCREAM", "GUARD_SCREAM",
      "BOSSYELL", "BOSS_YELL", "BOSSSCREAM", "BOSS_SCREAM",
      "MEGAYELL", "MEGA_YELL", "MEGABOSSYELL", "MEGA_BOSS_YELL",
      "SHRINEYTAUNT", "SHRINEY_TAUNT", "GUARDTAUNT",
      "SHRINEYANGRY", "SHRINEY_ANGRY",
      "SHRINEYGUARDYELLING", "SHRINEYYELLING",
      "ROAR", "SHRINEYROAR", "SHRINEY_ROAR", "GUARDROAR",
      "SHRINEYWARCRY", "SHRINEY_WARCRY", "SHRINEY_INTRO",
      ]),

    # GLEN boss (large enemy creature)
    ("0xc8f90114", 13, "GLEN", "glen boss",
     ["GLEN", "GLENBOSS", "GLEN_BOSS", "GLENN", "GLENNBOSS",
      "GLENBADDIE", "GLEN_BADDIE", "BADDIE",
      "GLENMONSTER", "GLEN_MONSTER", "MONSTER",
      "GLENCREATURE", "GLEN_CREATURE", "CREATURE",
      "GLENGIANT", "GLEN_GIANT", "GIANT",
      "GLENENEMY", "GLEN_ENEMY", "ENEMY",
      "PHARTHEAD", "PHART_HEAD", "PHART", "FARTHEAD", "FART_HEAD",
      "FARTBOMB", "FART_BOMB",
      "GLENPHART", "GLEN_PHART", "PHARTGLEN",
      "GLENBADGUY", "GLEN_BADGUY", "BADGUY",
      "MAGE", "GLENMAGE", "GLEN_MAGE",
      "GLENNBOSS", "GLENN_BOSS",
      ]),

    # Plain monkey (8 levels)
    ("0x61981a0c", 16, "BOIL/etc", "plain monkey enemy",
     ["MONKEY", "MONKEYS", "PLAINMONKEY", "PLAIN_MONKEY",
      "BASEMONKEY", "BASE_MONKEY", "GENERICMONKEY",
      "SCREAMER", "SCREAMERMONKEY", "SCREAMER_MONKEY",
      "ANGRYMONKEY", "ANGRY_MONKEY", "MONKEYANGRY",
      "MONKEYWALK", "MONKEY_WALK", "WALKINGMONKEY",
      "MONKEYIDLE", "MONKEY_IDLE", "IDLEMONKEY",
      "BARKINGBIRD", "MONKEYBARK", "MONKEY_BARK",
      "BARKMONKEY", "BARK_MONKEY",
      "POPCORNSKULL", "POPCORN_SKULL", "POPCORN", "SKULL",
      "MONKEYSCREAM", "MONKEY_SCREAM", "SCREAMINGMONKEY",
      "WORKERYNT", "WORKER_YNT", "YNT", "WORKERMONKEY", "WORKER_MONKEY",
      "GENERICENEMY", "BASIC_ENEMY", "BASEENEMY",
      "ROAMER", "WANDERER", "ENEMYWALK", "ENEMY_WALK",
      ]),

    ("0x75189608", 17, "BOIL/etc", "monkey enemy variant",
     ["MONKEYVARIANT", "MONKEY_VARIANT", "MONKEY2", "MONKEY3",
      "MONKEYALT", "MONKEY_ALT",
      "MONKEYDIE", "MONKEY_DIE", "MONKEYDEATH", "MONKEY_DEATH",
      "MONKEYHURT", "MONKEY_HURT", "MONKEYDAMAGED",
      "MONKEYSPAWN", "MONKEY_SPAWN", "MONKEYAPPEAR",
      "MONKEYJUMP", "MONKEY_JUMP", "MONKEYATTACK", "MONKEY_ATTACK",
      "MONKEYIDLEALT", "MONKEY_IDLE_ALT",
      "SCREAMERMONKEYDIE", "SCREAMER_MONKEY_DIE",
      "POPCORNSKULLDIE", "POPCORN_SKULL_DIE",
      "BARKINGBIRDDIE", "BARKING_BIRD_DIE",
      "MONKEYRUN", "MONKEY_RUN", "MONKEYWALK", "MONKEY_WALK",
      "MONKEYTURN", "MONKEY_TURN",
      ]),

    # LOADING screen (all levels)
    ("0x8ab92024", 14, "ALL", "loading screen + klaymen",
     ["LOADING", "LOAD", "LOADSCREEN", "LOAD_SCREEN", "LOADINGSCREEN",
      "LOADING_SCREEN", "LOADANIM", "LOAD_ANIM", "LOADINGANIM",
      "PLEASE_WAIT", "PLEASEWAIT", "WAITSCREEN", "WAIT_SCREEN",
      "LOADKLAYMEN", "LOAD_KLAYMEN", "KLAYMENLOAD", "KLAYMEN_LOAD",
      "LOADINGKLAYMEN", "LOADING_KLAYMEN",
      "LOADBG", "LOAD_BG", "LOADBACKGROUND", "LOAD_BACKGROUND",
      "LOADTEXT", "LOAD_TEXT", "LOADINGTEXT", "LOADING_TEXT",
      "TRANSITIONSCREEN", "TRANSITION", "TRANS",
      "LOADINGFRAME", "LOAD_FRAME",
      "LOADINGGFX", "LOAD_GFX",
      "LOADINGSPR", "LOAD_SPR",
      ]),

    # Number glyphs (audio?)
    ("0x00e2f188", 10, "ALL", "number glyph frames 0-9",
     ["DIGIT", "DIGITS", "NUMBER", "NUMBERS", "NUM", "NUMS",
      "DIGITSPRITE", "DIGIT_SPRITE", "DIGITICON", "DIGIT_ICON",
      "DIGIT0123456789", "0123456789",
      "FONTDIGIT", "FONT_DIGIT", "FONTDIGITS", "FONT_DIGITS",
      "FONTNUM", "FONT_NUM", "FONTNUMBERS", "FONT_NUMBERS",
      "GLYPHS", "GLYPH", "GLYPHSET", "GLYPH_SET",
      "TEXTDIGITS", "TEXT_DIGITS", "TEXTNUM", "TEXT_NUM",
      "SCOREFONT", "SCORE_FONT", "SCOREDIGITS", "SCORE_DIGITS",
      "COUNTERFONT", "COUNTER_FONT", "COUNTERDIGITS",
      "TIMERFONT", "TIMER_FONT", "TIMERDIGITS",
      "GAMEFONT", "GAME_FONT",
      "HUDFONT", "HUD_FONT", "HUDDIGITS", "HUD_DIGITS",
      "SCORE", "SCOREBG", "SCORE_BG",
      ]),

    # 0x90810000 - cohort C0006 (6 anchors!) - audio (all levels)
    ("0x90810000", 11, "ALL", "shared audio (6-anchor cohort)",
     ["MENU", "MENUSND", "MENU_SND", "MENUSFX", "MENU_SFX",
      "CONFIRM", "CONFIRMSND", "CONFIRM_SND",
      "SELECT", "SELECTSND", "SELECT_SND", "SELECTSFX", "SELECT_SFX",
      "OK", "OKSND", "OK_SND",
      "BEEP", "BEEPSND", "BEEP_SND", "BLIP",
      "CLICK", "CLICKSND", "CLICK_SND",
      "MOVECURSOR", "MOVE_CURSOR", "CURSORMOVE", "CURSOR_MOVE",
      "MENUMOVE", "MENU_MOVE", "MENUSELECT", "MENU_SELECT",
      "ENTERSND", "ENTER_SND", "BACKSND", "BACK_SND",
      "PAUSESND", "PAUSE_SND", "UNPAUSESND", "UNPAUSE_SND",
      "STARTSND", "START_SND",
      "ERROR", "ERRORSND", "ERROR_SND", "DENY", "DENYSND",
      "BUTTONSND", "BUTTON_SND", "BUTTONSFX",
      ]),
]


def load_targets():
    out = {}
    with (ROOT / "docs/reference/asset-ids-master.csv").open() as f:
        rdr = csv.DictReader(f)
        for r in rdr:
            i = int(r["id_hex"], 16)
            out[i] = r
    return out


def main():
    pool = load_targets()
    print(f"Loaded {len(pool)} asset IDs from master CSV")

    all_hits = []
    for tid_hex, floor, level, role, vocab in TARGETS:
        tid = int(tid_hex, 16)
        # Generate variations: each token + numeric/alpha suffixes
        candidates = set()
        for tok in vocab:
            candidates.add(tok)
            for n in NUMERIC_SUFFIXES:
                if n:
                    candidates.add(tok + n)
            for d in DIRECTION_SUFFIXES:
                candidates.add(tok + d)
                candidates.add(tok + "_" + d)
            for s in ASSET_TYPE_SUFFIXES:
                candidates.add(tok + s)
                candidates.add(tok + "_" + s)
                candidates.add(s + tok)

        # Test each
        hits = []
        for c in candidates:
            if asset_id(c) == tid:
                hits.append(c)
        print(f"\n0x{tid:08x}  floor={floor}  {level:10s} {role}")
        print(f"  tested {len(candidates):,} candidates")
        if hits:
            for h in hits:
                print(f"  *** HIT: {h}")
                all_hits.append((tid_hex, level, role, floor, h))
        else:
            print(f"  no hits")

    print(f"\n=== TOTAL: {len(all_hits)} hits ===")
    for h in all_hits:
        print(f"  {h[0]}  {h[1]:10s}  {h[3]:2d}ch  {h[4]}  ({h[2]})")

    out = ROOT / "docs/analysis/asset-identification/targeted_hits.csv"
    out.parent.mkdir(parents=True, exist_ok=True)
    with out.open("w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["id_hex", "level", "role", "floor", "name"])
        for r in all_hits:
            w.writerow(r)
    print(f"\nwrote {out.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
