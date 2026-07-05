#include "common.h"
#include "functions.h"
#include "Game/blb_records.h"
#include "Game/player_state.h"
#include "Game/callback_slot.h"

extern u8 *g_pBlbHeapBase;
extern u8 g_EntityVtable_SimpleDestruct[];
extern u8 g_EntityVtable_LevelDestroy[];

extern u8 CdBLB_ReadSectors(u16 arg0, u16 arg1);

/* u32->u16 trampoline around CdBLB_ReadSectors. Installed by LoadBLBHeader
 * as the streaming-read callback in InitLevelDataContext so the asset
 * loader can call back into the CD layer with a stable signature. */
u8 BLB_ReadSectorsWrapper(u32 sector, u32 count) {
    return CdBLB_ReadSectors((u16)sector, (u16)count);
}

extern void InitMenuEntityWithVtable(Entity *entity, s32 arg);
extern u8 *PassThroughFunction(u8 *ptr);
extern void RemoveEntityFromAllLists(Entity *entity, Entity *child);
extern void RemoveEntityFromUpdateQueue(Entity *entity);
extern void RemoveFromRenderList(Entity *entity);
void RemoveFromUpdateQueue(u8 *entity);
extern void RemoveFromZOrderList(u8 *entity);
extern void ClearEntityDefList(u8 *entity);
extern void builtin_delete(u8 *ptr);
extern void ConditionalDelete(u8 *ptr, s32 type);

/* Generic constructor for a BLB-heap-allocated entity: runs the standard
 * vtable init, patches the vtable ptr at +0x18 to g_EntityVtable_SimpleDestruct,
 * then pass-initialises the embedded sub-object at +0x84. */
BlbEntityWithSpriteSubobject *InitEntityWithTable(BlbEntityWithSpriteSubobject *entity) {
    InitMenuEntityWithVtable(&entity->base, 0);
    entity->base.collisionVtable = g_EntityVtable_SimpleDestruct;
    PassThroughFunction(&entity->spriteSubobject);
    return entity;
}

/* BLB bootstrap. Reads the first two sectors (0x1000-byte BLB header) into
 * the BLB heap, stashes the GameState ptr, writes the 0x01234567 sentinel
 * into GameState+0x12C, then hands off to InitLevelDataContext (with
 * BLB_ReadSectorsWrapper as the I/O callback) and SetSpriteTables. */
INCLUDE_ASM("asm/nonmatchings/blb", LoadBLBHeader);

/* GameState teardown. Swaps the polymorphic vtable slot (+0x18) to
 * SimpleDestruct for cleanup, drops from update / Z-order / def lists, deletes
 * the previous spawn-list buffer at +0x3C and the embedded LevelDataContext at
 * +0x84, swaps the vtable to LevelDestroy, then (if flags&1) frees the scene
 * object itself back to the BLB heap. */
