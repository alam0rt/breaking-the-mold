/* =============================================================================
 * TeleporterExitState.c  --  functional-C body for enemies.c TeleporterExitState
 * =============================================================================
 * Transcribed from asm/nonmatchings/enemies/TeleporterExitState.s (0x800455BC,
 * 0xBC). State the camera/teleporter entity enters when the transition
 * countdown expires (installed via EntitySetState from
 * TeleporterTransitionTickCallback with the D_800A5B28/2C pair).
 *
 * Clears the linked entity's sprite-context byte +0xA, restores the base scale
 * snapshot (+0x104) into the live X scale (+0x50/+0x54), swaps the tick slot to
 * the plain EntityUpdateCallback and the event slot to
 * TeleporterPortalEventHandler, switches to the idle portal sprite (0x1A2AB594,
 * the 2nd entry of the D_8009B6B0 list) and resets the render flags.
 * ========================================================================== */
#include "common.h"
#include "globals.h"

extern void EntityUpdateCallback(void *e);
extern void TeleporterPortalEventHandler(void);
extern void SetEntitySpriteId(void *e, u32 spriteId, s32 mode);
extern void EntitySetRenderFlags(void *e, s32 flags);

void TeleporterExitState(void *arg0) {
    u8 *e = (u8 *)arg0;
    u8 *linked = *(u8 **)(e + 0x108);

    if (linked != NULL) {
        u8 *sctx = *(u8 **)(linked + 0x34);
        sctx[0xA] = 0;
    }
    *(s32 *)(e + 0x50) = *(s32 *)(e + 0x104);
    *(s32 *)(e + 0x54) = *(s32 *)(e + 0x104);

    *(u32 *)(e + 0x00) = 0xFFFF0000u;           /* tick slot */
    *(void **)(e + 0x04) = (void *)EntityUpdateCallback;
    *(u32 *)(e + 0x08) = 0xFFFF0000u;           /* event slot */
    *(void **)(e + 0x0C) = (void *)TeleporterPortalEventHandler;

    SetEntitySpriteId(e, 0x1A2AB594u, 1);
    EntitySetRenderFlags(e, 0);
}
