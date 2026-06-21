#!/usr/bin/env python3
"""
asm_annotate.py - Decorate Skullmonkeys MIPS asm with human-readable context.

Walks splat-generated .s files in asm/nonmatchings/ and appends inline
annotations to each instruction it can decode:

  - `%hi(NAME)`, `%lo(NAME)`, `%gp_rel(NAME)`  -> symbol info from
    symbol_addrs.txt (type, size, comment).
  - `jal NAME`, `j NAME`                      -> callee details.
  - `lui $r, (0xVALUE >> 16)` paired with the next instruction's low half
                                              -> reconstructed 32-bit
                                                 immediate; looked up as a
                                                 RAM symbol AND as an asset
                                                 hash from include/Game/asset_ids.h.
  - `lw/sw/lh/lhu/sh/lb/lbu/sb OFF($reg)` when `$reg` is tracked as the
    address of a typed global (via a preceding `addiu $reg,$reg,%lo(NAME)`
    or `lui $reg,%hi(NAME); ... %lo(NAME)($reg)` pair) -> the named struct
    field via tools/hdr2schema.
  - `lw $rt, %lo(g_pStructName)($rs)` -> registers `$rt` as a typed pointer
    so subsequent `OFF($rt)` loads decode to `StructName.field`. Works for
    any global whose name matches `g_p<StructName>` where `<StructName>`
    is a struct defined in include/Game/.

Usage:
    tools/asm_annotate.py asm/nonmatchings/main/CheckCheatCodeInput.s
    tools/asm_annotate.py asm/nonmatchings/main/main.s --no-bytes
    tools/asm_annotate.py asm/nonmatchings/main/main.s -o /tmp/main.annot.s
    tools/asm_annotate.py asm/nonmatchings/player/*.s --inplace=.annot

Flags:
    -o, --out PATH      write to file (default: stdout)
    --inplace=SUFFIX    write each input to <input><SUFFIX> (e.g. .annot)
    --no-bytes          drop the `/* ROM VRAM HEX */` prefix to tighten output
    --color             enable ANSI color in annotations (default: auto)
    --no-color          force-disable color
    --json              emit JSON records instead of text (one per instruction)
    --stats             print resolution stats to stderr
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

# Reuse the existing resolver / schema parser.
import resolve as resolve_mod  # type: ignore


# -----------------------------------------------------------------------------
# Line model
# -----------------------------------------------------------------------------


# /* ROM_OFF VRAM HEX_BYTES */  mnemonic   operands
LINE_RE = re.compile(
    r"""^
    (?P<lead>\s*)
    (?:/\*\s*(?P<rom>[0-9A-Fa-f]+)\s+(?P<vram>[0-9A-Fa-f]+)\s+(?P<hex>[0-9A-Fa-f]+)\s*\*/\s*)?
    (?P<mnemonic>[A-Za-z][\w.]*)
    (?:\s+(?P<operands>.*?))?
    \s*$
    """,
    re.VERBOSE,
)

HI_RE      = re.compile(r"%hi\(([A-Za-z_]\w*)\)")
LO_RE      = re.compile(r"%lo\(([A-Za-z_]\w*)\)")
GP_REL_RE  = re.compile(r"%gp_rel\(([A-Za-z_]\w*)\)")
JUMP_RE    = re.compile(r"^\s*(?:jal|j|jalr)\s+([A-Za-z_]\w*)\b")
SHIFT_HI_RE = re.compile(
    r"\(\s*0x([0-9A-Fa-f]+)\s*>>\s*16\s*\)"
)
# Load/store memory accessor: OP $rt, OFF($base)
MEM_RE = re.compile(
    r"""^\s*
    (?P<op>lw|sw|lh|lhu|sh|lb|lbu|sb|swl|swr|lwl|lwr|ld|sd)\s+
    \$(?P<rt>\w+)\s*,\s*
    (?P<off>-?(?:0x[0-9A-Fa-f]+|\d+))\s*
    \(\s*\$(?P<base>\w+)\s*\)
    """,
    re.VERBOSE,
)
# `lw $rt, %lo(NAME)($rs)` -- load from a named global (low half).
MEM_LO_RE = re.compile(
    r"""^\s*
    (?P<op>lw|sw|lh|lhu|sh|lb|lbu|sb)\s+
    \$(?P<rt>\w+)\s*,\s*
    %lo\((?P<name>[A-Za-z_]\w*)\)\s*
    \(\s*\$(?P<base>\w+)\s*\)
    """,
    re.VERBOSE,
)
# `addiu $reg, $reg, %lo(NAME)` -> $reg holds &NAME afterward
ADDIU_LO_RE = re.compile(
    r"^\s*addiu\s+\$(\w+)\s*,\s*\$(\w+)\s*,\s*%lo\(([A-Za-z_]\w*)\)"
)
# `lui $reg, %hi(NAME)` -> $reg holds HI part of &NAME (tentative)
LUI_HI_RE = re.compile(
    r"^\s*lui\s+\$(\w+)\s*,\s*%hi\(([A-Za-z_]\w*)\)"
)
# `lui $reg, (0xVALUE >> 16)` -> $reg holds upper half of bare immediate
LUI_SHIFT_RE = re.compile(
    r"^\s*lui\s+\$(\w+)\s*,\s*\(\s*0x([0-9A-Fa-f]+)\s*>>\s*16\s*\)"
)
# `ori/addiu $rt, $rs, 0xLOW` (low half candidate)
LOW_HALF_RE = re.compile(
    r"^\s*(?:ori|addiu)\s+\$(\w+)\s*,\s*\$(\w+)\s*,\s*(-?0x[0-9A-Fa-f]+|-?\d+)"
)
# Any instruction whose first operand is `$rd,` and that writes it.
WRITES_REG_RE = re.compile(r"^\s*[A-Za-z][\w.]*\s+\$(\w+)\s*,")


@dataclass
class Inst:
    raw: str                # the original line, unchanged
    lead: str               # leading whitespace
    rom: str | None         # rom offset hex (no 0x)
    vram: str | None        # virtual address hex (no 0x)
    bytes_hex: str | None   # 8 hex chars of the instruction word
    mnemonic: str | None    # opcode mnemonic, lowercase
    operands: str           # everything after the mnemonic (rstripped)
    is_label: bool = False  # `glabel X` or `.Lxxxxxxxx:`
    annotation: str = ""    # populated by annotator


def parse_line(line: str) -> Inst:
    # Label lines: `glabel NAME` or `.Lxxxxxxxx:`
    stripped = line.rstrip()
    if not stripped.strip():
        return Inst(raw=line, lead="", rom=None, vram=None, bytes_hex=None,
                    mnemonic=None, operands="", is_label=False)
    bare = stripped.strip()
    if bare.startswith("glabel") or bare.endswith(":") or bare.startswith(".L"):
        return Inst(raw=line, lead="", rom=None, vram=None, bytes_hex=None,
                    mnemonic=None, operands="", is_label=True)
    m = LINE_RE.match(stripped)
    if not m:
        return Inst(raw=line, lead="", rom=None, vram=None, bytes_hex=None,
                    mnemonic=None, operands="", is_label=False)
    return Inst(
        raw=line,
        lead=m.group("lead") or "",
        rom=m.group("rom"),
        vram=m.group("vram"),
        bytes_hex=m.group("hex"),
        mnemonic=(m.group("mnemonic") or "").lower(),
        operands=(m.group("operands") or "").rstrip(),
        is_label=False,
    )


# -----------------------------------------------------------------------------
# Annotator
# -----------------------------------------------------------------------------


def _fmt_symbol_short(s: resolve_mod.Symbol) -> str:
    bits = [f"{s.name} @ 0x{s.address:08X}"]
    if s.size is not None:
        bits.append(f"size=0x{s.size:X}")
    if s.type:
        bits.append(f"type={s.type}")
    if s.comment:
        bits.append(s.comment)
    return ", ".join(bits)


def _fmt_field_short(struct: str, fname: str, foff: int, ftype: str, intra: int) -> str:
    suffix = f" + 0x{intra:X}" if intra else ""
    return f"{struct}.{fname}{suffix} (0x{foff:X}, {ftype})"


class Annotator:
    def __init__(self, r: resolve_mod.Resolver):
        self.r = r
        # Register state: regname -> ("addr", symbol_name) once we know it
        # points at &SYMBOL. Cleared when register is overwritten.
        self.regs: dict[str, tuple[str, str]] = {}
        # Pending lui state for two-instruction patterns:
        #   regname -> ("hi_named", symbol_name) | ("hi_shift", hi_value_int)
        self.pending_lui: dict[str, tuple[str, object]] = {}
        # Resolution counters
        self.stats = {
            "lines": 0,
            "instructions": 0,
            "symbol_refs": 0,
            "asset_hits": 0,
            "field_refs": 0,
            "jumps": 0,
            "unresolved": 0,
        }

    # ---- helpers ------------------------------------------------------------

    def _invalidate_reg(self, reg: str) -> None:
        self.regs.pop(reg, None)
        self.pending_lui.pop(reg, None)

    def _record_addr(self, reg: str, name: str) -> None:
        self.regs[reg] = ("addr", name)

    def _struct_field_for_load(self, base_reg: str, offset: int) -> str | None:
        info = self.regs.get(base_reg)
        if not info:
            return None
        kind = info[0]
        if kind == "addr":
            sym_name = info[1]
            sym = self.r.symbols_by_name.get(sym_name)
            if sym is None or not sym.type:
                return None
            type_name = sym.type.rstrip("*").strip()
            display_type = sym.type
        elif kind == "typed_ptr":
            type_name = info[1]
            display_type = f"{type_name} *"
        else:
            return None
        f = self.r.field_at(type_name, offset)
        if not f:
            return None
        fname, foff, ftype = f
        intra = offset - foff
        return _fmt_field_short(display_type, fname, foff, ftype, intra)

    def _maybe_typed_ptr_global(self, name: str) -> str | None:
        """If `name` is `g_p<StructName>` and StructName is a known struct,
        return the struct name. Otherwise None."""
        if not name.startswith("g_p"):
            return None
        candidate = name[3:]
        if not candidate or not candidate[0].isupper():
            return None
        structs = self.r.structs()
        if candidate in structs:
            return candidate
        return None

    # ---- main per-instruction step -----------------------------------------

    def annotate(self, inst: Inst) -> None:
        self.stats["lines"] += 1
        if not inst.mnemonic or inst.is_label:
            return
        self.stats["instructions"] += 1

        parts: list[str] = []
        ops = inst.operands

        # --- 1. Symbol references via %hi/%lo/%gp_rel ------------------------
        named_refs: list[str] = []
        for rx in (HI_RE, LO_RE, GP_REL_RE):
            for m in rx.finditer(ops):
                name = m.group(1)
                if name in named_refs:
                    continue
                named_refs.append(name)
                sym = self.r.symbols_by_name.get(name)
                if sym is None:
                    # `D_80xxxxxx` (or `jtbl_80xxxxxx`) self-encodes the
                    # address; emitting `= 0x80xxxxxx` is just noise. Mark
                    # it as unresolved without annotating.
                    if re.fullmatch(r"(?:D|jtbl|jpt|func)_[0-9A-Fa-f]{8}", name):
                        self.stats["unresolved"] += 1
                        continue
                    parts.append(f"{name} (unknown)")
                    self.stats["unresolved"] += 1
                    continue
                parts.append(_fmt_symbol_short(sym))
                self.stats["symbol_refs"] += 1

        # --- 2. Jump targets -------------------------------------------------
        jm = JUMP_RE.match(ops if not ops.startswith("$") else "")
        # Re-match using the full line text so we cover `jal NAME` exactly.
        line_text = f"{inst.mnemonic} {ops}"
        jm = JUMP_RE.match(line_text)
        if jm:
            target = jm.group(1)
            if not target.startswith(".L"):
                sym = self.r.symbols_by_name.get(target)
                if sym is not None:
                    # Avoid duplicating an annotation if the same name already
                    # appeared via %hi/%lo above.
                    if target not in named_refs:
                        parts.append(f"-> {_fmt_symbol_short(sym)}")
                        self.stats["jumps"] += 1
                else:
                    parts.append(f"-> {target} (unknown)")
                    self.stats["unresolved"] += 1

        # --- 3. Two-instruction lui+lo immediate reconstruction --------------
        # First handle lui that establishes pending state.
        lui_named = LUI_HI_RE.match(line_text)
        lui_shift = LUI_SHIFT_RE.match(line_text)
        lo_half_m  = LOW_HALF_RE.match(line_text)
        mem_m      = MEM_RE.match(line_text)
        addiu_lo_m = ADDIU_LO_RE.match(line_text)

        if lui_named:
            reg, name = lui_named.group(1), lui_named.group(2)
            # If the same instruction *also* contains %lo (it shouldn't, but
            # be safe), don't shadow. Otherwise record the pending hi.
            self.pending_lui[reg] = ("hi_named", name)
        elif lui_shift:
            reg, hi_hex = lui_shift.group(1), int(lui_shift.group(2), 16)
            full_hi = hi_hex & 0xFFFF0000
            self.pending_lui[reg] = ("hi_shift", full_hi)
            # `(0xVALUE >> 16)` literally encodes the full 32-bit constant.
            # If a matching low half never lands in this register, the upper
            # alone is the entire constant — try to annotate it now so asset
            # hashes whose low half is 0x0000 (or whose low half loads into
            # a different register) still get resolved.
            assets = self.r.asset_with_hash(full_hi)
            if assets:
                for a in assets:
                    parts.append(f"~ 0x{full_hi:08X} {a.name} [{a.namespace}]")
                self.stats["asset_hits"] += 1
            else:
                sym = self.r.symbol_at(full_hi)
                if sym is not None:
                    parts.append(f"~ 0x{full_hi:08X} {_fmt_symbol_short(sym)}")
                    self.stats["symbol_refs"] += 1

        # Low-half completion patterns -- examined here so that a single
        # instruction can both consume a pending lui AND produce annotations.
        consumed_pending: str | None = None

        # 3a. `addiu $rs, $rs, %lo(NAME)` => regs[rs] = addr_of(NAME)
        if addiu_lo_m:
            rd, rs, name = addiu_lo_m.group(1), addiu_lo_m.group(2), addiu_lo_m.group(3)
            if rd == rs and self.pending_lui.get(rs, (None,))[0] == "hi_named" \
                    and self.pending_lui[rs][1] == name:
                consumed_pending = rs
                self._record_addr(rd, name)

        # 3b. `lw $rt, %lo(NAME)($rs)` after `lui $rs,%hi(NAME)`: read the
        # global itself. resolve.py already annotated it above. If the global
        # is a typed pointer (by convention `g_pStructName`), mark $rt as
        # a `StructName *` so subsequent `OFF($rt)` loads decode via the
        # struct schema.
        ptr_consumed_target: str | None = None
        ptr_consumed_rt: str | None = None
        mem_lo_m = MEM_LO_RE.match(line_text)
        if mem_lo_m:
            base = mem_lo_m.group("base")
            rt = mem_lo_m.group("rt")
            deref_name = mem_lo_m.group("name")
            op = mem_lo_m.group("op")
            if self.pending_lui.get(base, (None,))[0] == "hi_named":
                consumed_pending = base
            if op == "lw":
                target_type = self._maybe_typed_ptr_global(deref_name)
                if target_type:
                    ptr_consumed_target = target_type
                    ptr_consumed_rt = rt
                    parts.append(f"-> ${rt} = ({target_type} *){deref_name}")

        # Also consume pending lui for the numeric-offset memory form that
        # uses %lo() in operands (rare but possible after migration).
        if mem_m:
            base = mem_m.group("base")
            if self.pending_lui.get(base, (None,))[0] == "hi_named":
                if "%lo(" in ops:
                    consumed_pending = base

        # 3c. `ori/addiu $rt, $rs, 0xLOW` after `lui $rs, (0xHI >> 16)`:
        # reconstruct full immediate and look it up.
        if lo_half_m:
            rt, rs, lo_text = lo_half_m.group(1), lo_half_m.group(2), lo_half_m.group(3)
            try:
                lo_val = int(lo_text, 16) if lo_text.lower().startswith("0x") else int(lo_text, 10)
            except ValueError:
                lo_val = None
            pending = self.pending_lui.get(rs)
            if pending and pending[0] == "hi_shift" and lo_val is not None:
                hi = pending[1]
                full = (hi + lo_val) & 0xFFFFFFFF
                consumed_pending = rs
                # Try asset lookup first (the dominant use-case for this idiom).
                assets = self.r.asset_with_hash(full)
                annotated = False
                if assets:
                    for a in assets:
                        parts.append(f"= 0x{full:08X} {a.name} [{a.namespace}]")
                    self.stats["asset_hits"] += 1
                    annotated = True
                sym = self.r.symbol_at(full)
                if sym is not None:
                    parts.append(f"= 0x{full:08X} {_fmt_symbol_short(sym)}")
                    self.stats["symbol_refs"] += 1
                    annotated = True
                if not annotated:
                    parts.append(f"= 0x{full:08X} (no symbol/asset)")
                    self.stats["unresolved"] += 1

        # --- 4. Struct-field annotation for OFF($base) loads/stores ----------
        if mem_m:
            base = mem_m.group("base")
            off_text = mem_m.group("off")
            try:
                off = int(off_text, 16) if off_text.lower().startswith("0x") else int(off_text)
            except ValueError:
                off = None
            if off is not None and off >= 0:
                field = self._struct_field_for_load(base, off)
                if field:
                    parts.append(field)
                    self.stats["field_refs"] += 1

        # --- 5. Maintain register state --------------------------------------
        # Any instruction writing a register invalidates prior tracking for it,
        # unless it's the very pattern we just consumed.
        # MIPS register-write conventions vary; use a conservative rule based
        # on the first operand position (sufficient for typical .s output).
        wm = WRITES_REG_RE.match(line_text)
        if wm:
            wreg = wm.group(1)
            # Don't invalidate if this instruction is the lui or addiu we just
            # recorded.
            if lui_named and lui_named.group(1) == wreg:
                pass
            elif lui_shift and lui_shift.group(1) == wreg:
                pass
            elif consumed_pending == wreg and addiu_lo_m and addiu_lo_m.group(1) == wreg:
                # addiu form has already established self.regs[wreg]
                pass
            else:
                # Loads/stores: `lw $rt, ...` overwrites $rt as a value, not
                # an address-of-symbol. Drop tracking.
                self._invalidate_reg(wreg)

        # Re-establish typed-pointer tracking for the `lw $rt, %lo(g_pX)($rs)`
        # pattern AFTER the generic invalidate step (which would have just
        # cleared $rt).
        if ptr_consumed_target is not None and ptr_consumed_rt is not None:
            self.regs[ptr_consumed_rt] = ("typed_ptr", ptr_consumed_target)

        # Clear any pending lui that was actually consumed.
        if consumed_pending is not None:
            self.pending_lui.pop(consumed_pending, None)

        # If we processed a lui that wasn't immediately consumed, leave
        # pending_lui in place for the next instruction to see.

        if parts:
            # Deduplicate while preserving order.
            seen: set[str] = set()
            uniq: list[str] = []
            for p in parts:
                if p not in seen:
                    uniq.append(p)
                    seen.add(p)
            inst.annotation = "  # " + " | ".join(uniq)


# -----------------------------------------------------------------------------
# Output
# -----------------------------------------------------------------------------


ANSI_DIM = "\x1b[2m"
ANSI_GREEN = "\x1b[32m"
ANSI_RESET = "\x1b[0m"


def render_line(inst: Inst, *, drop_bytes: bool, color: bool) -> str:
    if inst.annotation == "" and (drop_bytes is False or inst.rom is None):
        return inst.raw.rstrip("\n")
    # Build a new line, optionally stripping the byte/address comment block.
    if inst.rom is None or inst.mnemonic is None:
        base = inst.raw.rstrip("\n")
    elif drop_bytes:
        base = f"{inst.lead}{inst.mnemonic:<10} {inst.operands}"
    else:
        base = inst.raw.rstrip("\n")
    ann = inst.annotation
    if ann and color:
        ann = ANSI_DIM + ANSI_GREEN + ann + ANSI_RESET
    return base + ann


def annotate_file(
    path: Path, *, drop_bytes: bool, color: bool, json_out: bool,
) -> tuple[str, dict]:
    text = path.read_text(encoding="utf-8", errors="replace")
    lines = text.splitlines()
    r = resolve_mod.Resolver()
    ann = Annotator(r)
    insts: list[Inst] = []
    # Reset state per glabel boundary so cross-function leaks don't mislead.
    for line in lines:
        inst = parse_line(line)
        if inst.is_label and "glabel" in inst.raw:
            ann.regs.clear()
            ann.pending_lui.clear()
        ann.annotate(inst)
        insts.append(inst)
    if json_out:
        records = []
        for i in insts:
            if i.mnemonic is None and not i.is_label:
                continue
            records.append({
                "file": str(path.relative_to(ROOT) if path.is_absolute() and ROOT in path.parents else path),
                "vram": int(i.vram, 16) if i.vram else None,
                "vram_hex": f"0x{i.vram}" if i.vram else None,
                "mnemonic": i.mnemonic,
                "operands": i.operands,
                "annotation": i.annotation.lstrip("# ").strip() if i.annotation else None,
                "is_label": i.is_label,
            })
        return json.dumps(records, indent=2) + "\n", dict(ann.stats)
    rendered = []
    for i in insts:
        rendered.append(render_line(i, drop_bytes=drop_bytes, color=color))
    return "\n".join(rendered) + ("\n" if text.endswith("\n") else ""), dict(ann.stats)


# -----------------------------------------------------------------------------
# CLI
# -----------------------------------------------------------------------------


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    ap.add_argument("files", nargs="+", help="asm .s files to annotate")
    ap.add_argument("-o", "--out", help="write to file (single input only)")
    ap.add_argument(
        "--inplace", metavar="SUFFIX",
        help="write each input to <input><SUFFIX> (e.g. .annot)",
    )
    ap.add_argument(
        "--overwrite", action="store_true",
        help="overwrite each input file in place (no suffix)",
    )
    ap.add_argument(
        "--skip-annotated", action="store_true",
        help="skip files that already contain annotator output (idempotency)",
    )
    ap.add_argument(
        "--no-bytes", action="store_true",
        help="drop the `/* ROM VRAM HEX */` prefix for tighter output",
    )
    color_g = ap.add_mutually_exclusive_group()
    color_g.add_argument("--color", action="store_true", help="force ANSI color")
    color_g.add_argument("--no-color", action="store_true", help="force no color")
    ap.add_argument(
        "--json", action="store_true",
        help="emit JSON records (one per instruction) instead of text",
    )
    ap.add_argument(
        "--stats", action="store_true",
        help="print resolution stats to stderr after each file",
    )
    args = ap.parse_args()

    if args.out and (len(args.files) > 1 or args.inplace or args.overwrite):
        print("--out only works with a single input file and without --inplace/--overwrite",
              file=sys.stderr)
        return 2
    if args.inplace and args.overwrite:
        print("--inplace and --overwrite are mutually exclusive", file=sys.stderr)
        return 2

    # Color auto-detect: only when writing to a TTY and not given --no-color.
    if args.color:
        color = True
    elif args.no_color:
        color = False
    else:
        color = (sys.stdout.isatty() and not args.out
                 and not args.inplace and not args.overwrite)

    overall = {
        "lines": 0, "instructions": 0,
        "symbol_refs": 0, "asset_hits": 0, "field_refs": 0,
        "jumps": 0, "unresolved": 0,
    }

    for fp in args.files:
        path = Path(fp)
        if not path.exists():
            print(f"not found: {fp}", file=sys.stderr)
            return 2
        # Idempotency: detect existing annotations by looking for the
        # signature `  # ` suffix that the annotator emits at end of line.
        # We check for the more specific pattern `  # ` followed by `->`,
        # `@`, `(0x`, or `=` which are annotator-specific markers.
        if args.skip_annotated:
            with path.open("r", encoding="utf-8", errors="replace") as f:
                head = f.read(8192)
            if re.search(r"  # (-> |~ 0x|= 0x|[A-Za-z_]\w* @ 0x)", head):
                continue
        out_text, stats = annotate_file(
            path,
            drop_bytes=args.no_bytes,
            color=color and not args.json,
            json_out=args.json,
        )
        if args.overwrite:
            path.write_text(out_text)
        elif args.inplace:
            dest = path.with_name(path.name + args.inplace)
            dest.write_text(out_text)
            print(f"wrote {dest}", file=sys.stderr)
        elif args.out:
            Path(args.out).write_text(out_text)
            print(f"wrote {args.out}", file=sys.stderr)
        else:
            sys.stdout.write(out_text)
        for k, v in stats.items():
            overall[k] = overall.get(k, 0) + v
        if args.stats:
            resolved = stats["symbol_refs"] + stats["asset_hits"] + stats["field_refs"] + stats["jumps"]
            print(
                f"  {path}: {stats['instructions']} insns, "
                f"{resolved} resolved "
                f"(sym={stats['symbol_refs']}, asset={stats['asset_hits']}, "
                f"field={stats['field_refs']}, jump={stats['jumps']}), "
                f"unresolved={stats['unresolved']}",
                file=sys.stderr,
            )

    if args.stats and len(args.files) > 1:
        resolved = overall["symbol_refs"] + overall["asset_hits"] + overall["field_refs"] + overall["jumps"]
        print(
            f"TOTAL: {overall['instructions']} insns, "
            f"{resolved} resolved "
            f"(sym={overall['symbol_refs']}, asset={overall['asset_hits']}, "
            f"field={overall['field_refs']}, jump={overall['jumps']}), "
            f"unresolved={overall['unresolved']}",
            file=sys.stderr,
        )
    return 0


if __name__ == "__main__":
    sys.exit(main())
