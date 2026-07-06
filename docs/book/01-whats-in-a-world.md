---
title: "What's in a World?"
category: book
tags: [book, world, tiles, tilemaps, entities, rendering]
---

# What's in a World?

> *A note on names.* Skullmonkeys is a clean-room reconstruction. No original
> source, symbols, or comments survive. Every function and field name in this
> chapter — `LoadTileDataToVRAM`, `renderMarker`, `scroll_x` — is a guess we
> inferred from behaviour, and a guess can be wrong. The **data layouts**, on the
> other hand, are read straight off the disc and verified against the running
> machine; trust the byte offsets more than the labels attached to them.

Load a level in Skullmonkeys and the PlayStation shows you a world: a swamp, a
shrine, a factory floor scrolling past at three different speeds, with Klaymen
bouncing through it and monkeys lobbing fireballs from the ledges. It *looks* like
a place. It is not. There is no "swamp" anywhere in memory — no bitmap of the
level, no scene graph, nothing you could point at and call the world.

What there is, instead, is a recipe and a box of parts. Every time you enter a
stage the engine pulls a few blocks off the CD, scatters their contents across a
heap, wires up a tangle of pointers, and from that moment on *reassembles the
world sixty times a second* out of small reusable pieces. Tear down the illusion
and you find three questions, which are the three sections of this chapter:

1. **What are the parts?** — how tiles, the atoms of the scenery, are stored.
2. **How are they assembled?** — how stacked tilemaps combine into a world.
3. **Who lives there?** — how entities are turned into pixels on the screen.

---

## A world arrives in three blocks

Before any of that, the world has to come off the disc. `GAME.BLB` is one ~48 MB
archive holding all 90 stages, and a stage is never stored as a single thing.
Each one is split into **three blocks**, loaded by `LoadAssetContainer`
(`0x8007b074`) into a scratch structure we call the `LevelDataContext` — really
just a bag of pointers, one per asset type.

| Block | What we call it | What it holds |
|-------|-----------------|---------------|
| Shared | `shared` | geometry + audio shared by every stage of a world |
| Tileset | `stageN_tileset` | the **artwork**: tile pixels, palettes, tile flags |
| Map | `stageN_map` | the **layout**: tilemaps, collision, entity placements |

The split that matters most here is the second versus the third: **the tileset
block is the art, the map block is the arrangement.** One says *what a tile looks
like*; the other says *where tiles go and who stands on them*. Keeping them apart
is the single most important design decision in the whole world format, and you
will see its fingerprints everywhere — most strikingly when we get to entities,
where the art lives in one place and the placement in another and *neither of them
knows the other's name*.

Inside each block, assets are addressed by a numeric type ID through a small
table of contents (`{type, size, offset}`, twelve bytes per entry). When you see
"Asset 300" below, that is one such entry: the tile pixels. The loader's whole job
is to read the TOC and drop each asset's pointer into the right slot of the
context:

```
case 0x064: ctx->tile_header     = data;  // Asset 100
case 0x12C: ctx->tile_pixels     = data;  // Asset 300
case 0x12D: ctx->palette_indices = data;  // Asset 301
case 0x12E: ctx->tile_flags      = data;  // Asset 302
case 0x190: ctx->palette_container = data; // Asset 400
case 0x0C8: ctx->tilemap_container = data; // Asset 200
case 0x0C9: ctx->layer_entries     = data; // Asset 201
case 0x1F5: ctx->entities        = data;  // Asset 501
...
```

Nothing is copied or reformatted. The world, for the rest of the level's life, is
those raw blocks sitting in a heap with the engine reading through them in place.

---

## Section 1 — Tiles: the atoms of the scenery

### How a tile is stored

The PlayStation has no tilemap hardware. It draws textured quads, full stop. So a
"tile" in Skullmonkeys is not a hardware concept — it is a convention the engine
imposes: a small square of 8-bit indexed pixels that it will later paste onto the
screen as a 16×16 sprite.

Tiles come in two sizes, and the size is the first thing that shapes how they are
stored. **Asset 100**, the tile header (36 bytes), opens the tileset block and
counts them:

```
0x00  u8[3]  background RGB
0x08  u16    level width  (in tiles)
0x0A  u16    level height (in tiles)
0x0C  u16    spawn X (tiles)
0x0E  u16    spawn Y (tiles)
0x10  u16    count of 16x16 tiles
0x12  u16    count of  8x8  tiles
0x14  u16    count of extra tiles
```

