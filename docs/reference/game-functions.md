# Game Functions Reference

Key functions for Skullmonkeys (PAL SLES-01090).

## BLB Loading

| Address | Name | Purpose |
|---------|------|---------|
| 0x800208B0 | LoadBLBHeader | Load header at boot |
| 0x8007A1BC | InitLevelDataContext | Init context struct |
| 0x8007A62C | LevelDataParser | Parse primary segment |
| 0x8007B074 | LoadAssetContainer | Parse secondary/tertiary |
| 0x8007D1D0 | InitializeAndLoadLevel | Full level load |
| 0x8007D8A0 | SetupAndStartLevel | Full level setup: load, spawn, audio, remap |
| 0x8007CFC0 | RespawnAfterDeath | Respawn player without full reload |
| 0x8007DD0C | DisplayTransitionSprite | Show transition sprite (0x8ab92024) |
| 0x80038BA0 | CdBLB_ReadSectors | Low-level CD read |
| 0x80020848 | LoaderCallback | CD read wrapper |

## Asset Accessors

### Tile System

| Address | Name | Returns |
|---------|------|---------|
| 0x8007B53C | GetTotalTileCount | Sum of tile counts |
| 0x8007B588 | CopyTilePixelData | Tile pixels pointer |
| 0x8007B6B0 | GetPaletteIndices | ctx[6] - Asset 301 |
| 0x8007B6BC | GetTileSizeFlags | ctx[7] - Asset 302 |
| 0x8007B4D0 | GetPaletteGroupCount | Palette count |
| 0x8007B4F8 | GetPaletteDataPtr | Palette from sub-TOC |
| 0x8007B658 | GetAnimatedTileData | ctx[11] - Asset 303 |

### Layer System

| Address | Name | Returns |
|---------|------|---------|
| 0x8007B6C8 | GetLayerCount | u16 layer count |
| 0x8007B700 | GetLayerEntry | 92-byte layer entry |
| 0x8007B6DC | GetTilemapDataPtr | Tilemap sub-TOC |
| 0x8007B434 | GetLevelDimensions | Width/height from Asset 100 |
| 0x8007B458 | GetSpawnPosition | Spawn tile position |

### Audio System

| Address | Name | Returns |
|---------|------|---------|
| 0x8007BA50 | GetAsset601Size | Audio bank size |
| 0x8007BA78 | GetAsset601Ptr | Audio sample pointer |
| 0x8007BAA0 | GetAsset602Ptr | Volume/pan table |
| 0x8007C088 | UploadAudioToSPU | Upload to SPU RAM |
| 0x8007C7EC | StopAllSPUVoices | SpuSetKey(0, 0xffffff) - stop all voices |
| 0x8007CB20 | StopCDStreaming | Disable CD streaming |

### Entity System

| Address | Name | Returns |
|---------|------|---------|
| 0x8007B7A8 | GetEntityCount | Entity count from header |
| 0x8007B7C8 | GetAsset100Field1C | VRAM rect count |

## Sprite System

| Address | Name | Purpose |
|---------|------|---------|
| 0x8007BC3C | InitSpriteContext | Parse sprite header, find animation |
| 0x8007BB10 | LookupSpriteById | Find sprite in tertiary then secondary |
| 0x8007B968 | FindSpriteInTOC | Search ctx+0x70 and ctx+0x40 TOCs |
| 0x8007BBEC | FUN_8007bbec | Get sprite data pointer |
| 0x8007BEBC | GetFrameMetadata | Get 36-byte frame entry |
| 0x80010068 | DecodeRLESprite | RLE decoder with flip support |
| 0x8001CDAC | AllocSpriteRenderContext | Allocate SpriteRenderContext (0x3C bytes) |
| 0x8001D080 | SetEntitySpriteId | Set sprite ID at entity+0xBC |

### Sprite Lookup Order

```
LookupSpriteById(sprite_id) @ 0x8007bb10
  ├─► FindSpriteInTOC(g_pLevelDataContext, sprite_id)
  │     └─► Search ctx+0x70 (tertiary TOC)
  │     └─► Search ctx+0x40 (tertiary fallback)
  └─► If not found: search g_pSecondarySpriteBank (often NULL)
```

