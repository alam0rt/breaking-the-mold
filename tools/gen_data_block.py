#!/usr/bin/env python3
"""Generate C .data migration definitions from the pooled .data blob.

Usage: gen_data_block.py [--single] <lo_symbol> <hi_symbol_exclusive>
       (reads asm/data/80FEC.data.s)

Default: emits, for each dlabel in [lo, hi), a byte-exact C definition
  `u8 D_X[N] asm("D_X") = { 0x.., ... };`.

--single: emits ONE u8 array anchored at the previous dlabel's end (absorbing
any 2/4-byte alignment pad splat drops, read verbatim from bin/SLES_010.90),
plus `.set` aliases for every island symbol. Use for islands whose start is
not on the previous dlabel boundary (a "pad hole").

Aborts if any symbol contains relocs (pointer tables need typed arrays).

Bytes are decoded little-endian from .word/.half/.byte and verbatim from
.asciz/.ascii, matching the on-disk layout so cc1 reproduces identical bytes.
"""
import re
import sys
import glob

BLOB_GLOB = "asm/data/*.data.s"
GOLD = "bin/SLES_010.90"
RAM_BASE = 0x80010000
ROM_HDR = 0x800


def rom_off(addr):
    return addr - RAM_BASE + ROM_HDR


def load():
    # Concatenate all .data asm files (the pooled blob plus any carved gap
    # files) so migrated islands living in gap segments are still visible.
    lines = []
    for path in sorted(glob.glob(BLOB_GLOB),
                       key=lambda p: int(re.search(r"(\w+)\.data\.s$", p).group(1), 16)):
        lines.append(f"# FILE {path}")
        lines += open(path).read().splitlines()
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
        if re.match(r"\s*enddlabel\b", s):
            break
        if re.match(r"\s*dlabel\b", s):
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


def decode_words(lines, start, end):
    """Return list of (kind, value) for a symbol that is entirely .word.
    kind 'num' -> int literal; kind 'sym' -> symbol name. Returns None if the
    symbol contains any non-.word directive (shorts/bytes/strings)."""
    out = []
    for j in range(start + 1, end):
        s = lines[j]
        if re.match(r"\s*enddlabel\b", s):
            break
        if re.match(r"\s*dlabel\b", s):
            continue
        if not s.strip() or s.strip().startswith("#"):
            continue
        mw = re.search(r"\.word\s+(\S+)", s)
        if not mw:
            return None  # not a pure-word symbol
        v = mw.group(1)
        if v.startswith("0x") or v.lstrip("-").isdigit():
            out.append(("num", int(v, 16) if v.startswith("0x") else int(v)))
        else:
            out.append(("sym", v))
    return out


def main():
    args = sys.argv[1:]
    mode = "per"
    if args and args[0] in ("--single", "--group", "--mixed"):
        mode = args[0][2:]
        args = args[1:]
    lo_name, hi_name = args[0], args[1]
    lines, labs = load()
    names = [n for _, n, _ in labs]
    lo_i = names.index(lo_name)
    hi_i = names.index(hi_name)

    def emit_rows(bs):
        rows = []
        for x in range(0, len(bs), 8):
            rows.append("    " + ", ".join(f"0x{b:02X}" for b in bs[x:x + 8]) + ",")
        return "\n".join(rows)

    if mode == "per":
        for k in range(lo_i, hi_i):
            a, n, sl = labs[k]
            bs, relocs = decode_bytes(lines, sl, labs[k + 1][2])
            if relocs:
                sys.exit(f"ERROR: {n} contains relocs {relocs} — needs typed array, handle manually")
            print(f'u8 {n}[{len(bs)}] asm("{n}") = {{')
            print(emit_rows(bs))
            print("};")
        return

    if mode == "mixed":
        # Per-symbol, but pointer tables (all .word, containing relocs) become
        # void*[] arrays so the linker resolves the function-pointer relocs.
        # Pure-data symbols emit as u8[]. Every symbol keeps its own asm("D_X")
        # so ld lays them out contiguously in source order (all island symbols
        # must start on their natural boundary — 4-aligned for the ptr tables).
        externs = []
        blocks = []
        for k in range(lo_i, hi_i):
            a, n, sl = labs[k]
            bs, relocs = decode_bytes(lines, sl, labs[k + 1][2])
            if not relocs:
                blocks.append(f'u8 {n}[{len(bs)}] asm("{n}") = {{\n{emit_rows(bs)}\n}};')
                continue
            words = decode_words(lines, sl, labs[k + 1][2])
            if words is None:
                sys.exit(f"ERROR: {n} has relocs mixed with sub-word data — handle manually")
            ents = []
            for kind, v in words:
                if kind == "num":
                    ents.append(f"(void *)0x{v & 0xFFFFFFFF:08X}")
                else:
                    externs.append(v)
                    ents.append(f"(void *){v}")
            body = ",\n".join("    " + e for e in ents)
            blocks.append(f'void *{n}[{len(words)}] asm("{n}") = {{\n{body},\n}};')
        for e in dict.fromkeys(externs):
            print(f"extern void {e}();")
        print()
        print("\n\n".join(blocks))
        return


    # single / group: emit ONE u8 array + .set aliases for every island symbol.
    # The array body is copied verbatim from the gold ROM over the whole span,
    # so any inter-symbol or leading alignment pad is captured exactly.
    #   --single: anchor at the previous dlabel's end (absorbs the pad splat
    #             drops before a 2-mod-4 island start).
    #   --group : anchor exactly at lo (lo is already 4-aligned; groups several
    #             symbols into one contiguous array so ld never re-aligns them).
    lo_addr = labs[lo_i][0]
    if mode == "single":
        prev_addr, prev_name, prev_sl = labs[lo_i - 1]
        prev_bs, _ = decode_bytes(lines, prev_sl, labs[lo_i][2])
        anchor_addr = prev_addr + len(prev_bs)
    else:  # group
        anchor_addr = lo_addr
    pad_len = lo_addr - anchor_addr
    if pad_len < 0:
        sys.exit(f"ERROR: negative pad ({pad_len}); anchor 0x{anchor_addr:X} > lo 0x{lo_addr:X}")
    aliases = []
    end_addr = lo_addr
    for k in range(lo_i, hi_i):
        a, n, sl = labs[k]
        bs, relocs = decode_bytes(lines, sl, labs[k + 1][2])
        if relocs:
            sys.exit(f"ERROR: {n} contains relocs {relocs} — needs typed array, handle manually")
        aliases.append((n, a - anchor_addr))
        end_addr = a + len(bs)
    gold = open(GOLD, "rb").read()
    allb = list(gold[rom_off(anchor_addr):rom_off(end_addr)])
    anchor = f"D_{anchor_addr:08X}"
    print(f"/* {mode} island: {pad_len}-byte pad at 0x{anchor_addr:08X}, "
          f"{len(aliases)} aliased symbol(s); anchor {anchor} ({len(allb)}B). */")
    print(f'u8 {anchor}[{len(allb)}] asm("{anchor}") = {{')
    print(emit_rows(allb))
    print("};")
    parts = "".join(f".globl {n}\\n.set {n}, {anchor} + {off}\\n"
                    for n, off in aliases if off != 0 or n != anchor)
    if parts:
        print(f'asm("{parts}");')


if __name__ == "__main__":
    main()
