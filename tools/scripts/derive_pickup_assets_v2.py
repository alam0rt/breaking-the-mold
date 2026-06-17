"""Target specific sprite/asset hashes with vocab derived from the verified
FX_PICKUP_* sound names. The sprite asset likely uses the same root token.

Known sprite IDs and their game roles (from collectible Init functions):
  0x6A351094 -> UniverseEnema sprite       (FX_PICKUP_UNIVERSE_ENEMA exists)
  0x88A28194 -> 1970Icon sprite            (FX_PICKUP_1970 exists)
  0x80E85EA0 -> HamsterShield sprite       (no FX_PICKUP_HAMSTER yet)
  0x902C0002 -> SuperWillie sprite         (FX_PICKUP_SUPER_WILLIE exists)
  0x8C30008C -> ScaleReset sprite          (FX_PICKUP_GROW exists)
  0x08624580 -> SingleFrameDecor sprite    (100C trigger - generic)

Boss/death sprites:
  0xBE68D0C6 / 0xB868D0C6 -> death debris  (used in PlayerCallback_DeathDebrisSpawner)
  0xB01C25F0 -> death-grow / JHJ ball fall
  0x0E3889BE -> JoeHeadJoe charging/landed
  0x069580A0 -> JoeHeadJoe ball splat
  0xBB781481 -> wake-decor sprite (DecorPlaySoundAndAnimate plays 0x40023e30 then sprites to this)
"""
import sys, os
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

