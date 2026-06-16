#!/usr/bin/env python3
"""Long brute -> annotated candidate list for human review.

Produces docs/analysis/asset-identification/brute_review_hits.csv with every
hash collision found, annotated by the target ID's level coverage + visual role,
so a human can scan for clues (does a candidate name match the known picture?).

Two hash models are tested for every string:
  RAW  : id = calcHash(name)                       (audio + gameplay assets)
  WRAP : id = 0x28C0E011 ^ rotl(calcHash(name),27) (localized menu text)

Sources of candidate strings:
  1. rockyou wordlist (real English-ish words), len 3..14, alpha.
  2. a semantic grammar: AFFIX x SUBJECT x STATE x DECO using compound hashing.

Run:  python3 tools/scripts/sprite_brute_review.py
"""
from __future__ import annotations
import csv, itertools, sys, time
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
AID = ROOT / "docs/analysis/asset-identification"
SEED = 0x28C0E011
ROCK = "/nix/store/6q8vxs8wigcxkkbij9kz5hb5b77vkm8j-rockyou-2025.3/share/wordlists/rockyou.txt"


def chs(s: str):
    h = sh = 0
    for ch in s:
        c = ord(ch)
        if 65 <= c <= 90: pass
        elif 97 <= c <= 122: c -= 32
        elif 48 <= c <= 57: c += 22
        else: continue
        sh = (sh + c - 64) & 31
        h ^= 1 << sh
    return h, sh


def rotl(v, r):
    r &= 31
    return ((v << r) | (v >> ((32 - r) & 31))) & 0xFFFFFFFF


# ---------------------------------------------------------------- targets
def load_targets():
    """id -> (type, levels, visual_role)."""
    info = {}
    with (AID / "cracked_names.csv").open() as f:
        for r in csv.DictReader(f):
            if not r["id_hex"].startswith("0x"): continue
            if r["status"] != "uncracked": continue
            info[int(r["id_hex"], 16)] = [r["type"], r["levels"], ""]
    tpl = AID / "sprite_identification_template.csv"
    if tpl.exists():
        with tpl.open() as f:
            for r in csv.DictReader(f):
                try: idv = int(r["sprite_hex"], 16)
                except Exception: continue
                if idv in info and r.get("human_role"):
                    info[idv][2] = r["human_role"]
    return info


# ---------------------------------------------------------------- vocab
AFFIX = ["", "S_", "SP_", "SPR_", "SPRT_", "ANM_", "SEQ_", "A_", "OBJ_", "SM_",
         "MENU_", "UI_", "HUD_", "FX_", "GFX_", "BG_", "OBJ", "SPR"]

SUBJECT = [
    "KLAY","KLAYMEN","KLAYMAN","CLAYMAN","PLAYER","HERO","WILLIE","MONKEY","SKULLMONKEY",
    "SKULL","MAGE","MONKEYMAGE","SHRINEY","SHRINEYGUARD","GUARD","JOE","JOEHEAD","GLENN","GLEN",
    "YNTIS","WIZZ","WIZARD","KLOGG","KLOG","KING","FINN","PHART","FART","PHARTHEAD","GLIDEY","GLIDE",
    "BIRD","YELLOWBIRD","BARKINGBIRD","RAT","GUM","GUMBALL","BULLET","BULLETS","GREENBULLET","AMMO",
    "PHOENIX","PHOENIXHAND","HAND","ENERGYBALL","HAMSTER","SHIELD","HALO","SUPERWILLIE","BUTTBOUNCE",
    "FIFI","MICHAEL","DACENNA","GORDO","FETUS","BABY","IDOL","DEMON","DEVIL","ENEMY","GORILLA","JUMPYGORILLA",
    "TRIPLELASER","LASER","BARFO","ELBARFO","FLAPPER","TESTPILOT","PILOT","ROCKETPACK","BOMBER","ROBOT",
    "HEADSHOOTER","FORKSHOOTER","SNOBLO","EGGBEATER","PROPELLER","CASTLETROOPER","TROOPER","PIPECLEANER",
    "WORM","TONGUE","INFERNO","POPCORN","WORKERYNT","CENTURION","CLAYKEEPER","KEEPER","LOUDMOUTH","TEMPEST",
    "PLATFORM","MOVPLATFORM","MOVINGPLATFORM","LIFT","CLOCKPLAT","VIRTUALPLAT","BALL","SPIKEBALL","KLOGGBALL",
    "TREASUREBALL","TELEPORTBALL","PORTAL","WARP","DOOR","GATE","KEY","BUTTON","SWITCH","LEVER","CRATE","BLOCK",
    "BARREL","SPRING","TRAMPOLINE","SPIKE","SAW","FAN","WHEEL","TURRET","GUN","ONEUP","EXTRALIFE","KLAYHEAD",
    "SWIRLY","SWIRLYQ","ICON","CHECKPOINT","SPARKLE","SPARK","TWINKLE","GREENHEART","CHEVRON","ORB","GEM",
    "DEBRIS","PARTICLE","EXPLOSION","POOF","BURST","SMOKE","FLASH","DUST","SPLASH","GORE","GOO","SLIME","MUCUS",
    "BUBBLE","FIRE","FLAME","WATER","GLOW","STAR","RING","SHARD","ICE","FROST","LIGHTNING","BOLT","ZAP","BEAM",
    "MENU","OPTIONS","TITLE","LOGO","FONT","TEXT","DIGIT","DIGITS","NUMBER","CURSOR","POINTER","ARROW","CREDITS",
    "PASSWORD","INTRO","OUTRO","ENDING","GAMEOVER","LOADING","PRESS","START","BACKGROUND","FRAME","BANNER","PANEL",
    "HEAD","BODY","TAIL","WING","HORN","EYE","MOUTH","TEETH","ARM","LEG","FOOT","HAIR","BONE","STOMACH","CHEST",
    "PHRO","SCIE","TMPL","FINN","MEGA","BOIL","SNOW","FOOD","BRG1","CAVE","WEED","EGGS","CLOU","SEVN","SOAR",
    "CRYS","CSTL","RUNN","MOSS","EVIL",
]

