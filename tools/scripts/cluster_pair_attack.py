#!/usr/bin/env python3
"""Cluster-pair bidirectional attack on Skullmonkeys asset IDs.

For each cluster (component of IDs linked by a single suffix-swap rotation
class), enumerate plausible suffix pairs (S_a, S_b) from a rich vocabulary,
compute the required prefix shift and prefix hash, then search a prefix
vocabulary for any word P whose calcHash AND end-shift match. A match
recovers BOTH IDs at once (P+S_a and P+S_b).

This is much stronger than blind dictionary attacks because:
  1. The class (suffix-pair XOR delta) is mathematically locked.
  2. The required prefix hash is fully determined by S_a and shift_P.
  3. Validation requires multiple cohort members to crack with same prefix.

Math
----
For full_name = PREFIX + SUFFIX:
  h(full)  = h(P) XOR rotl(h(S), sh_P)
  asset_id = SEED XOR rotl(h(full), 27)

For pair (A, B) sharing prefix P:
  h(A) XOR h(B) = rotl(h(S_a) XOR h(S_b), sh_P)
  cluster_class_normalized = rotr(asset_id_a ^ asset_id_b, 27) up to rotation

Given cluster delta D = rotr(asset_id_a ^ asset_id_b, 27) and hypothesized
(S_a, S_b):
  sh_P = the unique r such that rotl(h(S_a) ^ h(S_b), r) == D    (often 1 r exists)
  h_P  = rotr(asset_id_a ^ SEED, 27) XOR rotl(h(S_a), sh_P)

Then search prefix vocab for any P with calc_hash_and_shift(P) == (h_P, sh_P).

Run:
    python3 tools/scripts/cluster_pair_attack.py
    python3 tools/scripts/cluster_pair_attack.py --max-cohort 30 --top 50
"""
from __future__ import annotations
import argparse
import csv
import itertools
import sys
import time
from collections import defaultdict
from pathlib import Path

sys.path.insert(0, "tools/scripts")
from compound_hash_attack import calc_hash_and_shift, rotl, rotr  # type: ignore

ROOT = Path(__file__).resolve().parents[2]
SEED = 0x28C0E011
OUT_ROT = 27


# =========================================================================
# Vocabulary: prefixes (subjects) and suffixes (variants)
# =========================================================================

