"""Attack uncracked audio ids using the verified FX_<ENTITY>_<ACTION>_<NN> convention.

Strategy: enumerate all (entity, action, suffix) combinations and check against
uncracked audio ids. Group by id to identify high-confidence matches that
have multiple sister candidates (e.g., _01, _02 both hit related ids).
"""
import sys, os, csv
from collections import defaultdict
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

# Load uncracked
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

# === Entity vocabulary (game-specific) ===
# Per-level entities + universal
ENTITIES = [
    # Player
    "KLAY", "KLAYMEN", "KLAYMAN", "KMAN", "KMEN", "PLAYER", "HERO",
    "FINN",  # FINN level player
    # Bosses (per level)
    "JOE", "JOEHEAD", "JHJ", "JOEJOE",
    "GLENN", "GLENNYNTIS", "GLEN",
    "KLOGG", "KLOG",
    "WIZARD", "WIZ", "WIZZ",
    "BIG", "BIGHEAD",  # MEGA boss = giant head
    "MONK", "MONKEY", "MONKEYMAGE",
    "SHRINE", "SHRINEY", "SHRINEY_GUARD",
    "MONKEY_MAGE", "MAGEBOSS", "MAGE",
    # Common enemies
    "BIRD", "BIRDIE",
    "BUDDY", "BUDDIES",
    "GHOST", "FOOTPRINT",
    "BUG", "WORM", "SLUG",
    "ROBO", "ROBOT",
    "GOON", "MOOK",
    "RAT", "MOUSE", "BAT",
    "FROG", "TOAD", "FISH",
    "SKULL", "BONE", "SKELE",
    "HEAD", "HEADJOE",
    # Misc enemies/items
    "BABY", "DOLL", "TOY",
    "EGG", "EGGCY", "EGGSHELL",
    "GUM", "GUMBO",
    "BOMB", "MINE", "DART",
    "FAN", "GEAR", "COG", "PIPE",
    "GAS", "SMOKE", "FIRE", "STEAM", "DUST",
    "ICE", "WATER", "MUD", "LAVA", "SLIME", "OIL", "BLOOD",
    "TRAP", "SPIKE",
    "BIG_WHITE", "BIGWHITE",
    "BOSS",
    # Effects
    "PUFF", "WIND", "GUST", "GALE", "BLOW",
    "POOF",
    "BUBBLE", "DROP", "DROPLET",
    "SPARK", "FLAME",
    # Audio
    "MUSIC", "SONG",
    "MENU",
    "INTRO", "OUTRO", "TITLE",
]

# === Action vocabulary ===
ACTIONS = [
    # Player/character actions
    "JUMP", "LAND", "FALL", "WALK", "RUN", "STOP", "STEP", "FOOTSTEP",
    "DIE", "DEATH", "HURT", "OUCH", "PAIN", "HIT", "DAMAGE", "SQUASH",
    "DUCK", "DUCK_DOWN", "DUCK_UP", "CROUCH", "STAND", "SLIDE",
    "CRAWL", "ROLL", "FLIP", "SPIN",
    "GRAB", "TAKE", "PICKUP", "DROP", "THROW", "TOSS",
    "PUNCH", "KICK", "SLAP", "PUSH", "PULL",
    "EAT", "BITE", "CHEW", "GULP", "SWALLOW", "SUCK", "SPIT",
    "BREATH", "BREATHE", "PANT", "GASP", "SIGH",
    "LAUGH", "GIGGLE", "CRY", "SCREAM", "YELL", "SHOUT", "MOAN",
    "TALK", "SPEAK", "MUMBLE", "WHISPER",
    "FART", "BURP", "BELCH", "COUGH", "SNEEZE",
    "BOUNCE", "SPRING",
    "SWIM", "DIVE", "PADDLE", "SURF",
    "FLY", "GLIDE", "SOAR", "FLAP",
    # Object/item actions
    "BREAK", "SHATTER", "CRACK", "SMASH", "CRUSH",
    "EXPLODE", "BLAST", "BANG", "BOOM",
    "POP", "ZAP", "ZOOM", "WHIRL", "WHOOSH", "WHISH", "WHIRR",
    "OPEN", "CLOSE", "SHUT", "SLAM",
    "DROP", "FIRE", "SHOOT", "FIRE_AT", "LAUNCH",
    "GROW", "SHRINK", "SWELL",
    "ROLL", "ROTATE", "TURN",
    "RESPAWN", "SPAWN", "APPEAR", "DISAPPEAR", "VANISH",
    # Audio
    "RING", "BELL", "CHIME", "DING", "BEEP", "BLIP", "BUZZ", "TICK",
    "JINGLE", "FANFARE", "TADA",
    "HUM", "DRONE",
    # Menu/UI
    "SELECT", "CONFIRM", "CANCEL", "BACK", "ENTER", "EXIT", "ESCAPE",
    "PAUSE", "RESUME", "SAVE", "LOAD",
    "WIN", "LOSE", "DRAW",
    # Effect-like
    "PIERCE", "STAB", "SLASH", "CUT", "RIP",
    "MELT", "FREEZE", "BURN", "ICE",
    "SHINE", "GLOW", "SPARK", "FLASH", "TWINKLE",
    "BUBBLE", "DRIP", "SPLASH", "SPLAT",
    "IDLE", "WAIT", "REST",
    "ATTACK", "DEFEND",
    "CHARGE",
    "DASH",
]

