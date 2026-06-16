#!/usr/bin/env python3
"""Test SPECIFIC candidate names matched to function-context.

For each labeled function-id pair, generate context-specific candidates
and test exhaustively.
"""
import sys, csv
sys.path.insert(0, "tools/scripts")
from compound_hash_attack import asset_id

ROOT = "/home/sam/projects/btm"

# Load all IDs
ids = set()
with open(f"{ROOT}/docs/reference/asset-ids-master.csv") as f:
    next(f)
    for line in f:
        ids.add(int(line.split(',')[0], 16))

# Function-context: asset ID → list of likely tokens
CONTEXTS = {
    # ShrineyGuardAttackAnimState: sprite for Shriney Guard's attack animation
    0x085860d4: {
        "context": "ShrineyGuardAttackAnimState",
        "subjects": ["SHRINEY","SHRINEYGUARD","GUARD","MEGA","MEGABOSS","BOSS","SG","SH"],
        "actions": ["ATTACK","ATTACKS","ATTACKING","SLAM","SLAMMING","FALL","FALLING","STRIKE","STRIKING","SMASH","SMASHING","CHARGE","CHARGING","JUMP","JUMPING","HIT","HITTING","HURT","HURTING"],
        "modifiers": ["","_ANIM","ANIM","_SPRITE","SPRITE","_LOOP","LOOP","_CYCLE","CYCLE","_STATE","_ANIMATION","_FRAME","FRAME","_FRAMES","FRAMES","_SPR","SPR","_FX","FX","_ICON","ICON"],
    },
    # ShrineyGuardReadyAttackState: sprite 0x8192250
    0x08192250: {
        "context": "ShrineyGuardReadyAttackState",
        "subjects": ["SHRINEY","SHRINEYGUARD","GUARD","MEGA","BOSS","SG"],
        "actions": ["READY","READYATTACK","READYING","PREPARE","PREPARING","CHARGE","CHARGING","WINDUP","WIND_UP","BUILDUP","TELL","INTRO","START","STARTING","STAND","STANDING","WAIT","WAITING","IDLE","TAUNT","TAUNTING"],
        "modifiers": ["","_ANIM","ANIM","_LOOP","LOOP","_STATE","_FRAME","FRAME","_SPR","SPR"],
    },
    # JoeHeadJoeMoveAndCheckAttack: 0x0b290ba2, 0x0e3889be, 0x603588e8
    0x0b290ba2: {
        "context": "JoeHeadJoeMoveAndCheckAttack",
        "subjects": ["JOE","JOEHEAD","JOEHEADJOE","JHJ","HEAD","JHEAD","HEADJOE"],
        "actions": ["MOVE","MOVING","WALK","WALKING","CHECK","CHECKING","ATTACK","ATTACKING","SCAN","SCANNING","TURN","TURNING","ROAM","ROAMING","HUNT","HUNTING","CHASE","CHASING","FOLLOW","FOLLOWING","SPOT","SPOTTING","ANIM"],
        "modifiers": ["","_ANIM","ANIM","_LOOP","LOOP","_STATE","_FRAME","FRAME","_SPR","SPR","_FX","FX"],
    },
    0x0e3889be: {
        "context": "JoeHeadJoeMoveAndCheckAttack",
        "subjects": ["JOE","JOEHEAD","JOEHEADJOE","JHJ","HEAD"],
        "actions": ["MOVE","MOVING","WALK","WALKING","CHECK","CHECKING","ATTACK","ATTACKING","SCAN","SCANNING","TURN","TURNING","ROAM","ROAMING","HUNT","HUNTING","CHASE","CHASING","FOLLOW","FOLLOWING","SPOT","SPOTTING","ANIM"],
        "modifiers": ["","_ANIM","ANIM","_LOOP","LOOP","_STATE","_FRAME","FRAME","_SPR","SPR","_FX","FX"],
    },
    # GlennYntisDeathEventHandler: 0x18e88310, 0x18e88510, 0x18e88910, 0x18e8c704
    0x18e88310: {
        "context": "GlennYntisDeathEventHandler",
        "subjects": ["GLEN","GLENN","GLENNYNTIS","YNTIS","YNT","GLENYNT","GY"],
        "actions": ["DIE","DEATH","DYING","DEAD","KILL","KILLED","DEFEAT","DEFEATED","FALL","FALLING","HURT","HURTING","DAMAGE","DAMAGED","DEBRIS","EXPLODE","EXPLOSION","DEATHANIM","DEATHFX"],
        "modifiers": ["","_ANIM","ANIM","_LOOP","LOOP","_STATE","_FRAME","FRAME","_SPR","SPR","_FX","FX","_PARTICLE","PARTICLE","_DEBRIS","DEBRIS"],
    },
    0x18e88510: {
        "context": "GlennYntisDeathEventHandler",
        "subjects": ["GLEN","GLENN","GLENNYNTIS","YNTIS","YNT","GLENYNT","GY"],
        "actions": ["DIE","DEATH","DYING","DEAD","KILL","KILLED","DEFEAT","DEFEATED","FALL","FALLING","HURT","HURTING","DAMAGE","DAMAGED","DEBRIS","EXPLODE","EXPLOSION","DEATHANIM","DEATHFX"],
        "modifiers": ["","_ANIM","ANIM","_LOOP","LOOP","_STATE","_FRAME","FRAME","_SPR","SPR","_FX","FX","_PARTICLE","PARTICLE","_DEBRIS","DEBRIS"],
    },
    0x18e88910: {
        "context": "GlennYntisDeathEventHandler",
        "subjects": ["GLEN","GLENN","GLENNYNTIS","YNTIS","YNT","GLENYNT","GY"],
        "actions": ["DIE","DEATH","DYING","DEAD","KILL","KILLED","DEFEAT","DEFEATED","FALL","FALLING","HURT","HURTING","DAMAGE","DAMAGED","DEBRIS","EXPLODE","EXPLOSION","DEATHANIM","DEATHFX"],
        "modifiers": ["","_ANIM","ANIM","_LOOP","LOOP","_STATE","_FRAME","FRAME","_SPR","SPR","_FX","FX","_PARTICLE","PARTICLE","_DEBRIS","DEBRIS"],
    },
    # InitMonkeyMageBoss: 0x0244655d, 0x181c3854, 0x8818a018
    0x0244655d: {
        "context": "InitMonkeyMageBoss",
        "subjects": ["MONKEYMAGE","MAGE","MONKEY","BOSS","MM","MONKEYMAGEBOSS","KING","MAGEMONK"],
        "actions": ["INIT","SPAWN","ENTRY","ENTER","INTRO","START","STARTING","APPEAR","APPEARING","BIRTH","ANIM","SPRITE","SPR","BASE","IDLE","STAND","STANDING"],
        "modifiers": ["","_ANIM","ANIM","_SPR","SPR","_FRAME","FRAME","_LOOP","LOOP","_STATE","_FX","FX","_ICON","ICON"],
    },
    # InitPhartHeadCollectible: 0x8c510186, 0xa9240484
    0x8c510186: {
        "context": "InitPhartHeadCollectible",
        "subjects": ["PHART","PHARTHEAD","FART","FARTHEAD","HEAD","COLLECTIBLE","ITEM","PICKUP","BONUS","POWERUP","WEAPON"],
        "actions": ["","ICON","ANIM","SPR","FX","ITEM","PICKUP","COLLECT","COLLECTED","SPIN","SPINNING","ROTATE","ROTATING","FLOAT","FLOATING"],
        "modifiers": ["","_ICON","ICON","_ANIM","ANIM","_SPR","SPR","_FX","FX","_PIC","PIC","_FRAME","FRAME","_LOOP","LOOP","_STATE","_PICKUP","PICKUP","_COLLECT","COLLECT","_BONUS","BONUS","_ITEM","ITEM"],
    },
    # CollectibleHamsterShieldTickCallback: 0x42906465, 0xc2906565
    0x42906465: {
        "context": "CollectibleHamsterShieldTickCallback",
        "subjects": ["HAMSTER","HAMSTERSHIELD","SHIELD","COLLECTIBLE","ITEM","PICKUP","BONUS","POWERUP"],
        "actions": ["","ICON","ANIM","SPR","FX","ITEM","PICKUP","COLLECT","SPIN","SPINNING","TICK","CALLBACK","STATE","FRAME","LOOP","CYCLE","SHIMMER","GLOW","SHINE","SPARKLE","TWINKLE"],
        "modifiers": ["","_ICON","ICON","_ANIM","ANIM","_SPR","SPR","_FX","FX","_PIC","PIC","_FRAME","FRAME","_LOOP","LOOP","_STATE","_PICKUP","PICKUP","_COLLECT","COLLECT","_BONUS","BONUS","_ITEM","ITEM"],
    },
    # Collectible1970IconTickCallback: 0x408a6461, 0x428a6465 
    0x408a6461: {
        "context": "Collectible1970IconTickCallback",
        "subjects": ["1970","ICON1970","SWIRLY","SWIRLYQ","CHECKPOINT","CHECKPNT","BONUS","ITEM","PICKUP","COLLECTIBLE","COLLECT","FREEBIE","Q","SWIRL"],
        "actions": ["","ICON","ANIM","SPR","FX","ITEM","PICKUP","COLLECT","SPIN","SPINNING","FLOAT","ROTATE","FLOATING","ROTATING","TICK","FRAME","LOOP","CYCLE","SHIMMER","GLOW","SHINE","SPARKLE","TWINKLE","BLINK"],
        "modifiers": ["","_ICON","ICON","_ANIM","ANIM","_SPR","SPR","_FX","FX","_PIC","PIC","_FRAME","FRAME","_LOOP","LOOP","_STATE","_PICKUP","PICKUP","_COLLECT","COLLECT","_BONUS","BONUS","_ITEM","ITEM","_BLINK","BLINK","_SHINE","SHINE","_GLOW","GLOW","_SHIMMER","SHIMMER","_SPARKLE","SPARKLE","_TWINKLE","TWINKLE"],
    },
    # KloggDeathEventHandler: 0x800da102, 0x800da116, 0x800da11a, 0x800da132, 0x800da152, 0x888da193
    0x800da102: {
        "context": "KloggDeathEventHandler",
        "subjects": ["KLOGG","KLOG","KLOGGTHEKLOGG","KLOGGBALL","BOSS","KING","SCIE","SCIEBOSS"],
        "actions": ["DIE","DEATH","DYING","DEFEAT","DEFEATED","FALL","FALLING","HURT","HURTING","EXPLODE","EXPLOSION","DEBRIS","BURST","SHATTER","CRACK","BREAK","DESTROY","DESTROYED","DEATHANIM","DEATHFX","DEATHPART","DEATHBURST"],
        "modifiers": ["","_ANIM","ANIM","_FX","FX","_PARTICLE","PARTICLE","_DEBRIS","DEBRIS","_PIECE","PIECE","_PART","PART","_FRAGMENT","FRAGMENT","_CHUNK","CHUNK","_SHARD","SHARD"],
    },
}


