#!/usr/bin/env python3
"""Analyze the pooled .data blob (asm/data/80FEC.data.s) for migration.

For each dlabel symbol: address, size, all-zero?, and owning TU set (grep of
the symbol name across asm/nonmatchings/**/*.s and src/*.c).

The dotdata() link mechanism pulls a TU's ENTIRE .data section to one
position, so only TUs whose full .data footprint is a SINGLE CONTIGUOUS run
(no foreign/multi-owner/unref/zero symbol interspersed) can be migrated.

Usage: python3 tools/analyze_data_blob.py [--all]
"""
import re
import sys
import glob
import os

BLOB = "asm/data/80FEC.data.s"


def load():
    lines = open(BLOB).read().splitlines()
    labs = []  # (addr, name, start_line)
    for i, l in enumerate(lines):
        m = re.match(r"\s*dlabel\s+(D_[0-9A-Fa-f]+)", l)
        if m:
            labs.append((int(m.group(1)[2:], 16), m.group(1), i))
    labs.sort()
    return lines, labs


def is_zero(lines, start, end):
    for j in range(start + 1, end):
        s = lines[j]
        if re.match(r"\s*(dlabel|enddlabel)\b", s):
            continue
        for mm in re.finditer(r"\.(?:word|half|short)\s+(\S+)", s):
            if mm.group(1) not in ("0x0", "0"):
                return False
        mb = re.search(r"\.byte\s+(.+)$", s)
        if mb and any(x.strip() not in ("0x0", "0") for x in mb.group(1).split(",")):
            return False
        if re.search(r"\.asci[iz]", s):
            return False
    return True


def owners():
    out = {}
    for f in glob.glob("asm/nonmatchings/**/*.s", recursive=True) + glob.glob("src/*.c"):
        try:
            txt = open(f, errors="ignore").read()
        except Exception:
            continue
        tu = f.split("nonmatchings/")[1].split("/")[0] if "nonmatchings" in f else os.path.basename(f)[:-2]
        out[f] = (tu, txt)
    return out


def main():
    show_all = "--all" in sys.argv
    lines, labs = load()
    src = owners()

    recs = []
    for k, (a, n, sl) in enumerate(labs):
        nxt_addr = labs[k + 1][0] if k + 1 < len(labs) else a
        nxt_line = labs[k + 1][2] if k + 1 < len(labs) else len(lines)
        sz = nxt_addr - a
        z = is_zero(lines, sl, nxt_line)
        ow = sorted({tu for tu, txt in src.values() if n in txt})
        recs.append((a, n, sz, z, ow))

    by_tu = {}
    for a, n, sz, z, ow in recs:
        if len(ow) == 1 and not z:
            by_tu.setdefault(ow[0], []).append(a)

    print("== Per-TU single-contiguous-run migratability ==")
    migratable = []
    for tu in sorted(by_tu):
        addrs = sorted(by_tu[tu])
        lo, hi = addrs[0], addrs[-1]
        span = [r for r in recs if lo <= r[0] <= hi]
        foreign = [r for r in span if r[4] != [tu]]
        anyzero = [r for r in span if r[3]]
        blocked = foreign or anyzero
        tag = "OK" if not blocked else f"BLOCKED(foreign={len(foreign)} zero={len(anyzero)})"
        total = sum(r[2] for r in span)
        print(f"  {tu:12} 0x{lo:X}..0x{hi:X} syms={len(span)} bytes={total} {tag}")
        if not blocked:
            migratable.append((tu, lo, hi, span))
        if show_all and foreign:
            for r in foreign[:8]:
                print(f"       foreign 0x{r[0]:X} {r[1]} owners={r[4] or 'UNREF'}")

    print(f"\n== {len(migratable)} whole-footprint runs ==")
    for tu, lo, hi, span in migratable:
        print(f"  {tu}: 0x{lo:X}..0x{hi:X} ({len(span)} syms)")

    # Maximal contiguous runs of single-owner, non-zero, same-TU symbols.
    # Each such run is independently migratable as build/src/<tu>.o(.data).
    runs = []
    cur = []

    def flush():
        if cur:
            tu = cur[0][4][0]
            lo = cur[0][0]
            end = cur[-1][0] + cur[-1][2]
            nb = sum(r[2] for r in cur)
            runs.append((tu, lo, end, len(cur), nb, list(cur)))
        cur.clear()

    for r in recs:
        a, n, sz, z, ow = r
        ok = (len(ow) == 1) and (not z) and (sz > 8)
        if ok and cur and cur[-1][4] == ow and cur[-1][0] + cur[-1][2] == a:
            cur.append(r)
        else:
            flush()
            if ok:
                cur.append(r)
    flush()

    runs.sort(key=lambda x: (-x[4], x[1]))
    print("\n== Maximal contiguous single-owner runs (all members >8B, non-zero) ==")
    # Largest island per TU (one .o(.data) block per TU).
    best = {}
    for tu, lo, end, ns, nb, span in runs:
        if tu not in best or nb > best[tu][4]:
            best[tu] = (tu, lo, end, ns, nb, span)
    for tu, lo, end, ns, nb, span in runs:
        star = " *BEST" if best[tu][1] == lo else ""
        print(f"  {tu:12} 0x{lo:X}..0x{end:X} syms={ns} bytes={nb}{star}")

    if show_all:
        print("\n== ALL symbols ==")
        for a, n, sz, z, ow in recs:
            print(f"  0x{a:X} {n} sz={sz} {'ZERO' if z else ''} {ow or 'UNREF'}")


if __name__ == "__main__":
    main()
