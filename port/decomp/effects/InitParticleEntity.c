/* =============================================================================
 * InitParticleEntity.c  --  PC-port one-shot particle/fade entity init
 * =============================================================================
 * Functional-C bodies for InitParticleEntity (0x108-byte one-shot decor
 * particle) and its tick EntityTimerTickAndNotify (both INCLUDE_ASM in
 * src/effects.c). The particle carries a countdown at +0x100 and a done flag
 * at +0x102; when the timer strikes, the tick notifies the linked entity
 * (+0x104, event 0x1009) and then the game state (event 3, removal funnel).
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include "Game/entity.h"
#include "../decor/fsm_event.h"

extern void *FADE_EFFECT_ENTITY_VTABLE asm("D_80010BA8");
extern Entity *InitEntitySprite(Entity *entity, u32 spriteId, s32 z, s16 x, s16 y, s32 flags);
extern void EntitySetRenderFlags();          /* real C */
extern Entity *DecorHandleCallback();        /* real C */
extern void EntityUpdateCallback(Entity *e);
extern s16 ScaleXByEntityScale();            /* real C, src/entity.c */
extern s16 ScaleYByEntityScale();            /* real C, src/entity.c */

void EntityTimerTickAndNotify(Entity *e) {
    u8 *tail = (u8 *)e;
    s16 timer = *(s16 *)(tail + 0x100);

    EntityUpdateCallback(e);
    if (timer != 0) {
        *(s16 *)(tail + 0x100) = timer - 1;
        if (timer == 1) {
            tail[0x102] = 1;
        }
    }
    if (tail[0x102] != 0) {
        u8 *linked = *(u8 **)(tail + 0x104);
        if (linked != NULL) {
            fsm_send_event(linked, 0x1009, 0, e);
        }
        fsm_send_event((u8 *)g_pGameState, 3, 0, e);
    }
}

Entity *InitParticleEntity(Entity *e, u32 spriteId, u32 packedXY, u8 facing,
                           s32 scale, s16 z, u8 flags) {
    u8 *tail;

    InitEntitySprite(e, spriteId, z, (s16)packedXY, (s16)(packedXY >> 16), flags);
    e->collisionVtable = &FADE_EFFECT_ENTITY_VTABLE;
    e->allocSize = 0x4B0;
    e->tickMarker = -0x10000;
    e->tickCallback = EntityTimerTickAndNotify;
    e->eventMarker = -0x10000;
    e->eventCallback = (EntityCallback)DecorHandleCallback;
    EntitySetRenderFlags(e, 0);
    ((u8 *)e)[0x74] = facing;
    if (scale != 0x10000) {
        e->scalePowerupX = scale;
        e->scalePowerupY = scale;
        e->scaleRender = scale;
        e->scaleRender2 = scale;
        e->moveMarkerX = -0x10000;
        e->moveCallbackX = (EntityCoordCallback)ScaleXByEntityScale;
        e->moveMarkerY = -0x10000;
        e->moveCallbackY = (EntityCoordCallback)ScaleYByEntityScale;
    }
    tail = (u8 *)e;
    tail[0x102] = 0;                /* done flag */
    *(u16 *)(tail + 0x100) = 0;     /* countdown */
    *(void **)(tail + 0x104) = NULL;/* linked entity */
    return e;
}
