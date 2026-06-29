---
title: "The BLB Data Format"
category: book
tags: [book, blb, file-format, assets, cd, streaming]
---

# The BLB Data Format

> *A note on names.* This is a clean-room reconstruction. Field names, asset
> labels, and function names are inferred from behaviour and may be wrong. The
> byte offsets, sizes, and the numeric asset IDs, though, are read directly from
> the file and the running machine — those are the parts to trust. Where a
> document (including this one) claims a format is "fully understood," read it as
> "understood as far as anyone has managed to break it," not as gospel.

The first two chapters took the *running* game apart — the world rebuilt every
frame, the entities ticking through their function pointers. But before any of
that can happen, the world has to come off a disc. Push a Skullmonkeys CD into a
PlayStation and almost everything the game will ever show you — every tile, every
sprite, every level layout, every sound, the menu, the bosses, the attract-mode
demos — lives inside a single file called **`GAME.BLB`**. It is about 48 MB, which
in 1998 was most of the disc, and it is the one artifact you must understand to
understand the game's data, because it *is* the game's data.

This chapter is the map of that file. Four questions:

1. **What is the shape of the file?** — sectors, the header, and the data behind it.
2. **How does the game find a level inside 48 MB?** — the header's directory.
3. **How does it find an asset inside a level?** — the recursive table of contents.
4. **What decides what loads next?** — the playback sequence, the game on a tape.

---

## Section 1 — A file measured in sectors

`GAME.BLB` is not really a "file" in the way a `.txt` is a file. It is a slab of a
CD, and everything about its layout is shaped by that fact. A PlayStation reads its
disc in **2048-byte sectors**, and it reads them *fast in a line and slow across a
seek* — pulling sector 1000 then 1001 is nearly free, but jumping from sector 1000
to sector 50000 means physically moving a laser head and waiting. So the BLB is
laid out to be read in long forward runs, and every position and length inside it
is counted not in bytes but in **sectors**.

The whole file is two regions:

```
GAME.BLB  (starts at disc sector 0x146)
├── Header        first 0x1000 bytes  = exactly 2 sectors
│                 a directory + a script; read once, kept forever
└── Level data    everything after, addressed by sector offset
                  streamed in on demand, level by level
```

The header is tiny and permanent. It is read once at boot into a fixed spot in RAM
(`0x800AE3E0` on the PAL disc) and stays resident for the entire session — it is
the index you consult every time you need to find anything else. The level data is
enormous and transient: it is streamed in a level at a time, used, and overwritten
by the next level. You never hold the whole 48 MB in memory; the PlayStation only
has 2 MB. You hold the 4 KB header, plus whichever one level you're currently
standing in.

---

## Section 2 — The header: a directory and a script

Those first 4096 bytes do two jobs at once. They are a **directory** (where is each
level on the disc?) and a **script** (in what order does the game play them?). Laid
out, the header is a row of fixed-size tables:

```
0x000   Level Metadata Table    26 entries × 0x70   "where each level lives"
0xB60   Movie Table             13 entries × 0x1C   "where each FMV lives"
0xCD0   Sector Table            32 entries × 0x10   loading screens / specials
0xECC   Mode-6 Sector Table     17 entries × 0x04   world-intro transitions
0xF10   Credits Table            2 entries × 0x0C
0xF31   Level / Movie / sector counts (three u8s)
0xF34   Playback Sequence Data                       the "script"
```

Two things in there are worth noticing immediately.

First, **not everything is inside the BLB.** The movie table doesn't contain movies;
it contains *pointers to external files* — `\MVLOGO.STR`, `\MVINTRO1.STR`, and so on
— the streaming FMV cutscenes that live as their own `.STR` files elsewhere on the
disc. The BLB header is the spine, but a few limbs (the movies) hang off it by
reference. Everything else — tiles, sprites, sound, layouts — really is embedded.

Second, the header is **version-specific**. The Japanese release (SLPS-01501) shifts
the sector table 32 bytes earlier, carries one fewer movie, and moves the credits
index to fix a trigger bug. The format is not a neutral container; it was generated
by a specific build for a specific disc, and the offsets are baked to that build.
This is the BLB telling you, structurally, that it was never meant to be read by
anything but the one executable that shipped with it.

