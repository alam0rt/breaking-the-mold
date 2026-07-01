#!/usr/bin/env python3
"""memreport.py -- Skullmonkeys (SLES-01090) live/offline memory analyzer.

Parses a PSX main-RAM image (0x80000000..0x80200000) into a structured JSON
report: current level, GameState flags, and the live entity tick/render lists
with each entity's position, sprite IDs, and callback/vtable symbol names.

WHY PYTHON (not a C tool reusing the game headers): the dump is full of 32-bit
PSX pointers. A host C struct cast would use 8-byte pointers and mis-place every
field; matching the PSX ABI host-side (-m32 + common.h tangle) is fragile and
buys nothing. Here we read each field by its *documented* offset (from
include/Game/*.h) with explicit width + little-endian, which is exact and has no
build step. The C headers remain the source of truth; keep the OFFSETS tables
below in sync with them.

Make a dump with PCSX-Redux's GDB stub (or any tool), then feed it in:
  gdb -nx -batch -ex 'target remote localhost:3333' \
      -ex 'dump binary memory ram.bin 0x80000000 0x80200000'

Examples:
  tools/memreport.py ram.bin                      # JSON on stdout
  tools/memreport.py ram.bin --summary -o report.json
  tools/memreport.py region.bin --base 0x8017c000 # partial dump
"""
import argparse, bisect, json, os, struct, sys

# --- globals (symbol_addrs.txt) ------------------------------------------------
G_GAME_FLAGS  = 0x800A5958   # u16
G_PGAMESTATE  = 0x800A5960   # u32 -> GameState*

RAM_BASE = 0x80000000
RAM_END  = 0x80200000

# --- struct offsets (keep in sync with include/Game/*.h) -----------------------
# GameState (include/Game/game_state.h)
GS = dict(
    tick_list_head=0x1C, render_list_head=0x20, collision_list_head=0x24,
    entity_spawn_list=0x28, player_entity=0x30, removal_mode=0x38,
    blb_header_ptr=0x40, player_hitbox_width=0x64,
    tile_collision_w=0x70, tile_collision_h=0x72, trigger_zone_count=0x78,
    entity_type_count=0x80, level_context=0x84,
    tile_render_state_count=0x104, frame_counter=0x10C,
    spawn_x=0x116, spawn_y=0x118,
    menu_active=0x150, level_active=0x170,
    boss_player_type=0x18E, boss_defeated=0x19C,
)
# LevelDataContext (include/Game/level_data_context.h), relative to gs+0x84
LDC = dict(
    current_sub_block=0x00, tile_header=0x04, vram_slot_config=0x08,
    tilemap_container=0x0C, layer_entries=0x10, tile_pixels=0x14,
    palette_indices=0x18, tile_flags=0x1C, palette_container=0x20,
    palette_anim=0x24, animated_tiles=0x28, tile_attributes=0x2C,
    anim_offsets=0x30, vehicle_data=0x34, entities=0x38, vram_rects=0x3C,
    sprites=0x40, sprites_size=0x44, audio=0x48, audio_size=0x4C,
    palette=0x50, spu_samples=0x54, spu_samples_size=0x58,
    blb_header=0x5C, current_sequence_index=0x60,
)
# Entity (include/Game/entity.h)
ENT = dict(
    tickCallback=0x04, eventCallback=0x0C, vtable=0x18, renderCallback=0x20,
    worldX=0x68, worldY=0x6A, pendingSpriteId=0xBC,
    currentSpriteId=0xCC, displayedSpriteId=0xD0,
)
# GetCurrentLevelDisplayName @0x8007AA90 byte layout (relative to blb_header+seq)
NAME_TYPE_OFF = 0xF36   # u8, must == 3 for a named level
NAME_IDX_OFF  = 0xF92   # u8, index into name records
NAME_STRIDE   = 0x70    # bytes per name record
NAME_FIELD    = 0x56    # name string offset within record (relative to blb_header)


