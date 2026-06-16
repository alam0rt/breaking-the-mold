#!/usr/bin/env python3
"""Meta-analysis of remaining uncracked Skullmonkeys asset IDs.

Goal: don't brute force blindly. Group every remaining ID by its multiple
constraints (floor, level scope, cohort, cluster family, visual role,
cracked-sibling status) and rank by **attackability**.

Outputs:
  docs/analysis/asset-identification/attack_priorities.csv
    - per-id row with all known constraints + attack hint + score
  docs/analysis/asset-identification/cohort_anchors.csv
    - cohorts with at least one cracked sibling (huge anchor for prefix recovery)
  docs/analysis/asset-identification/cluster_anchors.csv
    - cluster families with at least one cracked / labeled member

Run:
    python3 tools/scripts/meta_analysis.py
"""
from __future__ import annotations
import csv, sys
from collections import defaultdict
from pathlib import Path

sys.path.insert(0, "tools/scripts")
from compound_hash_attack import calc_hash_and_shift, rotl, rotr, asset_id  # type: ignore

ROOT = Path(__file__).resolve().parents[2]
SEED = 0x28C0E011
OUT_ROT = 27


# ---- 1. Confirmed cracked names (anchor set) -----------------------------
CONFIRMED_NAMES: dict[int, str] = {
    0x29C0E211: "NO",
    0x2AD0F011: "YES",
    0x0AD0F813: "PAUSED",
    0x68C0F413: "QUIT",
    0x69C04050: "CONTINUE",
    0x69C8F473: "QUITGAME",
}

# ---- 2. Visual role hints --------------------------------------------------
def load_visual_roles() -> dict[int, dict[str, str]]:
    """sprite_decimal -> {role, possible_name, confidence}."""
    out: dict[int, dict[str, str]] = {}
    p = ROOT / "docs/analysis/asset-identification/sprite_identification_template.csv"
    with p.open(newline="") as f:
        for row in csv.DictReader(f):
            try:
                idv = int(row["sprite_decimal"])
            except Exception:
                continue
            role = (row.get("human_role") or "").strip()
            if role:
                out[idv] = {
                    "role": role,
                    "possible_name": (row.get("possible_original_name") or "").strip(),
                    "confidence": (row.get("confidence") or "").strip(),
                    "notes": (row.get("notes") or "").strip(),
                }
    return out


# ---- 3. Categories (kind / floor / levels) --------------------------------
def load_categories() -> dict[int, dict[str, str]]:
    out: dict[int, dict[str, str]] = {}
    p = ROOT / "docs/analysis/asset-identification/asset_categories.csv"
    with p.open(newline="") as f:
        for row in csv.DictReader(f):
            idv = int(row["id_hex"], 16)
            out[idv] = {
                "type": row["type"],
                "floor": int(row["floor"]),
                "n_levels": int(row["n_levels"]),
                "levels": row["levels"],
                "code_refs": int(row["code_refs"]),
                "func_family": row["func_family"],
                "nn_dist": int(row["nn_dist"]),
                "nn_id": int(row["nn_id"], 16) if row["nn_id"].startswith("0x") else 0,
            }
    return out


# ---- 4. Cohort membership (function bodies) -------------------------------
def load_cohorts() -> tuple[dict[int, list[str]], dict[str, dict]]:
    """Return (id->cohort_ids, cohort_id->info)."""
    id_to_cohorts: dict[int, list[str]] = defaultdict(list)
    cohort_info: dict[str, dict] = {}
    p = ROOT / "docs/analysis/asset-identification/cohorts_anonymous.csv"
    with p.open(newline="") as f:
        for row in csv.DictReader(f):
            cid = row["cohort_id"]
            ids = [int(x, 16) for x in row["asset_ids"].split(";") if x.startswith("0x")]
            cohort_info[cid] = {
                "n_ids": int(row["n_ids"]),
                "members": ids,
                "kinds": row["asset_kinds"],
                "levels": row["asset_levels"],
                "labeled_ids": row["labeled_ids"],
                "opaque_name": row["opaque_name"],
            }
            for idv in ids:
                id_to_cohorts[idv].append(cid)
    return id_to_cohorts, cohort_info


