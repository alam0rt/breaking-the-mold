"""Bigger SKULL/BOSS/PLAYER attack: leverages strong existing family knowledge."""
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

# Per-entity action vocab (focused)
ENT_VOCAB = {
    "SKULL": [  # we know FOOTSTEP_LEFT/RIGHT, FLY_01/02, LAND_01/02, FIRE_01/02, BITE_01,
                # FLAP, EAT, SCREAM_01, UP, SPRING_01/02
        "FOOTSTEP","STEP","WALK","RUN","JUMP","LAND","FALL","BITE","CHEW","EAT","GULP","SPIT",
        "FLY","FLAP","HOVER","GLIDE","SOAR","DIVE","DROP","SWOOP",
        "FIRE","SHOOT","SHOT","ATTACK","BREATHE","BREATH","GROWL","HISS","SNARL",
        "DIE","DEATH","HURT","SCREAM","CRY","YELL","SHOUT","MOAN","GROAN","GRUNT","SOB","WAIL",
        "RIP","TEAR","ZAP","ZAPPED","STAB","SLASH","CUT","PIERCE","BURST","BOOM","BANG",
        "SPRING","BOUNCE","HOP","SKIP","STOMP",
        "TURN","SPIN","WHIRL","TWIRL","ROTATE","FLIP",
        "GLOW","FLASH","SPARK","SHINE","TWINKLE",
        "SQUEAK","SQUEAL","SQUASH","SQUISH","CRACK","SHATTER",
        "GULP","SUCK","CHOMP","SLURP",
        "PICKUP","GRAB","RELEASE","DROP",
        "ROLL","TUMBLE","CRAWL",
        "PRE","START","BEGIN","END","STOP","DONE",
        "UP","DOWN","LEFT","RIGHT",
    ],
    "KLAY": [
        "STEP","FOOTSTEP","WALK","RUN","JUMP","LAND","FALL","DUCK","CROUCH","STAND","SLIDE","ROLL",
        "DIE","DEATH","HURT","OUCH","PAIN","HIT","CRY","SCREAM","YELL","MOAN","GROAN","GRUNT",
        "EAT","BITE","CHEW","GULP","SUCK","SPIT","BREATH","HUFF","PUFF","HMPH",
        "FART","BURP","BELCH","COUGH","SNEEZE","SNORE","LAUGH","GIGGLE","SIGH","WHISTLE","HUM",
        "GRAB","PICKUP","DROP","TAKE","THROW","TOSS",
        "PUNCH","KICK","SLAP","PUSH","PULL",
        "SPIN","TURN","SWING","FLIP","TUMBLE","BOUNCE","SPRING","HOP","DASH","DODGE",
        "CLIMB","CRAWL","STOMP","SLIDE","SKID","BREATHE","TIPTOE","TROT","JOG","MARCH","SPRINT",
        "BURN","FREEZE","SHOCK","STUN","FRY","DROWN","ELECTROCUTE",
        "WIN","LOSE","CHEER","CLAP","CHEERS","BOO",
    ],
    "BOSS": ["HIT","IDLE","TURN","WALK","DIE","FIRE","ATTACK","SPAWN","INTRO","WARN","CHARGE",
             "LAUGH","ROAR","SCREAM","DAMAGE","HURT","RAGE","TAUNT","BREAK"],
    "WIZ": ["CAST","CHANT","SPELL","HEX","CURSE","SUMMON","TELEPORT","WARP","DISAPPEAR","APPEAR",
            "BLAST","BLOW","WIND","GUST","GALE","SHIELD","BARRIER","PROTECT","DIE","HURT","LAUGH"],
    "WIZARD": ["CAST","CHANT","SPELL","HEX","CURSE","SUMMON","TELEPORT","WARP","DISAPPEAR","APPEAR",
            "BLAST","BLOW","WIND","GUST","GALE","SHIELD","BARRIER","PROTECT","DIE","HURT","LAUGH",
            "RAGE","TAUNT","INTRO","OUTRO","DEATH"],
    "WIZZ": ["CAST","SPELL","BLAST","BLOW","GALE","WIND","SHIELD","DIE","HURT","LAUGH"],
    "BIRD": ["FLY","FLAP","CHIRP","CAW","TWEET","DIE","SQUISH","HATCH","LAND","TAKEOFF","PERCH","CALL"],
    "RAT": ["DASH","RUN","SQUEAK","DIE","BITE","SCURRY","SQUEAL"],
    "BAT": ["FLY","FLAP","SCREECH","DIE","HOVER"],
    "FROG": ["JUMP","CROAK","RIBBIT","DIE","HOP","SPLASH"],
    "FISH": ["SWIM","SPLASH","DIE","FLOP","JUMP"],
    "BUG": ["BUZZ","DIE","CRAWL","FLY"],
    "WORM": ["WRIGGLE","DIE","CRAWL","BURST"],
    "GUM": ["PIERCE","HIT","BURST","POP","STRETCH","STICK","BUBBLE","CHEW","BLOW","SHOOT","FIRE","DROP","POOP"],
    "BABY": ["CRY","LAUGH","COO","GIGGLE","WAIL","SCREAM","HEAD","GOO","GAGA","SUCK","BURP","SPIT","DIE","JUMP","WALK"],
    "EGG": ["CRACK","HATCH","BREAK","SHATTER","ROLL","FALL","DIE","SHELL","ZOOM"],
    "EGGSHELL": ["ZOOM","CRACK","BREAK","SHATTER","ROLL","FALL","FLY"],
    "JOE": ["HIT","ROLL","DIE","SCREAM","ATTACK","BALL","HEAD"],
    "JOEHEAD": ["HIT","DIE","BALL","SCREAM"],
    "KLOGG": ["DIE","HIT","SCREAM","ATTACK","SPAWN"],
    "KLOG": ["DIE","HIT","SCREAM","ATTACK","SPAWN"],
    "GLENN": ["DIE","HIT","HOWL","SCREAM","ATTACK","TROMBONE","BLOW"],
    "MA": ["CLAP","DIE","CHEER","HOLLER","CALL","BIRD","FLY","CHIRP"],
    "MABIRD": ["CHIRP","FLY","CALL","FLAP","LAND","TAKEOFF"],
    "WILLIE": ["TROMBONE","BLOW","HOWL","SCREAM","DIE","CHEER"],
    "HEAD": ["JUMP","HIT","ROLL","SCREAM","BIG","BABY","BOUNCE","DIE","SPIN","TURN","BOSS","JOE","MASH"],
    "BIG": ["WHITE","DIE","HIT","JUMP","SCREAM","ROAR","ATTACK","HEAD","BOSS","MAN","GUY"],
    "PHART": ["BLOW","SHOOT","FIRE","DIE","HIT","HEAD","BURST","ATTACK","BALL"],
    "MONK": ["LOCK","CHANT","WALK","CAST","CHANT","DIE","HIT","ATTACK","PRAY","MEDITATE","TEMPLE"],
    "MONKEY": ["LOCK","CAST","CHANT","DIE","HIT","ATTACK","CACKLE","LAUGH","SCREECH","MAGE"],
    "SHRINEY": ["IDLE","ATTACK","HIT","DIE","WAKE","REST","GUARD","WALK","CHARGE","RAGE"],
    "GUM_PIERCE": ["DN","UP","LF","RT","_LF","_RT","_UP","_DN"],
    "SMOKE": ["SLASH","BURST","RISE","CURL","FADE","PUFF","UP","DOWN"],
    "PUFF": ["FALL","UP","DOWN","BURST","BLOW","CURL"],
    "LAVA": ["SLIP","BURST","BUBBLE","FLOW","DRIP","SPLASH","HIT"],
    "JINGLE": ["LAND","FALL","END","START","LOOP"],
    "EGGY": ["BLAST","HIT","ROLL","DIE","BURST","BREAK"],
    "SPRAY": ["BOOM","BURST","HIT","SHOOT"],
    "MAGE": ["CAST","SPELL","CHANT","ATTACK","DIE","HIT","LAUGH"],
    "MONKEYMAGE": ["CAST","SPELL","CHANT","ATTACK","DIE","HIT","LAUGH"],
}