# Subjects: characters / objects / things — likely PREFIX in a compound name
SUBJECTS = [
    # Player / hero
    "KLAYMEN", "KLAY", "CLAY", "CLAYMAN", "PLAYER", "HERO", "MAIN", "WILLIE",
    "WILLY", "DOUG", "HOBORG", "TRIBE",
    # Skullmonkeys-specific
    "SKULL", "MONKEY", "SKULLMONKEY", "SKULLMONKEYS", "MAGE", "SHRINEY",
    "SHRINEYGUARD", "GUARD", "JOE", "JOEHEAD", "JOEHEADJOE", "JHJ", "GLENN",
    "GLEN", "YNTIS", "YNT", "WIZZ", "WIZARD", "MONKEYMAGE", "KLOGG", "KLOG",
    "KING", "MACHINE", "SKULLMACHINE",
    # Enemies (manual)
    "CLAYKEEPER", "KEEPER", "LOUDMOUTH", "LOUD", "MOUTH", "MENTAL",
    "MENTALMONKEY", "TEMPEST", "PULSATING", "JUMPY", "GORILLA", "JUMPYGORILLA",
    "TRIPLE", "LASER", "TRIPLELASER", "BARFO", "ELBARFO", "BARF", "FLAPPER",
    "FLAP", "JX1137", "TESTPILOT", "PILOT", "ROCKETPACK", "BOMBER", "BOMB",
    "MONK", "ROYAL", "ROYALGUARD", "ROBOT", "HOVER", "ROBOTHOVER", "SHOOTER",
    "HEADSHOOTER", "FORKSHOOTER", "FORK", "SNOBLO", "SNO", "BLO", "EGGBEATER",
    "EGG", "BEATER", "PROPELLER", "CASTLETROOPER", "TROOPER", "CASTLE", "SPEAR",
    "PIPECLEANER", "PIPE", "CLEANER", "TONGUE", "WORM", "SCREAMING",
    "INFERNO", "POPCORN", "POPCORNSKULL", "WORKER", "WORKERYNT", "CENTURION",
    "SWARM", "BARKING", "BARKINGBIRD",
    # Items
    "BULLET", "BULLETS", "GREENBULLET", "GREENBULLETS", "ENERGY", "ENERGYBALL",
    "AMMO", "PHOENIX", "PHOENIXHAND", "BIRD", "HAND", "SEEKER", "PHART", "FART",
    "PHARTHEAD", "FARTCLONE", "CLONE", "GAS", "UNIVERSE", "ENEMA", "SUPER",
    "SUPERPOWER", "UNIVERSEENEMA", "HALO", "SHIELD", "HAMSTER", "HAMSTERSHIELD",
    "GLIDEY", "YELLOWBIRD", "GLIDE", "SUPERWILLIE", "SPIN", "BUTT", "BOUNCE",
    "BUTTBOUNCE", "JUMP",
    # Other
    "FIFI", "MICHAEL", "DACENNA", "GORDO", "FETUS", "BABY", "IDOL", "DEMON",
    "DEVIL", "EVIL", "ENEMY", "ENEMIES", "MABIRD", "MA",
    # Objects
    "PLATFORM", "MOVPLATFORM", "MOVINGPLATFORM", "MOVPLAT", "PLAT", "LIFT",
    "CLOCK", "CLOCKPLAT", "VIRTUAL", "VIRTUALPLAT",
    "BALL", "BALLS", "CATCHBALL", "SPIKEBALL", "SPIKYBALL", "KLOGGBALL",
    "TREASURE", "TREASUREBALL", "TELEPORT", "TELEPORTBALL", "PORTAL", "WARP",
    "EXIT", "DOOR", "KEY", "BUTTON", "SWITCH", "LEVER", "CRATE", "BLOCK",
    "BARREL", "SPRING", "TRAMPOLINE", "BOUNCY", "SPIKE", "SPIKES", "SAW",
    "FAN", "WHEEL", "TURRET", "GUN",
    "ONEUP", "1UP", "EXTRALIFE", "LIFE", "EXTRALIFEHEAD", "KLAYHEAD",
    "KLAYMENHEAD", "SWIRLY", "SWIRLYQ", "ICON", "1970S", "1970",
    "CHECKPOINT", "CHECKPNT", "SPARKLE", "SPARK", "TWINKLE",
    "GREENHEART", "YELLOW", "YELLOWCHEVRON", "CHEVRON",
    "FATTY", "FATTYBOOM",
    "ORB", "ORANGE", "GEM",
    # Effects / particles
    "DEBRIS", "PARTICLE", "PARTICLES", "EXPLOSION", "EXPLODE", "POOF", "BURST",
    "POP", "SMOKE", "FLASH", "DUST", "SPLASH", "GORE", "BLOOD", "GOO", "SLIME",
    "MUCUS", "BUBBLE", "FIRE", "FLAME", "WATER", "GLOW", "SHINE", "GLINT",
    "STAR", "RING", "WAVE", "TRAIL", "STREAK", "SHARD", "FRAGMENT", "ICE",
    "FROST", "LIGHTNING", "BOLT", "ZAP", "MIST", "FOG", "BEAM", "RAY", "SPAWN",
    # UI
    "MENU", "OPTIONS", "TITLE", "LOGO", "FONT", "TEXT", "DIGIT", "DIGITS",
    "NUMBER", "NUMBERS", "GLYPH", "CURSOR", "POINTER", "ARROW", "SELECTOR",
    "CREDIT", "CREDITS", "PASSWORD", "INTRO", "OUTRO", "ENDING", "GAMEOVER",
    "GAME", "DEMO", "LOADING", "LOAD", "PRESS", "START", "PASS",
    "BG", "BACKGROUND", "BACKDROP", "FRAME", "BORDER", "BANNER", "PANEL",
    # Body parts
    "HEAD", "BODY", "TAIL", "WING", "WINGS", "HORN", "HORNS", "EYE", "EYES",
    "MOUTH", "TEETH", "TOOTH", "NOSE", "EAR", "ARM", "ARMS", "LEG", "LEGS",
    "FOOT", "FEET", "FINGER", "TOE", "HAIR", "BEARD", "SKIN", "BONE", "SKULL",
    "STOMACH", "BELLY", "CHEST", "TORSO", "WAIST", "HIP", "NECK", "BUTT",
    "BACK", "FRONT", "PIECE", "PIECES", "LIMB", "PART", "PARTS",
    # Levels
    "MENU", "PHRO", "SCIE", "TMPL", "FINN", "MEGA", "BOIL", "SNOW", "FOOD",
    "HEAD", "BRG1", "GLID", "CAVE", "WEED", "EGGS", "GLEN", "CLOU", "SEVN",
    "SOAR", "CRYS", "CSTL", "WIZZ", "RUNN", "MOSS", "KLOG", "EVIL",
]

