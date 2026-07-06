/* =============================================================================
 * SetupEntityScaleCallbacks.c  --  PC-port camera-scale move-callback install
 * =============================================================================
 * Functional-C body for SetupEntityScaleCallbacks (INCLUDE_ASM in
 * src/entity.c). When the level runs a scaled camera (camera_scroll_speed !=
 * 1.0 in 16.16), installs the Scale{X,Y}ByEntityScale move transforms, seeds
 * all four scale fields from the camera scale, and divides the entity's world
 * position into the scaled space. At 1.0 it clears the move slots instead.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include "Game/entity.h"

extern s16 ScaleXByEntityScale();   /* real C, src/entity.c */
extern s16 ScaleYByEntityScale();   /* real C, src/entity.c */

void SetupEntityScaleCallbacks(Entity *e) {
    s32 scale = (s32)g_pGameState->camera_scroll_speed;

    if (scale == 0x10000) {
        e->moveMarkerX = 0;
        e->moveCallbackX = NULL;
        e->moveMarkerY = 0;
        e->moveCallbackY = NULL;
    } else {
        e->moveMarkerX = -0x10000;
        e->moveCallbackX = (EntityCoordCallback)ScaleXByEntityScale;
        e->moveMarkerY = -0x10000;
        e->moveCallbackY = (EntityCoordCallback)ScaleYByEntityScale;
        e->scalePowerupX = scale;
        e->scalePowerupY = scale;
        e->scaleRender = scale;
        e->scaleRender2 = scale;
        e->worldX = (s16)(((s32)e->worldX << 16) / scale);
        e->worldY = (s16)(((s32)e->worldY << 16) / scale);
    }
}
