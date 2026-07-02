#include "common.h"
#include "functions.h"
#include "globals.h"
#include "Game/callback_slot.h"
#include "Game/spracc_records.h"
#include "Game/fsm_dispatch.h"

extern s32 PlayerEntityEventHandler(PlayerEntity *e, u32 event, u32 arg2, u32 arg3);
extern s32 PlayerEntityEventHandlerAlt(PlayerEntity *e, u32 event, u32 arg2, u32 arg3);
extern s32 PlayerEntityCollisionHandler(PlayerEntity *e, u32 event, u32 arg2, u32 arg3);
extern void RemoveEntityFromAllLists(GameState *gs, Entity *entity);
extern void PlayEntityPositionSound(Entity *e, u32 soundId);
extern void SetEntityStateFlagWithValue(void *e, u8 val);
extern s32 PlayerEvent_ZoneTriggerHandler();

INCLUDE_ASM("asm/nonmatchings/playst", PlayerProcessBounceCollision);

void PlayerClearSwirlPortalEntity(PlayerEntity *e) {
    g_pGameState->camera_follow_direction = 0;
    if (e->swirlPortalEntity != NULL) {
        RemoveEntityFromAllLists(g_pGameState, e->swirlPortalEntity);
        e->swirlPortalEntity = NULL;
    }
}

/* On release of the entity the player is interacting with (interactEntity),
 * fires that entity's FSM event handler with event 0x1005 ("released").
 * Decodes the standard marker+callback pair (eventMarker/eventCallback at
 * +0x8/+0xC): hi==0 -> no handler; hi<0 -> call eventCallback directly at
 * (entity + lo); hi>0 -> index an 8-byte slot table at *(entity + (s16)fn) by
 * hi, calling slot.fn at (entity + lo + slot.arg). The reported velocity is
 * the player's Y velocity, halved when the shrink powerup (scale 0x8000) is
 * active. Clears interactEntity afterward.
 *
 * SHELVED as asm: the C below reproduces the exact algorithm, but gcc 2.7.2
 * pins `held` to a1 (target keeps it in a0 to fold the final held+xOff into
 * the fn's a0 arg), rotating every register in the body, and schedules the
 * velY/scale loads and the two slot-table loads (-8/-4) in the opposite
 * order. All are allocation/scheduling residuals not reachable by source
 * restructuring, so the original asm is retained to preserve the byte-match.
 *
 * typedef void (*ReleaseNotifyCallback)(void *target, s32 event, s16 vel,
 *                                       PlayerEntity *sender);
 * void PlayerNotifyEntityRelease(PlayerEntity *e) {
 *     Entity *held;
 *     s32 rawVelY;
 *     s32 vel;
 *     s16 markerHi;
 *     s16 markerLo;
 *     s16 xOff;
 *     s32 slotArg;
 *     ReleaseNotifyCallback fn;
 *
 *     held = e->interactEntity;
 *     if (held != NULL) {
 *         rawVelY = e->velocityY_fixed;
 *         vel = rawVelY;
 *         if (e->sprite.base.scalePowerupX == 0x8000) {
 *             vel = (s32)(rawVelY << 16) >> 17;
 *         }
 *         markerHi = *(s16 *)((u8 *)held + 0xA);
 *         if (markerHi != 0) {
 *             if (markerHi > 0) {
 *                 u8 *tbl = *(u8 **)((u8 *)held + *(s16 *)((u8 *)held + 0xC));
 *                 slotArg = *(s32 *)(tbl + markerHi * 8 - 8);
 *                 fn = *(ReleaseNotifyCallback *)(tbl + markerHi * 8 - 4);
 *             } else {
 *                 fn = *(ReleaseNotifyCallback *)((u8 *)held + 0xC);
 *             }
 *             markerLo = *(s16 *)((u8 *)held + 8);
 *             if (markerHi > 0) {
 *                 xOff = (s16)slotArg + markerLo;
 *             } else {
 *                 xOff = markerLo;
 *             }
 *             fn((u8 *)held + xOff, 0x1005, (s16)vel, e);
 *         }
 *         e->interactEntity = NULL;
 *     }
 * }
 */
INCLUDE_ASM("asm/nonmatchings/playst", PlayerNotifyEntityRelease);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerProcessTileCollision);

/* Spawns the swirl portal (Swirly-Q pickup) at the player's position: X is
 * offset +/-0x14 by facing, Y is offset by the player's apexVelocity field.
 * The portal-cheat flag selects one of two portal sprite IDs. The portal is
 * scale-inherited, sorted into the render list, and one Swirly-Q is consumed
 * from the run's counter.
 *
 * SHELVED: the C below reproduces the exact algorithm; the position-struct
 * snapshot (sh/lh at sp+0x18/0x1A), the field reads, and the tail all match.
 * The only residual is in the two duplicated AllocateFromHeap calls: the target
 * keeps mem in v0 through both branches and emits the a0=v0 move once at the
 * merge (spriteId in t0, and sets a1=0x100 before the a0 load); gcc here emits
 * `move a0,v0` inside each branch (spriteId in v1, a1 after a0). Reordering
 * spriteId-vs-alloc only makes gcc hoist the call to a single jal (further off).
 * This is the scheduler's arg-setup/return-move placement for branch-duplicated
 * calls and is not source-reachable.
 *
 *   extern PlayerState *PLAYER_STATE_DATA asm("D_800A597C");
 *   extern Entity *InitScaledEntityWithSprite(u8 *mem, s16 x, s16 y, u8 facing,
 *                                             s32 scaleRender, u32 spriteId);
 *   void SpawnSwirlPortalEntity(PlayerEntity *e) {
 *       struct { s16 x; s16 y; } pos;
 *       u8 *mem; Entity *portal; u32 spriteId;
 *       pos.x = e->sprite.base.worldX + 0x14;
 *       if (e->sprite.base.facing != 0) {
 *           pos.x = e->sprite.base.worldX - 0x14;
 *       }
 *       pos.y = e->sprite.base.worldY + e->apexVelocity;
 *       if (e->portalCheatFlag != 0) {
 *           mem = AllocateFromHeap(g_pBlbHeapBase, 0x100, 1, 0);
 *           spriteId = 0x3D348056;
 *       } else {
 *           mem = AllocateFromHeap(g_pBlbHeapBase, 0x100, 1, 0);
 *           spriteId = 0xCA1B20CB;
 *       }
 *       portal = InitScaledEntityWithSprite(mem, pos.x, pos.y,
 *                    e->sprite.base.facing, e->sprite.base.scaleRender, spriteId);
 *       AddEntityToSortedRenderList((u8 *)g_pGameState, portal);
 *       PLAYER_STATE_DATA->swirly_q_count--;
 *   } */
INCLUDE_ASM("asm/nonmatchings/playst", SpawnSwirlPortalEntity);

