/* =============================================================================
 * CreatePlayerEntity.c  --  PC-port platformer player constructor
 * =============================================================================
 * Functional-C reconstructions of:
 *   CreatePlayerEntity          (asm/nonmatchings/player/CreatePlayerEntity.s,
 *                                0x800596A4, 0x3CC; export/SLES_010.90.c 69171)
 *   InitPlayerSpriteAvailability (asm 0x80059A70; export 69340)
 *
 * ABI notes resolved against the .s (Ghidra's 5-param signature is a register
 * artifact): the real call into InitEntityWithSprite is the plain 5-arg form
 * (entity, D_8009C174, 0x3E8, spawn_x, spawn_y), and `facing` is the caller's
 * 5th argument (lbu 0x48($sp)). The FSM state pairs installed at the end are
 * the src/player.c data island D_800A5D20..D_800A5D34 (Idle / HideAndClear-
 * Bounce / Callback_2), already strong C definitions in the port build.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include "Game/game_state.h"
#include "Game/player_state.h"
#include <stdint.h>

extern void InitEntityWithSprite(void *entity, s32 spawnData, s16 zOrder, s16 worldX, s16 worldY);
extern s32  InitSpriteContext(void *ctx, u32 spriteId);
extern void EntitySetState(void *entity, u32 marker, u32 callback);
extern void SetGameMode(u8 mode);
extern u8  *AllocateFromHeap(u8 *heap, s32 elemSize, s32 count, s32 flags);
extern void CreateHaloEntity(void *entity, u32 player);
extern void AddToZOrderList(void *gs, s32 entity);
extern void AddToXPositionList(void *gs, s32 renderCtx);

extern char g_PlayerCallbackTable[];
extern char ApplyEntityPositionUpdate[];
extern char PlayerTickCallback[];
extern char ScaleXByEntityScale[];
extern char ScaleYByEntityScale[];
extern u8   D_8009C174[] asm("D_8009C174");   /* player spawn spriteDef blob */
extern u32  D_8009C3A8[] asm("D_8009C3A8");   /* 7 pose-sprite candidates    */

/* player-state FSM slot pairs (src/player.c data island) */
extern u32 D_800A5D20 asm("D_800A5D20");  extern void *D_800A5D24 asm("D_800A5D24");
extern u32 D_800A5D28 asm("D_800A5D28");  extern void *D_800A5D2C asm("D_800A5D2C");
extern u32 D_800A5D30 asm("D_800A5D30");  extern void *D_800A5D34 asm("D_800A5D34");

/* Probe the 7 pose-sprite candidates; record the available ones (context
 * builds with nonzero max width/height) at +0x180, count at +0x19C. */
void InitPlayerSpriteAvailability(void *arg0) {
    u8 *e = (u8 *)arg0;
    u8  ctx[0x18];
    s32 i;

    e[0x19D] = 0xFF;               /* currentSpriteIndex sentinel */
    e[0x19C] = 0;                  /* availableSpriteCount */
    for (i = 0; i < 7; i++) {
        u32 id = D_8009C3A8[i];
        InitSpriteContext(ctx, id);
        if (*(s16 *)(ctx + 0xC) != 0 && *(s16 *)(ctx + 0xE) != 0) {
            *(u32 *)(e + 0x180 + (u32)e[0x19C] * 4) = id;
            e[0x19C]++;
        }
    }
}