### Sprite TOC Entry (12 bytes)

```
+0x00  u32  Sprite count (at TOC start)
+0x04  u32  Sprite ID (32-bit hash)
+0x08  u32  Data size
+0x0C  u32  Offset from container start
```

## Entity System

| Address | Name | Purpose |
|---------|------|---------|
| 0x8001A0C8 | InitEntityStruct | Zero and init 0x44C entity struct |
| 0x8001C980 | InitEntityAnimationState | Init animation fields +0x6c to +0xfe |
| 0x8001C720 | InitEntitySprite | Core entity sprite init |
| 0x8001C868 | InitEntityWithSprite | Full entity+sprite wrapper |
| 0x8001CB88 | EntityUpdateCallback | Default entity tick callback |
| 0x8001D0C0 | FUN_8001d0c0 | Set sprite frame index |
| 0x8001D218 | FUN_8001d218 | Set sprite visibility |
| 0x8001D290 | TickEntityAnimation | Animation frame tick handler |
| 0x8001D554 | ApplyPendingSpriteState | Apply pending sprite changes |
| 0x8001D748 | UpdateSpriteFrameData | Update frame dimensions |
| 0x8001EAAC | EntitySetState | State machine transitions |
| 0x8001FCFC | InitPlayerEntity | Player setup |
| 0x80020E1C | EntityTickLoop | Main entity update loop |
| 0x800223BC | FreeEntityLists | Free all entities in +0x1C, +0x20, +0x24 lists |
| 0x80024468 | CleanupDeadEntities | Remove entities with death flag (bit 2) |
| 0x80024F34 | InitAnimatedTileEntities | Create entities for animated tiles |

### Entity List Management

GameState uses several linked lists for entity management:
- **+0x1C**: Tick list (z-sorted, for EntityTickLoop)
- **+0x20**: Render list (z-sorted, for RenderEntities)
- **+0x24**: Update queue (priority sorted)
- **+0x28**: Entity definition list (loaded from Asset 501)

| Address | Name | Purpose |
|---------|------|---------|
| 0x80020F68 | AddToZOrderList | Add to +0x1C z-sorted tick list |
| 0x8002107C | AddToXPositionList | Add to +0x20 x-sorted render list |
| 0x80021190 | AddToUpdateQueue | Add to +0x24 priority queue |
| 0x80021350 | AddToEntityDefList | Prepend to +0x28 entity def list |
| 0x80021B48 | AddEntityToBothLists | Add to both +0x1C and +0x20 |
| 0x800213A8 | AddEntityToSortedRenderList | Register entity in sorted lists |
| 0x80021D30 | RemoveFromTickList | Remove entity from +0x1C |
| 0x80021DC0 | RemoveFromRenderList | Remove entity from +0x20 |
| 0x80021E50 | RemoveFromUpdateQueue | Remove entity from +0x24 |
| 0x80021EDC | ChangeRenderZOrder | Update z-order in +0x20 list |
| 0x80022074 | RemoveEntityFromAllLists | Remove from all lists + destructor |
| 0x80022218 | ClearTickList | Free all entries in +0x1C |
| 0x80022338 | ClearEntityDefList | Free all entries in +0x28 |

### Entity Scale Functions

| Address | Name | Purpose |
|---------|------|---------|
| 0x8001A26C | ScaleXByEntityScale | Scale by entity+0x58 (fixed-point) |
| 0x8001A29C | ScaleYByEntityScale | Scale by entity+0x5C (fixed-point) |

## Menu System

