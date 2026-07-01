#!/usr/bin/env python3
"""
Skullmonkeys Entity Viewer — a small local web app for browsing the entities
and sprite animations of any stage, so we can confirm behaviour and align on
naming.

It is deliberately thin: all the heavy lifting (BLB parsing, RLE sprite decode)
is reused from tools/extract_blb, and it reads the already-extracted tree under
`extracted/` rather than re-parsing the 76 MB GAME.BLB. No third-party server
framework — just the stdlib http.server — and Pillow (already in the venv) for
PNG encoding of decoded frames.

Run:
    python3 tools/entity_viewer/server.py            # serves http://localhost:8077
    python3 tools/entity_viewer/server.py --port 9000 --root extracted

What it serves:
    GET  /                      the single-page UI (app.html)
    GET  /api/worlds            worlds -> stages (+ entity / sprite counts)
    GET  /api/stage?world&stage entities[] + sprite metadata (anims, frames)
    GET  /api/frame.png?...     one decoded sprite frame, on demand (RGBA PNG)
    GET  /api/annotations       the alignment notes (names, anim tags, links)
    POST /api/annotations       overwrite the alignment notes
    POST /api/symbols/export    write symbol renames to symbol_addrs_new.txt

Annotations (sprite names, animation state tags like idle/walk/hit/death,
entity<->sprite links, free-text notes) persist to annotations.json next to
this script. That file is the shared substrate for "align on naming".
"""

import argparse
import io
import json
import struct
import sys
import threading
from functools import lru_cache
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlparse, parse_qs

HERE = Path(__file__).resolve().parent
TOOLS = HERE.parent
REPO = TOOLS.parent

# Reuse the existing, verified sprite decoder rather than re-implementing it.
sys.path.insert(0, str(TOOLS))
from extract_blb.handlers.sprites import (  # noqa: E402
    parse_sprite_header,
    parse_animation_entry,
    parse_frame_metadata,
    parse_palette,
    decode_rle_sprite,
)

# Map the 4-char world codes to readable names (from docs/blb/README.md). The
# viewer falls back to the bare code for anything not listed.
WORLD_NAMES = {
    "MENU": "Options Menu", "PHRO": "Phratghhh", "SCIE": "Science Centre",
    "TMPL": "Monkey Shrines", "FINN": "Finn (swim)", "MEGA": "Monkey Mage (boss)",
    "BOIL": "Boil", "SNOW": "Snow", "FOOD": "Food", "HEAD": "Joe-Head-Joe (boss)",
    "BRG1": "Bridge", "GLID": "Glide", "CAVE": "Cave", "WEED": "Weed",
    "EGGS": "Eggs", "GLEN": "Glenn Yntis (boss)", "CLOU": "Cloud", "SOAR": "Soar",
    "CRYS": "Crystal", "CSTL": "Castle", "WIZZ": "Wizard (boss)", "RUNN": "Runner",
    "MOSS": "Moss", "KLOG": "Klogg (final boss)", "EVIL": "Evil Engine",
    "SEVN": "1970's (bonus)",
}

ANNOTATIONS_PATH = HERE / "annotations.json"
_anno_lock = threading.Lock()

DOCS = REPO / "docs" / "reference"


def _hx(n: int) -> str:
    return f"0x{n & 0xFFFFFFFF:08x}"


# internal entity_type -> sprite hashes, built by build_sprite_map.py from the
# decompiled C (EntityTypeNNN_Init -> inner Init* -> SetEntitySpriteId / g_SpriteList
# array, arrays read from the ROM) and validated against the BLB hash set. These keys
# are *internal* (post-remap) types; for the handful of layer-remapped BLB types the
# raw lookup can differ (see RemapEntityTypesForLevel) — confirm those by hand.
def load_entity_sprite_map() -> dict:
    p = HERE / "entity_sprite_map.json"
    if p.exists():
        try:
            return {int(k): [int(h, 16) for h in v["hashes"]]
                    for k, v in json.loads(p.read_text()).items()}
        except Exception:
            pass
    return {}


ENTITY_SPRITE_MAP = load_entity_sprite_map()


