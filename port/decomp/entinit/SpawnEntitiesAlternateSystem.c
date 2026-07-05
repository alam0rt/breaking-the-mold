/* =============================================================================
 * SpawnEntitiesAlternateSystem.c  --  PC-port alternate (vehicle) entity spawner
 * =============================================================================
 * Faithful transcription of export/SLES_010.90.c SpawnEntitiesAlternateSystem
 * (0x80081D0C, INCLUDE_ASM in src/entinit.c). Walks the "alternate entity" array
 * (0x40-byte records at GameState.alternate_entity_data, one every other frame
 * for load-balancing) and, for any un-spawned record whose parallax-projected
 * bounding box overlaps the on-screen area (camera pos * per-record parallax
 * factor, expanded by the BLB screen dimensions + a 0x10 margin), allocates and
 * initialises the entity, adds it to the sorted render list, and marks it
 * spawned (record[+0x3C] = 1).
 *
 * Record layout (relative to piVar4 = record + 0x3C, so piVar4[-N] reaches back
 * into the record): [-0xd]=+0x08 minX, [-0xc]=+0x0C maxX, +0x0A minY, +0x0E maxY,
 * [-10]=+0x14 parallaxX, [-9]=+0x18 parallaxY, [0]=+0x3C spawnedFlag.
 *
 * The export's `(int)(X * 0x10000) >> 0x10` idiom sign-extends the low 16 bits of
 * X, i.e. it is `(s16)X` widened to int; transcribed as such.
 * ---------------------------------------------------------------------------*/
#include "common.h"
#include "globals.h"
#include "Game/game_state.h"

extern void *g_pBlbHeapBase;
extern u8   *AllocateFromHeap(u8 *heap, s32 align, s32 size, s32 flags);
extern void *InitAlternateEntity(void *entity, void *spawnData);
extern void  AddEntityToSortedRenderList(void *gs, void *entity);

void SpawnEntitiesAlternateSystem(GameState *gameState) {
    unsigned int i = gameState->frame_counter & 1;
    u8 *source = (u8 *)gameState->alternate_entity_data + i * 0x40;
    s32 *rec = (s32 *)(source + 0x3C);

    for (; (i & 0xffff) < (unsigned int)gameState->alternate_entity_count; i += 2) {
        unsigned int projX = (unsigned int)((int)gameState->camera_x * rec[-10]) >> 16;
        unsigned int projY = (unsigned int)((int)gameState->camera_y * rec[-9]) >> 16;
        s16 screenW = *(u16 *)g_pBlbHeapBase;
        s16 screenH = *(u16 *)((u8 *)g_pBlbHeapBase + 2);

        if (((s16)rec[-0xd] < (s16)(screenW + projX + 0x10)) &&
            ((s16)(projX - 0x10) < (s16)rec[-0xc]) &&
            (rec[0] == 0) &&
            (*(s16 *)((u8 *)rec - 0x32) < (s16)(screenH + projY + 0x10)) &&
            ((s16)(projY - 0x10) < *(s16 *)((u8 *)rec - 0x2e))) {
            void *entity = AllocateFromHeap((u8 *)g_pBlbHeapBase, 0x10C, 1, 0);
            InitAlternateEntity(entity, source);
            AddEntityToSortedRenderList(gameState, entity);
            rec[0] = 1;
        }
        rec += 0x20;
        source += 0x80;
    }
}
