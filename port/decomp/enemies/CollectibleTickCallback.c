/* =============================================================================
 * CollectibleTickCallback.c  --  functional-C bodies for enemies.c
 * CollectibleTickCallback + CheckCollectibleOffscreen (TARGET_PC)
 * =============================================================================
 * Transcribed from asm/nonmatchings/enemies/{CollectibleTickCallback.s
 * (0x8003ACC8, 0x1D8), CheckCollectibleOffscreen.s (0x8003AB70, 0x158)}.
 * The per-frame tick installed by InitCollectibleEntity (the hottest stub on
 * the SCIE demo path: ~1200 hits / 1500 frames).
 *
 * Collectible tail fields used here:
 *   +0x100 sprite spawn record ptr    +0x106 "collected" latch
 *   +0x107 "removal sent" latch       +0x108 collector object ptr (event dst)
 *   +0x10C collision-mode byte (2 = no player collision check)
 *
 * Tick: sprite update, player collision probe (event 0x1000), then on
 * collection: notify the collector object (event 0x1009 via its tagged slot),
 * release the spawn record (clear its in-use bit 0 at +0x16 when flag bit 0 of
 * +0x10 says it is releasable), funnel event 3 (entity removal) to the
 * GameState, spawn the pickup particle burst, and latch +0x107.
 *
 * Offscreen check: while un-collected and un-collared (+0x108 == 0), if the
 * entity is off-screen (and its spawn record's whole entity-loop is, too),
 * release the record and send the removal event -- same tail as collection.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include "../decor/fsm_event.h"

extern void EntityUpdateCallback(void *e);
extern void CollisionCheckWrapper(void *e, s32 a, s32 b, s32 c);
extern unsigned int IsEntityOffScreen(void *e);
extern unsigned int IsEntityOffScreen_EntityLoop(void *gs, void *rec);
extern void SpawnCollectibleParticles(void *e, s32 dx, s32 dy);

/* Release the sprite spawn record at +0x100 (clear in-use bit) and send the
 * entity-removal event (3) to the GameState; latch +0x107. Shared tail of the
 * collection and offscreen paths. `checkBit` replicates the asm difference:
 * the collection path only releases records whose +0x10 bit 0 is set. */
static void release_and_remove(u8 *tail, int checkBit) {
    u8 *rec = *(u8 **)(tail + 0x100);
    if (rec != NULL && (!checkBit || (*(u16 *)(rec + 0x10) & 1))) {
        rec[0x16] &= 0xFE;
        *(u32 *)(tail + 0x100) = 0;
    }
    fsm_send_event((u8 *)g_pGameState, 3, 0, tail);
    tail[0x107] = 1;
}

void CheckCollectibleOffscreen(void *arg0) {
    u8 *tail = (u8 *)arg0;
    int gone = 0;

    if (tail[0x107] == 0 && *(u32 *)(tail + 0x108) == 0 &&
        (IsEntityOffScreen(tail) & 0xFF) != 0) {
        u8 *rec = *(u8 **)(tail + 0x100);
        if (rec == NULL ||
            (IsEntityOffScreen_EntityLoop(g_pGameState, rec) & 0xFF) != 0) {
            gone = 1;
        }
    }
    if (gone) {
        release_and_remove(tail, 0);
    }
}

void CollectibleTickCallback(void *arg0) {
    u8 *tail = (u8 *)arg0;

    EntityUpdateCallback(tail);
    if (tail[0x10C] != 2) {
        CollisionCheckWrapper(tail, 2, 0x1000, tail[0x10C]);
    }

    if (tail[0x106] != 0) {
        u8 *collector = *(u8 **)(tail + 0x108);
        if (collector != NULL) {
            fsm_send_event(collector, 0x1009, 0, tail);
        }
        release_and_remove(tail, 1);
        SpawnCollectibleParticles(tail, 0, -0x14);
    }

    CheckCollectibleOffscreen(tail);
}
