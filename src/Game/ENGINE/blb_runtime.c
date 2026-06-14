#include "common.h"

extern void *g_pBlbHeapBase;
extern void FreeFromHeap(void *heap, void *ptr, s32 arg2, s32 arg3);
extern u8 g_EntityVtable_SimpleDestruct[];
extern u8 g_EntityVtable_LevelDestroy[];

extern u8 CdBLB_ReadSectors(u16 arg0, u16 arg1);

u8 BLB_ReadSectorsWrapper(u32 sector, u32 count) {
    return CdBLB_ReadSectors((u16)sector, (u16)count);
}

extern void InitMenuEntityWithVtable(void *entity, s32 arg);
extern void *PassThroughFunction(void *ptr);
extern void RemoveEntityFromAllLists(void *entity, void *child);
extern void RemoveEntityFromUpdateQueue(void *entity);
extern void RemoveFromRenderList(void *entity);
extern void RemoveFromTickList(void *entity, void *child);
void RemoveFromUpdateQueue(u8 *entity);
extern void RemoveFromZOrderList(void *entity);
extern void ClearEntityDefList(void *entity);
extern void FreeEntityLists(void *entity);
extern void builtin_delete(void *ptr);
extern void ConditionalDelete(void *ptr, s32 type);

void *InitEntityWithTable(void *entity) {
    InitMenuEntityWithVtable(entity, 0);
    *(s32 *)((u8 *)entity + 0x18) = (s32)g_EntityVtable_SimpleDestruct;
    PassThroughFunction((void *)((u8 *)entity + 0x84));
    return entity;
}

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/blb_runtime", LoadBLBHeader);

void DestroyEntity(u8 *entity, s32 flags) {
    *(s32 *)(entity + 0x18) = (s32)g_EntityVtable_SimpleDestruct;
    RemoveFromUpdateQueue(entity);
    RemoveFromZOrderList(entity);
    ClearEntityDefList(entity);
    FreeEntityLists(entity);
    if (*(s32 *)(entity + 0x3C) != 0) {
        builtin_delete(*(void **)(entity + 0x3C));
    }
    ConditionalDelete(entity + 0x84, 2);
    *(s32 *)(entity + 0x18) = (s32)g_EntityVtable_LevelDestroy;
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void RemoveFromUpdateQueue(u8 *entity) {
    u8 *heap = (u8 *)g_pBlbHeapBase;

    *(s16 *)(heap + 0xA08A) = 0;
    if (*(s32 *)(entity + 0x108) != 0) {
        FreeFromHeap(heap, *(void **)(entity + 0x108), 0, 0);
        *(s32 *)(entity + 0x108) = 0;
        *(s16 *)(entity + 0x104) = 0;
    }
}

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/blb_runtime", RemoveFromZOrderList);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/blb_runtime", EntityTickLoopWithCamera);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/blb_runtime", DeferredEntityRemoval);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/blb_runtime", EntityRemoval);

// lint:ok - common.h doesn't include entity.h, local definition needed
typedef struct EntityListNode {
    struct EntityListNode *next;  /* 0x00 */
    u8 *entity;                  /* 0x04 */
} EntityListNode;

typedef struct EntityListHead {
    u8 pad[0x1C];
    EntityListNode *head;        /* 0x1C */
} EntityListHead;

void EntityTickLoop(EntityListHead *list) {
    EntityListNode *node;

    node = list->head;
    while (node) {
        u8 *entity = (u8 *)node->entity;
        u8 *vtable = *(u8 **)(entity + 0x18);
        s16 offset = *(s16 *)(vtable + 0x10);
        void (*fn)(void *) = (void (*)(void *))*(s32 *)(vtable + 0x14);
        fn((void *)(entity + offset));
        node = node->next;
    }
}

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/blb_runtime", RenderEntities);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/blb_runtime", EntityDestructCallback);