def load_sprite_names() -> dict:
    """hash_hex -> human label, merged from the image-confirmed recon registry
    (docs/reference/entity_recon.json) and the broader sprite-ids-complete table.
    Recon wins on conflicts (it is hand-verified against extracted images)."""
    import re
    names = {}
    recon = DOCS / "entity_recon.json"
    if recon.exists():
        try:
            j = json.loads(recon.read_text())
            for h, desc in (j.get("sprite_hashes") or {}).items():
                names[_hx(int(h, 16))] = desc
        except Exception:
            pass
    complete = DOCS / "sprite-ids-complete.md"
    if complete.exists():
        row = re.compile(r"\|\s*(0x[0-9a-fA-F]+)\s*\|[^|]*\|\s*([^|]+?)\s*\|\s*([^|]+?)\s*\|")
        for line in complete.read_text().splitlines():
            m = row.match(line)
            if m:
                h = _hx(int(m.group(1), 16))
                names.setdefault(h, m.group(3).strip().strip("*"))
    return names


SPRITE_NAMES = load_sprite_names()


def load_entity_callbacks() -> dict:
    """internal entity_type -> {init_fn, init_addr, alias, category}, extracted
    from docs/reference/entity-types.md (the 121-entry g_EntityTypeCallbackTable
    @ 0x8009d5f8). The init function is the per-type dispatch entry that installs
    the entity's tick/event/state callbacks and sprite."""
    p = HERE / "entity_callbacks.json"
    if p.exists():
        try:
            return {int(k): v for k, v in json.loads(p.read_text()).items()}
        except Exception:
            pass
    return {}


ENTITY_CALLBACKS = load_entity_callbacks()


def load_symbols() -> dict:
    """ROM address (int) -> symbol name, from symbol_addrs.txt. The callback
    table's init_fn labels are clean-room placeholders (EntityCallback_TypeNN);
    the *real* editable symbol at each init_addr lives here (ground truth)."""
    out = {}
    p = REPO / "symbol_addrs.txt"
    if p.exists():
        for line in p.read_text().splitlines():
            m = re.match(r"(\w+)\s*=\s*(0x[0-9A-Fa-f]+)", line)
            if m:
                out.setdefault(int(m.group(2), 16), m.group(1))
    return out


SYMBOLS = load_symbols()
SYMBOLS_NEW = REPO / "symbol_addrs_new.txt"   # merge target (see CLAUDE.md)


def export_symbols(edits: dict) -> dict:
    """Write user symbol renames to symbol_addrs_new.txt in `NAME = 0xADDR;`
    format for manual merge into symbol_addrs.txt. `edits` maps an address
    string ("0x8007f050") to the new name. Only rows that differ from the
    current symbol are written."""
    rows = {}
    for addr_s, name in edits.items():
        try:
            addr = int(addr_s, 16)
        except (TypeError, ValueError):
            continue
        name = (name or "").strip()
        if not re.match(r"^[A-Za-z_]\w*$", name):
            continue
        if SYMBOLS.get(addr) == name:
            continue
        rows[addr] = name
    lines = ["// Entity-viewer symbol renames — merge into symbol_addrs.txt",
             f"// {len(rows)} symbol(s)"]
    for addr in sorted(rows):
        lines.append(f"{rows[addr]} = 0x{addr:08X};")
    SYMBOLS_NEW.write_text("\n".join(lines) + "\n")
    return {"ok": True, "count": len(rows), "path": str(SYMBOLS_NEW.relative_to(REPO))}


def load_remap() -> dict:
    """(layer,BLB_type) -> internal_type, decoded by build_remap.py from
    RemapEntityTypesForLevel. Asset-501 stores the BLB type; the callback table
    and sprite map are indexed by the post-remap internal type."""
    p = HERE / "entity_remap.json"
    if p.exists():
        try:
            return json.loads(p.read_text())
        except Exception:
            pass
    return {}


REMAP = load_remap()


def internal_type(blb_type, layer) -> int:
    return REMAP.get(f"{(layer or 0) & 0xFF},{blb_type}", blb_type)


