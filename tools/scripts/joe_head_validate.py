"""Test if FX_SKULL_GRUNT_01 is right name for JoeHeadJoe land sound (0xf2810224).
Try Joe-themed names too.
"""
import sys, os, csv
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

T = 0x40e28045  # second target
T2 = 0xf2810224  # JoeHeadJoe land sound

candidates = [
    "FX_SKULL_GRUNT_01","FX_SKULL_GRUNT","FX_SKULL_GRUNT_LAND",
    "FX_JOE_LAND","FX_JOE_BOUNCE","FX_JOE_LAND_01","FX_JOE_BOUNCE_01",
    "FX_HEAD_LAND","FX_HEAD_BOUNCE","FX_HEAD_HIT","FX_HEAD_HIT_01",
    "FX_HEAD_GRUNT","FX_HEAD_GRUNT_01","FX_HEAD_THUD","FX_HEAD_DROP",
    "FX_HEADJOE_LAND","FX_HEADJOE_BOUNCE","FX_JOEHEAD_LAND","FX_JOEHEAD_BOUNCE",
    "FX_BIG_LAND","FX_BIG_DROP","FX_BIG_THUD","FX_BIG_HIT",
    "FX_BABY_LAND","FX_BABY_HEAD_LAND","FX_BABY_HEAD_BOUNCE",
    "FX_BOSS_LAND","FX_BOSS_BOUNCE","FX_BOSS_HIT_01","FX_BOSS_GRUNT_01",
    "FX_FATTY_LAND","FX_FATBOY_LAND","FX_LAND_HEAD","FX_BOUNCE_HEAD",
    "FX_GIANT_LAND","FX_GIANT_HIT","FX_GIANT_THUD","FX_HEAVY_LAND","FX_HEAVY_THUD",
    "FX_JOE_VOICE","FX_HEAD_VOICE","FX_HEAD_VOICE_01",
    "FX_HEAD_GRUNT_LAND","FX_HEAD_LAND_GRUNT","FX_HEAD_HUFF",
    "FX_HEAD_BREATHE","FX_HEAD_BREATH_01","FX_HEAD_BREATHE_01",
    "FX_SKULL_HUFF","FX_SKULL_PUFF","FX_SKULL_BREATH","FX_SKULL_BREATHE",
]

for n in candidates:
    h = calcHash(n)
    m = ""
    if h == T2: m = " <-- TARGET 0xf2810224 JoeHeadJoe land sound"
    if m or h == T2:
        print(f"  {n:35s} -> 0x{h:08x}{m}")
