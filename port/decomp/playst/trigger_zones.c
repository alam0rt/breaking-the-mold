/* =============================================================================
 * trigger_zones.c  --  PC-port player trigger-zone processing
 * =============================================================================
 * Functional-C reconstructions of:
 *   CheckTriggerZoneCollision  (0x800245BC, 0xBC; export/SLES_010.90.c)
 *   PlayerProcessTileCollision (0x8005A914, 0x440; export 70290)
 *
 * NAME CHECK: "PlayerProcessTileCollision" is a clean-room misnomer -- it does
 * NO tile collision. It resolves the player's transformed world position and
 * dispatches the TRIGGER ZONE it stands in (rect list at GameState+0x74/+0x78,
 * BLB asset 602): level exit (zone 0 -> direct_level_load), game-mode switch
 * (2..7), crouch-slide chute (0x2A), wind/conveyor pushes (0x3D..0x41),
 * autoscroll toggles (0x51/0x52/0x65/0x66/0x79/0x7A), clayball milestone
 * flags (0x32..0x3B), and generic RGB tint zones (default). Real tile
 * collision lives in the PlayerCallback_* physics handlers. A better name
 * would be PlayerProcessTriggerZones.
 *
 * Zone record (16 bytes): {s16 left, top, right, bottom; s32 attr; s32 data}.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include "Game/game_state.h"
#include "Game/player_state.h"
#include <stdint.h>

extern void EntitySetState(void *entity, u32 marker, u32 callback);
extern void SetGameMode(u8 mode);
extern void PlayEntityPositionSound(void *entity, u32 soundId);
extern u32  PlaySoundEffect(u32 id, s16 pan, s8 force);
extern void StopSPUVoice(s32 handle);
extern void SetSpawnOffsetGroup1(s32 gs, u8 mode);
extern void SetSpawnOffsetGroup2(s32 gs, s32 mode);
extern void HandleGenericTriggerZone(void *gs, s8 zone, u8 *r, u8 *g, u8 *b);

/* state callbacks compared by address */
extern char PlayerState_DeathStart[];
extern char PlayerState_Jump[];
extern char PlayerState_Falling[];

/* crouch-slide FSM pair (strong C data in src/playst.c) */
extern u32 D_800A5D88 asm("D_800A5D88");  extern void *D_800A5D8C asm("D_800A5D8C");

typedef s32 (*CoordFn)(void *base, s32 coord);

s32 CheckTriggerZoneCollision(void *gameState, s16 x, s16 y,
                              s32 *out_attr, s32 *out_data) {
    u8 *gs = (u8 *)gameState;
    u8 *zone = *(u8 **)(gs + 0x74);
    u16 count = *(u16 *)(gs + 0x78);
    u16 i;

    for (i = 0; i < count; i++, zone += 16) {
        if (*(s16 *)(zone + 0) <= x && x < *(s16 *)(zone + 4) &&
            *(s16 *)(zone + 2) <= y && y < *(s16 *)(zone + 6)) {
            *out_attr = *(s32 *)(zone + 8);
            *out_data = *(s32 *)(zone + 12);
            return 1;
        }
    }
    return 0;
}

/* Resolve a coordinate through the entity's move-transform FSM slot. */
static s16 zone_coord(u8 *e, s32 hiOff, s32 fnOff, s32 loOff, s16 coord) {
    s16 hi = *(s16 *)(e + hiOff);
    CoordFn fn;
    s32 adj;

    if (hi == 0) {
        return coord;
    }
    adj = *(s16 *)(e + loOff);
    if (hi < 1) {
        fn = *(CoordFn *)(e + fnOff);
    } else {
        s32 base = *(s32 *)(e + *(s16 *)(e + fnOff));
        s32 slot = hi * 8 + base;
        adj += *(s16 *)((u8 *)(intptr_t)slot - 8);
        fn = *(CoordFn *)((u8 *)(intptr_t)slot - 4);
    }
    return (s16)fn(e + adj, (s32)coord);
}