class Mem:
    """Random-access view over a RAM image loaded at `base`."""
    def __init__(self, data, base):
        self.data, self.base, self.end = data, base, base + len(data)

    def ok(self, a, n=1):
        return self.base <= a and a + n <= self.end

    def _slice(self, a, n):
        if not self.ok(a, n):
            raise IndexError(f"0x{a:08x}+{n} outside dump [0x{self.base:08x},0x{self.end:08x})")
        o = a - self.base
        return self.data[o:o+n]

    def u8(self, a):  return self._slice(a, 1)[0]
    def u16(self, a): return struct.unpack_from("<H", self._slice(a, 2))[0]
    def s16(self, a): return struct.unpack_from("<h", self._slice(a, 2))[0]
    def u32(self, a): return struct.unpack_from("<I", self._slice(a, 4))[0]

    def cstr(self, a, maxlen=64):
        out = bytearray()
        for i in range(maxlen):
            if not self.ok(a+i, 1):
                break
            c = self.data[a - self.base + i]
            if c == 0:
                break
            out.append(c)
        return out.decode("latin-1", "replace")


class Symbols:
    """Address -> name resolver, parsed straight from the ELF .symtab."""
    def __init__(self, elf_path):
        self.syms = []
        if elf_path and os.path.exists(elf_path):
            self._parse_elf(elf_path)
        self.syms.sort()
        self.addrs = [a for a, _ in self.syms]

    def _parse_elf(self, path):
        d = open(path, "rb").read()
        if d[:4] != b"\x7fELF" or d[4] != 1:
            return  # not ELF32
        le = "<"
        e_shoff   = struct.unpack_from(le+"I", d, 0x20)[0]
        e_shentsz = struct.unpack_from(le+"H", d, 0x2E)[0]
        e_shnum   = struct.unpack_from(le+"H", d, 0x30)[0]
        for i in range(e_shnum):
            sh = e_shoff + i * e_shentsz
            sh_type = struct.unpack_from(le+"I", d, sh+0x04)[0]
            if sh_type != 2:  # SHT_SYMTAB
                continue
            sh_off  = struct.unpack_from(le+"I", d, sh+0x10)[0]
            sh_size = struct.unpack_from(le+"I", d, sh+0x14)[0]
            sh_link = struct.unpack_from(le+"I", d, sh+0x18)[0]
            sh_ent  = struct.unpack_from(le+"I", d, sh+0x24)[0] or 16
            # linked string table
            st = e_shoff + sh_link * e_shentsz
            str_off = struct.unpack_from(le+"I", d, st+0x10)[0]
            for s in range(sh_off, sh_off + sh_size, sh_ent):
                st_name = struct.unpack_from(le+"I", d, s+0)[0]
                st_val  = struct.unpack_from(le+"I", d, s+4)[0]
                if not st_val:
                    continue
                end = d.index(b"\x00", str_off + st_name)
                name = d[str_off + st_name:end].decode("latin-1", "replace")
                if not name or "NON_MATCHING" in name or name == "gcc2_compiled.":
                    continue
                self.syms.append((st_val, name))

    def name(self, a):
        if a == 0:
            return None
        i = bisect.bisect_right(self.addrs, a) - 1
        if i < 0:
            return None
        base, nm = self.syms[i]
        return nm if a == base else f"{nm}+0x{a-base:x}"


def in_ram(a):
    return RAM_BASE <= a < RAM_END


def find_sprite_file(extracted_dir, level, sprite_hex):
    """Map a sprite hash to its extracted image. Filenames are
    sprite_<decimal-hash>_anim00_f00.png (+ sprite_<dec>_anim00.gif). Prefer the
    animated gif, else the first PNG frame. Returns a repo-relative path or None."""
    if not (extracted_dir and level and sprite_hex):
        return None
    try:
        dec = int(sprite_hex, 16)
    except (TypeError, ValueError):
        return None
    import glob
    base = os.path.join(extracted_dir, level)
    # Prefer animated gif, then a sprite-sheet first frame, then a single-frame
    # static png (sprite_<dec>_anim00.png), then any png for that hash.
    for pat in (f"sprite_{dec}*.gif",
                f"sprite_{dec}_anim00_f00.png", f"sprite_{dec}_*_f00.png",
                f"sprite_{dec}_anim00.png", f"sprite_{dec}_*.png", f"sprite_{dec}.png"):
        hits = glob.glob(os.path.join(base, "**", pat), recursive=True)
        if hits:
            return sorted(hits)[0]
    return None


