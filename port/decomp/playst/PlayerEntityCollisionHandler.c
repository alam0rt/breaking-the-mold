/* =============================================================================
 * PlayerEntityCollisionHandler.c  --  PC-port player collision/event handler
 * =============================================================================
 * Functional-C conversion of
 * asm/nonmatchings/playst/PlayerEntityCollisionHandler.s
 * (0x8005C400, 0x660 bytes; reference = export/SLES_010.90.c line 72004).
 *
 * The airborne/collision event handler for the player entity.  Dispatches on
 * (eventId & 0xFFFF):
 *   0x100C  PlayerEnterShrinkZone
 *   0x100D  PlayerExitShrinkZone
 *   0x100F  checkpoint-reposition  -> EntitySetState(CheckpointExit)
 *   0x1010  death-trigger          -> EntitySetState(LevelExitTeleporter) unless locked
 *   0x1017  can-act query          -> !deathMode && !stateLocked
 *   0x1007  CheckPlayerHitByEnemy
 *   0x1000  camera-target collision (land-on-enemy / die / bounce / quick-turn)
 *   0x100A  landing on a platform  (jump-from-platform + powerup scale copy)
 *
 * Offsets and %gp_rel D_800A5Dxx slot symbols verified against the .s.  Every
 * D_800A5Dxx slot is a {marker:u32, fn:ptr} pair seeded in
 * port/decomp/data/player_state_slots.c (marker is always 0xFFFF0000).
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include <stdint.h>

/* ---- callees ------------------------------------------------------------- */
extern void PlayerEnterShrinkZone(void *entity, u32 eventId, s16 arg);
extern void PlayerExitShrinkZone(void *entity, u32 eventId, s16 arg);
extern void EntitySetState(void *entity, u32 newMarker, u32 newCallback);
extern void CheckPlayerHitByEnemy(void *entity, void *source, int arg3);
extern void GetEntityScreenBounds(void *ctx, void *outBounds);

/* ---- state-fn identity targets (compared against entity[+0xA4]) ---------- */
extern void PlayerStateInit_JumpFromPlatform(void);
extern void PlayerStateInit_BounceJumpAnimation(void);

/* ---- FSM slot pairs (seeded in player_state_slots.c) --------------------- */
extern u32   D_800A5DB8; /* LevelExitTeleporter marker */
extern void *D_800A5DBC; /* LevelExitTeleporter fn     */
extern u32   D_800A5DC0; /* CheckpointExit marker      */
extern void *D_800A5DC4; /* CheckpointExit fn          */
extern u32   D_800A5DC8; /* JumpFromPlatform marker    */
extern void *D_800A5DCC; /* JumpFromPlatform fn        */
extern u32   D_800A5D50; /* SetupBounceDown marker     */
extern void *D_800A5D54; /* SetupBounceDown fn         */
extern u32   D_800A5D58; /* QuickTurn marker           */
extern void *D_800A5D5C; /* QuickTurn fn               */
extern u32   D_800A5D78; /* RespawnAtCheckpoint marker */
extern void *D_800A5D7C; /* RespawnAtCheckpoint fn     */
extern u32   D_800A5D80; /* Death marker               */
extern void *D_800A5D84; /* Death fn                   */

typedef int (*CbEvent)(void *ctx, int a1, int a2, void *a3);
typedef int (*CbCoord)(void *ctx, int coord);

/*
 * Resolve one of param4's callback slots.  Slot layout at byte `baseOff`:
 *   base     = s16 @ baseOff
 *   count    = s16 @ baseOff + 2
 *   tableRel = s16 @ baseOff + 4
 * count > 0 : entry = *(int*)(p + tableRel) + count*8; arg = entry[-2]; fn = entry[-1]
 * count < 0 : fn = *(void**)(p + baseOff + 4)
 * count == 0: returns NULL (caller substitutes a fallback coordinate)
 * The returned context pointer is (param4 + off), off = base (or arg16+base when count>0).
 */
