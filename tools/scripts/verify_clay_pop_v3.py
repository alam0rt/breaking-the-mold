"""Brute small names for 0x4a806042 (popc=9 - very low).
Suggests very short name like FX_BLB or FX_POP_<X>.
"""
import sys, os, itertools, string
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

T = 0x4a806042

# Try simple names
single_words = ["BLB","POP","ORB","O","Q","SQ","BIT","CHIP","DOT","DROP","TIDBIT",
                "SHARD","SCRAP","CHUNK","GIB","NICE","GET","GOT","YOINK","SNAG",
                "BLOB","BLIP","GLOP","PLOP","BLOOP","DING","TWEET","BOOP","BEEP",
                "CLAY","NOM","BLBP","BLBPOP","ORBPOP","POPBLB","BLB1","BLB01",
                "CLAYPOP","CLAY_POP","POP_CLAY","CHIME","TWANG","WHOOP","WEE","WHEE"]

for w in single_words:
    for pre in ["", "FX_", "SFX_", "FX_PICKUP_"]:
        n = pre + w
        h = calcHash(n)
        if h == T:
            print(f"  {n:35s} -> 0x{h:08x} <-- TARGET")

# Try short FX_ABC patterns
print("Trying short patterns FX_<2-3char>...")
import string
alpha = string.ascii_uppercase + "_"
for L in [2, 3, 4]:
    for combo in itertools.product(alpha, repeat=L):
        word = "".join(combo)
        if word.startswith("_") or word.endswith("_") or "__" in word: continue
        n = "FX_" + word
        h = calcHash(n)
        if h == T:
            print(f"  {n:35s} -> 0x{h:08x} <-- TARGET (FX_<{L}>)")
print("Done.")
