"""0x686c1c97 popc=14, MENU - might be longer name like FX_MENU_<action>_<modifier>."""
import sys, os, itertools
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

T = 0x686c1c97

# Try three-token names
W1 = ["MENU","CURSOR","NAV","SCROLL","SELECT","INPUT","HUD","UI","BLB","PASS","BTN"]
W2 = ["UP","DOWN","LEFT","RIGHT","NEXT","PREV","MOVE","TICK","TAP","CLICK","BLIP",
      "BEEP","DING","BUMP","NUDGE","HOP","JUMP","INCREMENT","DECREMENT",
      "SHIFT","STEP","JUMP","FLICK","TILT","ROLL","TURN","SWITCH","FLIP",
      "SHIMMER","TWANG","BOING","DOINK","TWEET","CHIRP","CHIME","TINK","TONK",
      "PLOP","BLOOP","BUZZ","HUM","PIP","POP","BLB","TYPE","KEY","PRESS",
      "TICK_TICK","TICKTICK","TICK_TOCK"]
W3 = ["","_01","_02","_03","_1","_2","_3","_LO","_HI","_LOW","_HIGH","_SOFT","_LOUD","_QUIET",
      "_OK","_OKAY","_FAST","_SLOW","_DEEP","_FLAT","_BLIP","_BEEP","_TICK","_TAP","_PRESS",
      "_SOUND","_FX","_AUDIO","_SFX","_BLB","_TONE","_BUZZ"]

for w1 in W1:
    for w2 in W2:
        for w3 in W3:
            n = f"FX_{w1}_{w2}{w3}"
            h = calcHash(n)
            if h == T:
                print(f"  {n:35s} -> 0x{h:08x} <-- TARGET")
            n = f"SFX_{w1}_{w2}{w3}"
            h = calcHash(n)
            if h == T:
                print(f"  {n:35s} -> 0x{h:08x} <-- TARGET")

# Three-token: FX_<w1>_<w2>_<w3>
W3_TOK = ["UP","DOWN","BLIP","BEEP","TICK","CLICK","TAP","PRESS","BUMP","NUDGE","BUZZ",
          "OK","DONE","FX","SOUND","BLB","HUM","TONE","DING"]
for w1 in W1:
    for w2 in W2:
        for w3 in W3_TOK:
            n = f"FX_{w1}_{w2}_{w3}"
            h = calcHash(n)
            if h == T:
                print(f"  {n:35s} -> 0x{h:08x} <-- TARGET")
            n = f"SFX_{w1}_{w2}_{w3}"
            h = calcHash(n)
            if h == T:
                print(f"  {n:35s} -> 0x{h:08x} <-- TARGET")

print("Done.")
