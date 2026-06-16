#!/usr/bin/env python3
"""Apply Neverhood ToolX naming convention to Skullmonkeys IDs.

ScummVM source confirms calcHash() takes literal strings like:
  - "sqDefault" (sequence)
  - "asRecFont" (animated sprite)
  - "meNumRows", "meFirstChar", "meCharWidth", "meCharHeight", "meTracking" (font metrics)

The Neverhood class naming reflects asset name prefixes:
  - Ss = Static Sprite (StaticSprite-derived class)
  - As = Animated Sprite (AnimatedSprite-derived class)
  - Km = Klaymen (player)
  - Sc = Scene
  - Mo = Module
  - sq = sequence/animation file
  - me = metric/parameter

Try these prefix conventions on our role-annotated targets.
"""
import sys
sys.path.insert(0, 'tools/scripts')
from compound_hash_attack import asset_id

# Verify
assert asset_id("NO") == 0x29c0e211
assert asset_id("YES") == 0x2ad0f011
assert asset_id("PAUSED") == 0x0ad0f813
assert asset_id("QUITGAME") == 0x69c8f473

TARGETS = {
    0x00e2f188: "Number glyph frames 0-9",
    0x33808e1b: "Menu cursor animation frame",
    0x4a3ea110: "Menu door opens to reveal monkey animation",
    0x4aee0118: "Shriney Guard boss slamming down animation",
    0x4baf8200: "Shriney Guard boss yelling animation",
    0x61981a0c: "Plain monkey enemy",
    0x63848e59: "Menu cursor",  # uses same vocab as cursor anim frame
    0x75189608: "Plain monkey enemy variant",
}
# Map "Menu cursor" to the cursor anim vocab
VOCAB_ALIAS = {"Menu cursor": "Menu cursor animation frame"}

# Neverhood-style prefixes  
PREFIXES = ["", "Sc", "Ss", "As", "Km", "Mo", "sq", "me", "as", "ss",
            "Sa", "Sb", "ssa", "asa", "Cs", "Cl", "Cm",  # additional class prefixes
            "spr", "snd", "anm", "obj", "ent", "ico", "fnt", "tex", "tile",
            "Sm", "Sk",  # Skullmonkey
            "S_", "A_", "M_", "F_", "Sm_", "Sk_",
            "p_", "P_",
            ]