| Address | Name | Purpose |
|---------|------|---------|
| 0x80076928 | InitMenuEntity | Menu entity initialization |
| 0x80076BA0 | InitMenuStage1 | Main menu setup (4 buttons, Klaymen) |
| 0x80077068 | InitMenuStage2 | Password entry setup |
| 0x800771C4 | InitMenuStage3 | Options/color picker setup |
| 0x800773FC | InitMenuStage4 | Load game setup (3 slots) |
| 0x80077940 | MenuTickCallback | Menu per-frame update |
| 0x80077AF0 | MenuInputHandler | Menu input processing |
| 0x800778EC | SetMenuBackgroundColor | Set BG from color index |
| 0x800754CC | AttachMenuCursor | Create cursor for button |
| 0x80075FF4 | InitPasswordDisplay | 12-digit password entity |
| 0x8007C388 | PlaySoundEffect | Play sound effect with pan |
| 0x80020F68 | AddToZOrderList | Z-order sorted list (+0x1C) |
| 0x8002107C | AddToXPositionList | X-position sorted list (+0x20) |
| 0x800213A8 | AddEntityToSortedRenderList | Register entity |
| 0x80021190 | AddEntityToQueue | Add to update queue |
| 0x80024DC4 | LoadEntitiesFromAsset501 | Load 24-byte entity defs to ctx+0x28 |
| 0x800250C8 | AddPreInitEntitiesToList | Pre-init entities to ctx+0x1C |
| 0x80027A00 | InitEntity_8c510186 | Menu cursor entity |
| 0x80034BB8 | InitEntity_168254b5 | Particle entity |
| 0x80052678 | InitEntity_a89d0ad0 | Unknown entity |
| 0x80047FB8 | InitBossEntity | Boss setup |
| 0x80059A70 | InitPlayerSpriteAvailability | Check available player sprites |
| 0x800281A4 | CreateMenuEntities | Menu UI factory (creates ~30 entities) |

### Entity Init Pattern

All `InitEntity_*` functions follow this pattern:
```c
void InitEntity_XXXXXXXX(Entity* entity) {
    InitEntitySprite(entity, 0xXXXXXXXX, z_order, x, y, flags);
    entity[1] = TickCallback;      // Main update callback
    entity[6] = &Hitbox;           // Collision data
    EntitySetState(entity, initial_state, callback);
    // ... entity-specific setup
}
```

The sprite ID in the function name **IS** the sprite ID passed to InitEntitySprite.

## Layer Initialization

| Address | Name | Purpose |
|---------|------|---------|
| 0x80024778 | InitLayersAndTileState | Master layer init |
| 0x8001ECC0 | InitLayerRenderContext_Standard | Large layers |
| 0x8001F150 | InitLayerRenderContext_Medium | Medium layers |
| 0x8001F534 | InitLayerRenderContext_Small | Small layers |
| 0x80021590 | AddLayerToRenderList_Standard | Add to list C |
| 0x80021778 | AddLayerToRenderList_Medium | Add to list B |
| 0x80021960 | AddLayerToRenderList_Small | Add to list A |
| 0x80019238 | FreeLayerRenderSlot | Free single layer render entry |
| 0x80019474 | FreeAllLayerRenderSlots | Free all 20 layer render slots |

## Tile Rendering

| Address | Name | Purpose |
|---------|------|---------|
| 0x80025240 | LoadTileDataToVRAM | Upload tiles |
| 0x80017540 | FUN_80017540 | Tile rendering |
| 0x8001713C | RenderTilemapSprites16x16 | Render tiles |
| 0x8001601C | InitTilemapLayerRendering | Setup primitives |

## Display

| Address | Name | Purpose |
|---------|------|---------|
| 0x800399A8 | DisplayLoadingScreen | Show loading MDEC |
| 0x80024678 | LoadBGColorFromTileHeader | Set background color |
| 0x800134B8 | ClearOrderingTables | Clear OT buffers, set g_SkipVSync |

## Main Loop

| Address | Name | Purpose |
|---------|------|---------|
| 0x800828B0 | main | Main game loop |
| 0x8007CD34 | InitGameState | One-time game initialization |
| 0x80020E1C | EntityTickLoop | Per-frame entity updates (+0x1C list) |
| 0x80020E80 | RenderEntities | Draw entities via +0x20 render list |
| 0x8008150C | RemapEntityTypesForLevel | Entity type translation table |
| 0x80081E84 | ClearSaveSlotFlags | Reset save slot state |
| 0x8007EAAC | SaveCheckpointState | Save tick list to +0x134, set checkpoint flags |
| 0x8007EAEC | RestoreCheckpointEntities | Restore entities from checkpoint |
| 0x8007CA9C | StartCDAudioForLevel | Initialize CD audio for level |
| 0x8007CCB8 | TickCDStreamBuffer | Stream CD data every 4 frames |
| 0x80082C10 | ProcessDebugMenuInput | Debug level select menu |
| 0x800259D4 | UpdateInputState | Process controller input |
| 0x8001352C | WaitForVBlankIfNeeded | Conditional VSync wait |
| 0x80013500 | FlushDebugFontAndEndFrame | Draw debug text, end frame |
| 0x80013554 | SwapBuffersAndClearOT | Swap buffers, clear OT |
| 0x80023DBC | UpdateCameraPosition | Camera scroll based on player at +0x30 |

