"""Probe names for 0x64221e61 - generic player crawl/scoot/move sound played on
any directional input (especially during crouch-climb)."""
import sys, os
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

T = 0x64221e61
acts = ["CRAWL","SCOOT","DRAG","SHUFFLE","CREEP","SLIDE","SLIDING","CLIMB","CLIMBING",
        "CROUCH","CRAB","STEP","WALK","STEP_QUIET","STEP_SOFT","STEP_HARD",
        "PRESS","INPUT","MOVE","MOTION","SHIFT","NUDGE","TWITCH","WIGGLE",
        "SCRAPE","SCRATCH","SCRABBLE","SCRABBLED","WORM","SQUIRM",
        "BREATH","BREATHE","HUFF","PANT","PUFF","SIGH","GRUNT",
        "SHIMMY","SQUEEZE","WIGGLE","TWIDDLE","INCH",
        "CLAY","ROLL","TUMBLE","FLEX","STRETCH","BEND","PIVOT",
        "DUCK","HUNCH","STOOP","BOW","KNEEL"]
prefixes = ["FX_KLAY_","FX_KMAN_","FX_PLAYER_","FX_HERO_","FX_FINN_",""]
sufs = ["","_01","_02","_03","_LOOP","_LP","_LOOPED","_BOI","_OF","_OFF","_QUIET","_SOFT","_SOFT_01"]

found = False
for pre in prefixes:
    for a in acts:
        for s in sufs:
            n = f"{pre}{a}{s}" if pre or s else a
            h = calcHash(n)
            if h == T:
                print(f"{n:35s} -> 0x{h:08x} <-- TARGET")
                found = True
if not found:
    print("No hit in this batch. Trying more...")
    # try more
    for w1 in ["FX","SFX"]:
        for w2 in ["KLAY","K","CLAY","PLAYER"]:
            for w3 in ["DRAG","CRAWL","SHUFFLE","SLIDE","SCOOT","CREEP","SHIFT","STEP","MOVE","INCH","WIGGLE","SCRATCH"]:
                for w4 in ["","_LOOP","_LP","_BG","_FG","_01","_BOOM","_PRESS","_BUTTON","_INPUT","_DIR"]:
                    n = f"{w1}_{w2}_{w3}{w4}"
                    h = calcHash(n)
                    if h == T:
                        print(f"  {n} -> 0x{h:08x} <-- TARGET")
                        found = True
print("\nDone.")