# ---- 5. Cluster families (suffix-swap classes) ----------------------------
def load_clusters() -> tuple[dict[int, list[str]], dict[str, dict]]:
    """Return (id->cluster_ids, cluster_id->info). Each row in cluster_families.csv
    is a connected component within one rotation class."""
    id_to_clusters: dict[int, list[str]] = defaultdict(list)
    cluster_info: dict[str, dict] = {}
    p = ROOT / "docs/analysis/asset-identification/cluster_families.csv"
    with p.open(newline="") as f:
        for i, row in enumerate(csv.DictReader(f)):
            cid = f"K{i:04d}"
            ids = [int(x, 16) for x in row["members"].split(";") if x.startswith("0x")]
            cluster_info[cid] = {
                "class_hex": row["cluster_class_hex"],
                "swap_pop": int(row["swap_popcount"]),
                "n_members": int(row["n_members"]),
                "comp_size": int(row["component_size"]),
                "n_labeled": int(row["n_labeled"]),
                "members": ids,
                "swap_examples": row["swap_examples"],
            }
            for idv in ids:
                id_to_clusters[idv].append(cid)
    return id_to_clusters, cluster_info


# ---- 6. Master ID list (asset-ids-master.csv) -----------------------------
def load_master() -> dict[int, dict[str, str]]:
    out: dict[int, dict[str, str]] = {}
    p = ROOT / "docs/reference/asset-ids-master.csv"
    with p.open(newline="") as f:
        for row in csv.DictReader(f):
            idv = int(row["id_dec"])
            out[idv] = {
                "kinds": row["kinds"],
                "popcount": int(row["popcount"]),
                "sources": row["sources"],
            }
    return out


# ---- 7. Floor of an asset id ---------------------------------------------
def floor_of(idv: int) -> int:
    return bin((idv ^ SEED) & 0xFFFFFFFF).count("1")


# ---- 8. Attackability scoring ---------------------------------------------
def attackability_score(
    idv: int,
    floor: int,
    visual: dict | None,
    cohorts: list[str],
    cohort_info: dict,
    clusters: list[str],
    cluster_info: dict,
    n_levels: int,
    confirmed: dict[int, str],
) -> tuple[float, list[str]]:
    score = 0.0
    reasons: list[str] = []

    # -------- 1. floor cost: lower = far easier --------
    # exhaustive preimage enumeration is feasible up to floor ~10
    if floor <= 7:
        score += 30
        reasons.append(f"low floor {floor} (exhaustive feasible)")
    elif floor <= 10:
        score += 18
        reasons.append(f"floor {floor} (exhaustive borderline)")
    elif floor <= 13:
        score += 8
        reasons.append(f"floor {floor} (medium)")
    elif floor <= 16:
        score += 2
    else:
        score -= 3
        reasons.append(f"high floor {floor} (long name)")

    # -------- 2. visual role hint --------
    if visual is not None:
        role = visual["role"]
        score += 25
        reasons.append(f"visual role: {role[:40]}")
        # exact text label is gold
        if role.lower().startswith("text:"):
            score += 50
            reasons.append("EXACT TEXT LABEL")
        # a possible name guess from the user
        if visual.get("possible_name"):
            score += 15
            reasons.append(f"name guess: {visual['possible_name']}")

    # -------- 3. cohort with confirmed anchor --------
    cohort_anchor_count = 0
    cohort_total_size = 0
    for cid in cohorts:
        members = cohort_info[cid]["members"]
        anchors = [m for m in members if m in confirmed]
        if anchors:
            cohort_anchor_count += len(anchors)
            cohort_total_size = max(cohort_total_size, len(members))
    if cohort_anchor_count >= 2:
        score += 40
        reasons.append(f"cohort has {cohort_anchor_count} confirmed anchors")
    elif cohort_anchor_count == 1:
        score += 25
        reasons.append("cohort has 1 confirmed anchor")

    # -------- 4. cluster family with anchor --------
    cluster_anchor_count = 0
    cluster_label_count = 0
    swap_pop_min = 32
    for cid in clusters:
        info = cluster_info[cid]
        members = info["members"]
        anchors = [m for m in members if m in confirmed]
        cluster_anchor_count += len(anchors)
        cluster_label_count += info["n_labeled"]
        if anchors and info["swap_pop"] < swap_pop_min:
            swap_pop_min = info["swap_pop"]
    if cluster_anchor_count > 0:
        score += 20
        reasons.append(f"cluster has {cluster_anchor_count} confirmed anchors (min pop {swap_pop_min})")
    if cluster_label_count > 0:
        score += 8
        reasons.append(f"cluster has {cluster_label_count} visual labels")

    # -------- 5. level scope (smaller = more constrained vocabulary) --------
    if n_levels == 1:
        score += 12
        reasons.append("single-level (vocab constrained)")
    elif n_levels <= 3:
        score += 6
        reasons.append(f"only {n_levels} levels (vocab somewhat constrained)")
    elif n_levels >= 21:
        score += 4
        reasons.append("common asset (likely generic name)")

    return score, reasons