The pixels themselves live in **Asset 300**, one tile after another with no
headers between them, 8 bits per pixel, sixteen bytes to a row. The two sizes are
segregated — all the 16×16 tiles first, then all the 8×8 tiles — so finding tile
*i* is pure arithmetic, no table lookup:

```c
if (i < count_16x16)
    offset = i * 256;                                   // 16 rows x 16 bytes
else
    offset = count_16x16 * 256 + (i - count_16x16) * 128; // 8x8 tile
```

There is a wrinkle worth pausing on. An 8×8 tile is 128 bytes, but it is stored
with a **16-byte row stride**: eight bytes of pixels, eight bytes of padding,
eight times over. The art is in the left half of each row; the right half is dead
space. The engine pays for the slack in storage to keep one uniform stride for
both tile sizes, so the copy loop never has to special-case a size — a very PSX
trade, memory for instructions.

### From a number to a colour: palettes

Those 8-bit values are not colours. They are **indices into a palette**, and the
indirection is what makes a tile cheap. A 16×16 tile is 256 bytes of indices; the
palette it points into is shared with hundreds of other tiles. Change the palette
and the whole tileset recolours for free — which is exactly how a single set of
tiles serves a sunny stage and its murky twin.

Two more assets close the loop:

- **Asset 301** — one byte per tile, naming *which* palette that tile uses.
- **Asset 400** — the palettes themselves, 256 colours each, 512 bytes apiece, in
  PlayStation 15-bit BGR (5 bits per channel, top bit = semi-transparency). Colour
  index 0 is transparent by convention.

So a fully-described tile is a three-way join: *Asset 300* gives its pixels,
*Asset 301* says go to palette *p*, *Asset 400* turns each pixel byte into a
15-bit colour. None of these three knows the others exist; the engine holds them
together by index alone.

### Flags: the fine print on each tile

**Asset 302** is the last byte-per-tile array, carrying three bits of behaviour:

| Bit | Mask | Meaning |
|-----|------|---------|
| 0 | `0x01` | semi-transparent (GPU alpha blend) |
| 1 | `0x02` | tile size: 0 = 16×16, 1 = 8×8 |
| 2 | `0x04` | skip — don't upload this tile at all |

The size bit here is redundant with the counts in Asset 100, and that redundancy
is a tell: the tools that built these files tracked size per-tile, and the runtime
keeps both views because each is convenient at a different moment.

### The one-time upload to VRAM

All of the above is still in main RAM, and the GPU cannot texture from main RAM.
So the last step of building a tileset is to **stamp every tile into video
memory** — once, at load time — via `LoadTileDataToVRAM` (`0x80025240`). For each
tile that isn't flagged *skip*, it pairs the pixels (Asset 300) with the chosen
palette (Asset 400 via Asset 301) and `LoadImage`s the block into the PlayStation's
1024×512 framebuffer, building a texture atlas the renderer will sample from all
level long:

```c
for (i = 0; i < total_tiles; i++) {
    if (tile_flags[i] & 0x04) continue;          // skip flag
    palette = palettes[ palette_indices[i] ];
    pixels  = (i < count_16x16)
            ?  tile_pixels + i*256                // 16x16 source
            :  tile_pixels + count_16x16*256 + (i-count_16x16)*128;
    LoadImage(&rect, pixels);                     // -> VRAM atlas
    DrawSync(0);
}
```

After this runs, the tiles exist as actual pixels in VRAM and the original Asset
300 bytes are, for rendering purposes, spent. The world's *art* is now resident.
What's missing is the *arrangement* — and for that we leave the tileset block and
open the map.

---

## Section 2 — Tilemaps: stacking grids into a world

### A world is a stack, not a picture

Here is the second big idea. A Skullmonkeys world is not one grid of tiles. It is
**several grids stacked front-to-back** — a far backdrop, a couple of parallax
mid-distances, the layer you actually walk on, and one or two foreground layers
that pass in front of you. Each grid is a *layer*, and the finished frame is what
you see looking through the whole stack at once.

The map block carries the stack in two assets:

- **Asset 200** — the tilemap container: the raw grids of cell data, one per layer.
- **Asset 201** — the layer table: one 92-byte record per layer describing *how*
  that grid is positioned, scrolled, tinted, and ordered.

### What a single cell says

Every cell in a layer's grid is one 16-bit word, and it is packed:

