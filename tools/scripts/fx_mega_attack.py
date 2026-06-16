"""Mega FX convention attack with extended vocabulary.

Strategy: enumerate exhaustive (entity, action, suffix) combos.
For each id with 2+ sister hits at sequential ordinals, score HIGH.
"""
import sys, os, csv, itertools
from collections import defaultdict
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

# Load uncracked & cracked
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
            cracked[hid] = (row.get("name",""), row.get("type",""), row.get("levels",""))

# Entities (extended)
ENT = [
    "SKULL","SKULLBOT","SKULLMONK","SKULLMONKEY","SKM","SK",
    "KLAY","KLAYMEN","KLAYMAN","KMAN","KMEN","HERO","PLAYER","PROTAG",
    "FINN","FN",
    "BIRD","RAT","BAT","CAT","FROG","BUG","WORM","SLUG","FISH","PIG","COW",
    "GUM","GUMBO","GUMBALL","GUMBOSS",
    "RAT","MOUSE",
    "EGG","EGGSHELL","EGGY","EGGCY","EGGER","CRACKER",
    "BABY","BABYHEAD",
    "BOSS","FOE","ENEMY","GOON","MOOK","DUDE","GUY","ALIEN",
    "JOE","JOEHEAD","JHJ","HEADJOE",
    "GLENN","GLEN","GLENNYNTIS","GLENNHOWL","WILLIE","TROMBONE",
    "KLOGG","KLOG","KLOGGBOSS","KLOG_BOSS",
    "WIZ","WIZARD","WIZZ","MAGE","MAGEBOSS","MAGUS","SORCERER",
    "MONK","MONKEY","MONKEYMAGE",
    "BIG","BIGHEAD","BIG_HEAD","BIGGUY","BIGBOSS",
    "SHRINE","SHRINEY","SHRINEYGUARD","SHRINE_GUARD","GUARD",
    "PHART","PHARTHEAD","PHARTBALL",
    "MA","MABIRD","MA_BIRD","MOMMABIRD",
    "BLB","CLAYBALL","CLAY_BALL","BALL",
    "PUFF","POOF","BLOW","WIND","GALE","GUST","STORM","WHOOSH",
    "FAN","COG","GEAR","PIPE","BOMB","MINE","DART",
    "GAS","SMOKE","FIRE","FLAME","STEAM","DUST","ICE","WATER","MUD","LAVA",
    "SLIME","OIL","BLOOD","FOAM","SPRAY","DRIP","BUBBLE","DROP",
    "BRICK","ROCK","STONE","GEM","COIN","KEY","DOOR","GATE","WALL",
    "SPARK","FLASH","GLOW","STAR","HEART","RING","CROSS",
    "MUSIC","SONG","NOTE","TONE","DRUM","JINGLE","FANFARE","TADA",
    "MENU","TITLE","INTRO","OUTRO","CREDITS","ENDING","CUTSCENE",
    "LEVEL","STAGE","ROOM","ZONE",
    "GHOST","SPIRIT","DEMON","DEVIL","ANGEL",
    "ITEM","POWER","POWERUP","PWR","UP",
    "BOMB","ROCKET","MISSILE","BULLET","ARROW","DART","SPIKE","SAW","BLADE",
    "TRAP","SPRING","SPIKE",
    "JEAN","BIRD","FOREST","SAUSAGE","ICEFISH",
    "SCIENTIST","DOCTOR",
    "SHIP","BOAT","CAR","ENGINE",
    "MOVE","STOMP","PUSH","DRAG",
    "GIB","GIBLET","CHUNK","BIT",
    # actions-as-entities (FX_<ACTION>_<NN>)
]

# Actions (extended)
ACT = [
    # movement
    "JUMP","LAND","FALL","WALK","RUN","STOP","STEP","FOOTSTEP","DASH","DUCK","CROUCH","STAND","SLIDE",
    "CRAWL","ROLL","FLIP","SPIN","TUMBLE","SOMERSAULT",
    # combat
    "DIE","DEATH","HURT","OUCH","PAIN","HIT","DAMAGE","SQUASH","SQUISH",
    "KILL","KICK","PUNCH","SLAP","PUSH","PULL","ATTACK","DEFEND","CHARGE",
    "FIRE","SHOOT","LAUNCH","THROW","TOSS","CAST","CHANT","HEX","CURSE",
    "BITE","CHEW","GULP","SUCK","SPIT","EAT","DRINK",
    "BLEED","BURN","FREEZE","MELT","ELECTROCUTE","STUN",
    "BREAK","SHATTER","CRACK","SMASH","CRUSH","SPLINTER",
    "EXPLODE","BLAST","BANG","BOOM","ERUPT","BURST",
    # interaction
    "PICKUP","GRAB","TAKE","DROP","HOLD","RELEASE","CARRY","STASH",
    "OPEN","CLOSE","SHUT","SLAM","LOCK","UNLOCK","KNOCK","ENTER",
    "RING","BELL","CHIME","DING","BEEP","BLIP","BUZZ","TICK","TOCK","CLICK","CLINK","CLINK","SWITCH","FLICK","FLIP",
    # voice
    "TALK","SPEAK","MUMBLE","WHISPER","SHOUT","YELL","SCREAM","CRY",
    "LAUGH","GIGGLE","CHUCKLE","SNORT","SNICKER","CACKLE",
    "BREATH","BREATHE","PANT","GASP","SIGH","HUFF","PUFF",
    "FART","BURP","BELCH","COUGH","SNEEZE","SNORE","SOB","WAIL","MOAN","GROAN","GRUNT","HMPH",
    # water/swim
    "SWIM","DIVE","PADDLE","SURF","SPLASH","BUBBLE","DROWN","SINK","FLOAT",
    # fly
    "FLY","GLIDE","SOAR","FLAP","HOVER","DIVE","DROP",
    # surface
    "BOUNCE","SPRING","HOP","SKIP","SLIDE","SLIP","TRIP","STUMBLE",
    # transform
    "GROW","SHRINK","SWELL","EXPAND","CONTRACT",
    "APPEAR","DISAPPEAR","VANISH","TELEPORT","WARP","FADE","RESPAWN","SPAWN",
    # speed
    "ZOOM","WHIRL","WHOOSH","WHISH","WHIRR","SWISH","SWOOSH","RUSH",
    # mech
    "ROLL","ROTATE","TURN","TWIST","WIND","WRAP","COIL",
    # voice/music
    "HUM","DRONE","MOAN","SQUEAL","SQUEAK",
    "IDLE","WAIT","REST","SLEEP","WAKE","AWAKEN",
    # ui
    "SELECT","CONFIRM","CANCEL","BACK","ENTER","EXIT","ESCAPE",
    "PAUSE","RESUME","SAVE","LOAD","START","END","BEGIN","FINISH","DONE",
    "WIN","LOSE","DRAW","CHEER","CLAP","BOO",
    # pierce
    "PIERCE","STAB","SLASH","CUT","RIP","TEAR","SHRED",
    # 
    "ZAP","ZOOM","ZING","ZAPPED",
    "PIP","POP","POOP","BUMP","KNOCK","TAP",
    "SHINE","TWINKLE","GLOW","SHIMMER","GLEAM","SPARK","FLASH",
    "WIND","BLOW","BREEZE","GUST","GALE","STORM","HOWL",
    "BUBBLE","DRIP","DROPLET","SPLASH","SPLAT","PLOP","BLOOP","BLUB","GLUG",
    "SHRINE","CHANT","BLESS","CURSE",
]

