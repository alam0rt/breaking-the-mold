#!/usr/bin/env python3
"""
pcsx_stream.py -- streaming consumer + query tool for PS1 RAM traces.

The emulator side (scripts/ram_dumper.lua in FIFO mode) stays DUMB: it frames
raw region bytes and shoves them down a pipe. THIS process, external to the
emulator, does everything expensive -- delta-compresses frames and decodes
struct fields -- so the emulator hot path never pays for analysis (that in-VM
parsing is exactly what made the old game_watcher.lua slow and crashy).

Everything lands in ONE SQLite file:
  * frame_store(frame, kind, payload)  -- keyframe/xor-delta, zlib-compressed.
      Reconstructs ANY frame's full region for byte-diffing PS1 vs the port.
  * field(frame, name, addr, val)      -- decoded GameState scalars, stored on
      change only, so `watch` is a trivial query and the DB stays small.
  * meta(key, value)                   -- region base/size, gs addr, level/stage.

Wire protocol (one record per frame): b"FRM1" + frame:u32le + size:u32le + payload.

Commands:
  consume  --fifo P --db D --base 0xADDR --size N [--gs 0xADDR] [--keyframe 50]
  watch    D <name|0xADDR:type>          change points of a value over time
  field    D <name|0xADDR:type>          value every recorded frame
  gs       D [--frame N]                  decoded GameState at a frame
  frame    D N -o out.bin                 reconstruct full region of frame N
  diffdb   DA DB [--region 0xADDR:size]   first divergence between two traces
  info     D
"""
import argparse
import os
import sqlite3
import struct
import sys
import zlib

# reuse the header-schema parser + scalar decoders from the sibling tool
_HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, _HERE)
import pcsx_trace as pt  # noqa: E402

MAGIC = b"FRM1"
HDR = struct.Struct("<4sII")
KIND_KEY = 0
KIND_DELTA = 1

# region spec -> (base, size); MUST match parse_region() in scripts/ram_dumper.lua
RAM_BASE, RAM_SIZE = 0x80000000, 0x200000
GAMESTATE_SLICE = (0x8009DC40, 0x00013000)


def resolve_region(spec):
    if spec == "full":
        return RAM_BASE, RAM_SIZE
    if spec == "gamestate":
        return GAMESTATE_SLICE
    astr, sstr = spec.split(":", 1)
    return int(astr, 0), int(sstr, 0)


# --------------------------------------------------------------------------
# Delta codec: runs of changed bytes vs the previous stored frame.
# --------------------------------------------------------------------------
def pack_delta(prev, cur):
    """Return zlib(concat of (off:u32, len:u32, bytes) runs) for prev->cur."""
    runs = bytearray()
    n = len(cur)
    i = 0
    while i < n:
        if i < len(prev) and prev[i] == cur[i]:
            i += 1
            continue
        start = i
        while i < n and (i >= len(prev) or prev[i] != cur[i]):
            i += 1
        runs += struct.pack("<II", start, i - start)
        runs += cur[start:i]
    return zlib.compress(bytes(runs), 6)


def apply_delta(buf, payload):
    runs = zlib.decompress(payload)
    p = 0
    while p < len(runs):
        off, length = struct.unpack_from("<II", runs, p)
        p += 8
        buf[off:off + length] = runs[p:p + length]
        p += length
    return buf


