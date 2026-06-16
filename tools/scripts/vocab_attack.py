"""Comprehensive vocabulary attack against ALL uncracked ids using correct calcHash."""
import sys, os, csv, itertools
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

# Load all uncracked ids
uncracked = {}
with open("docs/analysis/asset-identification/cracked_names.csv") as f:
    r = csv.DictReader(f)
    for row in r:
        if row["status"] == "uncracked":
            try:
                hid = int(row["id_hex"], 16)
                uncracked[hid] = (row["type"], row["levels"], int(row["floor"]))
            except Exception:
                pass

print(f"Loaded {len(uncracked)} uncracked ids")

# ============================================================
# Vocabulary banks
# ============================================================

PREFIXES_FX = ["FX_", "SFX_", "SND_", "AUDIO_", "WAV_"]
PREFIXES_SPR = ["SPR_", "SPRITE_", "IMG_", "GFX_"]
PREFIXES_BG  = ["BG_", "BACK_", "BKG_"]
PREFIXES_MAP = ["MAP_", "LEVEL_", "LVL_"]
PREFIXES_ALL = PREFIXES_FX + PREFIXES_SPR + PREFIXES_BG + PREFIXES_MAP + [""]

# Common game asset roots (English vocab biased toward platformers + the Neverhood)
ROOTS_CORE = [
    # Character / Klaymen
    "KLAY","KLAYMAN","KLAYMEN","KMAN","KMEN","KLAYMN","PLAYER","HERO",
    "FINN","FINNAN","FINNAGEN","FINNEGAN",
    # Common audio events
    "FOOTSTEP","STEP","WALK","RUN","JUMP","LAND","FALL","DIE","DEATH",
    "HURT","HIT","PAIN","OUCH","CRY","YELL","SHOUT","SCREAM","GRUNT",
    "BREATH","HUFF","PANT","SIGH","LAUGH","GIGGLE","CHUCKLE",
    "PICKUP","GRAB","TAKE","DROP","TOSS","THROW",
    "OPEN","CLOSE","SHUT","SLAM","BREAK","SHATTER","CRACK","SMASH",
    "EXPLODE","BLOW","BLAST","BANG","BOOM","POW","KAPOW",
    "BEEP","BOOP","BLIP","DING","BELL","CHIME","TICK","TOCK","TINK",
    "SPLAT","SPLASH","DRIP","BLUB",
    "POP","ZAP","ZING","ZOOM","WHOOSH","WHISH","WHOOP","WHIRL","SWISH",
    "BOUNCE","SPRING","SLIDE","ROLL","SPIN","TWIRL",
    "GROW","SHRINK","STRETCH","SQUASH",
    "GIB","DEFEAT","WIN","LOSE","CHEER","CLAP",
    # Items / objects
    "EGG","SKULL","HEAD","HAND","FOOT","ARM","LEG","BODY",
    "BALL","BLOCK","BRICK","STONE","ROCK","GEM","COIN","COG","KEY","BOMB",
    "BABY","CHILD","KID","DOLL","TOY",
    "FOOD","CAKE","PIE","BREAD","MEAT","CANDY","SUGAR",
    "BOX","CRATE","BARREL","CHEST","CAGE",
    "DOOR","GATE","WINDOW","WALL","FLOOR","CEIL","PIPE",
    "BOMB","CANNON","MISSILE","ROCKET","BULLET","ARROW","DART","SPIKE",
    "GAS","SMOKE","FIRE","FLAME","ICE","WATER","MUD","SLIME","OIL","LAVA",
    "STAR","HEART","CROSS","ARROW","CIRCLE","SQUARE","RING",
    # Enemies
    "ENEMY","BOSS","FOE","MONSTER","GHOST","SPIRIT","CREATURE","BUG","WORM",
    "BIRD","FISH","BAT","CAT","DOG","RAT","MOUSE","PIG","COW","SNAKE",
    "FROG","TOAD","GIB",
    "WIZARD","WIZ","MAGE","MONK","KNIGHT","SOLDIER","GUARD","DRAGON",
    "KLOGG","KLOG","BUDDY","JOE",
    # Effects
    "PUFF","SPARK","FLASH","GLOW","SHINE","SHIMMER","TWINKLE","GLEAM",
    "RIPPLE","WAVE","RING","ECHO",
    "DUST","SAND","CRUMB","DEBRIS",
    "MAGIC","SPELL","CAST","CHANT","HEX",
    "WIND","GUST","BREEZE","GALE","STORM",
    "MUSIC","SONG","TUNE","NOTE","TONE","BEAT","DRUM",
    "MENU","TITLE","INTRO","OUTRO","CREDITS","ENDING","CUTSCENE",
    "SELECT","CONFIRM","CANCEL","ENTER","EXIT","BACK","NEXT","PREV",
    "PAUSE","RESUME","SAVE","LOAD","CONTINUE","RESTART","QUIT","NEW","HIGH","SCORE",
    "LIFE","LIVES","HEALTH","TIME","CLOCK","TIMER","COUNT","DOWN","UP",
    "WIN","LOSE","FINISH","GOAL","END","START","BEGIN",
    "INTRO","ENDING","FANFARE","TADA","JINGLE",
]

