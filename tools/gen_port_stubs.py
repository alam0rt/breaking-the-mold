#!/usr/bin/env python3
"""
gen_port_stubs.py -- emit the weak-stub / weak-global backing TUs for the PC port.

The native (PC) build reuses every matched C function as-is and turns every
still-`INCLUDE_ASM` MIPS function into an empty statement (see
port/include/port_config.h). That leaves two classes of symbols undefined at
link time:

  1. Functions whose only body is raw MIPS (asm/nonmatchings/<seg>/<Func>.s).
     Something still *calls* them, so the link needs a definition.
  2. Globals bound to absolute PSX RAM addresses via `... asm("D_xxxxxxxx")`.
     On MIPS the linker script places them; on PC that alias merely renames the
     symbol, so the native link needs real backing storage.

This script generates two additive translation units:

  port/decomp/_autostubs.c    weak `void NAME(void){ port_stub("NAME"); }` for
                              every asm-only function name.
  port/decomp/_autoglobals.c  weak `unsigned char storage[SIZE] asm("D_...");`
                              for every `asm("...")` DATA alias in src.

Both are *weak*: as soon as a real (matched or functional-C) body / definition
is linked in, it silently overrides the placeholder. This lets the whole tree
link at once and improves monotonically as functions are converted.

Nothing here touches the MIPS byte-match build -- these files live under port/
and are only compiled by the CMake PC target.

Usage:  python3 tools/gen_port_stubs.py [--check]
        --check : fail (non-zero) if the generated files are stale.
"""
from __future__ import annotations

import argparse
import os
import re
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ASM_DIR = os.path.join(REPO, "asm", "nonmatchings")
SRC_DIRS = [os.path.join(REPO, "src"), os.path.join(REPO, "src", "libs")]
SYMBOL_ADDRS = os.path.join(REPO, "symbol_addrs.txt")
OUT_DIR = os.path.join(REPO, "port", "decomp")
STUBS_OUT = os.path.join(OUT_DIR, "_autostubs.c")
GLOBALS_OUT = os.path.join(OUT_DIR, "_autoglobals.c")

# Weak global backing size heuristics.
GLOBAL_SIZE_DEFAULT = 0x400
GLOBAL_SIZE_MIN = 0x10
GLOBAL_SIZE_MAX = 0x2000

# libc routines the game re-implements in asm. On PC these must resolve to the
# host C library (Phase 0) or port/spec/bios.c (Phase 1) -- NOT a `void X(void)`
# weak stub, which clashes with the compiler builtins. Excluded from stubs.
LIBC_PROVIDED = {
    "memcpy", "memmove", "memset", "memcmp",
    "printf", "sprintf",
    "strcat", "strcmp", "strcpy", "strncmp", "strncpy", "strlen",
    "malloc", "calloc", "realloc", "free",
    "rand", "srand", "abs", "labs", "bcopy", "bzero", "qsort", "atoi",
}

# Symbols the PC runtime owns directly (port/spec/host_main.c). The game's PSX
# frame-loop entry is also named `main`; on PC the host process `main()` is the
# real entry, so the asm `main` must NOT get a weak `void main(void)` stub (it
# would clash with `int main(void)`). It is wired up in Phase 2.
RUNTIME_PROVIDED = {"main"}

EXCLUDED = LIBC_PROVIDED | RUNTIME_PROVIDED

# Only these alias prefixes are treated as DATA globals. Function aliases
# (e.g. asm("PlaySoundEffect")) are covered by the function stubs instead.
DATA_ALIAS_RE = re.compile(r'asm\("((?:D_|B_|g_|jpt_|jtbl_)[A-Za-z0-9_]+)"\)')
INCLUDE_ASM_RE = re.compile(r'\bINCLUDE_ASM\s*\(\s*"[^"]*"\s*,\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)')


def parse_symbol_addrs() -> dict[str, tuple[int, int]]:
    """name -> (address, size_bytes).

    Size comes from an explicit `// size:0xNN` (or `// size:NN`) comment when
    present, otherwise from the gap to the next distinct address (clamped).
    Only real RAM symbols (addr >= 0x80000000) are kept -- this drops the GTE
    macro pseudo-symbols (0x2000xxxx) and tiny sentinels like D_3/D_4.
    """
    line_re = re.compile(
        r"^\s*([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(0x[0-9A-Fa-f]+)\s*;"
        r"(?:.*?\bsize:\s*(0x[0-9A-Fa-f]+|\d+))?"
    )
    raw: list[tuple[str, int, int | None]] = []
    with open(SYMBOL_ADDRS, encoding="utf-8") as f:
        for line in f:
            m = line_re.match(line)
            if not m:
                continue
            name = m.group(1)
            addr = int(m.group(2), 16)
            if addr < 0x80000000:
                continue
            sz = m.group(3)
            explicit = int(sz, 0) if sz else None
            raw.append((name, addr, explicit))

    # Gap-derived fallback size needs a sorted address view.
    by_addr = sorted(raw, key=lambda e: e[1])
    out: dict[str, tuple[int, int]] = {}
    for i, (name, addr, explicit) in enumerate(by_addr):
        if explicit is not None:
            size = explicit
        else:
            size = GLOBAL_SIZE_DEFAULT
            for j in range(i + 1, len(by_addr)):
                if by_addr[j][1] > addr:
                    size = by_addr[j][1] - addr
                    break
        size = max(GLOBAL_SIZE_MIN, min(GLOBAL_SIZE_MAX, size))
        prev = out.get(name)
        if prev is None or size > prev[1]:
            out[name] = (addr, size)
    return out


