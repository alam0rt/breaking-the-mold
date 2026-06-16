#!/usr/bin/env python3
"""Round 10: function-context-driven targeted attack.

Uses the actual function context from asset_usage_by_function.csv to
generate name candidates that match what each function does.

Key insights from the CSV:
  - The player IS Finn (PlayerState_* functions = Finn states)
  - Clayball is a player projectile (ClayballState_*)
  - Joe Head Joe is a boss (JoeHeadJoe*)
  - Glenn Yntis is a boss (GlennYntis*)
  - Klogg is a boss (Klogg*)
  - Monkey Mage is a boss (MonkeyMage*)
  - Phart Head is a collectible/enemy (PhartHead)
  - Shriney Guard is enemy (Shriney*)

For each function, generate names matching:
  1. Function context (what it does)
  2. Subject (Finn/Joe/Klogg/etc.)
  3. Action (Idle/Walk/Jump/Attack/Death/etc.)
"""
import sys, csv, itertools
sys.path.insert(0, "tools/scripts")
from compound_hash_attack import asset_id

ROOT = "/home/sam/projects/btm"

ids_dict = {}
with open(f"{ROOT}/docs/reference/asset-ids-master.csv") as f:
    rdr = csv.DictReader(f)
    for r in rdr:
        ids_dict[int(r["id_hex"], 16)] = r
ids = set(ids_dict)

# Subject => alternatives for naming. Player IS Finn so try both!
SUBJECTS = {
    "PLAYER": ["PLAYER", "FINN", "KLAYMEN", "KLAY", "GUY", "P1", "PLAY", "HERO", "P", "MAN"],
    "FINN": ["FINN", "PLAYER", "P1", "P", "F", "GUY", "MAN"],
    "JOE": ["JOE", "JOEHEAD", "JOEHEADJOE", "JHJ", "HEAD", "BIGHEAD", "JOE_HEAD", "JOEHEADJOE", "JOEJOE"],
    "JOEHEAD": ["JOE", "JOEHEAD", "JOEHEADJOE", "JHJ", "HEAD", "JOEJOE"],
    "JOEHEADJOE": ["JOE", "JOEHEAD", "JOEHEADJOE", "JHJ", "HEAD", "BIGHEAD"],
    "KLOGG": ["KLOGG", "KLOG", "KLG", "KLOGGBOSS", "KLOGFINAL"],
    "GLENN": ["GLENN", "GLENNYNTIS", "YNTIS", "GLN", "GY", "GLEN"],
    "GLENNYNTIS": ["GLENN", "GLENNYNTIS", "YNTIS", "GLN", "GY"],
    "MONKEYMAGE": ["MONKEYMAGE", "MAGE", "MONKEY", "MM", "MAGE", "WIZ", "WIZARD"],
    "PHARTHEAD": ["PHART", "PHARTHEAD", "PH"],
    "CLAYBALL": ["CLAYBALL", "BALL", "CLAY", "CB"],
    "SHRINEY": ["SHRINEY", "SHRN", "SHR", "GUARD"],
    "SHRINEYGUARD": ["SHRINEY", "SHRN", "SHR", "GUARD"],
    "RUNN": ["RUNN", "RN", "RUN"],
    "BOSS": ["BOSS", "B"],
    "ENEMY": ["ENEMY", "EN", "FOE", "BAD"],
    "MENU": ["MENU", "MNU"],
    "PASSWORD": ["PASSWORD", "PWD", "PSWD", "PASS"],
    "HAZARD": ["HAZARD", "HAZ", "HZ"],
    "TELEPORTER": ["TELEPORTER", "TELE", "TP", "PORTAL"],
    "PLATFORM": ["PLATFORM", "PLAT", "PFM", "PF"],
    "CHECKPOINT": ["CHECKPOINT", "CHECK", "CP", "SAVE"],
    "BONUS": ["BONUS", "BNS", "BONUS_ROOM"],
    "COLLECTIBLE": ["COLLECTIBLE", "COIN", "ITEM", "COLLECT", "PICKUP"],
}

