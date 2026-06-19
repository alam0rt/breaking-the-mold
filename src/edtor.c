#include "common.h"
#include "functions.h"

extern void *g_pBlbHeapBase;
extern void FreeEntityNoTeardown_8002c7d8(void *e, u32 size);
extern void *D_800105CC;
extern void *D_800105EC;
extern void *D_8001060C;

INCLUDE_ASM("asm/nonmatchings/edtor", EntityDestructor_DestroyAllChildEntities);

void func_8002C528(void *e) {
    u8 *p = *(u8 **)((u8 *)e + 0x1C);
    *(u8 *)((u8 *)e + 0x20) = 1;
    p[0xA] = 0;
}

void EntityDestructor_Type0(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &D_800105CC;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestructor_Type1(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &D_800105CC;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestructor_Type2(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &D_800105CC;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestructor_Type3(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &D_800105CC;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestructor_Type4(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &D_800105CC;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestructor_Type5(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &D_800105EC;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void func_8002C794(void) {
}

void func_8002C79C(void) {
}

void EntityDestructor_Type6_Simple(void *entity, u32 flag) {
    ((Entity *)entity)->collisionVtable = &D_8001060C;
    if (flag & 1) {
        FreeEntityNoTeardown_8002c7d8(entity, 0x1C);
    }
}