# --------------------------------------------------------------------------- #
# Extracted-tree model
# --------------------------------------------------------------------------- #
class Library:
    """Indexes the extracted/ tree: which worlds/stages exist and where each
    stage's entities and sprite containers live on disk."""

    def __init__(self, root: Path):
        self.root = root
        self.worlds = {}  # code -> {"name", "stages": {n: stage_dict}, "shared_bin": path|None}
        self._scan()

    def _scan(self):
        for world_dir in sorted(p for p in self.root.iterdir() if p.is_dir()):
            code = world_dir.name
            if code == "header":
                continue
            shared_bin = self._first(world_dir / "shared", "600_*.bin")
            stages = {}
            for stage_dir in sorted(world_dir.glob("stage*")):
                if not stage_dir.is_dir():
                    continue
                try:
                    n = int(stage_dir.name[len("stage"):])
                except ValueError:
                    continue
                mapdir = stage_dir / "map"
                stages[n] = {
                    "entities_json": (mapdir / "entities" / "entities.json"),
                    "sprite_bin": self._first(mapdir, "600_*.bin"),
                }
            if stages or shared_bin:
                self.worlds[code] = {
                    "name": WORLD_NAMES.get(code, code),
                    "stages": stages,
                    "shared_bin": shared_bin,
                }

    @staticmethod
    def _first(d: Path, pattern: str):
        if not d.is_dir():
            return None
        hits = sorted(d.glob(pattern))
        return hits[0] if hits else None

    def sprite_bin(self, world: str, stage: int, src: str):
        """Resolve the raw 600 container for a stage's own sprites (src='map')
        or the world-shared sprites (src='shared')."""
        w = self.worlds.get(world)
        if not w:
            return None
        if src == "shared":
            return w["shared_bin"]
        st = w["stages"].get(stage)
        return st["sprite_bin"] if st else None

    def entities(self, world: str, stage: int):
        w = self.worlds.get(world)
        if not w:
            return []
        st = w["stages"].get(stage)
        if not st or not st["entities_json"] or not st["entities_json"].exists():
            return []
        try:
            return json.loads(st["entities_json"].read_text())
        except Exception:
            return []

    def index(self):
        out = []
        for code, w in self.worlds.items():
            stages = []
            for n, st in sorted(w["stages"].items()):
                ents = self.entities(code, n)
                stages.append({
                    "stage": n,
                    "entity_count": len(ents),
                    "has_sprites": bool(st["sprite_bin"]),
                })
            out.append({
                "code": code,
                "name": w["name"],
                "stages": stages,
                "has_shared_sprites": bool(w["shared_bin"]),
            })
        return out


# --------------------------------------------------------------------------- #
# Sprite container decode (reuses extract_blb decoders)
# --------------------------------------------------------------------------- #
@lru_cache(maxsize=64)
def _load_bin(path_str: str) -> bytes:
    return Path(path_str).read_bytes()


def container_sprites(data: bytes) -> dict:
    """Walk a 600 container TOC -> {sprite_id: (size, offset)}."""
    if len(data) < 4:
        return {}
    count = struct.unpack_from("<I", data, 0)[0]
    if count > 4096 or 4 + count * 12 > len(data):
        return {}
    out = {}
    for i in range(count):
        sid, size, off = struct.unpack_from("<III", data, 4 + i * 12)
        out[sid] = (size, off)
    return out


def sprite_metadata(data: bytes, sprite_off: int) -> dict:
    """Parse one sprite's header + every animation + per-frame metadata
    (no pixel decode — that happens lazily per frame)."""
    header = parse_sprite_header(data, sprite_off)
    anims = []
    fmeta_base = sprite_off + header["frame_meta_offset"]
    for ai in range(min(header["animation_count"], 64)):
        a = parse_animation_entry(data, sprite_off + 12 + ai * 12)
        if a["frame_count"] == 0 or a["frame_count"] > 256:
            anims.append({"anim": ai, "animation_id": a["animation_id"],
                          "flags": a["flags"], "frames": []})
            continue
        frames = []
        for fi in range(a["frame_count"]):
            abs_idx = a["frame_offset"] + fi
            foff = fmeta_base + abs_idx * 36
            if foff + 36 > len(data):
                break
            fm = parse_frame_metadata(data, foff)
            frames.append({
                "frame": fi,
                "w": fm["render_width"], "h": fm["render_height"],
                "render_x": fm["render_x"], "render_y": fm["render_y"],
                "delay": fm["frame_delay"],          # 0x04 ticks (real timing)
                "dx": fm["frame_delta_x"],           # 0x0E signed motion -> vx
                "dy": fm["frame_delta_y"],           # 0x10 signed motion -> vy
                "callback_id": fm["callback_id"],
                "hitbox": {"x": fm["hitbox_x"], "y": fm["hitbox_y"],
                           "w": fm["hitbox_width"], "h": fm["hitbox_height"]},
            })
        anims.append({
            "anim": ai,
            "animation_id": a["animation_id"],
            "flags": a["flags"],
            "has_callback": bool(a["flags"] & 1),
            "frames": frames,
        })
    return {"animation_count": header["animation_count"], "animations": anims}