```
bits 0–11   tile index   (12 bits)  — 1-based; 0 means "empty, draw nothing"
bits 12–15  tint selector (4 bits)  — picks one of the layer's 16 colour tints
```

Two details carry a lot of weight. First, the tile index is **1-based**: zero is
not "tile 0," it is *the absence of a tile*, and the renderer skips it outright.
In a typical layer most cells are zero — a background is mostly sky — so this
turns the common case into a single compare-and-continue. Second, the top four
bits are a **per-cell tint**: an index into a sixteen-entry colour table that
belongs to the layer (more on that in a moment). The same tile can appear bright
in one cell and shadowed in the next, at no storage cost beyond four bits, which
is how the artists got depth and lighting variation out of a small tile set.

### What a layer record says

Asset 201's 92-byte record is where a flat grid becomes a positioned, moving plane:

```
0x00  u16   x_offset       layer origin X (tiles)
0x02  u16   y_offset       layer origin Y (tiles)
0x04  u16   width          layer width  (tiles)
0x06  u16   height         layer height (tiles)
0x08  u16   level_width    \ copied from Asset 100
0x0A  u16   level_height   /
0x0C  u32   render_param   priority in the low 16 bits (signed)
0x10  u32   scroll_x       parallax factor X  (0x10000 = 1.0)
0x14  u32   scroll_y       parallax factor Y
0x26  u8    layer_type     3 = skip this layer
0x28  u16   skip_render    nonzero = skip this layer
0x2C  u8[48] color_tints   16 RGB triples — the tint table cells index into
```

Three fields make the magic:

