#!/usr/bin/env python3
"""Multi-token compound name attack on Skullmonkeys asset IDs.

The previous attack (name_attack.py) only tried 1- or 2-token compounds, missing
the common 3+ token "<character>_<part>_<action>_<direction>" pattern that the
user pointed out (e.g. HEAD_JOE_ATTACK_LEFT vs HEAD_JOE_ATTACK_RIGHT).

This script exploits the *cumulative* structure of calcHash to make N-token
search efficient:

    calcHash(A || B) = calcHash(A) XOR rotl(calcHash(B), shift_A)

    where shift_A = sum(char_value(c) for c in A) mod 32.

So a 4-token compound becomes a 2x2 meet-in-the-middle. We never have to
re-hash the same prefix; we precompute (hash, shift) per part and combine.

Output: docs/analysis/asset-identification/compound_hash_hits.csv
        with rows sorted by score (high = more likely to be a real asset name).

Run:
    python3 tools/scripts/compound_hash_attack.py
    python3 tools/scripts/compound_hash_attack.py --max-parts 4 --top 200
"""
from __future__ import annotations

import argparse
import csv
import itertools
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SEEDED_XOR_MASK = 0x28C0E011
SEEDED_OUTPUT_ROT = 27


# ---------------------------------------------------------------------------
# Hash primitives (mirror tools/klash.c exactly)
# ---------------------------------------------------------------------------


def char_value(ch: str):
    o = ord(ch)
    if "a" <= ch <= "z":
        o -= 32
    elif "0" <= ch <= "9":
        o += 22
    if ("A" <= ch <= "Z") or ("0" <= ch <= "9"):
        return (o - 64) & 31
    return None


def calc_hash_and_shift(text: str) -> tuple[int, int]:
    h = 0
    sh = 0
    for c in text:
        v = char_value(c)
        if v is None:
            continue
        sh = (sh + v) & 31
        h ^= 1 << sh
    return h, sh


def rotl(v: int, r: int) -> int:
    r &= 31
    return ((v << r) | (v >> ((32 - r) & 31))) & 0xFFFFFFFF


def rotr(v: int, r: int) -> int:
    return rotl(v, (-r) & 31)


def asset_id(name: str) -> int:
    h, _ = calc_hash_and_shift(name)
    return SEEDED_XOR_MASK ^ rotl(h, SEEDED_OUTPUT_ROT)


def asset_id_from_parts(parts) -> int:
    """Hash a concatenation of name parts; equivalent to asset_id(''.join(parts))."""
    h = 0
    sh = 0
    for p in parts:
        for c in p:
            v = char_value(c)
            if v is None:
                continue
            sh = (sh + v) & 31
            h ^= 1 << sh
    return SEEDED_XOR_MASK ^ rotl(h, SEEDED_OUTPUT_ROT)


# ---------------------------------------------------------------------------
# Vocabulary
# ---------------------------------------------------------------------------

# 4-letter level codes from the BLB header
LEVELS = [
    "MENU", "PHRO", "SCIE", "TMPL", "FINN", "MEGA", "BOIL", "SNOW",
    "FOOD", "HEAD", "BRG1", "GLID", "CAVE", "WEED", "EGGS", "GLEN",
    "CLOU", "SEVN", "SOAR", "CRYS", "CSTL", "WIZZ", "RUNN", "MOSS",
    "KLOG", "EVIL",
]

