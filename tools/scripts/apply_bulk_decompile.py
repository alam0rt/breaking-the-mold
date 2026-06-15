#!/usr/bin/env python3
"""Apply bulk decompilation candidates by replacing INCLUDE_ASM lines in .c files."""
import json
import os
import re
import sys

# Read candidates JSON from stdin
data = json.load(sys.stdin)

# Group by segment
by_seg = {}
for r in data:
    by_seg.setdefault(r["segment"], []).append(r)

# For each segment, find the .c file and replace
modified = []
for seg, items in by_seg.items():
    cfile = f"src/{seg}.c"
    if not os.path.exists(cfile):
        print(f"# SKIP: {cfile} not found", file=sys.stderr)
        continue
    with open(cfile) as f:
        text = f.read()
    new_text = text
    n = 0
    for r in items:
        name = r["name"]
        # Pattern: INCLUDE_ASM("asm/nonmatchings/SEG", NAME);
        pattern = f'INCLUDE_ASM("asm/nonmatchings/{seg}", {name});'
        if pattern in new_text:
            new_text = new_text.replace(pattern, r["c"])
            n += 1
        else:
            print(f"# NOT FOUND: {pattern}", file=sys.stderr)
    if new_text != text:
        with open(cfile, "w") as f:
            f.write(new_text)
        print(f"# {cfile}: {n}/{len(items)} replaced")
        modified.append(cfile)

print(f"\n# Modified {len(modified)} files")
