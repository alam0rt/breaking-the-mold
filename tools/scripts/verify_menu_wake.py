"""Verify 0x686c1c97 = menu navigation SFX (cursor up/down).
And 0x40023e30 = wake-decor / activation sound (used in ending credits too).
"""
import sys, os
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

T1 = 0x686c1c97  # menu nav
T2 = 0x40023e30  # decor wake / activate

# Menu nav candidates
NAMES_MENU = []
for r in ["CURSOR","NAV","SELECT","SCROLL","CHANGE","MOVE","SHIFT","INCREMENT","DECREMENT",
          "UP","DOWN","NEXT","PREV","PREVIOUS","ARROW","CHOICE","CYCLE","HIGHLIGHT","FOCUS",
          "POINT","TAB","JUMP","STEP","CHAR","DIGIT","ENTRY","ITEM","OPTION","ROW",
          "BLIP","BEEP","TICK","CLICK","TAP","DING","BLOOP","BUZZ","PIP","POP",
          "TYPE","KEY","KEYS","BUTTON","BTN","INPUT","KEYPRESS","PRESS"]:
    for pre in ["FX_","SFX_","FX_MENU_","FX_NAV_","FX_CURSOR_","SFX_MENU_","SFX_NAV_","SFX_CURSOR_"]:
        for suf in ["","_01","_02","_LO","_HI","_SOFT","_LOUD","_BLIP","_BEEP","_TICK","_CLICK"]:
            NAMES_MENU.append(pre + r + suf)
# Also bare names
NAMES_MENU += ["FX_MENU","FX_NAV","FX_CURSOR","FX_BLIP","FX_BEEP","FX_TICK","FX_CLICK","FX_TAP","FX_DING"]

NAMES_WAKE = []
for r in ["WAKE","WAKEUP","WAKE_UP","ACTIVATE","ACTIVE","ON","BEGIN","START","SPAWN","BIRTH",
          "AWAKEN","REVIVE","SUMMON","INVOKE","RISE","ARISE","STIR","ROUSE","ARMS",
          "INTRO","CUE","ENTER","TRIGGER","FIRE","KICK","KICKOFF","GO","LAUNCH","HEADLINE",
          "JOIN","ENGAGE","INIT","BOOT","LIVE","HUM","DRONE","MUSIC","SONG","JINGLE",
          "FANFARE","TADA","TUNE","INTRO_TUNE","INTRO_MUSIC","INTRO_JINGLE",
          "TRACK","SCORE","BGM","BG_MUSIC","BACKGROUND","BACKING","WAVE","INTRO_WAVE",
          "DECOR","OBJECT","THING","GADGET","CONTRAPTION","DEVICE","MACHINE","CONTRAPTION_ON"]:
    for pre in ["FX_","SFX_","FX_DECOR_","FX_OBJ_","SFX_DECOR_","SFX_OBJ_","FX_BGM_","SFX_BGM_"]:
        for suf in ["","_01","_02","_LOOP","_LP","_START","_INTRO","_BEGIN","_FX","_FADE","_END"]:
            NAMES_WAKE.append(pre + r + suf)
NAMES_WAKE += ["FX_WAKE","FX_ACTIVATE","FX_DECOR","FX_BGM","FX_MUSIC","FX_INTRO","FX_OUTRO",
               "FX_CREDITS","FX_CREDITS_MUSIC","FX_CREDITS_MUS","FX_ENDING_MUSIC","FX_ENDMUS"]

for tgt, label, cands in [(T1, "menu nav SFX", NAMES_MENU), (T2, "wake/activate", NAMES_WAKE)]:
    print(f"\n=== 0x{tgt:08x} ({label}) ===")
    found = False
    seen = set()
    for n in cands:
        if n in seen: continue
        seen.add(n)
        h = calcHash(n)
        if h == tgt:
            print(f"  {n:35s} -> 0x{h:08x} <-- TARGET")
            found = True
    if not found:
        print(f"  No hit in {len(seen)} unique candidates")
