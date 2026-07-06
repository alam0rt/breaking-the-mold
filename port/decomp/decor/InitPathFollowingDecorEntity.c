/* =============================================================================
 * InitPathFollowingDecorEntity.c  --  PC-port path-following decor shell init
 * =============================================================================
 * Functional-C body for InitPathFollowingDecorEntity (0x8002CA34; INCLUDE_ASM
 * in src/decor.c). Shared init for every path-decor pickup/platform shell:
 * installs the offscreen-culling tick + special-trigger event handler, captures
 * the spawn position as the path origin (after the camera-scale divide), and if
 * the spawn record names a path layer (u16 @+0xC) resolves its waypoint data,
 * staggers the start phase by frame counter, and snaps the entity onto the
 * interpolated path point. Finishes with the trigger-zone tint sample.
 * Tail bytes past TimedPathEntity: +0x11C tick phase (frame&3), +0x11D
 * triggered flag, +0x11E offscreen-dead flag.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include "Game/entity.h"
#include "Game/decor_entities.h"
#include <stdint.h>

extern void SetupEntityScaleCallbacks(Entity *e);
extern void DecorEntityTickWithOffscreenCheck(Entity *e);
extern Entity *EntityCollisionHandler_SpecialTrigger(); /* real C, src/decor.c */
extern void EntityTick_PathFollowUpdate();              /* real C, src/decor.c */
extern void GetEntitySpawnData(u8 *base, u32 layerIndex, u8 **outPtr, u16 *outWidth);
extern void InterpolateTimedPathPosition(u16 *time, s16 *out, u8 *pathData, s16 duration, s32 steps);
extern void UpdateDecorEntityTriggerColors(Entity *e);

void InitPathFollowingDecorEntity(TimedPathEntity *e, DecorSpawnData *data, u8 force_static) {
    Entity *base = &e->sprite.base;
    u8 *tail = (u8 *)e;
    u32 group = 0;

    tail[0x11E] = 0;                                  /* offscreen-dead flag */
    SetupEntityScaleCallbacks(base);
    *(DecorSpawnData **)&e->_pad100 = data;           /* spawn record ptr @+0x100 */
    base->tickMarker = -0x10000;
    base->tickCallback = (EntityCallback)DecorEntityTickWithOffscreenCheck;
    base->eventMarker = -0x10000;
    base->eventCallback = (EntityCallback)EntityCollisionHandler_SpecialTrigger;
    tail[0x11D] = 0;                                  /* triggered flag */
    base->collisionMask = 0x20;
    e->pathOriginX = base->worldX;                    /* after scale divide */
    e->pathOriginY = base->worldY;
    tail[0x11C] = (u8)(g_pGameState->frame_counter & 3);

    if (force_static == 0 && data != NULL) {
        group = *(u16 *)((u8 *)data + 0xC);
    }
    if (group != 0) {
        base->renderMarker = -0x10000;
        base->renderCallback = (EntityCallback)EntityTick_PathFollowUpdate;
        GetEntitySpawnData((u8 *)g_pGameState, group, &e->pathData,
                           (u16 *)&e->pathDuration);
        if (e->pathDuration != 0) {
            s16 out[2];
            e->pathTime = (u16)(g_pGameState->frame_counter %
                                (u32)((u16)e->pathDuration * 8));
            InterpolateTimedPathPosition(&e->pathTime, out, e->pathData,
                                         e->pathDuration, 8);
            base->worldX = e->pathOriginX + out[0];
            base->worldY = e->pathOriginY + out[1];
        } else {
            group = 0;
        }
    }
    if (group == 0) {
        e->pathData = NULL;
        base->worldX = e->pathOriginX;
        base->worldY = e->pathOriginY;
    }
    UpdateDecorEntityTriggerColors(base);
}
