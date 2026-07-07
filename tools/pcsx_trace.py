#!/usr/bin/env python3
"""
pcsx_trace.py -- offline analyzer for raw RAM dumps captured by
scripts/ram_dumper.lua.

The dumper's only job is to write raw region bytes to one file per frame. ALL
interpretation lives here, out of the emulator's hot path: parse GameState (and
any address) against the struct schema in include/Game/*.h, watch a value across
frames, and diff two frames or two whole recordings to find the first divergence
-- the core loop for validating the PC port and decompiled C against PS1 truth.

Struct field offsets are parsed live from the C headers (single source of truth,
auto-syncs with the docs), so `gs` decoding never drifts from include/Game/.

Usage:
  pcsx_trace.py list    <dump_dir>
  pcsx_trace.py gs      <dump_dir> [--frame N] [--fields a,b,c]
  pcsx_trace.py field   <dump_dir> <name|0xADDR:type> [--changes]
  pcsx_trace.py watch   <dump_dir> <name|0xADDR:type>
  pcsx_trace.py hexdump <dump_dir> <0xADDR> <size> [--frame N]
  pcsx_trace.py diff    <dump_dir> <frameA> <frameB> [--region 0xADDR:size]
  pcsx_trace.py diffdir <dirA> <dirB> [--region 0xADDR:size] [--stop-first]

Types for field/watch: u8 s8 u16 s16 u32 s32  (default u32).
"""
import argparse
import json
import os
import re
import struct
import sys
from glob import glob

RAM_MASK = 0x1FFFFF
GS_ADDR_DEFAULT = 0x8009DC40
REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GAME_STATE_HEADER = os.path.join(REPO_ROOT, "include", "Game", "game_state.h")

# --------------------------------------------------------------------------
# Scalar type decoding
# --------------------------------------------------------------------------
TYPES = {
    "u8":  ("<B", 1), "s8":  ("<b", 1), "char": ("<b", 1),
    "u16": ("<H", 2), "s16": ("<h", 2), "short": ("<h", 2),
    "u32": ("<I", 4), "s32": ("<i", 4), "int": ("<i", 4), "long": ("<i", 4),
}


def decode_scalar(buf, ty):
    fmt, size = TYPES[ty]
    if len(buf) < size:
        return None
    return struct.unpack(fmt, buf[:size])[0]


def ctype_to_scalar(ctype):
    """Map a C declaration type to a scalar decode key, or None if opaque."""
    ctype = ctype.strip()
    if "*" in ctype:
        return "u32"  # pointer
    key = ctype.split()[-1] if ctype else ""
    return key if key in TYPES else None


# --------------------------------------------------------------------------
# Header schema parser: extract "/* 0xNN */ ctype name;" fields from a struct
# --------------------------------------------------------------------------
# Match "/* 0xNN */ <decl> ;" then split <decl> into type + name in Python,
# which handles pointers written as either "void *name" or "Type* name".
FIELD_RE = re.compile(r"/\*\s*0x([0-9A-Fa-f]+)\s*\*/\s+([^;{}]+?)\s*;")
_ARRAY_RE = re.compile(r"\[(\d+)\]\s*$")


def parse_struct_fields(header_path):
    """Return ordered list of (offset, ctype, name, array_count)."""
    fields = []
    try:
        text = open(header_path).read()
    except OSError:
        return fields
    for m in FIELD_RE.finditer(text):
        off = int(m.group(1), 16)
        decl = m.group(2).strip()
        # trailing array subscript -> count
        count = 1
        am = _ARRAY_RE.search(decl)
        if am:
            count = int(am.group(1))
            decl = decl[:am.start()].strip()
        # last identifier is the field name; the rest (incl. any '*') is the type
        nm = re.search(r"([A-Za-z_]\w*)$", decl)
        if not nm:
            continue
        name = nm.group(1)
        ctype = decl[:nm.start()].strip()
        if not ctype:            # e.g. a bare identifier line, not a field decl
            continue
        fields.append((off, ctype, name, count))
    # dedupe by offset keeping first, sort by offset
    seen = {}
    for off, ctype, name, count in fields:
        seen.setdefault(off, (off, ctype, name, count))
    return sorted(seen.values(), key=lambda x: x[0])


