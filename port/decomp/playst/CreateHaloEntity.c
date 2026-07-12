/* =============================================================================
 * CreateHaloEntity.c  --  functional-C body for playst.c CreateHaloEntity
 * (TARGET_PC)
 * =============================================================================
 * Transcribed from asm/nonmatchings/playst/CreateHaloEntity.s
 * (0x8006DE98, 0xF0). The halo powerup companion created by PlayerTickCallback
 * when powerup_flags bit 0 comes on (granted by the first orb pickup --
 * CollectibleOrbTickCallback). A 0x30 menu entity (vtable D_800117A4) that
 * follows the player: parent ptr at +0x1C, its own 0x1E8 HUD icon prim object
 * at +0x24 (icon "always draw" byte +0x1E7 set), position at +0x20/+0x22
 * seeded from the player's worldX / worldY + sprite Y offset (+0x3A, read
 * unaligned on PSX via lwl/lwr), self-destruct flag at +0x2C. Tick =
 * FinnSubentityUpdatePositionFromParent.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include <stdint.h>

extern void *g_pBlbHeapBase;
extern u8  *AllocateFromHeap(void *heap, u32 size, u32 count, u8 flag);
extern void InitMenuEntityWithVtable(void *entity, s32 allocSize);
extern u16 *InitHUDIconEntity(u16 *entity, u8 smallSize);
extern void FinnSubentityUpdatePositionFromParent();

extern u8 VT_HALO[] asm("D_800117A4");

/* player+0x3A sprite Y offset, unaligned-safe (the asm uses lwl/lwr) */
static u16 read_u16_unaligned(const u8 *p) {
    return (u16)(p[0] | (p[1] << 8));
}

void CreateHaloEntity(void *arg0, u32 player) {
    u8 *halo = (u8 *)arg0;
    u8 *parent = (u8 *)(uintptr_t)player;
    u8 *icon;

    InitMenuEntityWithVtable(halo, 0x3E9);
    *(void **)(halo + 0x18) = VT_HALO;
    *(u8 **)(halo + 0x1C) = parent;

    /* tick slot: direct callback, marker 0xFFFF0000 */
    *(s32 *)(halo + 0x0) = (s32)0xFFFF0000;
    *(void **)(halo + 0x4) = (void *)FinnSubentityUpdatePositionFromParent;

    icon = AllocateFromHeap(g_pBlbHeapBase, 0x1E8, 1, 0);
    InitHUDIconEntity((u16 *)icon, 0);
    *(u8 **)(halo + 0x24) = icon;
    icon[0x1E7] = 1;

    *(u16 *)(halo + 0x20) = *(u16 *)(parent + 0x68);
    *(s16 *)(halo + 0x22) =
        (s16)(*(s16 *)(parent + 0x6A) + read_u16_unaligned(parent + 0x3A));
    halo[0x2C] = 0;
}