TARGETS = {
    0x6A351094: ("UniverseEnema sprite", [
        # Direct names from FX_PICKUP_UNIVERSE_ENEMA
        "UNIVERSE_ENEMA","UNIVERSEENEMA","UNIVERSE_ENEMA_ICON","ENEMA_ICON","UE_ICON",
        "ENEMA","UE","UNIV_ENEMA","UNIVERSE","UNV_ENEMA",
        # With common asset prefixes
        "ICON_UNIVERSE_ENEMA","ICON_UNIVERSEENEMA","ICON_ENEMA","ICON_UE",
        "SPR_UNIVERSE_ENEMA","SPR_UNIVERSEENEMA","SPR_ENEMA","SPR_UE",
        "PICKUP_UNIVERSE_ENEMA","PICKUP_UNIVERSEENEMA","PICKUP_ENEMA","PICKUP_UE",
        "BLB_UNIVERSE_ENEMA","BLB_ENEMA","BLB_UE",
        "AS_UNIVERSE_ENEMA","ENT_UNIVERSE_ENEMA","ITEM_UNIVERSE_ENEMA","COLLECT_UNIVERSE_ENEMA",
        "ANIM_UNIVERSE_ENEMA","ANIM_UE","AN_UNIVERSE_ENEMA","AN_UE",
        # Visual aliases
        "BAG","BAG_UE","ENEMABAG","ENEMA_BAG","NUKE","SCREEN_NUKE","SCREEN_CLEAR",
        "BOMB","SUPER_BOMB","SUPERBOMB","MEGA_BOMB","MEGABOMB","BIGBLAST","BIG_BLAST",
        "POOPBOMB","POOP_BOMB","BLB_BOMB","KILL_ALL","KILLALL","WIPEOUT",
    ]),
    0x88A28194: ("1970Icon sprite", [
        "1970","ICON_1970","1970_ICON","ONE_NINE_SEVEN_ZERO","1970ICON",
        "SPR_1970","BLB_1970","PICKUP_1970","COLLECT_1970","BONUS_1970",
        "AS_1970","ENT_1970","ITEM_1970","ANIM_1970","AN_1970",
        # 1970 = Earthworm Jim reference - try "HAMSTER" / "JIM" / "WORM"
        "EARTH_WORM","EARTHWORM","WORM","JIM","JIMICON","JIM_ICON",
        "PSYGNOSIS","PSYG","PSGN","RETRO","CLASSIC","OLDIE",
        "BONUS","BONUS_ICON","SECRET","SECRET_ICON","HIDDEN",
        "EXTRA","EXTRA_ICON","HAMSTER_ICON","HAM_ICON",
    ]),
    0x80E85EA0: ("HamsterShield sprite", [
        "HAMSTER","HAMSTER_SHIELD","HAMSTERSHIELD","HAMSTER_ICON","HAMSTERICON",
        "SPR_HAMSTER","BLB_HAMSTER","PICKUP_HAMSTER","COLLECT_HAMSTER","ITEM_HAMSTER",
        "ANIM_HAMSTER","AN_HAMSTER",
        "FUR","FURBALL","FUR_BALL","FUZZ","FUZZBALL","FUZZY","FUZZY_PET","PET","PETS",
        "BUDDY","FRIEND","HELPER","MINION","ANIMAL","CRITTER",
        "RAT","MOUSE","MICE","RODENT","GERBIL","GUINEA","GUINEAPIG",
        "SHIELD","HAMSHIELD","HAMSTER_SHLD","HSHIELD","H_SHIELD",
        "TRIPLET","TRIPLE","THREE_PET","THREE_FRIEND","TRIO",
    ]),
    0x902C0002: ("SuperWillie sprite", [
        "SUPER_WILLIE","SUPERWILLIE","WILLIE","SW_ICON","SW","SUPERWIL","SUPERW",
        "ICON_SUPER_WILLIE","ICON_SUPERWILLIE","ICON_WILLIE","ICON_SW",
        "SPR_SUPER_WILLIE","SPR_SUPERWILLIE","SPR_WILLIE","SPR_SW",
        "BLB_SUPER_WILLIE","BLB_WILLIE","BLB_SW","PICKUP_SUPER_WILLIE",
        "PICKUP_SUPERWILLIE","PICKUP_WILLIE","PICKUP_SW","COLLECT_SUPER_WILLIE",
        "ITEM_SUPER_WILLIE","BONUS_SUPER_WILLIE","ANIM_SUPER_WILLIE","AN_SW",
        # Visual aliases
        "WILLY","SUPER_WILLY","SUPERWILLY","TRANSFORM","MORPH","NEWFORM","FORM_CHANGE",
        "WILLIE_ICON","WILLIE_HEAD","WILLIE_FACE","WILLIE_PWR","WILLIE_PWRUP",
    ]),
    0x8C30008C: ("ScaleReset/Grow sprite", [
        "GROW","SCALE_RESET","SCALERESET","RESET","RESIZE","RESET_SCALE",
        "ICON_GROW","ICON_SCALE_RESET","ICON_RESET","ICON_RESIZE",
        "SPR_GROW","SPR_SCALE_RESET","SPR_RESET","SPR_RESIZE",
        "BLB_GROW","BLB_RESET","PICKUP_GROW","PICKUP_SCALE_RESET","PICKUP_RESET",
        "COLLECT_GROW","ITEM_GROW","ANIM_GROW","AN_GROW",
        "GROWTH","GROWING","BIG","BIGGER","ENLARGE","NORM","NORMAL","NORMAL_SIZE",
        "SCALE_NORM","SCALE_NORMAL","RESET_SIZE","UNSHRINK","UNSCALE",
        "RESTORE_SIZE","RESTORE","FULLSIZE","FULL_SIZE",
    ]),
    0x08624580: ("SingleFrameDecor / 100C trigger sprite", [
        "100C","100","HUNDRED","HUNDRED_C","HUNDREDC","C100","ONE_HUNDRED",
        "ICON_100","ICON_100C","ICON_C","ICON_HUNDRED","SPR_100","SPR_100C","SPR_C",
        "BLB_100","BLB_100C","BLB_C","TRIGGER","TRIGGER_100","TRIGGER_100C",
        "PICKUP_100","PICKUP_100C","PICKUP_C","COLLECT_100","COLLECT_100C",
        "DECOR","DECOR_ICON","SINGLE_FRAME","SINGLEFRAME","FRAME","FRAME_ICON",
        # Common Klaymen-themed
        "NEON","NEON_C","NEONC","FLOATY","FLOATY_C","FLOAT_C","FLOATING_C",
        "BONUS_GATE","BONUS_C","CENTENNIAL","BONUS_ENTRY","SECRET_C",
    ]),
    0xBE68D0C6: ("death debris sprite (large)", [
        "DEBRIS","GIB","GIBS","GIBLET","CHUNK","CHUNKS","BIT","BITS","FRAG","FRAGMENT",
        "EXPLODE","EXPLODE_BIG","EXPLOSION","BURST","SCATTER","SPLAT","SPLATTER",
        "BLB_DEBRIS","BLB_GIB","BLB_CHUNK","BLB_BIT","BLB_BLOB","BLB_BURST",
        "DEATH_DEBRIS","DEATH_GIB","DEATH_BURST","DEATH_BIT","KLAY_DEBRIS","KLAY_GIB",
        "DEATH","KLAY_DEATH","DEATH_BIG","DEATH_BIT","DEATH_BLOB","BLOOD_BLOB",
        "BLOB","BLOBS","CLAYBLOB","CLAY_BLOB","CLAYBIT","CLAY_BIT","CLAYCHUNK","CLAY_CHUNK",
        "BLB_FRAG","BLB_PARTICLE","PARTICLE","KLAY_PARTICLE","BLB_FX",
        "PIECE","PIECES","SCRAP","SCRAPS","SHARD","SHARDS","SLIVER",
    ]),
    0xB01C25F0: ("death-grow / JHJ ball sprite", [
        "DEATH_GROW","DEATHGROW","GROW_DEATH","BIG_DEATH","DEATH_BIG",
        "JHJ_BALL","JOE_BALL","JOEBALL","HEAD_BALL","HEADBALL","SKULL_BALL","SKULLBALL",
        "BALL","BIG_BALL","HEAVY_BALL","SPRITE_BALL","BOSS_BALL",
        "DEATH","KLAY_DEATH","DIE","DEATH_FALL","FALL_DEATH","DEATH_DROP",
        "EXPLODE","DEATH_EXPLODE","EXPLODE_GROW","EXPLODE_BIG","GROW_EXPLODE",
        "GROWING","GROWING_BLOB","SWELL","SWELLING","EXPAND","EXPANDING",
    ]),
    0x0E3889BE: ("JoeHeadJoe charging/landed sprite", [
        "JHJ","JOE","JOEHEAD","JOE_HEAD","HEAD","HEADJOE","HEAD_JOE",
        "JOE_LAND","JHJ_LAND","JOE_HEAD_LAND","JHJ_CHARGE","JOE_CHARGE","JOE_HEAD_CHARGE",
        "JHJ_ATTACK","JOE_ATTACK","JOE_HEAD_ATTACK","JHJ_ANGRY","JOE_ANGRY",
        "JOE_SPLAT","JHJ_SPLAT","JOE_LANDED","JHJ_LANDED",
        "JOE_BIG","BIG_JOE","BIG_HEAD","BIGGEST","BOSS","BOSS_BIG","BOSS_HEAD",
        "JOEACE","JOE_ACE","ACE","BOSS_JOE","BOSS_JHJ",
    ]),
    0x069580A0: ("JoeHeadJoe ball splat sprite", [
        "BALL_SPLAT","BALLSPLAT","BALL_LAND","BALLLAND","BALL_DOWN","BALLDOWN",
        "JHJ_BALL_SPLAT","JOE_BALL_SPLAT","JHJ_BALL_LAND","JOE_BALL_LAND",
        "JOE_HEAD_BALL_SPLAT","JOE_HEAD_BALL_LAND","HEAD_BALL_SPLAT","HEAD_BALL_LAND",
        "BALL_SQUASH","BALLSQUASH","BALL_FLAT","BALLFLAT","BALL_PLOP","BALLPLOP",
        "BALL_DEAD","BALLDEAD","BALL_END","BALLEND","SPLAT","SPLATTED",
        "SPLAT_LAND","LANDED","LANDED_FLAT","FLATTENED",
        "GUMBALL","GUMBALL_LAND","GUMBALL_SPLAT","BLB_SPLAT","CLAY_SPLAT",
    ]),
    0xBB781481: ("wake/activate decor sprite", [
        "WAKE","WAKEUP","WAKE_UP","AWAKEN","WAKING","SUMMON","SUMMONED","RISE",
        "ACTIVATE","ACTIVE","ACTIVATED","ON","TRIGGERED",
        "DECOR_WAKE","DECOR_ACTIVATE","DECOR_ON","DECOR_LIVE",
        "WAKEUP_DECOR","WAKE_DECOR","WAKING_DECOR",
        "GADGET","GADGET_ON","GADGET_ACTIVE","GADGET_TRIGGER","CONTRAPTION",
        "MACHINE","MACHINE_ON","MACHINE_ACTIVE","MACHINE_LIVE",
        "DEVICE","DEVICE_ON","CONTRAPTION_ON",
        "INIT","BOOT","BEGIN","START","SPAWN","BIRTH","BORN",
        "ALERT","ALERTED","SCARED","HOSTILE","ENRAGED","ANGRY",
        "AWAKE","STIR","STIRRED","NOTICED","ALERT_DECOR",
    ]),
}

found = []
for tgt, (label, names) in TARGETS.items():
    print(f"\n=== 0x{tgt:08x} ({label}) ===")
    hits = 0
    for n in names:
        h = calcHash(n)
        if h == tgt:
            print(f"  {n:35s} -> 0x{h:08x} <-- TARGET")
            found.append((tgt, n, label))
            hits += 1
    if hits == 0:
        print(f"  No hit in {len(names)} candidates")

print(f"\n\nTotal found: {len(found)}")
for t, n, lab in found:
    print(f"  0x{t:08x} = {n}  ({lab})")
