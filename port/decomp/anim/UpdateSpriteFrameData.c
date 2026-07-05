/* =============================================================================
 * UpdateSpriteFrameData.c  --  PC-port per-frame sprite metadata loader
 * =============================================================================
 * Faithful transcription of asm/nonmatchings/anim/UpdateSpriteFrameData.s
 * (0x8001D748, INCLUDE_ASM in src/anim.c). Loads the current animation frame's
 * 0x24-byte metadata record into the entity's live render fields: frame delay,
 * render offset/size, per-frame motion vectors, and hitbox, then updates the
 * sprite-context's texture rect and fires the frame's event callback (message 1)
 * if the record carries a callback id.
 *
 * The MIPS source uses lwl/lwr + swl/swr for the offset/size/hitbox copies
 * because the 0x24-stride record leaves those u32 blocks unaligned; on x86
 * unaligned `*(u32*)` accesses are legal, so the copies are written plainly.
 *
 * Frame record (0x24): +0x00 callbackId(u32), +0x04 frameDelay(u16),
 * +0x06 render off X/Y, +0x0A render W/H, +0x0E deltaX(s16), +0x10 deltaY(s16),
 * +0x12 hitbox off X/Y, +0x16 hitbox W/H, +0x1C flags(bit0 = position sound).
 * Entity fields: renderOffsetX@0x38, renderWidth@0x3C, renderHeight@0x3E,
 * hitboxOffsetX@0x40, hitboxWidth@0x44, spriteDirty@0x76, frameMotionX@0xB4,
 * frameMotionY@0xB8, frameDeltaX@0xE6, frameDeltaY@0xE8, frameRateDivisor@0xEC,
 * currentFrame@0xDA, spriteContext@0x34, doubleFrameDelay@0xFE, static@0xF7,
 * event FSM eventMarker@0x08 / eventCallback@0x0C.
 * ---------------------------------------------------------------------------*/
#include "common.h"
#include <stdint.h>

typedef void (*FrameEventFn)(void *base, int msg, u32 callbackId, void *entity);

extern void *GetSpriteFrameDataByIndex(void *pFrameData, u32 index);
extern void  PlayEntityPositionSound(void *entity);

void UpdateSpriteFrameData(void *entityPtr) {
    u8 *e = (u8 *)entityPtr;
    u8 *record;
    u16 frameDiv;
    u32 callbackId;

    if (*(u8 *)(e + 0xF7) == 0) {                       /* staticSpriteFlag */
        record = *(u8 **)(e + 0x78) + (u16)*(u16 *)(e + 0xDA) * 0x24;
    } else {
        record = (u8 *)GetSpriteFrameDataByIndex(e + 0x8C, (u16)*(u16 *)(e + 0xDA));
    }

    frameDiv = *(u16 *)(record + 0x4);
    *(u16 *)(e + 0xEC) = frameDiv;
    if (*(u8 *)(e + 0xFE) != 0) {                        /* doubleFrameDelay */
        *(u16 *)(e + 0xEC) = frameDiv << 1;
    }

    /* render offset X/Y and width/height (unaligned 32-bit copies) */
    *(u32 *)(e + 0x38) = *(u32 *)(record + 0x6);
    *(u32 *)(e + 0x3C) = *(u32 *)(record + 0xA);

    *(s16 *)(e + 0xE6) = *(s16 *)(record + 0xE);         /* frameDeltaX */
    *(s32 *)(e + 0xB4) = ((s32)*(s16 *)(e + 0xE6) << 16) / (s32)*(u16 *)(e + 0xEC);
    *(s16 *)(e + 0xE8) = *(s16 *)(record + 0x10);        /* frameDeltaY */
    *(s32 *)(e + 0xB8) = ((s32)*(s16 *)(e + 0xE8) << 16) / (s32)*(u16 *)(e + 0xEC);

    /* hitbox offset X/Y and width/height (unaligned 32-bit copies) */
    *(u32 *)(e + 0x40) = *(u32 *)(record + 0x12);
    *(u32 *)(e + 0x44) = *(u32 *)(record + 0x16);

    {
        u8 *sc = *(u8 **)(e + 0x34);                     /* spriteContext */
        if (sc != (u8 *)0) {
            *(u16 *)(sc + 0x6) = *(u16 *)(e + 0x3E);     /* heightOrV = renderHeight */
            *(u16 *)(sc + 0x4) = (*(u16 *)(e + 0x3C) + 3) & 0xFFFC; /* widthOrU */
        }
        *(u8 *)(e + 0x76) = 1;                           /* spriteDirty */
    }

    callbackId = *(u32 *)(record + 0);
    if (callbackId != 0) {
        s32 hi;
        if ((*(u32 *)(record + 0x1C) & 1) != 0) {
            PlayEntityPositionSound(e);
        }
        hi = *(s16 *)(e + 0xA);                          /* eventMarkerHi */
        if (hi != 0) {
            FrameEventFn fn;
            s16 slotArg = 0;
            s32 off;
            if (hi <= 0) {
                fn = *(FrameEventFn *)(e + 0xC);
            } else {
                s32 base = *(s32 *)(e + *(s16 *)(e + 0xC));
                s32 slot = hi * 8 + base;
                slotArg = (s16)*(u32 *)((u8 *)(intptr_t)slot - 8);
                fn = *(FrameEventFn *)((u8 *)(intptr_t)slot - 4);
            }
            off = *(s16 *)(e + 0x8);                     /* eventMarker lo */
            if (hi > 0) {
                off = slotArg + off;
            }
            fn(e + off, 1, callbackId, e);
        }
    }
}
