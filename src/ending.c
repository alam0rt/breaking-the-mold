#include "common.h"
#include "functions.h"
#include "globals.h"
#include "Game/entity.h"
#include "Game/callback_slot.h"
#include "Game/game_state.h"

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
extern u8 ENDING_FADE_STEP_TABLE[] asm("D_8009CBBC");

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
 *   +0x136 scroll-counter (ScrollTick)        +0x148 delay counter
 *   +0x142 scroll-counter (ScrollTick2)       +0x149 flag (FadeOutTick)
 *   +0x144 scroll-y position                  +0x14A fade counter
 *   +0x150 SPU sound-effect handle */
typedef struct EndingCreditsEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u32          destroyFlag;
    /* 0x104 */ u8           pad104[0x32];
    /* 0x136 */ u16          scrollCounter1;
    /* 0x138 */ u8           pad138[0x8];
    /* 0x140 */ u16          field140;
    /* 0x142 */ u16          scrollCounter2;
    /* 0x144 */ u16          scrollY;
    /* 0x146 */ u8           pad146[2];
    /* 0x148 */ u8           delayCounter;
    /* 0x149 */ u8           flag149;
    /* 0x14A */ u8           fadeCounter;
    /* 0x14B */ u8           pad14B;
    /* 0x14C */ u8           flag14C;
    /* 0x14D */ u8           pad14D[3];
    /* 0x150 */ s32          soundHandle;
} EndingCreditsEntity;

extern void EndingTickCallback(Entity *e);
extern void EndingCreditsScrollTick(EndingCreditsEntity *e);
extern void EndingCreditsScrollTick2(EndingCreditsEntity *e);
extern void EndingCreditsCompleteTick(EndingCreditsEntity *e);
extern void EndingCreditsFadeOutTick(EndingCreditsEntity *e);

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

/* Per-tick scroll handler installed once the delay elapses. Every 4 frames
 * (frame_counter % 4 == 0), advances scrollY by 1 until scrollCounter1
 * runs out, then swaps in EndingCreditsScrollTick2 for the second phase.
 * Always falls through to EndingTickCallback for the usual housekeeping. */
void EndingCreditsScrollTick(EndingCreditsEntity *e) {
    PadSlot slot;
    s16 m1;
    register void (*fn)() asm("$3");

    if ((g_pGameState->frame_counter & 0x3) == 0) {
        if (e->scrollCounter1 != 0) {
            e->scrollCounter1--;
            e->scrollY++;
        } else {
            m1 = -1;
            fn = (void (*)())EndingCreditsScrollTick2;
            __asm__ volatile("" : "=r"(fn) : "0"(fn));
            slot.s.markerLo = 0;
            slot.s.markerHi = m1;
            slot.s.fn = fn;
            *(CallbackSlot *)&e->sprite.base.tickMarker = slot.s;
        }
    }
    EndingTickCallback((Entity *)e);
}

/* Phase 2 of the credits scroll (installed by EndingCreditsScrollTick once
 * its scrollCounter1 hits zero). Same shape as ScrollTick: every 4th frame
 * advance scrollY by 1 until scrollCounter2 runs out, then swap in
 * EndingCreditsCompleteTick. */
void EndingCreditsScrollTick2(EndingCreditsEntity *e) {
    PadSlot slot;
    s16 m1;
    register void (*fn)() asm("$3");

    if ((g_pGameState->frame_counter & 0x3) == 0) {
        if (e->scrollCounter2 != 0) {
            e->scrollCounter2--;
            e->scrollY++;
        } else {
            m1 = -1;
            fn = (void (*)())EndingCreditsCompleteTick;
            __asm__ volatile("" : "=r"(fn) : "0"(fn));
            slot.s.markerLo = 0;
            slot.s.markerHi = m1;
            slot.s.fn = fn;
            *(CallbackSlot *)&e->sprite.base.tickMarker = slot.s;
        }
    }
    EndingTickCallback((Entity *)e);
}

/* Phase 3 / "complete" tick: every 4th frame, advances scrollY until
 * field140 runs out, then stops the scroll-SFX voice, plays the
 * completion SFX (0x40023E30), sets flag149=1, and installs either
 * EndingTickCallback (if flag14C is set) or EndingCreditsFadeOutTick +
 * the fade-SFX (0x02990901) for the final fade-out phase. */
void EndingCreditsCompleteTick(EndingCreditsEntity *e) {
    PadSlot slot;
    s16 m1;
    register void (*fn)() asm("$3");

    if ((g_pGameState->frame_counter & 0x3) == 0) {
        if (e->field140 != 0) {
            e->field140--;
            e->scrollY++;
        } else {
            StopSPUVoice(e->soundHandle);
            e->soundHandle = -1;
            PlaySoundEffect(0x40023E30u, 0xA0, 0);
            e->flag149 = 1;
            if (e->flag14C == 0) {
                fn = (void (*)())EndingCreditsFadeOutTick;
                m1 = -1;
                slot.s.markerLo = 0;
                slot.s.markerHi = m1;
                slot.s.fn = fn;
                *(CallbackSlot *)&e->sprite.base.tickMarker = slot.s;
                PlaySoundEffect(0x02990901u, 0xA0, 0);
            } else {
                fn = (void (*)())EndingTickCallback;
                m1 = -1;
                slot.s.markerLo = 0;
                slot.s.markerHi = m1;
                slot.s.fn = fn;
                *(CallbackSlot *)&e->sprite.base.tickMarker = slot.s;
            }
        }
    }
    EndingTickCallback((Entity *)e);
}

/* Final fade-out tick: each frame, advances entity->worldX by a step
 * looked up from ENDING_FADE_STEP_TABLE indexed by fadeCounter, then
 * increments the counter. After 32 ticks (counter reaches 0x20), sets
 * flag149=1 and installs EndingTickCallback as the resting tick. */
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

/* Destructor for the credit-scroll entity: stores the flags arg into
 * the entity's destroyFlag (+0x100) BEFORE doing anything else (cc1
 * emits this as a pre-prologue sw), then sets the collision vtable,
 * tears down the entity, and finally frees it if (flags & 1).
 *
 * SHELVED: 275-byte diff because cc1 schedules the `e->destroyFlag =
 * flags;` store AFTER the prologue saves (using $s0=flags via $s1=e)
 * instead of TARGET's pre-prologue `sw a1, 0x100(a0)` (using raw
 * argument registers). Equivalent C:
 *   void EndingEntityDestroyCallback(EndingCreditsEntity *e, u32 flags) {
 *       e->destroyFlag = flags;
 *       e->sprite.base.collisionVtable = ENDING_ENTITY_VTABLE_1EB4;
 *       DestroyEntityAndFreeMemory(&e->sprite, 0);
 *       if (flags & 1) FreeFromHeap(g_pBlbHeapBase, (u8 *)e, 0, 0);
 *   }
 */
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

