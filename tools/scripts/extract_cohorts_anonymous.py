#!/usr/bin/env python3
"""Anonymous cohort extraction.

Re-extract asset-id usage from the Ghidra decompilation, keying every cohort
by the function body's *line range* (a stand-in for its address - both are
structurally real even when the name is a guess). Function names are kept
only as opaque tags for human cross-reference; they are NOT used as semantic
signals.

For each cohort we record:
  - line_range: '<start>-<end>' in the decomp file (stand-in for address)
  - cohort_id: 'C<index>'
  - asset_ids: sorted list of IDs used inside that body
  - intra_cluster_class_histogram: count of each XOR-delta rotation class
    between members (this IS ground truth — pure math on the hex IDs)
  - co_occurring_cohort_ids: other cohorts that share any member

Output:
  docs/analysis/asset-identification/cohorts_anonymous.csv

Use these cohorts for any subsequent prefix-recovery attack. NEVER feed the
function name into the prefix vocabulary.
"""
from __future__ import annotations

import collections
import csv
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from compound_hash_attack import calc_hash_and_shift, rotl  # type: ignore

ROOT = Path(__file__).resolve().parents[2]
SEED = 0x28C0E011
OUT_ROT = 27


def classify(V: int) -> int:
    return min(rotl(V, r) for r in range(32))


def load_targets() -> dict[int, dict[str, str]]:
    info: dict[int, dict[str, str]] = {}
    with (ROOT / "docs/reference/asset-ids-master.csv").open() as f:
        next(f)
        for line in f:
            c = line.rstrip().split(",")
            if len(c) >= 5:
                info[int(c[1])] = {
                    "kinds": c[2],
                    "popcount": c[3],
                    "sources": c[4],
                    "levels": "",
                    "role": "",
                }
    ident = ROOT / "docs/analysis/asset-identification/sprite_identification_template.csv"
    if ident.exists():
        with ident.open(newline="") as f:
            for r in csv.DictReader(f):
                try:
                    i = int(r["sprite_decimal"])
                except Exception:
                    continue
                if i in info:
                    info[i]["levels"] = r.get("levels", "")
                    info[i]["role"] = r.get("human_role", "")
    return info


HEX_RE = re.compile(r"\b0x([0-9a-fA-F]{6,8})\b")
FUNC_RE = re.compile(r"^[A-Za-z_][\w\s\*]*\b([A-Za-z_][\w]+)\s*\([^)]*\)\s*$")


def iter_function_bodies(path: Path):
    """Yield (opaque_name, start_line, end_line, body_text) for each function."""
    with path.open() as f:
        lines = f.readlines()
    n = len(lines)
    i = 0
    while i < n:
        m = FUNC_RE.match(lines[i].rstrip())
        if not m:
            i += 1
            continue
        j = i + 1
        while j < n and lines[j].strip() == "":
            j += 1
        if j >= n or not lines[j].lstrip().startswith("{"):
            i += 1
            continue
        name = m.group(1)
        depth = 0
        k = j
        opened = False
        while k < n:
            for ch in lines[k]:
                if ch == "{":
                    depth += 1
                    opened = True
                elif ch == "}":
                    depth -= 1
            k += 1
            if opened and depth == 0:
                break
        yield name, i + 1, k, "".join(lines[i:k])
        i = k


