#!/usr/bin/env python3
"""
portize.py -- sweep the bare MIPS register pins to the PSX_REG() wrapper.

A few matched functions pin a value to a specific hardware register to reproduce
the original instruction scheduler, e.g.

    register s16 m1 asm("$17");
    register void (*fn)() asm("$3") = (void (*)())EntityUpdateCallback;

Those `asm("$N")` clauses are MIPS-only and stop the shared C from parsing on
x86. This tool rewrites just the clause to PSX_REG("$N"):

    register s16 m1 PSX_REG("$17");
    register void (*fn)() PSX_REG("$3") = (void (*)())EntityUpdateCallback;

PSX_REG is defined in include/common.h (gated on TARGET_PC): it expands to
`asm("$N")` on MIPS (byte-identical to the original tokens, so `make check`
stays green) and to nothing on the PC build.

Only genuine declaration statements are touched: a line is rewritten iff, after
leading whitespace, it starts with the `register` keyword. Comment lines and
doc-strings that merely mention `register ... asm("$N")` are left untouched.

Idempotent. Usage:
    python3 tools/portize.py            # apply the sweep in place
    python3 tools/portize.py --check    # exit non-zero if any pin still bare
    python3 tools/portize.py --dry-run  # report what would change
"""
from __future__ import annotations

import argparse
import os
import re
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(REPO, "src")

# A real pin statement: optional indentation, then the `register` storage class,
# somewhere an `asm("$<digits>")` clause. Comment lines start with `*` or `//`.
PIN_LINE_RE = re.compile(r'^\s*register\b.*\basm\("\$[0-9]+"\)')
CLAUSE_RE = re.compile(r'\basm\("(\$[0-9]+)"\)')

# Empty-asm scheduling fence with a MIPS register clobber, e.g.
#     __asm__ volatile("" ::: "$5");
# The register name is invalid on x86, so route it through PSX_CLOBBER("$N").
CLOBBER_LINE_RE = re.compile(
    r'^(\s*)__asm__\s+(?:volatile|__volatile__)\s*\(\s*""\s*:::\s*"(\$[0-9]+)"\s*\)\s*;'
)


def rewrite_line(line: str) -> tuple[str, int]:
    """Return (new_line, num_clauses_rewritten) for one source line."""
    m = CLOBBER_LINE_RE.match(line)
    if m:
        indent, reg = m.group(1), m.group(2)
        return '{}PSX_CLOBBER("{}");\n'.format(indent, reg), 1
    if PIN_LINE_RE.match(line):
        new_line, n = CLAUSE_RE.subn(r'PSX_REG("\1")', line)
        if n:
            return new_line, n
    return line, 0


def iter_c_files():
    for root, _dirs, files in os.walk(SRC):
        for fn in files:
            if fn.endswith((".c", ".h")):
                yield os.path.join(root, fn)


def process(text: str) -> tuple[str, int]:
    out_lines = []
    changed = 0
    for line in text.splitlines(keepends=True):
        new_line, n = rewrite_line(line)
        changed += n
        out_lines.append(new_line)
    return "".join(out_lines), changed


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()

    total = 0
    touched_files = []
    for path in iter_c_files():
        with open(path, encoding="utf-8", errors="replace") as f:
            text = f.read()
        new_text, changed = process(text)
        if changed:
            total += changed
            touched_files.append((path, changed))
            if not (args.check or args.dry_run):
                with open(path, "w", encoding="utf-8") as f:
                    f.write(new_text)

    if args.check:
        if total:
            for path, n in touched_files:
                print("bare pin(s) remaining: {} ({})".format(path, n), file=sys.stderr)
            return 1
        return 0

    verb = "would rewrite" if args.dry_run else "rewrote"
    for path, n in touched_files:
        print("{} {} pin(s) in {}".format(verb, n, os.path.relpath(path, REPO)))
    print("{} {} pin clause(s) across {} file(s)".format(verb, total, len(touched_files)))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
