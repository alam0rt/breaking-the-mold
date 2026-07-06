/* =============================================================================
 * collectible_inits.c  --  PC-port level-entity inits on the level-1 spawn path
 * =============================================================================
 * Functional-C bodies for the still-asm inner inits reached by the level-1
 * (PHRO stage0) entity spawn table, plus their two shared workers:
 *
 *   InitCollectibleEntity        shared collectible shell (tick/event/zone)
 *   UpdateCollectibleTriggerZone trigger-zone tint sample (== decor variant)
 *   CalcEntityYFromTileHeight    ground-snap Y from tile slope height
 *   InitCollectibleVariant       types 106/107/108 falling platform
 *   InitSpecialPickupEntity      types 024/118 special ammo
 *   InitBouncableClayEntity      type 045 bounce clay
 *   InitFloatingPlatformEntity   types 039/052 floating platform
 *   InitTriggerZoneEntity        types 085/104/105 teleporter/trigger zone
 *   InitCameraTrackingEntity     type 095 panning sound emitter
 *
 * All transcribed from export/SLES_010.90.c with ctor arities corrected
 * against the .s (InitEntityWithSprite takes (e, def, z, x, y-1); the export
 * drops the trailing three args). Per-frame behaviors (tick/render callbacks)
 * may still be weak stubs -- they log via PORT_STUBS_NONFATAL and are the
 * next conversion layer.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include "Game/entity.h"
#include "Game/spracc_records.h"
#include <stdint.h>

extern void *g_pBlbHeapBase;
extern u8 *AllocateFromHeap(void *heap, u32 size, u32 count, u8 flag);
extern void InitEntityWithSprite(void *entity, s32 spawnData, s16 zOrder, s16 worldX, s16 worldY);
extern void SetupEntityScaleCallbacks(Entity *e);
extern void GetEntitySpawnData(u8 *base, u32 layerIndex, u8 **outPtr, u16 *outWidth);
extern void InterpolateTimedPathPosition(u16 *time, s16 *out, u8 *pathData, s16 duration, s32 steps);
extern void EntitySetState(Entity *e, u32 marker, EntityCallback fn);
extern u32 EntityApplyMovementCallbacks(Entity *e, s16 x, s16 y);
extern u8 GetSlopeHeightAtSubpixel(void *gs, u32 tileIdx, s32 worldX);
extern s16 GetEntityXPosition(Entity *e);   /* real C, src/anim.c */
extern s16 GetEntityYPosition(Entity *e);   /* real C, src/anim.c */
extern s16 TransformXCoord(Entity *e, s16 val);  /* real C, src/anim.c */
extern s16 TransformYCoord(Entity *e, s16 val);  /* real C, src/anim.c */
extern s32 CheckTriggerZoneCollision(void *gs, s16 x, s16 y, s32 *outZoneId, void *outZoneRec);
extern s32 HandleGenericTriggerZone(s32 unused, u8 zoneId, u8 *outR, u8 *outG, u8 *outB);
extern void InitMenuEntityWithVtable(void *entity, s32 allocSize);
extern s32 PlayGameSoundById();
extern void AddEntityToSortedRenderList(void *gs, void *entity);

/* still-asm callbacks installed by these inits (weak stubs until converted) */
extern void CollectibleTickCallback();
extern void EntityEventHandlerWalk();
extern void ApplyAnimationPositionOffsets();
extern void EntityPathMovementUpdate();
extern void TeleporterTickCallback();
extern void SoundEmitterWithPanningTick();

/* vtables (strong data from gen_port_data) */
extern u8 COLLECTIBLE_ENTITY_VTABLE[] asm("D_80010DE4");
extern u8 VT_COLLECTIBLE_VARIANT[]   asm("D_80010C84");
extern u8 VT_SPECIAL_PICKUP[]        asm("D_80010D84");
extern u8 VT_BOUNCE_CLAY[]           asm("D_80011128");
extern u8 VT_FLOATING_PLATFORM[]     asm("D_80011208");
extern u8 VT_TRIGGER_ZONE[]          asm("D_800110A8");
extern u8 VT_CAMERA_TRACKING[]       asm("D_80011048");

/* sprite-def lists (strong data, sprite_def_lists.c) */
extern u32 D_8009B4DC[];
extern u32 D_8009B54C[];
extern u32 D_8009B6A4[];
extern u32 D_8009B6BC[];
extern u32 D_8009B6CC[];
extern u32 D_8009B6D8[];
extern u32 D_8009B844[];

