/* =============================================================================
 * SpawnPlayerDeathEffect.c  --  functional-C body for playst.c
 * SpawnPlayerDeathEffect (TARGET_PC)
 * =============================================================================
 * Transcribed from asm/nonmatchings/playst/SpawnPlayerDeathEffect.s
 * (0x8005B2D4, 0xFC). Called when the player dies: optionally plays the death
 * sound (0x5860C640) and, when level flag bit 14 is set, spawns the death
 * particle sprite (0x422A0225) at the player's world position -- the same
 * alloc + InitParticleEntity + GetTPage + render-list add shape as
 * SpawnDecorParticleEffect.
 * ========================================================================== */
#include "common.h"
#include "globals.h"

extern void  PlayEntityPositionSound(void *e, u32 soundId);
extern u32   GetLevelFlags(void *ctx);
extern void *AllocateFromHeap(void *heap, s32 size, s32 a2, s32 a3);
extern void *InitParticleEntity(void *e, u32 spriteId, u32 packedXY, u8 facing,
                                s32 scale, s16 z, u8 flags);
extern s32   GetTPage(s32 tp, s32 abr, s32 x, s32 y);
extern void  AddEntityToSortedRenderList(void *gs, void *entity);

void SpawnPlayerDeathEffect(void *arg0, s32 playSound) {
    u8 *e = (u8 *)arg0;
    u8 *gs = (u8 *)g_pGameState;

    if (playSound & 0xFF) {
        PlayEntityPositionSound(e, 0x5860C640u);
    }
    if ((GetLevelFlags(gs + 0x84) >> 14) & 1) {
        u32 packedXY = *(u16 *)(e + 0x68) | ((u32)*(u16 *)(e + 0x6A) << 16);
        u8 *p = AllocateFromHeap(g_pBlbHeapBase, 0x108, 1, 0);
        u8 *spr;
        p = InitParticleEntity(p, 0x422A0225u, packedXY, e[0x74],
                               *(s32 *)(e + 0x58), 0x3D4, 0);
        spr = *(u8 **)(p + 0x34);
        *(s16 *)(spr + 0x24) = (s16)GetTPage(spr[0x32], 1,
                                             *(s16 *)(spr + 0x10) & ~0x3F,
                                             *(s16 *)(spr + 0x12) & ~0xFF);
        AddEntityToSortedRenderList(gs, p);
    }
}
