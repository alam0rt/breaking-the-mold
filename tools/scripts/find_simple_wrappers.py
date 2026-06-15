#!/usr/bin/env python3
"""Find simple wrapper functions across all segments.

Categories:
- jal_only: stack frame + 1 jal + return (passes args through)
- jal_then_jr: jal followed by jr ra (tail call pattern)
- deref_setter_imm: lw v1, X(a0); sb imm, Y(v1)
- deref_setter_arg1: lw v1, X(a0); sb a1, Y(v1)
- copy_byte: lbu v0, X(a0); sb v0, Y(arg or global)
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


def is_load_global(m, o):
    return m == "lui" and "%hi(" in o


def is_stack_alloc(m, o):
    if m != "addiu":
        return False
    m2 = re.match(r"\$sp,\s*\$sp,\s*-(0x[0-9a-fA-F]+)", o)
    return bool(m2)


def is_save_ra(m, o):
    return m == "sw" and re.match(r"\$ra,\s*0x[0-9a-fA-F]+\(\$sp\)", o)


def is_load_ra(m, o):
    return m == "lw" and re.match(r"\$ra,\s*0x[0-9a-fA-F]+\(\$sp\)", o)


def is_stack_dealloc(m, o):
    if m != "addiu":
        return False
    m2 = re.match(r"\$sp,\s*\$sp,\s*0x[0-9a-fA-F]+", o)
    return bool(m2)


def find_jal_arg_forwarder(instrs):
    """Match: addiu sp,sp,-N; sw ra,M(sp); jal X; <delay>; lw ra,M(sp); addiu sp,sp,N; jr ra; nop"""
    if len(instrs) != 8:
        return None
    if not is_stack_alloc(*instrs[0]):
        return None
    if not is_save_ra(*instrs[1]):
        return None
    if instrs[2][0] != "jal":
        return None
    # delay slot at [3]
    if not is_load_ra(*instrs[4]):
        return None
    if not is_stack_dealloc(*instrs[5]):
        return None
    if instrs[6][0] != "jr" or instrs[6][1] != "$ra":
        return None
    if instrs[7][0] != "nop":
        return None
    return ("jal_arg_forwarder", instrs[2][1].strip(), instrs[3])


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

            kind = find_jal_arg_forwarder(instrs)
            if kind:
                results.append({
                    "segment": seg,
                    "name": name,
                    "kind": kind[0],
                    "callee": kind[1],
                    "delay": list(kind[2]),
                })

    print(json.dumps(results, indent=2))
    from collections import Counter
    c = Counter(r["kind"] for r in results)
    print(f"\n# total: {len(results)}", file=sys.stderr)
    for k, v in c.most_common():
        print(f"#   {k}: {v}", file=sys.stderr)


if __name__ == "__main__":
    main()
