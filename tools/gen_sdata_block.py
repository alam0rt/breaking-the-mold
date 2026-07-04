#!/usr/bin/env python3
"""Generate a C .sdata migration block from a splat sdata .s file.

Usage: gen_sdata_block.py <asm/data/<tu>.sdata.s> <src/<tu>.c>

Emits, to stdout:
  1) `EXTERN:` lines listing callback symbols referenced but NOT defined in the
     given .c (declare these as `extern void Fn();`).
  2) The address-ordered C block (markers as u32, callbacks cast to
     EntityCallback, END2+pad sentinels as u32 arrays). Friendly names are used
     for any address already carrying an `asm("D_...")` decl in the .c.
"""
import re
import sys

MARKER = 0xFFFF0000
END2 = 0x32444E45


def parse_words(path):
    # Each entry: (addr:int, kind:str, value)
    #   kind 'word'  -> value is symbol, '0xFFFF0000', or raw hex string
    #   kind 'ascii' -> value is the decoded C-string literal (e.g. "%s")
    words = []
    with open(path) as f:
        for line in f:
            m = re.search(r"/\*\s*[0-9A-Fa-f]+\s+([0-9A-Fa-f]{8})\s+[0-9A-Fa-f]{8}\s*\*/\s*\.word\s+(\S+)", line)
            if m:
                words.append((int(m.group(1), 16), "word", m.group(2)))
                continue
            # splat emits string data as `.asciz "..."` / `.ascii "..."` with the
            # same `/* off addr .. */` comment prefix. Capture so gaps are filled.
            ms = re.search(r"/\*\s*([0-9A-Fa-f]+)\s+([0-9A-Fa-f]{8})\s.*\*/\s*\.(?:asciz|ascii)\s+(\".*\")", line)
            if ms:
                words.append((int(ms.group(2), 16), "ascii", ms.group(3)))
    return words


def friendly_map(cpath):
    fm = {}
    with open(cpath) as f:
        for line in f:
            stripped = line.lstrip()
            # skip lines inside block comments or line comments (commented-out
            # example decls) so we only bind friendly names from real decls.
            if stripped.startswith("*") or stripped.startswith("/*") or stripped.startswith("//"):
                continue
            m = re.search(r'(\w+)\s+asm\("(D_800A[0-9A-Fa-f]+)"\)', line)
            if m:
                fm.setdefault(m.group(2), m.group(1))
    return fm


def main():
    spath, cpath = sys.argv[1], sys.argv[2]
    words = parse_words(spath)
    fm = friendly_map(cpath)
    ctext = open(cpath).read()

    # collect callback symbols (non-hex, non-marker)
    callbacks = []
    for _, kind, v in words:
        if kind != "word" or v.startswith("0x"):
            continue
        callbacks.append(v)
    externs = []
    for cb in dict.fromkeys(callbacks):
        if not re.search(r"^\s*(void|s32|u32|int)\s+" + re.escape(cb) + r"\s*\(", ctext, re.M):
            externs.append(cb)

    for e in externs:
        print(f"EXTERN: {e}")
    print("// ---- block ----")

    i = 0
    n = len(words)
    prev_end = None
    while i < n:
        addr, kind, val = words[i]
        if prev_end is not None and addr != prev_end:
            print(f'// WARNING: gap {prev_end:#010x}..{addr:#010x} '
                  f'({addr - prev_end} bytes) — add missing entries manually')
        name = fm.get(f"D_{addr:08X}", f"D_{addr:08X}")
        if kind == "ascii":
            # string data -> char[] sized to the on-disk length (incl. NUL)
            print(f'char {name}[] asm("D_{addr:08X}") = {val};')
            prev_end = None  # length unknown here; disable gap check across strings
            i += 1
            continue
        # END2 sentinel + pad pair
        if val.startswith("0x") and int(val, 16) == END2:
            # next should be 0x00000000
            print(f'u32 {name}[2] asm("D_{addr:08X}") = {{0x32444E45, 0x00000000}};')
            prev_end = addr + 8
            i += 2  # skip the pad word
            continue
        if val.startswith("0x"):
            iv = int(val, 16)
            if iv == MARKER:
                print(f'u32 {name} asm("D_{addr:08X}") = 0xFFFF0000;')
            else:
                # raw code address -> callback literal
                print(f'EntityCallback {name} asm("D_{addr:08X}") = (EntityCallback){val};')
        else:
            print(f'EntityCallback {name} asm("D_{addr:08X}") = (EntityCallback){val};')
        prev_end = addr + 4
        i += 1


if __name__ == "__main__":
    main()
