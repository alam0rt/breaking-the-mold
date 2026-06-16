#!/usr/bin/env python3
"""Mass token-pair attack with underscore awareness.

Key insight: GEM_COMMON hashes identically to GEMCOMMON. The hash only sees
A-Z and 0-9 chars. So we hash the bare alpha-numeric "TOKEN1+TOKEN2" but
report results as TOKEN1_TOKEN2 (the likely real form).

Strategy: combine high-prior game vocabulary heads with extensive English/
domain-specific tails. Score by:
  +10 if both tokens in game vocab
  +5  if A in game vocab
  +5  if B in game vocab
  +3  if either is a generic asset modifier (ANIM, ICON, etc.)

Output: docs/analysis/asset-identification/underscore_token_hits.csv
"""
from __future__ import annotations
import argparse, csv, sys, time
from collections import defaultdict
from pathlib import Path

sys.path.insert(0, "tools/scripts")
from compound_hash_attack import calc_hash_and_shift, rotl, rotr

ROOT = Path(__file__).resolve().parents[2]
SEED = 0x28C0E011
OUT_ROT = 27
CONFIRMED = {0x29C0E211, 0x2AD0F011, 0x0AD0F813, 0x68C0F413, 0x69C04050, 0x69C8F473,
             0x085860D4}  # GEM_COMMON


# Asset-naming type modifiers — used as prefix or suffix
TYPE_MODIFIERS = """
ANIM ANIMATION FRAME FRAMES LOOP CYCLE SEQ SEQUENCE
ICON SPRITE SPR PIC IMAGE IMG TEX BG
SND SFX FX MUSIC MUS VOICE VOX
FONT TEXT GLYPH DIGIT NUMBER NUM LABEL
DEMO INTRO OUTRO ENDING TRANS TRANSITION
FX1 FX2 FX0 FX01 FX02 FX00
""".split()


# Game-specific tokens (common subjects)
GAME_TOKENS_RAW = """
KLAYMEN KLAY CLAY PLAYER HERO WILLIE WILLY DOUG HOBORG TRIBE
SKULL MONKEY SKULLMONKEY SKULLMONKEYS MAGE MONKEYMAGE
SHRINEY SHRINEYGUARD GUARD JOE JOEHEAD JHJ
GLENN GLEN YNTIS YNT WIZZ KLOGG KLOG KING
KEEPER CLAYKEEPER LOUDMOUTH MENTALMONKEY TEMPEST
JUMPYGORILLA TRIPLELASER BARFO ELBARFO FLAPPER
JX1137 TESTPILOT ROCKETPACK BOMBER MONK
ROYALGUARD ROBOTHOVER HEADSHOOTER FORKSHOOTER
SNOBLO EGGBEATER PROPELLER CASTLETROOPER
PIPECLEANER TONGUEMONK INFERNO
POPCORNSKULL WORKERYNT CENTURION SWARM BARKINGBIRD
BULLET BULLETS GREENBULLET PHOENIXHAND PHART PHARTHEAD
HALO SHIELD HAMSTERSHIELD GLIDEY YELLOWBIRD SUPERWILLIE
ONEUP EXTRALIFE SWIRLY SWIRLYQ KLAYHEAD KLAYMENHEAD
SPARKLE GREENHEART YELLOWCHEVRON CHEVRON
FATTYBOOM FIFI MICHAEL DACENNA GORDO FETUS BABY IDOL DEMON DEVIL
PLATFORM MOVPLAT CLOCK VIRTUAL CATCHBALL SPIKEBALL
KLOGGBALL TREASUREBALL TELEPORTBALL EXIT DOOR KEY BUTTON SWITCH LEVER
BARREL SPRING TRAMPOLINE BOUNCY SPIKE SAW FAN WHEEL
DEBRIS PARTICLE EXPLOSION SMOKE FLASH DUST SPLASH GORE BLOOD
GOO SLIME MUCUS BUBBLE FIRE FLAME WATER GLOW SHINE STAR RING
WAVE TRAIL SHARD ICE FROST LIGHTNING BOLT BEAM RAY SPAWN
MENU OPTIONS TITLE LOGO FONT TEXT DIGIT NUMBER GLYPH CURSOR
POINTER ARROW SELECTOR CREDIT CREDITS PASSWORD INTRO OUTRO ENDING
GAMEOVER GAME DEMO LOADING PRESS START PAUSE QUIT CONTINUE
HEAD BODY TAIL WING HORN EYE MOUTH NOSE EAR ARM LEG FOOT
FINGER TOE HAIR BEARD SKIN BONE STOMACH BELLY CHEST TORSO NECK BACK
GEM HEART ORB COIN BALL ICON BG MAP TILE
KAB FIN FINFLAP NORM ROYAL MAGEMAN BOSS
LEFT RIGHT UP DOWN FRONT BACK SIDE TOP BOTTOM
WALK RUN JUMP FALL LAND DIE DEATH HURT HIT ATTACK SHOOT
FIRE SLAM BOUNCE OPEN CLOSE START END ENTER EXIT TURN
EAT SLEEP ROLL SLIDE FLY GLIDE SPIN SPIT YELL SCREAM TAUNT
VICTORY DANCE IDLE REST WAIT STAND SPAWN APPEAR DISAPPEAR
COMMON RARE ROYAL UNIQUE BIG SMALL TINY HUGE LARGE
GREEN BLUE RED YELLOW ORANGE PURPLE BLACK WHITE BROWN
NEW OLD ALT VARIANT MAIN BASE
HOVER FLOAT TURRET LASERGUN SPIKEBALL
CHECKPNT CHECKPOINT SAVE
ENERGY POWER POWERUP UPGRADE
LEVEL STAGE WORLD AREA ZONE ROOM
SKY GROUND FLOOR WALL CEILING CLOUD
""".split()