# Suffixes: variants — frame/state/direction/numbering
SUFFIX_VARIANTS = [
    # Frame numbers
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
    "00", "01", "02", "03", "04", "05", "06", "07", "08", "09",
    "10", "11", "12", "13", "14", "15", "16", "17", "18", "19",
    "20", "21", "22", "23", "24", "25", "26", "27", "28", "29",
    "30", "31", "32", "33", "34", "35", "36", "37", "38", "39",
    # Versions
    "A", "B", "C", "D", "E", "F", "G", "H",
    # Directions
    "L", "R", "U", "D", "N", "S", "E", "W",
    "LEFT", "RIGHT", "UP", "DOWN", "FRONT", "BACK", "REAR", "SIDE",
    "NORTH", "SOUTH", "EAST", "WEST",
    "TOP", "BOTTOM", "MIDDLE", "CENTER", "INNER", "OUTER",
    # Action states
    "STAND", "STANDING", "IDLE", "REST", "WAIT",
    "WALK", "WALKING", "RUN", "RUNNING", "JUMP", "JUMPING",
    "FALL", "FALLING", "LAND", "LANDING",
    "DIE", "DEATH", "DEAD", "DYING",
    "HURT", "HIT", "DAMAGE", "PAIN",
    "SHOOT", "FIRE", "FIRING",
    "ATTACK", "ATK", "ATTACKING",
    "SLAM", "SLAMMING", "BOUNCE", "BOUNCING",
    "OPEN", "OPENING", "CLOSE", "CLOSING",
    "START", "BEGIN", "INTRO", "END", "OUTRO",
    "ENTER", "EXIT", "TURN", "TURNING",
    "EAT", "EATING", "SLEEP", "SLEEPING", "WAKE",
    "ROLL", "ROLLING", "SLIDE", "SLIDING",
    "FLY", "FLYING", "GLIDE", "GLIDING",
    "SPIN", "SPINNING",
    "SPIT", "SPITTING", "HOCK", "HOCKING", "LOOGEY",
    "YELL", "YELLING", "SCREAM", "SCREAMING",
    "TAUNT", "VICTORY", "WIN", "LOSE", "DANCE",
    # Animation phases
    "INIT", "ANIM", "ANIMATION", "ANIM00", "ANIM01",
    # Common modifiers
    "MIRROR", "FLIP", "ROT", "ROTATE", "SCALE",
    "NEW", "OLD", "ALT", "VARIANT", "VAR", "BASE",
    "BIG", "SMALL", "TINY", "LARGE",
    "FX", "SFX", "EFX",
    "PART", "PARTS", "PIECE", "DEBRIS",
    "MAIN", "SUB", "AUX", "EXTRA",
    # Blank suffix (subject is the whole name)
    "",
]


# =========================================================================
# Targets and clusters
# =========================================================================

