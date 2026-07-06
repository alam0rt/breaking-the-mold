/* =============================================================================
 * InitDecorEntityWithScreenOffset.c  --  PC-port pickup-collected transition
 * =============================================================================
 * Functional-C body for InitDecorEntityWithScreenOffset (INCLUDE_ASM in
 * src/decor.c). Converts a just-collected pickup into a screen-space entity
 * flying to a HUD target: optionally spawns the collect particle, drops the
 * collision mask to 0x10, bumps the sprite to the top of the z-order (0x2706),
 * converts the world position to screen space (through the move-FSM transforms
 * then camera subtraction), re-points the move slots at GetWorldPositionX/Y,
 * records start (+0x114) / target (+0x110) / axis-direction flags (+0x118) and
 * resets the ease step (+0x11A), then installs the eased-flight render tick.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include "Game/entity.h"

extern void SpawnDecorParticleEffect(Entity *e);
extern void ChangeRenderZOrder(void *renderList, void *node, s16 newZ);
extern s16 GetEntityXPosition(Entity *e);   /* real C, src/anim.c */
extern s16 GetEntityYPosition(Entity *e);   /* real C, src/anim.c */
extern s16 GetWorldPositionX();             /* real C */
extern s16 GetWorldPositionY();             /* real C */
extern void EntityTick_EasedMovementInterpolation(Entity *e);
extern void EntityUpdateCallback(Entity *e);

void InitDecorEntityWithScreenOffset(Entity *e, s16 targetX, s16 targetY, s8 spawnParticle) {
    u8 *tail = (u8 *)e;

    if (spawnParticle != 0) {
        SpawnDecorParticleEffect(e);
    }
    e->collisionMask = 0x10;
    ChangeRenderZOrder(g_pGameState, e->spriteContext, 0x2706);

    *(s16 *)(tail + 0x110) = targetX;
    *(s16 *)(tail + 0x112) = targetY;

    e->worldX = GetEntityXPosition(e) - g_pGameState->camera_x;
    e->worldY = GetEntityYPosition(e) - g_pGameState->camera_y;

    e->moveMarkerX = -0x10000;
    e->moveCallbackX = (EntityCoordCallback)GetWorldPositionX;
    e->moveMarkerY = -0x10000;
    e->moveCallbackY = (EntityCoordCallback)GetWorldPositionY;

    *(s16 *)(tail + 0x114) = e->worldX;                 /* flight start */
    *(s16 *)(tail + 0x116) = e->worldY;
    tail[0x118] = (e->worldX < targetX) ? 0 : 1;        /* axis directions */
    tail[0x119] = (e->worldY < targetY) ? 0 : 1;
    *(u16 *)(tail + 0x11A) = 0;                         /* ease step */

    e->renderMarker = -0x10000;
    e->renderCallback = (EntityCallback)EntityTick_EasedMovementInterpolation;
    e->tickMarker = -0x10000;
    e->tickCallback = EntityUpdateCallback;
}
