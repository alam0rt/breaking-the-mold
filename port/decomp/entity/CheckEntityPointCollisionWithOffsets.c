/* =============================================================================
 * CheckEntityPointCollisionWithOffsets.c  --  PC-port point-collision probe
 * =============================================================================
 * Functional-C conversion of
 * asm/nonmatchings/entity/CheckEntityPointCollisionWithOffsets.s
 * (0x8001B594, 0x198 bytes; reference = export/SLES_010.90.c 11766).
 *
 * Runs the entity's X/Y coordinate-transform callbacks (the same tagged-union
 * FSM slot at +0x24/+0x28 and +0x2C/+0x30 used by EntityApplyMovementCallbacks)
 * to map the entity's (worldX, worldY) into a probe point, then dispatches a
 * point-collision test that notifies the first overlapping entity via
 * CheckPointCollisionAndNotify.
 *
 * Tagged-union decode: hi = *(s16*)(marker + 2); hi==0 -> no callback;
 *   hi<0 -> call fn field directly with ctx = entity + markerLo;
 *   hi>0 -> fn field low-half is a byte offset to an 8-byte slot table,
 *           slot = *(s32*)(entity + off) + hi*8, arg = slot[-8], fn = slot[-4],
 *           ctx = entity + markerLo + arg.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include <stdint.h>

extern int CheckPointCollisionAndNotify(void *gameState, s16 pointX, s16 pointY,
                                        u16 mask, u16 eventId, u32 eventArg,
                                        void *excludeEntity);

typedef int (*MoveCb)(void *ctx, int coord);

void CheckEntityPointCollisionWithOffsets(void *entity, u16 arg2, u16 arg3, u32 arg4) {
    u8 *e = (u8 *)entity;
    s16 hi;
    s16 arg;
    MoveCb fn;
    s32 ctxOff;
    s32 slotBase;
    s16 pointX;
    s16 pointY;

    /* --- X callback (marker @ +0x24, callback @ +0x28); seed = worldX @ +0x68 --- */
    pointX = *(s16 *)(e + 0x68);
    hi = *(s16 *)(e + 0x26);
    if (hi != 0) {
        arg = 0;
        if (hi < 1) {
            fn = *(MoveCb *)(e + 0x28);
        } else {
            slotBase = *(s32 *)(e + *(s16 *)(e + 0x28)) + hi * 8;
            arg = (s16)*(u32 *)(slotBase - 8);
            fn = *(MoveCb *)(slotBase - 4);
        }
        ctxOff = *(s16 *)(e + 0x24);
        if (hi > 0) {
            ctxOff = arg + ctxOff;
        }
        pointX = (s16)(*fn)(e + ctxOff, (int)pointX);
    }

    /* --- Y callback (marker @ +0x2C, callback @ +0x30); seed = worldY @ +0x6A --- */
    pointY = *(s16 *)(e + 0x6A);
    hi = *(s16 *)(e + 0x2E);
    if (hi != 0) {
        arg = 0;
        if (hi < 1) {
            fn = *(MoveCb *)(e + 0x30);
        } else {
            slotBase = *(s32 *)(e + *(s16 *)(e + 0x30)) + hi * 8;
            arg = (s16)*(u32 *)(slotBase - 8);
            fn = *(MoveCb *)(slotBase - 4);
        }
        ctxOff = *(s16 *)(e + 0x2C);
        if (hi > 0) {
            ctxOff = arg + ctxOff;
        }
        pointY = (s16)(*fn)(e + ctxOff, (int)pointY);
    }

    CheckPointCollisionAndNotify(g_pGameState, pointX, pointY, arg2, arg3, arg4, entity);
}
