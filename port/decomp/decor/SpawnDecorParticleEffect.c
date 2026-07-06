/* =============================================================================
 * SpawnDecorParticleEffect.c  --  PC-port pickup-collect particle burst
 * =============================================================================
 * Functional-C body for SpawnDecorParticleEffect (INCLUDE_ASM in src/decor.c).
 * Allocates a 0x108-byte particle entity at the source entity's position
 * (sprite 0xAC607, z 0x3D4), inherits facing and scale, builds the tpage from
 * the particle's reserved VRAM rect, and appends it to the render list.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include "Game/entity.h"
#include "Game/spracc_records.h"

extern void *g_pBlbHeapBase;
extern u8 *AllocateFromHeap(void *heap, u32 size, u32 count, u8 flag);
extern Entity *InitParticleEntity(Entity *e, u32 spriteId, u32 packedXY, u8 facing,
                                  s32 scale, s16 z, u8 flags);
extern s32 GetTPage(s32 tp, s32 abr, s32 x, s32 y);
extern void AddEntityToSortedRenderList(void *gs, void *entity);

void SpawnDecorParticleEffect(Entity *e) {
    u32 packedXY = (u16)e->worldX | ((u32)(u16)e->worldY << 16);
    Entity *p;
    RenderSpriteObj *spr;

    p = (Entity *)AllocateFromHeap(g_pBlbHeapBase, 0x108, 1, 0);
    p = InitParticleEntity(p, 0xAC607, packedXY, e->facing, e->scalePowerupX, 0x3D4, 1);
    spr = (RenderSpriteObj *)p->spriteContext;
    spr->tpage = (u16)GetTPage(spr->colorMode, 1, spr->vramX & ~0x3F, spr->vramY & ~0xFF);
    AddEntityToSortedRenderList(g_pGameState, p);
}
