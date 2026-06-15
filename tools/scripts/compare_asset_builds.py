#!/usr/bin/env python3
"""Cross-build asset-id comparison.

Compares every BLB build under disks/blb/ + the demo and reports per-build
inventory, pairwise differences, equivalence classes, and a detailed
per-id presence matrix.

Output:
  - /tmp/build_compare.log  (textual summary)
  - docs/reference/asset-ids-cross-build.csv  (per-id presence matrix)
"""
from __future__ import annotations

import collections
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SECTOR = 2048
SPRITE_TYPE = 600
AUDIO_TYPE = 601

BUILDS = [
    ("beta",    ROOT / "disks/blb/beta.blb"),
    ("demo_eu", ROOT / "disks/demo/N2/GAME.BLB"),
    ("demo_jp", ROOT / "disks/blb/papx-90053.blb"),
    ("pal_d1",  ROOT / "disks/blb/sles-01090.blb"),
    ("pal_d2",  ROOT / "disks/blb/sles-01091.blb"),
    ("pal_d3",  ROOT / "disks/blb/sles-01092.blb"),
    ("jp",      ROOT / "disks/blb/slps-01501.blb"),
    ("us",      ROOT / "disks/blb/slus-00601.blb"),
]


def u16(d, o):
    return struct.unpack_from("<H", d, o)[0]


def u32(d, o):
    return struct.unpack_from("<I", d, o)[0]


def segments(d):
    for level in range(26):
        b = level * 0x70
        psec, pcnt = u16(d, b), u16(d, b + 2)
        scnt = u16(d, b + 0x0E)
        if 0 < pcnt < 10000:
            yield level, psec * SECTOR, pcnt * SECTOR
        if not (0 < scnt <= 6):
            continue
        for st in range(scnt):
            asec, acnt = u16(d, b + 0x1E + st * 2), u16(d, b + 0x2C + st * 2)
            tsec, tcnt = u16(d, b + 0x3A + st * 2), u16(d, b + 0x48 + st * 2)
            if acnt:
                yield level, asec * SECTOR, acnt * SECTOR
            if tcnt:
                yield level, tsec * SECTOR, tcnt * SECTOR


def valid_toc(d, so, sl):
    if so < 0 or so + 4 > len(d):
        return None
    count = u32(d, so)
    if not (0 < count < 64) or so + 4 + count * 12 > len(d):
        return None
    entries = []
    for i in range(count):
        typ, size, rel = struct.unpack_from("<III", d, so + 4 + i * 12)
        if typ > 2000 or rel < 4 + count * 12 or rel + size > sl or so + rel + size > len(d):
            return None
        entries.append((typ, size, rel))
    return entries


def container_entries(d, abs_off, size):
    if size < 4 or abs_off + 4 > len(d):
        return
    count = u32(d, abs_off)
    if not (0 <= count < 2000) or 4 + count * 12 > size:
        return
    for i in range(count):
        eid, esize, erel = struct.unpack_from("<III", d, abs_off + 4 + i * 12)
        if erel + esize > size:
            return
        yield eid, esize, erel


def collect(path):
    d = path.read_bytes()
    kinds = collections.defaultdict(set)
    sizes = collections.defaultdict(set)  # id -> set of sizes seen
    for _level, so, sl in segments(d):
        toc = valid_toc(d, so, sl)
        if toc is None:
            continue
        for typ, size, rel in toc:
            abs_off = so + rel
            if typ == SPRITE_TYPE:
                for eid, esize, erel in container_entries(d, abs_off, size):
                    kinds[eid].add("sprite")
                    sizes[eid].add(esize)
                    if erel + 12 <= size:
                        acount = u16(d, abs_off + erel)
                        if 0 < acount < 256 and erel + 12 + acount * 12 <= size:
                            for j in range(acount):
                                aid = u32(d, abs_off + erel + 12 + j * 12)
                                kinds[aid].add("anim")
            elif typ == AUDIO_TYPE:
                for eid, esize, erel in container_entries(d, abs_off, size):
                    kinds[eid].add("audio")
                    sizes[eid].add(esize)
    return kinds, sizes


