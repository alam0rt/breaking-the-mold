"""Ultra-broad attack: iterate over many (prefix, e, a, suf) combos.
Output ids with 1+ candidate, grouped by family for review.
"""
import sys, os, csv
from collections import defaultdict
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

uncracked = {}
cracked = {}
with open("docs/analysis/asset-identification/cracked_names.csv") as f:
    r = csv.DictReader(f)
    for row in r:
        try: hid = int(row["id_hex"], 16)
        except: continue
        if row["status"] == "uncracked":
            uncracked[hid] = (row["type"], row["levels"], int(row["floor"]))
        else:
            cracked[hid] = row.get("name","")

# Even more entities/actions
ENTITIES = [
    # Per-level boss/enemies (matching levels)
    "SKULL","SKULLBOT",
    "KLAY","KLAYMEN","KMEN","KMAN","PLAYER","HERO","FINN",
    "BIRD","RAT","BAT","FROG","FISH","BUG","WORM","SLUG","CAT","DOG","MOUSE","PIG","COW","SNAKE",
    "GHOST","SPIRIT","DEMON","DEVIL","ANGEL",
    "BOSS","FOE","ENEMY","GOON","ALIEN",
    "JOE","JOEHEAD","HEAD","HEADJOE",
    "GLENN","GLEN","GLENNYNTIS","WILLIE","TROMBONE","JOEY",
    "KLOGG","KLOG","KLOGGBOSS",
    "WIZ","WIZARD","WIZZ","MAGE","MAGEBOSS","SORCERER","MONK","MONKEY","MONKEYMAGE",
    "BIG","BIGHEAD","BIG_HEAD","BIGGUY","BIGBOSS","BIGWHITE","BIG_WHITE",
    "SHRINE","SHRINEY","SHRINEYGUARD","GUARD",
    "PHART","PHARTBALL","PHARTHEAD",
    "MA","MABIRD","MA_BIRD",
    "BLB","CLAYBALL","CLAY_BALL","BALL",
    "PUFF","POOF","BLOW","WIND","GALE","GUST","STORM","WHOOSH","BREEZE",
    "FAN","COG","GEAR","PIPE","BOMB","MINE","DART",
    "GAS","SMOKE","FIRE","FLAME","STEAM","DUST","ICE","WATER","MUD","LAVA",
    "SLIME","OIL","BLOOD","FOAM","SPRAY","DRIP","BUBBLE","DROP",
    "BRICK","ROCK","STONE","GEM","COIN","KEY","DOOR","GATE","WALL",
    "SPARK","FLASH","GLOW","STAR","HEART","RING","CROSS",
    "MUSIC","SONG","NOTE","TONE","DRUM","JINGLE","FANFARE","TADA","CHIME",
    "MENU","TITLE","INTRO","OUTRO","CREDITS","ENDING","CUTSCENE","SAVE","LOAD",
    "LEVEL","STAGE","ROOM","ZONE","AREA",
    "ITEM","POWER","POWERUP","PWR","BONUS","ICON","ARMOR",
    "ROCKET","MISSILE","BULLET","ARROW","SPIKE","SAW","BLADE",
    "TRAP","SPRING",
    "EGG","EGGY","EGGSHELL","EGGCY","CRACKER","HATCH","SHELL",
    "BABY","BABYHEAD",
    "GUM","GUMBO","GUMBALL","GUMSHOE",
    "JEAN","FOREST","SAUSAGE","ICEFISH","SCIENTIST","DOCTOR",
    "SHIP","BOAT","CAR","ENGINE","TRAIN","PLANE",
    "MOVE","STOMP","PUSH","DRAG","SQUASH",
    "HAMSTER","DOG","CAT","MICE",
    "GIB","GIBLET","CHUNK","BIT","BIG_BIT",
    "BUDDY","FRIEND","PAL","COMPANION",
    "EVIL","DARK","LIGHT",
    "SAW","BLADE","NEEDLE","HOOK",
    "WAVE","TIDE",
    "VOLCANO","ERUPT",
    "CRYSTAL","DIAMOND","RUBY","JADE",
    "SNOW","FROST","ICE",
    "POTION","ELIXIR","SCROLL","BOOK","BOTTLE",
    "TENTACLE","LIMB","ARM","LEG","CLAW","TAIL",
    "AWAKEN","SUMMON","RAISE","FALL_DOWN",
    "FOREST","TREE","BUSH","FLOWER","VINE",
    "CLOUD","SKY","SUN","MOON",
    "BUTTON","SWITCH","LEVER","DIAL",
    "VOICE","SHOUT","ECHO","RUMBLE",
    "BREATH","HUFF","PUFF",
    "DUNGEON","TEMPLE","TOWER","HOUSE",
]