def read_entity(m, sym, addr, labels):
    e = {"addr": f"0x{addr:08x}",
         "pos": {"x": m.s16(addr+ENT["worldX"]), "y": m.s16(addr+ENT["worldY"])}}
    for key, off in (("tick", "tickCallback"), ("event", "eventCallback"),
                     ("render", "renderCallback"), ("vtable", "vtable")):
        v = m.u32(addr + ENT[off])
        e[key] = {"addr": f"0x{v:08x}", "name": sym.name(v)}

    # classify by vtable; for non-sprite kinds (e.g. parallax layers) the +0xCC
    # word is NOT a sprite hash, so don't present it as one.
    vt_name = e["vtable"]["name"]
    kind = labels.get("vtables", {}).get(vt_name)
    cc = m.u32(addr + ENT["currentSpriteId"])
    # kinds whose +0xCC word is not a sprite hash (layer data, code ptr, ...)
    if kind in ("parallax_layer", "non_sprite_node"):
        e["kind"] = kind
        e["sprite_id"] = None
        e["raw_0xCC"] = f"0x{cc:08x}"
    else:
        if kind:
            e["kind"] = kind
        sid = f"0x{cc:08x}"
        e["sprite_id"] = sid
        e["displayed_sprite_id"] = f"0x{m.u32(addr+ENT['displayedSpriteId']):08x}"
        lbl = labels.get("sprite_hashes", {}).get(sid)
        if lbl:
            e["sprite_label"] = lbl
    return e


def walk_list(m, sym, head, label, labels, cap=256):
    """Walk an EntityListNode chain {next@0, entity@4} -> list of entity dicts."""
    out, node, seen, i = [], head, set(), 0
    while in_ram(node) and m.ok(node, 8) and node not in seen and i < cap:
        seen.add(node)
        ent_ptr = m.u32(node + 4)
        nxt = m.u32(node)
        rec = {"index": i, "node": f"0x{node:08x}"}
        if in_ram(ent_ptr) and m.ok(ent_ptr, 0xD4):
            try:
                rec.update(read_entity(m, sym, ent_ptr, labels))
            except IndexError as ex:
                rec["error"] = str(ex)
        else:
            rec["error"] = f"entity ptr 0x{ent_ptr:08x} out of range"
        out.append(rec)
        node, i = nxt, i + 1
    return out


def level_name(m, gs):
    """Replicate GetCurrentLevelDisplayName @0x8007AA90."""
    ctx = gs + GS["level_context"]
    blbh = m.u32(ctx + LDC["blb_header"])
    seq = m.u8(ctx + LDC["current_sequence_index"])
    info = {"blb_header": f"0x{blbh:08x}", "seq_index": seq, "name": None, "name_index": None}
    if not in_ram(blbh):
        return info
    base = blbh + seq
    if not m.ok(base + NAME_TYPE_OFF, 1) or m.u8(base + NAME_TYPE_OFF) != 3:
        return info
    nidx = m.u8(base + NAME_IDX_OFF)
    info["name_index"] = nidx
    info["name"] = m.cstr(blbh + nidx * NAME_STRIDE + NAME_FIELD, 32)
    return info


def name_table(m, gs, n=24):
    """Dump the level-code name table (idx -> 4-char code)."""
    ctx = gs + GS["level_context"]
    blbh = m.u32(ctx + LDC["blb_header"])
    if not in_ram(blbh):
        return []
    out = []
    for i in range(n):
        a = blbh + i * NAME_STRIDE + NAME_FIELD
        if not m.ok(a, 1):
            break
        s = m.cstr(a, 8)
        if not s or not all(32 <= ord(c) < 127 for c in s):
            break
        out.append({"index": i, "code": s})
    return out