# --------------------------------------------------------------------------
# Dump directory access
# --------------------------------------------------------------------------
class Dump:
    def __init__(self, path):
        self.path = path
        mpath = os.path.join(path, "manifest.json")
        if os.path.exists(mpath):
            self.manifest = json.load(open(mpath))
        else:
            self.manifest = {}
        self.region_base = self.manifest.get("region_base", 0x80000000)
        self.gs_addr = self.manifest.get("gamestate_addr", GS_ADDR_DEFAULT)
        files = sorted(glob(os.path.join(path, "frame_*.bin")))
        self.frames = []       # list of (frame_no, filepath)
        for fp in files:
            m = re.search(r"frame_(\d+)\.bin$", fp)
            if m:
                self.frames.append((int(m.group(1)), fp))
        self.frames.sort()
        self._cache = {}

    def __bool__(self):
        return bool(self.frames)

    def frame_numbers(self):
        return [n for n, _ in self.frames]

    def nearest(self, frame_no):
        """Return (frame_no, path) at or before frame_no, else first."""
        chosen = self.frames[0]
        for n, fp in self.frames:
            if n <= frame_no:
                chosen = (n, fp)
            else:
                break
        return chosen

    def load(self, path):
        if path not in self._cache:
            self._cache[path] = open(path, "rb").read()
        return self._cache[path]

    def blob_index(self, addr):
        """Index into a frame blob for a PSX address, or None if out of range."""
        idx = (addr & RAM_MASK) - (self.region_base & RAM_MASK)
        size = self.manifest.get("region_size")
        if idx < 0:
            return None
        if size is not None and idx >= size:
            return None
        return idx

    def read(self, buf, addr, size):
        idx = self.blob_index(addr)
        if idx is None or idx + size > len(buf):
            return None
        return buf[idx:idx + size]


# --------------------------------------------------------------------------
# Field spec parsing:  "name"  or  "0xADDR:type"
# --------------------------------------------------------------------------
def resolve_spec(spec, dump, gs_fields):
    """Return (addr, ctype_scalar, label)."""
    if ":" in spec:
        astr, ty = spec.split(":", 1)
        ty = ty.strip()
        if ty not in TYPES:
            sys.exit(f"unknown type '{ty}' (use: {', '.join(TYPES)})")
        return (int(astr, 16), ty, f"{astr}:{ty}")
    # named GameState field
    for off, ctype, name, count in gs_fields:
        if name == spec:
            sc = ctype_to_scalar(ctype)
            if sc is None:
                sys.exit(f"field '{name}' has non-scalar type '{ctype}'; "
                         f"use 0xADDR:type instead")
            return (dump.gs_addr + off, sc, f"{name}@+0x{off:X}")
    sys.exit(f"unknown field '{spec}'. Run 'gs' to list, or use 0xADDR:type.")


def fmt_val(v, ty):
    if v is None:
        return "?"
    if ty in ("u8", "u16", "u32"):
        width = {"u8": 2, "u16": 4, "u32": 8}[ty]
        return f"{v} (0x{v:0{width}X})"
    return str(v)


# --------------------------------------------------------------------------
# Commands
# --------------------------------------------------------------------------
def cmd_list(args):
    d = Dump(args.dump_dir)
    if not d:
        sys.exit(f"no frames in {args.dump_dir}")
    nums = d.frame_numbers()
    print(f"dir:         {d.path}")
    print(f"frames:      {len(nums)}  ({nums[0]}..{nums[-1]})")
    print(f"region_base: 0x{d.region_base:08X}")
    print(f"region_size: {d.manifest.get('region_size', '?')}")
    print(f"gamestate:   0x{d.gs_addr:08X}")
    if "level" in d.manifest:
        print(f"level/stage: {d.manifest['level']}/{d.manifest.get('stage', 0)}")
    print(f"complete:    {d.manifest.get('complete', '?')}")


