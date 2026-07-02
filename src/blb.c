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

/* Entity destructor. Swaps the vtable to SimpleDestruct for cleanup, drops
 * from update / Z-order / def lists, deletes the helper buffer at +0x3C and
 * the embedded sub-object at +0x84, swaps the vtable to LevelDestroy, then
 * (if flags&1) frees the entity itself back to the BLB heap. */
void DestroyEntity(u8 *entity, s32 flags) {
    BlbEntity *e = (BlbEntity *)entity;
    e->vtable = g_EntityVtable_SimpleDestruct;
    RemoveFromUpdateQueue(entity);
    RemoveFromZOrderList(entity);
    ClearEntityDefList(entity);
    FreeEntityLists(entity);
    if (e->helperBuffer != NULL) {
        builtin_delete(e->helperBuffer);
    }
    ConditionalDelete(entity + 0x84, 2);
    e->vtable = g_EntityVtable_LevelDestroy;
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

/* Frees this entity's per-instance tick-queue (ptr at +0x108, count at
 * +0x104) back to the BLB heap and zeroes the queue-busy latch at
 * heap+0xA08A. Likely should be FreeEntityTickQueue: same simple name as
 * the engine-wide RemoveEntityFromUpdateQueue @ 0x80021E50. */
void RemoveFromUpdateQueue(u8 *entity) {
    u8 *heap = (u8 *)g_pBlbHeapBase;
    BlbEntity *e = (BlbEntity *)entity;

    ((BlbHeapHeader *)heap)->queueBusyLatch = 0;
    if (e->tickQueue != NULL) {
        FreeFromHeap(heap, e->tickQueue, 0, 0);
        e->tickQueue = NULL;
        e->tickQueueCount = 0;
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

/* Plain (no-camera) variant of the per-frame tick driver. Iterates an
 * EntityListHead's linked list, fetches the tick fn at vtable+0x14 and the
 * arg-offset at vtable+0x10, and invokes tick_fn(entity + offset). */
void EntityTickLoop(EntityListHead *list) {
    EntityListNode *node;

    node = list->head;
    while (node) {
        u8 *entity = (u8 *)node->entity;
        BlbEntityVtable *vtable = (BlbEntityVtable *)((BlbEntity *)entity)->vtable;
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

/* Default destruct vtable entry -- body is only 4 bytes. When dispatched
 * with type 3 (the "request kill" code used by DeferredEntityRemoval /
 * EntityRemoval / RemoveFromZOrderList) it stashes a3/a2 into the entity's
 * +0x34/+0x38 kill-request slots; for any other type returns 0 with no effect. */
INCLUDE_ASM("asm/nonmatchings/blb", EntityDestructCallback);
