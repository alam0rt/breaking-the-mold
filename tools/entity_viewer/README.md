# Entity Viewer

A small local web app for browsing a stage's **entity placements** and **sprite
animations** side by side — built to confirm behaviour and align on naming.

It does **not** re-parse `GAME.BLB`. It reads the already-extracted tree under
`extracted/` (entity lists + raw `600_*.bin` sprite containers) and reuses the
verified RLE decoder in `tools/extract_blb/handlers/sprites.py` to decode frames
on demand. Only dependency beyond the stdlib is Pillow (already in `.venv`).

## Run

```bash
python3 tools/entity_viewer/server.py            # http://localhost:8077
python3 tools/entity_viewer/server.py --port 9000 --root extracted
```

Then open the printed URL. (If `extracted/` is missing, regenerate it with the
BLB extractor — see `docs/blb/README.md`.)

## What you can do

- **Left:** pick a world → stage.
- **Entities tab:** every placement from asset 501 (type, name, pixel/tile
  position, variant, layer, bbox). Click one to inspect it and link it to a sprite.
- **Sprites tab:** the stage's sprite atlas (stage-`map` + world-`shared`), each a
  thumbnail. Click one to open the animation player.
- **Detail panel:**
  - Frame-accurate **animation player** (play/pause, step, scrub) rendered from
    decoded frames, anchored by each frame's `render_x/render_y` so it doesn't jitter.
  - Per-frame metadata: size, render offset, delay (ticks → ms), `callback_id`
    (frame SFX/particle hook), flip, hitbox.
  - **Tag** each animation with a state (`idle`/`walk`/`attack`/`hit`/`death`/…),
    then use the **trigger** row to jump straight to, e.g., the `death` clip — this
    is the "press onhit → play death animation" affordance.
  - **Name** a sprite (shared by hash across all stages) and leave notes.

## Annotations

Sprite names, per-animation state tags, entity↔sprite links, and notes persist to
`tools/entity_viewer/annotations.json` (created on first edit). That file is the
shared substrate for aligning names — commit it when it's worth keeping.

## Known limits / next steps

- Entity → sprite is **manual** for now (the type→sprite-hash map lives in the
  executable, not the BLB). The link picker lets us record it as we confirm pairs;
  a later pass can seed links from `docs/reference/entity-types.md` and the sprite
  ID arrays in `docs/systems/sprites.md`.
- "Trigger" previews the clip we *tagged* as the state; it does not run the real
  entity FSM. Driving live behaviour would mean hooking PCSX-Redux (the repo already
  has `game_watcher.lua` + the MCP bridge) — a natural follow-up.
