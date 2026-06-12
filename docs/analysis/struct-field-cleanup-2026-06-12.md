# Struct Field Cleanup Notes — 2026-06-12

Clean-room Ghidra MCP pass over placeholder/bad struct field names in `SLES_010.90`.

**Naming caveat:** these are not original source names. They are evidence-backed working names chosen to make Ghidra/source docs less misleading. Prefer conservative names when fields are overloaded or only partially understood.

## Evidence-backed inferred renames applied in Ghidra and source headers

| Struct | Offset | New name | Evidence |
|--------|--------|----------|----------|
| `Entity` / sprite-derived structs | `+0x7C` | `pPaletteData` | `UploadEntityTextureIfDirty @ 0x8001E5B8` passes it to `UploadCLUTToVRAM(entity->spriteContext, entity->pPaletteData)`. |
| `SpriteEntity` | `+0x8B` | `spriteContextValid` | Embedded 20-byte sprite context valid/decode byte; set by sprite context init and checked by decode helpers. |
| `SpriteEntity` | `+0xB4/+0xB8` | `frameMotionX/frameMotionY` | `UpdateSpriteFrameData @ 0x8001D748` computes 16.16 per-frame motion from frame deltas and frame timer. |
| `SpriteEntity` | `+0xE6/+0xE8` | `frameDeltaX/frameDeltaY` | Raw signed frame metadata deltas copied by `UpdateSpriteFrameData`. |
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
| `LayerEntry` | `+0x18/+0x1A` | `auto_scroll_speed_x/auto_scroll_speed_y` | Copied into layer contexts and used by autonomous wrapped-scroll update functions. |
| `LayerEntry` | `+0x1C/+0x1D` | `reverse_scroll_x/reverse_scroll_y` | Direction flags for autonomous wrapped scroll. |
| `LayerEntry` | `+0x22/+0x24` | `auto_scroll_enable_y/auto_scroll_enable_x` | Enables autonomous scroll axes. |

## Remaining ambiguous fields

- `SoarPlayerEntity+0x118..+0x11D` are still flight-mode flags. Current trace confirms initialization, `flag11B` gating one input-driven state transition, and `flag11A/flag11D` clearing during flight begin, but not enough to assign stable semantic names.
- `SpriteContext+0x12` still has no confirmed reader in the decompiler-wide placeholder search.
- `SpriteTypeCallbackEntry.callback_0/callback_1/callback_3` still need call-site-specific names; broad decompile search mostly hit comments/table references, not enough evidence for precise roles.
- `LevelDataContext.loader_callback` still needs a focused load-path trace before renaming.

## Placeholder audit after cleanup

Ghidra still intentionally contains generic names where evidence is weak:

- Padding/reserved fields in `Entity`, `GameState`, `SpriteHeader`, and vtables remain named as padding/reserved/unused rather than guessed.
- `SoarPlayerEntity` flight flags remain `flag118..flag11D`, `counter11E`, and `counter120` because current evidence only proves initialization and a few branch uses.
- `SpriteContext+0x12` remains `unknown_12` because no runtime reader was found in the decompiler-wide placeholder search.
- `SpriteTypeCallbackEntry.callback_0`, `callback_1`, and `callback_3` remain generic until slot-specific call sites prove lifecycle roles.
- Ghidra generated/export snapshot docs may preserve old names for historical comparison; current headers and Ghidra datatypes should be preferred.

## Documentation/source synchronization

- Source headers updated: `include/Game/entity.h`, `include/Game/game_state.h`, `include/Game/player_state.h`, `include/Game/sprite.h`, `include/Game/layer_entry.h`.
- BLB pattern updated for the corrected 0x24-byte `SpriteFrameMetadata` layout.
- Older generated Ghidra export docs may still contain stale names where they preserve historical snapshots.