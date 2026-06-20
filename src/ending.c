#include "common.h"
#include "functions.h"
#include "Game/entity.h"
#include "Game/callback_slot.h"

extern u8 *g_pBlbHeapBase;
/* PlaySoundEffect actually returns the SPU channel index used (stored to
 * +0x150 in EndingCreditsDelayTick). functions.h declares it void; alias
 * here to capture v0. */
extern s32 PlaySoundEffectRet(u32 soundId, s32 volume, s32 channel) asm("PlaySoundEffect");
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

/* Extended entity used by the ending-credits tick chain. The sprite base
 * runs 0x00..0xFF; the credits-specific scroll/delay/sound state lives at
 * 0x100+. Fields used by the tick callbacks observed in asm:
 *   +0x140/+0x142 word pair (cleared in RevealTick)
 *   +0x144         scroll-y (?) seeded from arg in RevealTick
 *   +0x148         delay counter (u8, decremented per tick)
 *   +0x149,+0x14C  scroll-state flags (u8) */
typedef struct EndingCreditsEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8           pad100[0x48];
    /* 0x148 */ u8           delayCounter;
    /* 0x149 */ u8           pad149[7];
    /* 0x150 */ s32          soundHandle;
} EndingCreditsEntity;

extern void EndingTickCallback(Entity *e);
extern void EndingCreditsScrollTick(EndingCreditsEntity *e);

INCLUDE_ASM("asm/nonmatchings/ending", EndingTickCallback);

INCLUDE_ASM("asm/nonmatchings/ending", TriggerEndingSequence);

INCLUDE_ASM("asm/nonmatchings/ending", EndingCreditsRevealTick);

/* Per-tick delay handler installed at the start of the ending-credits
 * sequence. Decrements the delay counter at +0x148 and, when it reaches
 * zero, replaces the entity's tick slot with EndingCreditsScrollTick
 * (marker 0xFFFF0000 via the stack-staged 8-byte block-copy idiom) and
 * kicks the scroll SFX. Always falls through to EndingTickCallback. */
void EndingCreditsDelayTick(EndingCreditsEntity *e) {
    PadSlot slot;
    s16 m1;
    register void (*fn)() asm("$3");

    e->delayCounter--;
    if (e->delayCounter == 0) {
        m1 = -1;
        fn = (void (*)())EndingCreditsScrollTick;
        /* Pin fn to $v1 so cc1 keeps $v0 alive for m1=-1 and emits
         * `li v0,-1` in the bnez delay slot. */
        __asm__ volatile("" : "=r"(fn) : "0"(fn));
        slot.s.markerLo = 0;
        slot.s.markerHi = m1;
        slot.s.fn = fn;
        *(CallbackSlot *)&e->sprite.base.tickMarker = slot.s;
        e->soundHandle = PlaySoundEffectRet(0x121941C4, 0xA0, 0);
    }
    EndingTickCallback((Entity *)e);
}

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

