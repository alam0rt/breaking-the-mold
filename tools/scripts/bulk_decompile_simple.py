#!/usr/bin/env python3
"""Bulk decompile simple stub functions across all segments.

Outputs JSON list of {segment, name, kind, c}.
"""
import os
import re
import sys
import json

ASM_ROOT = "asm/nonmatchings"


def parse_asm(path):
    instrs = []
    with open(path) as f:
        for line in f:
            line = line.strip()
            m = re.match(r"/\*[^*]+\*/\s+(\S+)\s*(.*)", line)
            if not m:
                continue
            mnem, ops = m.group(1), m.group(2).strip()
            instrs.append((mnem, ops))
    return instrs


def parse_imm(s):
    s = s.strip()
    sign = 1
    if s.startswith("-"):
        sign = -1
        s = s[1:]
    return sign * int(s, 0)


TYPE_LOAD = {"lb": "s8", "lbu": "u8", "lh": "s16", "lhu": "u16", "lw": "u32"}
TYPE_STORE = {"sb": "u8", "sh": "u16", "sw": "u32"}


def classify(instrs):
    # 2-instruction patterns: jr ra + delay slot
    if len(instrs) == 2 and instrs[0][0] == "jr" and instrs[0][1] == "$ra":
        mnem, ops = instrs[1]
        if mnem == "nop":
            return ("void_stub",)
        if mnem == "addu" and re.match(r"\$v0,\s*\$zero,\s*\$zero", ops):
            return ("return_zero",)
        if mnem == "addu" and re.match(r"\$v0,\s*\$a0,\s*\$zero", ops):
            return ("return_arg0",)
        if mnem in TYPE_STORE:
            m = re.match(r"\$(\w+),\s*(-?(?:0x)?[\dA-Fa-f]+)\(\$a0\)", ops)
            if m:
                src = m.group(1)
                off = parse_imm(m.group(2))
                if src == "zero":
                    return ("clear_field", mnem, off)
                if src == "a1":
                    return ("setter_arg1", mnem, off)
        return None

    if len(instrs) == 3:
        m0, o0 = instrs[0]
        m1, o1 = instrs[1]
        m2, o2 = instrs[2]
        if m1 == "jr" and o1 == "$ra":
            if m0 == "addiu" and m2 in TYPE_STORE:
                mm = re.match(r"\$v0,\s*\$zero,\s*(\S+)", o0)
                if mm:
                    val = parse_imm(mm.group(1))
                    mm2 = re.match(r"\$v0,\s*(-?(?:0x)?[\dA-Fa-f]+)\(\$a0\)", o2)
                    if mm2:
                        off = parse_imm(mm2.group(1))
                        return ("setter_imm", m2, off, val)
            if m0 in TYPE_LOAD and m2 == "nop":
                mm = re.match(r"\$v0,\s*(-?(?:0x)?[\dA-Fa-f]+)\(\$a0\)", o0)
                if mm:
                    off = parse_imm(mm.group(1))
                    return ("getter", m0, off)
    return None


def emit_c(name, kind):
    if kind[0] == "void_stub":
        return f"void {name}(void) {{\n}}"
    if kind[0] == "return_zero":
        return f"s32 {name}(void) {{\n    return 0;\n}}"
    if kind[0] == "return_arg0":
        return f"void *{name}(void *x) {{\n    return x;\n}}"
    if kind[0] == "clear_field":
        op, off = kind[1], kind[2]
        t = TYPE_STORE[op]
        return f"void {name}(void *e) {{\n    *({t} *)((u8 *)e + 0x{off:X}) = 0;\n}}"
    if kind[0] == "setter_arg1":
        op, off = kind[1], kind[2]
        t = TYPE_STORE[op]
        return f"void {name}(void *e, {t} val) {{\n    *({t} *)((u8 *)e + 0x{off:X}) = val;\n}}"
    if kind[0] == "setter_imm":
        op, off, val = kind[1], kind[2], kind[3]
        t = TYPE_STORE[op]
        if op == "sb":
            val_disp = f"0x{val & 0xFF:X}" if val >= 0 else str(val)
        elif op == "sh":
            val_disp = f"0x{val & 0xFFFF:X}" if val >= 0 else str(val)
        else:
            val_disp = f"0x{val:X}" if val >= 0 else str(val)
        return f"void {name}(void *e) {{\n    *({t} *)((u8 *)e + 0x{off:X}) = {val_disp};\n}}"
    if kind[0] == "getter":
        op, off = kind[1], kind[2]
        t = TYPE_LOAD[op]
        return f"{t} {name}(void *e) {{\n    return *({t} *)((u8 *)e + 0x{off:X});\n}}"
    return None


def main():
    results = []
    for seg in sorted(os.listdir(ASM_ROOT)):
        seg_dir = os.path.join(ASM_ROOT, seg)
        if not os.path.isdir(seg_dir):
            continue
        for fn in sorted(os.listdir(seg_dir)):
            if not fn.endswith(".s"):
                continue
            name = fn[:-2]
            instrs = parse_asm(os.path.join(seg_dir, fn))
            kind = classify(instrs)
            if kind is None:
                continue
            c = emit_c(name, kind)
            if c is None:
                continue
            results.append({
                "segment": seg,
                "name": name,
                "kind": kind[0],
                "c": c,
            })

    print(json.dumps(results, indent=2))
    from collections import Counter
    c = Counter(r["kind"] for r in results)
    print(f"\n# total: {len(results)}", file=sys.stderr)
    for k, v in c.most_common():
        print(f"#   {k}: {v}", file=sys.stderr)


if __name__ == "__main__":
    main()
