"""Exhaustive FX_KLAY_<X> attack."""
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
            cracked[hid] = (row.get("name",""), row.get("type",""), row.get("levels",""))

# Try every plausible action/modifier
actions = [
    # movement
    "JUMP","LAND","FALL","WALK","RUN","STOP","STEP","FOOTSTEP","CLIMB","CRAWL",
    "DUCK","CROUCH","STAND","SLIDE","ROLL","FLIP","SPIN","SKID","DASH","SPRINT",
    "TUMBLE","HOP","SKIP","STOMP","SHUFFLE","TIPTOE","SCAMPER","TROT","JOG","MARCH",
    "LEAP","DIVE","BOUNCE","SPRING",
    "JUMP_HARD","JUMP_HIGH","JUMP_LOW","JUMP_FORWARD","JUMP_BACK","JUMP_END",
    "JUMP_UP","JUMP_DOWN","JUMP_LAND","JUMP_FALL","JUMP_PRE","JUMP_START",
    "LAND_HARD","LAND_SOFT","LAND_BIG","LAND_SM","LAND_DEEP","LAND_HEAVY","LAND_LIGHT",
    "LAND_HOLLOW","LAND_FLAT","LAND_END","LAND_LOUD","LAND_QUIET",
    "FALL_HARD","FALL_SOFT","FALL_DOWN","FALL_BIG","FALL_LONG","FALL_SHORT","FALL_END",
    "RUN_FAST","RUN_SLOW","RUN_STOP","RUN_END","RUN_LOOP","RUN_HARD","RUN_SOFT","RUN_BIG","RUN_SM",
    "WALK_HARD","WALK_SOFT","WALK_FAST","WALK_SLOW","WALK_END","WALK_LOOP",
    "WALK_QUIET","WALK_LOUD","WALK_BIG","WALK_SM",
    "FOOTSTEP_LEFT","FOOTSTEP_RIGHT","FOOTSTEP_QUIET","FOOTSTEP_LOUD",
    "FOOTSTEP_HARD","FOOTSTEP_SOFT","FOOTSTEP_BIG","FOOTSTEP_SM",
    "STEP_LEFT","STEP_RIGHT","STEP_QUIET","STEP_LOUD","STEP_HARD","STEP_SOFT",
    "STEP_UP","STEP_DOWN","STEP_BIG","STEP_SM","STEP_HIGH","STEP_LOW",
    "DUCK_DOWN","DUCK_UP","DUCK_HIT","DUCK_END",
    "ROLL_DOWN","ROLL_UP","ROLL_END","ROLL_LEFT","ROLL_RIGHT","ROLL_LOOP","ROLL_BIG","ROLL_SM",
    "SLIDE_DOWN","SLIDE_UP","SLIDE_END","SLIDE_LEFT","SLIDE_RIGHT","SLIDE_BIG","SLIDE_SM",
    "SLIDE_LOOP",
    "CLIMB_UP","CLIMB_DOWN","CLIMB_END","CLIMB_LOOP",
    "DIE","DEATH","DIE_FALL","DIE_HIT","DIE_BURN","DIE_FRY","DIE_DROWN","DIE_CRY",
    "DIE_BIG","DIE_SM","DIE_LONG","DIE_SHORT","DIE_END","DIE_FINAL",
    "DIE_SQUISH","DIE_GIB","DIE_SHRINK","DIE_GAS","DIE_SPLAT",
    "HURT","HURT_HIT","HURT_BIG","HURT_SM","HURT_LO","HURT_HI","HURT_END","HURT_LIGHT","HURT_HEAVY",
    "OUCH","OUCH_BIG","OUCH_SM","PAIN","SCREAM","CRY","YELL","SHOUT","MOAN","GROAN",
    "SCREAM_LO","SCREAM_HI","SCREAM_BIG","SCREAM_SM","SCREAM_END","SCREAM_LONG",
    "CRY_BABY","CRY_LOUD","CRY_SOFT","CRY_END",
    "HIT","HIT_BIG","HIT_SM","HIT_END","HIT_GROUND","HIT_WALL","HIT_HEAD",
    "DAMAGE","DAMAGE_BIG","DAMAGE_SM","DAMAGE_END",
    "BREATH","BREATHE","PANT","GASP","SIGH","HUFF","PUFF","HMPH",
    "BREATH_IN","BREATH_OUT","BREATH_DEEP","BREATH_HARD",
    "EAT","EAT_END","EAT_LOOP","EAT_BITE","EAT_CHEW","EAT_GULP","EAT_SUCK","EAT_DROP",
    "BITE","CHEW","GULP","SUCK","SWALLOW","SPIT","SPIT_OUT","SPIT_FIRE",
    "DRINK","DRINK_END","DRINK_GULP","DRINK_LOOP","DRINK_SUCK",
    "TALK","SPEAK","MUMBLE","WHISPER","MUTTER",
    "LAUGH","LAUGH_BIG","LAUGH_SM","LAUGH_END","GIGGLE","CHUCKLE","SNORT","CACKLE",
    "FART","FART_BIG","FART_SM","FART_END","FART_LONG","FART_SHORT","FART_LOUD","FART_QUIET",
    "BURP","BURP_END","BURP_BIG","BURP_SM","BELCH","BELCH_BIG","BELCH_SM",
    "COUGH","COUGH_END","COUGH_BIG","COUGH_LOUD",
    "SNEEZE","SNEEZE_END","SNEEZE_BIG","SNEEZE_LOUD",
    "GRUNT","GRUNT_LO","GRUNT_HI","GRUNT_BIG","GRUNT_SM","GRUNT_END","GRUNT_LONG",
    "GULP","GULP_END","GULP_BIG","GULP_SM",
    "PUNCH","KICK","SLAP","PUSH","PULL",
    "PICKUP","GRAB","DROP","TAKE","THROW","TOSS","HOLD","RELEASE",
    "ATTACK","DEFEND","BLOCK","DODGE",
    "JUMP_TURN","TURN","TURN_LEFT","TURN_RIGHT","TURN_BACK",
    "FLY","SWIM","DIVE","FLOAT","SINK",
    "SPRING_UP","SPRING_DOWN","SPRING_END","BOUNCE_UP","BOUNCE_DOWN","BOUNCE_END",
    "SNORE","SLEEP","WAKE","AWAKEN",
    "IDLE","WAIT","REST",
    "RESPAWN","SPAWN","APPEAR","DISAPPEAR",
    "SPLASH","BUBBLE","DROWN",
    "BURN","FREEZE","SHOCK","STUN","FRY",
    "GUM_PIERCE","GUM_HIT",
    "WIN","LOSE",
]
mods = ["","_01","_02","_03","_04","_05","_1","_2","_3","_4","_5",
        "_LO","_HI","_LOW","_HIGH","_BIG","_SM","_LG","_HUGE","_END","_LOOP",
        "_A","_B","_C","_LEFT","_RIGHT","_UP","_DOWN","_DN",
        "_QUIET","_LOUD","_SOFT","_HARD","_FAST","_SLOW",
        "_SHORT","_LONG","_DEEP","_FLAT"]

hits_by_id = defaultdict(list)
total = 0
for act in actions:
    for mod in mods:
        for pre in ["FX_KLAY","FX_KMEN","FX_KLAYMEN","FX_KMAN","FX_PLAYER","FX_HERO","FX_KID"]:
            name = pre + "_" + act + mod
            total += 1
            h = calcHash(name)
            if h in uncracked:
                hits_by_id[h].append(name)
            if h in cracked:
                # Already known - sanity
                pass

print(f"Tried {total} names -> {len(hits_by_id)} ids hit")

def lvls(s): return len(s.split(';')) if s else 99
all_hits = sorted(hits_by_id.items(), key=lambda x: (lvls(uncracked[x[0]][1]), uncracked[x[0]][2], x[0]))
for hid, names in all_hits:
    t, lv, fl = uncracked[hid]
    print(f"\n0x{hid:08x} {t} popc={fl} levels=[{lv}]")
    for n in sorted(set(names))[:6]:
        print(f"    {n!r}")
