/* =============================================================================
 * InitEntitySprite.c  --  PC-port sprite-entity base constructor (TARGET_PC)
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/entity/InitEntitySprite.s
 * (0x8001C720, 0x148 bytes; reference = export/SLES_010.90.c 12811). The common
 * constructor for every sprite-backed entity: allocates the 0x44C-byte entity
 * struct, installs the sprite base vtable + per-frame update callback, clears the
 * sprite/animation context, allocates the render context (static vs animated per
 * `specialSpriteMode`), sets world position + sprite id, and stamps the level's
 * layer-0 tint into the sprite context.
 *
 * Byte offsets transcribed from the disassembly:
 *   entity+0x00 u32 FSM marker (0xFFFF0000)   entity+0x04 tick cb (EntityUpdateCallback)
 *   entity+0x18 collision vtable (g_EntityVtable_SpriteBase)
 *   entity+0x34 spriteContext ptr             entity+0x68 worldX  entity+0x6A worldY
 *   entity+0x78 sprite frame-table region (ClearSpriteContextWrapper)
 *   entity+0x8C frame-data field (ZeroEntityField)
 *   entity+0xF7 u8 staticSpriteFlag (= specialSpriteMode)
 *   spriteContext+0x33 = 1, +0x34/0x35/0x36 = layer0 tint r/g/b (GameState+0x124..0x126)
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include <stdint.h>

extern void  InitEntityStruct(void *entity, s32 allocSize);
extern void  ClearSpriteContextWrapper(void *p);
extern void  ZeroEntityField(void *p);
extern void  InitEntityAnimationState(void *entity);
extern void  AllocSpriteRenderContext(s32 entity, s16 zOrder, u32 spriteId);
extern void  AllocateSpriteContext(s32 entity, s16 zOrder);
extern void  SetEntitySpriteId(void *entity, u32 spriteId, s32 flag);
extern void  EntityUpdateCallback(void);   /* referenced by address only */

/* Sprite-base collision vtable at rodata 0x8001044C (weak-backed on PC). */
extern u8 g_EntityVtable_SpriteBase[] asm("g_EntityVtable_SpriteBase");

void *InitEntitySprite(void *entity, u32 spriteId, s16 zOrder, s16 x, s16 y, u8 specialSpriteMode) {
    u8 *e = (u8 *)entity;
    u8 *spriteCtx;

    InitEntityStruct(e, 0x44C);
    *(void **)(e + 0x18) = g_EntityVtable_SpriteBase;
    ClearSpriteContextWrapper(e + 0x78);
    ZeroEntityField(e + 0x8C);
    InitEntityAnimationState(e);
    e[0xF7] = specialSpriteMode;
    *(u32 *)(e + 0x00) = 0xFFFF0000;
    *(void **)(e + 0x04) = (void *)EntityUpdateCallback;

    if (e[0xF7] == 0) {
        AllocSpriteRenderContext((s32)(uintptr_t)e, zOrder, spriteId);
    } else {
        AllocateSpriteContext((s32)(uintptr_t)e, zOrder);
    }

    *(s16 *)(e + 0x68) = x;
    *(s16 *)(e + 0x6A) = y;
    SetEntitySpriteId(e, spriteId, 1);

    spriteCtx = *(u8 **)(e + 0x34);
    if (spriteCtx != NULL) {
        u8 *gs = (u8 *)g_pGameState;
        spriteCtx[0x34] = gs[0x124];    /* layer0_tint_r */
        spriteCtx[0x35] = gs[0x125];    /* layer0_tint_g */
        spriteCtx[0x36] = gs[0x126];    /* layer0_tint_b */
        spriteCtx = *(u8 **)(e + 0x34);
        spriteCtx[0x33] = 1;
    }
    return entity;
}