def load_clusters() -> list[dict]:
    """Each row is a connected component. We use 'class_hex' and 'members'."""
    clusters = []
    p = ROOT / "docs/analysis/asset-identification/cluster_families.csv"
    with p.open(newline="") as f:
        for row in csv.DictReader(f):
            members = [int(x, 16) for x in row["members"].split(";") if x.startswith("0x")]
            if int(row["component_size"]) < 2:
                continue
            clusters.append({
                "class_hex": int(row["cluster_class_hex"], 16),
                "swap_pop": int(row["swap_popcount"]),
                "comp_size": int(row["component_size"]),
                "members": members,
                "labeled_examples": row.get("labeled_examples", ""),
            })
    return clusters


def load_asset_ids() -> set[int]:
    out = set()
    with (ROOT / "docs/reference/asset-ids-master.csv").open() as f:
        next(f)
        for line in f:
            c = line.rstrip("\n").split(",")
            if len(c) >= 2:
                out.add(int(c[1]))
    return out


# =========================================================================
# Attack
# =========================================================================

def precompute(words: list[str]) -> list[tuple[str, int, int]]:
    return [(w, *calc_hash_and_shift(w)) for w in words if w]


def find_rotation(value: int, target: int) -> int:
    """Return r in [0,31] s.t. rotl(value, r) == target, or -1 if none."""
    for r in range(32):
        if rotl(value, r) == target:
            return r
    return -1


