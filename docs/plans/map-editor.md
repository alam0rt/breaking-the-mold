---
title: "Plan: a simple Skullmonkeys map editor"
category: plans
tags: [plans, tooling, map-editor, blb, tiles, entities]
---

# Plan: a simple Skullmonkeys map editor

Status: proposed (2026-07-01)
Scope: a local tool to **view and edit a stage's tilemap layers and entity
placements**, then **repack** the result into `GAME.BLB` so it can be booted in
PCSX-Redux. Builds directly on the existing `tools/entity_viewer/` and the
`tools/extract_blb/` decoders.

## Why this is tractable

We already have every piece except the write path:

- **Format is documented and verified.** Tile system, tilemap entry bits, and the
  92-byte layer entry are in `docs/systems/tiles-and-tilemaps.md`; TOC/sub-TOC in
  `docs/blb/toc-format.md`; asset pointers in
  `docs/reference/level-data-context.md`. `docs/blb.hexpat` is the source of truth.
- **Decoders exist.** `tools/extract_blb/handlers/` already parses layers (200/201),
  tile pixels/flags/palettes (300/301/302/400), entities (501), and sprites (600).
  `layers.py` can even render a layer to PNG when tile+palette context is present.
- **A viewer UI exists.** `tools/entity_viewer/` (Python stdlib server + single-file
  `app.html`) already browses stages, entity placements, and sprite animations, and
  persists annotations to JSON. The editor is the same shape plus a canvas and a save.
- **A write path into the ISO exists.** The PCSX-Redux MCP bridge exposes
  `patch_disc_file` (see CLAUDE.md → Runtime Verification), so a repacked `GAME.BLB`
  can be pushed to the running emulator without a full rebuild.
- **A live write path into RAM exists.** PCSX-Redux's Lua `PCSX.getMemPtr()` returns
  a writable pointer over the full 8MB of emulated RAM, and the `DrawImguiFrame` hook
  runs once per emulated vsync — the same mechanism cheats use to re-apply writes
  every frame. The MCP bridge already wraps this (`write_ram`, `read_ram`,
  `read_vram`). This gives us an **instant edit-preview loop that does not require
  repacking at all** (see "Two-speed editing" below).

The only genuinely new engineering is **repacking** (assets → segment TOC → BLB)
and a **tile/entity edit surface**. Everything else is reuse.

## Two-speed editing (live preview vs. persist)

The `getMemPtr` question the design turns on: we have two very different write paths,
and the editor should use both for different purposes.

- **Live preview (inner loop, instant).** Once a stage is loaded, its assets sit in
  RAM at the `LevelDataContext` pointers (`docs/reference/level-data-context.md`):
  tilemap (200) at `ctx[3]`, layer entries (201) at `ctx[4]`, entities (501) at
  `ctx[14]`, tile header (100) at `ctx[1]`. The editor can read those pointers over
  the MCP bridge and **overwrite the tilemap/entity bytes in place** to see an edit
  immediately — no repack, no reboot.
- **Persist (outer loop, for keeps).** Repack `GAME.BLB` so the edit survives a
  reboot and can be shipped/committed. This is the slow, authoritative path.

### The load-bearing unknown: does the renderer re-read the tilemap each frame?

`RenderTilemapSprites16x16` @ 0x8001713c reads `tilemap[y*width+x]` per frame, which
*suggests* a live poke to the asset-200 buffer shows up instantly. But
`InitTilemapLayer16x16` @ 0x80017540 and `InitLayersAndTileState` @ 0x80024778 build
per-layer render contexts/SPRT primitives once at load — so it's possible the
tilemap is **baked** at init and a mid-frame poke does nothing until re-init.

**Resolve this empirically first** (a `game_watcher.lua` poke + observe), because it
picks the live-preview strategy:

1. If re-read per frame → poke asset-200 RAM directly, done.
2. If baked at init → after poking RAM, re-trigger the layer init for that layer
   (call `InitTilemapLayer16x16`), **or** write the changed tile's SPRT primitives
   in the render list directly, **or** poke tile graphics straight into VRAM
   (`read_vram`/VRAM write) for a pure-graphics preview.

Entities (501) are parsed into live entity structs at spawn, so moving an *existing*
placement live means writing the entity's runtime position, not the 501 record;
the 501 edit is what persists. Treat live-entity-preview as a stretch goal.

## What "map" means here

A stage's editable content is three things:

1. **Tilemap layers** — asset 200 holds one `u16[width*height]` grid per layer;
   each cell is `tile_index (bits 0-11) | tint (bits 12-15)`. Asset 201 holds the
   92-byte per-layer metadata (position, size, parallax scroll, priority, tints).
2. **Entity placements** — asset 501, 24-byte records (type, name-hash, tile/pixel
   position, variant, layer, bbox) already parsed by `handlers/entities.py`.
3. **Tile header** — asset 100 (level width/height in tiles, spawn X/Y, background
   RGB, tile counts). Mostly read-only for the MVP but spawn point is worth editing.

The **tile graphics themselves** (asset 300 pixels + 400 palettes) are read-only in
the MVP — we place existing tiles, we don't paint new ones.

## MVP (the smallest useful thing)

A single stage, round-trip editable, bootable in the emulator.

### In scope

1. **Load & render a stage.** Reuse the extractor to decode a stage from
   `extracted/`. Render each layer as a tile grid onto an HTML canvas using the
   decoded tile atlas + palettes (the PNG path in `layers.py` is the reference).
   Layer visibility toggles; show the active layer on top.
2. **Tile painting.** A tile palette panel (all tiles for the stage as a picker).
   Click/drag on the canvas to set the active layer's cell to the selected tile
   index; right-click (or an eraser) sets 0 (transparent). Undo/redo stack.
