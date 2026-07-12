/* =============================================================================
 * InitDebrisParticleEntity.c  --  functional-C body for effects.c
 * InitDebrisParticleEntity (TARGET_PC)
 * =============================================================================
 * Transcribed from asm/nonmatchings/effects/InitDebrisParticleEntity.s
 * (0x80034EC8, 0x224). Physics debris chunk (0x10C alloc, vtable D_80010A48)
 * thrown on collectible pickup / destruction. 16.16 velocities at
 * +0x100 (X) / +0x104 (Y) and spin at +0x108; the magic 0x7FFFFFFF asks for a
 * randomized value (rand() is the PSX BIOS LCG in spec/bios.c, so the demo
 * replay stays deterministic):
 *   velX = ((rand() & 0x7FFF) << 5) - 0x60000       [-0x60000 .. +0x3FFE0)
 *   velY = -(((rand() & 0x7FFF) << 4) + 0x20000)    [falls up: negative]
 *   spin = (rand() & 0x1FFF) - 0x400
 * A random anim frame (rand() % 5) picks the debris shape. Tick =
 * DebrisParticleTickCallback (matched C, src/effects.c); update slot =
 * DebrisParticlePhysicsTick (weak stub until converted); render scale +
 * ScaleX/YByEntityScale coordinate transforms as in InitParticleEntity.
 * The spin is pre-scaled by the render scale (32-bit wrap, >> 16).
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include "Game/entity.h"
#include <stdint.h>

extern Entity *InitEntitySprite(Entity *entity, u32 spriteId, s32 z, s16 x, s16 y, s32 flags);
extern void SetAnimationFrameIndex(void *e, s16 idx);
extern int  rand(void);
extern void DebrisParticleTickCallback();      /* matched C, src/effects.c */
extern void DebrisParticlePhysicsTick();       /* still-asm (weak stub)    */
extern s16  ScaleXByEntityScale();             /* real C, src/entity.c     */
extern s16  ScaleYByEntityScale();             /* real C, src/entity.c     */

extern u8 VT_DEBRIS_PARTICLE[] asm("D_80010A48");

Entity *InitDebrisParticleEntity(Entity *e, u32 spriteId, s16 x, s16 y,
                                 s32 scale, s32 velX, s32 velY, s32 spin) {
    u8 *tail = (u8 *)e;

    InitEntitySprite(e, spriteId, 0xBB8, x, y, 0);
    e->collisionVtable = VT_DEBRIS_PARTICLE;
    *(s32 *)(tail + 0x100) = velX;
    e->allocSize = 0x44C;
    *(s32 *)(tail + 0x104) = velY;
    *(s32 *)(tail + 0x108) = spin;

    if (*(s32 *)(tail + 0x100) == 0x7FFFFFFF) {
        *(s32 *)(tail + 0x100) = (s32)(((u32)(rand() & 0x7FFF) << 5) + 0xFFFA0000u);
    }
    if (*(s32 *)(tail + 0x104) == 0x7FFFFFFF) {
        *(s32 *)(tail + 0x104) = -(s32)(((u32)(rand() & 0x7FFF) << 4) + 0x20000);
    }
    if (*(s32 *)(tail + 0x108) == 0x7FFFFFFF) {
        *(s32 *)(tail + 0x108) = (rand() & 0x1FFF) - 0x400;
    }

    SetAnimationFrameIndex(e, (s16)(rand() % 5));
    *(s16 *)(tail + 0x6C) = 0;
    *(s16 *)(tail + 0x6E) = 0;

    e->tickMarker = -0x10000;
    e->tickCallback = (EntityCallback)DebrisParticleTickCallback;
    /* update slot at +0x1C/+0x20 (EntityUpdateCallback dispatch) */
    *(s32 *)(tail + 0x1C) = (s32)0xFFFF0000;
    *(void **)(tail + 0x20) = (void *)DebrisParticlePhysicsTick;

    e->scalePowerupX = scale;
    e->scalePowerupY = scale;
    e->scaleRender = scale;
    e->scaleRender2 = scale;
    e->moveMarkerX = -0x10000;
    e->moveCallbackX = (EntityCoordCallback)ScaleXByEntityScale;
    e->moveMarkerY = -0x10000;
    e->moveCallbackY = (EntityCoordCallback)ScaleYByEntityScale;

    /* spin pre-scaled by render scale: 32-bit wraparound multiply, >> 16 */
    *(s32 *)(tail + 0x108) =
        (s32)((u32)*(s32 *)(tail + 0x108) * (u32)scale) >> 16;
    return e;
}
