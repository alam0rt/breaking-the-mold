/* =============================================================================
 * EntityApplyMovementCallbacks.c  --  PC-port move-callback dispatch + tile probe
 * =============================================================================
 * Functional-C conversion of asm/nonmatchings/anim/EntityApplyMovementCallbacks.s
 * (0x8001FEA8, 0x168 bytes; reference = export/SLES_010.90.c 16021).
 *
 * Runs the entity's X and Y coordinate-transform callbacks (the standard tagged-
 * union FSM slot at +0x24/+0x28 for X and +0x2C/+0x30 for Y) to map (worldX,
 * worldY) through any installed movement transforms, then returns the tile
 * attribute at the resulting world position via GetTileAttributeAtPosition.
 *
 * Tagged-union decode (per the moveMarkerX/Y docs in export/datatypes.txt):
 *   hi = *(s16*)(marker + 2)
 *     hi == 0 : no callback -> coordinate unchanged
 *     hi <  0 : call fn field directly with ctx = entity + markerLo
 *     hi >  0 : fn field low-half is a byte offset to an 8-byte slot table;
 *               slot = *(s32*)(entity + off) + hi*8; arg = slot[-8], fn = slot[-4];
 *               ctx = entity + markerLo + arg
 *   callback(ctx, coord) -> new coord (truncated to s16).
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include <stdint.h>

extern u8 GetTileAttributeAtPosition(s32 levelData, s32 worldX, s32 worldY);

typedef int (*MoveCb)(void *ctx, int coord);

u8 EntityApplyMovementCallbacks(void *entity, s16 worldX, s16 worldY) {
    u8 *e = (u8 *)entity;
    s16 hi;
    s16 arg;
    MoveCb fn;
    s32 ctxOff;
    s32 slotBase;

    /* --- X callback (marker @ +0x24, callback @ +0x28) --- */
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
        worldX = (s16)(*fn)(e + ctxOff, (int)worldX);
    }

    /* --- Y callback (marker @ +0x2C, callback @ +0x30) --- */
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
        worldY = (s16)(*fn)(e + ctxOff, (int)worldY);
    }

    return GetTileAttributeAtPosition((s32)(intptr_t)g_pGameState, (s32)worldX, (s32)worldY);
}
