/* =============================================================================
 * EntityTickLoopWithCamera.c  --  PC-port per-frame entity tick pass
 * =============================================================================
 * Faithful transcription of export/SLES_010.90.c EntityTickLoopWithCamera
 * (0x80020xxx, INCLUDE_ASM in src/blb.c). Walks the z-sorted tick list
 * (GameState.tick_list_head; node[0]=next, node[1]=entity) and dispatches each
 * entity's tick callback via the standard tagged-union FSM (base = the entity).
 * The camera follow (UpdateCameraPositionSmooth) is fired once, just before the
 * first entity whose sort key exceeds 1999 (i.e. it renders in front of the
 * camera plane), or at the end if no such entity exists. After every entity a
 * DeferredEntityRemoval pass culls anything marked dead during its tick. Ends by
 * ticking the screen-shake countdown and incrementing the frame counter.
 *
 * Tick FSM (entity+0x00 marker u32 = {lo s16, hi s16}, +0x04 callback):
 *   hi == 0 -> no tick handler.
 *   hi  < 0 -> call callback directly with (entity + lo).
 *   hi  > 0 -> the callback field is a s16 offset to an 8-byte slot table at
 *              entity+off; slot[hi].fn is the callback, slot[hi].arg is added to
 *              lo. The callback receives (entity + offset).
 *
 * Sort key = *(s16*)(entity+0x10) (the tick-list allocSize key).
 * ---------------------------------------------------------------------------*/
#include "common.h"
#include "globals.h"
#include "Game/game_state.h"
#include <stdint.h>

typedef void (*TickFn)(void *base);

extern void UpdateCameraPositionSmooth(GameState *gs);
extern void DeferredEntityRemoval(GameState *gs);

void EntityTickLoopWithCamera(GameState *gameState) {
    int cameraUpdated = 0;
    s32 *node = (s32 *)gameState->tick_list_head;
    s8 shake;

    while (node != NULL) {
        s16 *entity = (s16 *)(intptr_t)node[1];

        if ((!cameraUpdated) && (1999 < entity[8])) {
            UpdateCameraPositionSmooth(gameState);
            cameraUpdated = 1;
        }

        {
            s32 hi = (s32)entity[1];
            if (hi != 0) {
                TickFn fn;
                s16 slotArg = 0;
                s32 offset;
                if (hi < 1) {
                    fn = *(TickFn *)(entity + 2);
                } else {
                    s32 base = *(s32 *)((u8 *)entity + (s16)entity[2]);
                    s32 slot = hi * 8 + base;
                    slotArg = *(s16 *)((u8 *)(intptr_t)slot - 8);
                    fn = *(TickFn *)((u8 *)(intptr_t)slot - 4);
                }
                offset = (s16)entity[0];
                if (hi > 0) {
                    offset = slotArg + offset;
                }
                fn((u8 *)entity + offset);
            }
        }

        node = (s32 *)*node;
        DeferredEntityRemoval(gameState);
    }

    if (!cameraUpdated) {
        UpdateCameraPositionSmooth(gameState);
    }

    shake = (s8)gameState->screen_shake_index;
    if (shake != 0) {
        *(s8 *)&gameState->screen_shake_index = shake - 1;
    }
    gameState->frame_counter = gameState->frame_counter + 1;
}
