#!/usr/bin/env python3
"""
find_missing_functions.py -- read-only detector for REAL functions that
splat / spimdisasm has not carved into their own label.

Why this exists
---------------
spimdisasm derives function starts from control-flow analysis: direct `jal`
targets plus boundary heuristics. Functions reached ONLY indirectly -- via
function-pointer / FSM-descriptor / vtable tables in .data/.rodata -- are never
seen as function starts and get glued into a neighbouring blob (or missed
entirely). This tool rebuilds a function-start ground truth FROM THE BINARY and
diffs it against what we currently know / emit.

Ground truth (binary-derived, authoritative -- does not trust Ghidra):
  Signal A : every direct `jal` target that lands in .text
             (unambiguous: a `jal X` proves X is a call target)
  Signal B : every 4-aligned word in .data/.rodata that points into .text and
             lands on a plausible function prologue
             (recovers the indirect-only funcs -- spimdisasm's blind spot)

Cross-check sets:
  SYM   : addresses known to symbol_addrs.txt (Ghidra-derived master list)
  CARVED: functions actually emitted as their own label / C definition
          (glabel in asm/nonmatchings, glabel/alabel in pure-asm, or a C func)

Report buckets:
  UNKNOWN  : A/B target NOT in SYM        -> nobody (Ghidra or splat) knows it
  UNCARVED : in SYM but not in CARVED     -> known, but glued / not split out

Read-only: prints a report and writes a machine-readable JSON. Touches nothing.
"""
from __future__ import annotations

import json
import re
import struct
import sys
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parent.parent
ROM = ROOT / "bin/SLES_010.90"
YAML = ROOT / "SLES_010.90.yaml"
SYMS = ROOT / "symbol_addrs.txt"
ASM = ROOT / "asm"
SRC = ROOT / "src"
OUT = ROOT / "tools" / "missing_functions.json"

VRAM_BASE = 0x80010000
FILE_BASE = 0x800  # main code segment begins at file offset 2048

CODE_TYPES = {"asm", "c", "hasm"}
DATA_TYPES = {"rodata", "data", ".data", "sdata", ".sdata"}


def v2f(v: int) -> int:
    return v - VRAM_BASE + FILE_BASE


def f2v(f: int) -> int:
    return f - VRAM_BASE + FILE_BASE  # symmetric offset


def f2vram(f: int) -> int:
    return f - FILE_BASE + VRAM_BASE


# ---------------------------------------------------------------------------
# 1. Parse splat yaml -> code regions + data regions (file-offset intervals)
# ---------------------------------------------------------------------------
def parse_regions():
    cfg = yaml.safe_load(YAML.read_text())
    # find the main "code" segment (the one carrying subsegments)
    main = next(s for s in cfg["segments"]
                if isinstance(s, dict) and s.get("subsegments"))
    subs = main["subsegments"]

    # Normalise each subsegment to (start_off, type)
    norm = []
    for e in subs:
        if isinstance(e, dict):
            norm.append((int(e["start"]), str(e.get("type", ""))))
        elif isinstance(e, list) and e:
            norm.append((int(e[0]), str(e[1]) if len(e) > 1 else ""))
    norm.sort()

    # segment end = the trailing "- - <offset>" marker, else file size
    seg_end = ROM.stat().st_size
    for s in cfg["segments"]:
        if isinstance(s, list) and s and isinstance(s[0], list):
            seg_end = int(s[0][0])

    code, data = [], []
    for i, (start, typ) in enumerate(norm):
        end = norm[i + 1][0] if i + 1 < len(norm) else seg_end
        if end <= start:
            continue
        if typ in CODE_TYPES:
            code.append((start, end))
        elif typ in DATA_TYPES:
            data.append((start, end))
    return code, data


def in_ranges(vram: int, ranges_vram) -> bool:
    for a, b in ranges_vram:
        if a <= vram < b:
            return True
    return False


# ---------------------------------------------------------------------------
# 2. Binary scans
# ---------------------------------------------------------------------------
def scan(rom: bytes, code, data):
    code_vram = [(f2vram(a), f2vram(b)) for a, b in code]
    text_lo = min(a for a, _ in code_vram)
    text_hi = max(b for _, b in code_vram)

    jal_targets = set()
    for a, b in code:
        for off in range(a, b - 3, 4):
            w = struct.unpack_from("<I", rom, off)[0]
            if (w >> 26) == 3:  # jal
                tgt = ((w & 0x03FFFFFF) << 2) | (f2vram(off) & 0xF0000000)
                if in_ranges(tgt, code_vram):
                    jal_targets.add(tgt)

    ptr_targets = set()
    for a, b in data:
        for off in range(a, b - 3, 4):
            v = struct.unpack_from("<I", rom, off)[0]
            if text_lo <= v < text_hi and v % 4 == 0 and in_ranges(v, code_vram):
                ptr_targets.add(v)

    return jal_targets, ptr_targets, code_vram, (text_lo, text_hi)


