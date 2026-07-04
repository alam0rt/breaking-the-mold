#!/usr/bin/env python3
"""Phase 0 evidence tool for docs/plans/sdata-under-split.md.

Maps every symbol in the pooled .sdata / .data asm blob to its owning
translation unit(s) by cross-referencing symbol names against:
  * asm/nonmatchings/<tu>/*.s   (unmatched functions -> TU = dir name)
  * src/*.c                     (matched functions   -> TU = file stem)

Ownership = the set of TUs that reference a symbol. A symbol referenced by
exactly one TU is cleanly attributable; >1 TU means shared (keep in a shared
unnamed subsegment); 0 means unreferenced (data-only / referenced via another
data symbol).

Emits a markdown ownership table to stdout.
"""
from __future__ import annotations

import re
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
NONMATCH = ROOT / "asm" / "nonmatchings"
SRC = ROOT / "src"
SYMS = ROOT / "symbol_addrs.txt"

SDATA_END = 0x800A6120  # first byte past .sdata (== sbss start)
DATA_END = 0x800A5954   # first byte past .data (== sdata start)


def parse_blob(path: Path):
    """Return ordered list of dicts: {sym, vram, off, init} for each dlabel."""
    syms = []
    cur = None
    dlabel_re = re.compile(r"^dlabel\s+(\S+)")
    data_re = re.compile(r"/\*\s*([0-9A-Fa-f]+)\s+([0-9A-Fa-f]+)\s*([0-9A-Fa-f]*)\s*\*/\s*(\S+)")
    for line in path.read_text().splitlines():
        m = dlabel_re.match(line.strip())
        if m:
            cur = {"sym": m.group(1), "vram": None, "off": None, "init": False}
            syms.append(cur)
            continue
        if cur is not None and cur["vram"] is None:
            dm = data_re.search(line)
            if dm:
                cur["off"] = int(dm.group(1), 16)
                cur["vram"] = int(dm.group(2), 16)
                directive = dm.group(4)
                # A .word that resolves to a symbol/nonzero, or any nonzero
                # byte/short => initialized. We mark init True unless it's an
                # all-zero fill; refine below when computing.
                cur["_dir"] = directive
    return syms


def load_friendly_names(path: Path):
    """address(int) -> friendly name, from symbol_addrs.txt."""
    out = {}
    line_re = re.compile(r"^(\w+)\s*=\s*0x([0-9A-Fa-f]+)")
    for line in path.read_text().splitlines():
        m = line_re.match(line.strip())
        if m:
            out[int(m.group(2), 16)] = m.group(1)
    return out


def compute_sizes(syms, end):
    for i, s in enumerate(syms):
        nxt = syms[i + 1]["vram"] if i + 1 < len(syms) else end
        s["size"] = nxt - s["vram"]


def build_ref_index():
    """symbol_name -> set(TU). Scans nonmatchings dirs and src files."""
    idx = defaultdict(set)
    token = re.compile(r"[A-Za-z_]\w*")
    # nonmatchings: TU = immediate subdir under asm/nonmatchings
    for tu_dir in sorted(NONMATCH.iterdir()):
        if not tu_dir.is_dir():
            continue
        tu = tu_dir.name
        for s in tu_dir.rglob("*.s"):
            text = s.read_text(errors="ignore")
            for tok in set(token.findall(text)):
                idx[tok].add(tu)
    # src files: TU = file stem
    for c in sorted(SRC.rglob("*.c")):
        tu = str(c.relative_to(SRC)).removesuffix(".c")
        text = c.read_text(errors="ignore")
        for tok in set(token.findall(text)):
            idx[tok].add(tu)
    return idx


def main():
    which = sys.argv[1] if len(sys.argv) > 1 else "sdata"
    if which == "sdata":
        pattern = "*.sdata.s"
        end = SDATA_END
    else:
        pattern = "*.data.s"
        end = DATA_END

    # Phase 1 split the pooled blob across several per-TU .s files. Read every
    # matching piece and re-order all dlabels by VRAM so the ownership map is
    # complete regardless of how the blob is partitioned.
    syms = []
    for blob in sorted((ROOT / "asm" / "data").glob(pattern)):
        syms.extend(parse_blob(blob))
    syms = [s for s in syms if s["vram"] is not None]
    syms.sort(key=lambda s: s["vram"])
    compute_sizes(syms, end)
    friendly = load_friendly_names(SYMS)
    idx = build_ref_index()

    owned = shared = orphan = 0
    print(f"# {which} ownership map ({len(syms)} symbols)\n")
    print("| vram | size | symbol | friendly | init | owner TU(s) |")
    print("|---|---|---|---|---|---|")
    for s in syms:
        fname = friendly.get(s["vram"], "")
        names = {s["sym"]}
        if fname:
            names.add(fname)
        tus = set()
        for n in names:
            tus |= idx.get(n, set())
        # a symbol's own TU shouldn't count self-defn; nonmatch/src refs are uses
        init = "Y" if s.get("_dir", "").startswith(".word") and s.get("size", 0) else ("Y" if s.get("_dir", "") not in (".byte", ".short", ".word") else "?")
        owners = ",".join(sorted(tus)) if tus else "(none)"
        if len(tus) == 1:
            owned += 1
        elif len(tus) > 1:
            shared += 1
        else:
            orphan += 1
        print(f"| {s['vram']:08X} | {s['size']} | {s['sym']} | {fname} | {init} | {owners} |")

    print(f"\n**Summary:** {owned} single-owner, {shared} shared (>1 TU), {orphan} unreferenced.")


if __name__ == "__main__":
    main()
