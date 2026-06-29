---
title: "Struct Field Cleanup Notes — 2026-06-12"
category: analysis
tags: [analysis, struct, field, cleanup, 2026]
---

# Struct Field Cleanup Notes — 2026-06-12

Clean-room Ghidra MCP pass over placeholder/bad struct field names in `SLES_010.90`.

**Naming caveat:** these are not original source names. They are evidence-backed working names chosen to make Ghidra/source docs less misleading. Prefer conservative names when fields are overloaded or only partially understood.

## Evidence-backed inferred renames applied in Ghidra and source headers

| Struct | Offset | New name | Evidence |
|--------|--------|----------|----------|
| `Entity` / sprite-derived structs | `+0x7C` | `pPaletteData` | `UploadEntityTextureIfDirty @ 0x8001E5B8` passes it to `UploadCLUTToVRAM(entity->spriteContext, entity->pPaletteData)`. |
| `SpriteContext` / `SpriteEntity` | `+0x12` / `+0x8A` | `spriteLookupByte` | `InitSpriteContext @ 0x8007BC3C` stores the low byte of sprite lookup entry word `+0x08`; `LoadSpriteFramesToVRAM @ 0x80018DDC` copies it into level sprite-cache slot `+0x14`. No confirmed reader found in the checked cache/render paths. |
| `SpriteEntity` | `+0x8B` | `spriteContextValid` | Embedded 20-byte sprite context valid/decode byte; set by sprite context init and checked by decode helpers. |
| `SpriteEntity` | `+0x94` | `sequenceTable` | `StartAnimationSequence @ 0x8001E790` stores the callback sequence table and immediately calls `StepAnimationSequence @ 0x8001E7B8`. |
| `SpriteEntity` | `+0x98/+0x9C` | `queuedStateMarker/queuedStateCallback` | `EntityProcessCallbackQueue @ 0x8001E928` consumes this queued pair into the active pair, clears it, and dispatches the state. |
| `SpriteEntity` | `+0xA0/+0xA4` | `activeStateMarker/activeStateCallback` | `EntityProcessCallbackQueue` and `StepAnimationSequence` copy the next state/sequence entry here immediately before dispatch. |
| `SpriteEntity` | `+0xA8/+0xAC` | `exitCallbackMarker/exitCallback` | `EntitySetCallback @ 0x8001EC18` installs this finalizer hook; existing hooks are dispatched before replacement/state changes/sequence steps. Callers use it for sound teardown, flag cleanup, hitbox restore, delayed effects, and input re-enable. |
| `SpriteEntity` | `+0xB4/+0xB8` | `frameMotionX/frameMotionY` | `UpdateSpriteFrameData @ 0x8001D748` computes 16.16 per-frame motion from frame deltas and frame timer. |
| `SpriteEntity` | `+0xE6/+0xE8` | `frameDeltaX/frameDeltaY` | Raw signed frame metadata deltas copied by `UpdateSpriteFrameData`. |
| `SpriteEntity` | `+0xE2/+0xE4` | `sequenceStep/sequenceLength` | Sequence index and length maintained by `StartAnimationSequence` / `StepAnimationSequence`. |
| `SpriteEntity` | `+0xFE` | `doubleFrameDelay` | Non-zero doubles the frame delay loaded from `SpriteFrameEntry+0x04`. |
| `SpriteFrameEntry` | `+0x0E/+0x10` | `frameDeltaX/frameDeltaY` | Replaces stale `flip_flags` / `unknown_10` interpretation. Hitbox fields start at `+0x12`. |
| `SpriteHeader` | `+0x14/+0x16` | `format_flags/unused_clut_word` | Palette pointer is not here; palette data reaches entities through `Entity+0x7C`. |
| `GameState` | `+0x10/+0x14` | `reserved10/reserved14` | No current reader evidence; old layer-list-head theory is disproved. |
| `GameState` | `+0x18` | `postRenderCallbackContext` | `main @ 0x800828B0` calls through `ctx+0x1C` after `RenderEntities` and `DrawSync(0)`. |
| `GameState` | `+0x74/+0x78` | `trigger_zone_data_ptr/trigger_zone_count` | `CheckTriggerZoneCollision @ 0x800245BC` iterates a 16-byte trigger-zone array bounded by count. |
| `PlayerState` | `+0x02/+0x04` | `results_total_tally/world_index_tally` | `CreateResultsScreenEntity @ 0x80078200` displays and updates these results values. |
| `PlayerEntity` | `+0x11E` | `bounceLockTimer` | Player movement/collision traces use it as a short lockout counter. |
| `PlayerEntity` | `+0x134/+0x135` | `specialMoveQueued/specialMoveMode` | Input/state branches queue and select special movement mode. |
| `FinnPlayerEntity` | `+0x104/+0x108` | `wakeEntity_or_velocityX/velocityY_or_stateCallback` | Fields are intentionally named as overlays: wake pointer during creation, velocity/FSM storage during movement. |
| `FinnPlayerEntity` | `+0x10C..+0x10F` | `rotationAngle/rotationSpriteBucket/rotationVelocity` | `FinnHandleInput` and `FinnUpdateRotationSprite` derive 16-way sprite bucket from heading angle. |
| `SoarPlayerEntity` | `+0x118/+0x119` | `gravityHoldTimer/forcedGravityTimer` | `PlatformStateInit_BounceWithGravity @ 0x800727EC` sets `gravityHoldTimer` to 5; `PlatformEntityProcessInput @ 0x800720C4` decrements these while applying/capping vertical gravity response. No non-zero `forcedGravityTimer` producer was found in the checked SOAR/platform range. |
| `SoarPlayerEntity` | `+0x11A/+0x11B` | `jumpTransitionLock/jumpHoldTimer` | Jump input enters `PlatformStateInit_BounceWithGravity` when the lock is clear; when set, input refreshes a 10-frame hold timer used by `SoarState_SelectNextByInput @ 0x80071FD8`. |
| `SoarPlayerEntity` | `+0x11C/+0x11D` | `pendingLevelLoadId/inputEnabled` | Trigger-zone code stores the pending direct level id at `+0x11C`; `PlatformState_EnablePlayerInput @ 0x80072CAC` sets `inputEnabled`, and the main platform tick only calls input processing while it is non-zero. |
| `SoarPlayerEntity` | `+0x11E/+0x120` | `stateReturnTimer/levelLoadTimer` | `PlatformTick_MainWithSoundAndTimers @ 0x800713F4` decrements both; `stateReturnTimer` transitions back to animated idle, and `levelLoadTimer` copies `pendingLevelLoadId` to `GameState.direct_level_load` when it reaches zero. |
| `LayerEntry` | `+0x18/+0x1A` | `auto_scroll_speed_x/auto_scroll_speed_y` | Copied into layer contexts and used by autonomous wrapped-scroll update functions. |
| `LayerEntry` | `+0x1C/+0x1D` | `reverse_scroll_x/reverse_scroll_y` | Direction flags for autonomous wrapped scroll. |
| `LayerEntry` | `+0x22/+0x24` | `auto_scroll_enable_y/auto_scroll_enable_x` | Enables autonomous scroll axes. |
| `LevelDataContext` | `+0x64` | `sector_read_callback` | `InitLevelDataContext @ 0x8007A1BC` stores it; `LoadAssetContainer @ 0x8007B074` calls it as `(sectorOffset, sectorCount, dst)`. Normally `BLB_ReadSectorsWrapper @ 0x80020848`. |