ACTIONS = {
    "IDLE": ["IDLE", "STAND", "STANDING", "REST", "RESTING", "WAIT", "WAITING", "POSE", "DEFAULT", "STILL"],
    "STANDING": ["STAND", "STANDING", "IDLE", "REST"],
    "WALKING": ["WALK", "WALKING", "MOVE", "MOVING", "GO", "GOING"],
    "JUMP": ["JUMP", "JUMPING", "JMP", "LEAP", "AIR"],
    "FALLING": ["FALL", "FALLING", "FLL", "DROP"],
    "ATTACK": ["ATTACK", "ATK", "ATTACKING", "HIT", "HITTING", "STRIKE", "PUNCH", "KICK", "BASH"],
    "DEATH": ["DEATH", "DIE", "DYING", "DEAD", "KO", "KILLED"],
    "DAMAGE": ["DAMAGE", "DMG", "HURT", "HIT", "OUCH", "PAIN"],
    "SWIM": ["SWIM", "SWIMMING", "WATER"],
    "SPAWN": ["SPAWN", "INIT", "BIRTH", "ENTER"],
    "PICKUP": ["PICKUP", "PICK", "GRAB", "TAKE", "GET", "HOLD"],
    "ROLLING": ["ROLL", "ROLLING", "ROLLED", "ROLLER"],
    "BALL": ["BALL", "ROLL", "ROLLER", "SPHERE"],
    "DEFEAT": ["DEFEAT", "DIE", "LOSE", "LOST", "BEATEN"],
    "EXPLODE": ["EXPLODE", "BLAST", "BOOM", "EXPL", "BURST", "BURSTING"],
    "DESTROY": ["DESTROY", "BREAK", "SMASH", "DESTROYED", "BROKEN"],
    "CALLBACK": [],  # internal noun
    "EVENT": [],
    "HANDLER": [],
    "STATE": [],
    "INIT": ["INIT", "SPAWN", "CREATE", "NEW", "MAKE", "BIRTH"],
    "DEBRIS": ["DEBRIS", "PIECE", "PIECES", "FRAG", "PARTS", "BIT", "BITS", "CHUNK"],
    "PARTICLE": ["PARTICLE", "PCL", "PART", "FX", "EFFECT"],
    "PROJECTILE": ["PROJECTILE", "PROJ", "BULLET", "SHOT", "PRJ"],
    "FLY": ["FLY", "FLYING", "FLOAT", "FLOATING", "AIR", "AIRBORNE"],
    "RISE": ["RISE", "RISING", "UP", "FLOAT", "LIFT"],
    "FLOAT": ["FLOAT", "FLOATING", "BOB", "BOBBING", "HOVER", "HOVERING"],
    "PORTAL": ["PORTAL", "WARP", "TELEPORT", "GATE"],
    "SWIRL": ["SWIRL", "SPIRAL", "SPIN", "SPINNING"],
    "CUTSCENE": ["CUTSCENE", "CINEMA", "MOVIE", "SCENE", "STORY"],
    "SOUND": ["SOUND", "SND", "VOICE"],
    "VICTORY": ["VICTORY", "WIN", "WON", "WINNING", "TRIUMPH"],
    "BUTTON": ["BUTTON", "BTN", "B"],
    "ICON": ["ICON", "ICN", "IMAGE", "PIC"],
    "SKULL": ["SKULL", "SKL", "MONKEY", "SKULLMONKEY"],
}

