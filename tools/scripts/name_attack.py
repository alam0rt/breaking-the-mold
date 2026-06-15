#!/usr/bin/env python3
"""Dictionary/vocabulary attack on Skullmonkeys asset ids.

Brute force is useless here (the 32-bit hash is collision-dense: ~0.43% of all
4-letter strings collide with any given id). So instead of enumerating raw
strings we enumerate *plausible names* built from a curated vocabulary, hash
them with the C tool (tools/klash), and keep the ones that hit a real id.

Every hit is human-readable by construction; we then SCORE each hit and, most
usefully, flag hits whose word appears in the asset's known visual-role hint
(e.g. "MONKEY" landing on an id documented as a monkey enemy).

    gcc -O3 -o /tmp/klash tools/klash.c
    python3 tools/scripts/name_attack.py            # uses /tmp/klash
    python3 tools/scripts/name_attack.py --klash ./klash --max-tokens 2
"""
from __future__ import annotations
import argparse, csv, subprocess, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SEED = 0x28C0E011

# ---------------------------------------------------------------- vocabulary
LEVEL_CODES = "MENU PHRO SCIE TMPL FINN MEGA BOIL SNOW FOOD HEAD BRG1 GLID CAVE WEED EGGS GLEN CLOU SEVN SOAR CRYS CSTL WIZZ RUNN MOSS KLOG EVIL".split()

CHARACTERS = """KLAYMEN KLAYMAN KLAY CLAY CLAYMAN MONKEY MONKEYS SKULLMONKEY SKULLMONKEYS
KLOGG KLOG GLENN GLEN JOE WILLIE WILLY HOBORG TRIBE NEVERHOOD HERO PLAYER MAIN
PHART FART PHARTHEAD FARTHEAD BIRD PHOENIX HAND WIZARD MAGE MONKEYMAGE BOSS
SHRINEY SHRINEYGUARD GUARD SCREAMER IMP DEMON DEVIL FETUS BABY GORDO IDOL ENEMY
JOEHEAD FIFI MICHAEL DACENNA""".split()

ITEMS = """BULLET BULLETS GREENBULLET AMMO ONEUP EXTRALIFE LIFE LIVES ENO HALO BIRDIE
BIRDIES SWIRLY POWERUP CHECKPOINT IDOL FETUS HEAD KLAYHEAD SPARKLE COIN GEM ICON
KEY DOOR PLATFORM MOVINGPLATFORM LIFT SPRING TRAMPOLINE SPIKE SPIKES SAW FAN
WHEEL BALL CATCHBALL BUTTON SWITCH LEVER CRATE BLOCK BARREL FATTYBOOM""".split()

ACTIONS = """JUMP JUMPING WALK WALKING RUN RUNNING STAND STANDING IDLE DIE DEATH DEAD
HURT HIT SHOOT SHOOTING FIRE FALL FALLING CLIMB CLIMBING SWIM PUSH PULL KICK PUNCH
SPIT BOUNCE FLY FLYING GLIDE GLIDING SPIN ATTACK THROW LAND CROUCH DUCK SLIDE
TURN WAVE TAUNT VICTORY WIN LOSE CHEER YELL SLAM SCREAM POOP DANCE""".split()

FX = """BOOM POP POOF BURST EXPLODE EXPLOSION SPARKLE SPARK SMOKE FLASH SPLASH BLOOD
DUST PARTICLE SPLAT GOO SLIME BUBBLE FIRE FLAME WATER SPRAY GLOW SHINE TWINKLE
STAR SHOCKWAVE RING WAVE TRAIL""".split()

UI = """YES NO QUIT QUITGAME CONTINUE PAUSE PAUSED OPTIONS PASSWORD START NEWGAME
LOADGAME SAVEGAME MENU SCORE LIVES GAMEOVER NUMBERS NUMBER NUM DIGIT DIGITS FONT
CURSOR ARROW POINTER SELECT BACK EXIT TITLE LOGO CREDITS DEMO PRESS ENTER OK
EXTRA BONUS LEVEL STAGE WORLD MAP HUD METER BAR HEALTH""".split()

