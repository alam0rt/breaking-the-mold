/* =============================================================================
 * UpdateDigitDisplay.c  --  PC-port HUD digit-counter sprite refresh
 * =============================================================================
 * Faithful transcription of export/SLES_010.90.c UpdateDigitDisplay (0x80023BAC).
 * Selects which decimal digit a single-digit HUD counter sprite shows: the
 * entity stores the full value at +0x114 and a place-divisor at +0x118. When the
 * divisor is 0 the raw value is used directly; otherwise the digit for that place
 * is value/divisor, and a leading-zero digit hides the sprite (active=0). The
 * resulting digit (mod 10) drives both the animation frame index and sprite id.
 *
 * The export's `trap(0x1c00)` is the MIPS integer divide-by-zero guard on a
 * statically-unreachable branch (it sits inside `if (uVar1 != 0)`), so it is
 * omitted here.
 *
 * Offsets (Entity = 0x80): +0x34 spriteContext; spriteContext+0x0A active flag;
 * +0x114 value(u16); +0x118 place divisor(u16).
 * ---------------------------------------------------------------------------*/
#include "common.h"
#include <stdint.h>

extern void SetAnimationFrameIndex(void *e, u32 frame);
/* export names this SetAnimationSpriteId; the real src symbol (same address,
 * identical pendingTargetFrame/ANIM_CHG_TARGET_FRAME bit-ops) is
 * SetAnimationTargetFrameIndex in src/anim.c. */
extern void SetAnimationTargetFrameIndex(void *e, u32 id);
extern void EntitySetRenderFlags(void *e, u8 flags);

void UpdateDigitDisplay(int param_1) {
    u8 *e = (u8 *)(intptr_t)param_1;
    u8 *spriteCtx = *(u8 **)(e + 0x34);
    u32 digit;

    *(u8 *)(spriteCtx + 0x0A) = 1;
    digit = *(u16 *)(e + 0x118);
    if (digit == 0) {
        digit = *(u16 *)(e + 0x114);
    } else {
        digit = (*(u16 *)(e + 0x114) / digit) & 0xFF;
        if (digit == 0) {
            *(u8 *)(spriteCtx + 0x0A) = 0;
        }
    }
    SetAnimationFrameIndex(e, digit % 10);
    SetAnimationTargetFrameIndex(e, digit % 10);
    EntitySetRenderFlags(e, 0);
}
