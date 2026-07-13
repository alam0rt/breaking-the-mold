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
/* Single-entity removal path used inside the tick loop. If the kill-request
 * fields at +0x34/+0x38 are set, drops from update / render / tick lists,
 * fires the destruct vtable (type 3), then clears the request. */
void DeferredEntityRemoval(u8 *e) {
    register u8 *req PSX_REG("$5"); /* pin: load lands directly in the a1 arg */

    req = *(u8 **)(e + 0x34);
    if (req == NULL) {
        return;
    }
    if (*(s32 *)(e + 0x38) == 0) {
        RemoveEntityFromAllLists((Entity *)e, (Entity *)req);
        *(u32 *)(e + 0x34) = 0;
    } else {
        RemoveEntityFromUpdateQueue(e, req);
        if (*(s32 *)(e + 0x38) != 1) {
            RemoveFromRenderList(e, *(void **)(e + 0x38));
        }
        RemoveFromTickList(e, *(void **)(e + 0x34));
        {
            u8 *req2 = *(u8 **)(e + 0x34); /* unpinned: colors $v1 */
            if (req2 != NULL) {
                u8 *vt = *(u8 **)(req2 + 0x18);
                s16 ofs = *(s16 *)(vt + 8);
                void (*fn)(u8 *, s32) = *(void (**)(u8 *, s32))(vt + 0xC);
                fn(req2 + ofs, 3);
            }
        }
        *(u32 *)(e + 0x34) = 0;
    }
    *(u32 *)(e + 0x38) = 0;
}

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

/* Unlink the first tick-list node (head at GameState+0x1C; node = {next,
 * entity}) whose entity matches, free the 8-byte node. Returns 1 on hit. */
s32 RemoveFromTickList(u8 *gs, void *target) {
    u8 *prev = NULL;
    u8 *n = *(u8 **)(gs + 0x1C);

    while (n != NULL) {
        if (*(void **)(n + 4) == target) {
            if (prev == NULL) {
                *(u8 **)(gs + 0x1C) = *(u8 **)n;
            } else {
                *(u8 **)prev = *(u8 **)n;
            }
            FreeFromHeap(g_pBlbHeapBase, n, 8, 0);
            return 1;
        }
        prev = n;
        n = *(u8 **)n;
    }
    return 0;
}

/* Same unlink on the render list (head at GameState+0x20). */
s32 RemoveFromRenderList(u8 *gs, void *target) {
    u8 *prev = NULL;
    u8 *n = *(u8 **)(gs + 0x20);

    while (n != NULL) {
        if (*(void **)(n + 4) == target) {
            if (prev == NULL) {
                *(u8 **)(gs + 0x20) = *(u8 **)n;
            } else {
                *(u8 **)prev = *(u8 **)n;
            }
            FreeFromHeap(g_pBlbHeapBase, n, 8, 0);
            return 1;
        }
        prev = n;
        n = *(u8 **)n;
    }
    return 0;
}

/* Void variant on the update queue (head at +0x24). */
void RemoveEntityFromUpdateQueue(u8 *gs, void *target) {
    u8 *prev = NULL;
    u8 *n = *(u8 **)(gs + 0x24);

    while (n != NULL) {
        if (*(void **)(n + 4) == target) {
            if (prev == NULL) {
                *(u8 **)(gs + 0x24) = *(u8 **)n;
            } else {
                *(u8 **)prev = *(u8 **)n;
            }
            FreeFromHeap(g_pBlbHeapBase, n, 8, 0);
            return;
        }
        prev = n;
        n = *(u8 **)n;
    }
}

INCLUDE_ASM("asm/nonmatchings/blb", ChangeRenderZOrder);

INCLUDE_ASM("asm/nonmatchings/blb", RemoveEntityFromAllLists);

INCLUDE_ASM("asm/nonmatchings/blb", ClearTickList);

INCLUDE_ASM("asm/nonmatchings/blb", FreeEntityListA);

INCLUDE_ASM("asm/nonmatchings/blb", FreeEntityListB);

/* Frees every node of the entity-def list at entity+0x28: each node's payload
 * (+0x4) first, then the 8-byte node itself, advancing the list head. */
void ClearEntityDefList(u8 *entity) {
    while (*(u8 **)(entity + 0x28) != NULL) {
        u8 *node = *(u8 **)(entity + 0x28);
        u8 *heap;
        u8 *next;
        FreeFromHeap(g_pBlbHeapBase, *(u8 **)(node + 4), 0, 0);
        /* heap temp first: sinks the list-head store into the jal delay slot */
        heap = g_pBlbHeapBase;
        next = *(u8 **)node;
        *(u8 **)(entity + 0x28) = next;
        FreeFromHeap(heap, node, 8, 0);
    }
}

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