ROOTS_DIR = ["UP","DOWN","LEFT","RIGHT","FRONT","BACK","SIDE","TOP","BOT","MID","CENTER"]
ROOTS_SIZE = ["BIG","SMALL","SM","MD","LG","S","M","L","XL","XS","TINY","HUGE",
              "LARGE","SMALL","MED","MEDIUM"]

SUFFIXES_SIZE = ["","_SM","_MED","_MD","_M","_LG","_L","_LRG","_LRGE","_BIG","_HUGE",
                 "_S","_XS","_XL","_XXL","_TINY","_LARGE","_SMALL"]
SUFFIXES_NUM  = ["","_1","_2","_3","_4","_5","_6","_7","_8","_9","_0",
                 "_01","_02","_03","_001","_002"]
SUFFIXES_ALPHA = ["","_A","_B","_C","_D","_E","_F","_G","_H"]
SUFFIXES_LO_HI = ["","_LO","_HI","_LOW","_HIGH","_MID"]
SUFFIXES_ACT  = ["","_LOOP","_FADE","_END","_STOP","_START","_BEGIN","_FINISH",
                 "_DONE","_INIT","_RESET","_TICK","_TICK1","_TICK2",
                 "_IN","_OUT","_ON","_OFF",
                 "_HIT","_MISS","_PASS","_DIE","_DEATH","_SPAWN","_KILL",
                 "_LEFT","_RIGHT","_UP","_DOWN",
                 "_OPEN","_CLOSE","_SHUT",
                 "_CALL","_RING","_BELL","_HORN"]
SUFFIXES_TONE = ["","_QUIET","_LOUD","_SOFT","_HARD","_GENTLE","_HARSH","_DEEP","_THIN"]
SUFFIXES_NEAR = ["","_NEAR","_FAR","_CLOSE","_DISTANT"]
SUFFIXES_TIME = ["","_SHORT","_LONG","_QUICK","_SLOW"]

ALL_SUFFIXES = list(set(
    SUFFIXES_SIZE + SUFFIXES_NUM + SUFFIXES_ALPHA + SUFFIXES_LO_HI +
    SUFFIXES_ACT + SUFFIXES_TONE + SUFFIXES_NEAR + SUFFIXES_TIME
))

# Generate candidates
def gen_names():
    seen = set()
    # 1-token: prefix + root + suffix
    for pre in PREFIXES_ALL:
        for root in ROOTS_CORE + ROOTS_DIR + ROOTS_SIZE:
            for suf in ALL_SUFFIXES:
                name = pre + root + suf
                if name not in seen:
                    seen.add(name)
                    yield name
    # 2-token: prefix + root1_root2 + suffix (limited)
    for pre in PREFIXES_FX + ["FX_", ""]:
        for r1 in ROOTS_CORE[:60]:
            for r2 in ROOTS_CORE[:60] + ROOTS_SIZE + ROOTS_DIR:
                if r1 == r2:
                    continue
                for suf in ["", "_SM","_MD","_LG","_1","_2","_3","_A","_B","_C"]:
                    name = pre + r1 + "_" + r2 + suf
                    if name not in seen:
                        seen.add(name)
                        yield name

hits = []
total = 0
for name in gen_names():
    total += 1
    h = calcHash(name)
    if h in uncracked:
        hits.append((h, name))
        if len(hits) < 200:
            t, lv, fl = uncracked[h]
            print(f"  HIT 0x{h:08x}  {name!r}  ({t}, {lv}, floor={fl})")
print()
print(f"Tried {total} names -> {len(hits)} hits across {len(set(h for h,_ in hits))} distinct ids")
