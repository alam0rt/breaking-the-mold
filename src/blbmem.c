#include "common.h"
#include "globals.h"

typedef struct BLBEntityHeader {
    u8 pad[0x18];
    void *vtable;
} BLBEntityHeader;

extern u8 D_80012758[];
extern void FreeFromHeap(void *heap, void *ptr, s32 arg2, s32 arg3);

void FreeBLBMemory(void *ptr, s32 size);

void BLBEntityDestroyCallback_2758(BLBEntityHeader *entity, u32 flags) {
    entity->vtable = D_80012758;
    if (flags & 1) {
        FreeBLBMemory(entity, 0x1C);
    }
}

void FreeBLBMemory(void *ptr, s32 size) {
    FreeFromHeap(g_pBlbHeapBase, ptr, 0, 0);
}