def collect_asm_functions() -> list[str]:
    """Every function name that only has a raw-MIPS body."""
    names: set[str] = set()
    for root, _dirs, files in os.walk(ASM_DIR):
        for fn in files:
            if fn.endswith(".s"):
                names.add(fn[:-2])
    # Also sweep INCLUDE_ASM references in case a name lacks a .s on disk.
    for d in SRC_DIRS:
        if not os.path.isdir(d):
            continue
        for root, _dirs, files in os.walk(d):
            for fn in files:
                if not fn.endswith(".c"):
                    continue
                with open(os.path.join(root, fn), encoding="utf-8", errors="replace") as f:
                    for m in INCLUDE_ASM_RE.finditer(f.read()):
                        names.add(m.group(1))
    return sorted(names - EXCLUDED)


def collect_data_aliases() -> set[str]:
    aliases: set[str] = set()
    for d in SRC_DIRS:
        if not os.path.isdir(d):
            continue
        for root, _dirs, files in os.walk(d):
            for fn in files:
                if not fn.endswith(".c"):
                    continue
                with open(os.path.join(root, fn), encoding="utf-8", errors="replace") as f:
                    for m in DATA_ALIAS_RE.finditer(f.read()):
                        aliases.add(m.group(1))
    return aliases


def collect_backing_globals(
    sym_sizes: dict[str, tuple[int, int]],
    asm_funcs: set[str],
) -> list[tuple[str, int]]:
    """Every symbol that the MIPS linker script provides at an absolute address
    and that the native link must therefore back with real storage.

    Source = symbol_addrs.txt RAM symbols  UNION  the `asm("D_..")` aliases in
    src. We back them as WEAK storage so any strong C definition (a matched
    function, a real global) silently overrides. Names already stubbed as
    functions (asm-only bodies) are skipped -- they get a weak func stub instead.
    """
    names: set[str] = set(sym_sizes.keys()) | collect_data_aliases()
    names -= asm_funcs
    names -= EXCLUDED
    out: list[tuple[str, int]] = []
    for name in sorted(names):
        size = sym_sizes.get(name, (0, GLOBAL_SIZE_DEFAULT))[1]
        out.append((name, max(GLOBAL_SIZE_MIN, size)))
    return out


HEADER = """\
/* =============================================================================
 * {name}  --  GENERATED by tools/gen_port_stubs.py. DO NOT EDIT.
 * Part of the PC port (TARGET_PC). Weak placeholders that any real body /
 * definition overrides. Regenerate with `python3 tools/gen_port_stubs.py`.
 * ========================================================================== */
"""


def render_stubs(funcs: list[str]) -> str:
    out = [HEADER.format(name="_autostubs.c")]
    out.append('void port_stub(const char *fn);\n')
    out.append("/* {} asm-only functions */\n".format(len(funcs)))
    for name in funcs:
        out.append(
            '__attribute__((weak)) void {n}(void) {{ port_stub("{n}"); }}\n'.format(n=name)
        )
    return "".join(out)


def render_globals(globs: list[tuple[str, int]]) -> str:
    out = [HEADER.format(name="_autoglobals.c")]
    out.append(
        "/* {} absolute-address globals (weak backing storage; strong C defs "
        "win) */\n".format(len(globs))
    )
    for i, (sym, size) in enumerate(globs):
        out.append(
            '__attribute__((weak)) unsigned char _autoglob_{i}[0x{sz:X}] '
            'asm("{sym}");\n'.format(i=i, sz=size, sym=sym)
        )
    return "".join(out)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", action="store_true",
                    help="exit non-zero if generated files are stale")
    args = ap.parse_args()

    sym_sizes = parse_symbol_addrs()
    funcs = collect_asm_functions()
    globs = collect_backing_globals(sym_sizes, set(funcs))

    stubs = render_stubs(funcs)
    globs_txt = render_globals(globs)

    if args.check:
        stale = False
        for path, content in ((STUBS_OUT, stubs), (GLOBALS_OUT, globs_txt)):
            existing = ""
            if os.path.exists(path):
                with open(path, encoding="utf-8") as f:
                    existing = f.read()
            if existing != content:
                print("stale: {}".format(path), file=sys.stderr)
                stale = True
        return 1 if stale else 0

    os.makedirs(OUT_DIR, exist_ok=True)
    with open(STUBS_OUT, "w", encoding="utf-8") as f:
        f.write(stubs)
    with open(GLOBALS_OUT, "w", encoding="utf-8") as f:
        f.write(globs_txt)
    print("wrote {} ({} stubs)".format(STUBS_OUT, len(funcs)))
    print("wrote {} ({} globals)".format(GLOBALS_OUT, len(globs)))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