def stage_sprites(bin_path: Path) -> list:
    data = _load_bin(str(bin_path))
    toc = container_sprites(data)
    out = []
    for sid, (size, off) in toc.items():
        if off + 12 > len(data):
            continue
        try:
            meta = sprite_metadata(data, off)
        except Exception:
            continue
        out.append({
            "id": sid,
            "id_hex": f"0x{sid:08x}",
            "name": SPRITE_NAMES.get(f"0x{sid:08x}", ""),
            "animation_count": meta["animation_count"],
            "animations": meta["animations"],
        })
    return out


def decode_frame_png(bin_path: Path, sprite_id: int, anim_idx: int, frame_idx: int) -> bytes:
    """Decode a single frame to a PNG byte string (transparent background)."""
    from PIL import Image
    data = _load_bin(str(bin_path))
    toc = container_sprites(data)
    if sprite_id not in toc:
        raise KeyError(sprite_id)
    _, off = toc[sprite_id]
    header = parse_sprite_header(data, off)
    anim = parse_animation_entry(data, off + 12 + anim_idx * 12)
    abs_idx = anim["frame_offset"] + frame_idx
    fm = parse_frame_metadata(data, off + header["frame_meta_offset"] + abs_idx * 36)
    w, h = fm["render_width"], fm["render_height"]
    if w <= 0 or h <= 0 or w > 1024 or h > 1024:
        # 1x1 transparent placeholder
        img = Image.new("RGBA", (1, 1), (0, 0, 0, 0))
    else:
        palette = parse_palette(data, off + header["palette_offset"])
        rle_abs = off + header["rle_offset"] + fm["rle_offset"]
        pixels = decode_rle_sprite(data, rle_abs, w, h, palette)
        img = Image.frombytes("RGBA", (w, h), pixels)
    buf = io.BytesIO()
    img.save(buf, "PNG")
    return buf.getvalue()


# --------------------------------------------------------------------------- #
# Annotations (shared "align on naming" store)
# --------------------------------------------------------------------------- #
def load_annotations() -> dict:
    if ANNOTATIONS_PATH.exists():
        try:
            return json.loads(ANNOTATIONS_PATH.read_text())
        except Exception:
            pass
    return {"sprites": {}, "entities": {}, "links": {}, "version": 1}


def save_annotations(obj: dict):
    with _anno_lock:
        ANNOTATIONS_PATH.write_text(json.dumps(obj, indent=2, sort_keys=True))