---

## Section 3 — A level entry: where the parts are on the disc

The heart of the header is the **level metadata table**: 26 entries, each exactly
`0x70` (112) bytes, one per level theme (MENU, SCIE, TMPL, the bosses, …). An entry
is the answer to "if the player picks *this* level, which sectors do I read, and how
much RAM do I need?"

```
# Where the shared block lives, and how big a heap to carve
0x00  u16   primary sector offset
0x02  u16   primary sector count
0x04  u32   primary buffer size      (RAM to reserve for this level)
0x0C  u8    level asset index (0–25)
0x0D  u8    password-selectable?

# How many stages, and how big each stage's map is
0x0E  u16   stage count (1–6)
0x10  u16[6] tertiary data sizes (×32)

# Per-stage disc locations — parallel arrays, one slot per stage
0x1E  u16[6] secondary sector offsets   \  the TILESET blocks
0x2C  u16[6] secondary sector counts     /  (artwork)
0x3A  u16[6] tertiary  sector offsets    \  the MAP blocks
0x48  u16[6] tertiary  sector counts      /  (layout + entities)

# Who this is
0x56  char[5]   level ID    ("SCIE\0")
0x5B  char[21]  level name  ("Science Centre")
```

Recall from Chapter 1 that a stage is built from three blocks: the **shared** block
(geometry + audio, used by every stage of the level), the **tileset** block (the
artwork), and the **map** block (the layout). This entry is where those blocks are
*addressed*. The shared block has one location (the `primary` fields). The tileset
and map blocks are **per-stage**, and they're stored as six **parallel arrays** —
six secondary offsets, six secondary counts, six tertiary offsets, six tertiary
counts. To load stage *N* of a level, you index slot *N* of each array.

That parallel-array layout encodes the secondary/tertiary pairing physically:
`secondary[N]` is stage *N*'s artwork, `tertiary[N]` is stage *N*'s map, sitting
right next to each other in the same struct, the way they sit interleaved on the
disc. The format doesn't just store the data; it stores the *adjacency* the streamer
relies on.

Notice also `primary buffer size` at `0x04`. The level entry doesn't only say where
the data is — it says **how much memory the level will need**, computed at build time.
The loader reads this number first and carves a heap of exactly that size before it
reads a single asset. The file is, in part, a set of instructions for managing the
2 MB the data won't all fit in.

---

## Section 4 — The table of contents: finding an asset inside a block

So the header points you at a block on the disc, and you stream that block's sectors
into RAM. Now you're holding a few hundred KB of bytes. How do you find the tile
pixels, or the entity list, or the palettes inside it?

Every block — shared, tileset, or map — begins with the same little structure: a
**table of contents.** A count, followed by that many 12-byte entries:

```
0x00   u32   entry count
0x04   ┌─ entry 0 ─┐ ┌─ entry 1 ─┐ ...
       │ type  u32 │ each entry: which asset, how big, and where
       │ size  u32 │
       │ off   u32 │ (offset from the start of this block)
       └───────────┘
```

The `type` is a number — and here the format does something quietly elegant. The
asset types are just **decimal numbers**: 100, 101, 200, 300, 500, 600, 700. Stored
in the file they appear as their hex (`100 → 0x64`, `300 → 0x12C`, `600 → 0x258`),
but the numbering is human and deliberate, grouped by purpose. The 100s and 300s are
tiles, the 200s are tilemaps, the 500s are gameplay (collision, entities), the 600s
are shared sprites and audio. Reading a TOC is reading a labelled manifest: "this
block contains a 100, a 200, a 201, a 500, a 501…"

Most assets are **raw** — their bytes start right at the offset the TOC gives, no
further header. But three asset types (sprites `600`, audio `601`, palettes `400`)
are **containers**: their data begins with *another* TOC, a sub-table of the same
12-byte shape, indexing the things inside them. And here the first field changes
meaning: instead of an asset type, it's an **identity**. In the sprite container it's
the 32-bit sprite hash from Chapter 1; in the audio container, a sample ID; in the
palette container, a palette index.

