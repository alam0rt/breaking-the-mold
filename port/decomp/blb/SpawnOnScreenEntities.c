/* =============================================================================
 * SpawnOnScreenEntities.c  --  PC-port on-screen entity instantiator
 * =============================================================================
 * Faithful transcription of export/SLES_010.90.c SpawnOnScreenEntities
 * (0x80025xxx, INCLUDE_ASM in src/blb.c). Walks the level's entity spawn list
 * (24-byte defs from Asset 501, chained through GameState.entity_spawn_list) and
 * for each un-spawned def whose rect overlaps the on-screen area (camera +/- a
 * 0x10 margin, extended by the BLB screen dimensions) dispatches that entity
 * type's constructor via the callback table and marks it spawned.
 *
 * Load-balancing: a running parity counter (seeded from frame_counter) gates
 * processing to every other list node per frame, so a long spawn list is spread
 * across two frames.
 *
 * Dispatch is the standard tagged-union FSM (see DispatchEventToCollidingEntity):
 * callback_table[type] = {marker u32, fn}. hi = marker>>16:
 *   hi <= 0  -> call fn directly with base = gameState + (s16)marker_lo.
 *   hi  > 0  -> index an 8-byte slot table at gameState + (s16)fn: pick slot
 *               [hi], its .fn is the callback and its .arg is added to the lo
 *               offset. The dispatched callback receives (gameState + offset,
 *               spawnDef).
 *
 * Spawn def record (psVar9, s16*): [0]=x [1]=y [2]=x2 [3]=y2, +0x12 type(u16),
 * +0x16 flags byte (bit0 = already spawned).
 * ---------------------------------------------------------------------------*/
#include "common.h"
#include "globals.h"
#include "Game/game_state.h"
#include <stdint.h>

typedef void (*SpawnFn)(void *base, void *def);

void SpawnOnScreenEntities(GameState *gameState) {
    s16 camX = gameState->camera_x;
    s16 camY = gameState->camera_y;
    s16 screenW = *(s16 *)g_pBlbHeapBase;
    s16 screenH = *(s16 *)((u8 *)g_pBlbHeapBase + 2);
    unsigned int parity = gameState->frame_counter & 1;
    s32 *node;

    for (node = (s32 *)gameState->entity_spawn_list; node != NULL; node = (s32 *)*node) {
        s16 *def = (s16 *)(intptr_t)node[1];
        if (((parity & 1) != 0) &&
            (def[0] < (s16)(camX + screenW + 0x10)) &&
            ((s16)(camX - 0x10) < def[2]) &&
            ((*(u8 *)(def + 0xb) & 1) == 0) &&
            (def[1] < (s16)(camY + screenH + 0x10)) &&
            ((s16)(camY - 0x10) < def[3]) &&
            ((unsigned int)(u16)def[9] < (unsigned int)gameState->entity_type_count)) {
            u8 *table = (u8 *)gameState->entity_callback_table + (unsigned int)(u16)def[9] * 8;
            u32 marker = *(u32 *)table;
            SpawnFn fn = *(SpawnFn *)(table + 4);
            s16 hi = (s16)(marker >> 16);
            s16 slotArg = 0;
            s32 offset;

            if (hi > 0) {
                s16 tableOff = (s16)(intptr_t)fn;
                s32 slot = hi * 8 + *(s32 *)((u8 *)gameState + tableOff);
                slotArg = *(s16 *)((u8 *)(intptr_t)slot - 8);
                fn = *(SpawnFn *)((u8 *)(intptr_t)slot - 4);
            }
            offset = (s16)marker;
            if (hi > 0) {
                offset = slotArg + offset;
            }
            fn((u8 *)gameState + offset, def);
            *(u8 *)(def + 0xb) = *(u8 *)(def + 0xb) | 1;
        }
        parity = parity + 1;
    }
}