## Collision System

| Address | Name | Purpose |
|---------|------|---------|
| 0x800226F8 | CheckEntityCollision | Iterate +0x24 list for collision detection |
| 0x8001B3F0 | CheckBoundingBoxOverlap | Box overlap test (called by CheckEntityCollision) |

## Graphics Initialization

| Address | Name | Purpose |
|---------|------|---------|
| 0x80013268 | InitGraphicsSystem | Double-buffer GPU setup (320x256) |
| 0x80013B1C | InitVRAMSlotTable | Texture page slot configuration |
| 0x800260D0 | InitPlayerControllerState | Player 1 controller init |

## Player Creation

| Address | Name | Purpose |
|---------|------|---------|
| 0x8007DF38 | SpawnPlayerAndEntities | Player dispatch based on level flags |
| 0x8007B47C | GetLevelFlags | Read level type flags from header |
| 0x800596A4 | CreatePlayerEntity | Default platforming player |
| 0x8006EDB8 | CreateGlidePlayerEntity | GLID level player |
| 0x80070D68 | CreateSoarPlayerEntity | SOAR level player |
| 0x80073934 | CreateRunnPlayerEntity | RUNN level player |
| 0x80074100 | CreateFinnPlayerEntity | FINN level player |
| 0x80078200 | CreateBossPlayerEntity | Boss fight player |
| 0x80044F7C | CreateCameraEntity | Camera entity creation |
| 0x8007A33C | SetSequenceIndexByMode | Init playback sequence |

## Tile Header Accessors

| Address | Name | Purpose |
|---------|------|---------|
| 0x8007B3F0 | GetCurrentStageIndex | Current stage index |
| 0x8007B3FC | GetAsset101Entry | VRAM slot config (bank_a/b counts) |
| 0x8007B490 | GetTileHeaderWorldIndex | World index at +0x20 (was misnamed GetTileHeaderField08) |
| 0x8007B4A4 | GetTileHeaderField1A | Header field at +0x1A |
| 0x8007B910 | GetTileHeaderField16 | Header field at +0x16 |
| 0x8007B924 | GetVehicleDataPtr | Asset 504 pointer (FINN/RUNN) |

## Tile Attribute Accessors (Asset 500 / Collision)

| Address | Name | Purpose |
|---------|------|---------|
| 0x8007B74C | HasTileAttributes | Returns true if Asset 500 exists |
| 0x8007B758 | GetTileAttributeUnknown | Reads header bytes 0-3 (two u16, unknown purpose) |
| 0x8007B778 | GetTileAttributeDimensions | Reads header bytes 4-7 (width/height in tiles) |
| 0x8007B79C | GetTileAttributeData | Returns pointer to collision data (header+8) |
| 0x80024CF4 | InitTileAttributeState | Copies Asset 500 header to GameState |

## VRAM/Texture Management

| Address | Name | Purpose |
|---------|------|---------|
| 0x80013B1C | InitVRAMSlotTable | Configures texture page slots from Asset 101 |

## Memory Management

| Address | Name | Purpose |
|---------|------|---------|
| 0x800143F0 | AllocateFromHeap | Block-based heap allocator |
| 0x800145A4 | FreeFromHeap | Free allocated memory |
| 0x800213A8 | AddEntityToSortedRenderList | Sorted render list insertion |

## Player Callbacks

