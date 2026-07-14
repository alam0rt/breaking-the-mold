/* =============================================================================
 * InitIndexedSpriteEntity.c  --  PC-port indexed-sprite decor entity ctor
 * =============================================================================
 * Faithful transcription of InitIndexedSpriteEntity (INCLUDE_ASM in
 * src/enemies.c). Builds a decor sprite whose id is chosen from the D_8009B430
 * table by the spawn record's index (+0xC), at z-order 0x3C5 and world pos
 * (+0x8, +0xA-1). Installs the vtable (D_80011068), resolves the texture page,
 * stashes the spawn record at +0x100, and installs the offscreen-parent
 * cleanup tick (real C, src/enemies.c). Returns the entity.
 * ---------------------------------------------------------------------------*/
#include "common.h"

extern void *InitEntitySprite(void *e, u32 spriteId, s32 z, s16 x, s16 y, s32 flags);
extern long GetTPage(int tp, int abr, int x, int y);
extern void EntityOffscreenParentCleanupTick(void *e);   /* real C */

extern u32 D_8009B430[] asm("D_8009B430");   /* indexed sprite-id table */
extern u8  VT_INDEXED_SPRITE[] asm("D_80011068");

void *InitIndexedSpriteEntity(void *arg0, void *arg1) {
    u8 *e = (u8 *)arg0;
    u8 *spawn = (u8 *)arg1;
    u8 *sc;

    InitEntitySprite(e, D_8009B430[*(u16 *)(spawn + 0xC)], 0x3C5,
                     *(s16 *)(spawn + 0x8), (s16)(*(u16 *)(spawn + 0xA) - 1), 0);
    *(u8 **)(e + 0x18) = VT_INDEXED_SPRITE;
    sc = *(u8 **)(e + 0x34);
    *(u16 *)(sc + 0x24) = (u16)GetTPage(sc[0x32], 1,
                                        *(s16 *)(sc + 0x10) & ~0x3F,
                                        *(s16 *)(sc + 0x12) & ~0xFF);
    *(void **)(e + 0x100) = spawn;
    *(s32 *)(e + 0x00) = -0x10000;
    *(void **)(e + 0x04) = (void *)EntityOffscreenParentCleanupTick;
    return e;
}