/* Spawns a short-lived dust puff behind the player (e.g. skidding / landing).
 * The puff's X is offset +0x14 from the player, or -0x14 when facing left; its
 * Y is worldY biased by apexVelocity. X and Y are stored into a stack struct
 * {u16 x; u16 y;} and packed as (x | y<<16) into InitParticleEntity's coord arg
 * (sprite 0x49289C14, scaleRender = scalePowerupX, args 0x3D4/0). The particle
 * is then linked into the sorted render list via g_pGameState.
 *
 * SHELVED: the equivalent C below reproduces the exact 622-instruction body and
 * all logic, but two residuals remain that are not source-reachable under
 * gcc-2.7.2's sched2 / frame allocator: (1) the caller stores the three
 * InitParticleEntity stack args before reloading pos.x/pos.y for the pack,
 * whereas cc1 reloads first (x lands in a different temp reg); (2) the frame is
 * 0x38 in the original vs 0x30 emitted here. Left as INCLUDE_ASM to preserve the
 * byte match.
 *
 *   void PlayerSpawnDustParticle(PlayerEntity *e) {
 *       struct { u16 x; u16 y; } pos;
 *       u8 *mem;
 *       Entity *particle;
 *       pos.x = (u16)e->sprite.base.worldX + 0x14;
 *       if (e->sprite.base.facing != 0)
 *           pos.x = (u16)e->sprite.base.worldX - 0x14;
 *       pos.y = (u16)e->sprite.base.worldY + (u16)e->apexVelocity;
 *       mem = AllocateFromHeap(g_pBlbHeapBase, 0x108, 1, 0);
 *       particle = InitParticleEntity(mem, 0x49289C14, pos.x | (pos.y << 16),
 *                     e->sprite.base.facing, e->sprite.base.scalePowerupX, 0x3D4, 0);
 *       AddEntityToSortedRenderList((u8 *)g_pGameState, particle);
 *   }
 */
INCLUDE_ASM("asm/nonmatchings/playst", PlayerSpawnDustParticle);

/* Reaction dispatch when the player overlaps an enemy. No-op (returns 0) while
 * invincible (+0x128 flash timer). Otherwise picks the next player FSM state:
 *  - if the "hit-survives" powerup bit (g_pPlayerState->powerup_flags & 1) is
 *    set, the player recoils: QuickTurn when the enemy is to the right
 *    (player.worldX < enemy.worldX), else SetupBounceDown;
 *  - with the bit clear, RespawnAtCheckpoint if a powerup timer is still
 *    running (+0x144), else Death.
 * All four outcomes share the (0xFFFF0000 marker, callback) slot layout in
 * small-data and a single EntitySetState tail; returns 1 when a hit was
 * processed. */
extern PlayerState *PLAYER_STATE_DATA asm("D_800A597C");
u32 PlayerBounceDownMarker  asm("D_800A5D50");
EntityCallback PlayerBounceDownFn asm("D_800A5D54");
u32 PlayerQuickTurnMarker   asm("D_800A5D58");
EntityCallback PlayerQuickTurnFn  asm("D_800A5D5C");
u32 PlayerRespawnMarker     asm("D_800A5D78");
EntityCallback PlayerRespawnFn    asm("D_800A5D7C");
u32 PlayerDeathMarker       asm("D_800A5D80");
EntityCallback PlayerDeathFn      asm("D_800A5D84");

s32 PlayerProcessEnemyHit(PlayerEntity *e, Entity *enemy) {
    s32 marker;
    EntityCallback fn;

    if (e->invincibilityTimer != 0) {
        return 0;
    }
    if (PLAYER_STATE_DATA->powerup_flags & 1) {
        if (enemy->worldX > e->sprite.base.worldX) {
            marker = PlayerQuickTurnMarker;
            fn = PlayerQuickTurnFn;
        } else {
            marker = PlayerBounceDownMarker;
            fn = PlayerBounceDownFn;
        }
    } else if (*(u16 *)&e->powerupTimer != 0) {
        marker = PlayerRespawnMarker;
        fn = PlayerRespawnFn;
    } else {
        marker = PlayerDeathMarker;
        fn = PlayerDeathFn;
    }
    EntitySetState((Entity *)e, marker, fn);
    return 1;
}

INCLUDE_ASM("asm/nonmatchings/playst", CheckPlayerHitByEnemy);

/* Bulk-installs four caller-supplied (marker, callback) FSM slots onto an
 * entity -- tick (+0x00), event (+0x08), +0x104, and render (+0x1C) -- each
 * marshalled through a shared local scratch, then swaps the sprite. Used to
 * wire up a player-helper entity's callbacks in one shot.
 *
 * SHELVED (single-instruction scheduling diff): frame (0x28), the sp+4 scratch
 * hole (padded slot struct -> scratch at sp+0x14), and all four callback-pair
 * struct copies match byte-for-byte. The only diff: TARGET loads the last
 * (stack) arg `spriteId` into $a1 at instruction 2 -- reusing $a1 right after
 * spilling its original value -- and holds it across the whole body, while cc1
 * loads it just before the SetEntitySpriteId call. A `u32 sid = spriteId;`
 * hoist local did not move it (prologue reg-alloc choice, not source-reachable),
 * and the permuter (-j4, 150s) floored at 70. Equivalent C (parked in
 * nonmatchings/PlayerSetupCallbacksAndSprite):
 *   typedef struct { s32 marker; void *fn; } SetupSlot;      // 4 by-value pairs
 *   typedef struct { s32 pad; SetupSlot s; } PaddedSetupSlot; // sp+4 hole
 *   PaddedSetupSlot u;
 *   u.s = tick;  *(SetupSlot*)&e->tickMarker  = u.s;   // then event@0x8,
 *   ...          *(SetupSlot*)((u8*)e+0x104)  = u.s;   // slot104, render@0x1C
 *   SetEntitySpriteId(e, spriteId, 1); */
INCLUDE_ASM("asm/nonmatchings/playst", PlayerSetupCallbacksAndSprite);

/* Spawns an RGB-tinted particle attached to the player (stored in
 * attachedEntity/+0x1A0). Positions it at the player's X and Y+attachedYOff,
 * inits it via InitParticleEntity (sprite id = arg1, facing/scale inherited),
 * copies the player's currentRGB tint into the sprite context, sets the
 * semi-transparency flag from the active powerup timer, and sorts it into the
 * render list.
 *
 * SHELVED: the verified C below matches every instruction EXCEPT the same
 * irreducible InitParticleEntity packedXY delay-slot residual seen in
 * SpawnPlayerDeathEffect. The target reloads BOTH pos.x/pos.y from their stack
 * slots (lhu 0x20/0x22) at the end and fills the jal delay slot with the final
 * `or`; gcc instead reloads pos.x early into a temp and fills the delay slot
 * with `sw scalePowerupX,0x10(sp)`. This is the scheduler's delay-slot-filler
 * choice (dbranch/sched2) and is not source-reachable -- confirmed via inline
 * pack, explicit u32 temp, and flipped operand order, all identical. Every
 * other instruction (incl. the RGB-block load ordering and the lhu powerupTimer
 * test) matches.
 *
 *   extern Entity *InitParticleEntity(u8 *mem, u32 spriteId, u32 packedXY,
 *                       u8 facing, s32 scaleRender, s32 arg5, s32 arg6);
 *   void SpawnPlayerColoredParticle(PlayerEntity *e, u32 arg1) {
 *       struct { u16 x; u16 y; } pos;
 *       u8 *mem; Entity *particle; RenderSprite *ctx; u8 r, g, b;
 *       pos.x = (u16)e->sprite.base.worldX;
 *       pos.y = (u16)(e->sprite.base.worldY + e->attachedYOff);
 *       mem = AllocateFromHeap(g_pBlbHeapBase, 0x108, 1, 0);
 *       particle = InitParticleEntity(mem, arg1, pos.x | (pos.y << 16),
 *                      e->sprite.base.facing, e->sprite.base.scalePowerupX, 0x3E9, 0);
 *       e->attachedEntity = particle;
 *       *(PlayerEntity **)((u8 *)particle + 0x104) = e;
 *       r = e->currentRGB[0];
 *       g = e->currentRGB[1];
 *       b = e->currentRGB[2];
 *       ctx = (RenderSprite *)e->attachedEntity->spriteContext;
 *       ctx->r = r; ctx->g = g; ctx->b = b;
 *       if ((u16)e->powerupTimer != 0) {
 *           ((RenderSprite *)e->attachedEntity->spriteContext)->semiTransFlag2 = 1;
 *       } else {
 *           ((RenderSprite *)e->attachedEntity->spriteContext)->semiTransFlag2 = 0;
 *       }
 *       AddEntityToSortedRenderList((u8 *)g_pGameState, e->attachedEntity);
 *   } */
