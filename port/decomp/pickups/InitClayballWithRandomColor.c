/* =============================================================================
 * InitClayballWithRandomColor.c  --  PC-port type-007 clayball pickup init
 * =============================================================================
 * Functional-C body for InitClayballWithRandomColor (0x8002DBDC, 0x264).
 * Byte-match shelved in src/pickups.c (register-coloring residual only); this
 * is the same verified logic, kept behaviourally identical:
 *   - orange-ball sprite 0xB8700CA1, path-following decor shell
 *     (same shape as the matched InitGreenBulletsCollectible),
 *   - allocSize 0x458, tick = CollectibleClaySingleTickCallback,
 *   - tpage from the reserved VRAM rect,
 *   - random start frame (rand()&7) and per-channel random tint
 *     clamp(rand()%48 + base - 0x18, 0, 255).
 * ========================================================================== */
#include "common.h"
#include "functions.h"
#include "globals.h"
#include "Game/callback_slot.h"
#include "Game/pickups_entities.h"
#include "Game/spracc_records.h"

extern u8 DECOR_ENTITY_DESTRUCTOR_VTABLE[] asm("D_80010870");
extern u8 CLAYBALL_VTABLE[] asm("D_800107F0");
extern Entity *InitEntitySprite(Entity *entity, u32 spriteId, s32 z, s16 x, s16 y, s32 flags);
extern void InitPathFollowingDecorEntity(TimedPathEntity *e, DecorSpawnData *data, u8 flag);
extern s32 GetTPage(s32 tp, s32 abr, s32 x, s32 y);
extern void SetAnimationFrameIndex(Entity *e, u32 value);
extern void CollectibleClaySingleTickCallback(InteractiveDecorEntity *e);
extern s32 rand(void);

static u8 clay_tint(u8 base) {
    s32 v = (rand() % 48) + base - 0x18;
    if (v < 0) {
        v = 0;
    } else if (v > 0xFF) {
        v = 0xFF;
    }
    return (u8)v;
}

TimedPathEntity *InitClayballWithRandomColor(TimedPathEntity *e, DecorSpawnData *data) {
    TripadSlot u;
    RenderSpriteObj *spr;

    InitEntitySprite((Entity *)e, 0xB8700CA1, 0x3DE, data->x, data->y, 1);
    e->sprite.base.collisionVtable = DECOR_ENTITY_DESTRUCTOR_VTABLE;
    InitPathFollowingDecorEntity(e, data, 0);
    e->sprite.base.collisionVtable = CLAYBALL_VTABLE;
    e->sprite.base.allocSize = 0x458;

    u.s.markerLo = 0;
    u.s.markerHi = -1;
    u.s.fn = (void (*)())CollectibleClaySingleTickCallback;
    *(CallbackSlot *)&e->sprite.base.tickMarker = u.s;

    spr = (RenderSpriteObj *)e->sprite.base.spriteContext;
    spr->tpage = GetTPage(spr->colorMode, 1, spr->vramX & ~0x3F, spr->vramY & ~0xFF);

    SetAnimationFrameIndex((Entity *)e, rand() & 7);

    spr = (RenderSpriteObj *)e->sprite.base.spriteContext;
    spr->r = clay_tint(spr->r);
    spr->g = clay_tint(spr->g);
    spr->b = clay_tint(spr->b);
    return e;
}