# generic common words worth trying (kept small to limit collision noise)
COMMON = """BIG SMALL TINY GIANT LITTLE RED BLUE GREEN YELLOW BLACK WHITE GOLD SILVER
UP DOWN LEFT RIGHT TOP BOTTOM FRONT BACK INNER OUTER OLD NEW FAT THIN TALL SHORT
GOOD BAD EVIL HAPPY SAD ANGRY MAD CRAZY SCARED SICK FAST SLOW HARD SOFT HOT COLD
EYE EYES MOUTH TEETH TOOTH NOSE EAR ARM LEG FOOT HAND HEAD BODY TAIL WING HORN
ROCK STONE TREE WOOD FLOWER GRASS CLOUD RAIN FIRE ICE LAVA MUD SAND BONE SKULL
EGG WORM BUG FLY ANT BEE FISH FROG SNAKE RAT BAT CROW""".split()

PREFIXES = ["", "S", "SPR", "SEQ", "SND", "SFX", "FX", "OBJ", "ENT", "N2", "SM", "SK"]
SEPS = ["", "_"]
EXTS = ["", "SPR", "SEQ", "SND", "VAG", "TIM", "ANM"]


def build_vocab(extra_files=None, role_words=None):
    cats = {"char": CHARACTERS, "item": ITEMS, "act": ACTIONS, "fx": FX,
            "ui": UI, "level": LEVEL_CODES, "common": COMMON}
    vocab = {}
    for cat, words in cats.items():
        for w in words:
            vocab.setdefault(w.upper(), cat)
    # pull meaningful words out of crowd-sourced visual-role labels
    for w in (role_words or []):
        vocab.setdefault(w.upper(), "common")
    # external wordlist(s): one word per line
    for fp in (extra_files or []):
        for line in Path(fp).read_text(errors="ignore").splitlines():
            w = "".join(ch for ch in line.strip() if ch.isalnum()).upper()
            if 3 <= len(w) <= 14:
                vocab.setdefault(w, "common")
    return vocab


def candidates(vocab, max_tokens):
    """Yield unique candidate strings (uppercase, alnum joins)."""
    toks = list(vocab)
    seen = set()
    # single token, with optional asset-class prefix / extension
    for t in toks:
        for p in PREFIXES:
            for s in SEPS:
                stem = (p + s + t) if p else t
                for e in EXTS:
                    name = (stem + s + e) if e else stem
                    if name not in seen:
                        seen.add(name); yield name
    if max_tokens >= 2:
        # two-token compounds (the common "<thing><action>" naming pattern)
        important = [t for t in toks if vocab[t] in ("char", "item", "level", "ui")]
        seconds = [t for t in toks if vocab[t] in ("act", "fx", "item", "common", "ui")]
        for a in important:
            for b in seconds:
                if a == b:
                    continue
                for s in SEPS:
                    name = a + s + b
                    if name not in seen:
                        seen.add(name); yield name


def hash_with_klash(names, klash):
    """Hash all names via the C tool; return list of ints aligned to names."""
    proc = subprocess.run([klash], input="\n".join(names) + "\n",
                          capture_output=True, text=True)
    out = proc.stdout.split()
    if len(out) != len(names):
        sys.exit(f"klash returned {len(out)} hashes for {len(names)} names")
    return [int(x, 16) for x in out]


def load_targets():
    """id -> {'kinds','levels','role'} from the master + identification CSVs."""
    info = {}
    master = ROOT / "docs/reference/asset-ids-master.csv"
    with master.open() as f:
        next(f)
        for line in f:
            c = line.rstrip("\n").split(",")
            if len(c) >= 3:
                info[int(c[1])] = {"kinds": c[2], "levels": "", "role": ""}
    ident = ROOT / "docs/analysis/asset-identification/sprite_identification_template.csv"
    if ident.exists():
        with ident.open(newline="") as f:
            for row in csv.DictReader(f):
                try: i = int(row["sprite_decimal"])
                except Exception: continue
                if i in info:
                    info[i]["levels"] = row.get("levels", "")
                    info[i]["role"] = row.get("human_role", "")
    return info