# --------------------------------------------------------------------------
# Store
# --------------------------------------------------------------------------
class Store:
    def __init__(self, path, create=False):
        self.db = sqlite3.connect(path)
        self.db.execute("PRAGMA journal_mode=WAL")
        self.db.execute("PRAGMA synchronous=NORMAL")
        if create:
            self._schema()
        self._meta = None

    def _schema(self):
        c = self.db
        c.execute("CREATE TABLE IF NOT EXISTS meta(key TEXT PRIMARY KEY, value TEXT)")
        c.execute("CREATE TABLE IF NOT EXISTS frame_store("
                  "frame INTEGER PRIMARY KEY, kind INTEGER, payload BLOB)")
        c.execute("CREATE TABLE IF NOT EXISTS field("
                  "frame INTEGER, name TEXT, addr INTEGER, val INTEGER)")
        c.execute("CREATE INDEX IF NOT EXISTS ix_field ON field(name, frame)")
        c.commit()

    def set_meta(self, **kw):
        self.db.executemany("INSERT OR REPLACE INTO meta VALUES(?,?)",
                            [(k, str(v)) for k, v in kw.items()])
        self.db.commit()

    def meta(self, key, default=None, cast=str):
        if self._meta is None:
            self._meta = dict(self.db.execute("SELECT key,value FROM meta"))
        v = self._meta.get(key)
        return cast(v) if v is not None else default

    @property
    def base(self): return self.meta("region_base", 0x80000000, int)
    @property
    def size(self): return self.meta("region_size", 0, int)
    @property
    def gs_addr(self): return self.meta("gamestate_addr", pt.GS_ADDR_DEFAULT, int)

    def frame_numbers(self):
        return [r[0] for r in self.db.execute(
            "SELECT frame FROM frame_store ORDER BY frame")]

    def reconstruct(self, frame):
        """Full region bytes for `frame` (nearest stored <= frame)."""
        row = self.db.execute(
            "SELECT frame FROM frame_store WHERE frame<=? ORDER BY frame DESC LIMIT 1",
            (frame,)).fetchone()
        if not row:
            return None
        target = row[0]
        # latest keyframe at or before target
        kf = self.db.execute(
            "SELECT frame,payload FROM frame_store "
            "WHERE frame<=? AND kind=? ORDER BY frame DESC LIMIT 1",
            (target, KIND_KEY)).fetchone()
        if not kf:
            return None
        buf = bytearray(zlib.decompress(kf[1]))
        for fr, payload in self.db.execute(
                "SELECT frame,payload FROM frame_store "
                "WHERE frame>? AND frame<=? AND kind=? ORDER BY frame",
                (kf[0], target, KIND_DELTA)):
            apply_delta(buf, payload)
        return buf

    def read(self, buf, addr, sz):
        idx = (addr & pt.RAM_MASK) - (self.base & pt.RAM_MASK)
        if idx < 0 or idx + sz > len(buf):
            return None
        return bytes(buf[idx:idx + sz])

    # as-of value: latest recorded change at or before `frame`
    def value_at(self, name, frame):
        r = self.db.execute(
            "SELECT val FROM field WHERE name=? AND frame<=? "
            "ORDER BY frame DESC LIMIT 1", (name, frame)).fetchone()
        return r[0] if r else None

    def changes(self, name):
        return list(self.db.execute(
            "SELECT frame,val FROM field WHERE name=? ORDER BY frame", (name,)))


# --------------------------------------------------------------------------
# consume: read FIFO/stream -> store
# --------------------------------------------------------------------------
def _read_exact(f, n):
    buf = b""
    while len(buf) < n:
        chunk = f.read(n - len(buf))
        if not chunk:
            return None
        buf += chunk
    return buf


