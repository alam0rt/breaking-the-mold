# Newly Discovered Functions (2026-01-19)

## Summary
Created 255 new functions from `found_func.csv` bookmarks using Ghidra's Aggressive Instruction Finder.
Named **56 key functions** based on decompilation analysis. **199 FUN_ functions remain** for future naming.

## Function Categories

### Entity Destructors (EntityDestructor_*)
Pattern: Called when entities are destroyed. Sets entity+0x18 to debugger symbol address, frees resources, optionally frees heap.

| Address | Name | Description |
|---------|------|-------------|
| 0x8001ee74 | EntityDestructor_FreeMultiAlloc | Frees multi-alloc resources |
| 0x8001f2f0 | EntityDestructor_FreeResourceType2 | Frees type 2 resources |
| 0x8001f700 | EntityDestructor_FreeResourceType3 | Frees type 3 resources |
| 0x80020010 | EntityDestructor_FreeWithChildRef | Frees entity with child reference at +0x34 |
| 0x800207e4 | EntityDestructor_Simple | Basic destructor (FreeEntitySimple) |
| 0x80025930 | EntityDestructor_Simple2 | Basic destructor variant 2 |
| 0x80028068 | EntityDestructor_RemoveFromRenderList | Removes from render list, frees child at +0x1c |
| 0x8002bff0 | EntityDestructor_DestroyAllChildEntities | Destroys many child entities (offsets 0x20-0x9c) |
| 0x8002c53c-0x8002c7a4 | EntityDestructor_DestroyAndFree[1-7] | DestroyEntityAndFreeMemory + heap free |
| 0x8002d6cc | EntityDestructor_FreeRenderListAt120 | Removes render list +0x120, frees it |
| 0x80030658-0x80030ca8 | EntityDestructor_DestroyAndFree[8-24] | More destroy variants |
| 0x80031508 | EntityDestructor_FreeChildAt1c | Frees child at +0x1c before destroying |
| 0x8003203c | EntityDestructor_WithVRAMSlotFree | Removes +0x118, frees VRAM slot |
| 0x800336e0 | EntityDestructor_WithTextureAndMemory | Frees +0x3c, +0x40, texture resources |
| 0x80049c78 | EntityDestructor_WithSPUStop | Stops SPU voice before destroying |
| 0x8004c464 | EntityDestructor_WithVoiceReset | Resets voice ID to -1 before destroying |
| 0x80056bb8 | EntityDestructor_WithChildEntityCleanup | Cleans up child entity at +0x130 |
| 0x80059640 | EntityDestructor_Simple11 | Simple destructor (FreeEntitySimple11) |
| 0x8006df88 | EntityDestructor_FreeResourceAt24 | Frees resource at +0x24 |
| 0x80070514 | EntityDestructor_Simple13 | Simple destructor (FreeEntitySimple13) |
| 0x80073a10 | EntityDestructor_WithSPUStopAndFree | Stops SPU + destroy + heap free |
| 0x80070fd8 | EntityDestructor_WithSPUStopAt10c | Stops SPU voice at +0x10c |

### Entity Tick Callbacks (EntityTick_*)
Pattern: Called each frame to update entity state. Takes (entity, param2, param3).

| Address | Name | Description |
|---------|------|-------------|
| 0x8001fb0c | EntityTick_AnimationFrameAdvance | Advances animation frame, handles callbacks |
| 0x8002cfbc | EntityTick_EasedMovementInterpolation | Interpolates position using DAT_8009b240 table |
| 0x80026c50 | EntityTick_ScaleFadeOut | Fade out with increasing scale (+0xccc per frame) |
| 0x80026cd8 | EntityTick_ScaleFadeIn | Fade in with decreasing scale |
| 0x80026f34 | EntityTick_PlatformRideIdle | Platform idle state, transitions to PlatformRideStartUp/Down |
| 0x80027418 | EntityTick_DigitDisplayUpdate | Updates digit display HUD element |
| 0x80027860 | EntityTick_HUDItemUpdate | Updates HUD item visibility |
| 0x800326a4 | EntityTick_ParallaxScrollPosition | Parallax layer position from camera scroll |
| 0x800347c8 | EntityTick_UploadTextureWithCallback | Uploads texture if dirty, dispatches callback |
| 0x80042e98 | EntityTick_CollisionWithCleanup | Collision check + EntityOffScreenChildCleanup |
| 0x8002cf48 | EntityTick_PathFollowUpdate | Updates entity position along path |
| 0x80050ae4 | EntityTick_CollisionWithTimerTransition | Collision check, timer at +0x104, state transition |
| 0x8005163c | EntityTick_WithParticleSpawn | Collision check, spawns particle on event |
| 0x8006dbbc | EntityTick_ShadowMirror | Mirrors parent entity's sprite/animation |
| 0x8006e388 | EntityTick_FollowPlayerWithInterpolation | Follows player with position interpolation, 5-frame settle |
| 0x80073a88 | EntityTick_ScaledSpriteWithSound | Updates sound panning and scaled sprite |
| 0x80034ca8 | EntityTick_FrictionAndParallaxScale | Friction, parallax positioning, z-order change |