static void *resolve_p4_cb(u8 *p, int baseOff, void **out_ctx) {
    s16 count = *(s16 *)(p + baseOff + 2);
    if (count == 0) {
        *out_ctx = (void *)p;
        return (void *)0;
    }
    void *fn;
    int argWord = 0;
    if (count > 0) {
        int tableRel = *(s16 *)(p + baseOff + 4);
        int entry = *(int *)(p + tableRel) + count * 8;
        argWord = *(int *)(entry - 8);
        fn = *(void **)(entry - 4);
    } else {
        fn = *(void **)(p + baseOff + 4);
    }
    s16 base = *(s16 *)(p + baseOff);
    int off = base;
    if (count > 0) {
        off = (s16)argWord + base;
    }
    *out_ctx = (void *)(p + off);
    return fn;
}

/* Shared "arm jump-from-platform" transition used by the 0x1000 and 0x100A
 * paths: bump the platform-jump counter when the player is mid jump/bounce,
 * otherwise reset it, then install the JumpFromPlatform state. */
static void install_jump_from_platform(u8 *e) {
    void *cur = *(void **)(e + 0xA4);
    if (*(s16 *)(e + 0xA2) == -1 &&
        (cur == (void *)PlayerStateInit_JumpFromPlatform ||
         cur == (void *)PlayerStateInit_BounceJumpAnimation)) {
        e[0x164] = (u8)(e[0x164] + 1);
    } else {
        e[0x164] = 0;
    }
    EntitySetState(e, D_800A5DC8, (u32)(uintptr_t)D_800A5DCC);
}

/* Faithful 16.16 divide with the two PS1 trap-guards demoted to safe values. */
static s16 scale_divide(int numerator, int denom) {
    if (denom == 0) {
        return 0; /* orig: trap(0x1c00) */
    }
    if (denom == -1 && numerator == (int)0x80000000) {
        return 0; /* orig: trap(0x1800) overflow */
    }
    return (s16)(numerator / denom);
}

