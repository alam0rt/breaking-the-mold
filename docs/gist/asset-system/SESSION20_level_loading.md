# Session 20 — Level loading & stage-sequence architecture

Full trace of how a level loads primary/secondary/tertiary assets and maps between stages.

## Stage progression: the PLAYBACK SEQUENCE (one interleaved list of levels + movies)
- Lives in BLB header **+0xf34** (count +0xf30). Each entry has a **mode byte at +0xf36**:
  **0x01 = MOVIE**, **0x03 = LEVEL**. Levels and FMV cutscenes share one ordered sequence.
- A level entry's byte **+0xf92** indexes the **LevelMetadata table** (`idx * 0x70`).
- `SeekToLevelInSequence` walks the sequence to find a level by its asset-index byte
  (metadata+0x0C); `FUN_8007a4c0` matches MOVIES by `strcmp` vs the **Movie Table @ header+0xb64**
  (13 x 0x1c, 4-char IDs: DREA/LOGO/ELEC/INT1/.../YNTS/EVIL/END1/END2).
- Advance logic: `GameModeCallback @0x8007E654` — on level-complete, if lives `[0x11]`==0 ->
  game over, else seek next sequence entry.

## LevelMetadata entry (0x70 bytes, 26 entries @ header+0x000)
+0x04 asset/data ptr · +0x08 data offset · **+0x0C level asset index (stage id)** · +0x0D flag ·
+0x5B flag · rest = TOC sizes / sector / per-stage flags.

## Segment load (LoadAssetContainer @0x...38935 -> LevelDataContext ctx[])
ctx[1..0x16] map to BLB asset types (matches asset-types.md):
- **PRIMARY** = shared bank: Asset 600 geometry/bg + 601 audio + 602 (loaded once per session-ish)
- **SECONDARY** = level tiles: 100/300/301/302/303/400/401 + 601/602
- **TERTIARY** = sprites/entities: 200/201/500/501/502/503/504/600(sprites)/700
- **Audio uploaded TWICE** (UploadAudioToSPU): primary bank then secondary — the mechanism
  behind global vs level-specific sounds (Session 14).

## Shared sprite set: DAT_8009dab4
`LoadLevelSpriteAssets` calls `LoadSpriteHashArrayToVRAM(OT, &DAT_8009dab4)` — a
**zero-terminated array of sprite HASHES loaded to VRAM every level** (the sprite analog of the
audio primary bank: always-resident core sprites — Klaymen, common FX). Plus conditional load of
**0x168254b5 (Projectile)** when a level flag is set.
- VALUES live in the .data segment (only `undefined DAT_8009dab4;` in the .c). **Dumping
  DAT_8009dab4 from the EXE = the exact shared-sprite manifest** (high-value, like 0x8009b144).

## Special / vehicle stages
Asset 100 +0x1A special-level-id **99 = FINN/SEVN** (3D/vehicle). FINN/RUNN use **Asset 504
vehicle waypoint paths** (FINN 78 waypoints = swim rails, RUNN 1 = runner path).

## Net
- Complete level-load + stage-sequence pipeline mapped: playback sequence (levels+movies
  interleaved by mode byte) -> LevelMetadata (0x70) -> 3-segment asset load -> dual audio bank +
  shared-sprite VRAM array.
- Identified DAT_8009dab4 (shared sprite manifest) as the second high-value .data table to dump
  (alongside 0x8009b144 entity-type table).
- Movie/level interleaving + Movie Table (header+0xb64) explains cutscene placement.

## Files
- `level_loading_map.json`
