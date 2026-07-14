/* =============================================================================
 * InitEntityWithDeathSpawn.c  --  PC-port collectible tick with death-spawn
 * =============================================================================
 * Faithful transcription of InitEntityWithDeathSpawn (INCLUDE_ASM in
 * src/enemies.c). Despite the "Init" name this is a per-frame TICK: it runs the
 * base CollectibleTickCallback every frame, and when the entity's death flags
 * (+0x106 && +0x107) are both set it spawns a projectile-path entity at a
 * position resolved through the entity's bbox callback tables (X: +0x24 base /
 * +0x26 idx / +0x28 table; Y: +0x2C / +0x2E / +0x30) seeded from worldX-8 /
 * worldY-0x1C, then adds it to the sorted render list.
 *
 * The idx<<3 bbox dispatch here CALLS BACK and RETURNS an adjusted coordinate
 * (the CalcEntityYFromTileCollision family), unlike the void broadcast dispatch
 * in the collectible ticks.
 * ---------------------------------------------------------------------------*/
#include "common.h"
#include "globals.h"

extern void *g_pBlbHeapBase;
extern void  CollectibleTickCallback(void *e);
extern void *InitProjectilePathEntity(void *e, s16 x, s16 y);
extern void  AddEntityToSortedRenderList(void *gs, void *e);
extern void *AllocateFromHeap(void *heap, u32 size, u32 count, u8 flag);

/* Resolve one axis of the spawn position: idx==0 -> seed unchanged; else invoke
 * the handler (from the +fieldOff table, or that field itself when idx<=0) on
 * the entity advanced by the +baseOff offset, and return its adjusted coord. */
static s16 resolve_bbox_coord(u8 *e, s16 idxOff, s16 baseOff, s16 fieldOff, s16 seed) {
    s16 idx = *(s16 *)(e + idxOff);
    s16 (*fn)(void *, s16, void *, s16);
    s16 base;
    s32 offset;

    if (idx == 0) {
        return seed;
    }
    base = *(s16 *)(e + baseOff);
    if (idx > 0) {
        u8 *table = *(u8 **)(e + *(s16 *)(e + fieldOff));
        u8 *entry = (idx << 3) + table;
        s32 argv = *(s32 *)(entry - 8);
        fn = *(s16 (**)(void *, s16, void *, s16))(entry - 4);
        offset = (s16)argv + base;
    } else {
        fn = *(s16 (**)(void *, s16, void *, s16))(e + fieldOff);
        offset = base;
    }
    return fn(e + offset, seed, (void *)fn, idx);
}

void InitEntityWithDeathSpawn(void *arg0) {
    u8 *e = (u8 *)arg0;

    CollectibleTickCallback(e);

    if (e[0x106] != 0 && e[0x107] != 0) {
        void *proj = AllocateFromHeap(g_pBlbHeapBase, 0x104, 1, 0);
        s16 x = resolve_bbox_coord(e, 0x26, 0x24, 0x28, (s16)(*(s16 *)(e + 0x68) - 8));
        s16 y = resolve_bbox_coord(e, 0x2E, 0x2C, 0x30, (s16)(*(s16 *)(e + 0x6A) - 0x1C));
        AddEntityToSortedRenderList(g_pGameState, InitProjectilePathEntity(proj, x, y));
    }
}
