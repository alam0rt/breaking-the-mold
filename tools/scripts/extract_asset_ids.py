#!/usr/bin/env python3
"""Extract every 32-bit asset id from Skullmonkeys BLB archives.

The engine refers to sprites, animations and sounds by an opaque 32-bit
name-hash (see docs/reference/asset-hash-ids.md). The hash itself is:

    asset_id(name) = 0x28C0E011 ^ rotl(calcHash(name), 27)

This tool does NOT need the hash; it just walks the BLB container TOCs and
collects the ids that are baked in. It unions several builds so the output is
the most complete id set available (demo + beta + retail by default).

The hash namespace is three id kinds:
  * sprite ids  -- container type 600 (0x258)
  * anim ids    -- 32-bit cross-references embedded inside each sprite entry
  * audio ids   -- container type 601 (0x259)

Container types 400/401/200/... use small sequential indices (palette/clut
slots), not hashes, so they are ignored. A couple of minor id-like container
types (500, 700) are reported separately under --other for completeness.

Usage:
    python3 tools/scripts/extract_asset_ids.py                 # default sources -> CSV
    python3 tools/scripts/extract_asset_ids.py --out ids.csv
    python3 tools/scripts/extract_asset_ids.py --blb a.blb b.blb
    python3 tools/scripts/extract_asset_ids.py --other         # also list type 500/700 ids
"""
from __future__ import annotations

import argparse
import collections
import struct
from pathlib import Path

SECTOR = 2048
ROOT = Path(__file__).resolve().parents[2]

# (label, path) -- the builds to union. Order = preference for "first seen in".
DEFAULT_SOURCES = [
    ("demo", ROOT / "disks/demo/N2/GAME.BLB"),
    ("beta", ROOT / "disks/blb/beta.blb"),
    ("sles", ROOT / "disks/blb/sles-01090.blb"),
]

SPRITE_TYPE = 600       # 0x258 sprite container (entry id = sprite hash)
AUDIO_TYPE = 601        # 0x259 audio container  (entry id = sample hash)
OTHER_HASH_TYPES = {500, 700}   # minor id-like containers, reported under --other


def u16(d: bytes, o: int) -> int:
    return struct.unpack_from("<H", d, o)[0]


def u32(d: bytes, o: int) -> int:
    return struct.unpack_from("<I", d, o)[0]


def segments(d: bytes):
    """Yield (abs_offset, byte_length) for every primary/secondary/tertiary segment."""
    for level in range(26):
        b = level * 0x70
        psec, pcnt = u16(d, b), u16(d, b + 2)
        scnt = u16(d, b + 0x0E)
        if 0 < pcnt < 10000:
            yield psec * SECTOR, pcnt * SECTOR
        if not (0 < scnt <= 6):
            continue
        for st in range(scnt):
            asec, acnt = u16(d, b + 0x1E + st * 2), u16(d, b + 0x2C + st * 2)
            tsec, tcnt = u16(d, b + 0x3A + st * 2), u16(d, b + 0x48 + st * 2)
            if acnt:
                yield asec * SECTOR, acnt * SECTOR
            if tcnt:
                yield tsec * SECTOR, tcnt * SECTOR


def valid_toc(d: bytes, so: int, sl: int):
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


def container_entries(d: bytes, abs_off: int, size: int):
    """Yield (entry_id, entry_size, entry_rel) for a container sub-TOC, or nothing if invalid."""
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


def collect(d: bytes):
    """Return (kinds, other) where kinds[id] is a set of {'sprite','anim','audio'}."""
    kinds = collections.defaultdict(set)
    other = collections.defaultdict(set)   # id -> set of type numbers
    for so, sl in segments(d):
        toc = valid_toc(d, so, sl)
        if toc is None:
            continue
        for typ, size, rel in toc:
            abs_off = so + rel
            if typ == SPRITE_TYPE:
                for eid, esize, erel in container_entries(d, abs_off, size):
                    kinds[eid].add("sprite")
                    # embedded anim cross-references: u16 anim_count, then 12-byte records
                    if erel + 12 <= size:
                        acount = u16(d, abs_off + erel)
                        if 0 < acount < 256 and erel + 12 + acount * 12 <= size:
                            for j in range(acount):
                                aid = u32(d, abs_off + erel + 12 + j * 12)
                                kinds[aid].add("anim")
            elif typ == AUDIO_TYPE:
                for eid, esize, erel in container_entries(d, abs_off, size):
                    kinds[eid].add("audio")
            elif typ in OTHER_HASH_TYPES:
                for eid, esize, erel in container_entries(d, abs_off, size):
                    other[eid].add(typ)
    return kinds, other


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--blb", nargs="+", help="BLB files to scan (default: demo + beta + sles retail)")
    ap.add_argument("--out", default=str(ROOT / "docs/reference/asset-ids-master.csv"),
                    help="output CSV path")
    ap.add_argument("--other", action="store_true", help="also include minor id-like container types (500/700)")
    args = ap.parse_args()

    if args.blb:
        sources = [(Path(p).stem, Path(p).resolve()) for p in args.blb]
    else:
        sources = [(lbl, p) for lbl, p in DEFAULT_SOURCES if p.exists()]

    if not sources:
        ap.error("no BLB sources found")

    merged_kinds = collections.defaultdict(set)     # id -> kinds
    merged_src = collections.defaultdict(set)        # id -> source labels
    merged_other = collections.defaultdict(set)      # id -> type numbers
    other_src = collections.defaultdict(set)

    for label, path in sources:
        kinds, other = collect(path.read_bytes())
        for eid, ks in kinds.items():
            merged_kinds[eid] |= ks
            merged_src[eid].add(label)
        for eid, ts in other.items():
            merged_other[eid] |= ts
            other_src[eid].add(label)
        print(f"{label:6s} {path.name:18s} hash-ids={len(kinds):4d} other-ids={len(other):3d}")

    src_order = [lbl for lbl, _ in sources]

    def sort_src(s):
        return ",".join(x for x in src_order if x in s)

    out = Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    with out.open("w") as f:
        f.write("id_hex,id_dec,kinds,popcount,sources\n")
        for eid in sorted(merged_kinds):
            kinds = "|".join(k for k in ("sprite", "anim", "audio") if k in merged_kinds[eid])
            f.write(f"0x{eid:08x},{eid},{kinds},{bin(eid).count('1')},{sort_src(merged_src[eid])}\n")
        if args.other:
            for eid in sorted(merged_other):
                types = "|".join(f"type{t}" for t in sorted(merged_other[eid]))
                f.write(f"0x{eid:08x},{eid},{types},{bin(eid).count('1')},{sort_src(other_src[eid])}\n")

    # summary
    sprites = sum(1 for v in merged_kinds.values() if "sprite" in v)
    anims = sum(1 for v in merged_kinds.values() if "anim" in v)
    audio = sum(1 for v in merged_kinds.values() if "audio" in v)
    print(f"\nunion hash-ids: {len(merged_kinds)}  (sprite={sprites} anim={anims} audio={audio})")
    if args.other:
        print(f"other id-like (type 500/700): {len(merged_other)}")
    print(f"wrote {out.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
