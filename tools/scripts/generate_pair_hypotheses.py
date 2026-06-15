"""Build pair-hypothesis files using a rotation-class index.

For each suffix-pair (Sa, Sb), precompute V = h_Sa ^ h_Sb and its canonical
rotation class. Index them by class. For each id-pair, look up suffix-pairs
whose V matches the right rotation.
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


def classify(V):
    return min(rotl(V, r) for r in range(32))


SUFFIXES = (
    list("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ")
    + [f"{i}" for i in range(10, 33)]
    + [f"0{i}" for i in range(10)]
    + [f"{i:02d}" for i in range(10, 100)]
    + [f"F{i:02d}" for i in range(40)]
    + [f"FRAME{i}" for i in range(40)]
    + [
        "LEFT", "RIGHT", "UP", "DOWN", "FRONT", "BACK", "NORTH", "SOUTH",
        "EAST", "WEST", "TOP", "BOTTOM", "SIDE", "REAR", "FACE",
        "STAND", "WALK", "RUN", "JUMP", "FALL", "IDLE", "DIE", "ATTACK",
        "ATK", "HURT", "HIT", "SHOOT", "FIRE", "SPIT", "BOUNCE", "SLAM",
        "ROLL", "OPEN", "CLOSE", "START", "END", "INTRO", "OUTRO",
        "OPENING", "CLOSING", "ENTER", "EXIT",
        "L", "R", "U", "D",
        "HEAD", "BODY", "TAIL", "WING", "WINGS", "HORN", "HORNS",
        "EYE", "EYES", "MOUTH", "TEETH", "NOSE", "EAR", "EARS",
        "ARM", "ARMS", "LEG", "LEGS", "FOOT", "FEET", "HAND", "HANDS",
    ]
)


def main():
    sh_data = []
    for s in SUFFIXES:
        h, sh = calc_hash_and_shift(s)
        sh_data.append((s, h, sh))

    cls_index: dict[int, list[tuple[str, str, int]]] = collections.defaultdict(list)
    for i, (Sa, ha, _) in enumerate(sh_data):
        for Sb, hb, _ in sh_data[i + 1:]:
            V = ha ^ hb
            if V == 0:
                continue
            cls = classify(V)
            cls_index[cls].append((Sa, Sb, V))
    print(f"suffix-pair index: {sum(len(v) for v in cls_index.values()):,} entries "
          f"across {len(cls_index):,} classes")

    pcsv = ROOT / "docs/analysis/asset-identification/pair_clusters.csv"
    clusters = collections.defaultdict(list)
    with pcsv.open(newline="") as f:
        for row in csv.DictReader(f):
            cls = int(row["class_hex"], 16)
            a = int(row["id_a"], 16)
            b = int(row["id_b"], 16)
            clusters[cls].append((a, b))

    out_dir = Path("/tmp/pair_hyps")
    out_dir.mkdir(exist_ok=True)
    big = sorted(clusters.items(), key=lambda kv: -len(kv[1]))
    summary = []
    for cls, pairs in big[:40]:
        if len(pairs) < 5:
            break
        hyps = cls_index.get(cls, [])
        if not hyps:
            continue
        lines = []
        for a, b in pairs:
            delta = a ^ b
            for Sa, Sb, V in hyps:
                ok = False
                for sh in range(32):
                    if rotl(V, sh + OUT_ROT) == delta:
                        ok = True
                        break
                if ok:
                    lines.append(f"0x{a:08x},0x{b:08x},{Sa},{Sb}\n")
        out = out_dir / f"cluster_{cls:08x}.txt"
        out.write_text("".join(lines))
        summary.append((cls, len(pairs), len(hyps), len(lines), out))
    for cls, npairs, nhyps, nlines, out in summary:
        print(f"  cls=0x{cls:08x} pairs={npairs:3d} suffix-hyps={nhyps:5d} "
              f"lines={nlines:6d} -> {out}")


if __name__ == "__main__":
    main()