def attack_pair(
    id_a: int, id_b: int, suffixes: list[tuple[str, int, int]],
    prefix_index: dict[tuple[int, int], list[str]], cap_pairs: int = 200_000,
) -> list[dict]:
    """For one (id_a, id_b) pair, find prefix candidates."""
    delta = rotr(id_a ^ id_b, OUT_ROT)
    if delta == 0:
        return []
    # required hash that h(name_A) must equal: rotr(id_a ^ SEED, OUT_ROT)
    h_A_req = rotr(id_a ^ SEED, OUT_ROT)

    hits = []
    pair_count = 0
    for sa, ha, sha in suffixes:
        for sb, hb, shb in suffixes:
            if sa >= sb:
                continue  # avoid (X,X) and reduce by half
            pair_count += 1
            if pair_count > cap_pairs:
                return hits
            # Required: rotl(ha ^ hb, sh_P) == delta
            #           AND sh_P + sh_S_a (mod 32) is the end-shift after S_a
            # Note: sh_P is the shift AFTER consuming P (before S_a starts),
            # not the end-shift. We rotate suffix-hashes by sh_P.
            v = ha ^ hb
            if v == 0:
                continue
            r = find_rotation(v, delta)
            if r < 0:
                continue
            sh_P = r
            # required prefix hash:
            h_P = h_A_req ^ rotl(ha, sh_P)
            # search for any prefix word with (h_P, sh_P)
            ps = prefix_index.get((h_P, sh_P), [])
            for p in ps:
                hits.append({
                    "id_a": id_a, "id_b": id_b,
                    "prefix": p, "suffix_a": sa, "suffix_b": sb,
                    "name_a": p + sa, "name_b": p + sb,
                    "sh_P": sh_P,
                })
    return hits


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--max-cohort", type=int, default=30, help="max member cohorts to scan")
    ap.add_argument("--cap-pairs", type=int, default=200_000)
    ap.add_argument("--top", type=int, default=200)
    ap.add_argument("--out", default=str(ROOT / "docs/analysis/asset-identification/cluster_pair_solutions.csv"))
    args = ap.parse_args()

    clusters = load_clusters()
    asset_ids = load_asset_ids()
    print(f"clusters: {len(clusters)}")
    print(f"target ids: {len(asset_ids)}")

    # Build vocab indices
    suffixes = precompute(SUFFIX_VARIANTS)
    prefixes = precompute(SUBJECTS)
    print(f"vocab: prefixes={len(prefixes)}  suffixes={len(suffixes)}")

    # Index prefixes by (h, sh)
    prefix_index: dict[tuple[int, int], list[str]] = defaultdict(list)
    for p, h, sh in prefixes:
        prefix_index[(h, sh)].append(p)
    print(f"prefix index keys: {len(prefix_index)}")

    # Run pair attack on every cluster pair
    all_hits = []
    t0 = time.time()
    for ci, cl in enumerate(clusters):
        if cl["comp_size"] < 2:
            continue
        if cl["comp_size"] > args.max_cohort:
            continue
        members = cl["members"]
        # All unordered pairs in this component
        for a, b in itertools.combinations(members, 2):
            hits = attack_pair(a, b, suffixes, prefix_index, args.cap_pairs)
            for h in hits:
                h["class_hex"] = f"0x{cl['class_hex']:08x}"
                h["swap_pop"] = cl["swap_pop"]
                h["comp_size"] = cl["comp_size"]
                all_hits.append(h)
        if (ci + 1) % 25 == 0:
            elapsed = time.time() - t0
            print(f"  scanned {ci+1}/{len(clusters)} clusters in {elapsed:.1f}s  hits={len(all_hits)}")

    elapsed = time.time() - t0
    print(f"\nfinished in {elapsed:.1f}s — {len(all_hits)} raw pair hits")

    # ---- Validate hits: prefix should produce both (id_a, id_b) -----------
    def asset_of(name: str) -> int:
        h, _ = calc_hash_and_shift(name)
        return SEED ^ rotl(h, OUT_ROT) & 0xFFFFFFFF

    # group by prefix
    by_prefix: dict[str, list[dict]] = defaultdict(list)
    for h in all_hits:
        by_prefix[h["prefix"]].append(h)

    # filter to multi-hit prefixes (cluster-anchored)
    multi_hits = [(p, hs) for p, hs in by_prefix.items() if len(hs) >= 1]

    # validate explicitly
    validated = []
    for prefix, hs in multi_hits:
        for h in hs:
            a_calc = asset_of(h["name_a"])
            b_calc = asset_of(h["name_b"])
            if a_calc == h["id_a"] and b_calc == h["id_b"]:
                validated.append(h)

    # group validated by (prefix, n_unique_ids_recovered)
    by_prefix_v: dict[str, set[int]] = defaultdict(set)
    for h in validated:
        by_prefix_v[h["prefix"]].add(h["id_a"])
        by_prefix_v[h["prefix"]].add(h["id_b"])

    print(f"\n=== TOP PREFIX SOLUTIONS (≥2 IDs recovered) ===")
    print(f"{'n_ids':>5}  {'prefix':22}  examples")
    out_rows = []
    for prefix in sorted(by_prefix_v, key=lambda p: -len(by_prefix_v[p]))[:args.top]:
        ids = by_prefix_v[prefix]
        if len(ids) < 2:
            continue
        examples = [h for h in validated if h["prefix"] == prefix][:5]
        sample = "; ".join(f"{e['name_a']}=0x{e['id_a']:08x}" for e in examples)
        print(f"  {len(ids):5d}  {prefix:22}  {sample[:80]}")
        out_rows.append({
            "prefix": prefix,
            "n_ids": len(ids),
            "ids_recovered": ";".join(f"0x{i:08x}" for i in sorted(ids)),
            "examples": sample,
        })

    out = Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    if out_rows:
        with out.open("w", newline="") as f:
            w = csv.DictWriter(f, fieldnames=list(out_rows[0].keys()))
            w.writeheader()
            for r in out_rows:
                w.writerow(r)
        print(f"\nwrote {out.relative_to(ROOT)} ({len(out_rows)} prefix solutions)")
    else:
        print("\nNo prefix solutions found.")

    # Single hits: pairs cracked as 1-shot (no prefix recurrence)
    single_hits = [h for h in validated if len(by_prefix_v[h["prefix"]]) == 2]
    if single_hits:
        print(f"\n=== Single-pair hits (one prefix recovers exactly one pair) ===")
        for h in single_hits[:50]:
            print(f"  prefix={h['prefix']:18}  {h['name_a']:30}=0x{h['id_a']:08x}  "
                  f"{h['name_b']:30}=0x{h['id_b']:08x}")


if __name__ == "__main__":
    main()
