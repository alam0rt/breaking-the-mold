#!/usr/bin/env python3
"""
check_asset_ids.py - Deterministic audit of include/Game/asset_ids.h usage.

Every constant in asset_ids.h is a 32-bit asset hash. This tool answers two
questions with zero guesswork:

  1. COVERAGE: which constants are referenced by name in src/*.c, and which are
     not (the remaining backlog).
  2. MAGIC NUMBERS: where a raw hex literal in src/*.c CODE (comments stripped)
     equals a known asset id but is NOT written as the named constant. These are
     the actionable replacements -- and they survive sloppy formatting, e.g.
     `0x9382152` (no leading zero) still matches `0x09382152u`.

Exit status is non-zero if any magic numbers remain, so it can gate CI / a
pre-commit hook. Use --quiet to print only the magic-number findings.

Because these ids are 32-bit hashes, distinct assets can share a value (e.g.
0xc00200c9 is both FX_GUM_PIERCE_DN and an anim keyframe). When a raw literal
only *collides* with an asset id and is genuinely something else, put the marker
NOLINT_ASSET_ID in a comment on that line to suppress the false positive.

Usage:
  tools/check_asset_ids.py            # full report
  tools/check_asset_ids.py --quiet    # only actionable magic numbers
  tools/check_asset_ids.py --unused   # also list every still-unused constant
"""
import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
HEADER = ROOT / "include" / "Game" / "asset_ids.h"
SRC_DIR = ROOT / "src"

DEFINE_RE = re.compile(r"^\s*#define\s+([A-Za-z_][A-Za-z0-9_]*)\s+(0x[0-9a-fA-F]+)u?\b")
HEX_RE = re.compile(r"\b0x[0-9a-fA-F]+\b")
IDENT_RE = re.compile(r"[A-Za-z_][A-Za-z0-9_]*")


def strip_comments(text):
    """Remove /* */ and // comments while preserving newlines/offsets roughly.

    Block comments collapse to spaces so line numbers from splitlines stay sane
    for the surviving code. String literals are irrelevant here (no asset-id
    string literals exist) so we don't bother protecting them."""
    out = []
    i, n = 0, len(text)
    while i < n:
        two = text[i:i + 2]
        if two == "/*":
            j = text.find("*/", i + 2)
            if j == -1:
                j = n
            else:
                j += 2
            out.append("".join(c if c == "\n" else " " for c in text[i:j]))
            i = j
        elif two == "//":
            j = text.find("\n", i)
            if j == -1:
                j = n
            out.append(" " * (j - i))
            i = j
        else:
            out.append(text[i])
            i += 1
    return "".join(out)


def parse_header():
    """name -> int value, and value -> name (last wins, but values are unique)."""
    name_to_val = {}
    val_to_name = {}
    for line in HEADER.read_text().splitlines():
        m = DEFINE_RE.match(line)
        if not m:
            continue
        name, hexstr = m.group(1), m.group(2)
        val = int(hexstr, 16) & 0xFFFFFFFF
        name_to_val[name] = val
        val_to_name.setdefault(val, name)
    return name_to_val, val_to_name


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--quiet", action="store_true", help="only print magic numbers")
    ap.add_argument("--unused", action="store_true", help="list still-unused constants")
    args = ap.parse_args()

    name_to_val, val_to_name = parse_header()
    if not name_to_val:
        print("error: no #defines parsed from asset_ids.h", file=sys.stderr)
        return 2

    used_names = set()
    magic = []  # (file, lineno, literal, expected_name, value)

    for cfile in sorted(SRC_DIR.glob("*.c")):
        raw_lines = cfile.read_text().splitlines()
        code = strip_comments(cfile.read_text())
        for lineno, line in enumerate(code.splitlines(), 1):
            for ident in IDENT_RE.findall(line):
                if ident in name_to_val:
                    used_names.add(ident)
            # A line may legitimately carry a hex literal that only *collides*
            # in value with an asset id (hashes are not unique). Mark such lines
            # with the NOLINT_ASSET_ID comment to suppress the false positive.
            if "NOLINT_ASSET_ID" in raw_lines[lineno - 1]:
                continue
            for hexlit in HEX_RE.findall(line):
                val = int(hexlit, 16) & 0xFFFFFFFF
                if val in val_to_name:
                    magic.append((cfile.relative_to(ROOT), lineno, hexlit,
                                  val_to_name[val], val))

    total = len(name_to_val)
    used = len(used_names)
    unused = sorted(set(name_to_val) - used_names)

    if magic:
        print(f"MAGIC NUMBERS ({len(magic)}) -- raw literals that should use a constant:")
        for f, ln, lit, name, _ in magic:
            print(f"  {f}:{ln}: {lit:>12} -> {name}")
        print()

    if not args.quiet:
        print(f"COVERAGE: {used}/{total} constants referenced by name in src/*.c "
              f"({total - used} unused).")
        if args.unused and unused:
            print("\nUNUSED constants (backlog -- many are consumed by asm/-resident "
                  "functions and may never appear in src/):")
            for name in unused:
                print(f"  {name:<34} 0x{name_to_val[name]:08x}")

    return 1 if magic else 0


if __name__ == "__main__":
    sys.exit(main())