def build_report(m, sym, source, lists, labels, extracted_dir=None):
    if not m.ok(G_PGAMESTATE, 4):
        return {"meta": {"source": source, "ram_base": f"0x{m.base:08x}", "ram_size": len(m.data)},
                "error": f"g_pGameState (0x{G_PGAMESTATE:08x}) not in dump; "
                         f"need a full RAM image (0x80000000..0x80200000) or a wider --base region"}
    gs = m.u32(G_PGAMESTATE)
    rep = {
        "meta": {"source": source, "ram_base": f"0x{m.base:08x}",
                 "ram_size": len(m.data),
                 "g_pGameState": f"0x{gs:08x}",
                 "g_GameFlags": f"0x{m.u16(G_GAME_FLAGS):04x}" if m.ok(G_GAME_FLAGS, 2) else None},
    }
    if not in_ram(gs) or not m.ok(gs, 0x1A0):
        rep["error"] = f"GameState pointer 0x{gs:08x} not in dump (level not loaded?)"
        return rep

    def g(field, w="u32"):
        return getattr(m, w)(gs + GS[field])

    rep["game"] = {
        "frame_counter": g("frame_counter"),
        "level_active": g("level_active", "u8"),
        "menu_active": g("menu_active", "u8"),
        "boss_defeated": g("boss_defeated", "u8"),
        "boss_player_type": g("boss_player_type", "u8"),
        "removal_mode": g("removal_mode"),
        "player_spawn": {"x": g("spawn_x", "s16"), "y": g("spawn_y", "s16")},
        "player_hitbox_width": g("player_hitbox_width", "s16"),
        "geometry": {
            "tile_collision_w": g("tile_collision_w", "s16"),
            "tile_collision_h": g("tile_collision_h", "s16"),
            "tile_render_count": g("tile_render_state_count", "u16"),
            "trigger_zone_count": g("trigger_zone_count", "u16"),
            "entity_type_count": g("entity_type_count", "u16"),
        },
        "level": level_name(m, gs),
        "level_name_table": name_table(m, gs),
    }
    # loaded assets: nonzero pointers from the LevelDataContext
    ctx = gs + GS["level_context"]
    assets = {}
    for nm, off in LDC.items():
        if nm in ("blb_header", "current_sequence_index", "current_sub_block"):
            continue
        v = m.u32(ctx + off)
        if v:
            assets[nm] = f"0x{v:08x}"
    rep["game"]["loaded_assets"] = assets

    # player + requested lists
    player_ptr = g("player_entity")
    if in_ram(player_ptr) and m.ok(player_ptr, 0xD4):
        rep["player"] = read_entity(m, sym, player_ptr, labels)
        rep["player"]["addr"] = f"0x{player_ptr:08x}"

    rep["lists"] = {}
    head_field = {"tick": "tick_list_head", "render": "render_list_head",
                  "collision": "collision_list_head"}
    for lst in lists:
        head = g(head_field[lst])
        ents = walk_list(m, sym, head, lst, labels)
        rep["lists"][lst] = {"head": f"0x{head:08x}", "count": len(ents), "entities": ents}

    # surface work items: entities the registry can't yet name
    unknown_sprites, unknown_kinds = {}, {}
    for info in rep["lists"].values():
        for e in info["entities"]:
            if e.get("kind") == "parallax_layer" or "error" in e:
                continue
            sid = e.get("sprite_id")
            if sid and not e.get("sprite_label"):
                unknown_sprites.setdefault(sid, []).append(e["addr"])
            # only flag genuinely-unidentified vtables (auto-named D_xxxx / unresolved)
            vt = e["vtable"]["name"]
            if (vt is None or vt.startswith("D_")) and not e.get("kind"):
                unknown_kinds.setdefault(vt or e["vtable"]["addr"], []).append(e["addr"])
    rep["unknowns"] = {"sprite_hashes": unknown_sprites, "vtables": unknown_kinds}

    # link each entity's sprite hash to its extracted image (for visual ID)
    level = rep["game"]["level"].get("name")
    if extracted_dir and level:
        ents = [rep["player"]] if rep.get("player") else []
        for info in rep["lists"].values():
            ents += info["entities"]
        for e in ents:
            f = find_sprite_file(extracted_dir, level, e.get("sprite_id"))
            if f:
                e["sprite_file"] = f
    return rep


