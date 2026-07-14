/* =============================================================================
 * CreateGlidePlayerEntity.c  --  PC-port glide (SOAR) player entity constructor
 * =============================================================================
 * Faithful transcription of CreateGlidePlayerEntity (INCLUDE_ASM in src/finn.c).
 * Builds the gliding player: base sprite from the D_8009CA34 sprite list at
 * z-order 0x3E8, installs the glide vtable (D_80011CF4), the FinnMainTickHandler
 * tick and FinnEvent_DamageToDeathExplosion event callbacks, seeds physics
 * (velocityX 0x20000, +0x10C 0x400) and the 0x14-frame idle counter, then
 * enters its start state (D_800A5F84/D_800A5F88).
 *
 * This was the level-5 crash: the stub returned an uninitialised glide player.
 * ---------------------------------------------------------------------------*/
#include "common.h"

extern void InitEntityWithSprite(void *entity, void *spriteList, s16 z, s16 x, s16 y);
extern void EntitySetState(void *e, u32 marker, void *fn);
extern void FinnMainTickHandler(void);                    /* still-asm (weak stub) */
extern void FinnEvent_DamageToDeathExplosion(void);       /* still-asm (weak stub) */

extern u32 D_8009CA34[];                       /* glide-player sprite list */
extern u8  VT_GLIDE_PLAYER[] asm("D_80011CF4");
extern u32   START_MARKER    asm("D_800A5F84");
extern void *START_STATE_FN  asm("D_800A5F88");

void CreateGlidePlayerEntity(void *arg0, s32 arg1, s16 arg2, s16 arg3) {
    u8 *e = (u8 *)arg0;

    InitEntityWithSprite(e, D_8009CA34, 0x3E8, arg2, arg3);
    *(u8 **)(e + 0x18) = VT_GLIDE_PLAYER;
    *(s32 *)(e + 0x114) = -1;
    *(s16 *)(e + 0x10) = 0x3E8;
    *(s32 *)(e + 0x100) = arg1;
    *(s32 *)(e + 0x00) = -0x10000;
    *(void **)(e + 0x04) = (void *)FinnMainTickHandler;
    *(s32 *)(e + 0x08) = -0x10000;
    *(void **)(e + 0x0C) = (void *)FinnEvent_DamageToDeathExplosion;
    *(s32 *)(e + 0x104) = 0x20000;
    *(s16 *)(e + 0x10C) = 0x400;
    *(s32 *)(e + 0x108) = 0;
    *(s16 *)(e + 0x6C) = 0;
    *(s16 *)(e + 0x6E) = 0;
    e[0x10E] = 0;
    e[0x10F] = 0;
    e[0x110] = 0;
    e[0x111] = 0;
    e[0x112] = 0x14;
    EntitySetState(e, START_MARKER, START_STATE_FN);
    e[0x113] = 0;
    e[0x119] = 0;
}
