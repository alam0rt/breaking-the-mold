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

### Entity System

| Address | Name | Returns |
|---------|------|---------|
| 0x8007B7A8 | GetEntityCount | Entity count from header |
| 0x8007B7C8 | GetAsset100Field1C | VRAM rect count |

## Sprite System

| Address | Name | Purpose |
|---------|------|---------|
| 0x8007BC3C | InitSpriteContext | Setup sprite for entity |
| 0x8007BB10 | LookupSpriteById | Find sprite by 32-bit ID |
| 0x8007B968 | FindSpriteInTOC | Search container TOC |
| 0x8007BEBC | GetFrameMetadata | Get 36-byte frame entry |
| 0x80010068 | DecodeRLESprite | RLE decoder with flip |

## Entity System

| Address | Name | Purpose |
|---------|------|---------|
| 0x8001C720 | InitEntitySprite | Core entity sprite init |
| 0x8001FCFC | InitPlayerEntity | Player setup |
| 0x80047FB8 | InitBossEntity | Boss setup |
| 0x80020E1C | EntityTickLoop | Main entity update loop |
| 0x80024DC4 | LoadEntitiesFromAsset501 | Load entity defs |
| 0x800213A8 | AddEntityToList | Register entity |
| 0x80021190 | AddEntityToQueue | Add to update queue |

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

## Main Loop

| Address | Name | Purpose |
|---------|------|---------|
| 0x800828B0 | main | Main game loop |
| 0x80020E80 | RenderEntities | Draw entities |

## Global Variables

| Address | Name | Description |
|---------|------|-------------|
| 0x800AE3E0 | blbHeaderBuffer | BLB header in RAM |
| 0x8009DCC4 | LevelDataContext | Level loading state |
| 0x8009B4B4 | g_GameBLBFile | CdlFILE for GAME.BLB |
| 0x800A59F0 | g_GameBLBSector | BLB starting sector (0x146) |
| 0x800A6060 | g_pSecondarySpriteBank | Secondary sprites |
| 0x800A6064 | g_pLevelDataContext | Context pointer |

## Level Metadata Accessors

| Address | Name | Purpose |
|---------|------|---------|
| 0x8007A5CC | GetPrimaryBufferSize | Buffer allocation size |
| 0x8007AA28 | GetLevelFlagByIndex | Password-selectable flag |

## Related Documentation

- [LevelDataContext](level-data-context.md) - Context structure
- [PAL/JP Comparison](pal-jp-comparison.md) - Address differences
