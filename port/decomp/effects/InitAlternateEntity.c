/* =============================================================================
 * InitAlternateEntity.c  --  PC-port "alternate" spawn-bounded entity ctor
 * =============================================================================
 * Faithful transcription of InitAlternateEntity (INCLUDE_ASM in src/effects.c).
 * Allocates a 0x44C entity, installs its vtable (D_80010AB8) and the
 * spawn-bounds tick (EntityTickWithSpawnBoundsCheck), builds its sprite context
 * (SetupAlternateEntitySpriteContext), seeds world pos from the spawn record
 * (+0x10/+0x12) and the sprite id from the D_8009B144 table indexed by the
 * record's type word, tints the render context with the GameState backdrop RGB
 * (+0x124..126), and copies the two spawn params at +0x14/+0x18.
 *
 * Next crasher after CreateGlidePlayerEntity on level 5 ('Shriney Guard').
 * ---------------------------------------------------------------------------*/
#include "common.h"
#include "globals.h"

extern void InitFullEntityWithAnimation(void *e, s32 allocSize);
extern void SetupAlternateEntitySpriteContext(void *e);
extern void SetEntitySpriteId(void *e, u32 spriteId, s32 flag);
extern void EntityTickWithSpawnBoundsCheck(void);   /* still-asm (weak stub) */

extern u32 D_8009B144[] asm("D_8009B144");   /* sprite-id table */
extern u8  VT_ALT[] asm("D_80010AB8");

void InitAlternateEntity(void *arg0, void *arg1) {
    u8 *e = (u8 *)arg0;
    u8 *spawn = (u8 *)arg1;
    u8 *sc;

    InitFullEntityWithAnimation(e, 0x44C);
    *(u8 **)(e + 0x18) = VT_ALT;
    *(s32 *)(e + 0x104) = 0;
    *(void **)(e + 0x100) = spawn;
    *(s32 *)(e + 0x00) = -0x10000;
    *(void **)(e + 0x04) = (void *)EntityTickWithSpawnBoundsCheck;
    SetupAlternateEntitySpriteContext(e);

    *(s16 *)(e + 0x68) = *(u16 *)(spawn + 0x10);
    *(s16 *)(e + 0x6A) = *(u16 *)(spawn + 0x12);
    SetEntitySpriteId(e, D_8009B144[*(s32 *)(spawn + 0x00)], 1);

    sc = *(u8 **)(e + 0x34);
    if (sc != NULL) {
        u8 *gs = (u8 *)g_pGameState;
        sc[0x34] = gs[0x124];
        sc[0x35] = gs[0x125];
        sc[0x36] = gs[0x126];
        sc[0x33] = 1;
    }

    *(s32 *)(e + 0x60) = *(s32 *)(spawn + 0x14);
    e[0x109] = 0;
    *(s32 *)(e + 0x64) = *(s32 *)(spawn + 0x18);
}