/* FSM state slots (strong data, src/enemies.c sdata island) */
extern u32 FSM_800A5A78 asm("D_800A5A78"); extern EntityCallback FSM_800A5A7C asm("D_800A5A7C");
extern u32 FSM_800A5A80 asm("D_800A5A80"); extern EntityCallback FSM_800A5A84 asm("D_800A5A84");
extern u32 FSM_800A5AC8 asm("D_800A5AC8"); extern EntityCallback FSM_800A5ACC asm("D_800A5ACC");
extern u32 FSM_800A5B00 asm("D_800A5B00"); extern EntityCallback FSM_800A5B04 asm("D_800A5B04");
extern u32 FSM_800A5B38 asm("D_800A5B38"); extern EntityCallback FSM_800A5B3C asm("D_800A5B3C");
extern u32 FSM_800A5B40 asm("D_800A5B40"); extern EntityCallback FSM_800A5B44 asm("D_800A5B44");

/* --------------------------------------------------------------------------
 * Shared workers
 * ------------------------------------------------------------------------ */

void UpdateCollectibleTriggerZone(Entity *e) {
    s16 x = GetEntityXPosition(e);
    s16 y = GetEntityYPosition(e);
    s32 zoneId;
    u8 zoneRec[0x20];
    u8 r, g, b;

    if ((CheckTriggerZoneCollision(g_pGameState, x, y, &zoneId, zoneRec) & 0xFF) != 0 &&
        HandleGenericTriggerZone(0, (u8)zoneId, &r, &g, &b) != 0) {
        RenderSpriteObj *spr = (RenderSpriteObj *)e->spriteContext;
        spr->r = r;
        spr->g = g;
        spr->b = b;
    }
}

void InitCollectibleEntity(Entity *e, u8 *spawn) {
    u8 *tail = (u8 *)e;

    e->allocSize = 900;
    *(u8 **)(tail + 0x100) = spawn;
    *(u32 *)(tail + 0x104) = 0;
    *(u32 *)(tail + 0x108) = 0;
    tail[0x10C] = 0;
    tail[0x10D] = 0;
    e->tickMarker = -0x10000;
    e->tickCallback = (EntityCallback)CollectibleTickCallback;
    e->eventMarker = -0x10000;
    e->eventCallback = (EntityCallback)EntityEventHandlerWalk;
    e->collisionMask = 4;
    *(s16 *)(tail + 0x70) = 0;              /* targetX */
    SetupEntityScaleCallbacks(e);
    UpdateCollectibleTriggerZone(e);
}

s32 CalcEntityYFromTileHeight(Entity *e, u32 tileIdx, s16 worldX, s32 candidateY) {
    u8 slope;
    s32 y;

    candidateY = TransformYCoord(e, (s16)candidateY);
    worldX = TransformXCoord(e, worldX);
    slope = GetSlopeHeightAtSubpixel(g_pGameState, tileIdx & 0xFF, worldX);
    y = (s32)((((u32)candidateY | 0xF) - slope) << 16) / e->scalePowerupY;
    if (e->scalePowerupX == 0xC000 && ((u32)y & 3) - 1 < 2) {
        y = y + 1;
    }
    return (s16)y;
}

/* --------------------------------------------------------------------------
 * Ground snap helper used by the inits: apply movement transforms and snap Y
 * to the tile height when standing on a solid (< 0x3C) tile.
 * ------------------------------------------------------------------------ */
static void snap_to_ground(Entity *e) {
    u32 t = EntityApplyMovementCallbacks(e, e->worldX, (s16)(e->worldY + 2)) & 0xFF;
    if (t != 0 && t < 0x3C) {
        e->worldY = (s16)CalcEntityYFromTileHeight(e, t, e->worldX,
                                                   (s32)(s16)((u16)e->worldY + 2));
    }
}

/* --------------------------------------------------------------------------
 * Entity-type inits
 * ------------------------------------------------------------------------ */