INCLUDE_ASM("asm/nonmatchings/playst", SpawnPlayerColoredParticle);

/* Spawns the dissolving death-debris particle when the player dies. Optionally
 * plays the death sound (arg gates it), then only spawns the sprite if level
 * flag bit 14 is set. Allocates a 0x108-byte particle from the BLB heap at the
 * player's position, inits it as sprite 0x422A0225 (facing/scale inherited),
 * computes its VRAM TPage from the render context, and sorts it into the
 * render list.
 *
 * SHELVED: the verified C below reproduces the exact algorithm and matches every
 * instruction EXCEPT a 3-instruction scheduling swap in the InitParticleEntity
 * packedXY setup. The target reloads BOTH pos.x/pos.y from their stack slots
 * (lhu 0x20/0x22) after the AllocateFromHeap call and fills the jal delay slot
 * with the final `or`; gcc instead keeps pos.x in a caller-saved register and
 * fills the delay slot with the `sw scalePowerupX,0x10(sp)` stack-arg store.
 * This is an allocator register-pressure/spill decision (target's frame is 8
 * bytes larger to hold the extra reloaded value) and is not source-reachable.
 * All other instructions incl. the tricky lh-vs-lhu VRAM masking match.
 *
 *   extern Entity *InitParticleEntity(u8 *mem, u32 spriteId, u32 packedXY,
 *                       u8 facing, s32 scaleRender, s32 arg5, s32 arg6);
 *   extern s32 GetTPage(s32 tp, s32 abr, s32 x, s32 y);
 *   extern u32 GetLevelFlags();
 *   void SpawnPlayerDeathEffect(PlayerEntity *e, u8 playSound) {
 *       struct { u16 x; u16 y; } pos;
 *       u8 *mem; Entity *particle; RenderSprite *ctx; s32 cx, cy;
 *       if (playSound != 0) {
 *           PlayEntityPositionSound(&e->sprite.base, 0x5860C640);
 *       }
 *       if ((GetLevelFlags((u8 *)g_pGameState + 0x84) >> 14) & 1) {
 *           pos.x = (u16)e->sprite.base.worldX;
 *           pos.y = (u16)e->sprite.base.worldY;
 *           mem = AllocateFromHeap(g_pBlbHeapBase, 0x108, 1, 0);
 *           particle = InitParticleEntity(mem, 0x422A0225, pos.x | (pos.y << 16),
 *                          e->sprite.base.facing, e->sprite.base.scalePowerupX, 0x3D4, 0);
 *           ctx = (RenderSprite *)particle->spriteContext;
 *           cx = ctx->vramX;
 *           cy = ctx->vramY;
 *           ctx->tpage = GetTPage(ctx->colorMode, 1, cx & -0x40, cy & -0x100);
 *           AddEntityToSortedRenderList((u8 *)g_pGameState, particle);
 *       }
 *   } */
INCLUDE_ASM("asm/nonmatchings/playst", SpawnPlayerDeathEffect);

void ApplyRandomRGBEffect(PlayerEntity *e) {
    e->baseRGB[0] = rand();
    e->baseRGB[1] = rand();
    e->baseRGB[2] = rand();
    e->damageFlag = 1;
}

INCLUDE_ASM("asm/nonmatchings/playst", PlayerTickCallback);

/* PlayerState_IdleRandom: unit spans 0x8005BAD0..0x8005BB80 — absorbs former split symbols PlayerState_SimpleTick (Ghidra labels with no external references; merged 2026-07-02).
 *
 * Idle fidget scheduler: two independent countdowns pick a random idle
 * animation. The long timer (+0x13A) fires first and hands off to
 * PlayerState_PlayRandomIdleAnimation (marker/fn @ D_800A5DA0, reroll
 * 0x200..0x2FF); otherwise the short timer (+0x139) decrements and, on expiry,
 * hands off to PlayerState_IdleLookAround (marker/fn @ D_800A5DA8, reroll
 * 0x30..0xAF). Both hand-offs share a single EntitySetState tail, then
 * PlayerTickCallback.
 *
 * SHELVED (single-instruction scheduling diff, same class as
 * PlayerSetupCallbacksAndSprite): the long-timer path matches byte-for-byte,
 * but on the short-timer fall-through path TARGET schedules the arg setup
 * (move a0,s0 / lw a1,a2) ahead of the `sb` reroll store, while cc1 emits the
 * store first. Not reachable from source ordering; permuter floored above 200. */
INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_IdleRandom);


void PlayerState_FrameCountTick(PlayerEntity *e) {
    e->jumpParam += 1;
    PlayerTickCallback(e);
}

/* Bounce-cooldown tick installed by the bounce-on-enemy collision path
 * (PlayerCallback_CollisionHandlerWithQueue @ 0x8005CEC8). Per-frame
 * decrement of the cooldown timer at +0x13C, then tail-call into
 * PlayerTickCallback for animation + state-transition. NOTE: this
 * function does NOT itself check timer13C == 0 — the state-exit
 * transition happens inside PlayerTickCallback (which the trace
 * confirms is running every frame regardless of the wrapper). The
 * struct_watch trace from 2026-06-21 captured this tick running for
 * 5319+ consecutive frames during the Shriney Guard fight while
 * pFrameTable pointed at 0x801388E0 (bounce-cooldown anim). */
void PlayerState_CooldownTick(PlayerEntity *e) {
    if (e->timer13C != 0) {
        e->timer13C--;
    }
    PlayerTickCallback(e);
}

/* Fall-with-rotation tick: gradually decays the player's vertical velocity
 * (entity+0x50/+0x54) by 0xC00 per frame while still nonzero, and rotates
 * the visual angle (entity+0x72) by +0x20 per frame, wrapping at 0x1000.
 * Tail-calls PlayerTickCallback for the standard sprite/animation tick. */
void PlayerState_FallWithRotation(Entity *e) {
    s32 vy = e->scaleRender;
    if (vy != 0) {
        vy -= 0xC00;
        e->scaleRender = vy;
        e->scaleRender2 = vy;
    }
    {
        u16 angle = e->targetY;
        u16 new_angle = angle + 0x20;
        e->targetY = new_angle;
        if (new_angle >= 0x1000) {
            e->targetY = angle - 0xFE0;
        }
    }
    PlayerTickCallback(e);
}

INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_TransitionToPickup);

void PlayerState_QueuedCallbackTimer(PlayerEntity *e) {
    u16 *ctr = (u16 *)&e->respawnTimer;
    if (*ctr != 0) {
        *ctr -= 1;
        if (*ctr == 0) {
            EntityProcessCallbackQueue(&e->sprite.base);
        }
    }
    PlayerTickCallback(&e->sprite.base);
}

INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_UpdateAttachedEntity);
/* Per-tick sync of the player's attached rider entity: copies the player's
 * world X/Y (Y biased by attachedYOff) onto the attached entity, mirrors the
 * player's current RGB tint into the attached entity's sprite context
 * (+0x34..+0x36), and sets the context's glow flag (+0x38) from the player's
 * powerup timer. */
/*
void PlayerState_UpdateAttachedEntity(PlayerEntity *e) {
    Entity *attached;
    u8 *ctx;

    PlayerTickCallback(&e->sprite.base);
    attached = e->attachedEntity;
    if (attached != NULL) {
        u8 r, g, b;
        attached->worldX = e->sprite.base.worldX;
        e->attachedEntity->worldY = e->sprite.base.worldY + e->attachedYOff;
        ctx = (u8 *)e->attachedEntity->spriteContext;
        r = e->currentRGB[0];
        g = e->currentRGB[1];
        b = e->currentRGB[2];
        ctx[0x34] = r;
        ctx[0x35] = g;
        ctx[0x36] = b;
        if (*(u16 *)&e->powerupTimer != 0) {
            ((u8 *)e->attachedEntity->spriteContext)[0x38] = 1;
            return;
        }
        ((u8 *)e->attachedEntity->spriteContext)[0x38] = 0;
    }
}
*/

/* Player event dispatcher for zone-trigger events (0x100C..0x1017), routed via
 * a 12-entry jump table (jtbl_80011AE4). Enters/exits the shrink zone, warps
 * the player to a companion entity's position (+0x20 Y) and hands off to a warp
 * FSM state, or (0x1010) transitions into a zone-enter FSM state -- but only
 * when neither scale effects are disabled nor a level-exit is pending. Event
 * 0x1017 is a pure query: returns 1 when scale is not disabled and no level-exit
 * is pending, else 0; all other handled events return 0.
 *
 * SHELVED: the C below is VERIFIED byte-correct -- the compiled code diffs clean
 * across the whole function range. The only blocker is jump-table rodata
 * migration: gcc emits its own copy of jtbl_80011AE4 into build/src/playst.o's
 * .rodata, but the linker script only pulls build/asm/data/playst.rodata.o's
 * .rodata (jtbl lives there). Linking the compiled table would require editing
 * SLES_010.90.ld to add build/src/playst.o(.rodata) at the exact rodata offset
 * AND deleting jtbl_80011AE4 from asm/data/playst.rodata.s without disturbing
 * the surrounding rodata order -- an infra change deferred to a rodata-migration
 * pass. Keep as INCLUDE_ASM until then.
 *
 *   extern void PlayerEnterShrinkZone(void);
 *   extern void PlayerExitShrinkZone(void);
 *   u32 PlayerZoneEnterMarker asm("D_800A5DB8");
 *   EntityCallback PlayerZoneEnterFn asm("D_800A5DBC");
 *   u32 PlayerZoneWarpMarker asm("D_800A5DC0");
 *   EntityCallback PlayerZoneWarpFn asm("D_800A5DC4");
 *   s32 PlayerEvent_ZoneTriggerHandler(PlayerEntity *e, u32 event, u32 arg2,
 *                                      Entity *src) {
 *       s32 result = 0;
 *       switch (event & 0xFFFF) {
 *       case 0x100C: PlayerEnterShrinkZone(); break;
 *       case 0x100D: PlayerExitShrinkZone(); break;
 *       case 0x1010:
 *           if (e->disableScale != 0) break;
 *           if (e->levelExitFlag != 0) break;
 *           EntitySetState((Entity *)e, PlayerZoneEnterMarker, PlayerZoneEnterFn);
 *           break;
 *       case 0x100F:
 *           e->sprite.base.worldX = src->worldX;
 *           e->sprite.base.worldY = src->worldY + 0x20;
 *           EntitySetState((Entity *)e, PlayerZoneWarpMarker, PlayerZoneWarpFn);
 *           break;
 *       case 0x1017:
 *           if (e->disableScale != 0) break;
 *           result = (e->levelExitFlag == 0);
 *           break;
 *       }
 *       return result;
 *   } */
INCLUDE_ASM("asm/nonmatchings/playst", PlayerEvent_ZoneTriggerHandler);


/* PlayerEvent_ZoneTriggerHandlerAlt: unit spans 0x8005BF5C..0x8005C0A0 — absorbs former split symbols PlayerState_SetAndProcessQueue (Ghidra labels with no external references; merged 2026-07-02). */
INCLUDE_ASM("asm/nonmatchings/playst", PlayerEvent_ZoneTriggerHandlerAlt);


/* PlayerEvent_ZoneTriggerHandlerV2: unit spans 0x8005C0A0..0x8005C1C4 — absorbs former split symbols PlayerCallback_SetStateAndProcessQueue (Ghidra labels with no external references; merged 2026-07-02). */
INCLUDE_ASM("asm/nonmatchings/playst", PlayerEvent_ZoneTriggerHandlerV2);



INCLUDE_ASM("asm/nonmatchings/playst", PlayerEntityEventHandler);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerEntityCollisionHandler);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerEntityEventHandlerAlt);

s32 PlayerCallback_EventHandlerWithQueue(PlayerEntity *e, u32 event, u32 arg2, u32 arg3) {
    s32 result;
    u32 maskedEvent = event & 0xFFFF;
    result = PlayerEntityEventHandler(e, maskedEvent, arg2, arg3);
    if (maskedEvent == 2) {
        EntityProcessCallbackQueue((Entity *)e);
    }
    return result;
}

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_AnimFrameParticleHandler);

s32 PlayerCallback_CollisionHandlerWithQueue(PlayerEntity *e, u32 event, u32 arg2, u32 arg3) {
    s32 result;
    u32 maskedEvent = event & 0xFFFF;
    result = PlayerEntityCollisionHandler(e, maskedEvent, arg2, arg3);
    if (maskedEvent == 2) {
        EntityProcessCallbackQueue((Entity *)e);
    }
    return result;
}

INCLUDE_ASM("asm/nonmatchings/playst", PlayerBounceCallback);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_TeleporterAnimHandler);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_BounceEventHandler);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_DeathDebrisSpawner);

/* PlayerCallback_ThrowEventHandler: unit spans 0x8005DAA4..0x8005DDE0 — absorbs former split symbols PlayerCallback_ThrowSetStateAndSpawn (Ghidra labels with no external references; merged 2026-07-02). */
INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_ThrowEventHandler);



/* PlayerCallback_CheckpointEventHandler: unit spans 0x8005DDE0..0x8005E170 — absorbs former split symbols PlayerCallback_CheckpointSaveAndSpawnHUD (Ghidra labels with no external references; merged 2026-07-02). */
INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_CheckpointEventHandler);