```
Block TOC  ──►  Asset 600 (sprite container)
                  └─ Sub-TOC  ──►  sprite 0x09406d8a  (the clayball)
                                   sprite 0x400c989d
                                   ...
```

So the BLB is a **recursive directory**: a file of blocks, each block a TOC of assets,
some assets themselves TOCs of sub-assets. The same 12-byte `{id, size, offset}`
record describes a level's tile-graphics asset and an individual sprite within a
container. Learn the record once and you can walk the entire 48 MB, top to bottom,
without ever needing the executable to tell you what anything is. The file describes
itself.

### The asset catalog

The full set of asset types, grouped by the block they live in:

| ID | Hex | Block | Contents |
|----|-----|-------|----------|
| 100 | `0x064` | tileset/map | tile header: counts, BG colour, spawn |
| 101 | `0x065` | tileset | VRAM slot config (8 levels only) |
| 200 | `0x0C8` | map | tilemap container (grids per layer) |
| 201 | `0x0C9` | map | layer table (92-byte records) |
| 300 | `0x12C` | tileset | tile pixels (8bpp indexed) |
| 301 | `0x12D` | tileset | per-tile palette index |
| 302 | `0x12E` | tileset/map | per-tile flags |
| 303 | `0x12F` | tileset | animated-tile config |
| 400 | `0x190` | tileset | palette container (256-colour CLUTs) |
| 401 | `0x191` | tileset/map | palette animation |
| 500 | `0x1F4` | map | tile collision map |
| 501 | `0x1F5` | map | entity placements (24-byte records) |
| 502 | `0x1F6` | map | VRAM rectangles |
| 503 | `0x1F7` | map | sprite animation offsets |
| 504 | `0x1F8` | map | vehicle path data (FINN/RUNN only) |
| 600 | `0x258` | shared/map | sprite + geometry container |
| 601 | `0x259` | shared/tileset | ADPCM audio sample bank |
| 602 | `0x25A` | shared | per-sample audio metadata |
| 700 | `0x2BC` | map | demo input replay (9 levels) |

A pleasing property for anyone writing a parser: several asset sizes are **derivable
from each other**, so they double as integrity checks. `Asset 501 size ÷ 24` is the
entity count — and it must equal a field in the tile header. `Asset 302` is exactly
one byte per tile, so its size is the total tile count. `Asset 602` is four bytes per
audio sample; `Asset 401`, four bytes per palette. If those equations don't hold,
you've mis-parsed something. The format is redundant in just the places that let you
catch your own mistakes.

---

## Section 5 — The playback sequence: the game on a tape

We've covered the directory half of the header. The other half — the last ~200 bytes,
the **playback sequence** — is the script, and it's what turns a pile of addressable
levels into *a game with an order*.

The sequence is a linear list of steps, and each step has a **mode** that says what
kind of thing to play:

| Mode | Meaning | Where the data comes from |
|------|---------|---------------------------|
| 1 | play a movie | movie table (external `.STR`) |
| 2 | roll credits | credits table |
| 3 | **play a normal level** | level metadata entry (`0x70` bytes) |
| 4 / 5 | special loading | special sector loading |
| 6 | inter-level transition | mode-6 sector table (world intros) |

Read down the modes and you get the spine of the whole game: movie, movie, movie
(the logos and intro), then the menu, then level-3-level-3-level-3 (a world's stages
in order), a mode-6 world-intro, the next world, a boss, and eventually the ending
movies and credits. The game *is* this list, played front to back.

The engine tracks its position with a single number — the **current sequence index**,
a cursor into the script, kept in the `LevelDataContext`. Advancing the game is, at
its simplest, incrementing that cursor and acting on the mode it lands on. But the
cursor can also be *seeked*, and that's how the non-linear parts of the game work:

- The **menu** sets the cursor by searching the sequence for a level by ID
  (`SeekToLevelInSequence`).
- The **password system** does the same to drop you into a later world.
- The **attract-mode demo** is the prettiest case: leave the menu idle and the game
  seeks the cursor to a specific level (the Monkey Shrines), plays it under recorded
  input, then seeks back to the menu when the demo ends or you press a button.