def gen_candidates(ctx_dict):
    cands = set()
    for s in ctx_dict["subjects"]:
        for a in ctx_dict["actions"]:
            for m in ctx_dict["modifiers"]:
                if a:
                    cands.add(s + a + m)
                    cands.add(s + "_" + a + m)
                    cands.add(s + a + "_" + m if m else s + a)
                    cands.add(s + "_" + a + "_" + m if m else s + "_" + a)
                    cands.add(a + s + m)
                    cands.add(a + "_" + s + m)
                if m:
                    cands.add(s + m)
                    cands.add(s + "_" + m)
                cands.add(s)  # alone
    return cands


print(f"=== Loaded {len(ids)} asset IDs ===\n")
total = 0
hits = []
for tid, ctx in CONTEXTS.items():
    cands = gen_candidates(ctx)
    found = []
    for c in cands:
        if asset_id(c) == tid:
            found.append(c)
    total += len(cands)
    print(f"0x{tid:08x}  {ctx['context']:40s}  tested {len(cands):,}  hits={len(found)}")
    for f in found:
        print(f"    *** {f}")
        hits.append((tid, f, ctx['context']))

print(f"\n=== TOTAL: {len(hits)} hits across {total:,} candidates tested ===")
for tid, name, ctx in hits:
    print(f"  0x{tid:08x}  {name:40s}  ({ctx})")
