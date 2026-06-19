#include "common.h"
#include "functions.h"

typedef struct EditorEntity {
    /* 0x00 */ u8 pad00[0x1C];
    /* 0x1C */ struct {
        u8 pad00[0x0A];
        u8 enabled;
    } *primObject;
    /* 0x20 */ u8 childDisabledFlag;
} EditorEntity;

extern u8 *g_pBlbHeapBase;
extern void FreeEntityNoTeardown_8002c7d8(Entity *e, u32 size);
extern u8 ENTITY_DESTRUCTOR_TYPE0_VTABLE[] asm("D_800105CC");
extern u8 ENTITY_DESTRUCTOR_TYPE5_VTABLE[] asm("D_800105EC");
extern u8 ENTITY_DESTRUCTOR_SIMPLE_VTABLE[] asm("D_8001060C");

INCLUDE_ASM("asm/nonmatchings/edtor", EntityDestructor_DestroyAllChildEntities);

void func_8002C528(EditorEntity *e) {
    e->childDisabledFlag = 1;
    e->primObject->enabled = 0;
}

void EntityDestructor_Type0(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = ENTITY_DESTRUCTOR_TYPE0_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

void EntityDestructor_Type1(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = ENTITY_DESTRUCTOR_TYPE0_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

void EntityDestructor_Type2(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = ENTITY_DESTRUCTOR_TYPE0_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

void EntityDestructor_Type3(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = ENTITY_DESTRUCTOR_TYPE0_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

void EntityDestructor_Type4(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = ENTITY_DESTRUCTOR_TYPE0_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

void EntityDestructor_Type5(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = ENTITY_DESTRUCTOR_TYPE5_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

void func_8002C794(void) {
}

void func_8002C79C(void) {
}

void EntityDestructor_Type6_Simple(Entity *entity, u32 flag) {
    entity->collisionVtable = ENTITY_DESTRUCTOR_SIMPLE_VTABLE;
    if (flag & 1) {
        FreeEntityNoTeardown_8002c7d8(entity, 0x1C);
    }
}

