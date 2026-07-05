/* =============================================================================
 * PlayerTickCallback.c  --  PC-port player per-frame tick driver
 * =============================================================================
 * Functional-C reconstruction of PlayerTickCallback (0x8005B414, 0x6BC;
 * export/SLES_010.90.c 70907). The player's tick-slot callback, run once per
 * frame from EntityTickLoop. Order of operations:
 *   1. hamster-shield override: if g_GameFlags bit 0 and the current sprite
 *      (+0xCC) isn't the hamster sprite 0x1E28E0D4, switch state to the
 *      HamsterSpriteCallback pair (D_800A5D90/94).
 *   2. dispatch the input-state FSM slot (+0x104/+0x108).
 *   3. EntityUpdateCallback (base animation tick).
 *   4. shrink hitbox override; clear pushX/pushY.
 *   5. unless a checkpoint restore is in progress: tile collision
 *      (PlayerProcessTileCollision), invincibility/powerup timers (powerup
 *      expiry restores the palette and sets clutDirty +0xF8 so the CLUT
 *      re-uploads), pending-death transition (the +0x159 byte the header
 *      calls pendingStateChange is specifically a DEATH request), and halo /
 *      yellow-bird companion entity maintenance vs g_pPlayerState powerups.
 *   6. render-scale easing toward shrink (0x4000) / half (0x8000) / full.
 *   7. damage-flash / powerup-glow tint into currentRGB + sprite context
 *      (ctx +0x34..0x36 tint, +0x38 = STP semi-transparency toggle).
 *   8. particleFlag trail: spawn a PlayerParticle every 8th frame.
 *
 * Ghidra's 3-arg signature is register residue; the real callback is 1-arg.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include "Game/game_state.h"
#include "Game/player_state.h"
#include <stdint.h>

extern void EntitySetState(void *entity, u32 marker, u32 callback);
extern void EntityUpdateCallback(void *entity);
extern void PlayerProcessTileCollision(void *player);
extern void PlayEntityPositionSound(void *entity, u32 soundId);
extern u8  *AllocateFromHeap(u8 *heap, s32 elemSize, s32 count, s32 flags);
extern void CreateHaloEntity(void *entity, u32 player);
extern void *CreateYellowBirdEntity(void *entity, s32 player);
extern void CreatePlayerParticleEntity(void *entity, void *player);
extern void AddToZOrderList(void *gs, s32 entity);
extern void AddToXPositionList(void *gs, s32 renderCtx);
extern void RemoveEntityFromAllLists(s32 gs, s32 entity);
extern void RemoveFromTickList(void *gs, void *entity);
extern void RemoveFromRenderList(void *gs, s32 renderCtx);

/* playst FSM pairs (strong C data in src/playst.c) */
extern u32 D_800A5D90 asm("D_800A5D90");  extern void *D_800A5D94 asm("D_800A5D94");
extern u32 D_800A5D98 asm("D_800A5D98");  extern void *D_800A5D9C asm("D_800A5D9C");

typedef void (*TickFn)(void *base);

static u8 clamp255(s32 v) {
    if ((s16)v > 0xFF) return 0xFF;
    if ((s16)v < 0)    return 0;
    return (u8)v;
}