# Names lifted directly from the official manual / strategy guide.
# Source: docs/reference/official-enemy-names.md
CHARACTERS = [
    # Heroes
    "KLAYMEN", "KLAYMAN", "KLAY", "CLAY", "CLAYMAN", "HERO", "PLAYER", "MAIN",
    "WILLIE", "WILLY", "HOBORG", "TRIBE", "NEVERHOOD", "DOUG",
    # Bosses (manual names)
    "SHRINEY", "GUARD", "SHRINEYGUARD",
    "JOE", "HEAD", "JOEHEAD", "JOEHEADJOE", "JHJ",
    "GLENN", "GLEN", "YNTIS", "YNT",
    "MAGE", "WIZARD", "MONKEY", "MONKEYMAGE",
    "KLOGG", "KLOG", "KING", "MACHINE", "SKULL", "SKULLMACHINE",
    # Enemies (manual names)
    "CLAYKEEPER", "KEEPER",
    "LOUDMOUTH", "LOUD", "MOUTH",
    "MENTAL", "MENTALMONKEY",
    "TEMPEST", "PULSATING",
    "JUMPY", "GORILLA", "JUMPYGORILLA",
    "TRIPLE", "LASER", "TRIPLELASER",
    "BARFO", "ELBARFO", "BARF",
    "FLAPPER", "FLAP",
    "JX1137", "TESTPILOT", "PILOT", "ROCKETPACK",
    "BOMBER", "BOMB", "MONK",
    "ROYAL", "ROYALGUARD",
    "ROBOT", "HOVER", "ROBOTHOVER",
    "SHOOTER", "HEADSHOOTER", "FORKSHOOTER", "FORK",
    "SNOBLO", "SNO", "BLO",
    "EGGBEATER", "EGG", "BEATER", "PROPELLER",
    "CASTLETROOPER", "TROOPER", "CASTLE", "SPEAR",
    "PIPECLEANER", "PIPE", "CLEANER", "TONGUE", "WORM",
    "SCREAMING", "INFERNO", "SCREAMINGINFERNO",
    "POPCORN", "POPCORNSKULL",
    "WORKER", "WORKERYNT", "CENTURION", "SWARM",
    "BARKING", "BARKINGBIRD",
    # Player attacks/abilities
    "BULLET", "BULLETS", "GREENBULLET", "AMMO", "ENERGY", "ENERGYBALL",
    "PHOENIX", "PHOENIXHAND", "BIRD", "HAND", "SEEKER",
    "PHART", "FART", "PHARTHEAD", "FARTCLONE", "CLONE", "GAS",
    "UNIVERSE", "ENEMA", "SUPER", "SUPERPOWER", "UNIVERSEENEMA",
    "HALO", "SHIELD",
    "HAMSTER", "HAMSTERSHIELD",
    "GLIDEY", "YELLOWBIRD", "GLIDE",
    "SUPERWILLIE", "SPIN",
    "BUTT", "BOUNCE", "BUTTBOUNCE", "JUMP",
    # Other characters/things from the universe
    "FIFI", "MICHAEL", "DACENNA", "GORDO", "FETUS", "BABY", "IDOL", "DEMON",
    "DEVIL", "EVIL", "ENEMY", "ENEMIES",
    "MABIRD", "MA",
]

ITEMS = [
    "ONEUP", "1UP", "EXTRA", "EXTRALIFE", "LIFE", "LIVES", "HEAD", "KLAYHEAD", "KLAYMENHEAD",
    "SWIRLY", "SWIRLYQ", "SWIRLYQS",
    "ICON", "ICONS", "70S", "1970S", "1970SICON",
    "CHECKPOINT", "CHECKPNT",
    "TELEPORT", "TELEPORTBALL", "PORTAL", "WARP", "EXIT",
    "SPARKLE", "SPARKLES", "TWINKLE", "SPARK", "COLLECT", "PICKUP",
    "COIN", "GEM",
    "KEY", "DOOR", "BUTTON", "SWITCH", "LEVER", "CRATE", "BLOCK", "BARREL",
    "PLATFORM", "MOVINGPLATFORM", "MOVPLATFORM", "MOVPLAT", "PLAT", "PLATA", "PLATB", "PLATC",
    "LIFT", "ELEVATOR",
    "SPRING", "TRAMPOLINE", "BOUNCY",
    "SPIKE", "SPIKES", "SAW", "FAN", "WHEEL",
    "BALL", "BALLS", "CATCHBALL", "CATCHABLEBALL", "SPIKEBALL", "SPIKYBALL", "KLOGGBALL",
    "CLOCK", "CLOCKPLAT", "TIMER",
    "VIRTUAL", "VIRTUALPLAT", "SPOTLIGHT", "LIGHT",
    "TREASURE", "TREASUREBALL", "HIDDEN",
    "CLAYBALL", "ORB", "ORANGE",
    "GREENHEART", "YELLOW", "YELLOWCHEVRON", "CHEVRON", "SHRINK", "RESIZE",
    "FATTY", "FATTYBOOM", "BOOM",
    "TURRET", "GUN", "WEAPON",
]