/* Look up the collision-attribute byte for the tile containing world pixel
 * (px,py): convert to tile coords, offset by the map origin (+0x6C/+0x6E),
 * bounds-check against the map size (+0x70/+0x72), and index the attribute
 * grid at +0x68. Returns 0 when out of bounds or no grid is loaded. */
s32 GetTileAttributeAtPosition(u8 *gs, s16 px, s16 py) {
    u8 *attr;
    u8 *row;
    s32 sx, sy;
    s16 tx, ty, w;

    attr = *(u8 **)(gs + 0x68);
    if (attr == NULL) {
        goto out_of_bounds;
    }
    sx = px >> 4;
    sy = py >> 4;
    tx = sx - *(u16 *)(gs + 0x6C);
    ty = sy - *(u16 *)(gs + 0x6E);
    if (tx < 0) {
        goto out_of_bounds;
    }
    w = *(s16 *)(gs + 0x70);
    if (tx >= w) {
        goto out_of_bounds;
    }
    if (ty < 0) {
        return 0;
    }
    if (ty >= *(s16 *)(gs + 0x72)) {
        goto out_of_bounds;
    }
    row = attr + ty * w;
    return row[tx];
out_of_bounds:
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/blb", SpawnOnScreenEntities);

INCLUDE_ASM("asm/nonmatchings/blb", CleanupDeadEntities);

/* Screen-bounds cull test: returns 1 if the entity's world position
 * (+0x44/+0x46) is more than 16px outside the box (x1,y1,x2,y2 s16 quad),
 * using the screen w/h pair at *g_pBlbHeapBase for the far edges. */
s32 IsEntityOffScreen_EntityLoop(u8 *e, s16 *box) {
    s32 result;
    s16 x, y;
    s16 *scr;

    result = 0;
    x = *(s16 *)(e + 0x44);
    if (box[2] < x - 0x10) goto offscreen;
    scr = (s16 *)g_pBlbHeapBase;
    if (x + scr[0] + 0x10 < box[0]) goto offscreen;
    y = *(s16 *)(e + 0x46);
    if (box[3] < y - 0x10) goto offscreen;
    if (y + scr[1] + 0x10 < box[1]) {
    offscreen:
        result = 1;
    }
    return result;
}

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

extern u8 *GetSecondaryColorPtr(LevelDataContext *ctx);

/* Copies the level's secondary RGB triple from the tile header (via
 * GetSecondaryColorPtr) into the entity at +0x127..+0x129. Sibling of
 * LoadBGColorFromTileHeader, but with no "valid" flag byte. */
void LoadSecondaryColorFromTileHeader(u8 *entity) {
    u8 *ptr = GetSecondaryColorPtr((LevelDataContext *)(entity + 0x84));
    entity[0x127] = ptr[0];
    entity[0x128] = ptr[1];
    entity[0x129] = ptr[2];
}

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

extern void InitEntityDataPointers(InputState *in, void *dataBase);

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
    InitEntityDataPointers((InputState *)e, (void *)0x80780000);
    return e;
}

INCLUDE_ASM("asm/nonmatchings/blb", UpdateInputState);

/* Aim the InputState's replay pointers at a demo/replay data base: frame
 * count word at +0, RLE entry array at +4. (Also used by InitHUDEntity —
 * the "HUDEntityHeader" there is really this InputState.) */
void InitEntityDataPointers(InputState *in, void *dataBase) {
    in->pFrameCount = dataBase;
    in->pReplayBuffer = (u8 *)dataBase + 4;
}

/* Toggle demo input RECORDING. Ignored while playback is active. Enabling
 * zeroes the replay frame count and resets the stream cursor. */
void EnableDemoRecordingMode(InputState *in, u8 enable) {
    if (in->playback_active == 0) {
        in->record_active = enable;
        if (enable != 0) {
            in->playback_active = 0;
            *(u16 *)in->pFrameCount = 0;
            in->playback_index = 0;
            in->playback_timer = 0;
        }
    }
}

/* Toggle demo input PLAYBACK. Re-enabling while already active is a no-op.
 * Enabling stops recording, resets the stream cursor and primes the first
 * RLE entry's duration. */
void EnableDemoPlaybackMode(InputState *in, u8 enable) {
    if (in->playback_active == 0 || enable == 0) {
        in->playback_active = enable;
        if (enable != 0) {
            in->record_active = 0;
            in->playback_index = 0;
            in->playback_timer = *(u16 *)((u8 *)in->pReplayBuffer + 2);
        }
    }
}

