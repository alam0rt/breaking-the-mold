"""0x42906465 is primarily 'enemy hurt + flash white'.
Try hurt/damage/flash-themed names."""
import sys, os
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

T = 0x42906465
cands = []
for ent in ["","_ENEMY","_BOSS","_HIT","_FOE","_GIB","_GUY","_GOON","_THING","_CREATURE",
            "_BLOB","_BLB","_KMAN","_KLAY","_NPC","_BIRD","_RAT","_BAT","_FROG","_BUG","_WORM",
            "_BOSS_HIT","_SMALL","_BIG","_FLY"]:
    for verb in ["HURT","HIT","HURT_FLASH","HURT_WHITE","HURT_OUT","HURT_FX",
                 "DAMAGE","DMG","DAMAGED","DAMAGED_FX","DMG_FX",
                 "FLASH","FLASH_HURT","WHITE_FLASH","HIT_FLASH","FLASH_WHITE",
                 "POW","POW_FX","PUNCH","PUNCH_HIT","SLAP","SLAP_HIT","KICK","KICK_HIT",
                 "STAB","STABBED","SLASH","SLASHED","BITE","BITTEN","CHOMP","BURN","BURNED",
                 "ZAP","ZAPPED","SHOT","SHOT_HIT","BANG","BANG_HIT","BOOM_HIT","SMASHED",
                 "BUMP","BUMPED","KNOCK","KNOCKED","SHOCK","SHOCKED","STUNNED","STUN",
                 "OUCH","OUCHIE","PAIN","WAH","YIPE","YELP","SQUEAL","CRY",
                 "BURST","SHATTER","CRUNCH","CRUMBLE","BREAK",
                 "BIRTH","SPAWN","WAKE","DIE","DEATH","HEAL","REVIVE","SUMMON",
                 "FALL_HIT","HIT_FALL","HIT_LAND","HIT_GROUND","HIT_DROP","HIT_SHOCK",
                 "FLINCH","STUMBLE","TRIP","WHACK","WHACKED","KNOCKBACK","KO","FAINT",
                 "JOLT","JOLTED","SHAKE","SHIVER","TREMBLE","WIGGLE","TWITCH"]:
        for pre in ["FX","SFX"]:
            n = f"{pre}_{verb}{ent}"
            cands.append(n)

print(f"Probing {len(cands)} hurt-themed names for 0x42906465...")
for n in cands:
    h = calcHash(n)
    if h == T:
        print(f"  {n:35s} -> 0x{h:08x} <-- TARGET")