def cmd_gs(args):
    d = Dump(args.dump_dir)
    if not d:
        sys.exit(f"no frames in {args.dump_dir}")
    fields = parse_struct_fields(GAME_STATE_HEADER)
    if not fields:
        sys.exit(f"could not parse GameState fields from {GAME_STATE_HEADER}")
    want = set(args.fields.split(",")) if args.fields else None
    frame_no = args.frame if args.frame is not None else d.frame_numbers()[-1]
    fn, fp = d.nearest(frame_no)
    buf = d.load(fp)
    print(f"# GameState @ 0x{d.gs_addr:08X}  frame {fn}")
    for off, ctype, name, count in fields:
        if want and name not in want:
            continue
        sc = ctype_to_scalar(ctype)
        addr = d.gs_addr + off
        if sc is None or count > 1:
            raw = d.read(buf, addr, 4)
            hexs = raw.hex() if raw else "??"
            print(f"  +0x{off:03X} {name:<32} {ctype:<12} [{hexs}]")
        else:
            raw = d.read(buf, addr, TYPES[sc][1])
            v = decode_scalar(raw, sc) if raw else None
            print(f"  +0x{off:03X} {name:<32} {ctype:<12} = {fmt_val(v, sc)}")


def cmd_field(args):
    d = Dump(args.dump_dir)
    if not d:
        sys.exit(f"no frames in {args.dump_dir}")
    fields = parse_struct_fields(GAME_STATE_HEADER)
    addr, ty, label = resolve_spec(args.spec, d, fields)
    print(f"# {label}  @ 0x{addr:08X}  ({ty})")
    prev = object()
    for fn, fp in d.frames:
        v = decode_scalar(d.read(d.load(fp), addr, TYPES[ty][1]), ty)
        if args.changes and v == prev:
            continue
        marker = " *" if (args.changes and prev is not object()) else ""
        print(f"  frame {fn:>7}: {fmt_val(v, ty)}{marker}")
        prev = v


def cmd_watch(args):
    """Report every frame at which the value changes (first-write finder)."""
    d = Dump(args.dump_dir)
    if not d:
        sys.exit(f"no frames in {args.dump_dir}")
    fields = parse_struct_fields(GAME_STATE_HEADER)
    addr, ty, label = resolve_spec(args.spec, d, fields)
    print(f"# watch {label} @ 0x{addr:08X} ({ty}) -- change points")
    prev = object()
    changes = 0
    for fn, fp in d.frames:
        v = decode_scalar(d.read(d.load(fp), addr, TYPES[ty][1]), ty)
        if v != prev:
            if prev is object():
                print(f"  frame {fn:>7}: {fmt_val(v, ty)}  (initial)")
            else:
                print(f"  frame {fn:>7}: {fmt_val(prev, ty)} -> {fmt_val(v, ty)}")
            changes += 1
            prev = v
    print(f"# {changes} change point(s) across {len(d.frames)} frames")


def cmd_hexdump(args):
    d = Dump(args.dump_dir)
    if not d:
        sys.exit(f"no frames in {args.dump_dir}")
    addr = int(args.addr, 16)
    size = int(args.size, 0)
    frame_no = args.frame if args.frame is not None else d.frame_numbers()[-1]
    fn, fp = d.nearest(frame_no)
    raw = d.read(d.load(fp), addr, size)
    if raw is None:
        sys.exit(f"0x{addr:08X}+{size} out of captured region")
    print(f"# 0x{addr:08X} .. 0x{addr+size:08X}  frame {fn}")
    for i in range(0, len(raw), 16):
        chunk = raw[i:i + 16]
        hexs = " ".join(f"{b:02X}" for b in chunk)
        asc = "".join(chr(b) if 32 <= b < 127 else "." for b in chunk)
        print(f"  {addr+i:08X}: {hexs:<47}  {asc}")


def _region_arg(spec):
    astr, sstr = spec.split(":", 1)
    return int(astr, 16), int(sstr, 0)


def _diff_regions(a, b, base_addr):
    """Yield (addr, a_byte, b_byte) for differing bytes."""
    n = min(len(a), len(b))
    for i in range(n):
        if a[i] != b[i]:
            yield base_addr + i, a[i], b[i]


def _annotate(addr, gs_addr, fields):
    """Return 'field@+0xNN' if addr lands inside a known GameState field."""
    if not fields:
        return ""
    rel = addr - gs_addr
    if rel < 0:
        return ""
    best = ""
    for off, ctype, name, count in fields:
        if off <= rel:
            best = f"{name}+0x{rel-off:X}" if rel != off else name
        else:
            break
    return best