def word_at(rom: bytes, vram: int):
    off = v2f(vram)
    if 0 <= off <= len(rom) - 4:
        return struct.unpack_from("<I", rom, off)[0]
    return None


def has_prologue(rom: bytes, vram: int) -> bool:
    """addiu $sp,$sp,-N  (0x27BDxxxx, negative imm) -- the common frame setup."""
    w = word_at(rom, vram)
    if w is None:
        return False
    return (w & 0xFFFF0000) == 0x27BD0000 and (w & 0x8000) != 0


# ---------------------------------------------------------------------------
# 3. Known / carved sets
# ---------------------------------------------------------------------------
SYM_RE = re.compile(r"^(\w+)\s*=\s*0x([0-9A-Fa-f]+)\s*;(.*)$")


SIZE_RE = re.compile(r"size:0x([0-9A-Fa-f]+)")


def load_symbols():
    """name->addr and addr->name for game-text symbols, plus occupied byte
       intervals [addr, addr+size) so we can reject mid-function candidates."""
    by_addr = {}
    intervals = []
    for line in SYMS.read_text().splitlines():
        m = SYM_RE.match(line.strip())
        if not m:
            continue
        addr = int(m.group(2), 16)
        if not (VRAM_BASE <= addr < 0x80200000):
            continue
        by_addr.setdefault(addr, m.group(1))
        sm = SIZE_RE.search(m.group(3) or "")
        size = int(sm.group(1), 16) if sm else 0
        if size > 1:
            intervals.append((addr, addr + size))
    intervals.sort()
    return by_addr, intervals


def inside_known(vram: int, intervals) -> bool:
    """True if vram lands strictly inside a known symbol's body (mid-function)."""
    import bisect
    i = bisect.bisect_right([a for a, _ in intervals], vram) - 1
    return 0 <= i < len(intervals) and intervals[i][0] < vram < intervals[i][1]


LABEL_RE = re.compile(r"^\s*[ga]label\s+(\S+)")
ADDR_COMMENT_RE = re.compile(r"/\*\s*[0-9A-Fa-f]+\s+([0-9A-Fa-f]{8})\s")


def load_carved_asm_labels():
    """glabel/alabel addresses currently emitted in asm/ (both trees).
       Address is read from the immediately-following /* off VRAM bytes */ line.
       Only counts labels whose body has >=1 real instruction (skips data blobs
       like FONT_OBJ_* that are pure .word)."""
    carved = set()
    for s in ASM.rglob("*.s"):
        lines = s.read_text(errors="ignore").splitlines()
        for i, ln in enumerate(lines):
            if LABEL_RE.match(ln):
                # find address from next /* .. VRAM .. */ line
                for j in range(i + 1, min(i + 3, len(lines))):
                    am = ADDR_COMMENT_RE.search(lines[j])
                    if am:
                        carved.add(int(am.group(1), 16))
                        break
    return carved


INCASM_RE = re.compile(r'INCLUDE_ASM[A-Z_]*\("[^"]*",\s*(\w+)')


def load_stub_and_decompiled():
    """Return (nm_stub_names, include_asm_names).
       nm_stub_names   : functions with their OWN asm/nonmatchings/<seg>/<f>.s
       include_asm_names: names referenced via INCLUDE_ASM(...) in src
       A function is "decompiled to C" iff it has a symbol, is referenced in
       src, and is NOT in include_asm_names."""
    nm_stub_names = {p.stem for p in (ASM / "nonmatchings").rglob("*.s")}
    include_asm_names = set()
    src_referenced = set()
    for c in SRC.rglob("*.c"):
        txt = c.read_text(errors="ignore")
        include_asm_names.update(INCASM_RE.findall(txt))
        src_referenced.update(re.findall(r"\b([A-Za-z_]\w+)\b", txt))
    return nm_stub_names, include_asm_names, src_referenced