int PlayerEntityCollisionHandler(void *entityArg, u32 eventId, int param3, int param4) {
    u8 *e = (u8 *)entityArg;
    u8 *p = (u8 *)(uintptr_t)param4;
    u32 ret = 0;
    u16 code = (u16)(eventId & 0xFFFF);

    switch (code) {
    case 0x100C:
        PlayerEnterShrinkZone(e, eventId, (s16)param3);
        break;
    case 0x100D:
        PlayerExitShrinkZone(e, eventId, (s16)param3);
        break;
    case 0x100F:
        *(s16 *)(e + 0x68) = *(s16 *)(p + 0x68);
        *(s16 *)(e + 0x6A) = (s16)(*(s16 *)(p + 0x6A) + 0x20);
        EntitySetState(e, D_800A5DC0, (u32)(uintptr_t)D_800A5DC4);
        break;
    case 0x1010:
        if (*(u8 *)(e + 0x178) != 0) {
            break;
        }
        if (*(u8 *)(e + 0x1B2) != 0) {
            break;
        }
        EntitySetState(e, D_800A5DB8, (u32)(uintptr_t)D_800A5DBC);
        break;
    case 0x1017:
        ret = 0;
        if (*(u8 *)(e + 0x178) == 0) {
            ret = (u32)(*(u8 *)(e + 0x1B2) == 0);
        }
        break;
    default:
        break;
    }

    if (code == 0x1007) {
        CheckPlayerHitByEnemy(e, p, param3);
        return (int)ret;
    }

    if (code < 0x1008) {
        if (code != 0x1000) {
            return (int)ret;
        }

        /* ---- 0x1000: camera-target collision -------------------------- */
        unsigned char boundsA[8];
        unsigned char boundsB[8];
        GetEntityScreenBounds(p, boundsA);
        GetEntityScreenBounds(e, boundsB);

        int isP3_2 = (param3 == 2);
        int isP3_1 = (param3 == 1);
        int landOnTarget;
        if (isP3_2) {
            landOnTarget = 1;
        } else if (isP3_1) {
            landOnTarget = 0;
        } else {
            int diff = (int)*(s16 *)(boundsA + 6) - (int)*(s16 *)(boundsB + 6);
            landOnTarget = (diff > 1);
        }

        if (landOnTarget) {
            if (*(int *)(e + 0x110) > 0) {
                *(u8 *)((u8 *)g_pGameState + 0x11A) = 0x14;
            }
            if (isP3_2 && *(int *)(e + 0x110) <= 0) {
                return (int)ret;
            }
            void *ctx;
            void *fn = resolve_p4_cb(p, 0x8, &ctx);
            if (fn != (void *)0) {
                ((CbEvent)fn)(ctx, 0x1001, 0, e);
            }
            install_jump_from_platform(e);
            return (int)ret;
        }

        /* ---- death / bounce sub-branch -------------------------------- */
        if (*(u8 *)(e + 0x128) != 0) {
            return 0;
        }
        u32 dmarker;
        u32 dfn;
        if ((*(u8 *)((u8 *)g_pPlayerState + 0x17) & 1) == 0) {
            /* not invincible: fall to death (or powerup-fall survival) */
            if (*(u16 *)(e + 0x144) != 0) {
                dmarker = D_800A5D78;
                dfn = (u32)(uintptr_t)D_800A5D7C;
            } else {
                dmarker = D_800A5D80;
                dfn = (u32)(uintptr_t)D_800A5D84;
            }
        } else {
            /* invincible: quick-turn if hit from the left, else bounce down */
            if (*(s16 *)(e + 0x68) < *(s16 *)(p + 0x68)) {
                dmarker = D_800A5D58;
                dfn = (u32)(uintptr_t)D_800A5D5C;
            } else {
                dmarker = D_800A5D50;
                dfn = (u32)(uintptr_t)D_800A5D54;
            }
        }
        EntitySetState(e, dmarker, dfn);
        return 1;
    }

    if (code == 0x100A && *(int *)(e + 0x110) > 0) {
        /* ---- 0x100A: landed on a platform ----------------------------- */
        install_jump_from_platform(e);

        if (*(u16 *)(e + 0x144) != 0) {
            /* Powerup active: copy scale/color state from the platform. */
            void *ctxX;
            void *fnX = resolve_p4_cb(p, 0x24, &ctxX);
            int resX;
            if (fnX != (void *)0) {
                resX = ((CbCoord)fnX)(ctxX, (int)(s16)*(u16 *)(p + 0x68));
            } else {
                resX = *(u16 *)(p + 0x68);
            }
            *(s16 *)(e + 0x146) =
                scale_divide((int)((unsigned)resX << 16), *(int *)(e + 0x58));

            void *ctxY;
            void *fnY = resolve_p4_cb(p, 0x2C, &ctxY);
            int resY;
            if (fnY != (void *)0) {
                resY = ((CbCoord)fnY)(ctxY, (int)(s16)*(u16 *)(p + 0x6A));
            } else {
                resY = *(u16 *)(p + 0x6A);
            }
            s16 yr = scale_divide((int)((unsigned)resY << 16), *(int *)(e + 0x5C));

            /* byte/halfword shuffle of the powerup scale + color fields */
            e[0x151] = e[0x15E];
            e[0x152] = e[0x15F];
            e[0x153] = e[0x1B3];
            e[0x150] = e[0x15D];
            *(u16 *)(e + 0x1AC) = *(u16 *)(e + 0x1A8);
            *(u16 *)(e + 0x1AA) = *(u16 *)(e + 0x1A6);
            *(s16 *)(e + 0x148) = yr;
            e[0x154] = *(u8 *)((u8 *)g_pPlayerState + 0x18);
        }

        /* runs whether or not the powerup block executed */
        *(u8 *)((u8 *)g_pGameState + 0x14B) = *(u8 *)((u8 *)g_pPlayerState + 0x18);
        *(u8 *)((u8 *)g_pGameState + 0x198) = e[0x1B3];
        *(u8 *)((u8 *)g_pGameState + 0x199) = e[0x15D];
        *(u8 *)((u8 *)g_pGameState + 0x19A) = e[0x15E];
        *(u8 *)((u8 *)g_pGameState + 0x19B) = e[0x15F];
        return 1;
    }

    return (int)ret;
}