def cmd_consume(args):
    base, size = resolve_region(args.region)
    args.base, args.size = base, size
    st = Store(args.db, create=True)
    st.set_meta(region_base=base, region_size=size,
                gamestate_addr=args.gs,
                level=args.level if args.level is not None else "",
                stage=args.stage)
    fields = [(o, ct, nm, cnt) for (o, ct, nm, cnt) in
              pt.parse_struct_fields(pt.GAME_STATE_HEADER)
              if pt.ctype_to_scalar(ct) and cnt == 1]
    gs_off = args.gs - args.base

    src = open(args.fifo, "rb")
    print(f"[consume] reading {args.fifo} -> {args.db} "
          f"(region {args.size} B, keyframe every {args.keyframe})")
    prev = None
    prev_vals = {}
    count = 0
    stored = 0
    try:
        while True:
            hdr = _read_exact(src, HDR.size)
            if hdr is None:
                break
            magic, frame_no, size = HDR.unpack(hdr)
            if magic != MAGIC:
                sys.stderr.write(f"[consume] bad magic {magic!r}; resyncing\n")
                # crude resync: scan for next MAGIC
                continue
            payload = _read_exact(src, size)
            if payload is None:
                break
            cur = bytearray(payload)

            # frame store: keyframe or delta
            if prev is None or (count % args.keyframe) == 0:
                blob = zlib.compress(bytes(cur), 6)
                st.db.execute("INSERT OR REPLACE INTO frame_store VALUES(?,?,?)",
                              (frame_no, KIND_KEY, blob))
            else:
                st.db.execute("INSERT OR REPLACE INTO frame_store VALUES(?,?,?)",
                              (frame_no, KIND_DELTA, pack_delta(prev, cur)))

            # decode fields, store on change only
            rows = []
            for off, ctype, name, _ in fields:
                sc = pt.ctype_to_scalar(ctype)
                sz = pt.TYPES[sc][1]
                a = gs_off + off
                if a + sz > len(cur):
                    continue
                v = struct.unpack(pt.TYPES[sc][0], bytes(cur[a:a + sz]))[0]
                if prev_vals.get(name) != v:
                    rows.append((frame_no, name, args.gs + off, v))
                    prev_vals[name] = v
            if rows:
                st.db.executemany("INSERT INTO field VALUES(?,?,?,?)", rows)

            prev = cur
            count += 1
            stored += 1
            if (stored % 60) == 0:
                st.db.commit()
                print(f"[consume] {stored} frames (last frame#{frame_no})")
    except KeyboardInterrupt:
        print("[consume] interrupted")
    finally:
        st.set_meta(frames=stored, complete=1)
        st.db.commit()
        print(f"[consume] done: {stored} frames -> {args.db}")


# --------------------------------------------------------------------------
# query commands
# --------------------------------------------------------------------------
def _resolve(spec, st):
    """(addr, scalar_type, label) from 'name' or '0xADDR:type'."""
    if ":" in spec:
        astr, ty = spec.split(":", 1)
        if ty not in pt.TYPES:
            sys.exit(f"unknown type '{ty}'")
        return int(astr, 16), ty, spec
    for off, ctype, name, _ in pt.parse_struct_fields(pt.GAME_STATE_HEADER):
        if name == spec:
            return st.gs_addr + off, pt.ctype_to_scalar(ctype), f"{name}@+0x{off:X}"
    sys.exit(f"unknown field '{spec}'")


def _fmt(v, ty):
    if v is None:
        return "?"
    if ty and ty.startswith("u"):
        w = {"u8": 2, "u16": 4, "u32": 8}[ty]
        return f"{v} (0x{v:0{w}X})"
    return str(v)


def cmd_watch(args):
    st = Store(args.db)
    _, ty, label = _resolve(args.spec, st)
    name = args.spec.split(":")[0] if ":" in args.spec else args.spec
    rows = st.changes(name) if ":" not in args.spec else None
    print(f"# watch {label} -- change points")
    if rows is None:
        sys.exit("address-form watch needs reconstruction; use a field name here")
    prev = None
    for fr, v in rows:
        if prev is None:
            print(f"  frame {fr:>7}: {_fmt(v, ty)}  (initial)")
        else:
            print(f"  frame {fr:>7}: {_fmt(prev, ty)} -> {_fmt(v, ty)}")
        prev = v
    print(f"# {len(rows)} change point(s)")


def cmd_field(args):
    st = Store(args.db)
    _, ty, label = _resolve(args.spec, st)
    name = args.spec.split(":")[0] if ":" in args.spec else args.spec
    print(f"# {label}")
    for fr, v in st.changes(name):
        print(f"  frame {fr:>7}: {_fmt(v, ty)}")


def cmd_gs(args):
    st = Store(args.db)
    frames = st.frame_numbers()
    if not frames:
        sys.exit("no frames")
    fno = args.frame if args.frame is not None else frames[-1]
    print(f"# GameState @ 0x{st.gs_addr:08X}  (as of frame {fno})")
    for off, ctype, name, cnt in pt.parse_struct_fields(pt.GAME_STATE_HEADER):
        sc = pt.ctype_to_scalar(ctype)
        if sc is None or cnt > 1:
            continue
        v = st.value_at(name, fno)
        print(f"  +0x{off:03X} {name:<32} {ctype:<12} = {_fmt(v, sc)}")