def main():
    info = load_targets()
    target_ids = set(info.keys())

    decomp_path = ROOT / "export/SLES_010.90.c"
    if not decomp_path.exists():
        print(f"ERROR: {decomp_path} missing", file=sys.stderr)
        return 1
    print(f"scanning {decomp_path.relative_to(ROOT)}")

    cohorts: list[dict] = []  # each entry is a dict
    for opaque_name, start, end, body in iter_function_bodies(decomp_path):
        ids: set[int] = set()
        for line_text in body.splitlines():
            for m in HEX_RE.finditer(line_text):
                try:
                    v = int(m.group(1), 16)
                except Exception:
                    continue
                if v in target_ids:
                    ids.add(v)
        if not ids:
            continue
        cohorts.append({
            "opaque_name": opaque_name,
            "line_range": f"{start}-{end}",
            "n_lines": end - start + 1,
            "ids": sorted(ids),
        })

    print(f"cohorts with >=1 id: {len(cohorts)}")

    # Compute intra-cluster class histograms
    for c in cohorts:
        ids = c["ids"]
        hist: collections.Counter[int] = collections.Counter()
        deltas = []
        for i, a in enumerate(ids):
            for b in ids[i + 1:]:
                d = a ^ b
                if d == 0:
                    continue
                cls = classify(d)
                hist[cls] += 1
                deltas.append((a, b, cls))
        c["intra_class_hist"] = hist
        c["intra_pairs"] = deltas
        c["n_ids"] = len(ids)

    # Co-occurrence
    id_to_cohorts: dict[int, list[int]] = collections.defaultdict(list)
    for ci, c in enumerate(cohorts):
        for i in c["ids"]:
            id_to_cohorts[i].append(ci)

    # Write anonymous cohorts CSV
    out_path = ROOT / "docs/analysis/asset-identification/cohorts_anonymous.csv"
    out_path.parent.mkdir(parents=True, exist_ok=True)
    fields = [
        "cohort_id", "line_range", "opaque_name", "n_ids", "n_intra_pairs",
        "intra_class_top", "asset_ids", "asset_kinds",
        "asset_levels", "labeled_ids",
        "co_occurring_cohort_count",
    ]
    with out_path.open("w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        for ci, c in enumerate(cohorts):
            ids = c["ids"]
            kinds = {info.get(i, {}).get("kinds", "?") for i in ids}
            levels: set[str] = set()
            for i in ids:
                for lv in (info.get(i, {}).get("levels", "") or "").split(";"):
                    if lv:
                        levels.add(lv)
            labels = [(i, info[i]["role"]) for i in ids if info.get(i, {}).get("role")]
            top_classes = c["intra_class_hist"].most_common(6)
            top_str = ";".join(
                f"0x{cls:08x}({n})" for cls, n in top_classes if cls != 0
            )
            co_occ = set()
            for i in ids:
                for ci2 in id_to_cohorts[i]:
                    if ci2 != ci:
                        co_occ.add(ci2)
            w.writerow({
                "cohort_id": f"C{ci:04d}",
                "line_range": c["line_range"],
                "opaque_name": c["opaque_name"],
                "n_ids": c["n_ids"],
                "n_intra_pairs": sum(c["intra_class_hist"].values()),
                "intra_class_top": top_str,
                "asset_ids": ";".join(f"0x{i:08x}" for i in ids),
                "asset_kinds": ";".join(sorted(kinds)),
                "asset_levels": ";".join(sorted(levels)),
                "labeled_ids": " | ".join(
                    f"0x{i:08x}={role[:40]}" for i, role in labels
                ),
                "co_occurring_cohort_count": len(co_occ),
            })
    print(f"wrote {out_path.relative_to(ROOT)}")

    # Build a constraint file for the prefix sieve (PAIR FORMAT).
    # For every cohort with >=2 ids, emit every pair as a constraint, tagged
    # with the cohort id (so we can require multiple hits in same cohort
    # later).
    constraints_path = ROOT / "docs/analysis/asset-identification/cohort_pair_constraints.csv"
    n_pairs = 0
    n_cohorts_with_pairs = 0
    with constraints_path.open("w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["cohort_id", "id_a", "id_b", "class_hex", "popcount"])
        for ci, c in enumerate(cohorts):
            if len(c["intra_pairs"]) == 0:
                continue
            n_cohorts_with_pairs += 1
            for a, b, cls in c["intra_pairs"]:
                w.writerow([
                    f"C{ci:04d}",
                    f"0x{a:08x}", f"0x{b:08x}",
                    f"0x{cls:08x}", bin(cls).count("1"),
                ])
                n_pairs += 1
    print(f"wrote {constraints_path.relative_to(ROOT)} ({n_pairs} pair constraints "
          f"across {n_cohorts_with_pairs} multi-id cohorts)")

    # Distribution stats
    sizes = collections.Counter(c["n_ids"] for c in cohorts)
    print("\ncohort-size distribution:")
    for sz, n in sorted(sizes.items()):
        bar = "#" * min(n, 60)
        print(f"  {sz:3d} ids: {n:4d}  {bar}")


if __name__ == "__main__":
    main()