def cmd_diff(args):
    d = Dump(args.dump_dir)
    if not d:
        sys.exit(f"no frames in {args.dump_dir}")
    fields = parse_struct_fields(GAME_STATE_HEADER)
    fa, pa = d.nearest(args.frameA)
    fb, pb = d.nearest(args.frameB)
    ba, bb = d.load(pa), d.load(pb)
    if args.region:
        addr, size = _region_arg(args.region)
        sa, sb = d.read(ba, addr, size), d.read(bb, addr, size)
        base = addr
    else:
        sa, sb, base = ba, bb, d.region_base
    print(f"# diff frame {fa} vs {fb}  base 0x{base:08X}")
    n = 0
    for addr, x, y in _diff_regions(sa, sb, base):
        ann = _annotate(addr, d.gs_addr, fields)
        print(f"  0x{addr:08X}: {x:02X} -> {y:02X}   {ann}")
        n += 1
        if n >= args.limit:
            print(f"  ... (truncated at {args.limit}; raise --limit)")
            break
    print(f"# {n} differing byte(s)")


def cmd_diffdir(args):
    da, db = Dump(args.dirA), Dump(args.dirB)
    if not da or not db:
        sys.exit("both directories need frames")
    fields = parse_struct_fields(GAME_STATE_HEADER)
    common = sorted(set(da.frame_numbers()) & set(db.frame_numbers()))
    if not common:
        sys.exit("no common frame numbers between the two recordings")
    region = _region_arg(args.region) if args.region else None
    print(f"# diffdir {da.path}  vs  {db.path}   ({len(common)} common frames)")
    first_div = None
    for fn in common:
        _, pa = da.nearest(fn)
        _, pb = db.nearest(fn)
        ba, bb = da.load(pa), db.load(pb)
        if region:
            addr, size = region
            sa, sb, base = da.read(ba, addr, size), db.read(bb, addr, size), addr
        else:
            sa, sb, base = ba, bb, da.region_base
        diffs = list(_diff_regions(sa, sb, base))
        if diffs:
            if first_div is None:
                first_div = fn
            addr, x, y = diffs[0]
            ann = _annotate(addr, da.gs_addr, fields)
            print(f"  frame {fn:>7}: {len(diffs):>6} diff bytes; "
                  f"first 0x{addr:08X} {x:02X}!={y:02X} {ann}")
            if args.stop_first:
                break
    if first_div is None:
        print("# recordings are byte-identical over the compared region")
    else:
        print(f"# first divergence at frame {first_div}")


# --------------------------------------------------------------------------
def main():
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest="cmd", required=True)

    sp = sub.add_parser("list"); sp.add_argument("dump_dir"); sp.set_defaults(fn=cmd_list)

    sp = sub.add_parser("gs"); sp.add_argument("dump_dir")
    sp.add_argument("--frame", type=int); sp.add_argument("--fields")
    sp.set_defaults(fn=cmd_gs)

    sp = sub.add_parser("field"); sp.add_argument("dump_dir"); sp.add_argument("spec")
    sp.add_argument("--changes", action="store_true"); sp.set_defaults(fn=cmd_field)

    sp = sub.add_parser("watch"); sp.add_argument("dump_dir"); sp.add_argument("spec")
    sp.set_defaults(fn=cmd_watch)

    sp = sub.add_parser("hexdump"); sp.add_argument("dump_dir")
    sp.add_argument("addr"); sp.add_argument("size"); sp.add_argument("--frame", type=int)
    sp.set_defaults(fn=cmd_hexdump)

    sp = sub.add_parser("diff"); sp.add_argument("dump_dir")
    sp.add_argument("frameA", type=int); sp.add_argument("frameB", type=int)
    sp.add_argument("--region"); sp.add_argument("--limit", type=int, default=200)
    sp.set_defaults(fn=cmd_diff)

    sp = sub.add_parser("diffdir"); sp.add_argument("dirA"); sp.add_argument("dirB")
    sp.add_argument("--region"); sp.add_argument("--stop-first", action="store_true")
    sp.set_defaults(fn=cmd_diffdir)

    args = p.parse_args()
    args.fn(args)


if __name__ == "__main__":
    main()
