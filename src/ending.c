#include "common.h"
#include "functions.h"

extern u8 *g_pBlbHeapBase;
extern void ResetGameStateInputAndContext(void);
extern u8 ENDING_ENTITY_VTABLE_20CC[] asm("D_800120CC");
extern u8 ENDING_ENTITY_VTABLE_1E54[] asm("D_80011E54");
extern u8 ENDING_ENTITY_VTABLE_1E74[] asm("D_80011E74");
extern u8 ENDING_ENTITY_VTABLE_1EB4[] asm("D_80011EB4");
extern u8 ENDING_ENTITY_VTABLE_2034[] asm("D_80012034");
extern u8 ENDING_ENTITY_VTABLE_208C[] asm("D_8001208C");
extern u8 ENDING_ENTITY_VTABLE_20AC[] asm("D_800120AC");

typedef struct EndingEntityWithState {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u32 stateValue;
} EndingEntityWithState;

typedef struct EndingSequenceEntity {
    /* 0x00 */ u8 pad00[0x5C];
    /* 0x5C */ u32 argA;
    /* 0x60 */ u8 resetValue;
    /* 0x61 */ u8 pad61[3];
    /* 0x64 */ u32 argB;
} EndingSequenceEntity;

INCLUDE_ASM("asm/nonmatchings/ending", EndingTickCallback);

INCLUDE_ASM("asm/nonmatchings/ending", TriggerEndingSequence);

INCLUDE_ASM("asm/nonmatchings/ending", EndingCreditsRevealTick);

INCLUDE_ASM("asm/nonmatchings/ending", EndingCreditsDelayTick);

INCLUDE_ASM("asm/nonmatchings/ending", EndingCreditsScrollTick);

INCLUDE_ASM("asm/nonmatchings/ending", EndingCreditsScrollTick2);

INCLUDE_ASM("asm/nonmatchings/ending", EndingCreditsCompleteTick);

INCLUDE_ASM("asm/nonmatchings/ending", EndingCreditsFadeOutTick);

void EndingEntityDestroyCallback_1E54(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = ENDING_ENTITY_VTABLE_1E54;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

void EndingEntityDestroyCallback_1E74(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = ENDING_ENTITY_VTABLE_1E74;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

u32 func_80079D74(EndingEntityWithState *e) {
    return e->stateValue;
}

INCLUDE_ASM("asm/nonmatchings/ending", func_80079D80);

INCLUDE_ASM("asm/nonmatchings/ending", EndingEntityDestroyCallback);

void func_80079DEC(void) {
}

void EndingEntityDestroyCallback_2034(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = ENDING_ENTITY_VTABLE_2034;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

void EndingEntityDestroyCallback_2034_V2(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = ENDING_ENTITY_VTABLE_2034;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

void EndingEntityDestroyCallback_2034_V3(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = ENDING_ENTITY_VTABLE_2034;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

void EndingEntityDestroyCallback_2034_V4(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = ENDING_ENTITY_VTABLE_2034;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

u32 func_80079F84(EndingEntityWithState *e) {
    return e->stateValue;
}

void func_80079F90(void) {
}

void func_80079F98(void) {
}

void func_80079FA0(void) {
}

void func_80079FA8(void) {
}

void EndingEntityDestroyCallback_2034_V5(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = ENDING_ENTITY_VTABLE_2034;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

void EndingEntityDestroyCallback_208C(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = ENDING_ENTITY_VTABLE_208C;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

void EndingEntityDestroyCallback_20AC(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = ENDING_ENTITY_VTABLE_20AC;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

void func_8007A0DC(void) {
}

void func_8007A0E4(void) {
}

void FreeEntityNoTeardown_8007a120(Entity *e, u32 size);

void EndingEntityDestroyCallback_20CC(Entity *entity, u32 flag) {
    entity->collisionVtable = ENDING_ENTITY_VTABLE_20CC;
    if (flag & 1) {
        FreeEntityNoTeardown_8007a120(entity, 0x1C);
    }
}

void FreeEntityNoTeardown_8007a120(Entity *e, u32 size) {
    FreeFromHeap(g_pBlbHeapBase, (u8 *)e, 0, 0);
}

u8 *PassThroughFunction(u8 *x) {
    return x;
}

EndingSequenceEntity *InitEndingSequence(EndingSequenceEntity *e, u32 a, u32 b) {
    e->argA = a;
    e->argB = b;
    e->resetValue = 0xFF;
    ResetGameStateInputAndContext();
    return e;
}

extern void builtin_delete(u8 *ptr);

void ConditionalDelete(u8 *p, u32 doDelete) {
    if (doDelete & 1) {
        builtin_delete(p);
    }
}

