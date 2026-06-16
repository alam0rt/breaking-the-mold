#!/usr/bin/env python3
"""3-token compound name attack.

Uses cumulative property of calcHash:
    h(A+B+C) = h(A) ^ rotl(h(B), sh_A) ^ rotl(h(C), sh_A + sh_B)

For each (A, B) pair, compute the partial hash and shift, then query for C.

Strategy
--------
- Token1 (head): SUBJECT (character/object/level/UI element) — game vocab
- Token2 (mid):  ACTION/STATE — both game and dictionary
- Token3 (tail): VARIANT/INDEX — short letter/digit modifier

Output: docs/analysis/asset-identification/three_token_hits.csv
"""
from __future__ import annotations
import argparse
import csv
import sys
import time
from collections import defaultdict
from pathlib import Path

sys.path.insert(0, "tools/scripts")
from compound_hash_attack import calc_hash_and_shift, rotl, rotr  # type: ignore

ROOT = Path(__file__).resolve().parents[2]
SEED = 0x28C0E011
OUT_ROT = 27
CONFIRMED = {0x29C0E211, 0x2AD0F011, 0x0AD0F813, 0x68C0F413, 0x69C04050, 0x69C8F473}


# Heads: subjects (mostly nouns, characters, objects)
HEADS = """
KLAYMEN KLAY CLAY PLAYER HERO WILLIE WILLY DOUG HOBORG TRIBE
SKULL MONKEY SKULLMONKEY SKULLMONKEYS MAGE MONKEYMAGE
SHRINEY SHRINEYGUARD GUARD JOE JOEHEAD JOEHEADJOE JHJ
GLENN GLEN YNTIS YNT WIZZ KLOGG KLOG KING
KEEPER CLAYKEEPER LOUDMOUTH MENTALMONKEY TEMPEST
JUMPYGORILLA TRIPLELASER BARFO ELBARFO FLAPPER
JX TESTPILOT ROCKETPACK BOMBER MONK
ROYALGUARD ROBOTHOVER HEADSHOOTER FORKSHOOTER
SNOBLO EGGBEATER PROPELLER CASTLETROOPER
PIPECLEANER TONGUEMONK INFERNO
POPCORNSKULL WORKERYNT CENTURION SWARM BARKINGBIRD
BULLET BULLETS GREENBULLET PHOENIXHAND PHART PHARTHEAD
HALO SHIELD HAMSTERSHIELD GLIDEY YELLOWBIRD SUPERWILLIE
ONEUP EXTRALIFE SWIRLY SWIRLYQ KLAYHEAD KLAYMENHEAD
SPARKLE GREENHEART YELLOWCHEVRON CHEVRON
FATTYBOOM FIFI MICHAEL DACENNA GORDO FETUS BABY IDOL DEMON DEVIL
PLATFORM MOVPLATFORM CLOCK VIRTUAL CATCHBALL SPIKEBALL
KLOGGBALL TREASUREBALL TELEPORTBALL EXIT DOOR KEY BUTTON SWITCH LEVER
BARREL SPRING TRAMPOLINE BOUNCY SPIKE SAW FAN WHEEL
DEBRIS PARTICLE EXPLOSION SMOKE FLASH DUST SPLASH GORE BLOOD
GOO SLIME MUCUS BUBBLE FIRE FLAME WATER GLOW SHINE STAR RING
WAVE TRAIL SHARD ICE FROST LIGHTNING BOLT BEAM RAY SPAWN
MENU OPTIONS TITLE LOGO FONT TEXT DIGIT NUMBER GLYPH CURSOR
POINTER ARROW SELECTOR CREDIT CREDITS PASSWORD INTRO OUTRO ENDING
GAMEOVER GAME DEMO LOADING PRESS START PAUSE QUIT
HEAD BODY TAIL WING HORN EYE MOUTH NOSE EAR ARM LEG FOOT
FINGER TOE HAIR BEARD SKIN BONE STOMACH BELLY CHEST TORSO NECK BACK
GEM HEART ORB COIN BALL ICON BG MAP TILE
KAB FIN FINFLAP NORM ROYAL MAGEMAN MAGE BOSS
""".split()

# Mids: actions/states/types
MIDS = """
WALK RUN JUMP FALL LAND DIE DEATH HURT HIT ATTACK SHOOT
FIRE SLAM BOUNCE OPEN CLOSE START END ENTER EXIT TURN
EAT SLEEP ROLL SLIDE FLY GLIDE SPIN SPIT YELL SCREAM TAUNT
VICTORY DANCE IDLE REST WAIT STAND STANDING IDLING
WALKING RUNNING JUMPING FALLING LANDING DYING SHOOTING SHOT
ATTACKING SLAMMING BOUNCING OPENING CLOSING TURNING ROLLING
SLIDING FLYING GLIDING SPINNING SPITTING YELLING SCREAMING
COMMON RARE ROYAL UNIQUE
SMALL BIG TINY HUGE LARGE
GREEN BLUE RED YELLOW ORANGE PURPLE BLACK WHITE BROWN
INTRO OUTRO MAIN BASE ALT VARIANT NEW OLD
LEFT RIGHT UP DOWN NORTH SOUTH EAST WEST FRONT BACK SIDE
TOP BOTTOM MIDDLE
ANIM ANIMATION ICON SPRITE FONT TEXT IMAGE PIC
PROJ PROJECTILE SHOT BULLET FIREBALL
DEBRIS PIECE PART PARTS FRAGMENT SHARD CHUNK
SPAWN INIT START STOP HOLD HOLDING WAIT WAITING
HOVER HOVERING FLOAT FLOATING
""".split()

