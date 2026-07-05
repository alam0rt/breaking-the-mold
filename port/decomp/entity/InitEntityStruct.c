/* =============================================================================
 * InitEntityStruct.c  --  PC-port base entity-struct initialiser (TARGET_PC)
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/entity/InitEntityStruct.s
 * (0x8001A0C8, 0x118 bytes; reference = export/SLES_010.90.c 10561). Zero/­default
 * initialises the 0x80-byte Entity header for every entity: clears the four FSM
 * slot pairs (tick/event/render/moveX/moveY), sets active=1, records allocSize,
 * installs the partial-destroy collision vtable + StubReturnZero event handler,
 * and sets the four 16.16 scale fields (render/render2/powerupX/powerupY and the
 * two parallax scales at +0x60/+0x64) to 1.0 (0x10000).
 *
 * All offsets/widths transcribed directly from the disassembly. The vtable at
 * +0x18 is written twice (Destroyed then PartialDestroy) exactly as the original;
 * the net installed vtable is PartialDestroy. The {tick marker=0, cb=0} pair at
 * +0x8/+0xC is immediately overwritten with the {0xFFFF0000, StubReturnZero}
 * event slot via the packed-halfword store idiom.
 * ========================================================================== */
#include "common.h"

extern void StubReturnZero(void);   /* referenced by address only */

/* Entity collision vtables at fixed rodata addresses (weak-backed on PC). */
extern u8 g_EntityVtable_Destroyed[]      asm("g_EntityVtable_Destroyed");       /* 0x800104AC */
extern u8 g_EntityVtable_PartialDestroy[] asm("g_EntityVtable_PartialDestroy");  /* 0x8001046C */

void InitEntityStruct(void *entity, s16 allocSize) {
    u8 *e = (u8 *)entity;

    *(void **)(e + 0x18) = g_EntityVtable_Destroyed;
    *(s16 *)(e + 0x00) = 0;
    *(s16 *)(e + 0x02) = 0;
    *(s32 *)(e + 0x04) = 0;
    e[0x14] = 1;                                  /* active */
    *(s16 *)(e + 0x08) = 0;
    *(s16 *)(e + 0x0A) = 0;
    *(s32 *)(e + 0x0C) = 0;
    *(s16 *)(e + 0x10) = allocSize;
    *(s16 *)(e + 0x12) = 0;
    *(void **)(e + 0x18) = g_EntityVtable_PartialDestroy;

    /* event slot: marker 0xFFFF0000, callback StubReturnZero */
    *(u32 *)(e + 0x08) = 0xFFFF0000;
    *(void **)(e + 0x0C) = (void *)StubReturnZero;

    *(s16 *)(e + 0x1C) = 0;                       /* render slot marker */
    *(s16 *)(e + 0x1E) = 0;
    *(s32 *)(e + 0x20) = 0;                       /* render callback */
    *(s16 *)(e + 0x24) = 0;                       /* moveX slot */
    *(s16 *)(e + 0x26) = 0;
    *(s32 *)(e + 0x28) = 0;
    *(s16 *)(e + 0x2C) = 0;                       /* moveY slot */
    *(s16 *)(e + 0x2E) = 0;
    *(s32 *)(e + 0x30) = 0;
    *(s16 *)(e + 0x68) = 0;                       /* worldX */
    *(s16 *)(e + 0x6A) = 0;                       /* worldY */
    *(s32 *)(e + 0x34) = 0;                       /* spriteContext */
    e[0x74] = 0;
    e[0x75] = 0;

    /* render/hitbox/bounds halfword block (0x38..0x4E) */
    *(s16 *)(e + 0x38) = 0;
    *(s16 *)(e + 0x3A) = 0;
    *(s16 *)(e + 0x3C) = 0;
    *(s16 *)(e + 0x3E) = 0;
    *(s16 *)(e + 0x40) = 0;
    *(s16 *)(e + 0x42) = 0;
    *(s16 *)(e + 0x44) = 0;
    *(s16 *)(e + 0x46) = 0;
    *(s16 *)(e + 0x48) = 0;
    *(s16 *)(e + 0x4A) = 0;
    *(s16 *)(e + 0x4C) = 0;
    *(s16 *)(e + 0x4E) = 0;
    e[0x77] = 0;
    *(s16 *)(e + 0x70) = 0;

    /* 16.16 scale fields = 1.0 */
    *(s32 *)(e + 0x50) = 0x10000;                 /* scaleRender  */
    *(s32 *)(e + 0x54) = 0x10000;                 /* scaleRender2 */
    *(s32 *)(e + 0x58) = 0x10000;                 /* scalePowerupX */
    *(s32 *)(e + 0x5C) = 0x10000;                 /* scalePowerupY */
    *(s16 *)(e + 0x72) = 0;
    *(s32 *)(e + 0x60) = 0x10000;                 /* scaleParallaxX */
    *(s32 *)(e + 0x64) = 0x10000;                 /* scaleParallaxY */
    *(s16 *)(e + 0x6C) = 0;                       /* velocityX */
    *(s16 *)(e + 0x6E) = 0;                       /* velocityY */
    e[0x76] = 0;                                  /* textureDirty */
}
