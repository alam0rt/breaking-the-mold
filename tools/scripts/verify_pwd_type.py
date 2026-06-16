"""0x00880c1e (popc=13) = password type/digit-press sound.
Used 3x: PasswordDigitInputHandler, PasswordBackspaceHandler, EndingCredits scroll.
"""
import sys, os
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

T = 0x00880c1e

# Massive name list
NAMES = []
roots = ["TYPE","TYPED","TYPING","TYPER","KEY","KEYBD","KEYBOARD","KEYS","KEYTAP","KEYPRESS",
         "PRESS","TAP","HIT","CLICK","CLICKED","CLICKER","SMACK","SLAP",
         "BUTTON","BTN","BTNS","INPUT","INPUTS","BLIP","BLIPS","BEEP","BEEPS",
         "DING","DINGS","BING","BLOOP","BOOP","CHIRP","CHEEP","DOT","DOTS",
         "DIGIT","DIGITS","CHAR","CHARS","CHARACTER","CHARACTERS","LETTER","LETTERS",
         "NUM","NUMBER","NUMBERS","NUMERAL","NUMERIC",
         "PASSWORD","PASS","PWD","PW","PASS_ENTRY","PASSCODE","PASS_CODE","PWENTRY",
         "PASS_PRESS","PWD_PRESS","PW_PRESS","CODE_PRESS","CODEPRESS",
         "BLB","BLBT","BLBTAP","BLBKEY","BLBPRESS","BLBSCROLL",
         "DASH","ENTRY","INPUT_KEY","INPUT_TAP","INPUT_PRESS","INPUT_TYPE",
         "TICK","TICK_TICK","TYPETICK","TYPE_TICK","CREDIT","CREDITS",
         "DSP","DSPLAY","DISPLAY","DISP","DSP_TICK","DISP_TICK",
         "ICON","ICON_TYPE","ICONPRESS","ICON_PRESS",
         "SCROLL","SCROLLED","SCROLLING","SCROLLER",
         "FNT","FONT","GLYPH","GLYPHS","SYM","SYMBOL","SYMBOLS","CHARG",
         "FLIP","TURN","ROLL","SPIN","SHIFT","SHUFFLE","ROTATE","WIND","SLOT",
         "WHEEL","REEL","BAR","BARS","SLOTS","DROP","ROLLOVER","TUMBLE",
         "RING","RINGS","DOOR","DOORS","DOORBELL","BELL","BELLS","DOORTAG",
         "ENTERED","TYPED_OK","PRESSED","DOWN","DOWNED","STRUCK","HITTED",
         ]
for r in roots:
    for pre in ["FX_","SFX_","FX_PASSWORD_","FX_PWD_","FX_PW_","FX_TYPE_","FX_KEY_",
                "FX_MENU_","FX_INPUT_","FX_CREDITS_","FX_HUD_","SFX_PASSWORD_","SFX_PWD_",
                "SFX_TYPE_","SFX_KEY_","SFX_INPUT_","SFX_MENU_"]:
        for suf in ["","_01","_02","_03","_1","_2","_3","_BLIP","_BEEP","_TICK","_TAP",
                    "_PRESS","_DOWN","_FX","_BLB","_SFX","_FAST","_QUICK","_SHORT","_LOW","_HI",
                    "_BLBP","_BLBPRESS","_BLBKEY"]:
            n = pre + r + suf
            NAMES.append(n)

seen = set()
found = False
for n in NAMES:
    if n in seen: continue
    seen.add(n)
    h = calcHash(n)
    if h == T:
        print(f"  {n:35s} -> 0x{h:08x} <-- TARGET")
        found = True
print(f"\nTried {len(seen)} unique candidates, found={found}")
