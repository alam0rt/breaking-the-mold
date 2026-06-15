#!/usr/bin/env python3
"""Extract asset-id usage from the Ghidra decompiled program.

Scans the full-program decompilation for hex literals that match known
asset IDs and records:
  - the asset id
  - the enclosing function name (parsed from the C source)
  - the line context (one line of code)

Output:
  docs/analysis/asset-identification/asset_usage_raw.csv
      id_hex, function, file_kind, line_no, context
  docs/analysis/asset-identification/asset_usage_by_function.csv
      function, ids_used (semicolon list), n_ids, ids_in_clusters
  docs/analysis/asset-identification/asset_cohorts.csv
      cohort_signature, function_names, member_ids, cluster_classes_hit

A "cohort" is a set of asset IDs that appear in the same function. If
function `KlaymenPlayPaused_DrawCallback` uses {id1, id2, id3, id4, id5, id6},
those 6 ids form a strong semantic cohort.
"""
from __future__ import annotations

import collections
import csv
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from compound_hash_attack import calc_hash_and_shift, rotl, rotr  # type: ignore

ROOT = Path(__file__).resolve().parents[2]
SEED = 0x28C0E011
OUT_ROT = 27


def classify(V: int) -> int:
    return min(rotl(V, r) for r in range(32))


def load_targets() -> dict[int, dict[str, str]]:
    info = {}
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
# Function header (Ghidra output): "type name(args)\n{" or "type *name(args)\n{"
FUNC_RE = re.compile(r"^[A-Za-z_][\w\s\*]*\b([A-Za-z_][\w]+)\s*\([^)]*\)\s*$")
BRACE_OPEN = re.compile(r"^\{")


def parse_functions(path: Path):
    """Yield (function_name, start_line, end_line, code) for each function.

    Tracks brace depth to detect end. Lines are 1-based.
    """
    with path.open() as f:
        lines = f.readlines()
    n = len(lines)
    i = 0
    while i < n:
        m = FUNC_RE.match(lines[i].rstrip())
        if not m:
            i += 1
            continue
        # Next line might be '{' (Ghidra style)
        j = i + 1
        while j < n and lines[j].strip() == "":
            j += 1
        if j >= n or not lines[j].lstrip().startswith("{"):
            i += 1
            continue
        name = m.group(1)
        start = i
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
        yield name, start + 1, k, "".join(lines[start:k])
        i = k


