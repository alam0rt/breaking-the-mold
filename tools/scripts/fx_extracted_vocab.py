"""Focused attack: extract tokens from verified cracks, try all (E, A, S) combos.
Uses smaller scope to actually finish in reasonable time.
"""
import sys, os, csv, re
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

# Extract all tokens from verified crack names
tokens = set()
for name in cracked.values():
    if "_" not in name: continue
    parts = name.split("_")
    for p in parts:
        if p.isalpha() and len(p) >= 2:
            tokens.add(p.upper())
        elif p.isdigit():
            pass

# Also add common ones
EXTRA = ["JUMP","LAND","FALL","WALK","RUN","STOP","STEP","FOOTSTEP","CLIMB","CRAWL",
         "DUCK","DIE","DEATH","HURT","HIT","SQUASH","SPRING","BOUNCE","DROP",
         "BURST","BLAST","BOOM","POP","PICKUP","GRAB","ZAP","ZING","SHOCK",
         "BUMP","KNOCK","TAP","CHIME","DING","BEEP","BUZZ","CLICK","SWITCH",
         "BLOW","WIND","GUST","STORM","BREATH","HOWL","WHOOSH","WHIR",
         "FLAP","HOVER","SOAR","GLIDE","FLY","FLOAT","DIVE","SWIM","SINK","RISE",
         "INFLATE","DEFLATE","PUFF","POOF","BUBBLE","DROP","SPLASH",
         "FIRE","BURN","BURNED","FROZEN","FREEZE","MELT","STUN","FRY","SHOCK",
         "BREAK","SHATTER","CRACK","SMASH","CRUSH","SPLINTER","BREAK",
         "OPEN","CLOSE","SLAM","LOCK","UNLOCK","ENTER","EXIT","DOOR","GATE",
         "RING","BELL","CHIME","DING","TICK","TOCK",
         "TALK","SPEAK","WHISPER","SHOUT","YELL","SCREAM","CRY","CHATTER",
         "LAUGH","GIGGLE","CACKLE","SNORT","SOB","WAIL","MOAN","GROAN","GRUNT",
         "PANT","GASP","SIGH","HUFF","PUFF","FART","BURP","BELCH","COUGH","SNEEZE","SNORE",
         "ROLL","ROTATE","TURN","TWIST","SPIN","WIND","WRAP","COIL",
         "HUM","DRONE","SQUEAL","SQUEAK","RUMBLE","CLAP","CHEER",
         "IDLE","WAIT","REST","SLEEP","WAKE","AWAKEN","UP","DOWN","LEFT","RIGHT",
         "SHINE","TWINKLE","GLOW","SPARK","FLASH","ZAP",
         "BUBBLE","DRIP","SPLAT","PLOP","BLOOP","GLUG",
         "CHANT","BLESS","CURSE","HEAL","REVIVE","SUMMON",
         "FALL","DROP","KO","FAINT","WOOZY",
         "FLIP","SLIDE","SKID","DASH","SPRINT",
         "HOP","SKIP","STOMP","SHUFFLE","TIPTOE","SCAMPER","TROT","JOG","MARCH",
         "LEAP","CHARGE","FIRE","SHOOT","LAUNCH","THROW","TOSS","CAST",
         "KICK","PUNCH","SLAP","PUSH","PULL","ATTACK","DEFEND",
         "BITE","CHEW","GULP","SUCK","SPIT","EAT","DRINK",
         "BLEED","HURT","OUCH","PAIN","HIT","DAMAGE","SQUASH","SQUISH",
         "POW","POWER","UP","BONUS","ICON","ITEM","ARMOR","SHIELD",
         "SAVE","LOAD","PAUSE","RESUME","START","END","BEGIN","FINISH","DONE",
         "PIP","POP","BUMP","KNOCK","TAP","RATTLE","BUMP",
         "SHIM","SHIMMER","GLEAM","BLINK","DAZZLE","SHINE","SPARK","FLASH",
         "BLOW","GUST","GALE","STORM","HOWL","WIND","BREEZE","WHOOSH","WHISH","WHIRR","SWISH","SWOOSH",
         "BUBBLE","SPLASH","DROP","DROPLET","PLOP","BLOOP","GLUG","SPLAT","RIPPLE","SLOSH","WAVE",
         "CHANT","BLESS","CURSE","HEAL","REVIVE","SUMMON","INVOKE","BANISH",
         "BIRTH","WAKE","SLEEP","DEATH","DIE","KILL","REVIVE","RESPAWN","SPAWN",
         "WIN","LOSE","DRAW","TIE","CHEER","CLAP","BOO","CROW",
         "BREAK","RIP","TEAR","SHRED","SLASH","CUT","STAB","PIERCE","JAB","PUNCH",
         "TINY","SMALL","BIG","HUGE","GIANT","MASSIVE","MINI","MAX","FULL","HALF","QUARTER",
         "FIRST","LAST","NEXT","PREV","ONLY","ALL","NONE","SOME","MANY","FEW",
         "FAST","SLOW","QUICK","SLUGGISH","SPEEDY","TURTLE","RABBIT",
         "EARLY","LATE","ON","OFF","TRUE","FALSE","YES","NO","OK","CANCEL",
         "SOFT","HARD","DEEP","SHALLOW","FLAT","ROUND","SQUARE",
         "LIGHT","HEAVY","DARK","BRIGHT","DIM","DULL","SHARP","BLUNT","SHARP",
         "HOT","COLD","WARM","COOL","ICY","STEAM","SMOKE","FIRE","FLAME","LAVA",
         "RED","BLUE","GREEN","WHITE","BLACK","GOLD","SILVER","COPPER","BRONZE",
         "ZOOM","WHIRL","SWING","FLY","SHOOT","SPRAY","DRIP",
         "MEGA","ULTRA","SUPER","HYPER","TURBO","NITRO","BOOST",
         "DEFAULT","NORMAL","SPECIAL","UNIQUE","RARE","COMMON","BASIC","BASIC",
         ]
