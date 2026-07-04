#include "common.h"
#include "functions.h"
#include "Game/blb_records.h"

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

/* Default destruct vtable entry -- body is only 4 bytes. When dispatched
 * with type 3 (the "request kill" code used by DeferredEntityRemoval /
 * EntityRemoval / RemoveFromZOrderList) it stashes a3/a2 into the entity's
 * +0x34/+0x38 kill-request slots; for any other type returns 0 with no effect. */
INCLUDE_ASM("asm/nonmatchings/blb", EntityDestructCallback);
