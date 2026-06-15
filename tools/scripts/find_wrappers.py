#!/usr/bin/env python3
"""Find simple wrapper patterns across all segments."""
import os
import re

def parse(p):
    instrs = []
    for line in open(p):
        line = line.strip()
        m = re.match(r"/\*[^*]+\*/\s+(\S+)\s*(.*)", line)
        if m:
            instrs.append((m.group(1), m.group(2).strip()))
    return instrs


wrappers = []
for root, _, names in os.walk("asm/nonmatchings"):
    if "libs" in root:
        continue
    for n in names:
        if not n.endswith(".s"):
            continue
        p = os.path.join(root, n)
        i = parse(p)
        if 7 <= len(i) <= 10:
            mnems = [m for m, _ in i]
            if "jal" in mnems and "jr" in mnems and "addiu" in mnems[0]:
                wrappers.append((len(i), p, n[:-2], i))
wrappers.sort()
print(f"Total wrappers (7-10 instr, jal+jr+stack): {len(wrappers)}")
for cnt, p, n, _ in wrappers[:50]:
    print(cnt, p)
