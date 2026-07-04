#!/usr/bin/env python3
"""Generate C .data migration definitions from the pooled .data blob.

Usage: gen_data_block.py <lo_symbol> <hi_symbol_exclusive>
       (reads asm/data/80FEC.data.s)

Emits, for each dlabel in [lo, hi), a byte-exact C definition:
  - pure numeric data  -> `u8 D_X[N] asm("D_X") = { 0x.., ... };`
  - contains relocs    -> aborts with an error listing them (handle manually;
    pointer tables need typed arrays, not u8[]).

Bytes are decoded little-endian from .word/.half/.byte and verbatim from
.asciz/.ascii, matching the on-disk layout so cc1 reproduces identical bytes.
"""
import re
import sys

BLOB = "asm/data/80FEC.data.s"


def load():
    lines = open(BLOB).read().splitlines()
    labs = []
    for i, l in enumerate(lines):
        m = re.match(r"\s*dlabel\s+(D_[0-9A-Fa-f]+)", l)
        if m:
            labs.append((int(m.group(1)[2:], 16), m.group(1), i))
    labs.sort()
    return lines, labs


def decode_bytes(lines, start, end):
    """Return list of ints (bytes) and list of reloc symbol names in [start,end)."""
    out = []
    relocs = []
    for j in range(start + 1, end):
        s = lines[j]
        if re.match(r"\s*(dlabel|enddlabel)\b", s):
            continue
        mw = re.search(r"\.word\s+(\S+)", s)
        mh = re.search(r"\.(?:half|short)\s+(\S+)", s)
        mb = re.search(r"\.byte\s+(.+)$", s)
        ma = re.search(r"\.asci(z|i)\s+\"(.*)\"\s*$", s)
        if mw:
            v = mw.group(1)
            if v.startswith("0x") or v.lstrip("-").isdigit():
                iv = int(v, 16) if v.startswith("0x") else int(v)
                out += [(iv >> (8 * k)) & 0xFF for k in range(4)]
            else:
                relocs.append(v)
                out += [None, None, None, None]
        elif mh:
            v = mh.group(1)
            iv = int(v, 16) if v.startswith("0x") else int(v)
            out += [(iv >> (8 * k)) & 0xFF for k in range(2)]
        elif mb:
            for tok in mb.group(1).split(","):
                tok = tok.strip()
                if not tok:
                    continue
                iv = int(tok, 16) if tok.startswith("0x") else int(tok)
                out.append(iv & 0xFF)
        elif ma:
            body = bytes(ma.group(2), "latin-1").decode("unicode_escape").encode("latin-1")
            out += list(body)
            if ma.group(1) == "z":
                out.append(0)
    return out, relocs


def main():
    lo_name, hi_name = sys.argv[1], sys.argv[2]
    lines, labs = load()
    names = [n for _, n, _ in labs]
    lo_i = names.index(lo_name)
    hi_i = names.index(hi_name)
    for k in range(lo_i, hi_i):
        a, n, sl = labs[k]
        end_line = labs[k + 1][2]
        bs, relocs = decode_bytes(lines, sl, end_line)
        if relocs:
            sys.exit(f"ERROR: {n} contains relocs {relocs} — needs typed array, handle manually")
        rows = []
        for x in range(0, len(bs), 8):
            rows.append("    " + ", ".join(f"0x{b:02X}" for b in bs[x:x + 8]) + ",")
        print(f'u8 {n}[{len(bs)}] asm("{n}") = {{')
        print("\n".join(rows))
        print("};")


if __name__ == "__main__":
    main()
