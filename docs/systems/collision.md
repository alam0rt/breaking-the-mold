# Tile Collision System

> **This document has been consolidated.** The canonical reference for the tile
> collision system is [collision-complete.md](collision-complete.md).
>
> Quick reference (keep a copy open while coding): [../tile-collision-quick-ref.md](../tile-collision-quick-ref.md).

## One-line summary

Each tile has a **1-byte collision attribute** in Asset 500. Range `0x01-0x3B` is
solid (with per-value slope heights in `g_SlopeHeightTable @ 0x8009d228`); `0x3C+`
are trigger zones dispatched by `PlayerProcessTileCollision @ 0x8005a914`.

## Where to look

| Need | Go to |
|------|-------|
| Full attribute table (0x00-0xFF) | [collision-complete.md](collision-complete.md) |
| Slope height lookup | [collision-complete.md § Slope Height Table](collision-complete.md#slope-height-table-g_slopeheighttable--0x8009d228) |
| Wind / death / spawn-zone triggers | [collision-complete.md § Complete Trigger Attribute Map](collision-complete.md#complete-trigger-attribute-map) |
| Semi-solid (one-way) platforms | [collision-complete.md § Semi-Solid Override Attributes](collision-complete.md#semi-solid-override-attributes) |
| Entity-vs-entity collision masks | [collision-complete.md § Entity Collision Masks](collision-complete.md#entity-collision-masks) |
| Quick constant cheatsheet | [../tile-collision-quick-ref.md](../tile-collision-quick-ref.md) |
| Player physics / movement response | [player/player-physics.md](player/player-physics.md) |
| Asset 500 binary format | [../blb/asset-types.md § Asset 500](../blb/asset-types.md) |

## Key functions

| Address | Name |
|---------|------|
| 0x800241f4 | `GetTileAttributeAtPosition` |
| 0x800245bc | `CheckTriggerZoneCollision` |
| 0x80024cf4 | `InitTileAttributeState` |
| 0x8005a5c4 | `CheckTileCollisionOverride` (semi-solid) |
| 0x8005a914 | `PlayerProcessTileCollision` |
| 0x80059bc8 | `CheckWallCollision` |
| 0x800638d0 | Player movement callback (main) |
| 0x80081c0c | `GetSlopeHeightAtSubpixel` |
| 0x800226f8 | `CheckEntityCollision` |