### Render Functions
| Address | Name | Description |
|---------|------|-------------|
| 0x80033778 | RenderRopeSegments | Renders rope/cable using POLY_GT4 segments |
| 0x80032920 | Render_RotatingStarEffect | 4-pair POLY_G3 triangles with ccos/csin rotation |

### Entity Collision Handlers (EntityCollision_*)
Pattern: Called when entity collision event occurs. param_2 is event type (1=touch, 0x1013=specific, etc.)

| Address | Name | Description |
|---------|------|-------------|
| 0x80056718 | EntityCollision_SpawnSwitchBlock | Spawns switch block on touch |
| 0x8002abc0 | EntityCollision_HUDItemActivate | Activates HUD item groups (1-9) on event 0x1013 |
| 0x8002cef0 | EntityCollision_FlagAndDispatch | Sets flag on 0x1016, dispatches on event 2 |
| 0x8004e0ec | EntityCollision_EnemyHitWithCallback | Wraps EnemyHitMessageHandler, event 2 triggers queue |
| 0x80050cb8 | EntityCollision_ProcessQueueOnly | Simple handler, event 2 processes queue |

### FINN/Vehicle Functions
| Address | Name | Description |
|---------|------|-------------|
| 0x80074a40 | FINN_ApplyHorizontalVelocity | Applies horizontal velocity with friction |
| 0x80074328 | FINN_CollisionHandler | Checks 0x1000 collision, screen bounds check |
| 0x80074bf4 | FinnState_Spawn | Initial state, sets callback, sprites 0x1b280c33/0x3da80d13 |
| 0x80075858 | FINN_ClearSubentityState | Clears subentity at +0x100, sprite 0x39900619 |

### Boss/Enemy AI
| Address | Name | Description |
|---------|------|-------------|
| 0x800734fc | BossCollision_SpawnDebrisAndLayers | Spawns debris particles, scrolling layers on death |
| 0x8004c9e4 | JoeHeadJoe_CheckAttackAndUpdate | Checks attack range, transitions to attack pattern |
| 0x80049cfc | Hazard_TickWithBehaviorTransition | Tick, transitions to HazardSelectRandomBehavior |
| 0x8004fdb8 | HomingProjectile_TrackTarget | Complex homing AI with atan lookup table |

### Collectible/Decor Functions
| Address | Name | Description |
|---------|------|-------------|
| 0x8002d548 | DecorEntity_CollectWithSwirlyEffect | Adds swirlys, plays sound 0x2847462 |

### Menu/UI Functions
| Address | Name | Description |
|---------|------|-------------|
| 0x80075b84 | Menu_IncrementSelection | Increments menu selection, plays sound |
| 0x80075f94 | Menu_DecrementCounter | Decrements counter 0-4, plays sound 0x686c1c97 |

### BLB/Disc I/O
| Address | Name | Description |
|---------|------|-------------|
| 0x80020848 | BLB_ReadSectorsWrapper | Wrapper for CdBLB_ReadSectors |

### HUD Helpers
| Address | Name | Description |
|---------|------|-------------|
| 0x8002a378 | HUD_UpdateVisibilityWrapper | Simple wrapper for UpdateHUDEntityVisibility |

## Key Patterns Discovered

### Destructor Debug Symbols
Entity destructors set entity+0x18 to addresses in the 0x80010xxx range (debug symbols):
- 0x800103ec - ResourceType3 debug point
- 0x8001040c - ResourceType2 debug point
- 0x8001042c - MultiAlloc debug point
- 0x8001044c - ChildRef debug point
- 0x800104ac - Final cleanup debug point
- 0x800104ec - Simple2 debug point
- 0x8001050c - ChildEntities debug point
- 0x8001052c - RenderList debug point
- 0x800105cc - DestroyAndFree debug point
- 0x8001060c - Heap free debug point

### Platform State Machine
Platform entities use:
- PTR_PlatformRideStartUp_800a5994 - Transition to moving up state
- PTR_PlatformRideStartDown_800a599c - Transition to moving down state
- Entity fields: +0x10c (timer), +0x10e (direction flag), +0x111 (ride state)

### Animation Frame System
EntityTick_AnimationFrameAdvance shows:
- entity+0x30 = current frame index (byte)
- entity+0x31 = frame delay counter (byte)
- entity+0x1c = frame table pointer (16-byte entries)
- Calls parent callback when animation completes

## Remaining Work
**199 FUN_ functions** still need naming. Many appear to be:
- Additional destructor variants (many use identical patterns with different debug symbols)
- Entity-specific tick callbacks
- More collision handlers

The simple destructors in range 0x8002c53c-0x80030ca8 and 0x80055140-0x8005572c are nearly identical
(same code, different debug symbol address). They could be batch-named with indices.

### Notable Lookup Tables
- **DAT_8009c01c / DAT_8009c09c**: Atan lookup tables used by HomingProjectile_TrackTarget
- **DAT_8009b240 / DAT_8009b242**: Easing curve data used by EntityTick_EasedMovementInterpolation

### Interesting Render Primitives
- **Render_RotatingStarEffect**: Uses POLY_G3 (gouraud shaded triangles) with ccos/csin for rotation
- **RenderRopeSegments**: Uses POLY_GT4 (textured quads) for rope/cable segments

The Ghidra script at `/home/sam/Desktop/rename_new_functions.py` can be used to batch rename
additional functions as they are analyzed.