## Resolved misidentifications

- `/game/SpriteTypeCallbackEntry[481]` was a stale/misidentified Ghidra hypothesis, not a uniform sprite-type dispatch table. The packed region beginning at `0x80010344` is mixed callback/vtable data. `0x80010344` is now typed/labeled as `BasicEntityVtableShort_SubmitPrimitiveBuffer`; the old datatype was renamed to `/game/ObsoleteSpriteTypeCallbackEntry` and should not be used for lifecycle naming.

## Remaining ambiguous fields

- `LevelDataContext+0x64` is now resolved as `sector_read_callback`; no longer considered ambiguous.

## Placeholder audit after cleanup

Ghidra still intentionally contains generic names where evidence is weak:

- Padding/reserved fields in `Entity`, `GameState`, `SpriteHeader`, and vtables remain named as padding/reserved/unused rather than guessed.
- `SoarPlayerEntity.forcedGravityTimer` is named conservatively; current scan found decrement/use sites but no non-zero producer in the checked SOAR/platform range.
- `SpriteContext+0x12` is named `spriteLookupByte`, but the semantic meaning of the source lookup-entry byte is still unknown because no runtime reader was found after it is copied into cache metadata.
- `/game/ObsoleteSpriteTypeCallbackEntry` remains only as a tombstone datatype so future passes do not revive the bad table-wide sprite-dispatch interpretation.
- Ghidra generated/export snapshot docs may preserve old names for historical comparison; current headers and Ghidra datatypes should be preferred.

The unresolved Ghidra fields above now have comments explaining why they remain generic, so future passes do not accidentally promote weak guesses into authoritative names.

## Documentation/source synchronization

- Source headers updated: `include/Game/entity.h`, `include/Game/game_state.h`, `include/Game/player_state.h`, `include/Game/sprite.h`, `include/Game/layer_entry.h`.
- BLB pattern updated for the corrected 0x24-byte `SpriteFrameMetadata` layout.
- Older generated Ghidra export docs may still contain stale names where they preserve historical snapshots.