Entity *InitCollectibleVariant(Entity *e, u8 *spawn) {
    u8 *tail = (u8 *)e;
    s16 rawType;

    InitEntityWithSprite(e, (s32)(uintptr_t)D_8009B54C, 0x3CA,
                         *(s16 *)(spawn + 8), (s16)(*(s16 *)(spawn + 10) - 1));
    e->collisionVtable = COLLECTIBLE_ENTITY_VTABLE;
    InitCollectibleEntity(e, spawn);
    e->collisionVtable = VT_COLLECTIBLE_VARIANT;
    tail[0x10D] = 1;
    tail[0x10C] = 0;
    tail[0x110] = 0;
    rawType = *(s16 *)(*(u8 **)(tail + 0x100) + 0x12);
    if (rawType == 0x6A) {
        tail[0x110] = 1;
    } else if (rawType == 0x6B) {
        tail[0x110] = 2;
    }
    snap_to_ground(e);
    EntitySetState(e, FSM_800A5AC8, FSM_800A5ACC);
    return e;
}

Entity *InitSpecialPickupEntity(Entity *e, u8 *spawn) {
    u8 *tail = (u8 *)e;
    u32 phase;
    u32 marker;
    EntityCallback fn;

    InitEntityWithSprite(e, (s32)(uintptr_t)D_8009B4DC, 0x3CA,
                         *(s16 *)(spawn + 8), (s16)(*(s16 *)(spawn + 10) - 1));
    e->collisionVtable = COLLECTIBLE_ENTITY_VTABLE;
    InitCollectibleEntity(e, spawn);
    e->collisionVtable = VT_SPECIAL_PICKUP;
    tail[0x112] = 0;
    if (*(s16 *)(*(u8 **)(tail + 0x100) + 0x12) == 0x76) {
        RenderSpriteObj *spr = (RenderSpriteObj *)e->spriteContext;
        tail[0x112] = 1;
        spr->r = 0x40;
        spr->g = 200;
        spr->b = 100;
    }
    tail[0x10D] = 1;
    e->renderMarker = -0x10000;
    e->renderCallback = (EntityCallback)ApplyAnimationPositionOffsets;
    phase = *(u32 *)(*(u8 **)(tail + 0x100) + 0xC) & 0x7F;
    tail[0x110] = (u8)phase;
    marker = FSM_800A5A80;
    fn = FSM_800A5A84;
    if (((g_pGameState->frame_counter & 0x7F) - phase) < 2) {
        marker = FSM_800A5A78;
        fn = FSM_800A5A7C;
    }
    EntitySetState(e, marker, fn);
    snap_to_ground(e);
    return e;
}

Entity *InitBouncableClayEntity(Entity *e, u8 *spawn) {
    u8 *tail = (u8 *)e;
    u32 group;

    InitEntityWithSprite(e, (s32)(uintptr_t)D_8009B6BC, 0x3CA,
                         *(s16 *)(spawn + 8), *(s16 *)(spawn + 10));
    e->collisionVtable = VT_BOUNCE_CLAY;
    e->allocSize = 900;
    *(u8 **)(tail + 0x100) = spawn;
    SetupEntityScaleCallbacks(e);
    tail[0x110] = 0;
    *(s16 *)(tail + 0x104) = e->worldX;
    *(s16 *)(tail + 0x106) = e->worldY;
    group = *(u16 *)(spawn + 0xC);
    if (group != 0) {
        u8 **wpp = (u8 **)(tail + 0x108);
        s16 *countp = (s16 *)(tail + 0x10C);
        e->renderMarker = -0x10000;
        e->renderCallback = (EntityCallback)EntityPathMovementUpdate;
        GetEntitySpawnData((u8 *)g_pGameState, group, wpp, (u16 *)countp);
        if (*countp == 0) {
            group = 0;
        } else {
            s16 out[2];
            *(u16 *)(tail + 0x10E) = (u16)(g_pGameState->frame_counter %
                                           (u32)(*countp * 8));
            InterpolateTimedPathPosition((u16 *)(tail + 0x10E), out, *wpp, *countp, 8);
            e->worldX = *(s16 *)(tail + 0x104) + out[0];
            e->worldY = *(s16 *)(tail + 0x106) + out[1];
        }
    }
    if (group == 0) {
        *(u8 **)(tail + 0x108) = NULL;
        e->worldX = *(s16 *)(tail + 0x104);
        e->worldY = *(s16 *)(tail + 0x106);
    }
    EntitySetState(e, FSM_800A5B00, FSM_800A5B04);
    e->collisionMask = 0;
    *(s16 *)(tail + 0x70) = 0;              /* targetX */
    return e;
}