# Function-context-derived targets
TARGETS = [
    # PlayerState_* sprites (Finn states)
    (0x3838801a, "PlayerState_StandingIdle", ["FINN", "PLAYER"], ["IDLE", "STAND", "STANDING"]),
    (0x00388110, "PlayerState_StandingIdle", ["FINN", "PLAYER"], ["IDLE", "STAND", "STANDING"]),
    (0x1c395196, "PlayerState_StandingIdle/Pickup", ["FINN", "PLAYER"], ["PICKUP", "IDLE", "STAND", "GRAB"]),
    (0x1c3aa013, "PlayerState_PickupItem", ["FINN", "PLAYER"], ["PICKUP", "GRAB", "TAKE", "GET"]),
    (0x092b8480, "PlayerState_Jump/Falling", ["FINN", "PLAYER"], ["JUMP", "FALL", "JUMPING", "FALLING"]),
    (0x0b2084d0, "PlayerState_Falling", ["FINN", "PLAYER"], ["FALL", "FALLING", "DROP"]),
    (0x88b9833c, "PlayerState_Falling", ["FINN", "PLAYER"], ["FALL", "FALLING", "DROP"]),
    (0x1e089010, "PlayerStateInit_Walking", ["FINN", "PLAYER"], ["WALK", "WALKING", "MOVE"]),
    (0x70d006d8, "PlayerStateInit_Walking (anim loop)", ["FINN", "PLAYER"], ["WALK", "WALKING", "MOVE"]),
    (0x06e0a824, "PlayerState_StartSwimming (sound)", ["FINN", "PLAYER"], ["SWIM", "SPLASH", "WATER"]),
    
    # Joe Head Joe (boss)
    (0x0b290ba2, "JoeHeadJoeSetIdleState", ["JOE", "JOEHEAD", "JOEHEADJOE"], ["IDLE", "STAND", "REST"]),
    (0x0e3889be, "JoeHeadJoeMoveAndCheckAttack", ["JOE", "JOEHEAD", "JOEHEADJOE"], ["MOVE", "WALK", "ATTACK"]),
    (0x603588e8, "JoeHeadJoeSetIdleState (sound)", ["JOE", "JOEHEAD", "JOEHEADJOE"], ["IDLE", "VOICE", "GROAN"]),
    (0x069580a0, "JoeHeadJoeBallFallCallback", ["JOE", "JOEHEAD", "JOEHEADJOE", "BALL"], ["FALL", "BALL", "DROP"]),
    (0x34014841, "JoeHeadJoeBallFall (sound)", ["JOE", "JOEHEAD", "JOEHEADJOE", "BALL"], ["FALL", "DROP", "LAND"]),
    (0x0239cb33, "JoeHeadJoeBallStartRolling", ["JOE", "JOEHEAD", "BALL"], ["ROLL", "ROLLING", "BALL", "START"]),
    (0x70a09ce1, "JoeHeadJoeBallStartRolling (sound)", ["JOE", "JOEHEAD", "BALL"], ["ROLL", "BALL"]),
    
    # Klogg (boss)
    (0xcaadc032, "InitKloggBoss (HP bar)", ["KLOGG"], ["HP", "HPBAR", "BAR", "HEALTH", "GAUGE"]),
    (0x486492c4, "InitKloggBoss (sound)", ["KLOGG"], ["INIT", "SPAWN", "INTRO"]),
    (0x800da102, "KloggDeathEventHandler particle 1", ["KLOGG"], ["DEATH", "DIE", "DEBRIS", "PIECE", "EXPLODE"]),
    (0x800da116, "KloggDeathEventHandler particle 2", ["KLOGG"], ["DEATH", "DIE", "DEBRIS", "PIECE", "EXPLODE"]),
    (0x800da11a, "KloggDeathEventHandler particle 3", ["KLOGG"], ["DEATH", "DIE", "DEBRIS", "PIECE"]),
    (0x800da132, "KloggDeathEventHandler particle 4", ["KLOGG"], ["DEATH", "DIE", "DEBRIS", "PIECE"]),
    (0x800da152, "KloggDeathEventHandler particle 5", ["KLOGG"], ["DEATH", "DIE", "DEBRIS", "PIECE"]),
    (0x888da193, "KloggDeathEventHandler particle 6", ["KLOGG"], ["DEATH", "DIE", "DEBRIS", "PIECE"]),
    (0x083488e2, "KloggSpawnProjectilesCallback", ["KLOGG"], ["PROJECTILE", "SHOT", "BULLET"]),
    (0x0c34aa22, "KloggSpawnProjectilesCallback", ["KLOGG"], ["PROJECTILE", "SHOT", "BULLET"]),
    (0x1a2c8516, "KloggSpawnProjectilesCallback", ["KLOGG"], ["PROJECTILE", "SHOT", "BULLET"]),
    
    # Glenn Yntis (boss)
    (0x18e88310, "GlennYntisDeathEventHandler", ["GLENN", "YNTIS"], ["DEATH", "DEBRIS", "PIECE", "EXPLODE"]),
    (0x18e88510, "GlennYntisDeathEventHandler", ["GLENN", "YNTIS"], ["DEATH", "DEBRIS", "PIECE"]),
    (0x18e88910, "GlennYntisDeathEventHandler", ["GLENN", "YNTIS"], ["DEATH", "DEBRIS", "PIECE"]),
    (0x18e8c704, "GlennYntisDeathEventHandler", ["GLENN", "YNTIS"], ["DEATH", "DEBRIS", "PIECE"]),
    (0x8b603980, "GlennYntisDamageEventHandler", ["GLENN", "YNTIS"], ["DAMAGE", "HURT", "HIT"]),
    (0xe0c00650, "GlennYntis sound effect", ["GLENN", "YNTIS"], ["SOUND", "VOICE", "GRUNT"]),
    
    # Monkey Mage  
    (0x0244655d, "InitMonkeyMageBoss", ["MONKEYMAGE", "MAGE"], ["INIT", "SPAWN", "INTRO"]),
    (0x181c3854, "InitMonkeyMageBoss bonus", ["MONKEYMAGE", "MAGE"], ["BONUS", "ITEM", "REWARD"]),
    (0x8818a018, "InitMonkeyMageBoss particle", ["MONKEYMAGE", "MAGE"], ["PARTICLE", "MAGIC", "EFFECT", "SPELL"]),
    
    # Shriney Guard
    (0x086e44c0, "ShrineyGuard sound 1", ["SHRINEY", "GUARD"], ["VOICE", "SOUND", "GRUNT"]),
    (0x11eac4c0, "ShrineyGuard sound 2", ["SHRINEY", "GUARD"], ["VOICE", "SOUND", "GRUNT"]),
    (0x204e44c0, "ShrineyGuard sound 3", ["SHRINEY", "GUARD"], ["VOICE", "SOUND", "GRUNT"]),
    (0x205e40c2, "ShrineyGuard voice handle", ["SHRINEY", "GUARD"], ["VOICE", "SOUND"]),
    (0x40106054, "ShrineyGuardSetLoopingAttackState", ["SHRINEY", "GUARD"], ["ATTACK", "LOOP", "ATK"]),
    (0xc00200c9, "ShrineyGuardSetLoopingAttackState frame", ["SHRINEY", "GUARD"], ["FRAME", "LOOP"]),
    (0x085860d4, "ShrineyGuardAttackAnimState", ["SHRINEY", "GUARD"], ["ATTACK", "ATK", "ANIM"]),
    (0x8192250, "ShrineyGuardReadyAttackState", ["SHRINEY", "GUARD"], ["READY", "PREATTACK", "WINDUP"]),
    
    # Phart Head
    (0x8c510186, "InitPhartHeadCollectible", ["PHART"], ["IDLE", "FLOAT", "COLLECT"]),
    (0xa9240484, "InitPhartHeadCollectible", ["PHART"], ["IDLE", "FLOAT", "COLLECT"]),
    
    # Clayball  
    (0x462c6040, "ClayballState_DestroyWithDebris", ["CLAYBALL", "BALL"], ["DEBRIS", "EXPLODE", "DESTROY", "PIECE"]),
    (0x46346040, "ClayballState_DestroyWithDebris", ["CLAYBALL", "BALL"], ["DEBRIS", "EXPLODE", "DESTROY"]),
    (0x46386040, "ClayballState_DestroyWithDebris", ["CLAYBALL", "BALL"], ["DEBRIS", "EXPLODE", "DESTROY"]),
    (0x463e6040, "ClayballState_DestroyWithDebris", ["CLAYBALL", "BALL"], ["DEBRIS", "EXPLODE", "DESTROY"]),
    (0xd6bc2450, "ClayballState_DestroyWithDebris", ["CLAYBALL", "BALL"], ["DEBRIS", "EXPLODE"]),
    (0x082e03d8, "ClayballSpawnSwitchBlock", ["CLAYBALL", "BALL"], ["SWITCH", "BLOCK", "TRIGGER"]),
    (0x222222d8, "ClayballSpawnSwitchBlock", ["CLAYBALL", "BALL"], ["SWITCH", "BLOCK"]),
    (0x262e0319, "ClayballSpawnSwitchBlock", ["CLAYBALL", "BALL"], ["SWITCH", "BLOCK"]),
    
    # Finn (combined)
    (0x088c5011, "FinnDeathExplosion (floor 7)", ["FINN", "PLAYER"], ["DEATH", "EXPLODE", "DEBRIS", "DIE"]),
    (0x344210b1, "FinnDeathExplosion particle", ["FINN", "PLAYER"], ["DEATH", "EXPLODE", "DEBRIS"]),
    (0x3da80d13, "CreateFinnPlayerEntity sprite", ["FINN", "PLAYER"], ["PLAYER", "MAIN", "SPAWN", "BODY"]),
    (0xcc6c8070, "CreateFinnPlayer sound", ["FINN", "PLAYER"], ["SPAWN", "VOICE", "INTRO"]),
    (0x1b280c33, "FinnState_Spawn", ["FINN", "PLAYER"], ["SPAWN", "INIT", "INTRO", "BIRTH"]),
    
    # FinnDamage shared with bosses
    (0x146ce002, "FinnDamage shared sentinel", ["FINN", "PLAYER", "EXPLODE"], ["DAMAGE", "DAMAGE_SOURCE", "DEATH", "EXPLODE", "DEBRIS"]),
    (0x3d348056, "Shared death/damage sentinel", ["FINN", "BOSS", "EXPLODE"], ["DEATH", "EXPLODE", "DEBRIS", "DAMAGE"]),
    (0xb468d0c6, "Shared death debris", ["FINN", "BOSS", "EXPLODE"], ["DEBRIS", "PIECE", "EXPLODE"]),
    (0xb868d0c6, "Shared death debris", ["FINN", "BOSS", "EXPLODE"], ["DEBRIS", "PIECE", "EXPLODE"]),
    (0xbe68d0c6, "Shared death debris", ["FINN", "BOSS", "EXPLODE"], ["DEBRIS", "PIECE", "EXPLODE"]),
    
    # Menu
    (0x10094096, "InitMenuStage1/3/4 (button entity)", ["MENU"], ["BUTTON", "ENTITY", "OPTION"]),
    (0x81100030, "InitMenuStage3 entity", ["MENU"], ["BUTTON", "ENTITY", "OPTION"]),
    (0x68c01218, "InitMenuStage1 sprite", ["MENU"], ["FONT", "TEXT", "MAIN"]),
    
    # Password
    (0x42906465, "PasswordSubmit sound 1", ["PASSWORD", "PWD"], ["OK", "SUBMIT", "SOUND"]),
    (0x64221e61, "PasswordSubmit sound 2", ["PASSWORD", "PWD"], ["OK", "SUBMIT", "SOUND"]),
]

