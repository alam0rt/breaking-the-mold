#include "common.h"

extern void *g_pBlbHeapBase;
extern void FreeFromHeap(void *heap, void *ptr, s32 a2, s32 a3);
extern void *D_800120CC;

INCLUDE_ASM("asm/nonmatchings/ending", EndingTickCallback);

INCLUDE_ASM("asm/nonmatchings/ending", TriggerEndingSequence);

INCLUDE_ASM("asm/nonmatchings/ending", EndingCreditsRevealTick);

INCLUDE_ASM("asm/nonmatchings/ending", EndingCreditsDelayTick);

INCLUDE_ASM("asm/nonmatchings/ending", EndingCreditsScrollTick);

INCLUDE_ASM("asm/nonmatchings/ending", EndingCreditsScrollTick2);

INCLUDE_ASM("asm/nonmatchings/ending", EndingCreditsCompleteTick);

INCLUDE_ASM("asm/nonmatchings/ending", EndingCreditsFadeOutTick);

INCLUDE_ASM("asm/nonmatchings/ending", EndingEntityDestroyCallback_1E54);

INCLUDE_ASM("asm/nonmatchings/ending", EndingEntityDestroyCallback_1E74);

u32 func_80079D74(void *e) {
    return *(u32 *)((u8 *)e + 0x100);
}

INCLUDE_ASM("asm/nonmatchings/ending", func_80079D80);

INCLUDE_ASM("asm/nonmatchings/ending", EndingEntityDestroyCallback);

void func_80079DEC(void) {
}

INCLUDE_ASM("asm/nonmatchings/ending", EndingEntityDestroyCallback_2034);

INCLUDE_ASM("asm/nonmatchings/ending", EndingEntityDestroyCallback_2034_V2);

INCLUDE_ASM("asm/nonmatchings/ending", EndingEntityDestroyCallback_2034_V3);

INCLUDE_ASM("asm/nonmatchings/ending", EndingEntityDestroyCallback_2034_V4);

u32 func_80079F84(void *e) {
    return *(u32 *)((u8 *)e + 0x100);
}

void func_80079F90(void) {
}

void func_80079F98(void) {
}

void func_80079FA0(void) {
}

void func_80079FA8(void) {
}

INCLUDE_ASM("asm/nonmatchings/ending", EndingEntityDestroyCallback_2034_V5);

INCLUDE_ASM("asm/nonmatchings/ending", EndingEntityDestroyCallback_208C);

INCLUDE_ASM("asm/nonmatchings/ending", EndingEntityDestroyCallback_20AC);

void func_8007A0DC(void) {
}

void func_8007A0E4(void) {
}

void EndingEntityDestroyCallback_20CC(void *entity, u32 flag) {
    *(void **)((u8 *)entity + 0x18) = &D_800120CC;
    if (flag & 1) {
        FreeEntityNoTeardown_8007a120(entity, 0x1C);
    }
}

void FreeEntityNoTeardown_8007a120(void *e, u32 size) {
    FreeFromHeap(g_pBlbHeapBase, e, 0, 0);
}

void *PassThroughFunction(void *x) {
    return x;
}

INCLUDE_ASM("asm/nonmatchings/ending", InitEndingSequence);

extern void builtin_delete(void *ptr);

void ConditionalDelete(void *p, u32 doDelete) {
    if (doDelete & 1) {
        builtin_delete(p);
    }
}