SUF = ["","_01","_02","_03","_04","_05","_06","_07","_08","_09","_10","_11","_12",
       "_1","_2","_3","_4","_5","_6","_7","_8","_9","_0",
       "_A","_B","_C","_D","_E","_F","_G",
       "_UP","_DOWN","_LEFT","_RIGHT","_DN","_LF","_RT","_FW","_BK","_FR","_BK",
       "_SM","_MD","_LG","_BIG","_HUGE","_TINY","_S","_M","_L","_XL","_XS",
       "_END","_START","_BEGIN","_LOOP","_FADE","_STOP","_DONE","_FINAL",
       "_LO","_HI","_LOW","_HIGH","_MID","_NORM","_NORMAL","_MED",
       "_QUIET","_LOUD","_SOFT","_HARD","_FAST","_SLOW","_DEEP","_FLAT",
       "_SHORT","_LONG","_HIT","_MISS","_HEAD","_TURN","_WALK","_RUN"]

PREFIX = ["FX_","SFX_"]

hits_by_id = defaultdict(list)
total = 0
for pre in PREFIX:
    for ent, actions in ENT_VOCAB.items():
        for act in actions:
            for suf in SUF:
                name = pre + ent + "_" + act + suf
                total += 1
                h = calcHash(name)
                if h in uncracked:
                    hits_by_id[h].append(name)

# bare action
for pre in PREFIX:
    for ent, actions in ENT_VOCAB.items():
        for act in actions:
            for suf in SUF:
                name = pre + ent + suf
                total += 1
                h = calcHash(name)
                if h in uncracked:
                    hits_by_id[h].append(name)

print(f"Tried {total} -> {len(hits_by_id)} ids hit")
def lvls(s): return len(s.split(';')) if s else 99

# Group by id
all_hits = sorted(hits_by_id.items(), key=lambda x: (lvls(uncracked[x[0]][1]), uncracked[x[0]][2], x[0]))

for hid, names in all_hits[:50]:
    t, lv, fl = uncracked[hid]
    print(f"\n0x{hid:08x} {t} popc={fl} levels=[{lv}]  ({len(set(names))} candidates)")
    for n in sorted(set(names))[:6]:
        print(f"    {n!r}")
