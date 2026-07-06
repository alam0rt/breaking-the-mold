/* =============================================================================
 * UpdateDecorEntityTriggerColors.c  --  PC-port decor trigger-zone tint sample
 * =============================================================================
 * Functional-C body for UpdateDecorEntityTriggerColors (INCLUDE_ASM in
 * src/decor.c). Projects the entity's world position through its move-FSM
 * transforms (GetEntityXPosition/GetEntityYPosition, real C in src/anim.c),
 * asks the trigger-zone table whether that point sits in a zone, and if the
 * zone resolves to a tint (HandleGenericTriggerZone) writes the RGB into the
 * sprite context so the decor picks up the zone's ambient colour.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include "Game/entity.h"
#include "Game/spracc_records.h"

extern s16 GetEntityXPosition(Entity *e);   /* real C, src/anim.c */
extern s16 GetEntityYPosition(Entity *e);   /* real C, src/anim.c */
extern s32 CheckTriggerZoneCollision(void *gs, s16 x, s16 y, s32 *outZoneId, void *outZoneRec);
extern s32 HandleGenericTriggerZone(s32 unused, u8 zoneId, u8 *outR, u8 *outG, u8 *outB);

void UpdateDecorEntityTriggerColors(Entity *e) {
    s16 x = GetEntityXPosition(e);
    s16 y = GetEntityYPosition(e);
    s32 zoneId;
    void *zoneRec;
    u8 r, g, b;

    if ((CheckTriggerZoneCollision(g_pGameState, x, y, &zoneId, &zoneRec) & 0xFF) != 0 &&
        HandleGenericTriggerZone(0, (u8)zoneId, &r, &g, &b) != 0) {
        RenderSpriteObj *spr = (RenderSpriteObj *)e->spriteContext;
        spr->r = r;
        spr->g = g;
        spr->b = b;
    }
}