def main():
    info = load_targets()
    target_ids = set(info.keys())

    decomp_paths = [
        ROOT / "export/SLES_010.90.c",
        ROOT / "disks/SLES_010.90.c",
    ]
    decomp_paths = [p for p in decomp_paths if p.exists()]
    print(f"scanning {len(decomp_paths)} decomp files: "
          f"{[p.name for p in decomp_paths]}")

    usage_rows = []  # (id, function, file, line_no, context)
    func_id_map: dict[str, set[int]] = collections.defaultdict(set)
    func_file: dict[str, str] = {}
    func_examples: dict[str, list[tuple[int, str]]] = collections.defaultdict(list)

    for path in decomp_paths:
        file_kind = path.parent.name + "/" + path.name
        for name, start, end, code in parse_functions(path):
            offset = 0
            for ln, line_text in enumerate(code.splitlines(), start=start):
                for m in HEX_RE.finditer(line_text):
                    try:
                        v = int(m.group(1), 16)
                    except Exception:
                        continue
                    if v in target_ids:
                        usage_rows.append({
                            "id_hex": f"0x{v:08x}",
                            "function": name,
                            "file": file_kind,
                            "line_no": ln,
                            "context": line_text.strip()[:200],
                        })
                        func_id_map[name].add(v)
                        if name not in func_file:
                            func_file[name] = file_kind
                        if len(func_examples[name]) < 3:
                            func_examples[name].append(
                                (v, line_text.strip()[:120])
                            )

    print(f"raw usages: {len(usage_rows)}")
    print(f"functions referencing assets: {len(func_id_map)}")
    print(f"unique ids referenced: {len({r['id_hex'] for r in usage_rows})}")

    out_raw = ROOT / "docs/analysis/asset-identification/asset_usage_raw.csv"
    out_raw.parent.mkdir(parents=True, exist_ok=True)
    with out_raw.open("w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=["id_hex", "function", "file",
                                          "line_no", "context"])
        w.writeheader()
        for r in usage_rows:
            w.writerow(r)
    print(f"wrote {out_raw.relative_to(ROOT)}")

    # Build cluster index
    ids = sorted(info.keys())
    cls_of: dict[tuple[int, int], int] = {}
    for i, a in enumerate(ids):
        for b in ids[i + 1:]:
            d = a ^ b
            if d == 0:
                continue
            cls_of[(a, b)] = classify(d)

    out_func = ROOT / "docs/analysis/asset-identification/asset_usage_by_function.csv"
    by_func_rows = []
    for func, idset in func_id_map.items():
        idsl = sorted(idset)
        cluster_classes = collections.Counter()
        for i, a in enumerate(idsl):
            for b in idsl[i + 1:]:
                cluster_classes[cls_of[(a, b)]] += 1
        # Filter to fat clusters
        fat = {cls: n for cls, n in cluster_classes.items()
               if n >= 1 and cls != 0 and bin(cls).count("1") >= 2}
        by_func_rows.append({
            "function": func,
            "file": func_file.get(func, ""),
            "n_ids": len(idsl),
            "ids": ";".join(f"0x{i:08x}" for i in idsl[:40]),
            "n_intra_cluster_edges": sum(fat.values()),
            "top_cluster_classes": ";".join(
                f"0x{c:08x}({n})" for c, n in sorted(fat.items(),
                                                      key=lambda kv: -kv[1])[:6]
            ),
            "first_examples": " || ".join(
                f"0x{i:08x}: {ctx[:80]}" for i, ctx in func_examples.get(func, [])[:3]
            ),
        })
    by_func_rows.sort(key=lambda r: (-r["n_intra_cluster_edges"], -r["n_ids"]))
    with out_func.open("w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(by_func_rows[0].keys()))
        w.writeheader()
        for r in by_func_rows:
            w.writerow(r)
    print(f"wrote {out_func.relative_to(ROOT)}")

    # Cohort signatures: a "cohort" is a set of assets sharing identical caller-function set
    asset_callers: dict[int, frozenset[str]] = {}
    for func, idset in func_id_map.items():
        for i in idset:
            asset_callers.setdefault(i, set()).add(func)
    cohort_of_id = {i: frozenset(s) for i, s in asset_callers.items()}
    cohorts: dict[frozenset[str], list[int]] = collections.defaultdict(list)
    for i, sig in cohort_of_id.items():
        cohorts[sig].append(i)

    out_cohort = ROOT / "docs/analysis/asset-identification/asset_cohorts.csv"
    cohort_rows = []
    for sig, ids_in_cohort in cohorts.items():
        if len(ids_in_cohort) < 2:
            continue
        cluster_hits = collections.Counter()
        ids_sorted = sorted(ids_in_cohort)
        for i, a in enumerate(ids_sorted):
            for b in ids_sorted[i + 1:]:
                cluster_hits[cls_of[(a, b)]] += 1
        fat = {cls: n for cls, n in cluster_hits.items()
               if cls != 0 and bin(cls).count("1") >= 2}
        labels = [info.get(i, {}).get("role", "") for i in ids_sorted]
        cohort_rows.append({
            "n_callers": len(sig),
            "n_members": len(ids_sorted),
            "n_cluster_edges": sum(fat.values()),
            "callers": ";".join(sorted(sig)[:5]),
            "members": ";".join(f"0x{i:08x}" for i in ids_sorted[:30]),
            "labeled_roles": " | ".join(l for l in labels if l)[:160],
            "top_cluster_classes": ";".join(
                f"0x{c:08x}({n})" for c, n in sorted(fat.items(),
                                                      key=lambda kv: -kv[1])[:6]
            ),
        })
    cohort_rows.sort(key=lambda r: (-r["n_cluster_edges"], -r["n_members"]))
    if cohort_rows:
        with out_cohort.open("w", newline="") as f:
            w = csv.DictWriter(f, fieldnames=list(cohort_rows[0].keys()))
            w.writeheader()
            for r in cohort_rows:
                w.writerow(r)
        print(f"wrote {out_cohort.relative_to(ROOT)} ({len(cohort_rows)} cohorts)")

    print("\n=== TOP 20 FUNCTIONS BY CLUSTER-EDGE DENSITY ===")
    for r in by_func_rows[:20]:
        print(f"  {r['n_ids']:3d} ids, {r['n_intra_cluster_edges']:3d} edges  {r['function']:45s}  classes={r['top_cluster_classes'][:60]}")

    print("\n=== TOP 20 COHORTS (asset families per caller-set) ===")
    for r in cohort_rows[:20]:
        print(f"  members={r['n_members']:3d}  edges={r['n_cluster_edges']:3d}  callers={r['callers'][:60]}  roles={r['labeled_roles'][:60]}")


if __name__ == "__main__":
    main()
