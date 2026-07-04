#!/usr/bin/env python3
"""Split glued function blobs in asm/nonmatchings into per-function .s files.

Why this exists
---------------
spimdisasm derives function starts from control-flow analysis (direct `jal`
targets + boundary heuristics). Functions that are only reachable indirectly
(via function-pointer / state-machine descriptor tables) are never seen as
function starts. When such a run of functions is *preceded* by a sub-8-byte
fragment at a segment boundary, spimdisasm's boundary detector stalls on that
fragment and glues the entire run into a single .s file, emitting the real
sub-functions as interior `alabel`s instead of separate `glabel` files.

`// type:func` annotations in symbol_addrs.txt cannot fix this: the size-based
end-detection needs size >= 8, and the +8 look-ahead misses a successor sitting
at +4, so the cascade never starts.

This tool post-processes the extracted asm and splits every such blob at its
function boundaries (`glabel` / `alabel` lines) into one file per function.

Byte-safety
-----------
Splitting only *moves identical instruction bytes into separate files*. The
assembler emits the same object bytes and the linker concatenates them in
INCLUDE_ASM order, so the final ROM is byte-identical. `alabel` (`.aent`,
alternative entry) and `glabel` (`.ent`) both declare a global @function symbol
at the same address, so promoting an `alabel` to a `glabel` in its own file is
faithful.

Idempotent: `make extract` wipes asm/ first, so this always runs on clean
spimdisasm output.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

# Matches an emitted instruction/word line: `    /* VROM VRAM BYTES */  ...`
INSTR_RE = re.compile(r"^\s*/\*\s*[0-9A-Fa-f]+\s+[0-9A-Fa-f]+\s+[0-9A-Fa-f]+\s*\*/")
# Function-boundary lines produced by the macro.inc `glabel` / `alabel` macros.
BOUNDARY_RE = re.compile(r"^\s*(glabel|alabel)\s+(\S+)")
HEADER_RE = re.compile(r"^\s*nonmatching\s+\S+")
ENDLABEL_RE = re.compile(r"^\s*endlabel\s+\S+")


def find_boundaries(lines: list[str]) -> list[tuple[int, str]]:
    """Return (line_index, func_name) for each glabel/alabel boundary."""
    out = []
    for i, line in enumerate(lines):
        m = BOUNDARY_RE.match(line)
        if m:
            out.append((i, m.group(2)))
    return out


def instr_count(body: list[str]) -> int:
    return sum(1 for ln in body if INSTR_RE.match(ln))


def render_function(name: str, body: list[str]) -> str:
    """Rebuild a clean single-function .s from body lines (verbatim, minus
    boundary/header/endlabel lines and edge blanks)."""
    # Strip leading/trailing blank lines from the body.
    start, end = 0, len(body)
    while start < end and body[start].strip() == "":
        start += 1
    while end > start and body[end - 1].strip() == "":
        end -= 1
    body = body[start:end]

    size = instr_count(body) * 4
    lines = [f"nonmatching {name}, 0x{size:X}", "", f"glabel {name}"]
    lines.extend(body)
    lines.append(f"endlabel {name}")
    return "\n".join(lines) + "\n"


def split_file(path: Path, dry_run: bool) -> list[str]:
    """Split one blob .s into per-function files. Returns the ordered list of
    function names it contains (>= 2 means it was a blob and was split)."""
    text = path.read_text()
    lines = text.split("\n")
    if lines and lines[-1] == "":
        lines = lines[:-1]  # drop trailing empty from split

    boundaries = find_boundaries(lines)
    names = [name for _, name in boundaries]
    if len(boundaries) < 2:
        return names  # single function (or none) -> not a blob, leave as-is

    # Body of function k = lines strictly between boundary[k] and boundary[k+1]
    # (exclusive of the boundary line itself), dropping header/endlabel lines.
    segments: list[tuple[str, list[str]]] = []
    for k, (idx, name) in enumerate(boundaries):
        seg_end = boundaries[k + 1][0] if k + 1 < len(boundaries) else len(lines)
        body = []
        for ln in lines[idx + 1 : seg_end]:
            if HEADER_RE.match(ln) or ENDLABEL_RE.match(ln):
                continue
            body.append(ln)
        segments.append((name, body))

    directory = path.parent
    first_name = segments[0][0]
    for name, body in segments:
        out_path = directory / f"{name}.s"
        content = render_function(name, body)
        if dry_run:
            print(f"  would write {out_path} "
                  f"({instr_count(body) * 4:#x} bytes)")
        else:
            out_path.write_text(content)

    # If the blob's own filename does not match its first function, the original
    # file is now redundant (its content was rewritten under first_name.s).
    if not dry_run and path.stem != first_name:
        path.unlink()

    return names


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("root", nargs="?", default="asm/nonmatchings",
                    help="Directory to scan for glued .s blobs.")
    ap.add_argument("--only", action="append", default=[],
                    help="Restrict to .s files whose path contains this substring "
                         "(repeatable). Useful for validating one blob.")
    ap.add_argument("--dry-run", action="store_true",
                    help="Report what would be split without writing.")
    args = ap.parse_args()

    root = Path(args.root)
    if not root.is_dir():
        print(f"error: {root} is not a directory", file=sys.stderr)
        return 1

    split_count = 0
    fn_total = 0
    for path in sorted(root.rglob("*.s")):
        rel = str(path)
        if args.only and not any(o in rel for o in args.only):
            continue
        names = split_file(path, args.dry_run)
        if len(names) >= 2:
            split_count += 1
            fn_total += len(names)
            verb = "would split" if args.dry_run else "split"
            print(f"{verb} {path.relative_to(root)} -> {len(names)} functions: "
                  f"{', '.join(names)}")

    action = "would split" if args.dry_run else "Split"
    print(f"{action} {split_count} blob(s) into {fn_total} functions.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
