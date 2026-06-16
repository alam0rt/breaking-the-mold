"""FX_MENU_<X> family - try menu nav, password unlock, wrong-password, etc."""
import sys, os
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

targets = {
    0x686c1c97: "menu nav (cursor up/down)",
    0x42906465: "menu unlock (level-complete) / enemy hurt",
    0x64221e61: "menu wrong-password / dir input",
    0x40023e30: "decor wake / credits music start",
}

# Try all FX_MENU_<X> and SFX_MENU_<X>
MENU_ROOTS = [
    # Nav
    "NAV","NAV_UP","NAV_DOWN","NAV_LEFT","NAV_RIGHT","NAVIGATE","NAVTAP",
    "CURSOR","CURSOR_UP","CURSOR_DOWN","CURSOR_MOVE","CURSOR_NAV","CURSOR_TICK",
    "MOVE","MOVE_UP","MOVE_DOWN","MOVE_LEFT","MOVE_RIGHT","SHIFT",
    "UP","DOWN","LEFT","RIGHT","NEXT","PREV","PREVIOUS",
    "STEP","STEP_UP","STEP_DOWN","JUMP","JUMP_UP","JUMP_DOWN",
    "TICK","TICK_UP","TICK_DOWN","CLICK","CLICK_UP","CLICK_DOWN",
    "TAP","TAP_UP","TAP_DOWN","BLIP","BLIP_UP","BLIP_DOWN","BLIP_BLIP",
    "SCROLL","SCROLL_UP","SCROLL_DOWN","SCROLL_TICK","SCROLLER",
    "ARROW","ARROW_UP","ARROW_DOWN","ARROW_NAV","CHOICE","CHOOSE",
    "HIGHLIGHT","FOCUS","POINT","POINTER","SELECT","SELECTOR",
    # Password
    "PASSWORD","PASS","PWD","PW","PASSCODE","CODE","UNLOCK","LEVEL",
    "PASS_OK","PASS_NO","PASS_FAIL","PASS_GOOD","PASS_DONE","PASS_WIN","PASS_LOSE",
    "PWD_OK","PWD_NO","PWD_FAIL","PWD_GOOD","PWD_DONE","PWD_WIN","PWD_LOSE",
    "PASSWORD_OK","PASSWORD_NO","PASSWORD_FAIL","PASSWORD_GOOD","PASSWORD_DONE",
    "PASSWORD_WIN","PASSWORD_LOSE","PASSWORD_BAD","PASSWORD_WRONG","PASSWORD_VALID",
    "PASSWORD_INVALID","PASSWORD_ENTER","PASSWORD_SUBMIT","PASSWORD_ACCEPT",
    "LEVEL_PASS","LEVEL_PASSED","LEVEL_OK","LEVEL_DONE","LEVEL_BEAT","LEVEL_CLEAR",
    "WRONG","ERROR","FAIL","BAD","INVALID","NO","CANCEL","BACK","ESCAPE","RETURN",
    "DELETE","BACKSPACE","DEL","ERASE","CLEAR","RESET","REDO",
    "UNLOCK","LOCK","DOOR","DOOR_OPEN","DOOR_OPEN_OK","DOOR_OK","OPEN",
    "ACCEPT","CONFIRM","SUBMIT","ENTER","ENTRY",
    "TYPE","TYPED","KEY","KEYPRESS","INPUT","DIGIT","CHAR","LETTER",
    # Other menu actions
    "OPEN","CLOSE","SHOW","HIDE","FADE","FADE_IN","FADE_OUT","WIPE",
    "PAUSE","RESUME","START","INTRO","OUTRO","CREDITS","ENDING",
    "STATS","SCORE","RESULTS","CONTINUE","RETRY","QUIT","EXIT",
    "BUMP","NUDGE","WIGGLE","FLINCH","FLICK","SWIPE","SWIPER",
    "BLIP_LO","BLIP_HI","BLIP_DEEP","BLIP_HIGH","DING_LO","DING_HI",
]

for tgt, label in targets.items():
    print(f"\n=== 0x{tgt:08x} ({label}) ===")
    found = False
    for r in MENU_ROOTS:
        for pre in ["FX_MENU_","SFX_MENU_","FX_","SFX_"]:
            for suf in ["","_01","_02","_03","_1","_2","_3","_BLIP","_BEEP","_TICK","_TAP"]:
                n = pre + r + suf
                h = calcHash(n)
                if h == tgt:
                    print(f"  {n:40s} -> 0x{h:08x} <-- TARGET")
                    found = True
    if not found:
        print(f"  No hit")
