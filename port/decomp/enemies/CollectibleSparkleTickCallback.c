/* =============================================================================
 * CollectibleSparkleTickCallback.c  --  PC-port conversion (0x8003B828)
 * =============================================================================
 * Tick for the sparkling collectible: base CollectibleTickCallback, then --
 * only while on-screen (+0x119) -- a small glint state machine:
 *   +0x11B phase: 0 = brightening (+0xC/frame to the pulse byte +0x11A),
 *                 1 = hold at 0xBF while spawning sparkle particles,
 *                 2 = dimming (-0xC/frame), else = reset pulse to 0.
 *   +0x11C phase countdown: 0->1 (0x3C, sets the +0x10C glow byte),
 *                 1->2 (0xA, clears it), 2->3 (0x1E), else ->0 (0xA).
 *   +0x11D sparkle spawn countdown (phase 1 only): every (rand()&7)+4 frames
 *          spawn a 0x108-byte particle (sprite 0x5822C080, z 0x3D4) at
 *          x-0x10+(rand()&0x1F), y-(rand()&0x1F), stamp its prim's tpage
 *          (GetTPage(bpp, 1, u&~0x3F, v&~0xFF)) and register it via
 *          AddEntityToSortedRenderList.
 * The pulse byte + 0x40 drives the prim's r (g = b = 0x40).
 * Heap-alloc + rand draw order preserved for PSX-mirror/demo parity.
 * ========================================================================== */
#include "common.h"
#include "globals.h"

extern void CollectibleTickCallback(void *e);
extern u8  *AllocateFromHeap(void *heap, u32 size, u32 count, u8 flag);
extern Entity *InitParticleEntity(Entity *e, u32 spriteId, u32 packedXY,
                                  u8 facing, s32 scale, s16 z, u8 flags);
extern long GetTPage(int tp, int abr, int x, int y);
extern void AddEntityToSortedRenderList(void *gs, void *ent);
extern s32  rand(void);

void CollectibleSparkleTickCallback(void *arg0) {
    u8 *e = (u8 *)arg0;
    u8 *prim;
    u8 phase;

    CollectibleTickCallback(arg0);

    if (e[0x119] == 0) {
        return;
    }

    if (e[0x11B] == 1) {
        e[0x11D] = (u8)(e[0x11D] - 1);
        if (e[0x11D] == 0) {
            u16 sx, sy;
            u8 *p;
            Entity *part;
            u8 *pprim;

            sx = (u16)(*(u16 *)(e + 0x68) - 0x10 + (rand() & 0x1F));
            sy = (u16)(*(u16 *)(e + 0x6A) - (rand() & 0x1F));
            p = AllocateFromHeap(g_pBlbHeapBase, 0x108, 1, 0);
            part = InitParticleEntity((Entity *)p, 0x5822C080,
                                      ((u32)sy << 16) | sx, e[0x74],
                                      *(s32 *)(e + 0x50), 0x3D4, 0);
            pprim = *(u8 **)((u8 *)part + 0x34);
            *(s16 *)(pprim + 0x24) =
                (s16)GetTPage(pprim[0x32], 1,
                              *(s16 *)(pprim + 0x10) & ~0x3F,
                              *(s16 *)(pprim + 0x12) & ~0xFF);
            AddEntityToSortedRenderList(g_pGameState, part);
            e[0x11D] = (u8)((rand() & 7) + 4);
        }
    }

    phase = e[0x11B];
    if (phase == 0) {
        e[0x11A] = (u8)(e[0x11A] + 0xC);
    } else if (phase == 2) {
        e[0x11A] = (u8)(e[0x11A] - 0xC);
    } else if (phase == 1) {
        e[0x11A] = 0xBF;
    } else {
        e[0x11A] = 0;
    }

    prim = *(u8 **)(e + 0x34);
    prim[0x34] = (u8)(e[0x11A] + 0x40);
    prim[0x35] = 0x40;
    prim[0x36] = 0x40;

    e[0x11C] = (u8)(e[0x11C] - 1);
    if (e[0x11C] == 0) {
        phase = e[0x11B];
        if (phase == 0) {
            e[0x10C] = 1;
            e[0x11B] = 1;
            e[0x11C] = 0x3C;
        } else if (phase == 1) {
            e[0x11B] = 2;
            e[0x10C] = 0;
            e[0x11C] = 0xA;
        } else if (phase == 2) {
            e[0x11B] = 3;
            e[0x11C] = 0x1E;
        } else {
            e[0x11B] = 0;
            e[0x11C] = 0xA;
        }
    }
}