# Tails: numbers, letters, short suffixes
TAILS = ["", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
         "00", "01", "02", "03", "04", "05", "06", "07", "08", "09",
         "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "20",
         "A", "B", "C", "D", "E", "F",
         "L", "R", "U", "D",
         "FX", "ANIM", "PIC"]


def precompute(words: list[str]) -> list[tuple[str, int, int]]:
    out = []
    seen = set()
    for w in words:
        w = w.upper()
        if w in seen:
            continue
        seen.add(w)
        h, sh = calc_hash_and_shift(w)
        out.append((w, h, sh))
    return out


def load_targets() -> dict[int, list[int]]:
    out: dict[int, list[int]] = defaultdict(list)
    with (ROOT / "docs/reference/asset-ids-master.csv").open() as f:
        rdr = csv.DictReader(f)
        for r in rdr:
            i = int(r["id_hex"], 16)
            t = rotr(i ^ SEED, OUT_ROT)
            out[t].append(i)
    return out


def attack(heads, mids, tails, targets):
    # For each (A, B) prefix pair, compute partial (h_AB, sh_AB), then for each
    # target h_T, compute required h_C = rotr(h_T XOR h_AB, sh_AB) and check.

    # Index tails by hash
    c_by_h: dict[int, list[str]] = defaultdict(list)
    for c, h, _ in tails:
        c_by_h[h].append(c)

    target_hashes = list(targets.keys())
    print(f"  attack: {len(heads)} × {len(mids)} × {len(tails)} = {len(heads)*len(mids)*len(tails):,} compounds")
    print(f"  vs {len(target_hashes)} targets")

    hits = []
    t0 = time.time()
    pairs = 0
    for ai, (A, h_A, sh_A) in enumerate(heads):
        for B, h_B, sh_B in mids:
            # combine A + B
            h_AB = h_A ^ rotl(h_B, sh_A)
            sh_AB = (sh_A + sh_B) & 31
            pairs += 1
            for h_T in target_hashes:
                req_h_C = rotr(h_T ^ h_AB, sh_AB)
                if req_h_C in c_by_h:
                    for C in c_by_h[req_h_C]:
                        full = A + B + C
                        h_full, _ = calc_hash_and_shift(full)
                        if h_full != h_T:
                            continue
                        aid = (SEED ^ rotl(h_full, OUT_ROT)) & 0xFFFFFFFF
                        if aid in targets[h_T] and aid not in CONFIRMED:
                            hits.append((aid, A, B, C, full))
        if (ai + 1) % 20 == 0:
            elapsed = time.time() - t0
            rate = pairs / elapsed if elapsed > 0 else 0
            print(f"    head {ai+1}/{len(heads)}  ({rate:.0f} pairs/s)  hits={len(hits)}",
                  flush=True)

    elapsed = time.time() - t0
    print(f"  finished in {elapsed:.1f}s")
    return hits


def main():
    heads = precompute(HEADS)
    mids = precompute(MIDS)
    tails = precompute(TAILS)
    print(f"heads={len(heads)}  mids={len(mids)}  tails={len(tails)}")

    targets = load_targets()
    print(f"{sum(len(v) for v in targets.values())} ids → {len(targets)} unique calcHashes")

    hits = attack(heads, mids, tails, targets)

    # dedupe & sort
    seen = set()
    unique = []
    for aid, A, B, C, full in hits:
        key = (aid, full)
        if key in seen:
            continue
        seen.add(key)
        unique.append((aid, A, B, C, full))

    print(f"\n{len(unique)} unique hits across {len({h[0] for h in unique})} IDs")

    by_id = defaultdict(list)
    for aid, A, B, C, full in unique:
        by_id[aid].append((A, B, C, full))

    print(f"\n=== top hits by ID ===")
    for aid in sorted(by_id, key=lambda x: -len(by_id[x]))[:50]:
        n = len(by_id[aid])
        sample = "; ".join(f"{full}" for _, _, _, full in by_id[aid][:5])
        print(f"  0x{aid:08x}  ({n})  {sample[:120]}")

    out = ROOT / "docs/analysis/asset-identification/three_token_hits.csv"
    out.parent.mkdir(parents=True, exist_ok=True)
    with out.open("w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["id_hex", "tokenA", "tokenB", "tokenC", "full"])
        for aid, A, B, C, full in sorted(unique, key=lambda x: (x[0], x[4])):
            w.writerow([f"0x{aid:08x}", A, B, C, full])
    print(f"\nwrote {out.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