ACTIONS = [
    "JUMP", "JUMPING", "JUMPED",
    "WALK", "WALKING", "WALKED",
    "RUN", "RUNNING", "RAN",
    "STAND", "STANDING", "IDLE", "REST", "WAIT",
    "DIE", "DEATH", "DEAD", "DYING",
    "HURT", "HIT", "DAMAGE", "PAIN", "OUCH",
    "SHOOT", "SHOOTING", "FIRE", "FIRED", "FIRING",
    "FALL", "FALLING", "FALLEN",
    "CLIMB", "CLIMBING",
    "SWIM", "SWIMMING",
    "PUSH", "PULL", "KICK", "PUNCH", "SLAP",
    "SPIT", "SPITTING", "HOCK",
    "BOUNCE", "BOUNCING",
    "FLY", "FLYING", "FLEW",
    "GLIDE", "GLIDING",
    "SPIN", "SPINNING",
    "ATTACK", "ATTACKING", "ATK",
    "THROW", "THROWING",
    "LAND", "LANDING", "LANDED",
    "CROUCH", "DUCK", "DUCKING",
    "SLIDE", "SLIDING",
    "TURN", "TURNING",
    "WAVE", "WAVING",
    "TAUNT", "VICTORY", "WIN", "LOSE", "CHEER",
    "YELL", "YELLING", "SCREAM", "SCREAMING",
    "SLAM", "SLAMMING", "SLAMMED",
    "POOP", "DANCE", "DANCING",
    "ROLL", "ROLLING",
    "FLOAT", "FLOATING",
    "BLOW", "BLOWING",
    "EAT", "EATING",
    "SLEEP", "SLEEPING",
    "WAKE", "WAKING",
    "RUN", "WALK", "JUMP", "FALL",
    "STOMP", "STAMP", "STEP",
    "ENTER", "EXIT",
    "OPEN", "CLOSE", "OPENING", "CLOSING",
    "START", "BEGIN", "INTRO", "END", "OUTRO", "DONE",
    "DRAW", "ERASE",
]

DIRECTIONS = [
    "LEFT", "RIGHT", "UP", "DOWN",
    "L", "R", "U", "D",
    "FRONT", "BACK", "FORWARD", "BACKWARD",
    "NORTH", "SOUTH", "EAST", "WEST",
    "TOP", "BOTTOM", "MIDDLE", "CENTER",
    "FACE", "REAR", "SIDE",
    "INNER", "OUTER", "INSIDE", "OUTSIDE",
    "ABOVE", "BELOW",
    "TOLEFT", "TORIGHT", "TOUP", "TODOWN",
    "FROMLEFT", "FROMRIGHT",
]

BODYPARTS = [
    "HEAD", "BODY", "TAIL", "WING", "WINGS", "HORN", "HORNS",
    "EYE", "EYES", "MOUTH", "TEETH", "TOOTH", "NOSE", "EAR", "EARS",
    "ARM", "ARMS", "LEG", "LEGS", "FOOT", "FEET", "HAND", "HANDS",
    "FINGER", "TOE", "NAIL",
    "HAIR", "BEARD", "BROW",
    "SKIN", "BONE", "SKULL",
    "STOMACH", "BELLY", "CHEST",
    "PART", "PARTS", "PIECE", "PIECES", "LIMB", "LIMBS",
    "TORSO", "WAIST", "HIP", "NECK",
]

FX = [
    "FX", "EFX", "SFX", "EFFECT",
    "BOOM", "POP", "POOF", "BURST", "EXPLODE", "EXPLOSION",
    "SPARKLE", "SPARK", "SPARKS",
    "SMOKE", "FLASH", "GLEAM",
    "SPLASH", "BLOOD", "GORE",
    "DUST", "PARTICLE", "PARTICLES",
    "SPLAT", "GOO", "SLIME", "MUCUS",
    "BUBBLE", "BUBBLES",
    "FIRE", "FLAME", "FLAMES", "BURN", "BURNING",
    "WATER", "SPRAY", "DRIP",
    "GLOW", "SHINE", "TWINKLE", "GLINT",
    "STAR", "STARS", "SHOCKWAVE",
    "RING", "WAVE", "WAVES",
    "TRAIL", "STREAK",
    "SHADOW", "SILHOUETTE",
    "SHARD", "DEBRIS", "FRAGMENT",
    "ICE", "FROST", "SNOW",
    "LIGHTNING", "BOLT", "ZAP",
    "FLOAT", "MIST", "FOG",
    "LASER", "BEAM", "RAY",
    "EJECT", "SPAWN",
]