# Vocabulary  
VOCAB = {
    "Number glyph frames 0-9": [
        "Numbers", "Number", "NumberGlyph", "NumberGlyphs", "Digits", "Digit",
        "DigitGlyph", "Glyphs", "FontNum", "FontDigits", "NumFont",
        "Font0", "Font10", "FontDigit", "Counter", "ScoreFont", "HudFont",
        "Score", "ScoreNum", "ScoreNums", "Numbers09", "TimeFont", "TimerFont",
        "Counter", "CountFont", "ScoreDigits", "HudDigit", "GameNum", "BigNum",
        "TinyNum", "MicroFont", "DigitFont",
        "NumeralBank", "NumberBank", "DigitBank",
        # ScummVM-style with Mo/Sc prefixes
        "MoNumber", "ScNumber", "SsNumber", "AsNumber",
        "FontHud", "FontScore", "GlyphsNum", "GlyphsDigits",
    ],
    "Menu cursor animation frame": [
        "Cursor", "MenuCursor", "Cursor1", "Pointer", "Arrow",
        "MainCursor", "Hilight", "Hilite", "Highlight",
        "MenuArrow", "MenuPointer", "MenuMarker", "MenuHilight",
        "MenuHilite", "MenuHighlight", "MenuSelector", "Selector",
        "MainMenuCursor", "OptionCursor", "MenuActive",
        "ActiveCursor", "CursorIdle", "MenuCursorIdle",
        "CursorActive", "ActiveButton", "MenuActiveButton",
        "ButtonActive", "ButtonHilight", "ButtonHilite",
        "ButtonHighlight", "ButtonOn", "MenuOn", "ItemActive",
        "ItemSelected", "Selected", "MenuItem", "MenuItem1",
        "MenuButton", "MenuButtonActive", "MenuButtonOn",
        "MenuButtonOnAnim", "ButtonOnAnim", "MenuOnAnim",
        # short
        "Hi", "On", "Active", "Sel", "Cur", "Cursor1", "Cursor2",
    ],
    "Menu door opens to reveal monkey animation": [
        "Door", "Doors", "OpenDoor", "DoorOpen", "DoorAnim",
        "MenuDoor", "MainDoor", "DoorReveal",
        "Reveal", "Open", "Opening", "Intro", "MenuIntro",
        "TitleDoor", "TitleIntro", "MenuOpen", "MainMenuOpen",
        "MenuStart", "DoorStart", "TitleOpen",
        "DoorIn", "DoorIntro", "OpenAnim", "RevealAnim",
        "MenuOpening", "OpeningAnim", "TitleAnimOpen",
        "TitleAnim", "TitleScreen", "MenuTitle", "TitleMenu",
        "DoorMonkey", "MonkeyDoor", "DoorMonk", "MonkDoor",
        "DoorReveal", "RevealMonkey", "MenuRevealMonkey",
        "MenuRevealMonk", "DoorOpenMonkey", "DoorMonkeyOpen",
        "MenuStart", "MenuOpen", "MenuMain", "MainMenuStart",
        "Title", "TitleMain",
    ],
    "Shriney Guard boss slamming down animation": [
        "ShrineyGuard", "Shriney", "Guard", "Mega", "MegaBoss",
        "MegaGuard", "Boss", "BossMega", "ShrineyGuardBoss",
        "ShrineyGuardSlam", "ShrineySlam", "GuardSlam",
        "MegaSlam", "BossSlam", "ShrineyAttack", "GuardAttack",
        "MegaAttack", "BossAttack", "Slam", "SlamDown",
        "ShrineyDown", "ShrineySlamDown", "ShrineyGuardSlamDown",
        "GuardSlamDown", "MegaSlamDown", "BossSlamDown",
        "Crush", "Pound", "Stomp", "Smash", "Hit",
        "ShrineyCrush", "ShrineyPound", "ShrineyStomp",
        "ShrineySmash", "ShrineyHit",
        "ShrineyBossSlam", "MegaBossSlam", "ShrineyBossSlamDown",
        "BigGuard", "BigGuardSlam", "BigStone", "StoneFace",
        "StoneGuard", "TempleGuard", "Idol", "TikiBoss",
        "Statue", "StatueBoss", "BigStatue",
        "asShriney", "asGuard", "asMega",
        # ScummVM-style
        "AsShrineyGuard", "AsGuard", "AsMega", "AsBoss",
    ],
    "Shriney Guard boss yelling animation": [
        "ShrineyYell", "GuardYell", "ShrineyScream", "GuardScream",
        "ShrineyShout", "GuardShout", "ShrineyRoar", "GuardRoar",
        "ShrineyTaunt", "GuardTaunt", "MegaYell", "MegaRoar",
        "MegaScream", "BossYell", "BossRoar", "BossScream",
        "ShrineyBossYell", "ShrineyBossRoar", "ShrineyBossScream",
        "ShrineyAngry", "ShrineyRage", "GuardAngry", "GuardRage",
        "ShrineyOpen", "ShrineyMouth", "ShrineyMouthOpen",
        "OpenMouth", "GuardMouth", "MegaMouth",
        "ShrineyTalk", "GuardTalk", "MegaTalk", "BossTalk",
        "AsShrineyYell", "AsGuardYell", "AsBossYell",
    ],
    "Plain monkey enemy": [
        "Monkey", "Monk", "PlainMonkey", "PlainMonk", "BasicMonkey",
        "BasicMonk", "GenericMonkey", "GenericMonk",
        "GruntMonkey", "GruntMonk", "Grunt", "Mook",
        "MookMonkey", "MookMonk", "BadMonkey", "EvilMonkey",
        "DumbMonkey", "DumbMonk", "WalkMonkey", "WalkMonk",
        "MonkeyEnemy", "MonkEnemy", "EnemyMonkey", "EnemyMonk",
        "AsMonkey", "AsMonk", "AsPlainMonkey", "AsBasicMonkey",
        "Ape", "Primate", "ApeEnemy",
    ],
    "Plain monkey enemy variant": [
        "MonkeyVariant", "MonkVariant", "MonkeyAlt", "MonkAlt",
        "MonkeyBig", "MonkBig", "MonkeySmall", "MonkSmall",
        "MonkeyJump", "MonkJump", "MonkeyRun", "MonkRun",
        "Monkey2", "Monk2", "MonkeyA", "MonkeyB",
        "BigMonkey", "BigMonk", "JumpingMonkey", "JumpMonkey",
        "FlyingMonkey", "FlyMonkey", "RedMonkey", "BlueMonkey",
        "BossMonkey", "TallMonkey", "FastMonkey", "ToughMonkey",
        "ArmoredMonkey", "EliteMonkey",
        "MonkeyType2", "MonkType2", "MonkeyV2", "MonkV2",
        "AsMonkeyVariant", "AsMonk2", "AsMonkey2",
    ],
}

def test_with_prefixes(name, prefixes=PREFIXES):
    """Try ALL combinations of prefix + name."""
    candidates = []
    for p in prefixes:
        # Try prefix + name as written
        s = p + name
        candidates.append(s)
        # Also try lowercase first char
        if name and name[0].isalpha():
            s2 = p + name[0].lower() + name[1:]
            candidates.append(s2)
        # Also try with separator
        if p:
            candidates.append(p + "_" + name)
    return candidates

print(f"Hash function verified.\n")

total = 0
hits_found = []
for tid, role in TARGETS.items():
    print(f"\n=== 0x{tid:08x}  {role} ===")
    base_words = VOCAB[VOCAB_ALIAS.get(role, role)]
    all_cands = set()
    for w in base_words:
        for cand in test_with_prefixes(w):
            all_cands.add(cand)
    print(f"  testing {len(all_cands):,} candidates...")
    total += len(all_cands)
    found = []
    for c in all_cands:
        if asset_id(c) == tid:
            found.append(c)
    if found:
        print(f"  *** {len(found)} HITS ***")
        for h in found:
            print(f"    {h}")
        hits_found.extend([(tid, role, h) for h in found])
    else:
        print(f"  (no hit)")

print(f"\n\nTotal candidates tested: {total:,}")
print(f"Total HITS: {len(hits_found)}")
if hits_found:
    print("\n=== ALL HITS ===")
    for tid, role, name in hits_found:
        print(f"  0x{tid:08x}  {name!r}  ({role})")
