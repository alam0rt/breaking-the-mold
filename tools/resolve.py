#!/usr/bin/env python3
"""
resolve.py - Look up Skullmonkeys symbols, asset hashes, and addresses.

Combines:
  symbol_addrs.txt          (functions, globals, vtables)
  include/Game/asset_ids.h  (calcHash asset IDs grouped by namespace)

Heuristic dispatch on the query argument:
  0x80009DC40            -> address lookup (exact + nearest-containing symbol)
  0x80a0c240             -> address lookup; also matches asset hash if known
  FreeFromHeap           -> symbol name lookup (exact then substring)
  FX_AMBIENT_BLIZZARD    -> asset name lookup
  /AMBIENT/              -> regex (slashes delimit)

Examples:
  tools/resolve.py 0x8009DC40
  tools/resolve.py FreeFromHeap
  tools/resolve.py 0x80a0c240
  tools/resolve.py /^FX_KLAY_/ --kind=asset
  tools/resolve.py 0x80042AB4 --long
  tools/resolve.py --addr-info 0x80059e10

Exit codes:
  0  match found
  1  no match
  2  invalid argument
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from dataclasses import dataclass, asdict
from pathlib import Path
from typing import Iterable

ROOT = Path(__file__).resolve().parent.parent
SYMBOL_FILE = ROOT / "symbol_addrs.txt"
ASSET_FILE = ROOT / "include" / "Game" / "asset_ids.h"

sys.path.insert(0, str(ROOT / "tools"))
try:
    import hdr2schema
    _SCHEMA_OK = True
except Exception:
    hdr2schema = None
    _SCHEMA_OK = False

RAM_REGIONS = [
    (0x00000000, 0x001FFFFF, "KUSEG cached RAM"),
    (0x80000000, 0x801FFFFF, "KSEG0 cached RAM"),
    (0xA0000000, 0xA01FFFFF, "KSEG1 uncached RAM"),
    (0x1F800000, 0x1F8003FF, "scratchpad (D-cache)"),
    (0x1F801000, 0x1F803FFF, "hardware registers"),
    (0xBFC00000, 0xBFC7FFFF, "BIOS"),
    (0x20000000, 0x2FFFFFFF, "Ghidra GTE-pseudo memory"),
]


@dataclass
class Symbol:
    name: str
    address: int
    size: int | None
    type: str | None
    comment: str
    source: str = "symbol_addrs.txt"

    @property
    def end(self) -> int:
        return self.address + (self.size or 0)

    def to_json(self) -> dict:
        return {
            "name": self.name,
            "address": self.address,
            "address_hex": f"0x{self.address:08X}",
            "size": self.size,
            "size_hex": f"0x{self.size:X}" if self.size is not None else None,
            "type": self.type,
            "comment": self.comment,
            "source": self.source,
        }


@dataclass
class Asset:
    name: str
    hash_value: int
    namespace: str
    line: int

    def to_json(self) -> dict:
        return {
            "name": self.name,
            "hash": self.hash_value,
            "hash_hex": f"0x{self.hash_value:08x}",
            "namespace": self.namespace,
            "source": f"include/Game/asset_ids.h:{self.line}",
        }


SYMBOL_LINE_RE = re.compile(
    r"""^\s*
        (?P<name>[A-Za-z_][\w]*)
        \s*=\s*
        0x(?P<addr>[0-9A-Fa-f]+)
        \s*;
        (?P<trailer>.*)$
    """,
    re.VERBOSE,
)
SIZE_HINT_RE = re.compile(r"size:\s*(0x[0-9A-Fa-f]+|\d+)", re.IGNORECASE)
TYPE_HINT_RE = re.compile(r"type:\s*([\w*\s]+?)\s*(?://|$)")
DEFINE_RE = re.compile(
    r"^\s*#define\s+(?P<name>[A-Z_][A-Z0-9_]*)\s+0x(?P<val>[0-9A-Fa-f]+)u?\b"
)
SECTION_RE = re.compile(r"^/\*\s*-+\s*(.*?)\s*-+\s*\*/\s*$")


def parse_int_arg(text: str) -> int | None:
    t = text.strip().rstrip("uUlL")
    if not t:
        return None
    try:
        if t.lower().startswith("0x"):
            return int(t, 16)
        if all(c in "0123456789abcdefABCDEF" for c in t) and len(t) >= 4:
            try:
                return int(t, 16)
            except ValueError:
                pass
        return int(t, 10)
    except ValueError:
        return None


def parse_size(text: str) -> int | None:
    if text is None:
        return None
    text = text.strip()
    if text.lower().startswith("0x"):
        try:
            return int(text, 16)
        except ValueError:
            return None
    try:
        return int(text, 10)
    except ValueError:
        return None


def load_symbols(path: Path = SYMBOL_FILE) -> list[Symbol]:
    if not path.exists():
        return []
    out = []
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        if not line.strip() or line.lstrip().startswith("//"):
            continue
        m = SYMBOL_LINE_RE.match(line)
        if not m:
            continue
        addr = int(m.group("addr"), 16)
        trailer = m.group("trailer") or ""
        size_m = SIZE_HINT_RE.search(trailer)
        type_m = TYPE_HINT_RE.search(trailer)

        comment = trailer
        for r in (SIZE_HINT_RE, TYPE_HINT_RE):
            comment = r.sub("", comment)
        comment = re.sub(r"/\*.*?\*/", "", comment)
        comment = comment.lstrip("/ \t").strip()
        out.append(
            Symbol(
                name=m.group("name"),
                address=addr,
                size=parse_size(size_m.group(1)) if size_m else None,
                type=type_m.group(1).strip() if type_m else None,
                comment=comment,
            )
        )
    return out


def load_assets(path: Path = ASSET_FILE) -> list[Asset]:
    if not path.exists():
        return []
    out = []
    current_section = "(unknown)"
    for lineno, line in enumerate(
        path.read_text(encoding="utf-8", errors="replace").splitlines(), start=1
    ):
        sec = SECTION_RE.match(line.strip())
        if sec:
            current_section = sec.group(1)
            continue
        m = DEFINE_RE.match(line)
        if not m:
            continue
        out.append(
            Asset(
                name=m.group("name"),
                hash_value=int(m.group("val"), 16),
                namespace=current_section,
                line=lineno,
            )
        )
    return out


def ram_region(addr: int) -> str | None:
    for start, end, label in RAM_REGIONS:
        if start <= addr <= end:
            return label
    return None


class Resolver:
    def __init__(self) -> None:
        self._structs = None
        self.symbols = load_symbols()
        self.assets = load_assets()
        self._backfill_struct_sizes()
        self.symbols_by_addr = {}
        for s in self.symbols:
            self.symbols_by_addr.setdefault(s.address, []).append(s)
        self.symbols_by_name = {s.name: s for s in self.symbols}
        self.symbols_sorted = sorted(self.symbols, key=lambda s: s.address)
        self.assets_by_hash = {}
        for a in self.assets:
            self.assets_by_hash.setdefault(a.hash_value, []).append(a)
        self.assets_by_name = {a.name: a for a in self.assets}

    def _backfill_struct_sizes(self) -> None:
        structs = self._load_structs()
        if not structs:
            return
        for s in self.symbols:
            if s.size is not None or not s.type:
                continue
            base = s.type.rstrip("*").strip()
            st = structs.get(base)
            if st is None:
                continue
            sz = st.declared_size or st.computed_size
            if not sz:
                continue
            s.size = sz

    def _load_structs(self) -> dict:
        if self._structs is not None:
            return self._structs
        if not _SCHEMA_OK:
            self._structs = {}
            return self._structs
        try:
            headers = hdr2schema.gather_headers([hdr2schema.DEFAULT_HEADER_DIR])
            structs, _enums, _errs = hdr2schema.parse_all(headers)
            self._structs = structs
        except Exception:
            self._structs = {}
        return self._structs

    def structs(self) -> dict:
        """Public accessor for the parsed struct schema (name -> Struct)."""
        return self._load_structs()

    def field_at(self, type_name: str, offset: int) -> tuple[str, int, str] | None:
        """Return (field_name, field_offset, raw_type) at the given offset
        within `type_name`, if known. Picks the last field whose
        offset <= the query (handles arrays / nested padding)."""
        structs = self._load_structs()
        s = structs.get(type_name)
        if s is None:
            return None
        best = None
        for f in s.fields:
            f_size = f.size or 0
            if f.offset <= offset < f.offset + max(f_size, 1):
                best = (f.name, f.offset, f.raw_type)
        if best is not None:
            return best
        for f in s.fields:
            if f.offset <= offset and (best is None or f.offset > best[1]):
                best = (f.name, f.offset, f.raw_type)
        return best

    def symbol_at(self, addr: int) -> Symbol | None:
        hits = self.symbols_by_addr.get(addr)
        return hits[0] if hits else None

    def symbol_containing(self, addr: int) -> Symbol | None:
        """Largest symbol whose [address, address+size) contains addr."""
        best = None
        for s in self.symbols_sorted:
            if s.address > addr:
                return best
            end = s.address + (s.size or 0)
            if s.address <= addr < end:
                if best is None or s.address > best.address:
                    best = s
        return best

    def neighbors(
        self, addr: int, window: int = 3
    ) -> tuple[list[Symbol], list[Symbol]]:
        """Up to `window` symbols immediately before / after `addr`."""
        before = []
        after = []
        for s in self.symbols_sorted:
            if s.address < addr:
                before.append(s)
            elif s.address > addr:
                after.append(s)
                if len(after) >= window:
                    break
        return before[-window:], after

    def asset_with_hash(self, h: int) -> list[Asset]:
        return list(self.assets_by_hash.get(h, []))

    def find_symbols(
        self, pat: str, regex: bool, max_hits: int = 50
    ) -> list[Symbol]:
        if not regex:
            exact = self.symbols_by_name.get(pat)
            if exact is not None:
                return [exact]
        rx = re.compile(pat, re.IGNORECASE) if regex else None
        out = []
        for s in self.symbols:
            ok = rx.search(s.name) if rx else pat.lower() in s.name.lower()
            if not ok:
                continue
            out.append(s)
            if len(out) >= max_hits:
                return out
        return out

    def find_assets(
        self, pat: str, regex: bool, max_hits: int = 100
    ) -> list[Asset]:
        if not regex:
            exact = self.assets_by_name.get(pat)
            if exact is not None:
                return [exact]
        rx = re.compile(pat, re.IGNORECASE) if regex else None
        out = []
        for a in self.assets:
            ok = rx.search(a.name) if rx else pat.lower() in a.name.lower()
            if not ok:
                continue
            out.append(a)
            if len(out) >= max_hits:
                return out
        return out


def format_symbol_short(s: Symbol) -> str:
    size = f"size=0x{s.size:X}" if s.size is not None else "size=?"
    parts = [f"0x{s.address:08X}", s.name, size]
    if s.type:
        parts.append(f"type={s.type}")
    return "  ".join(parts)


def format_asset_short(a: Asset) -> str:
    return f"0x{a.hash_value:08x}  {a.name}  [{a.namespace}]"


def format_symbol_long(s: Symbol) -> str:
    lines = [
        f"symbol:  {s.name}",
        f"address: 0x{s.address:08X}",
    ]
    if s.size is not None:
        lines.append(f"size:    0x{s.size:X} ({s.size} bytes)")
        lines.append(f"range:   0x{s.address:08X} - 0x{s.end:08X}")
    if s.type:
        lines.append(f"type:    {s.type}")
    if s.comment:
        lines.append(f"comment: {s.comment}")
    region = ram_region(s.address)
    if region:
        lines.append(f"region:  {region}")
    lines.append(f"source:  {s.source}")
    return "\n".join(lines)


def format_asset_long(a: Asset) -> str:
    return "\n".join(
        [
            f"asset:     {a.name}",
            f"hash:      0x{a.hash_value:08x}",
            f"namespace: {a.namespace}",
            f"source:    include/Game/asset_ids.h:{a.line}",
        ]
    )


def cmd_address(
    r: Resolver, addr: int, kind: str, long: bool, json_out: bool, window: int
) -> int:
    sym_exact = r.symbol_at(addr) if kind in ("all", "symbol") else None
    sym_contain = r.symbol_containing(addr) if kind in ("all", "symbol") else None
    asset_hits = r.asset_with_hash(addr) if kind in ("all", "asset") else []
    region = ram_region(addr)

    field_info = None
    for s in (sym_exact, sym_contain):
        if s is None or not s.type:
            continue
        rel = addr - s.address
        f = r.field_at(s.type, rel)
        if f is None:
            continue
        field_info = (s, f, rel)
        break

    has_match = bool(sym_exact or sym_contain or asset_hits)

    if json_out:
        before, after = r.neighbors(addr, window=window)
        payload = {
            "query": f"0x{addr:08X}",
            "region": region,
            "exact_symbol": sym_exact.to_json() if sym_exact else None,
            "containing_symbol": sym_contain.to_json() if sym_contain else None,
            "assets_with_hash": [a.to_json() for a in asset_hits],
            "neighbors_before": [s.to_json() for s in before],
            "neighbors_after": [s.to_json() for s in after],
        }
        if field_info is not None:
            sym, (fname, foff, ftype), rel = field_info
            payload["struct_field"] = {
                "symbol": sym.name,
                "struct": sym.type,
                "field": fname,
                "field_offset": foff,
                "field_offset_hex": f"0x{foff:X}",
                "field_type": ftype,
                "intra_offset": rel,
                "intra_offset_hex": f"0x{rel:X}",
            }
        print(json.dumps(payload, indent=2))
        return 0 if has_match else 1

    print(f"query:   0x{addr:08X}")
    if region:
        print(f"region:  {region}")
    print()

    if sym_exact:
        print("== exact symbol ==")
        print(format_symbol_long(sym_exact) if long else format_symbol_short(sym_exact))
        print()
    if sym_contain and sym_contain is not sym_exact:
        offset = addr - sym_contain.address
        print(f"== containing symbol (+0x{offset:X}) ==")
        print(
            format_symbol_long(sym_contain)
            if long
            else format_symbol_short(sym_contain)
        )
        print()
    if field_info is not None:
        sym, (fname, foff, ftype), rel = field_info
        intra = rel - foff
        suffix = f" + 0x{intra:X}" if intra else ""
        print("== struct field ==")
        print(
            f"  {sym.name}.{fname}{suffix}    (struct "
            f"{sym.type}, offset 0x{foff:X}, type {ftype})"
        )
        print()
    if asset_hits:
        print(f"== asset hash match ({len(asset_hits)}) ==")
        for a in asset_hits:
            print(format_asset_long(a) if long else format_asset_short(a))
        print()
    if not has_match:
        print("(no matches)")

    if long or not has_match:
        before, after = r.neighbors(addr, window=window)
        if before or after:
            print(f"== nearest symbols (window={window}) ==")
            for s in before:
                delta = addr - s.address
                print(f"  -0x{delta:<6X}  {format_symbol_short(s)}")
            for s in after:
                delta = s.address - addr
                print(f"  +0x{delta:<6X}  {format_symbol_short(s)}")
    return 0 if has_match else 1


def cmd_name(
    r: Resolver,
    query: str,
    kind: str,
    regex: bool,
    long: bool,
    json_out: bool,
    limit: int,
) -> int:
    sym_hits = []
    asset_hits = []
    if kind in ("all", "symbol"):
        sym_hits = r.find_symbols(query, regex=regex, max_hits=limit)
    if kind in ("all", "asset"):
        asset_hits = r.find_assets(query, regex=regex, max_hits=limit)
    has_match = bool(sym_hits or asset_hits)

    if json_out:
        payload = {
            "query": query,
            "regex": regex,
            "symbols": [s.to_json() for s in sym_hits],
            "assets": [a.to_json() for a in asset_hits],
        }
        print(json.dumps(payload, indent=2))
        return 0 if has_match else 1

    if sym_hits:
        if len(sym_hits) != 1 or long:
            print(f"== {len(sym_hits)} symbol match(es) ==")
        for s in sym_hits:
            print(format_symbol_long(s) if long else format_symbol_short(s))
            if long and len(sym_hits) > 1:
                print()
    if asset_hits:
        if sym_hits:
            print()
        if len(asset_hits) != 1 or long:
            print(f"== {len(asset_hits)} asset match(es) ==")
        for a in asset_hits:
            print(format_asset_long(a) if long else format_asset_short(a))
            if long and len(asset_hits) > 1:
                print()
    if not has_match:
        print(f"no matches for {query!r}", file=sys.stderr)
    return 0 if has_match else 1


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument(
        "query",
        nargs="?",
        help="Address (0xHEX / decimal), symbol name, asset name, or /regex/",
    )
    ap.add_argument(
        "--kind",
        choices=("all", "symbol", "asset"),
        default="all",
        help="Restrict lookup to one source (default: all)",
    )
    ap.add_argument("--long", "-l", action="store_true", help="verbose output")
    ap.add_argument("--json", action="store_true", help="JSON output")
    ap.add_argument(
        "--limit", type=int, default=50, help="max name-match hits (default 50)"
    )
    ap.add_argument(
        "--window", type=int, default=3, help="neighbor window for addr lookup"
    )
    ap.add_argument(
        "--addr-info",
        metavar="ADDR",
        help="address-only mode (no name fallback); implies --long",
    )
    ap.add_argument(
        "--stats",
        action="store_true",
        help="print loaded source counts and exit",
    )
    args = ap.parse_args()

    r = Resolver()

    if args.stats:
        print(f"symbols: {len(r.symbols)} ({SYMBOL_FILE.relative_to(ROOT)})")
        print(f"assets:  {len(r.assets)} ({ASSET_FILE.relative_to(ROOT)})")
        return 0

    if args.addr_info:
        addr = parse_int_arg(args.addr_info)
        if addr is None:
            print(f"invalid address: {args.addr_info!r}", file=sys.stderr)
            return 2
        return cmd_address(
            r, addr, args.kind, long=True, json_out=args.json, window=args.window
        )

    if not args.query:
        ap.print_help(sys.stderr)
        return 2

    query = args.query

    if query.startswith("/") and query.endswith("/") and len(query) >= 2:
        return cmd_name(
            r,
            query[1:-1],
            args.kind,
            regex=True,
            long=args.long,
            json_out=args.json,
            limit=args.limit,
        )

    addr = parse_int_arg(query)
    if addr is not None:
        return cmd_address(r, addr, args.kind, args.long, args.json, args.window)

    return cmd_name(
        r,
        query,
        args.kind,
        regex=False,
        long=args.long,
        json_out=args.json,
        limit=args.limit,
    )


if __name__ == "__main__":
    sys.exit(main())