/* PlayerCallback_ProjectileEventHandler: unit spans 0x8005E170..0x8005E450 — absorbs former split symbols PlayerCallback_ProjectileSetStateAndSpawn (Ghidra labels with no external references; merged 2026-07-02). */
INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_ProjectileEventHandler);



/* PlayerCallback_BaseEventHandler: unit spans 0x8005E450..0x8005E574 — absorbs former split symbols PlayerCallback_SetStateWithQueueProcess (Ghidra labels with no external references; merged 2026-07-02). */
INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_BaseEventHandler);



/* PlayerCallback_DeathAnimEventHandler: unit spans 0x8005E574..0x8005F29C — absorbs former split symbols PlayerCallback_DeathAnimSetStateHandler, PlayerCallback_DeathSpawnMenuEntities, func_8005EAAC, PlayerCallback_DeathSpawnMenuEntity, func_8005ECEC, PlayerCallback_AllocAndInitScrollLayer, func_8005EF68, PlayerCallback_InitScrollLayerAndAdd, func_8005F0DC, PlayerCallback_SpawnTrailParticle (Ghidra labels with no external references; merged 2026-07-02). */
INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_DeathAnimEventHandler);












INCLUDE_ASM("asm/nonmatchings/playst", SpawnPlayerTrailParticle);

s32 PlayerCallback_EventHandlerAltWithQueue(PlayerEntity *e, u32 event, u32 arg2, u32 arg3) {
    s32 result;
    u32 maskedEvent = event & 0xFFFF;
    result = PlayerEntityEventHandlerAlt(e, maskedEvent, arg2, arg3);
    if (maskedEvent == 2) {
        EntityProcessCallbackQueue((Entity *)e);
    }
    return result;
}

/* Player collision-damage event handler. Delegates to
 * PlayerEntityCollisionHandler, then post-processes two event types:
 *   event 1: on the specific collision id 0x50900004, clears the camera-follow
 *            direction and tears down any child swirl-portal entity.
 *   event 2: installs PlayerEntityCollisionHandler as the entity's event slot
 *            (direct-call marker 0xFFFF0000) and switches to the hurt sprite /
 *            animation. */
extern s32 SetEntitySpriteId(PlayerEntity *e, u32 spriteId, s32 flag);
extern void SetAnimationLoopFrame(PlayerEntity *e, u32 animId);
extern void SetAnimationFrameCallback(PlayerEntity *e, u32 animId);

s32 PlayerCallback_CollisionDamageSetup(PlayerEntity *e, u32 event, u32 arg2,
                                        u32 arg3) {
    PadSlot slot;
    u32 maskedEvent = event & 0xFFFF;
    s32 result = PlayerEntityCollisionHandler(e, maskedEvent, arg2, arg3);

    switch (maskedEvent) {
    case 1:
        if (arg2 == 0x50900004) {
            g_pGameState->camera_follow_direction = 0;
            if (e->swirlPortalEntity != NULL) {
                RemoveEntityFromAllLists(g_pGameState, e->swirlPortalEntity);
                e->swirlPortalEntity = NULL;
            }
        }
        break;
    case 2: {
        register void (*fn)() asm("$3") = (void (*)())PlayerEntityCollisionHandler;
        slot.s.markerLo = 0;
        slot.s.markerHi = -1;
        slot.s.fn = fn;
        *(CallbackSlot *)&e->sprite.base.eventMarker = slot.s;
    }
        SetEntitySpriteId(e, 0x388110, 1);
        SetAnimationLoopFrame(e, 0x1084280);
        SetAnimationFrameCallback(e, 0x1084280);
        break;
    }
    return result;
}

extern void EntitySetState(Entity *e, u32 marker, EntityCallback fn);
/* FSMStateSlot for PlayerState_Running (marker 0xFFFF0000, fn PlayerState_Running)
 * lives in the small-data segment at 0x800A5DD0/0x800A5DD4. Declared as tentative
 * definitions (not extern) so gcc emits them as .comm small-common symbols, which
 * the assembler addresses GP-relative to match the original. */
u32 PlayerRunningStateMarker asm("D_800A5DD0");
EntityCallback PlayerRunningStateFn asm("D_800A5DD4");

/* Walk-tick event handler that, on the walk-anim "run threshold" frame event
 * (event 1, arg 0x1210CD03), promotes the player into the running state when
 * the D-pad is held into the current facing direction (facing left -> bit
 * 0x8000, facing right -> bit 0x2000 of the held-button mask). Event 2
 * flushes the queued FSM callback like the other wrappers. */
s32 PlayerCallback_WalkToRunTransition(PlayerEntity *e, u16 event, u32 arg2, u32 arg3) {
    s32 ret2; /* load-bearing survival copy: keeps the return value in $s2 */
    s32 ret;

    ret = PlayerEntityEventHandler(e, event, arg2, arg3);
    if (event == 2) {
        EntityProcessCallbackQueue((Entity *)e);
    }
    ret2 = ret;
    if ((event == 1) && (arg2 == 0x1210CD03)) {
        u32 dirMask = 0x2000;
        if (e->sprite.base.facing != 0) {
            dirMask = 0x8000;
        }
        if (dirMask & e->pInput->buttons_held) {
            EntitySetState((Entity *)e, PlayerRunningStateMarker, PlayerRunningStateFn);
        }
    }
    return ret2;
}

/* Collision-handler wrapper for the trail entity: forwards to
 * PlayerEntityCollisionHandler, then on release (event 2) flushes the queued
 * FSM callback, and on event 1 with arg 0x10022814 sets the glide entity's
 * state flag. */
s32 PlayerCallback_CollisionTrailEntityUpdate(PlayerEntity *e, u16 event, u32 arg2, u32 arg3) {
    s32 ret2; /* load-bearing survival copy: keeps the return value in $s2 */
    s32 ret;

    ret = PlayerEntityCollisionHandler(e, event, arg2, arg3);
    if (event == 2) {
        EntityProcessCallbackQueue((Entity *)e);
    }
    ret2 = ret;
    if ((event == 1) && (arg2 == 0x10022814)) {
        SetEntityStateFlagWithValue(e->glideEntity, 1);
    }
    return ret2;
}

INCLUDE_ASM("asm/nonmatchings/playst", TryActivatePowerup);

/* Plays the directional-walk SFX (asset 0x64221E61) if any of the four
 * cardinal D-pad bits are pressed in the player's current input snapshot
 * (e->input->u2 — bit 1 = left, 2 = down, 4 = right, 8 = up). The
 * compiler short-circuits the four bit tests in the rather odd
 * 4 → 1 → 8 → 2 order seen in the original — preserved here for byte-match. */
void PlayerPlaySoundOnDirectionInput(PlayerEntity *e) {
    InputState *input = e->pInput;
    u16 flags = input->buttons_pressed;
    s32 a1 = 0;
    if (flags & 4) goto fire;
    if (flags & 1) goto fire;
    if (flags & 8) goto fire;
    if (flags & 2) goto fire;
    goto check;
fire:
    a1 = 1;
check:
    if ((u8)a1) {
        PlayEntityPositionSound(&e->sprite.base, 0x64221E61);
    }
}

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_IdleInputHandler);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_WalkInputHandler);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_WalkInputWithJump);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_WalkInputWithFall);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_JumpWallCollisionInput);