def main():
    log_lines = []

    def log(msg=""):
        print(msg)
        log_lines.append(msg)

    inv = {}
    inv_sizes = {}
    for lbl, p in BUILDS:
        if not p.exists():
            log(f"SKIP {lbl}: {p} not found")
            continue
        log(f"scanning {lbl}: {p}")
        kinds, sizes = collect(p)
        inv[lbl] = kinds
        inv_sizes[lbl] = sizes

    sets = {lbl: set(k.keys()) for lbl, k in inv.items()}
    union = set().union(*sets.values())

    log()
    log(f"=== UNION: {len(union)} unique asset ids across {len(inv)} builds ===")
    log()
    log("Per-build counts:")
    for lbl in inv:
        s = sets[lbl]
        sprites = sum(1 for i in s if 'sprite' in inv[lbl][i])
        anims   = sum(1 for i in s if 'anim'   in inv[lbl][i])
        audio   = sum(1 for i in s if 'audio'  in inv[lbl][i])
        log(f"  {lbl:10s}  total={len(s):4d}   sprite={sprites:3d}  anim={anims:3d}  audio={audio:3d}")

    log()
    log("Pairwise differences:")
    labels = list(inv.keys())
    for i, a in enumerate(labels):
        for b in labels[i+1:]:
            sa, sb = sets[a], sets[b]
            only_a = sa - sb
            only_b = sb - sa
            shared = sa & sb
            tag = "  (IDENTICAL)" if not only_a and not only_b else ""
            log(f"  {a:10s} vs {b:10s}: shared={len(shared):4d}  {a}-only={len(only_a):4d}  {b}-only={len(only_b):4d}{tag}")

    log()
    log("=== Equivalence classes (builds with identical id sets) ===")
    groups = collections.defaultdict(list)
    for lbl, s in sets.items():
        groups[frozenset(s)].append(lbl)
    for i, (s, lbls) in enumerate(sorted(groups.items(), key=lambda kv: -len(kv[1]))):
        log(f"  class {i}: {len(s)} ids, builds: {', '.join(lbls)}")

    log()
    log("=== Key differences ===")
    if 'pal_d1' in sets and 'jp' in sets:
        pal_only = sets['pal_d1'] - sets['jp']
        jp_only  = sets['jp'] - sets['pal_d1']
        log(f"PAL only (not in JP): {len(pal_only)}")
        for eid in sorted(pal_only)[:20]:
            kk = "|".join(sorted(inv['pal_d1'][eid]))
            log(f"  0x{eid:08x}  {kk}")
        log(f"JP only (not in PAL): {len(jp_only)}")
        for eid in sorted(jp_only)[:20]:
            kk = "|".join(sorted(inv['jp'][eid]))
            log(f"  0x{eid:08x}  {kk}")
    if 'beta' in sets and 'pal_d1' in sets:
        bp = sets['beta'] - sets['pal_d1']
        pb = sets['pal_d1'] - sets['beta']
        log(f"Beta only (not in PAL retail): {len(bp)}")
        for eid in sorted(bp)[:20]:
            log(f"  0x{eid:08x}  {'|'.join(sorted(inv['beta'][eid]))}")
        log(f"PAL retail only (not in beta): {len(pb)}")
        for eid in sorted(pb)[:20]:
            log(f"  0x{eid:08x}  {'|'.join(sorted(inv['pal_d1'][eid]))}")

    # Detailed CSV
    out_csv = ROOT / "docs/reference/asset-ids-cross-build.csv"
    with out_csv.open("w") as f:
        cols = ["id_hex", "id_dec", "kinds", "popcount", "n_builds"] + list(inv.keys())
        f.write(",".join(cols) + "\n")
        for eid in sorted(union):
            kinds_union = set()
            for lbl in inv:
                kinds_union |= inv[lbl].get(eid, set())
            kinds_str = "|".join(k for k in ("sprite", "anim", "audio") if k in kinds_union)
            present = [("1" if eid in sets[lbl] else "0") for lbl in inv]
            n_builds = sum(int(x) for x in present)
            row = [f"0x{eid:08x}", str(eid), kinds_str, str(bin(eid).count('1')), str(n_builds)] + present
            f.write(",".join(row) + "\n")
    log(f"\nwrote {out_csv.relative_to(ROOT)}")

    log_path = Path("/tmp/build_compare.log")
    log_path.write_text("\n".join(log_lines) + "\n")
    print(f"\nlog: {log_path}")


if __name__ == "__main__":
    main()