def cmd_frame(args):
    st = Store(args.db)
    buf = st.reconstruct(args.n)
    if buf is None:
        sys.exit(f"frame {args.n} not in store")
    with open(args.o, "wb") as f:
        f.write(buf)
    print(f"# reconstructed frame {args.n} ({len(buf)} B) -> {args.o}")


def cmd_diffdb(args):
    a, b = Store(args.dba), Store(args.dbb)
    common = sorted(set(a.frame_numbers()) & set(b.frame_numbers()))
    if not common:
        sys.exit("no common frames")
    if args.region:
        astr, sstr = args.region.split(":")
        raddr, rsz = int(astr, 16), int(sstr, 0)
    else:
        raddr, rsz = None, None
    fields = pt.parse_struct_fields(pt.GAME_STATE_HEADER)
    print(f"# diffdb {args.dba} vs {args.dbb} ({len(common)} common frames)")
    first = None
    for fr in common:
        ba, bb = a.reconstruct(fr), b.reconstruct(fr)
        if raddr is not None:
            sa, sb, base = a.read(ba, raddr, rsz), b.read(bb, raddr, rsz), raddr
        else:
            sa, sb, base = bytes(ba), bytes(bb), a.base
        diffs = [(base + i, sa[i], sb[i])
                 for i in range(min(len(sa), len(sb))) if sa[i] != sb[i]]
        if diffs:
            first = first or fr
            addr, x, y = diffs[0]
            ann = pt._annotate(addr, a.gs_addr, fields)
            print(f"  frame {fr:>7}: {len(diffs):>6} diff bytes; "
                  f"first 0x{addr:08X} {x:02X}!={y:02X} {ann}")
            if args.stop_first:
                break
    print("# identical" if first is None else f"# first divergence at frame {first}")


def cmd_info(args):
    st = Store(args.db)
    frames = st.frame_numbers()
    nfield = st.db.execute("SELECT COUNT(*) FROM field").fetchone()[0]
    size_b = os.path.getsize(args.db)
    print(f"db:          {args.db}  ({size_b/1e6:.1f} MB)")
    print(f"frames:      {len(frames)}"
          + (f"  ({frames[0]}..{frames[-1]})" if frames else ""))
    print(f"region_base: 0x{st.base:08X}   size: {st.size}")
    print(f"gamestate:   0x{st.gs_addr:08X}")
    print(f"field rows:  {nfield} (stored on change)")
    if st.meta("level"):
        print(f"level/stage: {st.meta('level')}/{st.meta('stage')}")


# --------------------------------------------------------------------------
def main():
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest="cmd", required=True)

    c = sub.add_parser("consume")
    c.add_argument("--fifo", required=True)
    c.add_argument("--db", required=True)
    c.add_argument("--region", default="full",
                   help="full | gamestate | 0xADDR:SIZE (match ram_dumper.lua)")
    c.add_argument("--gs", type=lambda s: int(s, 0), default=pt.GS_ADDR_DEFAULT)
    c.add_argument("--keyframe", type=int, default=50)
    c.add_argument("--level", type=int, default=None)
    c.add_argument("--stage", type=int, default=0)
    c.set_defaults(fn=cmd_consume)

    c = sub.add_parser("watch"); c.add_argument("db"); c.add_argument("spec"); c.set_defaults(fn=cmd_watch)
    c = sub.add_parser("field"); c.add_argument("db"); c.add_argument("spec"); c.set_defaults(fn=cmd_field)
    c = sub.add_parser("gs"); c.add_argument("db"); c.add_argument("--frame", type=int); c.set_defaults(fn=cmd_gs)
    c = sub.add_parser("frame"); c.add_argument("db"); c.add_argument("n", type=int); c.add_argument("-o", required=True); c.set_defaults(fn=cmd_frame)
    c = sub.add_parser("diffdb"); c.add_argument("dba"); c.add_argument("dbb"); c.add_argument("--region"); c.add_argument("--stop-first", action="store_true"); c.set_defaults(fn=cmd_diffdb)
    c = sub.add_parser("info"); c.add_argument("db"); c.set_defaults(fn=cmd_info)

    args = p.parse_args()
    args.fn(args)


if __name__ == "__main__":
    main()