3. **Entity move/edit.** Show 501 placements as draggable markers on the canvas
   (reuse entity_viewer's type/name lookup for labels). Drag to reposition; edit
   type/variant/layer in a side panel. Add/delete a placement.
4. **Spawn point.** Drag a distinct marker for asset-100 spawn X/Y.
5. **Save = repack.** Re-serialize the edited assets, rebuild the affected segment
   TOC(s), and write a new `GAME.BLB` (to a copy, never in place). This is the new
   code; see "Repack" below.
6. **Preview it.** Live: push the edited bytes to the running emulator via the MCP
   bridge (`write_ram` at the `LevelDataContext` pointer) for instant feedback.
   Persist: repack `GAME.BLB` and boot it — `patch_disc_file` via the bridge, or
   swap the file in `disks/blb/` and `make record LEVEL=… STAGE=…`.

### Explicitly out of scope for MVP

- Editing tile *graphics* or palettes (place-only).
- Resizing the level or adding/removing layers.
- Animated tiles (303/503/401), vehicle paths (504), audio (601/602), demo replay
  (700) — preserved byte-for-byte on repack, never touched.
- Live in-emulator editing / hot reload (nice follow-up, not MVP).
- Multi-stage project management, collaborative annotations.

### Success criterion

Load stage → move one entity and change one tile → save → boot in PCSX-Redux →
observe the change in-game, with every untouched asset byte-identical to the
original (verify by re-extracting the repacked BLB and diffing non-edited assets).

## Repack (the one new subsystem)

This is the load-bearing risk, so design it deliberately.

- **Round-trip identity first.** Before any editing, prove `extract → repack`
  produces a byte-identical `GAME.BLB` (or a documented, understood diff). This is
  the single most important milestone; if we can't reproduce the container, edits
  aren't trustworthy. Add a `tools/pack_blb/` that is the inverse of
  `tools/extract_blb/`, driven by the same handler registry.
- **Preserve everything unedited.** Repack from the original raw asset bytes for
  every asset we didn't touch; only re-serialize the 2-3 assets the editor changed
  (100, 200, 501). This minimizes surface area and makes diffs auditable.
- **TOC math.** Recompute each segment TOC (`entry_count`, per-entry `type/size/
  offset`) and any container sub-TOCs after sizes change. `docs/blb/toc-format.md`
  documents the invariant `entry[0].offset == 4 + count*12`; assert it.
- **Alignment/padding.** Determine the original inter-asset alignment empirically
  during the round-trip step and replicate it. Don't guess — diff until zero.
- **Sector layout.** `GAME.BLB` is read by sector; confirm whether asset offsets
  must stay sector-aligned (`BLB_ReadSectorsWrapper` @ 0x80020848). If edits change
  sizes enough to shift sectors, the ISO-level offsets in the disc image may also
  need updating — scope check this during round-trip.

## Architecture

Keep the entity_viewer shape — it's already the right size.

```
tools/map_editor/
  server.py        # stdlib HTTP; serves app.html + JSON/PNG endpoints; POST /save
  app.html         # single-file UI: canvas + layer list + tile palette + entity panel
  README.md
tools/pack_blb/    # inverse of extract_blb, shared handler registry
  __main__.py
  handlers/        # mirror extract_blb handlers: serialize back to bytes
```

- **Backend** decodes via the existing extractor, serves per-layer tile grids +
  the tile atlas (as PNG, reusing `layers.py`/`sprites.py` decode) + entity list.
  `POST /save` takes the edited grids/entities, calls `pack_blb`, writes a copy.
- **Frontend** is one canvas per rendered layer stack, a tile picker, a layer list
  with visibility/lock, and a reused entity inspector. No framework; vanilla JS and
  canvas, matching `app.html`.
- **No new deps.** Python stdlib + Pillow (already in `.venv`), as the viewer does.

## Milestones

0. **Settle the live-preview question** — poke asset-200 RAM in a running stage via
   `game_watcher.lua`/the bridge and see whether the tile changes without re-init.
   Cheap, and it picks the inner-loop strategy. (Do in parallel with milestone 1.)
1. **Round-trip repack** — `extract → pack_blb → byte-identical BLB`. (Highest risk
   for *persistence*; do first. No UI needed — a script + a diff.)
2. **Read-only render** — canvas view of layers using the decoded atlas; layer
   toggles; entity + spawn markers overlaid. (Mostly reuse.)
3. **Tile painting + live preview** — edit asset 200, push to RAM over the bridge,
   see the tile change in-emulator instantly. (Fast loop; no repack yet.)
4. **Persist + entity/spawn editing** — repack on save; edit 501 and 100; boot to
   confirm edits survive a reboot.
5. **Polish** — undo/redo, keyboard nav, save-as, a "diff vs original assets" report.

## Open questions

- Does the game tolerate a size-changed `GAME.BLB` in place, or must total size /
  sector count be preserved? (Answer during milestone 1.)
- Tint bits (12-15) and per-layer color tables: expose in the tile picker, or leave
  tint at 0 for placed tiles in the MVP? (Lean: leave at 0, add later.)
- Entity `name`/type hashes are opaque build-time hashes (see memory
  `asset-hash-ids-not-ascii`) — editing type means picking from *known* placed
  hashes, not inventing new ones. Confirm the editable set.
- Is `patch_disc_file` (live push) or swap-file-and-`make record` the smoother boot
  loop? Try both in milestone 3.

## Related

- `tools/entity_viewer/` — the UI/base to fork.
- `tools/extract_blb/` — the decoders to invert.
- `docs/systems/tiles-and-tilemaps.md` — tile/layer/tilemap format.
- `docs/blb/toc-format.md`, `docs/blb.hexpat` — container format (source of truth).
- `docs/reference/level-data-context.md` — asset-ID ↔ pointer map.
