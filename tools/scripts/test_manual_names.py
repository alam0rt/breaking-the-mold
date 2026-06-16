#!/usr/bin/env python3
"""Check whether known manual enemy/character names are in the asset table.

If NONE match, the asset table doesn't index by user-facing names —
the names must be internal codenames or stripped engine identifiers.
"""
import sys, csv
sys.path.insert(0, "tools/scripts")
from compound_hash_attack import asset_id

ROOT = "/home/sam/projects/btm"

# Load all IDs
ids = {}  # int -> rest of CSV row
with open(f"{ROOT}/docs/reference/asset-ids-master.csv") as f:
    rdr = csv.DictReader(f)
    for r in rdr:
        ids[int(r["id_hex"], 16)] = r

# Manual enemy/character/item names, plus common variants
NAMES = """
KLAYMEN KLAY CLAYMEN
SKULLMONKEY SKULLMONKEYS MONKEY
KING KINGSKULLMONKEY KINGMONKEY
JOEHEAD JOEHEADJOE JOE
MAGE MONKEYMAGE
SHRINEY SHRINEYGUARD
GLENN GLEN
KLOGG KLOG KLOGGTHEKLOGG
PHARTHEAD PHART FARTHEAD FART
FATTYBOOMBOOM FATTY BOOMBOOM
WIZZ WIZARD
JX1137 JX
TESTPILOT
ROYALGUARD ROYALMONK
ROBOTHOVER ROBOTSHOOTER
HEADSHOOTER FORKSHOOTER
SNOBLO
EGGBEATER PROPELLER
CASTLETROOPER
PIPECLEANER
TONGUEMONK
MENTALMONKEY MENTAL
POPCORNSKULL POPCORN
BARKINGBIRD BARKING
WORKERYNT WORKER YNT
CENTURION
SWARM
TEMPEST
INFERNO
LOUDMOUTH LOUD
CLAYKEEPER KEEPER
JUMPYGORILLA JUMPY GORILLA
TRIPLELASER LASER
BARFO ELBARFO BARF
FLAPPER FLAP
BOMBER
PHOENIXHAND PHOENIX
GREENBULLETS GREENBULLET
HAMSTERSHIELD HAMSTER
UNIVERSEENEMA SUPERPOWER
GLIDEY YELLOWBIRD
SUPERWILLIE SUPER
ONEUP EXTRALIFE LIFE
CHEVRON YELLOWCHEVRON
GREENHEART GREEN
FIFI MICHAEL DACENNA GORDO
KLAYMENHEAD KLAYHEAD
SWIRLY SWIRLYQ
ICON1970
SPARKLE
GEM GEMS
BULLETS BULLET
KEY DOOR EXIT BUTTON
SPRING TRAMPOLINE
SPIKEBALL SPIKE
PLATFORM MOVPLATFORM
CLOCKPLAT CLOCK
KLOGGBALL TREASUREBALL TELEPORTBALL
HALO SHIELD
PASSWORD PAUSE OPTIONS QUIT CONTINUE QUITGAME
PRESSSTART STARTGAME
FONT TITLE INTRO OUTRO
GAMEOVER GAMEOVERANIM
""".split()

# Underscore/CamelCase variants
def variants(n):
    out = {n, n.upper(), n.lower(), n.title()}
    return out

print(f"=== Testing {len(NAMES)} names plus variants ===\n")
hits = []
for n in NAMES:
    for v in variants(n):
        h = asset_id(v)
        if h in ids:
            hits.append((v, h, ids[h]))
            print(f"  HIT  {v:25s} -> 0x{h:08x}  kind={ids[h]['kinds']}  popc={ids[h]['popcount']}")

print(f"\n{len(hits)} matches")

# Now try compound forms
print("\n=== Testing 2-token compounds with manual names ===")
ACTIONS = "WALK RUN JUMP DIE DEATH ATTACK SHOOT JUMP JUMPS JUMPING ATTACKING SHOOTING DYING DEAD ALIVE IDLE SPAWN SPAWNS HIT HURT HOVER FLY GLIDE SPIN ROLL SLIDE FALL FALLING LAND LANDING TURN".split()
for n in NAMES[:30]:
    for a in ACTIONS:
        for fmt in [f"{n}{a}", f"{n}_{a}", f"{a}{n}", f"{a}_{n}"]:
            h = asset_id(fmt)
            if h in ids:
                hits.append((fmt, h, ids[h]))
                print(f"  HIT  {fmt:30s} -> 0x{h:08x}  kind={ids[h]['kinds']}")

print(f"\nFINAL: {len(hits)} hits across {len(set(h[1] for h in hits))} unique IDs")

# Save
with open(f"{ROOT}/docs/analysis/asset-identification/manual_name_hits.csv", "w", newline="") as f:
    w = csv.writer(f)
    w.writerow(["name","id_hex","kinds","floor","sources"])
    seen = set()
    for name, h, row in hits:
        key = (name, h)
        if key in seen: continue
        seen.add(key)
        w.writerow([name, f"0x{h:08x}", row['kinds'], row['popcount'], row['sources']])
