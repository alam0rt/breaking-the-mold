/* =============================================================================
 * InitGenericSpriteEntity.c  --  PC-port generic platform/sprite entity init
 * =============================================================================
 * Functional-C bodies for InitGenericSpriteEntity and its setup worker
 * GenericSpriteEntityInitCallback (both INCLUDE_ASM in src/bosses.c). The
 * shared init behind entity types 001/005/006/012/019-021/031-038 (moving
 * platforms, clock platforms, "boss entity" platform balls): 0x124-byte
 * entity, sprite by hash at the spawn point, vtable D_800116E8, and an
 * optional waypoint path (layer index from spawnData+0xC) driven by
 * EntityTick_InterpolateTimedPath with per-entity steps-per-segment (+0x119)
 * and closed-loop detection (first==last waypoint -> loop flag +0x118).
 *
 * Tail layout (Entity = 0x80, tail = +0x100):
 *   +0x100 spawnData   +0x104 originX/Y     +0x108 waypoints ptr
 *   +0x10C count       +0x10E path timer    +0x110/111 init flag bytes
 *   +0x114..117 zeroed +0x118 loop flag     +0x119 steps/segment (8)
 *   +0x11A zero        +0x11C state word=0  +0x120 path group (u16)
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include "Game/entity.h"
#include "Game/spracc_records.h"
#include <string.h>

extern u8 EntityCallbackTableBase_800116e8[] asm("D_800116E8");
extern Entity *InitEntitySprite(Entity *entity, u32 spriteId, s32 z, s16 x, s16 y, s32 flags);
extern void GenericSpriteEntityTickCallback();
extern void EntityCollision_SpawnSwitchBlock();
extern void EntityTick_InterpolateTimedPath();
extern s32 GetTPage(s32 tp, s32 abr, s32 x, s32 y);
extern u16 GetLevelFlags(void *levelContext);          /* real C */
extern void SetupEntityScaleCallbacks(Entity *e);
extern void GetEntitySpawnData(u8 *base, u32 layerIndex, u8 **outPtr, u16 *outWidth);
extern void InterpolateTimedPathPosition(u16 *time, s16 *out, u8 *pathData, s16 duration, s32 steps);
extern void EntityCheckTriggerZone(Entity *e);

static s16 wp16(u8 *p, s32 idx) {
    s16 v;
    memcpy(&v, p + idx * 2, sizeof(v));
    return v;
}

void GenericSpriteEntityInitCallback(Entity *e, u16 pathGroup, u8 flagHi, u8 flagLo) {
    u8 *tail = (u8 *)e;
    RenderSpriteObj *spr;
    GameState *gs;

    e->allocSize = 900;
    e->collisionMask = 8;
    *(s16 *)(tail + 0x70) = 0;                  /* targetX */
    tail[0x111] = flagHi;
    tail[0x110] = flagLo;
    e->tickMarker = -0x10000;
    e->tickCallback = (EntityCallback)GenericSpriteEntityTickCallback;
    e->eventMarker = -0x10000;
    e->eventCallback = (EntityCallback)EntityCollision_SpawnSwitchBlock;

    spr = (RenderSpriteObj *)e->spriteContext;
    spr->tpage = (u16)GetTPage(spr->colorMode, 1, spr->vramX & ~0x3F, spr->vramY & ~0xFF);

    gs = g_pGameState;
    if (gs->camera_scroll_speed == 0xC000 &&
        (GetLevelFlags(&gs->level_context) >> 0xB & 1) == 0) {
        SetupEntityScaleCallbacks(e);
    }

    *(u16 *)(tail + 0x120) = pathGroup;
    tail[0x119] = 8;                            /* steps per segment */
    tail[0x11A] = 0;
    tail[0x114] = 0;
    tail[0x115] = 0;
    tail[0x116] = 0;
    tail[0x117] = 0;
    tail[0x118] = 0;                            /* loop flag */
    *(s16 *)(tail + 0x104) = e->worldX;         /* path origin */
    *(s16 *)(tail + 0x106) = e->worldY;

    if (*(u16 *)(tail + 0x120) != 0) {
        u8 **wpp = (u8 **)(tail + 0x108);
        s16 *countp = (s16 *)(tail + 0x10C);
        GetEntitySpawnData((u8 *)g_pGameState, *(u16 *)(tail + 0x120), wpp, (u16 *)countp);
        if (*countp == 0) {
            *(u16 *)(tail + 0x120) = 0;
        } else {
            s16 out[2];
            e->renderMarker = -0x10000;
            e->renderCallback = (EntityCallback)EntityTick_InterpolateTimedPath;
            *(u16 *)(tail + 0x10E) = (u16)(g_pGameState->frame_counter %
                                           (u32)(*countp * tail[0x119]));
            InterpolateTimedPathPosition((u16 *)(tail + 0x10E), out, *wpp,
                                         *countp, tail[0x119]);
            e->worldX = *(s16 *)(tail + 0x104) + out[0];
            e->worldY = *(s16 *)(tail + 0x106) + out[1];
        }
        if (*(u16 *)(tail + 0x120) != 0 || *(u8 **)(tail + 0x108) != NULL) {
            u8 *wps = *(u8 **)(tail + 0x108);
            s16 count = *(s16 *)(tail + 0x10C);
            if (count > 0 &&
                wp16(wps, count * 2 - 2) == wp16(wps, 0) &&
                wp16(wps, count * 2 - 1) == wp16(wps, 1)) {
                tail[0x118] = 1;                /* closed loop */
                *(s16 *)(tail + 0x10C) = count - 1;
            }
        }
    }
    if (*(u16 *)(tail + 0x120) == 0) {
        *(u8 **)(tail + 0x108) = NULL;
        *(s16 *)(tail + 0x10C) = 0;
        e->worldX = *(s16 *)(tail + 0x104);
        e->worldY = *(s16 *)(tail + 0x106);
    }

    *(u32 *)(tail + 0x11C) = 0;
    spr = (RenderSpriteObj *)e->spriteContext;
    spr->r = g_pGameState->layer1_tint_r;
    spr->g = g_pGameState->layer1_tint_g;
    spr->b = g_pGameState->layer1_tint_b;
    EntityCheckTriggerZone(e);
}

Entity *InitGenericSpriteEntity(Entity *e, u8 *spawnData, u32 spriteHash, u8 flagHi, u8 flagLo) {
    InitEntitySprite(e, spriteHash, 0x3C0, *(s16 *)(spawnData + 8),
                     (s16)(*(s16 *)(spawnData + 10) - 1), 0);
    e->collisionVtable = EntityCallbackTableBase_800116e8;
    *(u8 **)((u8 *)e + 0x100) = spawnData;
    GenericSpriteEntityInitCallback(e, *(u16 *)(spawnData + 0xC), flagHi, flagLo);
    return e;
}
