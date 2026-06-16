"""Massive variant attack on 0x4a806042 ('clay collect pop')."""
import sys, os
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

T = 0x4a806042

# Various roots
ROOTS = [
    "ORB","ORBS","ORBIE","O","Q","CLAYBALL","CLAY_BALL","CLAYBLOB","CLAY","BLB",
    "BALL","SHARD","SPHERE","BLOB","BIT","SCRAP","CHUNK","DROP","DROPLET","NUGGET",
    "GLOB","DOLLOP","WAD","DAB","PINCH","BIT","PIECE","TID","TIDBIT","SMIDGEN",
    "PIP","POP","DOT","SPECK","SPOT","BLOT","PINPRICK","NUB","NIB","NUGGET","KNOB",
    "BEAD","BEADY","BUTTON","KNOT","DROPLET","DROP","TEAR","PEARL","PELLET",
    "POPCORN","POP_CORN","POPPIN","KIBBLE","ROCK","PEBBLE","STONE","GEM","CRYSTAL",
    "CURRENCY","COIN","TOKEN","CREDIT","CASH","DOUGH","CLAY_DOUGH","DOH","CLAYDOH",
    "BLOB_OF_CLAY","BLOB_CLAY","CLAY_BLOB","CB","BC","SC","SINGLE","SOLO","UNIT","ONE",
    "TINY","SMALL","LITTLE","MICRO","MINI","TIDDLY","DINKY","SLIVER","NUGGET","CRUMB",
    "PILE","STUFF","GUNK","MUCK","GOO","OOZE","SLIME","MUD","SLUDGE","MUSH",
    "DAB","SQUIRT","SPURT","SPECK","BIT","FRAG","FRAGMENT","PARTICLE",
    "SPLASH","SPLATTER","SPLAT","GLOB","WAD","BLOB","WAFER","CHIP","FLAKE",
    "BLOB_PLAYER","KLAYBLOB","KLAYORB","KLAY_BLOB","KLAY_ORB","KMENORB","KMEN_ORB",
    "PLAYER_ORB","PLAYER_BIT","PLAYER_CLAY","PLAYER_BALL",
    "KLAYMENBIT","KLAYMEN_BIT","KLAYMEN_ORB","KLAYMEN_BALL","KLAYMEN_BLOB",
    "MONEY","REWARD","PRIZE","BLB_MONEY","BLB_REWARD","BLB_BIT","BLB_PIECE",
    "PEA","BEAN","SEED","GUM","GUMDROP","GUM_DROP","DOT","SPOT",
    "ITEM","ICON","HUD_ICON","HUD","FAB","FABULOUS","FAB_BIT",
]

# Prefixes
PRE = [
    "FX_","SFX_","FX_PICKUP_","FX_GET_","FX_GAIN_","FX_GRAB_","FX_TAKE_",
    "FX_NEW_","FX_COLLECT_","FX_COLLECTED_","FX_BLB_","FX_PWR_","FX_BONUS_",
    "FX_KLAY_","FX_PLAYER_","FX_HERO_","SFX_PICKUP_","SFX_COLLECT_","SFX_GET_",
    "FX_POP_","FX_DING_","FX_CHIME_","FX_BLIP_","FX_BLOOP_","FX_PLOP_",
    "FX_GLUG_","FX_CLUNK_","FX_THUD_","FX_TWANG_","FX_BLAB_",
]

# Suffixes
SUF = [
    "","_01","_02","_03","_1","_2","_3","_SM","_LG","_BIG","_S","_L","_M",
    "_GET","_GAIN","_TAKE","_PICKUP","_ON","_OFF","_END","_DONE","_OK","_OKAY",
    "_GO","_NICE","_POP","_PLOP","_BLIP","_DING","_BLB","_BLBED","_BLOBBED",
    "_GOT","_GRABBED","_TAKEN","_COLLECTED","_NABBED","_GAINED","_NOM","_NOMNOM",
    "_NOMMED","_YUM","_YOINK","_SCOOPED","_NABBED","_BLBBLB","_GULP",
    "_NICE","_NEWHIT","_WIN","_GOOD","_BLBOK","_BLBGO","_BLBGET",
    "_FULL","_NORM","_REG","_MAX","_HALF","_QTR","_PWR","_SOFT","_LOUD","_QUIET",
    "_FX","_AUDIO","_SOUND","_SFX","_BLB","_CHIME","_BEEP","_TWEET","_BLB_FX",
    "_FX_01","_FX_02","_BLB_01","_BLB_02",
]

found = False
total = 0
for r in ROOTS:
    for pre in PRE:
        for suf in SUF:
            n = pre + r + suf
            total += 1
            h = calcHash(n)
            if h == T:
                print(f"  {n:35s} -> 0x{h:08x} <-- TARGET")
                found = True

print(f"\nTried {total} variants, found={found}")