UI = [
    "YES", "NO", "OK", "CANCEL", "BACK", "EXIT", "QUIT", "QUITGAME",
    "CONTINUE", "PAUSE", "PAUSED",
    "OPTIONS", "PASSWORD",
    "START", "BEGIN", "NEW", "NEWGAME", "LOAD", "SAVE", "LOADGAME", "SAVEGAME",
    "MENU", "MAIN", "MAINMENU",
    "SCORE", "LIVES", "LIFE", "GAMEOVER",
    "NUMBER", "NUMBERS", "NUM", "DIGIT", "DIGITS", "DIGITAL",
    "FONT", "TEXT", "LETTER", "LETTERS", "GLYPH", "CHAR",
    "CURSOR", "ARROW", "POINTER", "SELECTOR", "SELECT", "MARKER",
    "TITLE", "LOGO", "BANNER",
    "CREDITS", "DEMO", "PRESS", "ENTER",
    "EXTRA", "BONUS", "EXTRAS",
    "LEVEL", "STAGE", "WORLD", "MAP",
    "HUD", "METER", "BAR", "HEALTH", "ENERGY",
    "FRAME", "PANEL", "WINDOW", "BOX", "BORDER",
    "BACKGROUND", "BG", "BACKDROP",
    "ICON", "BUTTON",
    "DEFAULT", "EMPTY", "BLANK",
    "TIME", "TIMER",
    "RANK", "RANKING",
    "BONUS",
    "BACKBUTTON",
]

# Generic descriptors / colors / sizes that often appear in asset names.
DESCRIPTORS = [
    "BIG", "SMALL", "TINY", "GIANT", "LITTLE", "HUGE", "MINI", "MEGA",
    "RED", "BLUE", "GREEN", "YELLOW", "BLACK", "WHITE", "GOLD", "SILVER",
    "ORANGE", "PURPLE", "PINK", "BROWN", "GREY", "GRAY",
    "OLD", "NEW", "FAT", "THIN", "TALL", "SHORT",
    "GOOD", "BAD", "HAPPY", "SAD", "ANGRY", "MAD", "CRAZY", "SCARED", "SICK",
    "FAST", "SLOW", "HARD", "SOFT", "HOT", "COLD", "WARM", "COOL",
    "DARK", "LIGHT", "BRIGHT", "DIM",
    "LOW", "HIGH", "MID",
    "REGULAR", "NORMAL", "BASE", "BASIC", "STANDARD", "ALT", "VARIANT",
    "AUX", "SPECIAL", "RARE", "COMMON",
    "ROUGH", "SMOOTH",
    "LONG", "WIDE", "NARROW", "ROUND", "FLAT", "SQUARE",
    "STATIC", "ANIMATED",
    "TEMP", "PERM",
    "SHARED",
]

# Asset class prefixes/suffixes derived from the beta-binary debug strings:
# sprt, seq, sndEfx, tile, clut, tpage, xtile, event, Master, Overlay, Layers
ASSET_KINDS = [
    "SPR", "SPRT", "SPRITE",
    "SEQ", "SEQUENCE", "ANM", "ANIM", "ANIMATION",
    "SND", "SOUND", "SNDEFX", "SFX", "VAG",
    "TIM", "TIMG", "IMG", "IMAGE",
    "TILE", "TILES", "XTILE", "TILESET",
    "CLUT", "PAL", "PALETTE",
    "TPAGE", "PAGE",
    "EVENT", "EVT",
    "MASTER", "OVERLAY", "LAYER", "LAYERS", "SUB", "SUBLEVEL", "SUBLEVELS",
    "BACKGROUND", "BG", "FOREGROUND", "FG",
    "GFX", "DAT", "BIN", "RAW",
]

NUMERIC = [str(i) for i in range(10)] + [f"{i:02d}" for i in range(40)] + ["A", "B", "C", "D", "E"]