void DestroyEntity(u8 *entity, s32 flags) {
    GameState *gs = (GameState *)entity;
    gs->postRenderCallbackContext = g_EntityVtable_SimpleDestruct;
    RemoveFromUpdateQueue(entity);
    RemoveFromZOrderList(entity);
    ClearEntityDefList(entity);
    FreeEntityLists(entity);
    if (gs->previous_spawn_list != NULL) {
        builtin_delete(gs->previous_spawn_list);
    }
    ConditionalDelete((u8 *)&gs->level_context, 2);
    gs->postRenderCallbackContext = g_EntityVtable_LevelDestroy;
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

/* Frees the scene's tile-render lookup table (ptr at +0x108, count at +0x104)
 * back to the BLB heap and zeroes the queue-busy latch at heap+0xA08A. Likely
 * should be FreeTileRenderState. */
void RemoveFromUpdateQueue(u8 *entity) {
    u8 *heap = (u8 *)g_pBlbHeapBase;
    GameState *gs = (GameState *)entity;

    ((BlbHeapHeader *)heap)->queueBusyLatch = 0;
    if (gs->tile_render_state_ptr != NULL) {
        FreeFromHeap(heap, gs->tile_render_state_ptr, 0, 0);
        gs->tile_render_state_ptr = NULL;
        gs->tile_render_state_count = 0;
    }
}

/* Walks this entity's per-instance Z-order array (count at +0x114, base at
 * +0x110), fires the child destruct vtable (type 3) on each, then frees the
 * array back to the BLB heap and clears the count. */
INCLUDE_ASM("asm/nonmatchings/blb", RemoveFromZOrderList);

/* Main per-frame entity tick driver (input is a GameState*). Walks the
 * update list at GameState+0x1C, dispatches each entity's tick vtable,
 * fires UpdateCameraPositionSmooth once at the player anchor (state >=
 * 0x7D0), runs DeferredEntityRemoval after each, bumps frame counter +0x10C. */
INCLUDE_ASM("asm/nonmatchings/blb", EntityTickLoopWithCamera);

/* Single-entity removal path used inside the tick loop. If the kill-request
 * fields at +0x34/+0x38 are set, drops from update / render / tick lists,
 * fires the destruct vtable (type 3), then clears the request. */
INCLUDE_ASM("asm/nonmatchings/blb", DeferredEntityRemoval);

/* Drains every entity from the update list (GameState+0x1C), firing the
 * secondary destruct vtable on each followed by the same teardown that
 * DeferredEntityRemoval performs. Used at level teardown / scene switches. */
INCLUDE_ASM("asm/nonmatchings/blb", EntityRemoval);

/* Plain (no-camera) variant of the per-frame tick driver. Iterates the
 * GameState tick list at +0x1C, fetches the tick fn at vtable+0x14 and the
 * arg-offset at vtable+0x10, and invokes tick_fn(entity + offset). */
void EntityTickLoop(GameState *gameState) {
    EntityListNode *node;

    node = gameState->tick_list_head;
    while (node) {
        u8 *entity = (u8 *)node->entity;
        BlbEntityVtable *vtable = (BlbEntityVtable *)((Entity *)entity)->collisionVtable;
        s16 offset = vtable->argOffset;
        vtable->tickFn((void *)(entity + offset));
        node = node->next;
    }
}

/* Per-frame entity render pass. If the dirty flag at +0x130 is set, copies
 * the queued backdrop RGB at +0x131..+0x133 into both GPU draw-env buffers
 * on the BLB heap (offsets 0x1D and 0x505D -- the double-buffered DRAWENVs),
 * then walks the render list at +0x20 and calls each entity's render vtable. */
INCLUDE_ASM("asm/nonmatchings/blb", RenderEntities);

extern void PlatformRideComplete(Entity *entity);
extern void PlatformRideStartUp(Entity *entity);
extern void PlatformRideStartDown(Entity *entity);

/* blb .sdata (0x800A5980..0x800A59A8): an 8-byte config header followed by the
 * platform-ride state table read by EntityDestructCallback -- three
 * {marker, callback} pairs (marker 0xFFFF0000 = -1 duration) plus an "END2"
 * sentinel and a trailing pad word. Migrated from the pooled asm sdata blob to
 * natural C definitions (sdata-under-split Phase 2). The exact D_ symbol names
 * are preserved via asm() so the unmatched EntityDestructCallback asm still
 * resolves each entry. */
u8 D_800A5980[8] asm("D_800A5980") = {0x1F, 0x63, 0x07, 0x07, 0x07, 0x07, 0x03, 0x30};
u32 D_800A5988 asm("D_800A5988") = 0xFFFF0000;
void (*D_800A598C)(Entity *) asm("D_800A598C") = PlatformRideComplete;
u32 D_800A5990 asm("D_800A5990") = 0xFFFF0000;
void (*D_800A5994)(Entity *) asm("D_800A5994") = PlatformRideStartUp;
u32 D_800A5998 asm("D_800A5998") = 0xFFFF0000;
void (*D_800A599C)(Entity *) asm("D_800A599C") = PlatformRideStartDown;
u32 D_800A59A0[2] asm("D_800A59A0") = {0x32444E45, 0x00000000};

/* Default destruct vtable entry. When dispatched with type 3 (the "request
 * kill" code used by DeferredEntityRemoval / EntityRemoval /
 * RemoveFromZOrderList) it stashes arg3/arg2 into the entity's +0x34/+0x38
 * kill-request slots; for any other type it returns 0 with no effect. */
s32 EntityDestructCallback(Entity *entity, u16 msg, u32 arg2, u32 arg3) {
    if (msg == 3) {
        *(u32 *)((u8 *)entity + 0x34) = arg3;
        *(u32 *)((u8 *)entity + 0x38) = arg2;
    }
    return 0;
}

/* Inserts `entity` into the GameState's z-sorted tick list (head at +0x1C),
 * keeping nodes ascending by Entity.allocSize (+0x10). Allocates an 8-byte EntityListNode
 * from the BLB heap for each insert. Empty-list and append-at-tail cases share
 * the node-init tail. */
void AddToZOrderList(GameState *list, Entity *entity) {
    EntityListNode *newNode;
    EntityListNode *node;
    EntityListNode *head;
    EntityListNode *prev;
    s32 align;
    s32 key;
    u8 inserted;

    head = list->tick_list_head;
    if (head == NULL) {
        newNode = (EntityListNode *)AllocateFromHeap(g_pBlbHeapBase, 8, 1, 0);
        list->tick_list_head = newNode;
        goto init;
    }
    node = head;
    prev = NULL;
    inserted = 0;
    key = entity->allocSize;
loop:
    if (node != NULL) {
        do {
            align = 8;
            if (node->entity->allocSize >= key) {
                newNode = (EntityListNode *)AllocateFromHeap(g_pBlbHeapBase, align, 1, 0);
                if (prev == NULL) {
                    list->tick_list_head = newNode;
                } else {
                    prev->next = newNode;
                }
                newNode->next = node;
                newNode->entity = entity;
                inserted = 1;
            } else {
                prev = node;
                node = node->next;
                goto loop;
            }
        } while (0);
    }
    if (inserted == 0) {
        newNode = (EntityListNode *)AllocateFromHeap(g_pBlbHeapBase, 8, 1, 0);
        prev->next = newNode;
    init:
        newNode->next = NULL;
        newNode->entity = entity;
    }
}

INCLUDE_ASM("asm/nonmatchings/blb", AddToXPositionList);

INCLUDE_ASM("asm/nonmatchings/blb", AddToUpdateQueue);

INCLUDE_ASM("asm/nonmatchings/blb", AddToEntityList);

/* Singly-linked node prepended to an object's entity-definition list (head at
 * +0x28). Each node is an 8-byte cell allocated from the BLB heap. */
typedef struct EntityDefNode {
    struct EntityDefNode *next; /* 0x0 */
    void *item;                 /* 0x4 */
} EntityDefNode;

typedef struct EntityDefListOwner {
    u8 pad[0x28];
    struct EntityDefNode *defList; /* 0x28 */
} EntityDefListOwner;

void AddToEntityDefList(EntityDefListOwner *owner, void *item) {
    EntityDefNode *n = (EntityDefNode *)AllocateFromHeap(g_pBlbHeapBase, 8, 1, 0);
    n->next = owner->defList;
    n->item = item;
    owner->defList = n;
}

INCLUDE_ASM("asm/nonmatchings/blb", AddEntityToSortedRenderList);

INCLUDE_ASM("asm/nonmatchings/blb", AddLayerToRenderList_Standard);

INCLUDE_ASM("asm/nonmatchings/blb", AddLayerToRenderList_Medium);

INCLUDE_ASM("asm/nonmatchings/blb", AddLayerToRenderList_Small);

INCLUDE_ASM("asm/nonmatchings/blb", AddEntityToBothLists);

INCLUDE_ASM("asm/nonmatchings/blb", InsertIntoZSortedRenderList);

INCLUDE_ASM("asm/nonmatchings/blb", RemoveFromTickList);

INCLUDE_ASM("asm/nonmatchings/blb", RemoveFromRenderList);

INCLUDE_ASM("asm/nonmatchings/blb", RemoveEntityFromUpdateQueue);

INCLUDE_ASM("asm/nonmatchings/blb", ChangeRenderZOrder);

INCLUDE_ASM("asm/nonmatchings/blb", RemoveEntityFromAllLists);

INCLUDE_ASM("asm/nonmatchings/blb", ClearTickList);

INCLUDE_ASM("asm/nonmatchings/blb", FreeEntityListA);

INCLUDE_ASM("asm/nonmatchings/blb", FreeEntityListB);

INCLUDE_ASM("asm/nonmatchings/blb", ClearEntityDefList);

INCLUDE_ASM("asm/nonmatchings/blb", FreeEntityLists);

INCLUDE_ASM("asm/nonmatchings/blb", CheckPointCollisionAndNotify);

INCLUDE_ASM("asm/nonmatchings/blb", DispatchEventToCollidingEntity);

INCLUDE_ASM("asm/nonmatchings/blb", BroadcastPointCollision);

INCLUDE_ASM("asm/nonmatchings/blb", BroadcastBoxCollision);

INCLUDE_ASM("asm/nonmatchings/blb", SendMessageToPlayer);

INCLUDE_ASM("asm/nonmatchings/blb", SendMessageToPlayerVariant);

INCLUDE_ASM("asm/nonmatchings/blb", TestPointAgainstEntities);

INCLUDE_ASM("asm/nonmatchings/blb", CheckBoxCollision);

INCLUDE_ASM("asm/nonmatchings/blb", CameraFollowTargetTick);

INCLUDE_ASM("asm/nonmatchings/blb", UpdateCameraPositionSmooth);

INCLUDE_ASM("asm/nonmatchings/blb", SetCameraPositionDirect);

INCLUDE_ASM("asm/nonmatchings/blb", GetTileAttributeAtPosition);

INCLUDE_ASM("asm/nonmatchings/blb", SpawnOnScreenEntities);

INCLUDE_ASM("asm/nonmatchings/blb", CleanupDeadEntities);

INCLUDE_ASM("asm/nonmatchings/blb", IsEntityOffScreen_EntityLoop);

INCLUDE_ASM("asm/nonmatchings/blb", CheckTriggerZoneCollision);

/* Marks the level background colour as valid (+0x130 = 1) and copies the RGB
 * triple from the level's tile header into the entity at +0x131..+0x133. */
void LoadBGColorFromTileHeader(u8 *entity) {
    u8 *hdr = GetTileHeaderPtr((LevelDataContext *)(entity + 0x84));
    entity[0x130] = 1;
    entity[0x131] = hdr[0];
    entity[0x132] = hdr[1];
    entity[0x133] = hdr[2];
}

INCLUDE_ASM("asm/nonmatchings/blb", LoadSecondaryColorFromTileHeader);

/* Fetches the tile spawn coordinates for this level and converts them to
 * pixel-space entity spawn position: x = (tileX << 4) + 8 (tile centre),
 * y = (tileY << 4) + 15 (tile bottom). */
void InitPlayerSpawnPosition(u8 *entity) {
    u16 pos[2];
    GetSpawnPosition((u8 *)pos, (LevelDataContext *)(entity + 0x84));
    *(s16 *)(entity + 0x116) = (pos[0] << 4) + 8;
    *(s16 *)(entity + 0x118) = (pos[1] << 4) + 0xF;
}

INCLUDE_ASM("asm/nonmatchings/blb", InitLayersAndTileState);

INCLUDE_ASM("asm/nonmatchings/blb", InitTileAttributeState);

INCLUDE_ASM("asm/nonmatchings/blb", LoadEntitiesFromAsset501);

INCLUDE_ASM("asm/nonmatchings/blb", InitAnimatedTileEntities);

INCLUDE_ASM("asm/nonmatchings/blb", AddPreInitEntitiesToList);

INCLUDE_ASM("asm/nonmatchings/blb", LoadTileDataToVRAM);

extern u8 *GetTilemapLayerDataPtr(LevelDataContext *ctx, u16 layer_idx);
extern u16 GetTilemapLayerWidth(LevelDataContext *ctx, u16 layer_idx);

/* Resolves the spawn tilemap layer: writes the layer's data pointer to *outPtr
 * and its width to *outWidth, reading from the level context embedded at +0x84. */
void GetEntitySpawnData(u8 *base, u32 layerIndex, u8 **outPtr, u16 *outWidth) {
    *outPtr = GetTilemapLayerDataPtr((LevelDataContext *)(base + 0x84), (u16)layerIndex);
    *outWidth = GetTilemapLayerWidth((LevelDataContext *)(base + 0x84), (u16)layerIndex);
}

extern void CopyTilemapLayerIndex(void *dst, void *src, u16 layerIndex);

/* Copies the tilemap layer index for `layerIndex` from the level's tilemap
 * region (src + 0x84) into the destination entity, returning it. */
void *SetEntitySpawnData(void *dst, void *src, u32 layerIndex) {
    CopyTilemapLayerIndex(dst, (u8 *)src + 0x84, (u16)layerIndex);
    return dst;
}

/* Maps a facing/direction code to the horizontal glide-spawn offset stored in
 * GameState.glide_boss_state_x: 0 -> centred, 1 -> left (-0x30), 2 -> right
 * (+0x30). Any other value leaves the field untouched. */
void SetSpawnOffsetGroup1(GameState *gs, u8 dir) {
    switch (dir) {
    case 0:
        gs->glide_boss_state_x = 0;
        break;
    case 1:
        gs->glide_boss_state_x = -0x30;
        break;
    case 2:
        gs->glide_boss_state_x = 0x30;
        break;
    }
}

/* Vertical counterpart of SetSpawnOffsetGroup1: maps the direction code to
 * GameState.glide_boss_state_y (0 / -0x30 / +0x30). */
void SetSpawnOffsetGroup2(GameState *gs, u8 dir) {
    switch (dir) {
    case 0:
        gs->glide_boss_state_y = 0;
        break;
    case 1:
        gs->glide_boss_state_y = -0x30;
        break;
    case 2:
        gs->glide_boss_state_y = 0x30;
        break;
    }
}

extern u8 GetCurrentLevelAssetIndex(LevelDataContext *ctx);
extern char *GetCurrentLevelDisplayName(LevelDataContext *ctx);

/* Thin GameState wrapper: resolves the asset index of the current level from
 * the LevelDataContext embedded at GameState+0x84. */
u8 GetLevelAssetIndexFromGameState(GameState *gs) {
    return GetCurrentLevelAssetIndex(&gs->level_context);
}

/* GameState wrapper: fetches the current level's display name string from the
 * embedded LevelDataContext. */
char *GetLevelNameFromGameState(GameState *gs) {
    return GetCurrentLevelDisplayName(&gs->level_context);
}

INCLUDE_ASM("asm/nonmatchings/blb", GetStageIndexFromGameState);

/* Forward decl: the size argument is currently ignored by the callee body. */
extern void FreeEntityNoTeardown_80025964(u8 *entity, s32 size);

/* Level-teardown destructor: installs the level-destroy vtable at +0x18, then
 * (when the caller requests it via bit 0) frees the entity's storage. */
void EntityDestructor_Simple2(Entity *e, u32 flags) {
    *(u8 **)((u8 *)e + 0x18) = g_EntityVtable_LevelDestroy;
    if (flags & 1) {
        FreeEntityNoTeardown_80025964((u8 *)e, 0x1C);
    }
}

/* Returns an entity's backing storage to the BLB heap without running any
 * teardown/destructor logic first. `size` is accepted for call-site symmetry
 * but is not consulted by the free path. */
void FreeEntityNoTeardown_80025964(u8 *entity, s32 size) {
    FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
}

extern void InitEntityDataPointers(void *entity, void *dataBase);

/* Zeroes a HUD entity's header counters and installs its data pointers off the
 * fixed HUD data base at 0x80780000. Returns the entity. */
typedef struct HUDEntityHeader {
    s16 field_0x0; /* 0x0 */
    s16 field_0x2; /* 0x2 */
    u8  field_0x4; /* 0x4 */
    u8  field_0x5; /* 0x5 */
} HUDEntityHeader;

HUDEntityHeader *InitHUDEntity(HUDEntityHeader *e) {
    e->field_0x0 = 0;
    e->field_0x2 = 0;
    e->field_0x4 = 0;
    e->field_0x5 = 0;
    InitEntityDataPointers(e, (void *)0x80780000);
    return e;
}

INCLUDE_ASM("asm/nonmatchings/blb", UpdateInputState);

INCLUDE_ASM("asm/nonmatchings/blb", InitEntityDataPointers);

INCLUDE_ASM("asm/nonmatchings/blb", EnableDemoPlaybackMode);

INCLUDE_ASM("asm/nonmatchings/blb", CRT_InitStaticData1);

INCLUDE_ASM("asm/nonmatchings/blb", BuildPasswordFromPlayerState);

INCLUDE_ASM("asm/nonmatchings/blb", DecodePassword);

INCLUDE_ASM("asm/nonmatchings/blb", initPlayerState);

/* Zeroes the player's hamster collectible counter (PlayerState+0x1A). */
void ClearHamsterCount(PlayerState *p) {
    p->hamster_count = 0;
}

/* NOTE: ResetPlayerCollectibles accesses the g_pPlayerState pointer at
 * D_800A597C gp-relatively. That symbol is splat-owned initialized .sdata, so
 * cc1 can only emit gp-relative loads for it once the data definition is
 * migrated from asm/data into this TU. Left as asm until that migration. */
INCLUDE_ASM("asm/nonmatchings/blb", ResetPlayerCollectibles);

INCLUDE_ASM("asm/nonmatchings/blb", InitializePlayerState);

INCLUDE_ASM("asm/nonmatchings/blb", MarkLevelCompleteAndClearCollectibles);

/* On player death: clears the active powerup bitfield and boss HP, then spends a
 * life (guarded so it never underflows past zero). */
void DecrementPlayerLives(PlayerState *p) {
    u8 lives = p->lives;
    p->powerup_flags = 0;
    p->boss_hp = 0;
    if (lives != 0) {
        p->lives = lives - 1;
    }
}

INCLUDE_ASM("asm/nonmatchings/blb", AddSwirlys);

INCLUDE_ASM("asm/nonmatchings/blb", AddPlayerLives);

INCLUDE_ASM("asm/nonmatchings/blb", AddPlayerOrbs);

INCLUDE_ASM("asm/nonmatchings/blb", AddPhoenixHands);

INCLUDE_ASM("asm/nonmatchings/blb", AddPhartHeads);

INCLUDE_ASM("asm/nonmatchings/blb", AddUniverseEnemas);

INCLUDE_ASM("asm/nonmatchings/blb", Add1970Icons);

INCLUDE_ASM("asm/nonmatchings/blb", AwardSwirlyQsAndHamsters);

INCLUDE_ASM("asm/nonmatchings/blb", AddSuperWillies);

INCLUDE_ASM("asm/nonmatchings/blb", ResetPlayerUnlocksByLevel);

INCLUDE_ASM("asm/nonmatchings/blb", InitLevelTitleEntity);

INCLUDE_ASM("asm/nonmatchings/blb", EntityTick_ScaleFadeOut);

INCLUDE_ASM("asm/nonmatchings/blb", EntityTick_ScaleFadeIn);

INCLUDE_ASM("asm/nonmatchings/blb", SetEntityScaleFadeOut);

INCLUDE_ASM("asm/nonmatchings/blb", ResetEntityScaleState);

INCLUDE_ASM("asm/nonmatchings/blb", SetEntityScaleFadeIn);

INCLUDE_ASM("asm/nonmatchings/blb", InitCountdownTimerEntity);

INCLUDE_ASM("asm/nonmatchings/blb", EntityTick_PlatformRideIdle);

INCLUDE_ASM("asm/nonmatchings/blb", PlatformInterpolatePosition);

INCLUDE_ASM("asm/nonmatchings/blb", PlatformRideStartUp);

INCLUDE_ASM("asm/nonmatchings/blb", PlatformRideStartDown);

INCLUDE_ASM("asm/nonmatchings/blb", PlatformRideComplete);

INCLUDE_ASM("asm/nonmatchings/blb", InitDigitDisplayEntity);

INCLUDE_ASM("asm/nonmatchings/blb", EntityTick_DigitDisplayUpdate);

INCLUDE_ASM("asm/nonmatchings/blb", UpdateDigitDisplay);

INCLUDE_ASM("asm/nonmatchings/blb", InitHUDItemEntity);

INCLUDE_ASM("asm/nonmatchings/blb", EntityTick_HUDItemUpdate);

/* Toggles a HUD sub-entity's hidden flag (byte +0x37 of the pointer at +0x34):
 * shows it (1) while the current item index is still within the item count,
 * hides it (0) otherwise. */
void UpdateHUDItemVisibility(u8 *hud) {
    if (hud[0x114] >= hud[0x116] + 1) {
        *(*(u8 **)(hud + 0x34) + 0x37) = 0;
    } else {
        *(*(u8 **)(hud + 0x34) + 0x37) = 1;
    }
}

INCLUDE_ASM("asm/nonmatchings/blb", InitEntity_8c510186);

INCLUDE_ASM("asm/nonmatchings/blb", PlatformOscillateWithBobbingTick);

INCLUDE_ASM("asm/nonmatchings/blb", InitTimerDisplayEntity);

INCLUDE_ASM("asm/nonmatchings/blb", PlatformOscillateWithRotationTick);

INCLUDE_ASM("asm/nonmatchings/blb", InitFadeOverlayEntity);

INCLUDE_ASM("asm/nonmatchings/blb", EntityDestructor_RemoveFromRenderList);

INCLUDE_ASM("asm/nonmatchings/blb", TriggerFadeTransition);

INCLUDE_ASM("asm/nonmatchings/blb", CreateMenuEntities);

/* .data migration (Phase 4): pooled-blob island carved to this TU's .o(.data). */
u8 D_8009B038[60] asm("D_8009B038") = {
    0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x01, 0x00,
    0x01, 0x00, 0xFF, 0xFF, 0xFE, 0xFF, 0x00, 0x00,
    0x02, 0x00, 0x02, 0x00, 0x01, 0x00, 0xFF, 0xFF,
    0xFD, 0xFF, 0xFE, 0xFF, 0x00, 0x00, 0x02, 0x00,
    0x04, 0x00, 0x02, 0x00, 0x00, 0x00, 0xFE, 0xFF,
    0xFD, 0xFF, 0xFA, 0xFF, 0xFF, 0xFF, 0x01, 0x00,
    0x04, 0x00, 0x08, 0x00, 0x02, 0x00, 0xFF, 0xFF,
    0xFE, 0xFF, 0xFB, 0xFF,
};
u8 D_8009B074[72] asm("D_8009B074") = {
    0x00, 0x00, 0x01, 0x00, 0x00, 0x80, 0x01, 0x00,
    0x00, 0x80, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00,
    0x00, 0x00, 0x02, 0x00, 0x00, 0x80, 0x02, 0x00,
    0x00, 0x80, 0x02, 0x00, 0x00, 0x00, 0x03, 0x00,
    0x00, 0x00, 0x03, 0x00, 0x00, 0x80, 0x03, 0x00,
    0x00, 0x00, 0x04, 0x00, 0x00, 0x80, 0x04, 0x00,
    0x00, 0x00, 0x05, 0x00, 0x00, 0x80, 0x05, 0x00,
    0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x07, 0x00,
    0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x09, 0x00,
};
u8 D_8009B0BC[72] asm("D_8009B0BC") = {
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00,
    0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x05, 0x00,
    0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x0A, 0x00,
    0x00, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x0A, 0x00,
    0x00, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x0A, 0x00,
    0x00, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x0A, 0x00,
    0x00, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x0A, 0x00,
    0x00, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x0A, 0x00,
    0x00, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x0A, 0x00,
};
u8 D_8009B104[64] asm("D_8009B104") = {
    0x00, 0x00, 0x01, 0x00, 0x00, 0x80, 0x01, 0x00,
    0x00, 0x00, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00,
    0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x03, 0x00,
    0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x04, 0x00,
    0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x05, 0x00,
    0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x06, 0x00,
    0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x07, 0x00,
    0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x08, 0x00,
};
