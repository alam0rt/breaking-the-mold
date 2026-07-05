#!/usr/bin/env python3
"""Count PC-port seed bodies still awaiting a byte-match.

A "port seed" is a function that (a) has a hand-written functional-C body under
port/decomp/ and (b) is still shelved as INCLUDE_ASM in src/. These are the
un-matched functions whose structure/intent is already worked out in the port,
so they are prime decomp targets. The count shrinks toward the number of
genuinely port-only HAL helpers as functions get folded into src/.

Prints one integer (the intersection size) to stdout.
"""
import re
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent

# Functions still shelved as INCLUDE_ASM in src/.
shelved = set()
for f in (ROOT / "src").rglob("*.c"):
    for line in f.read_text(errors="replace").splitlines():
        m = re.search(r'INCLUDE_ASM\(\s*"[^"]*"\s*,\s*([A-Za-z_]\w*)\s*\)', line)
        if m:
            shelved.add(m.group(1))

# Functions DEFINED under port/decomp/ (definition opener: no trailing ';',
# which excludes extern forward-declarations). Skip generated stub TUs.
port_defs = set()
for f in (ROOT / "port" / "decomp").rglob("*.c"):
    if f.name in ("_autostubs.c", "_autoglobals.c"):
        continue
    for line in f.read_text(errors="replace").splitlines():
        m = re.match(r'^[A-Za-z_][\w \t*]+\b([A-Za-z_]\w*)\s*\([^;]*$', line)
        if m:
            port_defs.add(m.group(1))

print(len(shelved & port_defs))