**Parallax (`scroll_x` / `scroll_y`).** These are 16.16 fixed-point multipliers on
the camera position. A layer with `scroll_x = 0x10000` moves exactly with the
camera (it's the ground you stand on). A layer at `0x8000` moves at half speed and
reads as twice as distant; `0x0000` is pinned to the screen, an infinitely far
backdrop. Give a handful of layers a graded set of scroll factors and the flat
stack acquires depth the instant the player moves. The observed SCIE opening
stage is a clean example:

| Layer | priority | scroll_x | Reads as |
|-------|----------|----------|----------|
| 0 | 150  | `0x0000`  | far backdrop (locked) |
| 1 | 250  | `0xAAAA`  | distant parallax |
| 3 | 350  | `0xCCCC`  | parallax |
| 6 | 650  | `0xF333`  | near parallax |
| 7 | 950  | `0x10000` | **the gameplay plane** |
| 9 | 1250 | `0x10CCC` | foreground (drifts *faster* than the camera) |

**Priority (`render_param`).** The low 16 bits are a signed depth key, and — this
is the part that ties the whole renderer together — **layers and entities share
one priority axis.** Lower is further back. Backgrounds sit around 150–800, the
main gameplay layers around 900–1100, foreground 1200–1500, and the player and HUD
all the way out at 10000. The player's high number is *why* he passes in front of
every scenery layer but behind the HUD. There is no separate "entity plane"; a
sprite at priority 1000 simply lands between the layer at 950 and the layer at
1050.

**The tint table (`color_tints`).** Sixteen RGB triples — these are the very
entries the four high bits of each cell select. Entry 0 is white (no tint).

### Turning a layer into pixels

Here is where the naive picture — "walk the whole grid every frame and emit a
sprite per cell" — turns out to be wrong, and the real design is far cannier.

A level is big: a layer's grid can be hundreds of tiles across. But the *screen*
is not. At any instant you can see only about **21 columns by 15 rows** of tiles.
So `InitTilemapLayerRendering` (`0x8001601c`) allocates exactly that much and no
more: a fixed **screen-sized grid** of `0x15` (21) × (screen-height-in-tiles + 1)
primitive cells — and, because the PlayStation renders one frame while building the
next, *two* copies of it (double-buffered). Each cell owns two little packets: a
16×16 sprite (`SPRT_16`) and a "which texture page" command (`DR_TPAGE`).

The clever part is that these packets are **wired together once, at load time,
into a single chain** and then left alone:

```
dr[0] → spr[0] → dr[1] → spr[1] → dr[2] → spr[2] → … → spr[n]
```

`CatPrim` is the little "point this packet's next-link at that one" primitive that
threads them. After init, the whole screen-grid is one long pre-linked list. The
per-frame job is *not* to rebuild it — it's to **edit it in place.**

That editing is `RenderTilemapSprites16x16` (`0x8001713c`). Given a rectangular
window of cells to refresh, it visits each slot and does one of two things:

```c
cell = in_bounds ? tilemap[(tx - originX) + (ty - originY) * mapW] : 0;

if ((cell & 0x0FFF) == 0) {                 // empty (1-based; 0 = nothing here)
    CatPrim(dr[i], dr[i+1]);                // UNLINK this cell's sprite:
                                            //   dr hops straight to the next dr,
                                            //   the GPU never sees spr[i]
} else {                                    // occupied
    CatPrim(dr[i], spr[i]);                 // RELINK the sprite back into the chain
    tile = tileTable[cell & 0x0FFF];        // -> its rect in the VRAM atlas
    tint = colorTable[cell >> 12];          // the cell's 4-bit tint
    SetSprt16(spr[i]);                      // 16x16 sprite; screen pos = sx, sy
    spr[i].u0/v0/clut = tile.uv, tile.clut; // texture coords into the atlas
    spr[i].rgb        = tint;               // per-cell shading
    SetDrawTPage(dr[i], tile.tpage);        // which VRAM page to sample
}
```

So an **empty** cell isn't drawn-and-skipped — its sprite is *spliced out of the
linked list entirely*, and the chain jumps over it. (On real hardware the empty
path also zeroes the packet's length byte, so even the DMA skips its body.) An
**occupied** cell splices its sprite back in and stamps it with the right texture,
tint, and screen position. A background that's mostly sky becomes a chain that's
mostly `DR_TPAGE` hops with a few sprites threaded between them.

> **A packet is tag + code — eight bytes, not four.** Every one of these `SPRT_16`
> and `DR_TPAGE` packets begins with a **tag word** (the "next" link that `CatPrim`
> writes) followed by its payload. The two are load-bearing and separate: the tag
> is the chain, the payload is the draw. Confuse the two — write a `DR_TPAGE` as if
> it were four bytes, or let a "set the texture page" helper clobber the tag it was
> just linked into — and you sever the chain at that cell. The scenery still *looks*
> present in memory but draws with a stale texture page or vanishes. (This is
> exactly the failure the port hit on level 1: coherent tile data, scrambled
> output, because the chain — not the data — was broken.)

**Why go to all this trouble?** Because now scrolling is nearly free. Move the
camera one tile and 20 of your 21 columns are *still valid* — they held last frame
and hold this frame. Only the single new column sliding in at the edge needs
re-stamping, so the per-frame scroll callbacks (`RenderTilemapHorizontalScroll` /
`…VerticalScroll`) re-run `RenderTilemapSprites16x16` on just that **edge strip**,
and only when the integer tile position actually changed. The grid *wraps*: column
21 reuses the packets of column 0, so a layer scrolls forever through a 21-wide
ring of cells. Sub-tile smoothness — the 0–15px between whole-tile steps — comes
not from re-stamping at all but from a single **`DR_OFFSET`** packet bracketing the
chain (`SetupTilemapPrimitives`), which shifts the entire layer by the leftover
pixels in one stroke. Whole tiles wrap; fractional pixels slide; almost nothing is
rebuilt.

Two details survive from the naive version and still matter. The cell step is
**always 16 pixels** and the primitive is **always a `SetSprt16`** — that's how the
two tile sizes reconcile: a 16×16 tile fills its cell pixel-for-pixel, while an 8×8
tile is half-resolution art *stretched* to fill the same 16×16 cell — chunkier,
cheaper to store, used where the sharpness wouldn't be missed (distant parallax,
large flat fills).

Finally, that pre-linked chain gets handed to the **ordering table** (OT) — the
PlayStation's z-sort: a long array of linked-list heads, drawn back-to-front by one
`DrawOTag`. `SetupTilemapPrimitives` drops the layer's whole chain in at the slot
named by the layer's priority. Do that for every layer — and, as we'll see next,
every entity's sprite — and the single `DrawOTag` at frame's end paints the entire
stack in order. *That* is "drawing a world": not compositing pictures, and not even
rebuilding the scenery each frame, but keeping a small ring of pre-wired sprites,
nudging its edges as you move, and flushing one sorted table once.

---

## Section 3 — Entities: drawing the things that live there

### Placement without art

Now the people. Or monkeys. The map block carries **Asset 501**, a flat array of
24-byte records — one per entity the stage will spawn:

```
0x00  u16   bbox x1      0x08  u16   spawn X (pixels)
0x02  u16   bbox y1      0x0A  u16   spawn Y (pixels)
0x04  u16   bbox x2      0x0C  u16   variant / parameter
0x06  u16   bbox y2      0x12  u16   entity type ID
                          0x14  u16   layer flags
```

Read one and notice what's missing: **there is no sprite, no art, no animation, no
filename.** A record says "an entity of *type 27* exists at (x, y), with this
bounding box and this variant." That's all. The world's layout knows *where* a
thing is and *what kind* it is — and nothing about what it looks like. This is the
art/arrangement split from the very first section, now at its most radical: the
map describes a cast by role, like a script that says "ENEMY enters, stage left,"
and trusts something else to know how an enemy is drawn.

### The hidden join: type → hash → sprite

That something else is the **game executable itself.** The engine carries roughly
ninety per-type initialiser functions, dispatched on the `entity type ID`. Each one
hardcodes a **32-bit sprite hash** and hands it to `InitEntitySprite`
(`0x8001c720`) along with the entity's depth and position:

```c
//                entity   sprite hash    z_order   x      y    flags
InitEntitySprite(self,   0xa89d0ad0,     1001,    0xa0,  0x78,  0);  // a game entity
InitEntitySprite(self,   0x168254b5,      959,    x,     y,     1);  // a particle
```

The hash is the name of an art asset — but a *one-way* name. These IDs are opaque
build-time hashes; the original human-readable sprite names did not survive into
the data, and you cannot turn `0xa89d0ad0` back into a word. Type IDs live in the
disc data; sprite hashes live in the code; the only place the two meet is the body
of an init function. So the chain that finally connects a placement to a picture
runs *through the executable*:

```
Asset 501 record  ──(entity type)──►  per-type Init function (in code)
                                            │ hardcodes a 32-bit sprite hash
                                            ▼
                       LookupSpriteById  ──►  sprite data in Asset 600 (this stage)
```

`LookupSpriteById` (`0x8007bb10`) searches the stage's sprite container (Asset 600)
for that hash, falling back to a shared secondary bank. One consequence is worth
stating plainly: **if a stage's Asset 600 doesn't include the hash an entity
asks for, that entity has no art and won't draw.** Per-stage sprite availability,
not the map, decides what can appear.

### What a sprite actually is

The thing the hash resolves to is not a bitmap either. A sprite is a little archive
of its own:

- a **12-byte header** (animation count, and offsets to frame metadata, RLE pixel
  data, and palette);
- an **embedded 512-byte palette** — sprites carry their own colours rather than
  borrowing the tileset's;
- a table of **animations**, each a run of frames;
- per-frame **metadata** (36 bytes): render width/height, X/Y offsets, the hitbox,
  the frame delay, an optional sound callback, a horizontal-flip flag, and the
  offset to that frame's pixels;
- the **pixels**, run-length encoded.

The RLE is the same scheme as the rest of the engine (`DecodeRLESpriteCore`,
`0x80010068`): a stream of 16-bit commands, each one "skip *n* transparent pixels,
then copy *m* literal pixels," with a top-bit flag to advance to the next row.
Sprites are mostly empty space around a character, and RLE makes that emptiness
nearly free — the decoder *skips* transparent runs instead of storing them. The
same decoder runs backwards when a frame's flip flag is set, which is how Klaymen
faces left without a second copy of every frame.

### From sprite to screen, every frame

Tiles were uploaded to VRAM once and left there. Entities can't be, because they
*animate* — the visible frame changes, and so the pixels in VRAM must change. So an
entity's art makes the same journey, but **once per frame**, driven by
`UpdateEntityRender` (`0x8001d988`), the tick callback installed in every sprite
entity's vtable. Each frame it:

1. resolves which animation frame should be showing (advancing timers, applying
   any queued animation change);
2. if the frame changed, **RLE-decodes** it into the entity's private pixel buffer;
3. marks the texture dirty, and `UploadEntityTextureIfDirty` re-uploads that buffer
   to the entity's **VRAM slot** and reloads its palette/CLUT;
4. computes the entity's **screen position** from its world position, the camera,
   the layer's parallax, and any powerup scale;
5. emits a `SetSprt`-family primitive and `AddPrim`s it into the OT at the slot
   named by the entity's `z_order`.

That last step is the payoff of the shared priority axis: the entity's sprite goes
into the *same* ordering table as the tilemap layers, at *its* depth, and is sorted
in among the scenery automatically. Klaymen at priority 10000 lands in front
of the foreground layer at 1350 without anyone comparing the two — the OT does it.

### The dispatch, and the once-per-frame walk

One more layer of indirection, because entities are state machines and "how do I
draw right now" is itself stateful. An entity doesn't call its renderer directly;
it stores an 8-byte *[marker, function]* slot at `+0x1C/+0x20`, and the renderer is
invoked through `InvokeEntityRenderCallback` (`0x80020744`), which reads the marker,
resolves the right function (a direct call, or an entry pulled from a per-entity
slot table), nudges the entity pointer by an encoded offset, and calls it. It is
the same FSM-slot dispatch the engine uses for state, animation, and collision —
the whole engine is built out of these little *[marker, fn]* pairs — and it lets a
boss with several body parts, or an entity mid-transformation, swap *how it draws*
as fluidly as it swaps *how it behaves*.

Pulling the per-entity machinery up to the top, the frame's entity pass is just a
walk. `RenderEntities` (`0x80020e80`) marches the **z-sorted render list** — every
live entity, kept in priority order as they're inserted — and dispatches each one's
render callback, which deposits its sprite into the OT.

---

## Putting one frame together

Step back and the whole world is one loop. The main loop (`0x800828b0`) runs, each
frame:

```
EntityTickLoop          // advance every entity: physics, AI, animation timers
WaitForVBlankIfNeeded   // sync to the display (unless told to run free)
RenderEntities          // walk the z-sorted list; each entity -> OT
DrawSync(0)             // let the GPU catch up
<layer render callback> // walk the layer list; each tile -> OT
DrawSync(0)
VSync(2)                // frame-rate timing
```

Entities and layers both pour their sprites into the **one ordering table**, each
filed under its priority. Nothing in this loop knows what a "swamp" is. There is no
scene, no world object, no composited image held anywhere. There is a heap full of
raw blocks streamed off a CD; a VRAM atlas of 16×16 tiles uploaded once; a stack of
sparse grids that say which tile sits in which cell and how to tint it; a cast of
24-byte records that name a type and a place; and a body of code that, between two
vertical blanks, joins all of it by index and hash and priority, drops the result
into a sorted table, and flushes the table to the screen.

Run that loop sixty times a second and a world appears. Stop the loop and there was
never a world there at all — only the recipe, and the box of parts.

---

## Where the parts live

| The part | Asset / source | Read by |
|----------|----------------|---------|
| Tile counts, spawn, BG colour | Asset 100 | `GetTotalTileCount` `0x8007b53c` |
| Tile pixels (8bpp indexed) | Asset 300 | `LoadTileDataToVRAM` `0x80025240` |
| Per-tile palette index | Asset 301 | `LoadTileDataToVRAM` |
| Per-tile flags (size/skip/blend) | Asset 302 | `LoadTileDataToVRAM` |
| Palettes (256-colour CLUTs) | Asset 400 | `LoadTileDataToVRAM` |
| Tilemap grids (cells) | Asset 200 | `RenderTilemapSprites16x16` `0x8001713c` |
| Screen-grid alloc + pre-chain | — | `InitTilemapLayerRendering` `0x8001601c` |
| Per-frame edge re-stamp | — | `RenderTilemapHorizontalScroll` `0x80016838` / `…VerticalScroll` `0x80016ac8` |
| Submit layer chain + scroll offset | — | `SetupTilemapPrimitives` `0x800178fc` |
| Layer records (92 bytes) | Asset 201 | `InitLayersAndTileState` `0x80024778` |
| Entity placements (24 bytes) | Asset 501 | per-type init dispatch |
| Sprite art (RLE + palette) | Asset 600 | `LookupSpriteById` `0x8007bb10` |
| Per-frame entity draw | — | `UpdateEntityRender` `0x8001d988` |
| Entity render dispatch | — | `InvokeEntityRenderCallback` `0x80020744` |
| Entity render walk | — | `RenderEntities` `0x80020e80` |
| RLE decoder (tiles + sprites) | — | `DecodeRLESpriteCore` `0x80010068` |

---

### Further reading (the working notes behind this chapter)

- [Tiles and Tilemaps](../systems/tiles-and-tilemaps.md)
- [Rendering Order](../systems/rendering-order.md)
- [Sprite System](../systems/sprites.md)
- [BLB File Format](../blb/README.md) · [Asset Types](../blb/asset-types.md)
- [Level Loading](../systems/level-loading.md)