def load_pureasm_label_names():
    """glabel/alabel names living in a SHARED whole-segment .s (not nonmatchings).
       These are 'known but glued' -- emitted, but not their own carve unit."""
    names = set()
    for s in ASM.glob("*.s"):
        for ln in s.read_text(errors="ignore").splitlines():
            m = LABEL_RE.match(ln)
            if m:
                names.add(m.group(1))
    for s in (ASM / "libs").glob("*.s") if (ASM / "libs").exists() else []:
        for ln in s.read_text(errors="ignore").splitlines():
            m = LABEL_RE.match(ln)
            if m:
                names.add(m.group(1))
    return names


# ---------------------------------------------------------------------------
def main():
    rom = ROM.read_bytes()
    code, data = parse_regions()
    jal, ptr, code_vram, (tlo, thi) = scan(rom, code, data)

    by_addr, intervals = load_symbols()
    sym_addrs = set(by_addr)
    nm_stub_names, include_asm_names, src_referenced = load_stub_and_decompiled()
    pureasm_names = load_pureasm_label_names()

    all_targets = jal | ptr

    def is_real_func(vram):
        """binary-level evidence that vram is a function START (not data / mid)."""
        return vram in jal or has_prologue(rom, vram)

    # A symbol is CARVED (its own linkable unit) iff it has its own nonmatchings
    # stub, or it is decompiled to C (referenced in src, not via INCLUDE_ASM).
    def is_carved(name):
        if name in nm_stub_names:
            return True
        if name in src_referenced and name not in include_asm_names:
            return True
        return False

    # UNKNOWN: binary says function start, symbol_addrs has never heard of it,
    # AND it does not land mid-body of a known function (rejects jump-table
    # entries and +N interior pointers).
    unknown = sorted(t for t in all_targets
                     if t not in sym_addrs and is_real_func(t)
                     and not inside_known(t, intervals))
    # UNCARVED: known real function that is not its own linkable unit
    # (glued inside a shared pure-asm .s, or otherwise unsplit).
    uncarved = sorted(
        a for a, n in by_addr.items()
        if in_ranges(a, code_vram) and is_real_func(a) and not is_carved(n)
    )

    def classify(t):
        tags = []
        if t in jal:
            tags.append("jal")
        if t in ptr:
            tags.append("ptr")
        if has_prologue(rom, t):
            tags.append("prologue")
        return tags

    # split UNCARVED by whether it lives in a shared pure-asm .s (PSY-Q libs etc.)
    unc_pureasm = [a for a in uncarved if by_addr[a] in pureasm_names]
    unc_other = [a for a in uncarved if by_addr[a] not in pureasm_names]

    print("=" * 68)
    print("MISSING-FUNCTION REPORT (binary-derived, read-only)")
    print("=" * 68)
    print(f"text range          : 0x{tlo:08X} .. 0x{thi:08X}")
    print(f"jal targets         : {len(jal)}  (unknown: "
          f"{sum(1 for t in unknown if 'jal' in classify(t))})")
    print(f"data->text pointers : {len(ptr)}")
    print(f"symbol_addrs (text) : {len(sym_addrs)}")
    print()
    print(f"UNKNOWN  (real func, not in symbol_addrs) : {len(unknown)}")
    print(f"UNCARVED (known real func, not own unit)  : {len(uncarved)}")
    print(f"    - in shared pure-asm .s (libs etc.)   : {len(unc_pureasm)}")
    print(f"    - other (game code)                   : {len(unc_other)}")
    print()
    print("--- UNKNOWN (real function starts nobody knows) ---")
    for t in unknown:
        w = word_at(rom, t)
        print(f"  0x{t:08X}  [{','.join(classify(t)) or '-':<16}]  first=0x{w:08X}")

    print()
    print("--- UNCARVED game code (not pure-asm libs), first 40 ---")
    for a in unc_other[:40]:
        print(f"  0x{a:08X}  {by_addr[a]:<40} [{','.join(classify(a)) or '-'}]")

    OUT.write_text(json.dumps({
        "text_range": [tlo, thi],
        "counts": {
            "jal": len(jal), "ptr": len(ptr), "sym": len(sym_addrs),
            "unknown": len(unknown), "uncarved": len(uncarved),
            "uncarved_pureasm": len(unc_pureasm),
            "uncarved_other": len(unc_other),
        },
        "unknown": [{"addr": t, "tags": classify(t), "first_word": word_at(rom, t)}
                    for t in unknown],
        "uncarved_other": [{"addr": a, "name": by_addr[a], "tags": classify(a)}
                           for a in unc_other],
        "uncarved_pureasm": [{"addr": a, "name": by_addr[a], "tags": classify(a)}
                             for a in unc_pureasm],
    }, indent=2))
    print(f"\nwrote {OUT.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