tokens.update(EXTRA)

print(f"Vocab: {len(tokens)} tokens")

# Numeric suffixes
SUF = ["","_01","_02","_03","_04","_05","_06","_07","_08","_09","_10","_11","_12",
       "_1","_2","_3","_4","_5","_6","_7","_8","_9","_0"]
PREFIX = ["FX_","SFX_"]

# Generate all (E, A) pairs from tokens
hits_by_id = defaultdict(list)
total = 0
tlist = sorted(tokens)
for pre in PREFIX:
    for e in tlist:
        for a in tlist:
            base = pre + e + "_" + a
            for suf in SUF:
                name = base + suf
                total += 1
                h = calcHash(name)
                if h in uncracked:
                    hits_by_id[h].append(name)

print(f"Tried {total} names -> {len(hits_by_id)} ids hit")

def lvls(s): return len(s.split(';')) if s else 99

# Multi-candidate ids first (those are strongest)
multi_hits = [(h, sorted(set(ns))) for h, ns in hits_by_id.items() if len(set(ns)) > 1]
multi_hits.sort(key=lambda x: (-len(x[1]), x[0]))
print(f"\n=== Multi-candidate ids ({len(multi_hits)}) ===")
for hid, names in multi_hits[:30]:
    t, lv, fl = uncracked[hid]
    print(f"\n0x{hid:08x} {t} popc={fl} levels=[{lv}] ({len(names)} candidates)")
    for n in names[:6]:
        print(f"    {n!r}")

# Single-hit (probably suspicious but record them)
single = [(h, sorted(set(ns))) for h, ns in hits_by_id.items() if len(set(ns)) == 1]
single.sort(key=lambda x: (uncracked[x[0]][0], lvls(uncracked[x[0]][1]), uncracked[x[0]][2], x[0]))
print(f"\n=== Single-candidate ids ({len(single)}) ===")
for hid, names in single[:60]:
    t, lv, fl = uncracked[hid]
    nl = lvls(lv)
    print(f"  0x{hid:08x} {t:6s} popc={fl:2d} levels({nl})=[{lv[:40]:40s}] {names[0]!r}")
