"""Find suffix-pair candidates that explain the swap classes observed in
named-function cohorts. For each cohort, compute pairwise XOR deltas, find
their rotation classes, then enumerate suffix tokens whose hash XOR matches.

The output:
  per function -> per cluster class -> candidate (suffix_a, suffix_b) pairs
"""
import collections
import csv
import sys
from pathlib import Path

sys.path.insert(0, "tools/scripts")
from compound_hash_attack import calc_hash_and_shift, rotl, rotr  # type: ignore

ROOT = Path(__file__).resolve().parents[2]
SEED = 0x28C0E011
OUT_ROT = 27

# Vocabulary tuned to function-context semantics:
#   - Frame/animation indices
#   - Direction tokens
#   - Action/state names
#   - Body parts
#   - Boss-specific states (DEATH, IDLE, ATTACK, etc.)
VOCAB = (
    list("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ") +
    [f"{i:02d}" for i in range(100)] +
    [f"{i:01d}" for i in range(10)] +
    [f"F{i:02d}" for i in range(60)] +
    [
        # directions
        "LEFT", "RIGHT", "UP", "DOWN", "FRONT", "BACK", "NORTH", "SOUTH",
        "EAST", "WEST", "TOP", "BOTTOM", "SIDE", "REAR", "FACE", "INNER",
        "OUTER", "ABOVE", "BELOW", "L", "R", "U", "D",
        "MIRROR", "MIRRORED", "FLIP", "FLIPPED",
        # actions
        "STAND", "WALK", "RUN", "JUMP", "FALL", "IDLE", "DIE", "DEATH",
        "DEAD", "HURT", "HIT", "SHOOT", "FIRE", "SPIT", "BOUNCE", "ATTACK",
        "ATK", "LAND", "SLIDE", "TURN", "TAUNT", "SCREAM", "YELL", "SLAM",
        "ROLL", "FLOAT", "FLY", "GLIDE", "SPIN", "CLIMB", "SWIM", "PUSH",
        "PULL", "KICK", "PUNCH", "OPEN", "CLOSE", "START", "END", "INTRO",
        "OUTRO", "OPENING", "CLOSING", "ENTER", "EXIT", "STOMP", "STEP",
        "WAKE", "SLEEP", "WAIT", "REST", "EAT", "DANCE", "SPAWN", "DRAW",
        # body
        "HEAD", "BODY", "TAIL", "WING", "WINGS", "HORN", "HORNS", "EYE",
        "EYES", "MOUTH", "TEETH", "NOSE", "EAR", "EARS", "ARM", "ARMS",
        "LEG", "LEGS", "FOOT", "FEET", "HAND", "HANDS", "FINGER", "HAIR",
        "STOMACH", "BELLY", "CHEST", "TORSO", "WAIST", "NECK", "PART",
        "PARTS", "PIECE", "LIMB",
        # FX
        "SHOT", "BLAST", "BURST", "PARTICLE", "DEBRIS", "CHUNK", "FLASH",
        "GLOW", "AURA", "RING", "WAVE", "FLAME", "SPARK", "SMOKE", "DUST",
        # boss states
        "PHASE1", "PHASE2", "PHASE3", "PHASE", "STATE1", "STATE2", "STATE3",
        "TIRED", "ANGRY", "CHARGE", "DEFEND", "RECOVER", "ENRAGE", "BERSERK",
        # FX/SFX
        "PICKUP", "COLLECT", "USE", "ACTIVATE", "DEACTIVATE", "TRIGGER",
        # menu/UI
        "TEXT", "ICON", "GLYPH", "FRAME", "NORMAL", "HOVER", "SELECT",
        "DISABLED", "ENABLED", "PRESSED", "RELEASED",
        # collectibles
        "COIN", "GEM", "STAR", "HEART", "LIFE", "POWER", "BONUS",
        "1UP", "2UP",
    ]
)
VOCAB = list(dict.fromkeys(VOCAB))  # dedup keeping order


def classify(V: int) -> int:
    return min(rotl(V, r) for r in range(32))


