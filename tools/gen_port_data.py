#!/usr/bin/env python3
"""
gen_port_data.py -- faithfully transcribe splat `asm/data/*.s` pointer-table
segments (entity/render vtables, callback tables) into C for the PC port.

WHY: the byte-match build assembles `asm/data/*.s` with the MIPS assembler and
the linker resolves each `.word Symbol` entry to the symbol's address. The PC
port can't assemble MIPS, so those data segments are otherwise weak-zeroed by
gen_port_stubs.py -- which nulls every vtable's function-pointer slots, breaking
the render/entity dispatch (`RenderEntities` calls `(*(vtable+0xC))(...)`).

This tool reproduces those segments *faithfully*: same label, same word layout,
same symbol references. A `.word Symbol` becomes `(void *)Symbol`; a `.word 0`
becomes `0`. Each referenced target is declared as an opaque `extern char SYM[];`
so taking its address never conflicts with the real prototype in functions.h
(this TU deliberately includes no game headers) -- the linker binds it to the
port's converted-or-weak-stub function exactly as the MIPS linker would.

Unresolved raw addresses (mid-function labels splat didn't name) are emitted as
local no-op return stubs with a clear UNRESOLVED marker; the two known cases
(0x80025920/0x80025928) are `jr $ra` returns on an off-boot-path vtable.

Only `.word` entries are handled -- these files are pure pointer tables. Mixed
scalar/string data segments are out of scope (assert if encountered).

Usage:  python3 tools/gen_port_data.py <asm/data/file.s> [more.s ...] --out <dir>
"""
from __future__ import annotations

import argparse
import os
import re
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SYMBOL_ADDRS = os.path.join(REPO, "symbol_addrs.txt")

DLABEL_RE = re.compile(r"^\s*dlabel\s+([A-Za-z_][A-Za-z0-9_]*)\s*$")
ENDLABEL_RE = re.compile(r"^\s*enddlabel\s+([A-Za-z_][A-Za-z0-9_]*)\s*$")
# `/* off vram bytes */ .word OPERAND`
WORD_RE = re.compile(r"\.word\s+([A-Za-z_0-9]+)\s*$")
SYMADDR_RE = re.compile(r"^([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(0x[0-9A-Fa-f]+)\s*;")


def load_symbol_addrs() -> dict[int, str]:
    addr2sym: dict[int, str] = {}
    with open(SYMBOL_ADDRS, encoding="utf-8") as fh:
        for line in fh:
            m = SYMADDR_RE.match(line.strip())
            if m:
                addr2sym.setdefault(int(m.group(2), 16), m.group(1))
    return addr2sym


def parse_file(path: str):
    """Yield (label, [operands]) for each dlabel..enddlabel block."""
    blocks = []
    cur_label = None
    cur_words: list[str] = []
    with open(path, encoding="utf-8") as fh:
        for line in fh:
            m = DLABEL_RE.match(line)
            if m:
                cur_label = m.group(1)
                cur_words = []
                continue
            m = ENDLABEL_RE.match(line)
            if m:
                assert cur_label == m.group(1), f"label mismatch in {path}"
                blocks.append((cur_label, cur_words))
                cur_label = None
                continue
            if cur_label is not None:
                w = WORD_RE.search(line)
                if w:
                    cur_words.append(w.group(1))
                elif line.strip() and not line.strip().startswith(("/*", "nonmatching")):
                    # A non-.word data directive in a block we assume is pointers.
                    raise SystemExit(f"{path}: non-pointer data in {cur_label}: {line!r}")
    return blocks


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("files", nargs="+")
    ap.add_argument("--out", required=True, help="output directory")
    args = ap.parse_args()

    addr2sym = load_symbol_addrs()
    os.makedirs(args.out, exist_ok=True)

    for path in args.files:
        blocks = parse_file(path)
        if not blocks:
            continue
        base = os.path.splitext(os.path.basename(path))[0]  # e.g. gfx.rodata
        out_path = os.path.join(args.out, base + ".c")

        referenced: set[str] = set()   # opaque extern char SYM[]
        unresolved: dict[str, int] = {}  # stub name -> address
        body_lines: list[str] = []

        for label, words in blocks:
            entries = []
            for w in words:
                if re.fullmatch(r"0x0+", w):
                    entries.append("0")
                elif w.startswith(("0x", "0X")):
                    val = int(w, 16)
                    if val >= 0x80000000:
                        # PSX RAM range -> a pointer (resolve to a symbol).
                        sym = addr2sym.get(val)
                        if sym:
                            referenced.add(sym)
                            entries.append(f"(void *){sym}")
                        else:
                            stub = f"unresolved_{val:08x}"
                            unresolved[stub] = val
                            entries.append(
                                f"(void *){stub}  /* UNRESOLVED 0x{val:08x} */")
                    else:
                        # Below RAM base -> a scalar constant, kept as a bit
                        # pattern in the pointer-sized slot (faithful layout).
                        entries.append(f"(void *)0x{val:x}")
                else:
                    referenced.add(w)
                    entries.append(f"(void *){w}")
            joined = ",\n    ".join(entries)
            body_lines.append(
                f"void *{label}[{len(entries)}] = {{\n    {joined}\n}};\n"
            )

        with open(out_path, "w", encoding="utf-8") as out:
            out.write(
                "/* =============================================================================\n"
                f" * {base}.c  --  faithful C reproduction of asm/data/{os.path.basename(path)}\n"
                " * -----------------------------------------------------------------------------\n"
                " * GENERATED by tools/gen_port_data.py -- do not edit by hand.\n"
                " * Pointer tables (entity/render vtables) transcribed word-for-word with the\n"
                " * original symbol references; the linker binds each to the port's converted or\n"
                " * weak-stub function, exactly as the MIPS byte-match linker resolves the\n"
                " * `.word Symbol` entries. Targets are declared opaque (extern char SYM[]) so\n"
                " * address-of never conflicts with the real prototype in functions.h.\n"
                " * ========================================================================== */\n\n"
            )
            for stub, addr in sorted(unresolved.items()):
                out.write(
                    f"/* UNRESOLVED mid-function label 0x{addr:08x} (splat left it un-named);\n"
                    f" * faithful behaviour is a bare return. */\n"
                    f"static void {stub}(void) {{}}\n"
                )
            if unresolved:
                out.write("\n")
            for sym in sorted(referenced):
                out.write(f"extern char {sym}[];\n")
            out.write("\n")
            out.write("\n".join(body_lines))

        n_ptr = sum(1 for _l, w in blocks for x in w if not re.fullmatch(r"0x0+", x))
        print(f"{out_path}: {len(blocks)} tables, {len(referenced)} symbols, "
              f"{len(unresolved)} unresolved, {n_ptr} pointer slots")
    return 0


if __name__ == "__main__":
    sys.exit(main())