def summarize(rep):
    out = []
    g = rep.get("game")
    if not g:
        return rep.get("error", "no GameState")
    lv = g["level"]
    out.append(f"LEVEL {lv.get('name')!r} (idx {lv.get('name_index')})  frame={g['frame_counter']}"
               f"  active={g['level_active']} menu={g['menu_active']} boss_defeated={g['boss_defeated']}")
    geo = g["geometry"]
    out.append(f"  map {geo['tile_collision_w']}x{geo['tile_collision_h']} tiles, "
               f"{geo['tile_render_count']} render entries, {geo['trigger_zone_count']} trigger zones")
    def ent_line(e):
        if "error" in e:
            return f"    [{e['index']:2}] {e.get('node')} -- {e['error']}"
        lbl = e.get("sprite_label") or e.get("kind") or "?"
        sid = e.get("sprite_id") or e.get("raw_0xCC", "-")
        img = f"  img={e['sprite_file']}" if e.get("sprite_file") else ""
        return (f"    [{e['index']:2}] {e['addr']} ({e['pos']['x']:>6},{e['pos']['y']:>6}) "
                f"sprite={sid} {e['tick']['name']}  <{lbl}>{img}")

    p = rep.get("player")
    if p:
        out.append(f"  PLAYER @{p['addr']} pos=({p['pos']['x']},{p['pos']['y']}) "
                   f"sprite={p.get('sprite_id')} state={p['tick']['name']}"
                   + (f"  <{p['sprite_label']}>" if p.get("sprite_label") else ""))
    for lst, info in rep.get("lists", {}).items():
        out.append(f"  {lst} list: {info['count']} entities")
        for e in info["entities"]:
            out.append(ent_line(e))
    u = rep.get("unknowns", {})
    if u.get("sprite_hashes") or u.get("vtables"):
        out.append("  UNKNOWNS to identify:")
        for sid, addrs in u.get("sprite_hashes", {}).items():
            out.append(f"    sprite {sid} @ {', '.join(addrs)}")
        for vt, addrs in u.get("vtables", {}).items():
            out.append(f"    vtable {vt} @ {', '.join(addrs)}")
    return "\n".join(out)


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    root = os.path.dirname(here)
    default_elf = os.path.join(root, "build", "SLES_010.90.elf")
    default_labels = os.path.join(root, "docs", "reference", "entity_recon.json")
    ap = argparse.ArgumentParser(description="Skullmonkeys memory dump -> JSON report")
    ap.add_argument("dump", metavar="DUMP.bin", help="PSX RAM dump file to parse")
    ap.add_argument("--base", type=lambda x: int(x, 0), default=RAM_BASE,
                    help="load address of the dump (default 0x80000000)")
    ap.add_argument("--elf", default=default_elf, help="ELF for symbol resolution")
    ap.add_argument("--labels", default=default_labels,
                    help="entity_recon registry JSON (sprite_hashes/vtables); '' to disable")
    ap.add_argument("--lists", default="tick",
                    help="comma list of: tick,render,collision (default tick)")
    ap.add_argument("--extracted-dir", default=os.path.join(root, "extracted"),
                    help="extracted assets root; links sprite hashes to images ('' to disable)")
    ap.add_argument("-o", "--out", help="write JSON here (default stdout)")
    ap.add_argument("--summary", action="store_true", help="also print a summary to stderr")
    args = ap.parse_args()

    labels = {}
    if args.labels and os.path.exists(args.labels):
        labels = json.load(open(args.labels))

    data, base, source = open(args.dump, "rb").read(), args.base, f"file:{args.dump}"
    m = Mem(data, base)
    sym = Symbols(args.elf)
    lists = [s.strip() for s in args.lists.split(",") if s.strip()]
    rep = build_report(m, sym, source, lists, labels, args.extracted_dir or None)

    text = json.dumps(rep, indent=2)
    if args.out:
        open(args.out, "w").write(text)
        sys.stderr.write(f"wrote {args.out}\n")
    else:
        print(text)
    if args.summary:
        sys.stderr.write(summarize(rep) + "\n")


if __name__ == "__main__":
    main()