# Suffixes
SUF = [
    "",
    "_01","_02","_03","_04","_05","_06","_07","_08","_09","_10","_11","_12",
    "_1","_2","_3","_4","_5","_6","_7","_8","_9",
    "_A","_B","_C","_D","_E",
    "_UP","_DOWN","_LEFT","_RIGHT","_DN","_LF","_RT",
    "_SM","_MD","_LG","_BIG","_HUGE","_S","_M","_L","_XS","_XL",
    "_END","_START","_BEGIN","_LOOP","_FADE","_STOP",
    "_LO","_HI","_LOW","_HIGH","_MID","_QUIET","_LOUD",
    "_SHORT","_LONG","_HIT","_MISS","_TURN",
]

PREFIX = ["FX_","SFX_"]

print("Generating combinations...")
hits_by_id = defaultdict(list)
total = 0
for pre in PREFIX:
    for e in ENT:
        for a in ACT:
            for s in SUF:
                name = pre + e + "_" + a + s
                total += 1
                h = calcHash(name)
                if h in uncracked:
                    hits_by_id[h].append(name)
                # Also try with double underscore stripped
                if "__" not in name:
                    name2 = pre + e + a + s  # no underscore
                    h2 = calcHash(name2)
                    if h2 in uncracked:
                        hits_by_id[h2].append(name2)

# Also FX_<ACTION>_<NN> (no entity)
for pre in PREFIX:
    for a in ACT:
        for s in SUF:
            name = pre + a + s
            total += 1
            h = calcHash(name)
            if h in uncracked:
                hits_by_id[h].append(name)

print(f"Tried {total} names -> {len(hits_by_id)} ids hit")

# Score: ids hit by name + sister at sequential ordinal
# For each id hit, check if a sibling ordinal of the same name also hits
def score(names_list):
    """Higher score if multiple sister names share a common prefix and differ by ordinal."""
    # Group by stem (strip suffix)
    stems = defaultdict(list)
    for n in names_list:
        # Strip trailing _NN or _N or _A-E or _UP/_DOWN/... to get stem
        parts = n.rsplit("_", 1)
        if len(parts) == 2:
            stems[parts[0]].append(parts[1])
        else:
            stems[n].append("")
    # Score = max group size
    return max(len(v) for v in stems.values()) if stems else 0

# Audio hits, sorted by level narrowness then floor
def lvls(s):
    return len(s.split(';')) if s else 99

audio_hits = []
for hid, names in hits_by_id.items():
    t, lv, fl = uncracked[hid]
    if t == "audio":
        audio_hits.append((hid, sorted(set(names)), t, lv, fl))

audio_hits.sort(key=lambda x: (lvls(x[3]), x[4], x[0]))
print(f"\n=== AUDIO hits ({len(audio_hits)} ids) ===")
for hid, names, t, lv, fl in audio_hits[:40]:
    print(f"\n0x{hid:08x} popc={fl} levels=[{lv}]  ({len(names)} candidates)")
    for n in names[:8]:
        print(f"    {n!r}")

# Sprite hits
sprite_hits = []
for hid, names in hits_by_id.items():
    t, lv, fl = uncracked[hid]
    if t == "sprite":
        sprite_hits.append((hid, sorted(set(names)), t, lv, fl))

sprite_hits.sort(key=lambda x: (lvls(x[3]), x[4], x[0]))
print(f"\n=== SPRITE hits ({len(sprite_hits)} ids) ===")
for hid, names, t, lv, fl in sprite_hits[:20]:
    print(f"\n0x{hid:08x} popc={fl} levels=[{lv}]  ({len(names)} candidates)")
    for n in names[:6]:
        print(f"    {n!r}")