That recorded-input demo closes a loop we opened in Chapter 2. Remember that a tick
is a pure function of (state, input), which is why a recording of nothing but
controller states, replayed through the same loop, reproduces the same play. **That
recording is asset 700** — the demo input replay, embedded in the map block of the
nine levels that have a demo. The sequence script seeks to the level; the loader
streams in asset 700 alongside the tiles and entities; and the game ticks forward
feeding stored button presses to an invisible player. The whole attract mode is two
small pieces of the BLB — a cursor seek and a buffer of inputs — riding on the exact
machinery that runs the real game.

---

## From disc to RAM, end to end

Stitch the pieces together and the path a level takes from the disc into a playable
world is one clean pipeline:

```
boot
  └─ read disc sector 0x146, 2 sectors  ──►  header @ 0x800AE3E0  (resident)

enter a level (sequence cursor lands on a mode-3 step)
  ├─ read the 0x70-byte level entry           "where & how big"
  ├─ carve a heap of `primary buffer size`     reserve the RAM
  ├─ stream the SHARED block      ─┐
  ├─ stream stage N's TILESET block │  sectors → RAM, by offset+count
  ├─ stream stage N's MAP block    ─┘  from the level entry's parallel arrays
  ├─ for each block: read its TOC, drop each asset's pointer into LevelDataContext
  ├─ upload tiles to VRAM, audio to SPU         (Chapter 1)
  └─ spawn entities from asset 501, build layers
        └─ the world is now resident, and the tick loop (Chapter 2) takes over
```

Every disc read at the bottom of this funnels through one tiny PSY-Q call,
`CdBLB_ReadSectors`, reached via a stable callback the loader was handed at boot —
so the high-level asset code never touches the CD hardware directly; it just asks
"give me these sectors" and gets bytes back. The header told it which sectors. The
TOCs told it what the bytes mean. The sequence told it this was the level to load
at all.

And that is the BLB, whole: a single sector-addressed archive, fronted by a 4 KB
header that is equal parts disc map and game script, holding a recursive tree of
self-describing blocks. There is no separate level format, no separate sprite format,
no separate sound format — there is *one* record, `{id, size, offset}`, applied at
every level of nesting, from "which level" down to "which frame of which sprite." The
genius of it isn't sophistication; it's that almost the entire 48 MB can be walked
with a single 12-byte struct and a count, and that the same file which stores the
game also stores the order in which to play it.

---

## Where the format lives

| The piece | Doc / function | Address |
|-----------|----------------|---------|
| Header bootstrap (read 2 sectors) | `LoadBLBHeader` | `0x800208b0` |
| Header in RAM (PAL) | — | `0x800AE3E0` |
| BLB start sector | — | `0x146` (stored at `0x800A59F0`) |
| Parse a level's primary block | `LevelDataParser` | `0x8007a62c` |
| Parse a tileset/map block's TOC | `LoadAssetContainer` | `0x8007b074` |
| Find a sprite in a container sub-TOC | `FindSpriteInTOC` | `0x8007b968` |
| Low-level CD read | `CdBLB_ReadSectors` | `0x80038ba0` |
| Streaming read callback | `BLB_ReadSectorsWrapper` | `0x80020848` |
| Advance the sequence cursor | `AdvancePlaybackSequence` | (src/level.c) |
| Seek the cursor to a level | `SeekToLevelInSequence` | (src/level.c) |
| The asset → context pointer map | `LevelDataContext` | — |

---

### Further reading (the working notes behind this chapter)

- [BLB File Format overview](../blb/README.md)
- [BLB Header](../blb/header.md) · [Level Metadata](../blb/level-metadata.md) · [TOC Format](../blb/toc-format.md)
- [Asset Types (complete reference)](../blb/asset-types.md)
- [Level Loading System](../systems/level-loading.md)
- The ImHex template — `docs/blb.hexpat` — is the project's source of truth for the layout.
- Previous chapters: [What's in a World?](01-whats-in-a-world.md) · [Making Things Tick](02-making-things-tick.md)
