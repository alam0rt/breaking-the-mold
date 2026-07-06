/* =============================================================================
 * CheckPointCollisionAndNotify.c  --  PC-port point-collision dispatch
 * =============================================================================
 * Functional-C conversion of
 * asm/nonmatchings/blb/CheckPointCollisionAndNotify.s
 * (0x800224D0, 0x228 bytes; reference = export/SLES_010.90.c 18652).
 *
 * Finds the first collision-list entity whose AABB contains the test point,
 * dispatches that entity's event callback (tagged-union FSM slot at +0x08/+0x0C)
 * and returns the callback's result. First-hit: returns on the first match.
 *
 * Player fast path: when mask==2 and gameState->player_entity_alt is set, only
 * the player entity is tested.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include <stdint.h>

extern s32 CheckPointInBox(void *entity, s16 pointX, s16 pointY);

typedef int (*EventCb)(void *ctx, int eventId, int eventArg, void *excludeEntity);

/* Dispatch entity's event FSM (marker @ +0x08, callback @ +0x0C). */
static int notify_entity(u8 *e, u16 eventId, u32 eventArg, void *excludeEntity) {
    s16 hi = *(s16 *)(e + 0x0A);
    s16 arg = 0;
    EventCb fn;
    s32 ctxOff;
    s32 slotBase;

    if (hi == 0) {
        return 0;
    }
    if (hi < 1) {
        fn = *(EventCb *)(e + 0x0C);
    } else {
        slotBase = *(s32 *)(e + *(s16 *)(e + 0x0C)) + hi * 8;
        arg = (s16)*(u32 *)(slotBase - 8);
        fn = *(EventCb *)(slotBase - 4);
    }
    ctxOff = *(s16 *)(e + 0x08);
    if (hi > 0) {
        ctxOff = arg + ctxOff;
    }
    return (*fn)(e + ctxOff, (int)eventId, (int)eventArg, excludeEntity);
}

int CheckPointCollisionAndNotify(void *gameState, s16 pointX, s16 pointY,
                                 u16 mask, u16 eventId, u32 eventArg,
                                 void *excludeEntity) {
    u8 *gs = (u8 *)gameState;
    u8 *playerAlt = *(u8 **)(gs + 0x2C);

    if (mask == 2 && playerAlt != NULL) {
        if (CheckPointInBox(playerAlt, pointX, pointY)) {
            return notify_entity(playerAlt, eventId, eventArg, excludeEntity);
        }
    } else {
        u32 *node = *(u32 **)(gs + 0x24); /* collision_list_head */
        for (; node != NULL; node = (u32 *)node[0]) {
            u8 *entity = (u8 *)(uintptr_t)node[1];
            int hit = 0;
            if (entity != (u8 *)excludeEntity &&
                (mask & *(u16 *)(entity + 0x12)) != 0) {
                hit = CheckPointInBox(entity, pointX, pointY);
            }
            if (hit) {
                return notify_entity(entity, eventId, eventArg, excludeEntity);
            }
        }
    }
    return 0;
}