/* PlayerCallback_JumpInputAndCounters: unit spans 0x800602E0..0x800605D8 — absorbs former split symbols PlayerCallback_JumpDirectionAndCounters (Ghidra labels with no external references; merged 2026-07-02). */
INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_JumpInputAndCounters);


/* PlayerCallback_JumpTickHandler: unit spans 0x800605D8..0x80060890 — absorbs former split symbols PlayerCallback_SetStateAndUpdateCounters, PlayerCallback_JumpCounterUpdate (Ghidra labels with no external references; merged 2026-07-02). */
INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_JumpTickHandler);



INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_FallInputHandler);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_CrouchClimbInputHandler);




/* PlayerCallback_CrouchClimbTickHandler: unit spans 0x80060C9C..0x80060E24 — absorbs former split symbols PlayerCallback_SetStatePassthrough (Ghidra labels with no external references; merged 2026-07-02). */
INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_CrouchClimbTickHandler);




/* PlayerCallback_DebugCameraInput: unit spans 0x80060E24..0x80061180 — absorbs former split symbols PlayerCallback_DebugCameraVertical, PlayerCallback_DebugCameraUpdate (Ghidra labels with no external references; merged 2026-07-02). */
INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_DebugCameraInput);



/* Integrates the per-frame motion vectors (frameMotionX/Y, 16.16 fixed) into
 * the entity's position, treating worldX/Y as the integer part and
 * velocityX/Y as the fractional part of a 32-bit fixed-point position.
 * facing/flipY select whether each axis' motion is subtracted or added. */
void ApplyEntityPositionUpdate(SpriteEntity *e) {
    s32 x, y;

    x = (e->base.worldX << 16) + (u16)e->base.velocityX;
    y = (e->base.worldY << 16) + (u16)e->base.velocityY;
    if (e->base.facing != 0) {
        x -= e->frameMotionX;
    } else {
        x += e->frameMotionX;
    }
    if (e->base.flipY != 0) {
        y -= e->frameMotionY;
    } else {
        y += e->frameMotionY;
    }
    e->base.worldX = x >> 16;
    e->base.worldY = y >> 16;
    e->base.velocityX = x;
    e->base.velocityY = y;
}

/* PlayerCallback_HorizontalWallCollision: unit spans 0x8006120C..0x8006187C — absorbs former split symbols PlayerCallback_ProcessBounceWrapper3, func_80061774, PlayerCallback_SetStatePassthrough2 (Ghidra labels with no external references; merged 2026-07-02). */
INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_HorizontalWallCollision);




/* Vertical collision resolution: probes 7px above the player's feet for a
 * ground/ceiling collision (result byte in [1,0x3B]); if none, probes 2px
 * below. On a hit, snaps worldY to the collision-adjusted value returned by
 * PlayerApplyPositionWithCollision (passing the collision type through).
 *
 * SHELVED as asm: the C below reproduces the exact 0xB8-byte algorithm but
 * gcc 2.7.2 coalesces the boolean into v0 (target keeps it in v1 with an
 * extra `move v0,v1`) and defers the call-arg (a2=worldX / a3=y) setup to the
 * merge point instead of pre-positioning it in each predecessor. Both are
 * register-allocation/coalescing residuals that no source restructuring
 * (&&, explicit two-if, ternary, goto tail-merge; u8/u32 collType) could
 * dislodge, so the original asm is retained to preserve the byte-match.
 *
 * extern u16 PlayerApplyPositionWithCollision(void *e, u32 collType,
 *                                             s16 x, s16 y);
 * void PlayerCallback_VerticalCollisionCheck(SpriteEntity *e) {
 *     u32 collType;
 *     s32 cond1;
 *     s32 cond2;
 *     s16 y;
 *
 *     collType = EntityApplyMovementCallbacks((Entity *)e, e->base.worldX,
 *                    (s16)(e->base.worldY - 7)) & 0xFF;
 *     cond1 = 0;
 *     if (collType != 0) {
 *         cond1 = collType < 0x3C;
 *     }
 *     if (cond1 != 0) {
 *         y = e->base.worldY - 7;
 *         goto apply;
 *     }
 *     collType = EntityApplyMovementCallbacks((Entity *)e, e->base.worldX,
 *                    (s16)(e->base.worldY + 2)) & 0xFF;
 *     cond2 = 0;
 *     if (collType != 0) {
 *         cond2 = collType < 0x3C;
 *     }
 *     if (cond2 != 0) {
 *         y = e->base.worldY + 2;
 * apply:
 *         e->base.worldY = PlayerApplyPositionWithCollision(e, collType,
 *                              e->base.worldX, y);
 *     }
 * }
 */
INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_VerticalCollisionCheck);

/* PlayerCallback_RidingPlatformPhysics: unit spans 0x80061934..0x800625B4 —
 * absorbs former split symbols PlayerCallback_ProcessBounceWrapper1,
 * PlayerRidingPlatformUpdate, PlayerCallback_ScaleBoundsWrapper,
 * PlayerCallback_ScaleBoundsAndCollision, PlayerCallback_RidingPositionUpdate,
 * PlayerCallback_ScaleBoundsOffset, PlayerCallback_ScaleBoundsCore
 * (j/branch-only intra-function labels with no external references;
 * merged 2026-07-03 full-ROM ref scan). */
INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_RidingPlatformPhysics);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_PlatformFollowUpdate);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_WalkingOnPlatform);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_HandleMovementAndCollision);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_IdleOnPlatform);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_WalkingCollisionHandler);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_FallingPhysicsMain);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_KnockbackPhysics);

u32 PlayerSwimExitMarker asm("D_800A5D30");
EntityCallback PlayerSwimExitFn asm("D_800A5D34");

void PlayerCallback_SwimmingRisingPhysics(PlayerEntity *e) {
    s32 pos;
    s32 v;
    u8 tile;
    pos = (e->sprite.base.worldY << 16) + *(u16 *)&e->sprite.base.velocityY;
    tile = EntityApplyMovementCallbacks((Entity *)e, e->sprite.base.worldX,
                                        e->sprite.base.worldY);
    if (tile == 0x53) {
        v = e->velocityY_fixed - 0x5000;
        e->velocityY_fixed = v;
        if (v < -0x80000) {
            e->velocityY_fixed = -0x80000;
        }
    } else {
        EntitySetState((Entity *)e, PlayerSwimExitMarker, PlayerSwimExitFn);
    }
    pos += e->velocityY_fixed;
    e->sprite.base.worldY = pos >> 16;
    *(s16 *)&e->sprite.base.velocityY = pos;
}

INCLUDE_ASM("asm/nonmatchings/playst", PlayerStateInit_Idle);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_SetIdleStateCallbacks);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_IdleLookAround);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_PlayRandomIdleAnimation);

extern s32 GetTPage(s32 tp, s32 abr, s32 x, s32 y);