def build_vocab():
    """Return dict {token -> category}. Tokens are uppercase alnum."""
    cats = {
        "level": LEVELS,
        "char": CHARACTERS,
        "item": ITEMS,
        "act": ACTIONS,
        "dir": DIRECTIONS,
        "body": BODYPARTS,
        "fx": FX,
        "ui": UI,
        "desc": DESCRIPTORS,
        "kind": ASSET_KINDS,
        "num": NUMERIC,
    }
    vocab = {}
    for cat, words in cats.items():
        for w in words:
            t = "".join(c for c in w if c.isalnum()).upper()
            if t:
                vocab.setdefault(t, cat)
    return vocab


# ---------------------------------------------------------------------------
# Target loading
# ---------------------------------------------------------------------------


def load_targets():
    """id -> {'kinds','levels','role','sources'} from asset-ids-master + identification CSV."""
    info: dict[int, dict[str, str]] = {}
    master = ROOT / "docs/reference/asset-ids-master.csv"
    with master.open() as f:
        next(f)
        for line in f:
            c = line.rstrip("\n").split(",")
            if len(c) >= 5:
                info[int(c[1])] = {
                    "kinds": c[2],
                    "popcount": c[3],
                    "sources": c[4],
                    "levels": "",
                    "segments": "",
                    "role": "",
                }
    ident = ROOT / "docs/analysis/asset-identification/sprite_identification_template.csv"
    if ident.exists():
        with ident.open(newline="") as f:
            for row in csv.DictReader(f):
                try:
                    i = int(row["sprite_decimal"])
                except Exception:
                    continue
                if i in info:
                    info[i]["levels"] = row.get("levels", "")
                    info[i]["segments"] = row.get("segments", "")
                    info[i]["role"] = row.get("human_role", "")
    return info


# ---------------------------------------------------------------------------
# Compound enumeration
# ---------------------------------------------------------------------------


def precompute_part_table(vocab):
    """For each vocab token, compute (calc_hash, shift)."""
    return {w: calc_hash_and_shift(w) for w in vocab}


def part_hash_and_shift_of_join(parts, table):
    h = 0
    sh = 0
    for p in parts:
        ph, psh = table[p]
        h ^= rotl(ph, sh)
        sh = (sh + psh) & 31
    return h, sh


def enumerate_grammar(structure_categories, vocab):
    """Yield tuples of vocab tokens matching the category sequence."""
    cat_to_words = {c: [w for w, k in vocab.items() if k == c] for c in set(structure_categories)}
    pools = [cat_to_words[c] for c in structure_categories]
    for combo in itertools.product(*pools):
        yield combo


# ---------------------------------------------------------------------------
# Main attack passes
# ---------------------------------------------------------------------------


def collect_hits(target_set, generators, table, max_hits_per_id=20):
    """For each candidate (list of name parts), check if asset_id matches a target."""
    hits = {}  # id -> set of (joined_name, structure_label)
    seen_names = set()
    count = 0
    for structure_label, gen in generators:
        for parts in gen:
            joined = "".join(parts)
            if joined in seen_names:
                continue
            seen_names.add(joined)
            count += 1
            h, _ = part_hash_and_shift_of_join(parts, table)
            asset = SEEDED_XOR_MASK ^ rotl(h, SEEDED_OUTPUT_ROT)
            if asset in target_set:
                slot = hits.setdefault(asset, [])
                if len(slot) < max_hits_per_id:
                    slot.append((joined, structure_label, parts))
    return hits, count