STATE = ["", "IDLE","STAND","STANDING","WALK","WALKING","RUN","RUNNING","JUMP","JUMPING","FALL","FALLING",
         "LAND","LANDING","DIE","DEATH","DEAD","HURT","HIT","BLINK","SHOOT","FIRE","FIRING","ATTACK","SLAM",
         "BOUNCE","OPEN","CLOSE","TURN","EAT","SLEEP","ROLL","SLIDE","FLY","FLYING","GLIDE","SPIN","SPIT",
         "SCREAM","TAUNT","VICTORY","DANCE","LEAP","CYCLE","SWING","CHARGE","DODGE","BLOCK","CLIMB","SWIM",
         "DUCK","CRAWL","REST","WAIT","SPAWN","GROW","SHRINK","BASE","MAIN","FULL","NORMAL","HEAD","BODY"]

DECO = ["", "0","1","2","3","00","01","02","03","04","A","B","C","L","R","U","D",
        "_0","_1","_2","_00","_01","_02","_A","_B","_L","_R","_LEFT","_RIGHT","_UP","_DN",
        "ANIM","SEQ","SPR","_ANIM","_SEQ","_SPR",".SPR",".SEQ",".TIM",".CEL"]


def precompute(words):
    return [(w, *chs(w)) for w in words]


def combine(a, b):
    """compound: (ha,sha) followed by (hb,shb)."""
    ha, sha = a
    hb, shb = b
    return (ha ^ rotl(hb, sha)) & 0xFFFFFFFF, (sha + shb) & 31


def main():
    info = load_targets()
    targets = set(info)
    print(f"uncracked targets: {len(targets)} ({sum(1 for v in info.values() if v[0]=='sprite')} sprite)")

    out = AID / "brute_review_hits.csv"
    fo = out.open("w", newline="")
    w = csv.writer(fo)
    w.writerow(["id_hex", "model", "candidate", "type", "n_levels", "visual_role", "levels"])

    def emit(idv, model, name):
        t, lv, role = info[idv]
        nlv = len([x for x in lv.split(";") if x])
        w.writerow([f"0x{idv:08x}", model, name, t, nlv, role, lv[:60]])

    nhits = 0
    t0 = time.time()

    # -------- pass 1: rockyou dictionary --------
    try:
        words = set()
        with open(ROCK, encoding="latin-1") as f:
            for line in f:
                s = line.strip()
                if 3 <= len(s) <= 14 and s.isalpha():
                    words.add(s.upper())
        print(f"pass1 rockyou: {len(words):,} words")
        for s in words:
            h, _ = chs(s)
            if h in targets: emit(h, "RAW", s); nhits += 1
            wv = (SEED ^ rotl(h, 27)) & 0xFFFFFFFF
            if wv in targets: emit(wv, "WRAP", s); nhits += 1
        print(f"  after pass1: {nhits} hits ({time.time()-t0:.0f}s)")
    except FileNotFoundError:
        print("  rockyou not found, skipping pass1")

    # -------- pass 2: semantic grammar --------
    aff = precompute(AFFIX)
    sub = precompute(SUBJECT)
    st = precompute(STATE)
    dc = precompute(DECO)
    # precompute STATE x DECO tails
    tails = []  # (name_tail, h, sh)
    for sn, sh_, ssh in st:
        for dn, dh, dsh in dc:
            th, tsh = combine((sh_, ssh), (dh, dsh))
            tails.append((sn + dn, th, tsh))
    # precompute AFFIX x SUBJECT heads
    heads = []
    for an, ah, ash in aff:
        for subn, subh, subsh in sub:
            hh, hsh = combine((ah, ash), (subh, subsh))
            heads.append((an + subn, hh, hsh))
    print(f"pass2 grammar: {len(heads):,} heads x {len(tails):,} tails = {len(heads)*len(tails):,} names")

    rotl_l = rotl
    for hi, (hn, hh, hsh) in enumerate(heads):
        for tn, th, tsh in tails:
            full = (hh ^ rotl_l(th, hsh)) & 0xFFFFFFFF
            if full in targets:
                emit(full, "RAW", hn + tn); nhits += 1
            wv = (SEED ^ rotl_l(full, 27)) & 0xFFFFFFFF
            if wv in targets:
                emit(wv, "WRAP", hn + tn); nhits += 1
        if (hi + 1) % 500 == 0:
            print(f"  heads {hi+1}/{len(heads)}  hits={nhits}  ({time.time()-t0:.0f}s)")

    fo.close()
    print(f"\nDONE: {nhits} total hits in {time.time()-t0:.0f}s -> {out.relative_to(ROOT)}")
    print("Sort tip: labeled rows first ->  (awk -F, '$6!=\"\"') ; or open in a spreadsheet.")


if __name__ == "__main__":
    main()