| Address | Name | Purpose |
|---------|------|---------|
| 0x8005B414 | PlayerTickCallback | Main player per-frame update |
| 0x8005A914 | PlayerProcessTileCollision | Handle tile attribute effects |
| 0x80059BC8 | CheckWallCollision | Check 4-point vertical wall collision |
| 0x80026164 | ResetPlayerCollectibles | Clear collectible flags on level start |
| 0x800262AC | DecrementPlayerLives | Decrement lives counter at +0x11 |
| 0x80026B18 | ResetPlayerUnlocksByLevel | Reset unlocks based on level index |
| 0x80077940 | MenuTickCallback | Menu entity per-frame update |
| 0x800589E8 | InitHaloPowerup | Create halo effect entity |

## Collision Functions

| Address | Name | Purpose |
|---------|------|---------|
| 0x800241F4 | GetTileAttributeAtPosition | Read tile attribute from collision map |
| 0x800245BC | CheckTriggerZoneCollision | Check bbox trigger zones |
| 0x80059BC8 | CheckWallCollision | 4-point vertical wall check |

See [Normal Player Documentation](../systems/player-normal.md) for details.

## FINN Player (Swimming Levels)

| Address | Name | Purpose |
|---------|------|---------|
| 0x80074100 | CreateFinnPlayerEntity | FINN player creation |
| 0x80074B54 | FinnStateIdle | Main swimming state handler |
| 0x80074BF4 | (inline) | FINN state enter (not defined as function) |
| 0x80074C84 | (inline) | FINN state turn |
| 0x80074D18 | (inline) | FINN state special |
| 0x80074DB0 | (inline) | FINN state swim |
| 0x8001FEA8 | EntityApplyMovementCallbacks | X/Y movement dispatch |
| 0x800241F4 | GetTileAttributeAtPosition | Tile collision lookup |

See [FINN Player Documentation](../systems/player-finn.md) for details.

## Game Mode

| Address | Name | Purpose |
|---------|------|---------|
| 0x8007C36C | SetGameMode | Set game mode (0-6) |

## Global Variables

| Address | Name | Description |
|---------|------|-------------|
| 0x800AE3E0 | blbHeaderBufferBase | BLB header buffer, also GPU state |
| 0x8009DC40 | g_GameStateBase | Main game state structure |
| 0x8009DCC4 | LevelDataContext | Level loading state (GameState+0x84) |
| 0x8009D5F8 | g_EntityTypeCallbackTable | Entity type dispatch table (121 entries × 8 bytes) |
| 0x8009B4B4 | g_GameBLBFile | CdlFILE for GAME.BLB |
| 0x800A59F0 | g_GameBLBSector | BLB starting sector (0x146) |
| 0x800A6060 | g_pSecondarySpriteBank | Secondary sprites |
| 0x800A6064 | g_pLevelDataContext | Context pointer |
| 0x8009C174 | g_PlayerSpriteTable | Player sprite ID lookup table (16+ entries) |
| 0x8009C3A8 | g_PlayerSpriteVariants | 7 player sprite variants |
| 0x8009B174 | g_MenuCursorSprites | Menu cursor sprite table |
| 0x8009B180 | g_MenuSpriteTable2 | Menu sprite table 2 |
| 0x8009B18C | g_MenuSpriteTable3 | Menu sprite table 3 |
| 0x800A6082 | DAT_800a6082 | Current game mode (0-6) |
| 0x800A5764 | g_pPlayer1Input | Player 1 input state pointer |
| 0x800A5768 | g_pPlayer2Input | Player 2 input state pointer |
| 0x800A5754 | g_pPlayerState | Player persistent state |
| 0x800A576C | g_pCurrentInputState | Active input state |
| 0x800A5950 | g_GameFlags | Game flags (bit 0x80=debug menu) |
| 0x800A594C | g_SkipVSync | Skip VSync flag |
| 0x800A5948 | g_FrameReady | Frame complete flag |
| 0x800A5960 | g_GameStatePtr | Pointer to active GameState |
| 0x800A5770 | g_DefaultBGColorR | Default BG red component |
| 0x800A5771 | g_DefaultBGColorG | Default BG green component |
| 0x800A5772 | g_DefaultBGColorB | Default BG blue component |

### Entity Type Callback Table (g_EntityTypeCallbackTable)