void PlayerTickCallback(void *arg0) {
    u8 *e = (u8 *)arg0;
    u8 *ctx;
    s16 hi;

    /* 1. hamster-shield sprite override */
    if ((g_GameFlags & 1) != 0 && *(u32 *)(e + 0xCC) != 0x1E28E0D4u) {
        EntitySetState(e, D_800A5D90, (u32)(uintptr_t)D_800A5D94);
    }

    /* 2. input-state FSM slot (+0x104 marker / +0x108 callback) */
    hi = *(s16 *)(e + 0x106);
    if (hi != 0) {
        TickFn fn;
        s32 adj = *(s16 *)(e + 0x104);
        if (hi < 1) {
            fn = *(TickFn *)(e + 0x108);
        } else {
            s32 base = *(s32 *)(e + *(s16 *)(e + 0x108));
            s32 slot = hi * 8 + base;
            adj += *(s16 *)((u8 *)(intptr_t)slot - 8);
            fn = *(TickFn *)((u8 *)(intptr_t)slot - 4);
        }
        fn(e + adj);
    }

    /* 3. base animation tick */
    EntityUpdateCallback(e);

    /* 4. shrink hitbox + clear external pushes */
    if (e[0x1B0] != 0) {                       /* shrinkFlag */
        *(s16 *)(e + 0x40) = -5;
        *(s16 *)(e + 0x42) = -10;
        *(s16 *)(e + 0x44) = 10;
        *(s16 *)(e + 0x46) = 10;
    }
    *(s16 *)(e + 0x160) = 0;                   /* pushX */
    *(s16 *)(e + 0x162) = 0;                   /* pushY */

    if (g_pGameState->checkpoint_active == 0) {
        /* 5a. tile collision */
        PlayerProcessTileCollision(e);

        /* 5b. invincibility countdown */
        if (e[0x128] != 0) {
            e[0x128]--;
        }

        /* 5c. powerup timer; on expiry restore palette + drop the HUD timer */
        {
            s16 t = *(s16 *)(e + 0x144);
            if (t != 0) {
                *(s16 *)(e + 0x144) = (s16)(t - 1);
                if (t == 1) {
                    PlayEntityPositionSound(e, 0x40E28045);
                    ctx = *(u8 **)(e + 0x34);
                    ctx[0x37] = 0;             /* palette-cycle index */
                    ctx[0x38] = 0;             /* STP blend off */
                    e[0x15A] = e[0x15D];       /* currentRGB = baseRGB */
                    e[0x15B] = e[0x15E];
                    e[0x15C] = e[0x15F];
                    e[0xF8]  = 1;              /* clutDirty -> re-upload */
                    ctx[0x34] = e[0x15A];
                    ctx[0x35] = e[0x15B];
                    ctx[0x36] = e[0x15C];
                    if (*(s32 *)(e + 0x14C) != 0) {    /* powerup HUD entity */
                        u8 *hud = *(u8 **)(e + 0x14C);
                        (*(u8 **)(hud + 0x34))[0x0A] = 0;
                        RemoveEntityFromAllLists((s32)(uintptr_t)g_pGameState,
                                                 (s32)(uintptr_t)hud);
                        *(s32 *)(e + 0x14C) = 0;
                    }
                }
            }
        }

        /* 5d. pending death request */
        if (e[0x159] != 0) {
            e[0x159] = 0;
            EntitySetState(e, D_800A5D98, (u32)(uintptr_t)D_800A5D9C);
        }

        /* 5e. halo companion maintenance */
        if (*(s32 *)(e + 0x168) == 0) {
            if ((g_pPlayerState->powerup_flags & 1) != 0) {
                u8 *halo = AllocateFromHeap((u8 *)g_pBlbHeapBase, 0x30, 1, 0);
                CreateHaloEntity(halo, (u32)(uintptr_t)e);
                *(void **)(e + 0x168) = halo;
                PlayEntityPositionSound(e, 0xE0880448);
                *(u8 *)(*(u8 **)(halo + 0x24) + 0x1E0) = 0x20;
                AddToZOrderList(g_pGameState, (s32)(uintptr_t)halo);
                AddToXPositionList(g_pGameState, *(s32 *)(halo + 0x24));
            }
        } else if ((g_pPlayerState->powerup_flags & 1) == 0) {
            (*(u8 **)(e + 0x168))[0x2C] = 1;   /* halo self-destruct flag */
            *(s32 *)(e + 0x168) = 0;
        }

        /* 5f. yellow-bird (glide) companion maintenance */
        if (*(s32 *)(e + 0x16C) == 0) {
            if ((g_pPlayerState->powerup_flags & 2) != 0) {
                u8 *bird = AllocateFromHeap((u8 *)g_pBlbHeapBase, 0x110, 1, 0);
                bird = (u8 *)CreateYellowBirdEntity(bird, (s32)(uintptr_t)e);
                *(void **)(e + 0x16C) = bird;
                AddToZOrderList(g_pGameState, (s32)(uintptr_t)bird);
                AddToXPositionList(g_pGameState, *(s32 *)(bird + 0x34));
            }
        } else if ((g_pPlayerState->powerup_flags & 2) == 0) {
            u8 *bird = *(u8 **)(e + 0x16C);
            RemoveFromTickList(g_pGameState, bird);
            RemoveFromRenderList(g_pGameState, *(s32 *)(bird + 0x34));
            if (bird != NULL) {
                u8 *vt = *(u8 **)(bird + 0x18);
                void (*dtor)(void *, s32) =
                    *(void (**)(void *, s32))(vt + 0x0C);
                dtor(bird + *(s16 *)(vt + 0x08), 3);
            }
            *(s32 *)(e + 0x16C) = 0;
        }
    }

    /* 6. render-scale easing (unless disableScale) */
    if (e[0x178] == 0) {
        s32 sr = *(s32 *)(e + 0x50);
        if (e[0x1B0] != 0) {                       /* shrinking */
            if (sr > 0x4000) {
                *(s32 *)(e + 0x50) = sr - 0x1000;
                *(s32 *)(e + 0x54) = sr - 0x1000;
            }
        } else if (g_pPlayerState->shrink_mode != 0) {
            if (sr > 0x8000) {
                *(s32 *)(e + 0x50) = sr - 0x1000;
                *(s32 *)(e + 0x54) = sr - 0x1000;
            }
        } else if (g_pGameState->camera_scroll_speed == 0x10000 && sr < 0x10000) {
            *(s32 *)(e + 0x50) = sr + 0x1000;
            *(s32 *)(e + 0x54) = sr + 0x1000;
        }
    }

    /* 7. damage-flash / powerup-glow tint */
    ctx = *(u8 **)(e + 0x34);
    if (e[0x17D] != 0) {
        e[0x17D]--;                                /* rgbCooldown */
    } else if (e[0x128] == 0) {                    /* not damage-flashing */
        if (*(s16 *)(e + 0x144) == 0) {
            /* steady state: tint = baseRGB, opaque */
            ctx[0x38] = 0;
            e[0x15A] = e[0x15D];
            e[0x15B] = e[0x15E];
            e[0x15C] = e[0x15F];
            ctx[0x34] = e[0x15A];
            ctx[0x35] = e[0x15B];
            ctx[0x36] = e[0x15C];
        } else {
            /* powerup active: fixed glow tint, STP blend on */
            ctx[0x38] = 1;
            e[0x15A] = 0x20;
            e[0x15B] = 0x60;
            e[0x15C] = 0x30;
            ctx[0x34] = e[0x15A];
            ctx[0x35] = e[0x15B];
            ctx[0x36] = e[0x15C];
        }
    } else {
        /* damage-flash: pulse tint on (frame & 3) */
        s32 base = (s32)(g_pGameState->frame_counter & 3u) * 0x20;
        s32 adj = base - 0x30;
        s32 r, g, b;
        if (*(s16 *)(e + 0x144) == 0) {
            ctx[0x38] = 0;
            r = (s32)e[0x15D] + adj;
            g = (s32)e[0x15E] + adj;
            b = (s32)e[0x15F] + adj;
        } else {
            ctx[0x38] = 1;
            r = base - 0x10;
            g = base + 0x30;
            b = base;
        }
        e[0x15A] = clamp255(r);
        e[0x15B] = clamp255(g);
        e[0x15C] = clamp255(b);
        ctx[0x34] = e[0x15A];
        ctx[0x35] = e[0x15B];
        ctx[0x36] = e[0x15C];
    }

    /* 8. particle trail */
    if (e[0x1AF] != 0 && (g_pGameState->frame_counter & 7u) == 0) {
        u8 *p = AllocateFromHeap((u8 *)g_pBlbHeapBase, 0x84, 1, 0);
        CreatePlayerParticleEntity(p, e);
        AddToZOrderList(g_pGameState, (s32)(uintptr_t)p);
        AddToXPositionList(g_pGameState, *(s32 *)(p + 0x34));
    }
}
