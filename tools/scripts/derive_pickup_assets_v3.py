"""Verify space-insensitivity + direct probe for sprite IDs."""
import sys, os
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

# Confirm space-insensitivity
print("QUIT GAME  ->", hex(calcHash("QUIT GAME")))
print("QUITGAME   ->", hex(calcHash("QUITGAME")))
print("QUIT_GAME  ->", hex(calcHash("QUIT_GAME")))
print("quit-game  ->", hex(calcHash("quit-game")))
print("PAUSED     ->", hex(calcHash("PAUSED")))
print("CONTINUE   ->", hex(calcHash("CONTINUE")))
print()

# Direct probe with simple display names for our targets
targets = {
    0x6A351094: ("UniverseEnema", [
        "UNIVERSE ENEMA","UNIVERSEENEMA","UNIVERSE_ENEMA","ENEMA",
        "U E","UE","UNIV ENEMA","UNIVENEMA","COSMIC ENEMA",
    ]),
    0x88A28194: ("1970 Icon", [
        "1970","1970 ICON","BONUS 1970","ICON 1970","1970 BONUS","SECRET 1970",
        "BONUS","BONUS GATE","BONUS WARP","WARP TO 1970","WARP 1970",
        "JIM","EARTHWORM JIM","EARTHWORM","WORM","BONUS ENTRANCE",
    ]),
    0x80E85EA0: ("HamsterShield", [
        "HAMSTER SHIELD","HAMSTERSHIELD","HAMSTER","SHIELD","H SHIELD","HAM SHIELD",
        "HAMSTERSHEILD","HAM SHEILD","HAMSTER POWERUP","HAM POWERUP",
        "PROTECTION","ORBIT FRIENDS","ROUND ONE","HAMSTER POWER",
    ]),
    0x902C0002: ("SuperWillie", [
        "SUPER WILLIE","SUPERWILLIE","WILLIE","SUPER","S WILLIE","SUPERWIL",
        "WILLY","SUPER WILLY","SUPERWILLY","SUPER W","S W",
        "TRANSFORM","TRANSFORMATION","MORPH","WILLIE POWERUP",
    ]),
    0x8C30008C: ("ScaleReset/Grow", [
        "GROW","SCALE RESET","SCALERESET","RESET","GROW UP","GROW BIG","BIG",
        "RESIZE","SCALE","SCALE BACK","REGROW","UNSCALE","FULL SIZE",
        "BIG AGAIN","BIG KLAYMEN","KLAYMEN GROW","GROW UP","RESCALE",
    ]),
    0x08624580: ("SingleFrame/100C", [
        "100C","100 C","100","HUNDRED","HUNDRED C","C","TRIGGER","TRIGGER 100",
        "BONUS","100 COIN","COIN","100 ORB","100 ORBS","ORB 100","SCORE 100",
    ]),
    0xBE68D0C6: ("death debris large", [
        "DEBRIS","DEBRIS BIG","BIG DEBRIS","GIB","BIG GIB","CHUNK","BIG CHUNK",
        "EXPLODE","BIG EXPLODE","EXPLODE BIG","DEATH","DEATH BIG","BIG DEATH",
        "BLOOD","BIG BLOOD","SPLAT","BIG SPLAT","BLOB","BIG BLOB",
        "CHUNK BIG","CLAY BIG","BIG CLAY","CLAY CHUNK",
    ]),
    0xB868D0C6: ("death debris small", [
        "DEBRIS SMALL","SMALL DEBRIS","DEBRIS LITTLE","DEBRIS LO","DEBRIS S",
        "GIB SMALL","SMALL GIB","CHUNK SMALL","SMALL CHUNK","BIT SMALL","SMALL BIT",
        "CLAY SMALL","SMALL CLAY","CLAY BIT","CLAY GIB",
    ]),
    0xB01C25F0: ("death grow / JHJ ball", [
        "DEATH GROW","DEATHGROW","GROW DEATH","BIG DEATH","DEATH BIG",
        "JHJ BALL","JOE BALL","HEAD BALL","SKULL BALL","BIG BALL","HEAVY BALL",
        "DEATH ANIM","DEATH GROW ANIM","DEATH FALL","FALL DOWN","SQUASH","BIG SQUASH",
    ]),
    0xBB781481: ("wake decor", [
        "WAKE","WAKE UP","WAKEUP","AWAKEN","ACTIVATE","ON","TRIGGER","ALERT",
        "STIR","RISE","ENRAGED","ANGRY","WAKE DECOR","DECOR WAKE","DECOR ON",
        "INTRO","BEGIN","START","SPAWN","BIRTH","BORN","COME ALIVE","ALIVE",
        "WOKEN","AWAKE","REVIVE","REVIVED","ANIMATE","ANIMATED",
    ]),
}

for h, (label, names) in targets.items():
    print(f"\n0x{h:08x} ({label}):")
    found = False
    for n in names:
        nh = calcHash(n)
        if nh == h:
            print(f"  {n!r:35s} -> 0x{nh:08x} <-- TARGET")
            found = True
    if not found:
        print(f"  No hit in {len(names)} candidates")