INCLUDE_ASM("asm/nonmatchings/blb", CRT_InitStaticData1);

INCLUDE_ASM("asm/nonmatchings/blb", BuildPasswordFromPlayerState);

INCLUDE_ASM("asm/nonmatchings/blb", DecodePassword);

/* Reset a PlayerState block to new-game defaults: 1 life-ish flags at +0/+1,
 * 5 at +0x11 (health?), everything else zeroed, +0x10 = 1, and the 10-byte
 * array at +6 cleared. Twin of InitializePlayerState (byte-identical body). */
void initPlayerState(u8 *ps) {
    s16 i;

    ps[0] = 1;
    ps[1] = 1;
    ps[0x11] = 5;
    ps[0x12] = 0;
    ps[0x14] = 0;
    ps[0x15] = 0;
    ps[0x16] = 0;
    ps[0x1C] = 0;
    ps[0x13] = 0;
    ps[0x19] = 0;
    ps[0x1A] = 0;
    ps[0x1B] = 0;
    ps[0x17] = 0;
    ps[0x18] = 0;
    ps[0x1D] = 0;
    ps[0x10] = 1;
    *(s16 *)(ps + 2) = 0;
    ps[4] = 0;
    ps[5] = 0;
    i = 0;
    do {
        /* addr temp keeps the addu base-first (a0 + i) */
        u8 *p = &ps[i];
        p[6] = 0;
        i++;
    } while (i < 10);
}

/* Zeroes the player's hamster collectible counter (PlayerState+0x1A). */
void ClearHamsterCount(PlayerState *p) {
    p->hamster_count = 0;
}

/* NOTE: ResetPlayerCollectibles accesses the g_pPlayerState pointer at
 * D_800A597C gp-relatively. That symbol is splat-owned initialized .sdata, so
 * cc1 can only emit gp-relative loads for it once the data definition is
 * migrated from asm/data into this TU. Left as asm until that migration. */
INCLUDE_ASM("asm/nonmatchings/blb", ResetPlayerCollectibles);

/* Byte-identical twin of initPlayerState (the binary carries two copies). */
void InitializePlayerState(u8 *ps) {
    s16 i;

    ps[0] = 1;
    ps[1] = 1;
    ps[0x11] = 5;
    ps[0x12] = 0;
    ps[0x14] = 0;
    ps[0x15] = 0;
    ps[0x16] = 0;
    ps[0x1C] = 0;
    ps[0x13] = 0;
    ps[0x19] = 0;
    ps[0x1A] = 0;
    ps[0x1B] = 0;
    ps[0x17] = 0;
    ps[0x18] = 0;
    ps[0x1D] = 0;
    ps[0x10] = 1;
    *(s16 *)(ps + 2) = 0;
    ps[4] = 0;
    ps[5] = 0;
    i = 0;
    do {
        /* addr temp keeps the addu base-first (a0 + i) */
        u8 *p = &ps[i];
        p[6] = 0;
        i++;
    } while (i < 10);
}

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

/* Per-tick eased platform move (render-slot callback): counter at +0x10B,
 * duration at +0x10A. First half approaches with delta>>k, second half with
 * delta - (delta>>k); when the counter passes the duration, snap to the
 * target Y (+0x100) and fire the queued-slot callback. */
void PlatformInterpolatePosition(u8 *e) {
    s32 raw;
    u8 next;
    u32 dur;
    s16 delta;

    raw = e[0x10B] + 1;
    next = raw;
    dur = e[0x10A];
    e[0x10B] = raw;
    if (dur < next) {
        *(s16 *)(e + 0x6A) = *(u16 *)(e + 0x100);
        EntityProcessCallbackQueue((Entity *)e);
    } else {
        dur >>= 1; /* half; hoisted into the branch delay slot */
        if (next <= dur) {
            s32 k = dur - next + 1;
            *(s16 *)(e + 0x6A) = *(u16 *)(e + 0x102) + (*(s16 *)(e + 0x104) >> k);
        } else {
            s32 k = next - dur + 1;
            delta = *(s16 *)(e + 0x104);
            *(s16 *)(e + 0x6A) = *(u16 *)(e + 0x102) + (delta - (delta >> k));
        }
    }
}

/* Begin a platform ride toward the "up" anchor (+0x106): record start/target
 * Y, install PlatformInterpolatePosition on the render slot and
 * PlatformRideComplete on the queued slot, direction flag (+0x10E) = 1. */