# Single-token actions (already had many; add more)
ACTIONS = [
    "JUMP","LAND","FALL","WALK","RUN","STOP","STEP","FOOTSTEP","CLIMB","CRAWL",
    "DUCK","CROUCH","STAND","SLIDE","ROLL","FLIP","SPIN","SKID","DASH","SPRINT",
    "TUMBLE","HOP","SKIP","STOMP","SHUFFLE","TIPTOE","SCAMPER","TROT","JOG","MARCH",
    "LEAP","DIVE","BOUNCE","SPRING","HOVER","SOAR","GLIDE","FLY","FLAP","FLOAT",
    "DIE","DEATH","HURT","OUCH","PAIN","HIT","DAMAGE","SQUASH","SQUISH",
    "KILL","KICK","PUNCH","SLAP","PUSH","PULL","ATTACK","DEFEND","CHARGE",
    "FIRE","SHOOT","LAUNCH","THROW","TOSS","CAST","CHANT","HEX","CURSE",
    "BITE","CHEW","GULP","SUCK","SPIT","EAT","DRINK",
    "BLEED","BURN","FREEZE","MELT","STUN","FRY","SHOCK","DROWN",
    "BREAK","SHATTER","CRACK","SMASH","CRUSH","SPLINTER",
    "EXPLODE","BLAST","BANG","BOOM","ERUPT","BURST","POP",
    "PICKUP","GRAB","TAKE","DROP","HOLD","RELEASE","CARRY","STASH","GET","GIVE","PLACE",
    "OPEN","CLOSE","SHUT","SLAM","LOCK","UNLOCK","KNOCK","ENTER",
    "RING","BELL","CHIME","DING","BEEP","BLIP","BUZZ","TICK","TOCK","CLICK","CLINK","SWITCH","FLICK",
    "TALK","SPEAK","MUMBLE","WHISPER","SHOUT","YELL","SCREAM","CRY","CHATTER",
    "LAUGH","GIGGLE","CHUCKLE","SNORT","CACKLE",
    "BREATH","BREATHE","PANT","GASP","SIGH","HUFF","PUFF","HMPH",
    "FART","BURP","BELCH","COUGH","SNEEZE","SNORE","SOB","WAIL","MOAN","GROAN","GRUNT",
    "SWIM","DIVE","PADDLE","SURF","SPLASH","BUBBLE","DROWN","SINK",
    "GROW","SHRINK","SWELL","EXPAND","CONTRACT",
    "APPEAR","DISAPPEAR","VANISH","TELEPORT","WARP","FADE","RESPAWN","SPAWN",
    "ZOOM","WHIRL","WHOOSH","WHISH","WHIRR","SWISH","SWOOSH","RUSH",
    "ROLL","ROTATE","TURN","TWIST","WIND","WRAP","COIL",
    "HUM","DRONE","SQUEAL","SQUEAK","RUMBLE",
    "IDLE","WAIT","REST","SLEEP","WAKE","AWAKEN",
    "SELECT","CONFIRM","CANCEL","BACK","ENTER","EXIT","ESCAPE",
    "PAUSE","RESUME","SAVE","LOAD","START","END","BEGIN","FINISH","DONE",
    "WIN","LOSE","DRAW","CHEER","CLAP","BOO",
    "PIERCE","STAB","SLASH","CUT","RIP","TEAR","SHRED",
    "ZAP","ZING","ZAPPED",
    "PIP","POP","BUMP","KNOCK","TAP","RATTLE",
    "SHINE","TWINKLE","GLOW","SHIMMER","GLEAM","SPARK","FLASH",
    "WIND","BLOW","BREEZE","GUST","GALE","STORM","HOWL",
    "BUBBLE","DRIP","DROPLET","SPLASH","SPLAT","PLOP","BLOOP","GLUG",
    "CHANT","BLESS","CURSE","HEAL","REVIVE","SUMMON",
    "FALL_OUT","FALL_DOWN","DROP_DOWN","KO","KOd","FAINT","WOOZY",
    "FATFART","SUPERFART","MEGAFART",  # silly variants
]

SUF = [
    "","_01","_02","_03","_04","_05","_06","_07","_08","_09","_10","_11","_12",
    "_1","_2","_3","_4","_5","_6","_7","_8","_9","_0",
    "_A","_B","_C","_D","_E","_F","_G","_H",
    "_UP","_DOWN","_LEFT","_RIGHT","_DN","_LF","_RT","_FW","_BK","_FR","_BK",
    "_SM","_MD","_LG","_BIG","_HUGE","_TINY","_S","_M","_L","_XL","_XS",
    "_END","_START","_BEGIN","_LOOP","_FADE","_STOP","_DONE","_FINAL","_FIRST",
    "_LO","_HI","_LOW","_HIGH","_MID","_NORM","_NORMAL","_MED",
    "_QUIET","_LOUD","_SOFT","_HARD","_FAST","_SLOW","_DEEP","_FLAT",
    "_SHORT","_LONG","_HIT","_MISS","_HEAD","_TURN","_WALK","_RUN",
]

PREFIX = ["FX_","SFX_"]

hits_by_id = defaultdict(list)
total = 0
for pre in PREFIX:
    for ent in ENTITIES:
        for act in ACTIONS:
            for suf in SUF:
                name = pre + ent + "_" + act + suf
                total += 1
                h = calcHash(name)
                if h in uncracked:
                    hits_by_id[h].append(name)

print(f"Tried {total} -> {len(hits_by_id)} ids hit")

def lvls(s): return len(s.split(';')) if s else 99

# Audio-only, single-level, low-popc first
audio_single = []
audio_multi = []
sprite_hits = []
for hid, names in hits_by_id.items():
    t, lv, fl = uncracked[hid]
    if t == "audio":
        if lvls(lv) <= 1:
            audio_single.append((hid, sorted(set(names)), lv, fl))
        else:
            audio_multi.append((hid, sorted(set(names)), lv, fl))
    else:
        sprite_hits.append((hid, sorted(set(names)), lv, fl))

print(f"\n=== Single-level audio hits ({len(audio_single)}) ===")
audio_single.sort(key=lambda x: (x[3], x[0]))
for hid, names, lv, fl in audio_single[:30]:
    print(f"\n0x{hid:08x} popc={fl} level=[{lv}]")
    for n in names[:5]:
        print(f"    {n!r}")

print(f"\n=== Multi-level audio hits ({len(audio_multi)}) ===")
audio_multi.sort(key=lambda x: (lvls(x[2]), x[3], x[0]))
for hid, names, lv, fl in audio_multi[:30]:
    print(f"\n0x{hid:08x} popc={fl} levels=[{lv}]")
    for n in names[:5]:
        print(f"    {n!r}")