def score_hit(parts, structure_label, asset_id_val, vocab, target_info):
    """Heuristic score: high for plausible single/dual-word names with role
    corroboration and reasonable length."""
    info = target_info.get(asset_id_val, {})
    role = info.get("role", "").upper()
    levels = info.get("levels", "").upper()
    sources = info.get("sources", "")
    kinds = info.get("kinds", "")

    cats = [vocab[p] for p in parts]
    n = len(parts)
    score = 0.0

    # Base: prefer compact 1-3 part names
    if n == 1:
        score += 40
    elif n == 2:
        score += 30
    elif n == 3:
        score += 20
    elif n == 4:
        score += 10

    # Avoid "level-only" or "kind-only" multi-token results (likely junk)
    real_cats = [c for c in cats if c not in ("kind", "num", "desc", "dir")]
    if not real_cats:
        score -= 30

    # Strong role corroboration: any meaningful token appears in the role label
    if role:
        for p, cat in zip(parts, cats):
            if cat in ("char", "item", "act", "ui", "fx", "body") and p in role:
                score += 60
                break

    # Level scoping: a level token in the name must match the asset's levels
    for p, cat in zip(parts, cats):
        if cat == "level":
            if p in levels:
                score += 30
            elif levels:
                score -= 25

    # Single bare token (e.g. "PAUSED") is usually the best signal
    if n == 1 and cats[0] in ("ui", "char", "item", "act"):
        score += 30

    # Avoid degenerate all-numeric/all-direction names
    if all(c in ("num", "desc", "dir") for c in cats):
        score -= 40

    # Penalize very long composite names
    eff_len = sum(len(p) for p in parts)
    if eff_len > 20:
        score -= (eff_len - 20)

    # Sources hint
    if "sles" in sources and "demo" not in sources:
        # Asset only in retail (not demo) — could be late-game name
        score += 2

    # Kind hint
    if "audio" in kinds and any(p in {"VAG", "SND", "SOUND", "SFX"} for p in parts):
        score += 20
    if "sprite" in kinds and any(p in {"SPR", "SPRITE", "SPRT", "ANM", "ANIM"} for p in parts):
        score += 10
    return score


