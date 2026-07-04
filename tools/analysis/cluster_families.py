#!/usr/bin/env python3
"""Cluster still-shelved functions into structural families for exemplar-sweep
matching.

Two passes:
  strict   -- mipsmatch exact fingerprint (byte-identical modulo masked global
              addresses). Requires build/<PROJECT>.map + .elf and a built
              tools/mipsmatch. True duplicates: matching one member's C matches
              every member verbatim (only global refs differ).
  skeleton -- opcode skeleton over asm/nonmatchings/*.s: each instruction reduced
              to mnemonic + register-operand shape, immediates/symbols/offsets
              blanked. Catches "same opcode sequence, different constants" twins
              the strict pass misses (e.g. PlayerState_Walking{Left,Right}).

Only the skeleton pass runs by default (no build inputs needed). Shelved =
appears in an INCLUDE_ASM(...) line under src/. Re-run as matching progresses;
families shrink as members get matched.

Usage:
    python3 tools/analysis/cluster_families.py [--md]      # skeleton pass
"""
import os, re, sys, hashlib
from collections import defaultdict

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
NM = os.path.join(ROOT, "asm", "nonmatchings")
SRC = os.path.join(ROOT, "src")
MD = "--md" in sys.argv

def shelved_names():
    pat = re.compile(r'INCLUDE_ASM\("[^"]+",\s*([A-Za-z0-9_]+)\)')
    names = set()
    for dp, _, fns in os.walk(SRC):
        for fn in fns:
            if fn.endswith(".c"):
                names |= set(pat.findall(open(os.path.join(dp, fn), errors="ignore").read()))
    return names

_reg = re.compile(r'\$\w+')
_num = re.compile(r'-?0x[0-9a-fA-F]+|-?\b\d+\b')
_sym = re.compile(r'\b[A-Za-z_][A-Za-z0-9_]+\b')

def skeleton(path):
    ops = []
    for line in open(path, errors="ignore"):
        m = re.search(r'\*/\s+([a-z][a-z0-9.]+)\s*(.*)$', line)
        if not m:
            continue
        mnem, operands = m.group(1), m.group(2).strip()
        operands = _reg.sub('$R', operands)
        operands = _num.sub('#', operands)
        operands = _sym.sub('S', operands)
        ops.append(mnem + ' ' + operands)
    return ops

def main():
    shelved = shelved_names()
    funcs = []
    for seg in sorted(os.listdir(NM)):
        d = os.path.join(NM, seg)
        if not os.path.isdir(d):
            continue
        for fn in os.listdir(d):
            if not fn.endswith(".s"):
                continue
            name = fn[:-2]
            ops = skeleton(os.path.join(d, fn))
            if len(ops) < 12:
                continue
            h = hashlib.md5('\n'.join(ops).encode()).hexdigest()[:10]
            funcs.append((name, seg, len(ops), h))

    groups = defaultdict(list)
    for name, seg, n, h in funcs:
        groups[(n, h)].append((name, seg))
    fams = [(n, members) for (n, h), members in groups.items() if len(members) >= 2]
    rows = sorted(((len(m) * n, n, m) for n, m in fams), reverse=True)

    out = []
    p = out.append if MD else (lambda s: print(s))
    if MD:
        p("| payoff | instrs | members | segments | functions (exemplar first) |")
        p("|-------:|-------:|--------:|----------|----------------------------|")
    else:
        print(f"{'payoff':>7} {'nops':>5} {'memb':>4}  segments")
        print("-"*100)
    for payoff, n, members in rows:
        segs = defaultdict(int)
        for _, sg in members:
            segs[sg] += 1
        segstr = ",".join(f"{s}×{c}" for s, c in sorted(segs.items(), key=lambda x: -x[1]))
        names = ", ".join(nm for nm, _ in members)
        if MD:
            p(f"| {payoff} | {n} | {len(members)} | {segstr} | {names} |")
        else:
            print(f"{payoff:>7} {n:>5} {len(members):>4}  {segstr}  | {names[:70]}")

    tail = []
    tail.append("")
    tail.append(f"skeleton families (>=2 members, >=12 instrs): {len(fams)}")
    tail.append(f"shelved funcs inside families: {sum(len(m) for _, m in fams)} / {len(shelved)}")
    for line in tail:
        p(line) if MD else print(line)

    if MD:
        sys.stdout.write("\n".join(out) + "\n")

if __name__ == "__main__":
    main()
