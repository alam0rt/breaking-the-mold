/* =============================================================================
 * InitEntityWithSprite.c  --  PC-port spawn-data sprite-entity ctor (TARGET_PC)
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/entity/InitEntityWithSprite.s
 * (0x8001C868, 0x118 bytes; reference = export/SLES_010.90.c 12874). A sibling of
 * InitEntitySprite for entities whose sprite/frame data comes from a spawn-data
 * record: allocates the 0x44C-byte entity, installs the sprite base vtable +
 * per-frame update callback, clears the sprite/anim context, builds the
 * multi-frame render context from the spawn data, sets world position + sprite
 * id (spriteId = *(u32*)spawnData), and stamps the level's layer-0 tint.
 *
 * Signature (from the disassembly; the export mislabels args 3-5 as "unknown"):
 *   InitEntityWithSprite(entity, spawnData, zOrder, worldX, worldY)
 *     a0=entity  a1=spawnData  a2=zOrder(s16)  a3=worldX  stack=worldY
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include <stdint.h>

extern void  InitEntityStruct(void *entity, s32 allocSize);
extern void  ClearSpriteContextWrapper(void *p);
extern void  ZeroEntityField(void *p);
extern void  InitEntityAnimationState(void *entity);
extern void  CreateMultiFrameRenderContext(void *entity, s16 zOrder, u32 *spawnData);
extern void  SetEntitySpriteId(void *entity, u32 spriteId, s32 flag);
extern void  EntityUpdateCallback(void);   /* referenced by address only */

extern u8 g_EntityVtable_SpriteBase[] asm("g_EntityVtable_SpriteBase");

void InitEntityWithSprite(void *entity, s32 spawnData, s16 zOrder, s16 worldX, s16 worldY) {
    u8 *e = (u8 *)entity;
    u32 *spawn = (u32 *)(uintptr_t)spawnData;
    u8 *spriteCtx;

    InitEntityStruct(e, 0x44C);
    *(void **)(e + 0x18) = g_EntityVtable_SpriteBase;
    ClearSpriteContextWrapper(e + 0x78);
    ZeroEntityField(e + 0x8C);
    InitEntityAnimationState(e);
    *(u32 *)(e + 0x00) = 0xFFFF0000;
    *(void **)(e + 0x04) = (void *)EntityUpdateCallback;

    CreateMultiFrameRenderContext(e, zOrder, spawn);
    *(s16 *)(e + 0x68) = worldX;
    *(s16 *)(e + 0x6A) = worldY;
    SetEntitySpriteId(e, spawn[0], 1);

    spriteCtx = *(u8 **)(e + 0x34);
    if (spriteCtx != NULL) {
        u8 *gs = (u8 *)g_pGameState;
        spriteCtx[0x34] = gs[0x124];    /* layer0_tint_r */
        spriteCtx[0x35] = gs[0x125];    /* layer0_tint_g */
        spriteCtx[0x36] = gs[0x126];    /* layer0_tint_b */
        spriteCtx = *(u8 **)(e + 0x34);
        spriteCtx[0x33] = 1;
    }
}
