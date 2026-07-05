#!/usr/bin/env python3
"""Post-splat rodata migration patcher.

splat regenerates asm/ and the linker script from the *base ROM* on every
extract, so it always re-emits jump tables (and other pooled rodata) that belong
to functions we have since matched in C. Once such a function is compiled, its
asm jump table references code labels (.L8005xxxx) that no longer exist, and the
compiler emits its own copy of the table into build/src/<tu>.o(.rodata) that the
linker never pulls. This tool reconciles both sides, idempotently, after splat:

  1. Splits the owning asm rodata file around the migrated symbol, excising the
     symbol's block so it is no longer assembled (part A keeps everything before
     it, a new <name>_b.rodata.s keeps everything after).
  2. Rewrites the linker script so the compiler-emitted rodata lands exactly
     where the excised table was:
         build/asm/data/<tu>.rodata.o(.rodata);      # part A (ends at the hole)
         build/src/<tu>.o(.rodata);                   # migrated table (from C)
         build/asm/data/<tu>_b.rodata.o(.rodata);     # part B

Migrations are declared in tools/rodata_migrations.json. Run automatically from
the Makefile after every splat invocation; safe to run by hand and to run twice.

See docs/plans/rodata-sdata-migration.md and the `rodata-migrate` skill.
"""
from __future__ import annotations

import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
REGISTRY = ROOT / "tools" / "rodata_migrations.json"
LD_SCRIPT = ROOT / "SLES_010.90.ld"

PART_B_HEADER = ['.include "macro.inc"', "", '.section .rodata, "a"', ""]


def split_rodata(rodata_file: Path, symbol: str, part_b_file: Path) -> bool:
    """Excise `symbol`'s block from rodata_file into part A / part_b_file.

    Returns True if a split was performed, False if the symbol was absent
    (already migrated — nothing to do)."""
    lines = rodata_file.read_text().splitlines()

    dlabel = f"dlabel {symbol}"
    enddlabel = f"enddlabel {symbol}"
    nonmatching = f"nonmatching {symbol}"

    try:
        dlabel_idx = next(i for i, l in enumerate(lines) if l.strip() == dlabel)
        end_idx = next(i for i, l in enumerate(lines) if l.strip() == enddlabel)
    except StopIteration:
        return False  # symbol not present — assume already migrated

    # Walk back from the `nonmatching <symbol>` marker over the leading
    # `.align` / blank lines that belong to this block, so part A ends cleanly
    # at the previous table (no dangling .align that would pad it).
    nm_idx = next(
        (i for i, l in enumerate(lines) if l.strip() == nonmatching), dlabel_idx
    )
    a_end = nm_idx
    while a_end - 1 >= 0 and (
        lines[a_end - 1].strip() == "" or lines[a_end - 1].strip().startswith(".align")
    ):
        a_end -= 1

    # part B starts at the first content line after the block (keeping the next
    # table's own leading `.align`).
    b_start = end_idx + 1
    while b_start < len(lines) and lines[b_start].strip() == "":
        b_start += 1

    part_a = lines[:a_end]
    part_b_body = lines[b_start:]

    rodata_file.write_text("\n".join(part_a) + "\n")
    part_b_file.write_text("\n".join(PART_B_HEADER + part_b_body) + "\n")
    return True


def patch_ld(asm_line: str, src_line: str, part_b_line: str) -> None:
    """Replace the single asm rodata linker entry with the 3-line sequence.

    Idempotent. Handles two splat layouts for the migrated TU's own
    `build/src/<tu>.o(.rodata)` line:
      * TU rodata that splat already emits immediately after the asm pool
        (e.g. playst) -- detected as already-patched, left untouched.
      * TU rodata that splat sorts to its natural position elsewhere in the
        pool (e.g. blbacc) -- the stray line is relocated up to sit between
        part A and part B so the compiler-emitted table lands in the hole.
    """
    lines = LD_SCRIPT.read_text().splitlines()

    # Already patched: src_line sits immediately after asm_line.
    for i, l in enumerate(lines):
        if l.strip() == asm_line and i + 1 < len(lines) and lines[i + 1].strip() == src_line:
            return

    # Drop any stray occurrence of the TU rodata line (splat's natural slot),
    # then splice the 3-line sequence in at the asm pool boundary.
    lines = [l for l in lines if l.strip() != src_line]
    out = []
    for l in lines:
        out.append(l)
        if l.strip() == asm_line:
            indent = l[: len(l) - len(l.lstrip())]
            out.append(indent + src_line)
            out.append(indent + part_b_line)
    LD_SCRIPT.write_text("\n".join(out) + "\n")


def main() -> int:
    if not REGISTRY.exists():
        return 0
    migrations = json.loads(REGISTRY.read_text())

    for m in migrations:
        rodata_file = ROOT / m["rodata_file"]
        part_b_file = ROOT / m["part_b_file"]
        if not rodata_file.exists():
            print(f"migrate_rodata: {rodata_file} missing, skipping {m['symbol']}")
            continue
        did = split_rodata(rodata_file, m["symbol"], part_b_file)
        patch_ld(m["ld_asm_line"], m["ld_src_line"], m["ld_part_b_line"])
        status = "split" if did else "already-migrated"
        print(f"migrate_rodata: {m['symbol']} ({status}) -> {m['ld_src_line']}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