# --------------------------------------------------------------------------- #
# HTTP
# --------------------------------------------------------------------------- #
class Handler(BaseHTTPRequestHandler):
    library: Library = None  # set on the class before serving

    def log_message(self, *_):  # quiet
        pass

    # -- helpers ----------------------------------------------------------
    def _send(self, code, body, ctype="application/json"):
        if isinstance(body, (dict, list)):
            body = json.dumps(body).encode()
        elif isinstance(body, str):
            body = body.encode()
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        if self.command != "HEAD":
            self.wfile.write(body)

    def _qs(self):
        return {k: v[0] for k, v in parse_qs(urlparse(self.path).query).items()}

    # -- routing ----------------------------------------------------------
    def do_GET(self):
        path = urlparse(self.path).path
        try:
            if path in ("/", "/index.html", "/app.html"):
                return self._send(200, (HERE / "app.html").read_text(), "text/html; charset=utf-8")
            if path == "/api/worlds":
                return self._send(200, self.library.index())
            if path == "/api/stage":
                return self._api_stage()
            if path == "/api/frame.png":
                return self._api_frame()
            if path == "/api/annotations":
                return self._send(200, load_annotations())
            return self._send(404, {"error": "not found", "path": path})
        except Exception as e:  # never crash the loop
            return self._send(500, {"error": str(e)})

    def do_POST(self):
        path = urlparse(self.path).path
        try:
            if path == "/api/annotations":
                n = int(self.headers.get("Content-Length", 0))
                obj = json.loads(self.rfile.read(n) or b"{}")
                save_annotations(obj)
                return self._send(200, {"ok": True})
            if path == "/api/symbols/export":
                n = int(self.headers.get("Content-Length", 0))
                obj = json.loads(self.rfile.read(n) or b"{}")
                return self._send(200, export_symbols(obj))
            return self._send(404, {"error": "not found"})
        except Exception as e:
            return self._send(500, {"error": str(e)})

    # -- endpoints --------------------------------------------------------
    def _api_stage(self):
        q = self._qs()
        world = q.get("world", "")
        stage = int(q.get("stage", "0"))
        entities = self.library.entities(world, stage)
        sprites = {}
        for src in ("map", "shared"):
            p = self.library.sprite_bin(world, stage, src)
            if p:
                sprites[src] = stage_sprites(p)
        # Annotate each entity with its known sprite candidates and whether they
        # are actually present in this stage's containers (best-known mapping;
        # confirm/correct by hand in the viewer).
        present = {}
        for src in ("map", "shared"):
            for sp in sprites.get(src, []):
                present.setdefault(sp["id_hex"], src)
        for e in entities:
            it = internal_type(e.get("entity_type"), e.get("layer", 0))
            e["internal_type"] = it
            cands = []
            for h in ENTITY_SPRITE_MAP.get(it, []):
                hx = _hx(h)
                cands.append({"hash": hx, "present": hx in present, "src": present.get(hx)})
            e["mapped"] = cands
            cb = ENTITY_CALLBACKS.get(it)
            if cb:
                cb = dict(cb)
                # the editable, address-backed symbol (falls back to the label)
                cb["symbol"] = SYMBOLS.get(int(cb.get("init_addr", "0x0"), 16),
                                           cb.get("init_fn"))
            e["cb"] = cb
        return self._send(200, {
            "world": world, "stage": stage,
            "name": self.library.worlds.get(world, {}).get("name", world),
            "entities": entities,
            "sprites": sprites,
        })

    def _api_frame(self):
        q = self._qs()
        bin_path = self.library.sprite_bin(q["world"], int(q["stage"]), q.get("src", "map"))
        if not bin_path:
            return self._send(404, {"error": "no sprite container"})
        png = decode_frame_png(bin_path, int(q["sprite"]), int(q.get("anim", 0)), int(q.get("frame", 0)))
        self.send_response(200)
        self.send_header("Content-Type", "image/png")
        self.send_header("Content-Length", str(len(png)))
        self.send_header("Cache-Control", "max-age=3600")
        self.end_headers()
        self.wfile.write(png)


def main():
    ap = argparse.ArgumentParser(description="Skullmonkeys entity / sprite viewer")
    ap.add_argument("--port", type=int, default=8077)
    ap.add_argument("--root", type=Path, default=REPO / "extracted",
                    help="extracted/ tree (default: <repo>/extracted)")
    args = ap.parse_args()

    if not args.root.is_dir():
        sys.exit(f"extracted tree not found: {args.root}\n"
                 f"Run the BLB extractor first (see docs/blb/README.md).")

    Handler.library = Library(args.root)
    n_worlds = len(Handler.library.worlds)
    n_stages = sum(len(w["stages"]) for w in Handler.library.worlds.values())
    srv = ThreadingHTTPServer(("127.0.0.1", args.port), Handler)
    print(f"Entity Viewer: indexed {n_worlds} worlds / {n_stages} stages from {args.root}")
    print(f"  -> http://localhost:{args.port}   (Ctrl-C to stop)")
    try:
        srv.serve_forever()
    except KeyboardInterrupt:
        print("\nbye")


if __name__ == "__main__":
    main()
