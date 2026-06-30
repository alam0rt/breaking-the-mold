#!/usr/bin/env python3
"""Build tools/entity_viewer/entity_remap.json: (layer, BLB_type) -> internal_type,
decoded from RemapEntityTypesForLevel @ 0x8008150C.

The function switches on the entity's layer byte (def +0x14) and routes the BLB
type (def +0x12) through a per-layer jumptable:
    layer 1 -> jtbl_80012158   layer 2 -> jtbl_80012390   layer 3 -> jtbl_80012628
Each jumptable entry points at a case that sets the internal type. We parse the
case->value labels from the .s, auto-detect each table's index offset/length, and
read the table pointers straight from the ROM. Run from repo root.
"""
import re, json, struct, pathlib

REPO = pathlib.Path(__file__).resolve().parents[2]
ROM = (REPO / "bin/SLES_010.90").read_bytes()
ASM = (REPO / "asm/nonmatchings/entinit/RemapEntityTypesForLevel.s").read_text().splitlines()

def romw(addr):
    o = addr - 0x80010000 + 0x800
    return struct.unpack_from("<I", ROM, o)[0] if 0 <= o <= len(ROM) - 4 else None

# case address -> internal type (addiu v0,zero,0xNN after each jlabel; sh zero => 0)
case_val = {}
for i, l in enumerate(ASM):
    m = re.search(r"jlabel \.L([0-9A-F]+)", l)
    if not m:
        continue
    addr = int(m.group(1), 16)
    blk = "\n".join(ASM[i + 1:i + 4])
    mv = re.search(r"addiu\s+\$v0, \$zero, (0x[0-9A-Fa-f]+)", blk)
    case_val[addr] = int(mv.group(1), 16) if mv else (0 if "sh         $zero, 0x12" in blk else None)

# per-jumptable: (symbol_addr, layer). Offset/length auto-detected from the asm.
TABLES = [(0x80012158, 1), (0x80012390, 2), (0x80012628, 3)]
text = "\n".join(ASM)
remap = {}   # "layer,blbtype" -> internal
for tbl_addr, layer in TABLES:
    sym = f"jtbl_{tbl_addr:08x}"
    j = text.find(f"%hi({sym})")
    pre = text[max(0, j - 600):j]
    off = int(re.findall(r"addiu\s+\$a0, \$\w+, -0x([0-9A-Fa-f]+)", pre)[-1], 16)
    length = int(re.findall(r"sltiu\s+\$v0, \$a0, 0x([0-9A-Fa-f]+)", pre)[-1], 16)
    for idx in range(length):
        tgt = romw(tbl_addr + idx * 4)
        if tgt in case_val and case_val[tgt] is not None:
            remap[f"{layer},{idx + off}"] = case_val[tgt]

# layer-2 special range: BLB 0xC9..0xE4 (201..228) -> internal 0x5F (95)
for bt in range(0xC9, 0xC9 + 0x1C):
    remap[f"2,{bt}"] = 0x5F

out = REPO / "tools/entity_viewer/entity_remap.json"
out.write_text(json.dumps(remap, indent=0, sort_keys=True))
print(f"decoded {len(case_val)} cases; {len(remap)} (layer,type)->internal remaps")
for k in ("1,27", "1,25", "2,25", "1,10", "1,24"):
    print(f"  {k} -> {remap.get(k, '(none/identity)')}")
print("wrote", out)