def main():
    # Build suffix-pair index by class
    print(f"vocab size: {len(VOCAB)}")
    sh_data = []
    for s in VOCAB:
        h, sh = calc_hash_and_shift(s)
        sh_data.append((s, h, sh))

    cls_to_swaps: dict[int, list[tuple[str, str, int]]] = collections.defaultdict(list)
    for i, (Sa, ha, _) in enumerate(sh_data):
        for Sb, hb, _ in sh_data[i + 1:]:
            V = ha ^ hb
            if V == 0:
                continue
            cls_to_swaps[classify(V)].append((Sa, Sb, V))
    print(f"suffix-pair index: {sum(len(v) for v in cls_to_swaps.values()):,} "
          f"in {len(cls_to_swaps):,} classes")

    # Load asset usage by function
    usage_csv = ROOT / "docs/analysis/asset-identification/asset_usage_by_function.csv"
    func_ids: dict[str, list[int]] = {}
    with usage_csv.open(newline="") as f:
        for r in csv.DictReader(f):
            ids = [int(x, 16) for x in r["ids"].split(";") if x.startswith("0x")]
            func_ids[r["function"]] = ids

    out_path = ROOT / "docs/analysis/asset-identification/cohort_swap_candidates.csv"
    out_md = ROOT / "docs/analysis/asset-identification/cohort_swap_candidates.md"

    cohort_rows = []
    md_chunks = ["# Cohort swap-candidate report\n\n",
                 "For each named function that touches multiple asset IDs, we compute the "
                 "pairwise XOR-delta rotation class and look up which suffix-token pairs "
                 "would produce that class. A small number of candidates with strong "
                 "semantic match to the function's name is the gold signal.\n\n"]

    for func, ids in func_ids.items():
        if len(ids) < 2:
            continue
        sorted_ids = sorted(set(ids))
        pair_data = []
        cls_examples: dict[int, list[tuple[int, int]]] = collections.defaultdict(list)
        for i, a in enumerate(sorted_ids):
            for b in sorted_ids[i + 1:]:
                d = a ^ b
                if d == 0:
                    continue
                cls = classify(d)
                pair_data.append((a, b, cls))
                cls_examples[cls].append((a, b))

        cls_counts = collections.Counter(c for _, _, c in pair_data)
        # For each class hit by the cohort, find candidate suffix swaps
        for cls, count in cls_counts.most_common(8):
            if cls == 0:
                continue
            cands = cls_to_swaps.get(cls, [])
            cohort_rows.append({
                "function": func,
                "n_ids": len(sorted_ids),
                "cluster_class": f"0x{cls:08x}",
                "n_pairs_in_class": count,
                "n_suffix_candidates": len(cands),
                "example_pair": f"0x{cls_examples[cls][0][0]:08x}↔0x{cls_examples[cls][0][1]:08x}",
                "suffix_candidates": " | ".join(f"{a}↔{b}" for a, b, _ in cands[:8]),
            })

        # Markdown section for the top-density functions
        if len(sorted_ids) >= 4 and sum(cls_counts.values()) >= 5:
            md_chunks.append(f"## `{func}` ({len(sorted_ids)} ids)\n\n")
            md_chunks.append("| pair (IDs) | class | popcount | suffix-pair candidates |\n")
            md_chunks.append("|---|---|---|---|\n")
            for a, b, cls in pair_data[:30]:
                cands = cls_to_swaps.get(cls, [])
                pop = bin(cls).count("1")
                if not cands:
                    sample = "(none in vocab)"
                else:
                    sample = ", ".join(f"`{x}↔{y}`" for x, y, _ in cands[:5])
                md_chunks.append(
                    f"| `0x{a:08x}↔0x{b:08x}` | `0x{cls:08x}` | {pop} | {sample} |\n"
                )
            md_chunks.append("\n")

    cohort_rows.sort(key=lambda r: (-r["n_pairs_in_class"], -r["n_ids"]))
    out_path.parent.mkdir(parents=True, exist_ok=True)
    with out_path.open("w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(cohort_rows[0].keys()))
        w.writeheader()
        for r in cohort_rows:
            w.writerow(r)
    print(f"wrote {out_path.relative_to(ROOT)} ({len(cohort_rows)} rows)")

    out_md.write_text("".join(md_chunks))
    print(f"wrote {out_md.relative_to(ROOT)}")

    # Preview
    print("\n=== TOP COHORT-CLASS HITS (likely suffix swaps for that function) ===")
    for r in cohort_rows[:20]:
        print(f"  {r['function']:42s}  cls={r['cluster_class']}  pairs={r['n_pairs_in_class']:2d}  "
              f"cands={r['suffix_candidates'][:60]}")


if __name__ == "__main__":
    main()