void PlayerProcessTileCollision(void *arg0) {
    u8 *e = (u8 *)arg0;
    s16 x, y;
    s32 zoneType;
    s32 zoneData;
    u8  zoneArg;

    /* no zones while the death-start state is active */
    if (*(s16 *)(e + 0xA2) == -1 &&
        *(void **)(e + 0xA4) == (void *)PlayerState_DeathStart) {
        return;
    }

    x = zone_coord(e, 0x26, 0x28, 0x24, *(s16 *)(e + 0x68));
    y = zone_coord(e, 0x2E, 0x30, 0x2C, *(s16 *)(e + 0x6A));

    if ((CheckTriggerZoneCollision(g_pGameState, x, y, &zoneType, &zoneData) & 0xFF) == 0) {
        return;
    }
    zoneArg = (u8)zoneData;

    switch (zoneType) {
    case 0:                                     /* level exit / warp */
        g_pGameState->direct_level_load = zoneArg;
        break;
    case 2: case 3: case 4: case 5: case 6: case 7:   /* game-mode zones */
        if ((u32)(zoneType - 2) != e[0x1B3]) {
            u8 mode = (u8)(zoneType - 2);
            SetGameMode(mode);
            e[0x1B3] = mode;
            /* mid-jump mode change with the jump voice active: swap SFX */
            if (*(s32 *)(e + 0x174) >= 0 && *(s16 *)(e + 0xA2) == -1 &&
                *(void **)(e + 0xA4) == (void *)PlayerState_Jump &&
                *(u32 *)(e + 0xCC) == 0x092B8480u) {
                u32 v = PlaySoundEffect(0x248E52, 0xA0, 0);
                StopSPUVoice(*(s32 *)(e + 0x174));
                *(s32 *)(e + 0x174) = (s32)v;
            }
        }
        break;
    case 0x2A:                                  /* crouch-slide chute */
        if (*(s16 *)(e + 0xA2) == -1 &&
            (*(void **)(e + 0xA4) == (void *)PlayerState_Falling ||
             *(void **)(e + 0xA4) == (void *)PlayerState_Jump)) {
            EntitySetState(e, D_800A5D88, (u32)(uintptr_t)D_800A5D8C);
        }
        break;
    case 0x3D: *(s16 *)(e + 0x160) = -1; break;         /* wind left  */
    case 0x3E: *(s16 *)(e + 0x160) = 1;  break;         /* wind right */
    case 0x3F:                                          /* diag left  */
        *(s16 *)(e + 0x160) = -2;
        if (e[0x170] != 0) *(s16 *)(e + 0x162) = -1;
        break;
    case 0x40:                                          /* diag right */
        *(s16 *)(e + 0x160) = 2;
        if (e[0x170] != 0) *(s16 *)(e + 0x162) = -1;
        break;
    case 0x41: *(s16 *)(e + 0x162) = -4; break;         /* updraft    */
    case 0x51: SetSpawnOffsetGroup1((s32)(uintptr_t)g_pGameState, 1);
               *(s16 *)(e + 0x1A6) = 1; break;
    case 0x52: SetSpawnOffsetGroup2((s32)(uintptr_t)g_pGameState, 1);
               *(s16 *)(e + 0x1A8) = 1; break;
    case 0x65: SetSpawnOffsetGroup1((s32)(uintptr_t)g_pGameState, 0);
               *(s16 *)(e + 0x1A6) = 0; break;
    case 0x66: SetSpawnOffsetGroup2((s32)(uintptr_t)g_pGameState, 0);
               *(s16 *)(e + 0x1A8) = 0; break;
    case 0x79: SetSpawnOffsetGroup1((s32)(uintptr_t)g_pGameState, 2);
               *(s16 *)(e + 0x1A6) = 2; break;
    case 0x7A: SetSpawnOffsetGroup2((s32)(uintptr_t)g_pGameState, 2);
               *(s16 *)(e + 0x1A8) = 2; break;
    default:
        if ((u32)(zoneType - 0x32) < 10) {              /* clayball flags */
            u8 *flag = &g_pPlayerState->clayball_flag_0 + (u32)(zoneType - 0x32);
            if (*flag == 0) {
                *flag = 1;
                PlayEntityPositionSound(e, 0x7003474C);
            }
        } else if (e[0x1AE] == 0) {                     /* RGB tint zone */
            HandleGenericTriggerZone(g_pGameState, (s8)zoneType,
                                     e + 0x15D, e + 0x15E, e + 0x15F);
        }
        break;
    }
}
