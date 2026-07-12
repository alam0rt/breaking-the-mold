/* =============================================================================
 * SpawnCollectibleParticles.c  --  functional-C body for enemies.c
 * SpawnCollectibleParticles (TARGET_PC)
 * =============================================================================
 * Transcribed from asm/nonmatchings/enemies/SpawnCollectibleParticles.s
 * (0x8003B2D0, 0x364). The pickup burst spawned by CollectibleTickCallback at
 * (worldX + dx, worldY + dy):
 *   1. a one-shot sprite particle (0x108 alloc, sprite hash 0x98004092,
 *      z 0x3D4) inheriting the collectible's facing (+0x74) and render scale
 *      (+0x50); its sprite tpage is recomputed with GetTPage(abr=1) so it
 *      draws semi-transparent; sorted-render-listed.
 *   2. a screen-space scaled menu effect at the TransformX/YCoord-mapped
 *      position (z arg 0x3D3, unused by the ctor); z-order-listed.
 *   3. when the collectible's +0x10D flag is set (non-0x56..0x58 sprite
 *      types), three physics debris chunks (hashes 0x5A89815F / 0x5AB9815F /
 *      0x5AD9815F, all-random velocities via the 0x7FFFFFFF sentinels);
 *      each sorted-render-listed.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include "Game/entity.h"
#include "Game/spracc_records.h"
#include <stdint.h>

extern void *g_pBlbHeapBase;
extern u8  *AllocateFromHeap(void *heap, u32 size, u32 count, u8 flag);
extern Entity *InitParticleEntity(Entity *e, u32 spriteId, u32 packedXY, u8 facing,
                                  s32 scale, s16 z, u8 flags);
extern void *InitScaledMenuEntityWithChild(void *e, s16 x, s16 y, s32 zOrder, s32 scale);
extern Entity *InitDebrisParticleEntity(Entity *e, u32 spriteId, s16 x, s16 y,
                                        s32 scale, s32 velX, s32 velY, s32 spin);
extern long GetTPage(int tp, int abr, int x, int y);
extern void AddEntityToSortedRenderList(void *gs, void *entity);
extern void AddToZOrderList(void *gs, void *entity);
extern s16  TransformXCoord(Entity *e, s16 val);   /* real C, src/anim.c */
extern s16  TransformYCoord(Entity *e, s16 val);   /* real C, src/anim.c */

static const u32 k_debris_hashes[3] = { 0x5A89815F, 0x5AB9815F, 0x5AD9815F };

void SpawnCollectibleParticles(Entity *e, s32 dx, s32 dy) {
    u8 *tail = (u8 *)e;
    s16 x = (s16)(*(u16 *)(tail + 0x68) + dx);
    s16 y = (s16)(*(u16 *)(tail + 0x6A) + dy);
    s32 scale = *(s32 *)(tail + 0x50);
    Entity *p;
    RenderSpriteObj *spr;
    int i;

    /* 1. sprite particle with semi-transparent tpage */
    p = (Entity *)AllocateFromHeap(g_pBlbHeapBase, 0x108, 1, 0);
    InitParticleEntity(p, 0x98004092,
                       ((u32)(u16)y << 16) | (u16)x,
                       tail[0x74], scale, 0x3D4, 0);
    spr = (RenderSpriteObj *)p->spriteContext;
    spr->tpage = (u16)GetTPage(spr->colorMode, 1,
                               spr->vramX & ~0x3F, spr->vramY & ~0xFF);
    AddEntityToSortedRenderList(g_pGameState, p);

    /* 2. screen-space scaled menu effect */
    {
        u8 *m = AllocateFromHeap(g_pBlbHeapBase, 0x2C, 1, 0);
        s16 sx = TransformXCoord(e, x);
        s16 sy = TransformYCoord(e, y);
        AddToZOrderList(g_pGameState,
                        InitScaledMenuEntityWithChild(m, sx, sy, 0x3D3, scale));
    }

    /* 3. debris burst for non-plain collectibles */
    if (tail[0x10D] != 0) {
        for (i = 0; i < 3; i++) {
            u8 *d = AllocateFromHeap(g_pBlbHeapBase, 0x10C, 1, 0);
            AddEntityToSortedRenderList(
                g_pGameState,
                InitDebrisParticleEntity((Entity *)d, k_debris_hashes[i], x, y,
                                         scale, 0x7FFFFFFF, 0x7FFFFFFF,
                                         0x7FFFFFFF));
        }
    }
}