void PlatformRideStartUp(Entity *entity) {
    PadSlot slot;
    s16 m1;
    /* Pins reproduce the target's register file: the two worldY reads stay
     * separate loads (hard regs block the cse merge) and both callback
     * addresses materialise up front in $a3/$t0. */
    register u16 y1 PSX_REG("$5");
    register u16 y2 PSX_REG("$6");
    register void (*fnI)() PSX_REG("$7");
    register void (*fnD)() PSX_REG("$8");
    register u16 tgt PSX_REG("$3");
    u8 *e = (u8 *)entity;

    fnI = PlatformInterpolatePosition;
    fnD = (void (*)())PlatformRideComplete;
    y1 = *(u16 *)(e + 0x6A);
    /* coalesce barrier: force the second worldY read to stay a real load */
    __asm__ volatile("" : "=r"(y1) : "0"(y1));
    tgt = *(u16 *)(e + 0x106);
    y2 = *(u16 *)(e + 0x6A);
    e[0x10F] = 0;
    e[0x110] = 0;
    e[0x10A] = 0x14;
    e[0x10B] = 0;
    /* fence: keep the start-Y store from hoisting above this pair */
    do {
        *(s16 *)(e + 0x100) = tgt;
        tgt -= y2;
    } while (0);
    *(s16 *)(e + 0x102) = y1;
    *(s16 *)(e + 0x104) = tgt;
    m1 = -1;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fnI;
    *(CallbackSlot *)(e + 0x1C) = slot.s;
    e[0x10E] = 1;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fnD;
    *(CallbackSlot *)(e + 0x98) = slot.s;
}

/* Mirror of PlatformRideStartUp toward the "down" anchor (+0x108);
 * direction flag (+0x10E) = 0. */
void PlatformRideStartDown(Entity *entity) {
    PadSlot slot;
    s16 m1;
    register u16 y1 PSX_REG("$5");
    register u16 y2 PSX_REG("$6");
    register void (*fnI)() PSX_REG("$7");
    register void (*fnD)() PSX_REG("$8");
    register u16 tgt PSX_REG("$3");
    u8 *e = (u8 *)entity;

    fnI = PlatformInterpolatePosition;
    fnD = (void (*)())PlatformRideComplete;
    y1 = *(u16 *)(e + 0x6A);
    __asm__ volatile("" : "=r"(y1) : "0"(y1));
    tgt = *(u16 *)(e + 0x108);
    y2 = *(u16 *)(e + 0x6A);
    e[0x10A] = 0x14;
    m1 = -1;
    e[0x10F] = 0;
    e[0x110] = 0;
    e[0x10B] = 0;
    /* fence: keep the start-Y store from hoisting above this pair */
    do {
        *(s16 *)(e + 0x100) = tgt;
        tgt -= y2;
    } while (0);
    *(s16 *)(e + 0x102) = y1;
    *(s16 *)(e + 0x104) = tgt;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fnI;
    *(CallbackSlot *)(e + 0x1C) = slot.s;
    e[0x10E] = 0;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fnD;
    *(CallbackSlot *)(e + 0x98) = slot.s;
}

/* Default "ride complete / idle" state for platform-ride entities (installed
 * via D_800A598C). Clears the render FSM slot (block-copy of a zeroed
 * CallbackSlot into renderMarker/renderCallback so the entity stops advancing
 * its ride animation), then raises two "settled" bytes in the extended entity
 * region (offsets 0x10E-0x110 — the struct's field names there belong to other
 * overlaid entity types, so accessed by raw offset). PaddedSlotPair pins the
 * 0x18 leaf frame (slot at sp+4). */
void PlatformRideComplete(Entity *entity) {
    PaddedSlotPair slot;

    slot.s[0].markerLo = 0;
    slot.s[0].markerHi = 0;
    slot.s[0].fn = NULL;
    *(CallbackSlot *)&entity->renderMarker = slot.s[0];
    *((u8 *)entity + 0x10F) = 1;
    if (*((u8 *)entity + 0x10E) == 0) {
        *((u8 *)entity + 0x110) = 1;
    }
}

/* Three tiny UNREFERENCED helpers (dead code) that trailed PlatformRideComplete
 * in its pre-split asm blob — same platform-ride field cluster (+0x10C timer,
 * +0x110/+0x111 flags). No callers or data refs anywhere in the ROM (Ghidra +
 * binary ptr-scan agree), so they keep splat's anonymous func_ names. */
INCLUDE_ASM("asm/nonmatchings/blb", func_80027210);

INCLUDE_ASM("asm/nonmatchings/blb", func_80027234);

INCLUDE_ASM("asm/nonmatchings/blb", func_80027240);

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
