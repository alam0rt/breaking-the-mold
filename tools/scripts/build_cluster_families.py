"""Generate cluster-family reference: every fat rotation class with member IDs
and what suffix swap V it represents."""
import csv
import collections
import sys
from pathlib import Path

sys.path.insert(0, "tools/scripts")
from compound_hash_attack import calc_hash_and_shift, rotl  # type: ignore

ROOT = Path(__file__).resolve().parents[2]
SEED = 0x28C0E011
OUT_ROT = 27


def classify(V: int) -> int:
    return min(rotl(V, r) for r in range(32))


# Vocabulary of known suffix tokens
SWAP_VOCAB = (
    list("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ") +
    [f"{i:02d}" for i in range(100)] +
    [
        "LEFT", "RIGHT", "UP", "DOWN", "FRONT", "BACK", "NORTH", "SOUTH",
        "EAST", "WEST", "TOP", "BOTTOM", "STAND", "WALK", "RUN", "JUMP",
        "FALL", "IDLE", "DIE", "ATTACK", "HURT", "HIT", "SHOOT", "FIRE",
        "SPIT", "BOUNCE", "SLAM", "ROLL", "OPEN", "CLOSE", "START", "END",
        "INTRO", "OUTRO", "OPENING", "CLOSING", "ENTER", "EXIT",
        "L", "R", "U", "D",
        "HEAD", "BODY", "TAIL", "ARM", "LEG", "FOOT", "HAND", "EYE", "EYES",
    ]
)


def main():
    target_info = {}
    with (ROOT / "docs/reference/asset-ids-master.csv").open() as f:
        next(f)
        for line in f:
            c = line.rstrip().split(",")
            if len(c) >= 5:
                target_info[int(c[1])] = {"kinds": c[2], "sources": c[4]}

    # Visual role labels
    ident_path = ROOT / "docs/analysis/asset-identification/sprite_identification_template.csv"
    roles = {}
    if ident_path.exists():
        with ident_path.open(newline="") as f:
            for r in csv.DictReader(f):
                try:
                    i = int(r["sprite_decimal"])
                except Exception:
                    continue
                if r.get("human_role"):
                    roles[i] = r["human_role"]

    # Cluster all id-pair deltas
    ids = sorted(target_info.keys())
    cls_pairs: dict[int, list[tuple[int, int]]] = collections.defaultdict(list)
    for i, a in enumerate(ids):
        for b in ids[i + 1:]:
            d = a ^ b
            cls_pairs[classify(d if d else 0)].append((a, b))

    # Match swap vocab to clusters
    swap_h = [(s, *calc_hash_and_shift(s)) for s in SWAP_VOCAB]
    cls_to_swaps: dict[int, list[tuple[str, str]]] = collections.defaultdict(list)
    for i, (Sa, ha, _) in enumerate(swap_h):
        for Sb, hb, _ in swap_h[i + 1:]:
            V = ha ^ hb
            if V == 0:
                continue
            cls_to_swaps[classify(V)].append((Sa, Sb))

    # Build the report
    out_csv = ROOT / "docs/analysis/asset-identification/cluster_families.csv"
    out_md = ROOT / "docs/analysis/asset-identification/cluster_families.md"

    rows = []
    for cls, pairs in cls_pairs.items():
        if len(pairs) < 4 or cls == 0:
            continue
        # Members of cluster (unique IDs touched)
        members = sorted({a for p in pairs for a in p})
        # Build adjacency: id → connected ids
        adj: dict[int, set[int]] = collections.defaultdict(set)
        for a, b in pairs:
            adj[a].add(b)
            adj[b].add(a)
        # Find connected components
        seen = set()
        components = []
        for m in members:
            if m in seen:
                continue
            stack = [m]
            comp = set()
            while stack:
                cur = stack.pop()
                if cur in seen:
                    continue
                seen.add(cur)
                comp.add(cur)
                stack.extend(adj[cur] - seen)
            components.append(sorted(comp))

        swap_examples = cls_to_swaps.get(cls, [])[:6]
        # Count labeled members per component
        for comp in components:
            labeled = [(i, roles[i]) for i in comp if i in roles]
            kinds = {target_info.get(i, {}).get("kinds", "?") for i in comp}
            rows.append({
                "cluster_class_hex": f"0x{cls:08x}",
                "swap_popcount": bin(cls).count("1"),
                "n_pairs": len(pairs),
                "n_members": len(members),
                "component_size": len(comp),
                "component_kinds": ";".join(sorted(kinds)),
                "n_labeled": len(labeled),
                "labeled_examples": " | ".join(
                    f"0x{i:08x}={role[:40]}" for i, role in labeled[:3]
                ),
                "swap_examples": " | ".join(f"{a}↔{b}" for a, b in swap_examples[:4]),
                "members": ";".join(f"0x{i:08x}" for i in comp[:30]),
                "members_truncated": "yes" if len(comp) > 30 else "no",
            })

    rows.sort(key=lambda r: (-r["component_size"], -r["n_labeled"]))
    out_csv.parent.mkdir(parents=True, exist_ok=True)
    with out_csv.open("w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        w.writeheader()
        for r in rows:
            w.writerow(r)
    print(f"wrote {out_csv.relative_to(ROOT)} ({len(rows)} components)")

    # Markdown summary
    with out_md.open("w") as f:
        f.write("# Asset-hash cluster families\n\n")
        f.write("Auto-generated by `tools/scripts/build_cluster_families.py`.\n\n")
        f.write("Each row is a **connected component** of asset IDs linked through a single\n")
        f.write("suffix-swap rotation class. Members of a component share the same name prefix\n")
        f.write("and differ only by a fixed suffix-pair swap (e.g. `LEFT`↔`RIGHT`,\n")
        f.write("frame-number `01`↔`02`, etc).\n\n")
        f.write("See `docs/reference/asset-hash-ids.md` Round 9 for derivation.\n\n")
        f.write("## Top families (≥5 members)\n\n")
        f.write("| class | pop | n_members | comp_size | labeled | suffix swaps (example) | example labels |\n")
        f.write("|---|---|---|---|---|---|---|\n")
        for r in rows:
            if r["component_size"] < 5:
                continue
            f.write(f"| `{r['cluster_class_hex']}` | "
                    f"{r['swap_popcount']} | "
                    f"{r['n_members']} | "
                    f"{r['component_size']} | "
                    f"{r['n_labeled']} | "
                    f"{r['swap_examples']} | "
                    f"{r['labeled_examples']} |\n")

        f.write("\n\n## All families (full member lists in CSV)\n\n")
        for r in rows:
            if r["component_size"] < 3:
                continue
            f.write(f"### Class `{r['cluster_class_hex']}` "
                    f"(popcount {r['swap_popcount']}, {r['component_size']} members)\n")
            f.write(f"**Suffix swaps (examples):** {r['swap_examples']}\n\n")
            if r["labeled_examples"]:
                f.write(f"**Known labels:** {r['labeled_examples']}\n\n")
            f.write(f"**Members ({r['component_size']}):** {r['members']}\n\n")

    print(f"wrote {out_md.relative_to(ROOT)}")
    print()
    print(f"Total fat components (>=3 ids): {sum(1 for r in rows if r['component_size'] >= 3)}")
    print(f"Components with labeled members: {sum(1 for r in rows if r['n_labeled'])}")


if __name__ == "__main__":
    main()
