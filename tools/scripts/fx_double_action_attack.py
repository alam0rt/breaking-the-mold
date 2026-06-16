"""Multi-token action attack: FX_<E>_<A>_<B>_<N> form."""
import sys, os, csv
from collections import defaultdict
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

uncracked = {}
with open("docs/analysis/asset-identification/cracked_names.csv") as f:
    r = csv.DictReader(f)
    for row in r:
        if row["status"] != "uncracked": continue
        try: hid = int(row["id_hex"], 16)
        except: continue
        uncracked[hid] = (row["type"], row["levels"], int(row["floor"]))

# Entities for multi-token: focus on SKULL, KLAY, FINN
ENT = ["SKULL","KLAY","FINN","BIRD","KMEN","BOSS","JOE","RAT","BUG",
       "WORM","FROG","FISH","GUM","BABY","EGG","WIZ","BIG","MONK",
       "PHART","MA","BLB","HEAD","WILLIE","TROMBONE","GLENN","KLOGG"]

# Two-word actions
DOUBLE_ACT = [
    "JUMP_UP","JUMP_DOWN","JUMP_LAND","JUMP_FALL","JUMP_HIGH","JUMP_LOW",
    "FALL_DOWN","FALL_HARD","FALL_SOFT","FALL_LAND","FALL_HIT",
    "WALK_LEFT","WALK_RIGHT","WALK_UP","WALK_DOWN",
    "RUN_LEFT","RUN_RIGHT","RUN_FAST","RUN_SLOW","RUN_STOP",
    "DIE_HIT","DIE_FALL","DIE_BURN","DIE_FRY","DIE_SQUISH","DIE_BIG","DIE_SM",
    "HURT_HIT","HURT_FALL","HURT_FLY","HURT_BIG","HURT_SM",
    "PICK_UP","PUT_DOWN","TAKE_HIT","DROP_DOWN",
    "OPEN_DOOR","CLOSE_DOOR","SHUT_DOOR","BREAK_GLASS","BREAK_STONE",
    "EAT_FOOD","DRINK_WATER","SPIT_OUT","SPIT_FIRE",
    "SHOOT_FIRE","SHOOT_BIG","SHOOT_SM","SHOOT_AIM","SHOOT_AT",
    "BLOW_GALE","BLOW_GUST","BLOW_WIND","BLOW_AIR","BLOW_BIG","BLOW_SM",
    "DUCK_DOWN","DUCK_UP","DUCK_HIT","DUCK_FALL",
    "CRY_LOUD","CRY_SOFT","CRY_BABY",
    "LAND_HARD","LAND_SOFT","LAND_BIG","LAND_SM",
    "SPIN_UP","SPIN_DOWN","SPIN_FAST","SPIN_SLOW",
    "ROLL_LEFT","ROLL_RIGHT","ROLL_FAST",
    "FLY_UP","FLY_DOWN","FLY_BY","FLY_AWAY",
    "GLIDE_UP","GLIDE_DOWN","DIVE_DOWN",
    "PUFF_UP","PUFF_DOWN","PUFF_OUT","PUFF_FALL",
    "GROW_BIG","GROW_SM","SHRINK_DOWN","SHRINK_UP",
    "SPLASH_BIG","SPLASH_SM","SPLASH_DOWN",
    "BOUNCE_UP","BOUNCE_DOWN","BOUNCE_HIGH",
    "ROLL_OVER","ROLL_DOWN","ROLL_UP",
    "FOOTSTEP_LEFT","FOOTSTEP_RIGHT","FOOTSTEP_QUIET","FOOTSTEP_LOUD",
    "ZAP_HIT","ZAP_SM","ZAP_BIG","ZAP_END",
    "RIP_DN","RIP_UP","RIP_BIG","RIP_SM","RIP_END",
    "GRUNT_LO","GRUNT_HI","GRUNT_BIG","GRUNT_SM","GRUNT_END","GRUNT_LONG",
    "BITE_DN","BITE_UP","BITE_SM","BITE_BIG",
    "EAT_FAST","EAT_SLOW","EAT_DONE",
    "SCREAM_LO","SCREAM_HI","SCREAM_END","SCREAM_LONG",
    "WALK_HARD","WALK_SOFT","WALK_QUIET","WALK_LOUD",
    "STEP_HARD","STEP_SOFT","STEP_LO","STEP_HI",
    "SPRING_UP","SPRING_DOWN","SPRING_END",
    "POOP_OUT","POOP_DN","POOP_UP","POOP_LF","POOP_RT",
    "BREAK_OPEN","BREAK_DOWN","CRASH_DOWN",
    "PIERCE_DN","PIERCE_UP","PIERCE_SM",
    "GUM_PIERCE","GUM_HIT","GUM_BURST",
    "CLAP_END","CLAP_LOUD","CLAP_LO","CLAP_HI",
    "FIRE_END","FIRE_START","FIRE_HIT","FIRE_BIG","FIRE_SM",
]

SUF = ["", "_01","_02","_03","_04","_05","_06","_07","_08","_09","_10",
       "_1","_2","_3","_4","_5","_6","_7","_8","_9",
       "_A","_B","_C","_D"]
PREFIX = ["FX_","SFX_"]

hits_by_id = defaultdict(list)
total = 0
for pre in PREFIX:
    for e in ENT:
        for a in DOUBLE_ACT:
            for s in SUF:
                name = pre + e + "_" + a + s
                total += 1
                h = calcHash(name)
                if h in uncracked:
                    hits_by_id[h].append(name)

print(f"Tried {total} names -> {len(hits_by_id)} ids hit")

def lvls(s): return len(s.split(';')) if s else 99

# Sort and print
all_hits = []
for hid, names in hits_by_id.items():
    t, lv, fl = uncracked[hid]
    all_hits.append((hid, sorted(set(names)), t, lv, fl))
all_hits.sort(key=lambda x: (x[2], lvls(x[3]), x[4], x[0]))

for hid, names, t, lv, fl in all_hits:
    print(f"\n0x{hid:08x} {t} popc={fl} levels=[{lv}]")
    for n in names[:5]:
        print(f"    {n!r}")