# Generate and test
print(f"Testing {len(TARGETS)} targeted IDs...")

# Build candidate set per target
new_hits = []
all_tested = 0
all_seen = set()

modifiers_with = ["", "_ANIM", "ANIM", "_SPR", "SPR", "_FRAME", "FRAME", "_LOOP", "LOOP",
                  "_FX", "FX", "_FRAME1", "_FRAME2", "_1", "_2", "_3", "_A", "_B"]

for tid, role, subj_list, act_list in TARGETS:
    cands = set()
    
    # Build subject + action cross
    subj_alts = set()
    for s in subj_list:
        subj_alts.update(SUBJECTS.get(s, [s]))
    
    act_alts = set()
    for a in act_list:
        act_alts.update(ACTIONS.get(a, [a]))
    
    for s in subj_alts:
        for a in act_alts:
            for mod in modifiers_with:
                # Try various formats
                cands.add(s + a + mod)
                cands.add(s + "_" + a + mod)
                cands.add(s + "_" + a + "_" + mod.lstrip("_") if mod else s + "_" + a)
                cands.add(a + s + mod)
                cands.add(a + "_" + s + mod)
                cands.add(s + a)
                # Just subject (might be the sprite)
                cands.add(s + mod)
                cands.add(s)
                # Just action  
                cands.add(a + mod)
                cands.add(a)
    
    found = []
    for c in cands:
        h = asset_id(c)
        all_tested += 1
        if h == tid:
            found.append(c)
        elif h in ids and h not in all_seen:
            # Bonus: caught a different ID!
            all_seen.add(h)
            info = ids_dict[h]
            if h in {asset_id("NO"), asset_id("YES"), asset_id("PAUSED"),
                     asset_id("QUIT"), asset_id("CONTINUE"), asset_id("QUITGAME")}:
                continue
            print(f"  BONUS  0x{h:08x}  {c:35s}  ({role[:30]})  popc={info['popcount']}")
            new_hits.append((h, c, "bonus from " + role))
    
    if found:
        print(f"  ✓✓✓  0x{tid:08x}  {role:50s}  →  {found[0]}")
        for f in found:
            new_hits.append((tid, f, role))

print(f"\n=== Total candidates tested: {all_tested:,} ===")
print(f"=== New hits found: {len(new_hits)} ===")

if new_hits:
    with open(f"{ROOT}/docs/analysis/asset-identification/round10_function_context.csv", "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["id_hex","candidate","context"])
        for h, c, ctx in new_hits:
            w.writerow([f"0x{h:08x}", c, ctx])