Entity *InitFloatingPlatformEntity(Entity *e, u8 *spawn) {
    extern Entity *CreateCollectibleFromSpawn(); /* real C, src/enemies.c */
    u8 *tail = (u8 *)e;
    u32 marker;
    EntityCallback fn;

    ((Entity *(*)(Entity *, u8 *, u32 *))CreateCollectibleFromSpawn)(e, spawn, D_8009B844);
    e->collisionVtable = VT_FLOATING_PLATFORM;
    marker = FSM_800A5B40;
    fn = FSM_800A5B44;
    if (*(s16 *)(*(u8 **)(tail + 0x100) + 0x12) == 0x34) {
        marker = FSM_800A5B38;
        fn = FSM_800A5B3C;
    }
    EntitySetState(e, marker, fn);
    e->collisionMask = 0;
    *(s16 *)(tail + 0x70) = 0;              /* targetX */
    snap_to_ground(e);
    return e;
}

Entity *InitTriggerZoneEntity(Entity *e, u8 *spawn) {
    u8 *tail = (u8 *)e;
    s32 zoneId;
    u8 zoneRec[0x20];
    s16 x, y;
    s16 rawType;
    int deathZone = 0, altZone = 0;
    RenderSpriteObj *spr;

    InitEntityWithSprite(e, (s32)(uintptr_t)D_8009B6A4, 0x3CA,
                         *(s16 *)(spawn + 8), (s16)(*(s16 *)(spawn + 10) - 1));
    e->collisionVtable = VT_TRIGGER_ZONE;
    e->allocSize = 900;
    *(u8 **)(tail + 0x100) = spawn;
    e->tickMarker = -0x10000;
    e->tickCallback = (EntityCallback)TeleporterTickCallback;
    SetupEntityScaleCallbacks(e);
    tail[0x10D] = 0;
    tail[0x10E] = 0;
    *(u32 *)(tail + 0x108) = *(u32 *)((u8 *)g_pGameState->player_entity_alt + 0x50);

    x = GetEntityXPosition(e);
    y = GetEntityYPosition(e);
    if ((CheckTriggerZoneCollision(g_pGameState, x, (s16)(y + 0x10), &zoneId, zoneRec) & 0xFF) != 0) {
        if (zoneId == 0x2C) {
            deathZone = 1;
        } else if (zoneId == 0x2E) {
            altZone = 1;
        }
    }
    *(void **)(tail + 0x104) = NULL;
    if (altZone) {
        spr = (RenderSpriteObj *)e->spriteContext;
        spr->r = 0x74;
        spr->g = 0x24;
        spr->b = 0x38;
    } else if (deathZone) {
        spr = (RenderSpriteObj *)e->spriteContext;
        spr->r = 0x38;
        spr->g = 0x38;
        spr->b = 0x60;
    }
    rawType = *(s16 *)(*(u8 **)(tail + 0x100) + 0x12);
    if (rawType == 0x68 || rawType == 0x69) {
        Entity *child;
        u32 *def;
        if (rawType == 0x68) {
            spr = (RenderSpriteObj *)e->spriteContext;
            spr->r = 0x38;
            spr->g = 0x38;
            spr->b = 0x60;
            def = D_8009B6CC;
        } else {
            spr = (RenderSpriteObj *)e->spriteContext;
            spr->r = 0x40;
            spr->g = 0x40;
            spr->b = 0x60;
            def = D_8009B6D8;
        }
        child = (Entity *)AllocateFromHeap(g_pBlbHeapBase, 0x100, 1, 0);
        InitEntityWithSprite(child, (s32)(uintptr_t)def, 0x3CB,
                             *(s16 *)(spawn + 8), (s16)(*(s16 *)(spawn + 10) - 1));
        *(Entity **)(tail + 0x104) = child;
        SetupEntityScaleCallbacks(child);
        AddEntityToSortedRenderList(g_pGameState, *(Entity **)(tail + 0x104));
    }
    return e;
}

Entity *InitCameraTrackingEntity(Entity *e, u8 *spawn) {
    s16 x;

    InitMenuEntityWithVtable(e, 0x44C);
    e->collisionVtable = VT_CAMERA_TRACKING;
    e->renderMarker = (s32)(uintptr_t)spawn;
    x = *(s16 *)(spawn + 8);
    *(s16 *)&e->moveMarkerX = x;
    e->renderCallback = (EntityCallback)(uintptr_t)
        ((s32 (*)(u16, s32))PlayGameSoundById)(*(u16 *)((u8 *)(uintptr_t)e->renderMarker + 0xC),
                                               x - g_pGameState->camera_x);
    e->tickMarker = -0x10000;
    e->tickCallback = (EntityCallback)SoundEmitterWithPanningTick;
    return e;
}