# ---- 9. Attack hint per ID ------------------------------------------------
def attack_hint(
    floor: int,
    visual: dict | None,
    cohorts: list[str],
    cohort_info: dict,
    clusters: list[str],
    cluster_info: dict,
    confirmed: dict[int, str],
) -> str:
    # Strongest first.
    if visual and visual["role"].lower().startswith("text:"):
        return "exact-text-label"
    for cid in cohorts:
        members = cohort_info[cid]["members"]
        anchors = [m for m in members if m in confirmed]
        if anchors:
            return f"cohort-anchored:{cid}({len(anchors)})"
    for cid in clusters:
        info = cluster_info[cid]
        if any(m in confirmed for m in info["members"]):
            return f"cluster-anchored:{cid}(pop{info['swap_pop']})"
    if visual:
        return f"visual-targeted:{visual['role'][:30]}"
    if floor <= 8:
        return "exhaustive-floor"
    if floor <= 11:
        return "exhaustive-floor+2"
    return "dictionary"


# ---- 10. main -------------------------------------------------------------
def main():
    print("loading data sources…")
    visual = load_visual_roles()
    categories = load_categories()
    id_to_cohorts, cohort_info = load_cohorts()
    id_to_clusters, cluster_info = load_clusters()
    master = load_master()

    all_ids = set(master.keys()) | set(categories.keys())
    print(f"  master ids: {len(master)}")
    print(f"  categorized ids: {len(categories)}")
    print(f"  visual-labeled ids: {len(visual)}")
    print(f"  cohorts: {len(cohort_info)}")
    print(f"  clusters: {len(cluster_info)}")
    print(f"  total unique ids: {len(all_ids)}")
    print(f"  confirmed names: {len(CONFIRMED_NAMES)}")

    # Compute attackability for every id (skip confirmed)
    rows = []
    for idv in sorted(all_ids):
        if idv in CONFIRMED_NAMES:
            continue
        cat = categories.get(idv, {})
        n_levels = int(cat.get("n_levels", 0))
        levels = cat.get("levels", "")
        kind = cat.get("type", master.get(idv, {}).get("kinds", ""))
        floor = floor_of(idv)
        cohorts = id_to_cohorts.get(idv, [])
        clusters = id_to_clusters.get(idv, [])

        v = visual.get(idv)
        score, reasons = attackability_score(
            idv, floor, v, cohorts, cohort_info, clusters, cluster_info, n_levels, CONFIRMED_NAMES,
        )
        hint = attack_hint(floor, v, cohorts, cohort_info, clusters, cluster_info, CONFIRMED_NAMES)

        rows.append({
            "score": round(score, 1),
            "id_hex": f"0x{idv:08x}",
            "id_dec": idv,
            "floor": floor,
            "kind": kind,
            "n_levels": n_levels,
            "levels": levels,
            "n_cohorts": len(cohorts),
            "cohort_ids": ";".join(cohorts),
            "n_clusters": len(clusters),
            "cluster_ids": ";".join(clusters),
            "visual_role": v["role"] if v else "",
            "visual_name_guess": v["possible_name"] if v else "",
            "attack_hint": hint,
            "reasons": " | ".join(reasons),
        })

    rows.sort(key=lambda r: (-r["score"], r["floor"], r["id_dec"]))

    out = ROOT / "docs/analysis/asset-identification/attack_priorities.csv"
    with out.open("w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        w.writeheader()
        for r in rows:
            w.writerow(r)
    print(f"\nwrote {out.relative_to(ROOT)} ({len(rows)} rows)")

    # ---- print floor distribution + top priorities ----
    print(f"\n=== Floor distribution (uncracked, {len(rows)} ids) ===")
    floor_counts = defaultdict(int)
    for r in rows:
        floor_counts[r["floor"]] += 1
    for f in sorted(floor_counts):
        bar = "█" * (floor_counts[f] // 4)
        print(f"  floor={f:2d}: {floor_counts[f]:4d}  {bar}")

    print("\n=== TOP 30 ATTACK TARGETS ===")
    print(f"{'score':>5}  {'id':12} {'floor':>5} {'kind':14} {'hint':28}  reasons")
    for r in rows[:30]:
        print(f"  {r['score']:5.1f}  {r['id_hex']:12} {r['floor']:5d} {r['kind'][:14]:14} {r['attack_hint'][:28]:28}  {r['reasons'][:80]}")

    # ---- cohort anchor table ----
    cohort_anchors = []
    for cid, info in cohort_info.items():
        members = info["members"]
        anchors = [m for m in members if m in CONFIRMED_NAMES]
        if not anchors:
            continue
        unsolved = [m for m in members if m not in CONFIRMED_NAMES]
        cohort_anchors.append({
            "cohort_id": cid,
            "opaque_name": info["opaque_name"],
            "n_members": len(members),
            "n_anchors": len(anchors),
            "n_unsolved": len(unsolved),
            "kinds": info["kinds"],
            "anchor_names": ";".join(f"0x{m:08x}={CONFIRMED_NAMES[m]}" for m in anchors),
            "unsolved_ids": ";".join(f"0x{m:08x}" for m in unsolved),
            "unsolved_floors": ";".join(str(floor_of(m)) for m in unsolved),
        })
    cohort_anchors.sort(key=lambda r: (-r["n_anchors"], -r["n_unsolved"]))
    out2 = ROOT / "docs/analysis/asset-identification/cohort_anchors.csv"
    with out2.open("w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(cohort_anchors[0].keys()))
        w.writeheader()
        for r in cohort_anchors:
            w.writerow(r)
    print(f"\nwrote {out2.relative_to(ROOT)} ({len(cohort_anchors)} cohorts with anchors)")

    print("\n=== COHORTS WITH CONFIRMED ANCHORS ===")
    for r in cohort_anchors[:20]:
        print(f"  {r['cohort_id']} ({r['opaque_name'][:30]}): "
              f"{r['n_anchors']} anchors / {r['n_unsolved']} unsolved")
        print(f"    anchors: {r['anchor_names']}")
        print(f"    unsolved: {r['unsolved_ids']} (floors {r['unsolved_floors']})")

    # ---- cluster anchor table ----
    cluster_anchors = []
    for cid, info in cluster_info.items():
        anchors = [m for m in info["members"] if m in CONFIRMED_NAMES]
        labeled = [m for m in info["members"] if m in visual]
        if not (anchors or labeled):
            continue
        unsolved = [m for m in info["members"] if m not in CONFIRMED_NAMES]
        cluster_anchors.append({
            "cluster_id": cid,
            "class_hex": info["class_hex"],
            "swap_pop": info["swap_pop"],
            "comp_size": info["comp_size"],
            "n_anchors": len(anchors),
            "n_visual_labeled": len(labeled),
            "n_unsolved": len(unsolved),
            "anchor_names": ";".join(f"0x{m:08x}={CONFIRMED_NAMES[m]}" for m in anchors),
            "labeled_examples": " | ".join(f"0x{m:08x}={visual[m]['role'][:20]}" for m in labeled[:3]),
            "unsolved_ids": ";".join(f"0x{m:08x}" for m in unsolved[:30]),
            "swap_examples": info["swap_examples"],
        })
    cluster_anchors.sort(key=lambda r: (-r["n_anchors"], -r["n_visual_labeled"], r["swap_pop"]))
    out3 = ROOT / "docs/analysis/asset-identification/cluster_anchors.csv"
    with out3.open("w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(cluster_anchors[0].keys()))
        w.writeheader()
        for r in cluster_anchors:
            w.writerow(r)
    print(f"\nwrote {out3.relative_to(ROOT)} ({len(cluster_anchors)} cluster components with anchors/labels)")

    print("\n=== TOP 10 ANCHOR-BEARING CLUSTERS (by anchor count) ===")
    for r in cluster_anchors[:10]:
        print(f"  {r['cluster_id']} class={r['class_hex']} pop={r['swap_pop']} "
              f"comp={r['comp_size']} anchors={r['n_anchors']} labeled={r['n_visual_labeled']} "
              f"unsolved={r['n_unsolved']}")


if __name__ == "__main__":
    main()