Located at `0x8009d5f8`, this table maps entity types (0-120) to initialization callbacks.
Each entry is 8 bytes: `[flags u32, callback_ptr u32]`.

Used by `RemapEntityTypesForLevel` to translate BLB entity types to internal types,
then index this table to get the init callback. Stored at `GameState+0x7C` at runtime.

### Player Sprite Tables

Player uses `InitEntityWithSprite` with sprite ID tables at fixed addresses:

**g_PlayerSpriteTable (0x8009c174)** - 16 player state sprite IDs:
```
[0]  0x08208902    [1]  0x48204012    [2]  0x8569A090    [3]  0x0708A4A0
[4]  0x052AA082    [5]  0x393C80C2    [6]  0x1CF99931    [7]  0x00388110
[8]  0x1C3AA013    [9]  0x1C395196    [10] 0x3838801A    [11] 0x04084011
[12] 0x092B8480    [13] 0x0B2084D0    [14] 0x292E8480    [15] 0x282B8491
```

**g_PlayerSpriteVariants (0x8009c3a8)** - 7 player variant sprites:
```
[0] 0x48608484    [1] 0xF936C015    [2] 0xFA20CD14    [3] 0x5D2CE434
[4] 0x52A08394    [5] 0x5A204C30    [6] 0xD305D045
```

These tables are indexed by player state to select different animations.
`InitPlayerSpriteAvailability()` at 0x80059a70 checks which variants are present.

## GameState Offsets

| Offset | Type | Purpose |
|--------|------|---------|
| +0x00 | s16 | Mode callback base offset |
| +0x02 | s16 | Mode callback table index (or -1) |
| +0x04 | ptr | Mode callback pointer or table |
| +0x0C | ptr | Layer render context pointer |
| +0x1C | ptr | Entity tick list head (z-sorted) |
| +0x20 | ptr | Entity render list head (z-sorted) |
| +0x24 | ptr | Entity collision/update queue head |
| +0x28 | ptr | Entity definition pool (raw Asset 501 defs) |
| +0x2C | ptr | Player entity (alternate ref) |
| +0x30 | ptr | Player entity pointer |
| +0x48 | s16 | Level width in pixels |
| +0x4A | s16 | Level height in pixels |
| +0x50 | ptr | Player 1 input state pointer |
| +0x7C | ptr | Entity type callback table (→ 0x8009d5f8) |
| +0x84 | struct | LevelDataContext base |
| +0x10C | u32 | Input repeat timer/flags |
| +0x116 | s16 | Spawn X position (pixels) |
| +0x118 | s16 | Spawn Y position (pixels) |
| +0x11C | u32 | Scale factor (0x8000/0xC000/0x10000) |
| +0x124/5/6 | u8[3] | Player RGB color |
| +0x130 | u8 | BG color update request flag |
| +0x131/2/3 | u8[3] | Pending BG RGB color |
| +0x134 | ptr | Checkpoint entity list (saved from +0x1C) |
| +0x138 | u32 | Checkpoint score |
| +0x140 | ptr | Checkpoint/HUD data pointer |
| +0x148 | u8 | Level transition state |
| +0x14C | ptr | HUD entity pointer |
| +0x161 | u8 | Respawn/continue flag |
| +0x16C | ptr | Glide/scrolling buffer |
| +0x170 | u8 | Level has player flag |
| +0x171-17A | u8[10] | Password-selectable level list |
| +0x17B | u8 | Password level count |
| +0x198-19B | u8[4] | Respawn state data |
| +0x19C | u8 | Level flag bit 1 (from GetLevelFlags) |
| +0x19D | u8 | TileHeader field 0x1A value |

## Level Metadata Accessors

| Address | Name | Purpose |
|---------|------|---------|
| 0x8007A5CC | GetPrimaryBufferSize | Buffer allocation size |
| 0x8007AA28 | GetLevelFlagByIndex | Password-selectable flag |

## Related Documentation

- [Game Loop](../systems/game-loop.md) - Main loop and player creation
- [LevelDataContext](level-data-context.md) - Context structure
- [PAL/JP Comparison](pal-jp-comparison.md) - Address differences