void *CreatePlayerEntity(void *arg0, void *input, s16 x, s16 y, u8 facing) {
    u8 *e = (u8 *)arg0;
    GameState *gs;
    s32 scale, v;
    u32 marker;
    void *fn;

    InitEntityWithSprite(e, (s32)(uintptr_t)D_8009C174, 0x3E8, x, y);
    *(void **)(e + 0x18) = g_PlayerCallbackTable;
    *(s16 *)(e + 0x10) = 1000;                     /* allocSize / sort key */
    *(void **)(e + 0x100) = input;                 /* pInput */
    *(s32 *)(e + 0x174) = -1;                      /* soundHandle */
    *(u32 *)(e + 0x1C) = 0xFFFF0000u;              /* renderMarker */
    *(void **)(e + 0x20) = (void *)ApplyEntityPositionUpdate;
    *(u32 *)(e + 0x00) = 0xFFFF0000u;              /* tickMarker */
    *(void **)(e + 0x04) = (void *)PlayerTickCallback;
    *(u32 *)(e + 0x24) = 0xFFFF0000u;              /* moveMarkerX */
    *(void **)(e + 0x28) = (void *)ScaleXByEntityScale;
    *(u32 *)(e + 0x2C) = 0xFFFF0000u;              /* moveMarkerY */
    *(void **)(e + 0x30) = (void *)ScaleYByEntityScale;

    scale = 0x8000;
    if (g_pPlayerState->shrink_mode == 0) {
        scale = (s32)g_pGameState->camera_scroll_speed;
    }
    *(s32 *)(e + 0x58) = scale;                    /* scalePowerupX */
    *(s32 *)(e + 0x5C) = scale;                    /* scalePowerupY */
    *(s32 *)(e + 0x54) = scale;                    /* scaleRender2 */
    *(s32 *)(e + 0x50) = scale;                    /* scaleRender */
    *(s16 *)(e + 0x68) = (s16)(((s32)*(s16 *)(e + 0x68) << 16) / scale);  /* worldX */
    v = ((s32)*(s16 *)(e + 0x6A) << 16) / scale;
    *(s16 *)(e + 0x6A) = (s16)v;                                          /* worldY */
    if (scale == 0xC000 && ((u32)v & 3) - 1 < 2) {
        *(s16 *)(e + 0x6A) = (s16)(v + 1);         /* 0.75-scale rounding nudge */
    }

    InitPlayerSpriteAvailability(e);

    e[0x1B3] = 0xFF;       /* gameMode */
    e[0x1AE] = 0;          /* damageFlag */
    e[0x1AF] = 0;          /* particleFlag */
    e[0x1B0] = 0;          /* shrinkFlag */
    e[0x1B1] = 0;          /* portalCheatFlag */
    e[0x178] = 0;          /* disableScale */
    e[0x1B2] = 0;          /* levelExitFlag */
    e[0x128] = 0;          /* invincibilityTimer */
    e[0x13C] = 0;          /* timer13C */
    e[0x134] = 0;          /* specialMoveQueued */
    e[0x135] = 0;          /* specialMoveMode */
    gs = g_pGameState;
    *(s16 *)(e + 0x1A6) = 0;    /* scrollFlagX */
    *(s16 *)(e + 0x1A8) = 0;    /* scrollFlagY */
    *(s32 *)(e + 0x10C) = 0;    /* carryMotionX */
    *(s32 *)(e + 0x110) = 0;    /* velocityY (16.16) */
    *(s32 *)(e + 0x114) = 0;    /* velocityX (16.16) */
    *(s32 *)(e + 0x118) = 0;    /* cushionVelY */
    *(s16 *)(e + 0x160) = 0;    /* pushX */
    *(s16 *)(e + 0x162) = 0;    /* pushY */
    *(void **)(e + 0x12C) = NULL;  /* interactEntity */
    *(s16 *)(e + 0x136) = 0;    /* apexVelocity */
    *(s32 *)(e + 0xB4) = 0;     /* frameMotionX */
    *(s32 *)(e + 0xB8) = 0;     /* frameMotionY */
    e[0x15D] = gs->layer0_tint_r;   /* baseRGB */
    e[0x15E] = gs->layer0_tint_g;
    e[0x15F] = gs->layer0_tint_b;
    e[0x17D] = 0;               /* rgbCooldown */
    *(void **)(e + 0x140) = NULL;  /* swirlPortalEntity */
    e[0x159] = 0;               /* pendingStateChange */
    e[0x164] = 0;               /* airJumpMode */
    *(s16 *)(e + 0x144) = 0;    /* powerupTimer */
    *(void **)(e + 0x14C) = NULL;  /* hudEntity */
    *(s16 *)(e + 0x156) = 0;    /* jumpParam */

    if (gs->spawn_freeze_flag == 0) {
        marker = D_800A5D30;    /* PlayerStateCallback_2 pair */
        fn     = D_800A5D34;
        if (facing != 0) {
            marker = D_800A5D28;    /* PlayerState_HideAndClearBounce pair */
            fn     = D_800A5D2C;
        }
    } else {
        u8 mode = gs->_reserved_198;
        e[0x1B3] = mode;
        SetGameMode(mode);
        marker = D_800A5D20;    /* PlayerStateInit_Idle pair */
        fn     = D_800A5D24;
        e[0x15D] = gs->bg_color_r;
        e[0x15E] = gs->bg_color_g;
        e[0x15F] = gs->bg_color_b;
    }
    EntitySetState(e, marker, (u32)(uintptr_t)fn);

    *(void **)(e + 0x168) = NULL;   /* haloEntity */
    if ((g_pPlayerState->powerup_flags & 1) != 0) {
        u8 *halo = AllocateFromHeap((u8 *)g_pBlbHeapBase, 0x30, 1, 0);
        CreateHaloEntity(halo, (u32)(uintptr_t)e);
        *(void **)(e + 0x168) = halo;
        AddToZOrderList(g_pGameState, (s32)(uintptr_t)halo);
        AddToXPositionList(g_pGameState, *(s32 *)(halo + 0x24));
    }
    *(void **)(e + 0x16C) = NULL;   /* glideEntity */
    e[0x170] = 0;                   /* groundedFlag */
    *(void **)(e + 0x1A0) = NULL;   /* attachedEntity */
    return e;
}