# Levels (4-letter)
LEVELS = "BOIL BRG1 CAVE CLOU CRYS CSTL EGGS EVIL FINN FOOD GLEN GLID HEAD KLOG MEGA MENU MOSS PHRO RUNN SCIE SEVN SNOW SOAR TMPL WEED WIZZ".split()


def precompute(words):
    out = []
    seen = set()
    for w in words:
        w = w.upper()
        if w in seen: continue
        seen.add(w)
        h, sh = calc_hash_and_shift(w)
        out.append((w, h, sh))
    return out


def load_targets():
    out = defaultdict(list)
    with (ROOT/"docs/reference/asset-ids-master.csv").open() as f:
        rdr = csv.DictReader(f)
        for r in rdr:
            i = int(r["id_hex"], 16)
            t = rotr(i ^ SEED, OUT_ROT)
            out[t].append((i, r))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--extra", help="Extra wordlist file")
    ap.add_argument("--max-extra", type=int, default=50000)
    args = ap.parse_args()

    GAME = set(w.upper() for w in GAME_TOKENS_RAW)
    GAME |= set(LEVELS)
    GAME |= set(TYPE_MODIFIERS)

    # Token pool A & B
    poolA = list(GAME) + list(TYPE_MODIFIERS) + LEVELS
    poolB = list(GAME) + list(TYPE_MODIFIERS) + LEVELS
    if args.extra:
        with open(args.extra) as f:
            extra = [w.strip().upper() for w in f if 2 <= len(w.strip()) <= 8]
        if len(extra) > args.max_extra:
            extra = extra[:args.max_extra]
        poolB.extend(extra)
        print(f"Loaded {len(extra)} extra dictionary words")

    A = precompute(poolA)
    B = precompute(poolB)
    print(f"|A|={len(A)}  |B|={len(B)}")

    targets = load_targets()
    print(f"{len(targets)} unique target hashes ({sum(len(v) for v in targets.values())} ids)")

    # Index B by hash
    b_by_h = defaultdict(list)
    for w, h, sh in B:
        b_by_h[h].append((w, sh))

    target_hashes = list(targets.keys())
    hits = []
    t0 = time.time()
    for ai, (a, h_A, sh_A) in enumerate(A):
        for h_T in target_hashes:
            req_h_B = rotr(h_T ^ h_A, sh_A)
            if req_h_B in b_by_h:
                for b, _ in b_by_h[req_h_B]:
                    full = a + b   # bare; underscore inserted on output
                    h_full, _ = calc_hash_and_shift(full)
                    if h_full != h_T:
                        continue
                    aid = (SEED ^ rotl(h_full, OUT_ROT)) & 0xFFFFFFFF
                    matched = [(i, r) for i, r in targets[h_T] if i == aid]
                    if not matched: continue
                    if aid in CONFIRMED: continue
                    score = 0
                    if a in GAME: score += 5
                    if b in GAME: score += 5
                    if a in GAME and b in GAME: score += 5
                    if a in TYPE_MODIFIERS or b in TYPE_MODIFIERS: score += 3
                    if a in LEVELS: score += 3
                    if b in LEVELS: score += 3
                    hits.append((aid, a, b, score, matched[0][1]))
        if (ai + 1) % 50 == 0:
            elapsed = time.time() - t0
            rate = (ai + 1) / elapsed
            print(f"  {ai+1}/{len(A)}  ({rate:.0f}/s)  hits={len(hits)}",
                  flush=True)

    elapsed = time.time() - t0
    print(f"\nfinished in {elapsed:.1f}s")

    # Dedupe
    seen = set()
    unique = []
    for h in hits:
        key = (h[0], h[1], h[2])
        if key in seen: continue
        seen.add(key)
        unique.append(h)
    print(f"{len(unique)} unique hits across {len({h[0] for h in unique})} IDs")

    # By score
    print("\n=== HIGH CONFIDENCE (score >= 10) ===")
    for aid, a, b, score, row in sorted(unique, key=lambda x: (-x[3], x[0])):
        if score < 10: break
        print(f"  0x{aid:08x}  s={score:2d}  {a}_{b}  ({row['kinds']}, popc={row['popcount']})")

    print("\n=== MEDIUM CONFIDENCE (5-9) — top 50 ===")
    medium = [h for h in unique if 5 <= h[3] < 10]
    medium.sort(key=lambda x: (-x[3], x[0]))
    for aid, a, b, score, row in medium[:50]:
        print(f"  0x{aid:08x}  s={score:2d}  {a}_{b}  ({row['kinds']}, popc={row['popcount']})")

    out = ROOT/"docs/analysis/asset-identification/underscore_token_hits.csv"
    with out.open("w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["id_hex","tokenA","tokenB","name_underscore","name_bare","score","kinds","popcount"])
        for aid, a, b, score, row in sorted(unique, key=lambda x: (-x[3], x[0])):
            w.writerow([f"0x{aid:08x}", a, b, f"{a}_{b}", a+b, score, row['kinds'], row['popcount']])
    print(f"\nwrote {out.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
