"""Function-name-driven prefix solver.

For each function with a cohort of asset IDs, extract semantic tokens from the
function name itself, generate compound-prefix candidates, and check if any
prefix P + suffix S (digit, action, etc.) reproduces multiple IDs in the
cohort.

This is the most constraint-rich attack: we use the developer's own
function-naming vocabulary as the prefix corpus.
"""
import collections
import csv
import itertools
import re
import sys
from pathlib import Path

sys.path.insert(0, "tools/scripts")
from compound_hash_attack import calc_hash_and_shift, rotl  # type: ignore

ROOT = Path(__file__).resolve().parents[2]
SEED = 0x28C0E011
OUT_ROT = 27


def asset_id(name: str) -> int:
    h, _ = calc_hash_and_shift(name)
    return SEED ^ rotl(h, OUT_ROT) & 0xFFFFFFFF


# Common type prefixes to prepend
TYPE_PREFIXES = ["", "S_", "SPR_", "SEQ_", "SND_", "SFX_", "FX_", "ANM_",
                 "ANIM_", "N2_", "SM_", "BG_", "FG_", "MENU_", "ITEM_",
                 "ENT_", "OBJ_", "BOSS_", "ENEMY_", "HUD_",
                 "S", "SPR", "SEQ", "SND", "SFX", "FX", "ANM"]
SEPARATORS = ["", "_", "-"]

# Suffix vocab for frame/state numbering
SUFFIX_TOKENS = [
    "", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
    "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K",
    "01", "02", "03", "04", "05", "06", "07", "08", "09", "10",
    "11", "12", "13", "14", "15", "16", "17", "18", "19", "20",
    "F00", "F01", "F02", "F03", "F04", "F05",
    "LEFT", "RIGHT", "UP", "DOWN", "FRONT", "BACK",
    "STAND", "WALK", "RUN", "JUMP", "FALL", "IDLE", "DIE", "DEATH",
    "ATTACK", "ATK", "HURT", "HIT", "SHOOT", "FIRE", "SLAM", "ROLL",
    "OPEN", "CLOSE", "START", "END", "INTRO", "OUTRO",
    "HEAD", "BODY", "TAIL", "ARM", "LEG", "FOOT", "HAND",
    "EYE", "EYES", "MOUTH",
    "PART", "PARTS", "PIECE", "PARTICLE", "DEBRIS",
    "MIRROR", "FLIP",
]


# Words to ignore when tokenizing function names
IGNORE_TOKENS = {"INIT", "TICK", "UPDATE", "CALLBACK", "HANDLER", "EVENT",
                 "STATE", "FUNC", "CB", "DO", "GET", "SET", "MAKE", "TYPE",
                 "ENTITY", "PROCESS", "CHECK", "TEST", "RUN", "BEGIN", "END",
                 "ENTER", "EXIT", "WITH", "AND", "OR", "FROM", "TO", "FOR",
                 "ON", "OFF", "UN", "IS", "BY"}


def tokenize_function(name: str) -> list[str]:
    """Split a CamelCase / underscore function name into uppercase tokens."""
    raw = re.findall(r"[A-Z][a-z0-9]+|[A-Z]+(?=[A-Z]|$|_)|[a-z0-9]+", name)
    out = []
    for r in raw:
        ru = r.upper()
        if len(ru) >= 2 and ru not in IGNORE_TOKENS and not ru.isdigit():
            out.append(ru)
    return out


def generate_prefixes(tokens: list[str]) -> list[str]:
    """Generate compound prefix candidates from tokens.

    Yields TYPE_PREFIX + token_combo + SEPARATOR where token_combo is
    1-3 tokens joined with separators.
    """
    seen = set()
    for tp in TYPE_PREFIXES:
        for n in range(1, min(4, len(tokens) + 1)):
            for combo in itertools.permutations(tokens, n):
                for sep in SEPARATORS:
                    for trail in ("", "_", "-"):
                        prefix = tp + sep.join(combo) + trail
                        if prefix not in seen:
                            seen.add(prefix)
                            yield prefix


def try_prefix(prefix: str, target_ids: set[int]) -> dict[int, str]:
    """For each suffix token, compute asset id. Return matches."""
    matches: dict[int, str] = {}
    h_p, sh_p = calc_hash_and_shift(prefix)
    for suf in SUFFIX_TOKENS:
        h_s, _ = calc_hash_and_shift(suf)
        h = h_p ^ rotl(h_s, sh_p)
        a = SEED ^ rotl(h, OUT_ROT) & 0xFFFFFFFF
        if a in target_ids:
            matches[a] = suf
    return matches


def main():
    usage_csv = ROOT / "docs/analysis/asset-identification/asset_usage_by_function.csv"
    func_ids: dict[str, set[int]] = {}
    with usage_csv.open(newline="") as f:
        for r in csv.DictReader(f):
            ids = {int(x, 16) for x in r["ids"].split(";") if x.startswith("0x")}
            if len(ids) >= 2:
                func_ids[r["function"]] = ids

    print(f"functions with >=2 ids: {len(func_ids)}")

    target_info = {}
    with (ROOT / "docs/analysis/asset-identification/sprite_identification_template.csv").open(newline="") as f:
        for r in csv.DictReader(f):
            try:
                target_info[int(r["sprite_decimal"])] = r.get("human_role", "")
            except Exception:
                continue

    out_path = ROOT / "docs/analysis/asset-identification/cohort_prefix_solutions.csv"
    out_rows = []
    total_combos = 0

    for func, cohort_ids in func_ids.items():
        tokens = tokenize_function(func)
        if not tokens:
            continue
        best = None
        for prefix in generate_prefixes(tokens):
            total_combos += 1
            matches = try_prefix(prefix, cohort_ids)
            if len(matches) >= 2:
                if best is None or len(matches) > best[1]:
                    best = (prefix, len(matches), matches)
        if best is not None:
            prefix, n, matches = best
            # Validate
            sample_id = next(iter(matches))
            sample_suf = matches[sample_id]
            sample_full = prefix + sample_suf
            assert asset_id(sample_full) == sample_id
            out_rows.append({
                "function": func,
                "tokens": ";".join(tokens),
                "cohort_size": len(cohort_ids),
                "matches": len(matches),
                "prefix": prefix,
                "sample_name": sample_full,
                "sample_id": f"0x{sample_id:08x}",
                "all_matches": "; ".join(
                    f"{prefix}{s}=0x{i:08x}" for i, s in sorted(matches.items())
                ),
                "labels": " | ".join(
                    f"0x{i:08x}={target_info.get(i, '')[:30]}"
                    for i in sorted(matches.keys())
                    if target_info.get(i)
                ),
            })

    print(f"scanned {total_combos:,} prefix candidates")
    out_rows.sort(key=lambda r: (-r["matches"], -r["cohort_size"]))
    out_path.parent.mkdir(parents=True, exist_ok=True)
    if out_rows:
        with out_path.open("w", newline="") as f:
            w = csv.DictWriter(f, fieldnames=list(out_rows[0].keys()))
            w.writeheader()
            for r in out_rows:
                w.writerow(r)
        print(f"wrote {out_path.relative_to(ROOT)} ({len(out_rows)} hits)")

    print("\n=== TOP PREFIX SOLUTIONS (function -> recovered prefix + matches) ===")
    for r in out_rows[:30]:
        print(f"  {r['matches']:2d}/{r['cohort_size']:2d}  {r['function']:42s}  prefix='{r['prefix']}'  sample={r['sample_name']}")


if __name__ == "__main__":
    main()