void PlayerState_UpdateTPageAndHideHalo(PlayerEntity *e) {
    RenderSprite *ctx = (RenderSprite *)e->sprite.base.spriteContext;
    Entity *halo;
    s32 cx, cy;
    cx = ctx->vramX;
    cy = ctx->vramY;
    ctx->tpage = GetTPage(ctx->colorMode, 0, cx & -0x40, cy & -0x100);
    halo = e->attachedEntity;
    if (halo != NULL) {
        *(s16 *)((u8 *)halo + 0x100) = 1;
        EntitySetRenderFlags(halo, 1);
        e->attachedEntity = NULL;
    }
}

INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_WalkingRight);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_WalkingLeft);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_Running);

/*
 * Enters the jump-apex state: seeds apex velocity threshold (-0x28), clears
 * Y velocity / jump param, flags special-move mode, then installs the
 * tick/event/input/render FSM callback slots (WalkInput driving, render
 * routed to WalkingOnPlatform when standing on an interact entity else the
 * normal movement/collision handler), queues PlayerState_Falling, and arms
 * the apex-transition callback via EntitySetCallback.
 *
 * SHELVED: verified algorithm below compiles to a byte-for-byte semantic
 * match, but gcc 2.7.2's instruction scheduler / register allocator diverges
 * in ways that are NOT reachable from clean source:
 *   - the two independent zero-stores (velocityY_fixed @0x110, jumpParam
 *     @0x156) are scheduled BEFORE `sb specialMoveMode,0x135` in the target
 *     but AFTER the first callback-address materialization in our output
 *     (the scheduler collapses `li v0,1; sb v0,0x135` to shorten v0's live
 *     range; the original build keeps v0=1 live across the zero-stores);
 *   - the shared markerHi=-1 constant is held in a2 for the first four slot
 *     installs and reloaded into v0 after the SetAnimationSpriteFlags call
 *     (our output reloads into a2); and a `li v0,-0x28` / `sw ra` pair is
 *     swapped in the prologue.
 * All remaining diffs are semantically-equivalent reorderings. Verified C:
 *
 *   void PlayerState_JumpApex(PlayerEntity *e) {
 *       PadSlot slot;
 *       register s16 m1 asm("$6");
 *       e->apexVelocity = -0x28;
 *       e->velocityY_fixed = 0;
 *       e->jumpParam = 0;
 *       e->specialMoveMode = 1;
 *       m1 = -1;
 *       slot.s.markerLo = 0; slot.s.markerHi = m1;
 *       slot.s.fn = (void (*)())PlayerTickCallback;
 *       *(CallbackSlot *)&e->sprite.base.tickMarker = slot.s;
 *       slot.s.markerLo = 0; slot.s.markerHi = m1;
 *       slot.s.fn = (void (*)())PlayerCallback_TeleporterAnimHandler;
 *       *(CallbackSlot *)&e->sprite.base.eventMarker = slot.s;
 *       slot.s.markerLo = 0; slot.s.markerHi = m1;
 *       slot.s.fn = (void (*)())PlayerCallback_WalkInputHandler;
 *       *(CallbackSlot *)&e->inputStateMarker = slot.s;
 *       slot.s.markerLo = 0; slot.s.markerHi = m1;
 *       if (e->interactEntity != NULL)
 *           slot.s.fn = (void (*)())PlayerCallback_WalkingOnPlatform;
 *       else
 *           slot.s.fn = (void (*)())PlayerCallback_HandleMovementAndCollision;
 *       *(CallbackSlot *)&e->sprite.base.renderMarker = slot.s;
 *       SetAnimationSpriteFlags(e, 0x88B9833C, 1);
 *       m1 = -1;
 *       slot.s.markerLo = 0; slot.s.markerHi = m1;
 *       slot.s.fn = (void (*)())PlayerState_Falling;
 *       *(CallbackSlot *)&e->sprite.queuedStateMarker = slot.s;
 *       EntitySetCallback((Entity *)e, PlayerJumpApexNextMarker,
 *                         PlayerJumpApexNextFn);
 *   }
 */
INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_JumpApex);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_Falling);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerEnterLandingState);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerSetupIdleState);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_SpecialMove2);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_Jump);

void PlayerState_StopSoundAndClear(PlayerEntity *e) {
    StopSPUVoice(e->soundHandle);
    e->soundHandle = -1;
}

INCLUDE_ASM("asm/nonmatchings/playst", PlayerStateInit_Walking);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerStateInit_ResumeWalking);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerStateInit_CrouchSlide);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerStateInit_IdleWithZoneTrigger);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerStateInit_IdleStanding);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerStateCallback_2);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_StandingIdle);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_SpecialMove1);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_PickupItem);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_LandingFromRide);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerStateInit_BounceFromEnemy);

void PlayerState_ClearBounceFlag(PlayerEntity *e) {
    g_pGameState->bounce_active_flag = 0;
}

void PlayerState_ClearBounceAndAirFlag(PlayerEntity *e) {
    g_pGameState->bounce_active_flag = 0;
    e->specialMoveMode = 0;
}

INCLUDE_ASM("asm/nonmatchings/playst", PlayerStateInit_JumpFromPlatform);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerStateInit_BounceJumpAnimation);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerStateInit_BounceActive);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_SpecialIdleAnim);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerStateInit_TeleportIdleOnPlatform);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerStateInit_TeleportWalking);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerStateInit_TeleportFalling);

void PlayerState_ClearInAirFlag(PlayerEntity *e) {
    e->specialMoveMode = 0;
}

INCLUDE_ASM("asm/nonmatchings/playst", PlayerSetupDeathState);

extern void SetAnimationTargetFrameIndex(Entity *e, s32 frame);
extern void EntitySetRenderFlags(Entity *e, u32 flags);

void PlayerState_LevelExitComplete(PlayerEntity *e) {
    SetAnimationTargetFrameIndex((Entity *)e, e->sprite.currentFrame);
    EntitySetRenderFlags((Entity *)e, 0);
    g_pGameState->level_clear_pending = 1;
    e->pendingStateChange = 1;
}

INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_DeathStart);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_Death);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_LevelExitTeleporter);

extern void EntitySetCallback(Entity *e, u32 marker, EntityCallback fn);
u32 PlayerHiddenIdleMarker asm("D_800A5F14");
EntityCallback PlayerHiddenIdleFn asm("D_800A5F18");

/* PlayerState_HideAndClearBounce @ 0x8006A310 -- hides the player sprite and
 * parks the FSM: sets bounce_active_flag, installs EntityUpdateCallback as
 * tick and PlayerEvent_ZoneTriggerHandler as event slot, zeroes the input
 * (+0x104) and render (+0x1C) slots, freezes the anim frame, hides render +
 * sprite (spriteContext->enabled=0), then EntitySetCallback(HiddenIdle pair).
 *
 * SHELVED (4-instr prologue rotation): body matches except cc1 places the
 * `sw ra,0x34(sp)` prologue save BEFORE the g_pGameState load + li 1 block,
 * target places it after. Both fn-address temps (a0/a3), the 0x38 frame
 * (PadSlot + dead PadSlot hole), scratch reuse, and the tail (marker temp +
 * fence, sb in jal delay) are exact. Not source-reachable (same class as
 * PlayerSetupCallbacksAndSprite); permuter 5min floored at 60. Draft kept in
 * nonmatchings/PlayerState_HideAndClearBounce/base.c. */
INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_HideAndClearBounce);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_CheckpointExit);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerStateInit_FallFromLedge);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerStateInit_TeleportFallVariant);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerStateInit_FallingWithInput);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerStateInit_Falling);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerStateInit_TeleportLanding);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerStateInit_TeleportEntry);

extern void PlayerSetupBounceAnimation(PlayerEntity *e);

void PlayerBounceCallback_Primary(PlayerEntity *e) {
    e->landingTimer = 8;
    e->bounceLockTimer = 8;
    e->altSpeed = 0;
    e->velocityY_fixed = -0x24000;
    e->jumpHoldCounter = 8;
    PlayerSetupBounceAnimation(e);
}

void PlayerState_BounceLaunch(PlayerEntity *e) {
    e->landingTimer = 8;
    e->bounceLockTimer = 8;
    e->sprite.base.facing = 1;
    e->altSpeed = 0x28000;
    e->velocityY_fixed = -0x24000;
    e->jumpHoldCounter = 8;
    PlayerSetupBounceAnimation(e);
}

void PlayerState_SetupBounceDown(PlayerEntity *e) {
    e->landingTimer = 4;
    e->bounceLockTimer = 4;
    e->sprite.base.facing = 1;
    e->altSpeed = 0x28000;
    e->velocityY_fixed = -0x24000;
    e->jumpHoldCounter = 0xA;
    PlayerSetupBounceAnimation(e);
}

void PlayerState_SetupBounceRight(PlayerEntity *e) {
    e->sprite.base.facing = 1;
    e->landingTimer = 0;
    e->bounceLockTimer = 0;
    e->altSpeed = 0x28000;
    e->velocityY_fixed = 0x24000;
    e->jumpHoldCounter = 8;
    PlayerSetupBounceAnimation(e);
}

void PlayerState_SetupBounceUp(PlayerEntity *e) {
    e->velocityY_fixed = 0x24000;
    e->landingTimer = 0;
    e->bounceLockTimer = 0;
    e->altSpeed = 0;
    e->jumpHoldCounter = 8;
    PlayerSetupBounceAnimation(e);
}

void PlayerState_SetupBounceUpWithVelocity(PlayerEntity *e) {
    s32 alt = -0x28000;
    e->velocityY_fixed = 0x24000;
    e->landingTimer = 0;
    e->bounceLockTimer = 0;
    e->sprite.base.facing = 0;
    e->altSpeed = alt;
    e->jumpHoldCounter = 8;
    PlayerSetupBounceAnimation(e);
}

void PlayerState_QuickTurn(PlayerEntity *e) {
    e->landingTimer = 4;
    e->bounceLockTimer = 4;
    e->sprite.base.facing = 0;
    e->altSpeed = -0x28000;
    e->velocityY_fixed = -0x24000;
    e->jumpHoldCounter = 0xA;
    PlayerSetupBounceAnimation(e);
}

void PlayerState_SetupBounceDownFast(PlayerEntity *e) {
    e->landingTimer = 8;
    e->bounceLockTimer = 8;
    e->sprite.base.facing = 0;
    e->altSpeed = -0x28000;
    e->velocityY_fixed = -0x24000;
    e->jumpHoldCounter = 8;
    PlayerSetupBounceAnimation(e);
}

INCLUDE_ASM("asm/nonmatchings/playst", PlayerSetupBounceAnimation);

u32 PlayerLevelExitMarker asm("D_800A5F2C");
EntityCallback PlayerLevelExitFn asm("D_800A5F30");

void PlayerState_TransitionToLevelExit(PlayerEntity *e) {
    EntitySetState((Entity *)e, PlayerLevelExitMarker, PlayerLevelExitFn);
}

INCLUDE_ASM("asm/nonmatchings/playst", PlayerStateInit_ClimbIdle);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerStateInit_DamageKnockback);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_DamageKnockback);

void PlayerDestroyVoiceCallback(PlayerEntity *e) {
    StopSPUVoice(e->soundHandle);
}

INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_CrouchOrClimbTransition);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerStateInit_ClimbWithMirror);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerStateInit_HeavyDamageLaunch);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerStateInit_DamageRecoilFall);

void PlayerState_RemoveAttachedEntity(PlayerEntity *e) {
    Entity *portal;
    g_pGameState->camera_follow_direction = 0;
    portal = e->swirlPortalEntity;
    if (portal != NULL) {
        RemoveEntityFromAllLists(g_pGameState, portal);
        e->swirlPortalEntity = NULL;
    }
}

INCLUDE_ASM("asm/nonmatchings/playst", PlayerStateInit_ThrowProjectile);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerStateInit_CheckpointRestore);

void PlayerState_CheckpointRestoreComplete(PlayerEntity *e) {
    g_pGameState->checkpoint_restore_pending = 0;
    if (e->invincibilityTimer == 0) {
        e->invincibilityTimer = 1;
    }
}

INCLUDE_ASM("asm/nonmatchings/playst", UniverseEnemaActivate);

INCLUDE_ASM("asm/nonmatchings/playst", UniverseEnemaKillAllEnemies);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerStateInit_CheckpointEntry);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerStateInit_ProjectileThrowAnim);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerStateInit_PostThrowRecovery);

void PlayerState_ClearCheckpointAndSetInvincible(PlayerEntity *e) {
    g_pGameState->checkpoint_restore_pending = 0;
    if (e->invincibilityTimer == 0) {
        e->invincibilityTimer = 1;
    }
}

INCLUDE_ASM("asm/nonmatchings/playst", PlayerEnterNormalState);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerStateInit_RespawnAtCheckpoint);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerEnterShrinkZone);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerExitShrinkZone);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerStateInit_ExitShrinkWithRestore);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerStateInit_RestoreNormalFromShrink);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerStateInit_GrowFromShrink);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerStateInit_ShrinkAndFall);

void PlayerCallback_RestoreNormalHitbox(PlayerEntity *e) {
    SetEntityStateFlagWithValue(e->glideEntity, 0);
    e->groundedFlag = 0;
    g_pGameState->bounce_active_flag = 0;
    g_pGameState->player_hitbox_width = 0x28;
    g_pGameState->player_hitbox_y_offset = -0x30;
}

INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_StartSwimming);

INCLUDE_ASM("asm/nonmatchings/playst", HamsterSpriteCallback);

INCLUDE_ASM("asm/nonmatchings/playst", InitShadowMirrorSubentity);

INCLUDE_ASM("asm/nonmatchings/playst", EntityTick_ShadowMirror);

INCLUDE_ASM("asm/nonmatchings/playst", InitHUDAnimatedEntity);

INCLUDE_ASM("asm/nonmatchings/playst", CreateHaloEntity);

extern u8 PLAYER_DESTRUCTOR_VTABLE_17A4[] asm("D_800117A4");
extern u8 PLAYST_PLAYER_CALLBACK_TABLE[] asm("g_PlayerCallbackTable");

void EntityDestructor_FreeResourceAt24(Entity *e, s32 flags) {
    e->collisionVtable = PLAYER_DESTRUCTOR_VTABLE_17A4;
    FreeFromHeap(g_pBlbHeapBase, *(u8 **)&e->moveMarkerX, 0, 0);
    e->collisionVtable = PLAYST_PLAYER_CALLBACK_TABLE + 0x20;
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)e, 0, 0);
    }
}