def make_generators(vocab, max_parts):
    """Build the structure grammars we'll enumerate."""
    cats = list({c for c in vocab.values()})
    # We want to focus on combinations that resemble real asset names.
    # Order: most likely first.
    grammars = []

    if max_parts >= 1:
        grammars.append(("char", [["char"]]))
        grammars.append(("item", [["item"]]))
        grammars.append(("ui", [["ui"]]))
        grammars.append(("act", [["act"]]))
        grammars.append(("fx", [["fx"]]))
        grammars.append(("level", [["level"]]))
        grammars.append(("body", [["body"]]))

    if max_parts >= 2:
        struct2 = [
            ["char", "act"],
            ["char", "body"],
            ["char", "dir"],
            ["char", "fx"],
            ["char", "num"],
            ["item", "num"],
            ["item", "dir"],
            ["item", "desc"],
            ["item", "act"],
            ["act", "dir"],
            ["body", "act"],
            ["body", "dir"],
            ["fx", "num"],
            ["fx", "dir"],
            ["level", "char"],
            ["level", "item"],
            ["level", "act"],
            ["ui", "num"],
            ["ui", "act"],
            ["desc", "char"],
            ["desc", "item"],
            ["kind", "char"],
            ["kind", "item"],
            ["kind", "fx"],
            ["kind", "level"],
            ["char", "kind"],
            ["item", "kind"],
            ["fx", "kind"],
        ]
        grammars.append(("2-tok", struct2))

    if max_parts >= 3:
        struct3 = [
            ["char", "act", "dir"],
            ["char", "body", "act"],
            ["char", "body", "dir"],
            ["char", "act", "num"],
            ["char", "act", "fx"],
            ["char", "dir", "num"],
            ["char", "body", "num"],
            ["level", "char", "act"],
            ["level", "char", "dir"],
            ["level", "item", "num"],
            ["item", "dir", "num"],
            ["item", "act", "dir"],
            ["body", "act", "dir"],
            ["body", "act", "num"],
            ["body", "dir", "num"],
            ["kind", "char", "act"],
            ["kind", "char", "dir"],
            ["kind", "level", "char"],
            ["kind", "level", "item"],
        ]
        grammars.append(("3-tok", struct3))

    if max_parts >= 4:
        struct4 = [
            ["char", "body", "act", "dir"],
            ["char", "act", "dir", "num"],
            ["char", "body", "act", "num"],
            ["level", "char", "act", "dir"],
            ["level", "char", "act", "num"],
            ["kind", "char", "act", "dir"],
            ["kind", "char", "body", "act"],
        ]
        grammars.append(("4-tok", struct4))

    out = []
    for label, structs in grammars:
        for struct in structs:
            out.append((f"{label}:{'|'.join(struct)}",
                        enumerate_grammar(struct, vocab)))
    return out


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--max-parts", type=int, default=4)
    ap.add_argument("--top", type=int, default=400, help="rows in summary preview")
    ap.add_argument("--out", default=str(ROOT / "docs/analysis/asset-identification/compound_hash_hits.csv"))
    ap.add_argument("--targets-out", default=str(ROOT / "docs/analysis/asset-identification/compound_hash_by_target.csv"))
    args = ap.parse_args()

    vocab = build_vocab()
    table = precompute_part_table(vocab)
    print(f"vocabulary: {len(vocab)} unique tokens")

    target_info = load_targets()
    target_set = set(target_info.keys())
    print(f"targets: {len(target_set)} unique asset ids")

    # Always include the 6 confirmed text anchors as sanity tests.
    sanity = {
        "NO": 0x29C0E211,
        "YES": 0x2AD0F011,
        "PAUSED": 0x0AD0F813,
        "QUIT": 0x68C0F413,
        "CONTINUE": 0x69C04050,
        "QUITGAME": 0x69C8F473,
    }
    for name, expect in sanity.items():
        got = asset_id(name)
        assert got == expect, f"calcHash regression on {name}: 0x{got:08x} != 0x{expect:08x}"
    print("sanity: 6 text anchors hash correctly")

    generators = make_generators(vocab, args.max_parts)
    print(f"running {len(generators)} grammar passes…")

    t0 = time.time()
    hits, total = collect_hits(target_set, generators, table)
    elapsed = time.time() - t0
    print(f"enumerated {total:,} unique candidate names in {elapsed:.1f}s")
    print(f"hit {len(hits)} unique target ids")

    # Score all (id, candidate) pairs
    rows = []
    for asset_id_val, slot in hits.items():
        info = target_info[asset_id_val]
        for joined, structure_label, parts in slot:
            score = score_hit(parts, structure_label, asset_id_val, vocab, target_info)
            rows.append({
                "score": round(score, 1),
                "id_hex": f"0x{asset_id_val:08x}",
                "candidate": joined,
                "parts": "|".join(parts),
                "structure": structure_label,
                "n_parts": len(parts),
                "role": info.get("role", ""),
                "levels": info.get("levels", ""),
                "kinds": info.get("kinds", ""),
                "sources": info.get("sources", ""),
            })
    rows.sort(key=lambda r: (-r["score"], r["id_hex"], r["candidate"]))

    out = Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    with out.open("w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0].keys()) if rows else
                           ["score", "id_hex", "candidate", "parts", "structure",
                            "n_parts", "role", "levels", "kinds", "sources"])
        w.writeheader()
        for r in rows:
            w.writerow(r)

    # Also write a per-target summary with the top candidate per id, useful as
    # the "if I had to bet, this is the name" cheat sheet.
    by_target: dict[int, dict] = {}
    for r in rows:
        idv = int(r["id_hex"], 16)
        cur = by_target.get(idv)
        if cur is None or r["score"] > cur["score"]:
            by_target[idv] = r

    target_out = Path(args.targets_out)
    target_out.parent.mkdir(parents=True, exist_ok=True)
    with target_out.open("w", newline="") as f:
        fields = ["id_hex", "score", "candidate", "structure", "role", "levels", "kinds", "sources"]
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        for idv in sorted(by_target):
            r = by_target[idv]
            w.writerow({k: r.get(k, "") for k in fields})

    print(f"wrote {out.relative_to(ROOT)} ({len(rows)} rows)")
    print(f"wrote {target_out.relative_to(ROOT)} ({len(by_target)} target ids hit)")

    # Preview the most promising candidates
    print("\n=== TOP CANDIDATES ===")
    print(f"{'score':>6}  {'id':10}  {'name':28}  {'structure':30}  role / levels")
    role_hits = 0
    for r in rows[:args.top]:
        marker = " ★" if r["role"] and any(p.upper() in r["role"].upper()
                                            for p in r["parts"].split("|")) else "  "
        print(f"{r['score']:6.1f}  {r['id_hex']}  {r['candidate'][:28]:28}  "
              f"{r['structure'][:30]:30}  {marker}{r['role'][:30]}  {r['levels'][:30]}")
        if r["role"]:
            role_hits += 1
    print(f"\n{role_hits}/{min(args.top, len(rows))} top hits have a visual role label")


if __name__ == "__main__":
    main()
