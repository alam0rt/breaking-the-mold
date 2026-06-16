"""Try different conventions for 0x4a806042 - the clay-ball 'pop' / orb-collect sound.
Use simple two-word and short conventions.
"""
import sys, os
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

T = 0x4a806042

# Common naming patterns
patterns = []
# Two-word patterns
words1 = ["ORB","CLAY","BLB","POP","BLOB","GET","NICE","NOM","YUM","GOT","GRAB",
          "PICKUP","COLLECT","COIN","CASH","MONEY","CURRENCY","TOKEN","CREDIT",
          "SCRAP","BIT","CHUNK","PIECE","PARTICLE","FRAG","DOT","BEAD","SCRAP",
          "BALL","ITEM","ICON","HUD","Q","SQ","SWIRLY","SWIRL","KLAY","KMEN","PLAYER",
          "BLOB","SHARD","SPECK","NUGGET","SPHERE","DOLLOP","WAD","DAB","DROP",
          "TINY","SINGLE","ONE","MICRO","MITE","MINIBALL","MINI_BALL","MINICLAY",
          "MINICLAYBALL","SMALL","LITTLE","BIT","GIB","GIBLET","PIP","POP","BLIP"]
words2 = ["ORB","BIT","POP","NICE","NOM","YUM","GOT","NABBED","GAINED","SCOOPED",
          "OK","OKAY","NORM","GO","FOUND","BAGGED","WIN","WINNER","NOMS","CHIME",
          "DING","BEEP","BLIP","BLOOP","PLOP","GLUG","CLUNK","THUD","TWANG",
          "POP","DROP","BUMP","BING","FX","SFX","SOUND","AUDIO","BLB","BLOB",
          "SHINY","SPARK","SPARKLE","GLITTER","TWINKLE","SHINE","GLEAM",
          "PROFIT","DOSH","BUCKS","SCORE","SCORED","ADD","ADDED","SUMMED",
          "1","ONE","UNIT","CT","HIT","HUH","SUP","HI","OY","HEY","WOW","WHEE","HUZZAH"]

for w1 in words1:
    for w2 in words2:
        for pre in ["FX_","SFX_",""]:
            for sep in ["_",""]:
                n = pre + w1 + sep + w2
                h = calcHash(n)
                if h == T:
                    print(f"  {n:35s} -> 0x{h:08x} <-- TARGET")

# Three-word
for w1 in ["ORB","CLAY","BLB","BIT","KLAY","KMEN","PLAYER"]:
    for w2 in ["GET","TAKE","GRAB","GAIN","COLLECT","PICKUP","PICK_UP","NAB","BAG"]:
        for w3 in ["","_01","_02","_SM","_LG","_OK","_FX"]:
            for pre in ["FX_","SFX_"]:
                n = pre + w1 + "_" + w2 + w3
                h = calcHash(n)
                if h == T:
                    print(f"  {n:35s} -> 0x{h:08x} <-- TARGET")
                n = pre + w2 + "_" + w1 + w3
                h = calcHash(n)
                if h == T:
                    print(f"  {n:35s} -> 0x{h:08x} <-- TARGET")

# Single-word: just FX_<X>
for w in ["BLBPOP","ORBPOP","ORB_POP","ORBSGET","ORBSGOT","BLBGET","BLBGOT",
          "BLOOP","BLIP","PLOP","DING","NOM","GET","WIN","POP","CHIME","BEEP",
          "GAIN","NICE","YUM","NOMNOM","GULP","SLURP","WHOOP","WHEE",
          "CLAYPOP","CLAY_POP","CLAYBLB","BLB_POP","CLAYBALL_POP","BALL_POP"]:
    for pre in ["FX_","SFX_"]:
        n = pre + w
        h = calcHash(n)
        if h == T:
            print(f"  {n:35s} -> 0x{h:08x} <-- TARGET")

print("Done.")