def popcount(v): return bin(v & 0xFFFFFFFF).count("1")
def eff_len(s): return sum(ch.isalnum() for ch in s)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--klash", default="/tmp/klash", help="path to compiled klash")
    ap.add_argument("--max-tokens", type=int, default=2)
    ap.add_argument("--words", nargs="*", default=[], help="extra wordlist file(s), one word per line")
    ap.add_argument("--use-roles", action="store_true", help="add words from crowd-sourced role labels")
    ap.add_argument("--out", default=str(ROOT / "docs/analysis/asset-identification/name_attack_hits.csv"))
    args = ap.parse_args()

    if not Path(args.klash).exists():
        sys.exit(f"{args.klash} not found — build it: gcc -O3 -o /tmp/klash tools/klash.c")

    role_words = []
    if args.use_roles:
        for info in load_targets().values():
            for tok in info["role"].replace(":", " ").split():
                t = "".join(ch for ch in tok if ch.isalnum())
                if len(t) >= 3:
                    role_words.append(t)
    vocab = build_vocab(args.words, role_words)
    names = list(candidates(vocab, args.max_tokens))
    print(f"vocabulary {len(vocab)} words -> {len(names):,} candidate names")

    hashes = hash_with_klash(names, args.klash)
    targets = load_targets()
    target_ids = set(targets)

    # collect hits: id -> list of candidate names
    hits = {}
    for name, h in zip(names, hashes):
        if h in target_ids:
            hits.setdefault(h, []).append(name)

    def core_words(name):
        # split on separators / known affixes to find the meaningful token(s)
        parts = name.replace("_", " ").split()
        return [p for p in parts if p in vocab]

    rows = []
    for idv, cands in hits.items():
        info = targets[idv]
        floor = popcount(idv ^ SEED)
        role = info["role"].upper()
        for c in cands:
            words = core_words(c)
            corrob = any(w in role for w in words) if role else False
            score = 0
            score += 100 if len(words) == 1 and c == words[0] else 0   # bare single dict word
            score += 40 if len(words) == 1 else 0
            score += 30 if words and all(vocab[w] != "common" for w in words) else 0
            score += 60 if corrob else 0
            score += 10 if eff_len(c) == floor else 0                  # no internal collision
            score -= 5 * (len(c) - eff_len(c))                         # penalize affixes/seps
            rows.append((score, corrob, idv, c, floor, eff_len(c), info["role"], info["levels"]))

    rows.sort(key=lambda r: (-r[0], r[2]))

    out = Path(args.out)
    with out.open("w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["score", "role_corroborated", "id_hex", "candidate", "floor", "eff_len", "role", "levels"])
        for sc, cor, idv, c, fl, el, role, lv in rows:
            w.writerow([sc, int(cor), f"0x{idv:08x}", c, fl, el, role, lv])

    print(f"\n{len(hits)} ids hit by {len(rows)} candidate names; wrote {out.relative_to(ROOT)}\n")
    print("=== role-corroborated hits (strongest) ===")
    shown = 0
    for sc, cor, idv, c, fl, el, role, lv in rows:
        if cor:
            print(f"  ★ 0x{idv:08x}  {c:22s}  (role: {role})")
            shown += 1
    if not shown:
        print("  (none)")
    print("\n=== top bare single-word hits ===")
    for sc, cor, idv, c, fl, el, role, lv in rows[:40]:
        if not cor:
            tag = f"role:{role}" if role else f"levels:{lv[:30]}"
            print(f"  0x{idv:08x}  {c:22s}  floor={fl} {tag}")


if __name__ == "__main__":
    main()