# === Suffixes (ordinal/modifier) ===
SUFFIXES = [
    "",
    "_01", "_02", "_03", "_04", "_05", "_06", "_07", "_08", "_09",
    "_10", "_11", "_12",
    "_1", "_2", "_3", "_4", "_5", "_6", "_7", "_8", "_9",
    "_A", "_B", "_C", "_D",
    "_UP", "_DOWN", "_LEFT", "_RIGHT", "_FRONT", "_BACK",
    "_DN", "_LF", "_RT", "_FW", "_BW",
    "_SM", "_MD", "_LG", "_BIG", "_SML", "_HUGE",
    "_S", "_M", "_L", "_XS", "_XL",
    "_END", "_START", "_BEGIN", "_LOOP", "_FADE", "_STOP",
    "_LO", "_HI", "_LOW", "_HIGH",
    "_QUIET", "_LOUD",
    "_SHORT", "_LONG",
    "_HIT", "_MISS",
]

# === Generate and match ===
print(f"Generating combinations...")
total = 0
hits_by_id = defaultdict(list)
prefixes = ["FX_", "FX", "SFX_"]
for pre in prefixes:
    for ent in ENTITIES:
        for act in ACTIONS:
            for suf in SUFFIXES:
                name = pre + ent + "_" + act + suf
                total += 1
                h = calcHash(name)
                if h in uncracked:
                    hits_by_id[h].append(name)

# Also try FX_<ACTION>_<NN> alone (no entity)
for pre in prefixes:
    for act in ACTIONS:
        for suf in SUFFIXES:
            name = pre + act + suf
            total += 1
            h = calcHash(name)
            if h in uncracked:
                hits_by_id[h].append(name)

# Output: ids with multiple hits are highest confidence (multiple candidates for same id)
print(f"Tried {total} names -> {len(hits_by_id)} ids hit")
print()
# Sort by id, prefer audio types and single-level
audio_hits = []
for hid, names in hits_by_id.items():
    t, lv, fl = uncracked[hid]
    if t == "audio":
        audio_hits.append((hid, names, t, lv, fl))

audio_hits.sort(key=lambda x: (len(x[3].split(';')), x[4], x[0]))
print("=== AUDIO hits (sorted by level narrowness, then floor) ===")
for hid, names, t, lv, fl in audio_hits[:50]:
    nlev = len(lv.split(';')) if lv else 99
    print(f"0x{hid:08x} popc={fl} levels=[{lv}] type={t}  ({len(names)} candidates)")
    for n in names[:6]:
        print(f"    {n!r}")

print()
print("=== SPRITE hits (sorted by level narrowness) ===")
sprite_hits = []
for hid, names in hits_by_id.items():
    t, lv, fl = uncracked[hid]
    if t == "sprite":
        sprite_hits.append((hid, names, t, lv, fl))
sprite_hits.sort(key=lambda x: (len(x[3].split(';')), x[4], x[0]))
for hid, names, t, lv, fl in sprite_hits[:30]:
    nlev = len(lv.split(';')) if lv else 99
    print(f"0x{hid:08x} popc={fl} levels=[{lv}] type={t}  ({len(names)} candidates)")
    for n in names[:4]:
        print(f"    {n!r}")
