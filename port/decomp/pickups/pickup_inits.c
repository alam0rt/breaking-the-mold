/* =============================================================================
 * pickup_inits.c  --  PC-port level-1 pickup/decor entity inits
 * =============================================================================
 * Functional-C bodies for InitPlatformDecorEntity (type 009 moving-platform
 * decor) and InitDecorEntityWithHUDIcon (type 022 orb pickup with HUD icon),
 * plus the small InitHUDIconEntity worker (src/effects.c). Transcribed from
 * export/SLES_010.90.c with ctor arities from the src plate comments
 * (InitEntityWithSprite(e, D_8009B214, 0x3DE, spawn->x, spawn->y)).
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include "Game/entity.h"
#include "Game/spracc_records.h"
#include <stdint.h>

extern void *g_pBlbHeapBase;
extern u8 *AllocateFromHeap(void *heap, u32 size, u32 count, u8 flag);
extern void InitEntityWithSprite(void *entity, s32 spawnData, s16 zOrder, s16 worldX, s16 worldY);
extern Entity *InitEntitySprite(Entity *entity, u32 spriteId, s32 z, s16 x, s16 y, s32 flags);
extern void InitPathFollowingDecorEntity(void *e, void *data, u8 flag);
extern void EntitySetState(Entity *e, u32 marker, EntityCallback fn);
extern u32 EntityApplyMovementCallbacks(Entity *e, s16 x, s16 y);
extern s32 CalcEntityYFromTileHeight(Entity *e, u32 tileIdx, s16 worldX, s32 candidateY);
extern s16 TransformXCoord(Entity *e, s16 val);  /* real C, src/anim.c */
extern s16 TransformYCoord(Entity *e, s16 val);  /* real C, src/anim.c */
extern void SetAnimationActive(void *e, u8 active);      /* real C, src/anim.c */
extern void AddToXPositionList();                        /* still-asm (weak stub) */
extern void CollectibleOrbTickCallback();                /* still-asm (weak stub) */
extern void CheckpointSwampTickCallback();               /* still-asm (weak stub) */
extern void InitBasicEntityWithVtable(void *p, u16 val); /* real C, src/sprite.c */

extern u8 VT_PATH_DECOR[]      asm("D_80010870");
extern u8 VT_PLATFORM_DECOR[]  asm("D_800107D0");
extern u8 VT_ORB_PICKUP[]      asm("D_80010830");
extern u8 VT_HUD_ICON_SHORT[]  asm("D_80010B18");

extern u32 D_8009B214[];       /* sprite list, sprite_def_lists.c */

extern u32 FSM_800A59A8 asm("D_800A59A8"); extern EntityCallback FSM_800A59AC asm("D_800A59AC");
extern u32 FSM_800A59B0 asm("D_800A59B0"); extern EntityCallback FSM_800A59B4 asm("D_800A59B4");

u16 *InitHUDIconEntity(u16 *entity, u8 smallSize) {
    u8 *p = (u8 *)entity;

    InitBasicEntityWithVtable(entity, 999);
    p[0x1E0] = 2;
    *(u8 **)(p + 0xC) = VT_HUD_ICON_SHORT;
    p[0x1E7] = 0;
    p[0x1E6] = (u8)g_pGameState->frame_counter;
    if (smallSize == 0) {
        p[0x1E1] = 0x20;
        p[0x1E3] = 8;
        p[0x1E2] = 0x10;
        p[0x1E4] = 4;
        p[0x1E5] = 5;
    } else {
        p[0x1E1] = 0x10;
        p[0x1E3] = 4;
        p[0x1E2] = 0x10;
        p[0x1E4] = 4;
        p[0x1E5] = 0;
    }
    return entity;
}

Entity *InitDecorEntityWithHUDIcon(Entity *e, u8 *spawn) {
    u8 *tail = (u8 *)e;
    u16 *icon;

    InitEntitySprite(e, 0xB8700CA1, 0x3DE, *(s16 *)(spawn + 8), *(s16 *)(spawn + 10), 1);
    e->collisionVtable = VT_PATH_DECOR;
    InitPathFollowingDecorEntity(e, spawn, 0);
    e->collisionVtable = VT_ORB_PICKUP;
    e->tickMarker = -0x10000;
    e->tickCallback = (EntityCallback)CollectibleOrbTickCallback;
    SetAnimationActive(e, 0);
    ((u8 *)e->spriteContext)[10] = 0;       /* BasicEntity.active off */

    icon = (u16 *)AllocateFromHeap(g_pBlbHeapBase, 0x1E8, 1, 0);
    icon = InitHUDIconEntity(icon, 1);
    *(u16 **)(tail + 0x120) = icon;
    ((u8 *)icon)[0x1E7] = 1;
    ((void (*)(void *, u32))AddToXPositionList)(g_pGameState, *(u32 *)(tail + 0x120));
    tail[0x124] = 0;
    return e;
}

Entity *InitPlatformDecorEntity(Entity *e, u8 *spawn) {
    u8 *tail = (u8 *)e;
    u32 t;
    u8 atSpawn = 0;

    InitEntityWithSprite(e, (s32)(uintptr_t)D_8009B214, 0x3DE,
                         *(s16 *)(spawn + 8), *(s16 *)(spawn + 10));
    e->collisionVtable = VT_PATH_DECOR;
    InitPathFollowingDecorEntity(e, spawn, 0);
    e->collisionVtable = VT_PLATFORM_DECOR;
    e->collisionMask = 0;

    /* position from the spawn tile ref via sub-pixel divide by camera scale */
    e->worldX = (s16)((((u32)*(u16 *)(*(u8 **)(tail + 0x100) + 8) + 8) << 16) /
                      (u32)e->scalePowerupX);
    e->worldY = (s16)((((u32)*(u16 *)(*(u8 **)(tail + 0x100) + 10) + 0x10) << 16) /
                      (u32)e->scalePowerupY);

    e->tickMarker = -0x10000;
    e->tickCallback = (EntityCallback)CheckpointSwampTickCallback;

    t = EntityApplyMovementCallbacks(e, e->worldX, (s16)(e->worldY + 2)) & 0xFF;
    if (t != 0 && t < 0x3C) {
        e->worldY = (s16)CalcEntityYFromTileHeight(e, t, e->worldX,
                                                   (s32)(s16)((u16)e->worldY + 2));
    }

    /* is this platform at the player's spawn point? (checkpoint state) */
    if (g_pGameState->spawn_x == TransformXCoord(e, e->worldX) &&
        g_pGameState->spawn_y == TransformYCoord(e, e->worldY)) {
        atSpawn = 1;
    }
    tail[0x120] = atSpawn;
    tail[0x121] = 1;
    if (atSpawn) {
        EntitySetState(e, FSM_800A59A8, FSM_800A59AC);
    } else {
        EntitySetState(e, FSM_800A59B0, FSM_800A59B4);
    }
    return e;
}
