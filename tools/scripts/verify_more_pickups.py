"""Verify names for newly-discovered collectible pickup sounds."""
import sys, os
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

targets = {
    0x4a806042: "ClaySingle / Orb pickup",
    0x428254e2: "ExtraLife pickup ('life collect jingle')",
    0xc88a346a: "UniverseEnema pickup",
    0x62000441: "UniverseEnema secondary",
    0x40864668: "ScaleReset pickup",
}

# Generic candidate generator
PRE = ["FX_PICKUP_","FX_GAIN_","FX_GET_","FX_GRAB_","FX_","SFX_PICKUP_","SFX_",
       "FX_NEW_","FX_TAKE_","FX_BLB_","FX_PWR_","FX_POWER_","FX_BONUS_",]
SUF = ["","_01","_02","_03","_1","_2","_3","_SM","_LG","_S","_L",
       "_FULL","_NORM","_REG","_MAX","_GET","_GAIN","_TAKE","_PICKUP","_ON","_OFF","_END","_DONE"]

CANDIDATES_BY_ROOT = {
    "clay_single": ["CLAY","CLAYBALL","CLAY_BALL","BLB","BLBSPLIT","CLAYORB","ORB","CLAYSINGLE",
                    "CLAY_SINGLE","SINGLE","ONE","SOLO","SINGLE_CLAY","BALL","BLOB","SHARD"],
    "extra_life": ["LIFE","EXTRA_LIFE","EXTRALIFE","ONEUP","ONE_UP","1UP","1_UP",
                   "EXTRA","EXTRAS","HEART","BONUSLIFE","BONUS_LIFE","NEWLIFE","NEW_LIFE",
                   "KLAY","KLAYMEN","HERO","PLAYER","SPARE","PLUS_ONE","PLUS1","PLUSONE"],
    "universe_enema": ["UNIVERSE","ENEMA","UNIVERSEENEMA","UNIVERSE_ENEMA","UE","UEN",
                       "SUPERBOMB","SUPER_BOMB","BOMB","NUKE","BLAST","BIG_BLAST",
                       "DESTROY","DESTROY_ALL","CLEAR","CLEAR_ALL","WIPEOUT","WIPE_OUT",
                       "MEGA","SUPER","ULTIMATE","ROCKETLAUNCHER","ROCKET_LAUNCHER"],
    "scale_reset": ["SCALE","SCALE_RESET","SCALERESET","RESET","RESET_SCALE","SHRINK",
                    "SHRINK_RESET","NORMAL","NORMALIZE","REGULAR_SIZE","REGULAR","ORIGINAL",
                    "RESIZE","RESET_SIZE","GROW","GROW_RESET","CLAYDOH","BLBDOH","BLB_DOH"],
}

target_map = {
    0x4a806042: "clay_single",
    0x428254e2: "extra_life",
    0xc88a346a: "universe_enema",
    0x62000441: "universe_enema",  # secondary
    0x40864668: "scale_reset",
}

for tgt, label in targets.items():
    root = target_map[tgt]
    print(f"\nTarget 0x{tgt:08x} ({label}):")
    found = False
    for r in CANDIDATES_BY_ROOT[root]:
        for pre in PRE:
            for suf in SUF:
                n = pre + r + suf
                h = calcHash(n)
                if h == tgt:
                    print(f"  {n:35s} -> 0x{h:08x} <-- TARGET")
                    found = True
    if not found:
        print(f"  No hit in standard variants. Trying extended...")
        # Extended: prefix with "_FX" suffix, different name shapes
        extras = []
        for r in CANDIDATES_BY_ROOT[root]:
            for f_pre in ["", "FX_"]:
                for f_suf in ["_FX","_AUDIO","_SOUND","_SFX","_BLB","_CHIME","_DING","_BEEP",
                              "_TWEET","_BOI","_OF","_BOOM","_LIVE","_LIFE","_PWRUP","_PWR"]:
                    extras.append(f"{f_pre}{r}{f_suf}")
        for n in extras:
            h = calcHash(n)
            if h == tgt:
                print(f"  {n:35s} -> 0x{h:08x} <-- TARGET (extended)")
                found = True
        if not found:
            print(f"  Still no hit.")
