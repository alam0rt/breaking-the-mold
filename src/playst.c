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
 * FSM-slot dispatch t-reg family (see Game/fsm_dispatch.h): held pinned $a0
 * (folds the final held+xOff into the callback's first arg), fn/$t2 home with
 * $t1 relay, separate s16 survivor copy (a1->v0->a3), and an in-place
 * `vel = (s16)vel` truncation statement (target schedules it before the
 * marker test, so it cannot be a cast at the call). */
typedef void (*ReleaseNotifyCallback)(void *target, s32 event, s32 vel,
                                      PlayerEntity *sender);
void PlayerNotifyEntityRelease(PlayerEntity *e) {
    FSM_REG(Entity *, held, "$4");
    FSM_REG(s32, rawVelY, "$5");
    s32 vel;
    s16 markerHi;
    s32 markerLo;
    s32 xOff;
    s32 slotArg;

    held = e->interactEntity;
    if (held != NULL) {
        FSM_REG(ReleaseNotifyCallback, fn2, "$10");
        rawVelY = e->velocityY_fixed;
        vel = rawVelY;
        if (e->sprite.base.scalePowerupX == 0x8000) {
            vel = (s32)(rawVelY << 16) >> 17;
        }
        FSM_KEEP_LIVE(rawVelY);
        vel = (s16)vel;
        markerHi = *(s16 *)((u8 *)held + 0xA);
        if (markerHi != 0) {
            s16 s = markerHi;
            if (markerHi > 0) {
                FSM_REG(ReleaseNotifyCallback, ft, "$9");
                u8 *tbl = *(u8 **)((u8 *)held + *(s16 *)((u8 *)held + 0xC));
                slotArg = *(s32 *)(tbl + markerHi * 8 - 8);
                ft = *(ReleaseNotifyCallback *)(tbl + markerHi * 8 - 4);
                FSM_RELAY(fn2, ft);
            } else {
                fn2 = *(ReleaseNotifyCallback *)((u8 *)held + 0xC);
            }
            markerLo = *(s16 *)((u8 *)held + 8);
            if (s > 0) xOff = (s16)slotArg + markerLo;
            else xOff = markerLo;
            fn2((u8 *)held + xOff, 0x1005, vel, e);
        }
        e->interactEntity = NULL;
    }
}

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

/* Player/enemy overlap test that, on a hit, drives the player into the same
 * four damage outcomes as PlayerProcessEnemyHit. The enemy's screen AABB is
 * shrunk on its top edge by (hitboxExpand>>16) - 0xD (doubled while the
 * player is mid grow/shrink, scalePowerupX == 0x8000), then the player's
 * worldX must lie inside the enemy's horizontal span and the player's screen
 * bottom must reach below that adjusted top edge. Ignored while the player is
 * invincible (+0x128).
 *
 * Byte-match levers: `hitboxExpand *= 2;` as a statement reproduces the
 * in-place `sll s0,s0,1`; the adj if/else split (not assign-then-overwrite)
 * keeps adj's range clear of the scale/0x8000 compare so the else-arm sra
 * fills the bne delay; FSM_KEEP_LIVE(hitboxExpand) after the if adds the ref
 * that wins it s0 over the player in the saved-register priority tie
 * (target: hitboxExpand->s0, player->s1, enemy->s2) while keeping the
 * doubled def alive (before the if, the *=2 store gets elided into a temp);
 * adj pinned $v0. */
extern void GetEntityScreenBounds(Entity *entity, s16 *out);
void CheckPlayerHitByEnemy(PlayerEntity *e, Entity *enemy, s32 hitboxExpand) {
    s16 enemyBounds[4], playerBounds[4], playerX;
    FSM_REG(s32, adj, "$2");
    s32 marker; EntityCallback fn;
    GetEntityScreenBounds(enemy, enemyBounds);
    GetEntityScreenBounds((Entity *)e, playerBounds);
    if (e->sprite.base.scalePowerupX == 0x8000) {
        hitboxExpand *= 2;
        adj = hitboxExpand >> 16;
    } else {
        adj = hitboxExpand >> 16;
    }
    FSM_KEEP_LIVE(hitboxExpand);
    adj -= 0xD;
    if ((enemyBounds[1] - adj) < playerBounds[3]) {
        playerX = e->sprite.base.worldX;
        if (playerX >= enemyBounds[0] && enemyBounds[2] >= playerX &&
            e->invincibilityTimer == 0) {
            if (PLAYER_STATE_DATA->powerup_flags & 1) {
                if (playerX < enemy->worldX) { marker = PlayerQuickTurnMarker; fn = PlayerQuickTurnFn; }
                else { marker = PlayerBounceDownMarker; fn = PlayerBounceDownFn; }
            } else if (*(u16 *)&e->powerupTimer != 0) { marker = PlayerRespawnMarker; fn = PlayerRespawnFn; }
            else { marker = PlayerDeathMarker; fn = PlayerDeathFn; }
            EntitySetState((Entity *)e, marker, fn);
        }
    }
}

/* Bulk-installs four caller-supplied (marker, callback) FSM slots onto an
 * entity -- tick (+0x00), event (+0x08), +0x104, and render (+0x1C) -- each
 * marshalled through a shared local scratch, then swaps the sprite. Used to
 * wire up a player-helper entity's callbacks in one shot.
 *
 * Structural discovery: the target's double memory round-trip per pair
 * (shadow-arg -> scratch -> entity, not a single shadow -> entity copy) only
 * falls out of source if the four (marker,fn) pairs are BY-VALUE `SetupSlot`
 * struct PARAMETERS (not raw scalar marker/fn args) assigned into a reused
 * scratch local before the store to the entity -- gcc-2.7.2's o32 aggregate
 * ABI packs a byval SetupSlot across regs/stack exactly like two scalars
 * (tick in a1/a2, event.marker in a3, the rest spilling to the stack args
 * area), so the register/stack layout is identical either way; only the
 * codegen shape differs. spriteId (the last, stack-passed scalar arg) is
 * pinned $a1 and re-assigned right after entry so it re-spills to its own
 * shadow slot immediately, holding it live across the whole body the way the
 * target does (an unpinned local instead gets reloaded from the stack just
 * before the SetEntitySpriteId call). */
typedef struct { s32 marker; void *fn; } SetupSlot;
typedef struct { s32 pad; SetupSlot s; } PaddedSetupSlot;
extern s32 SetEntitySpriteId(PlayerEntity *e, u32 spriteId, s32 flag);

void PlayerSetupCallbacksAndSprite(Entity *e, SetupSlot tick, SetupSlot event,
                                   SetupSlot slot104, SetupSlot render,
                                   u32 spriteId) {
    PaddedSetupSlot u;
    FSM_REG(u32, sid, "$5");
    sid = spriteId;
    u.s = tick;
    *(SetupSlot *)&e->tickMarker = u.s;
    u.s = event;
    *(SetupSlot *)((u8 *)e + 0x8) = u.s;
    u.s = slot104;
    *(SetupSlot *)((u8 *)e + 0x104) = u.s;
    u.s = render;
    *(SetupSlot *)((u8 *)e + 0x1C) = u.s;
    SetEntitySpriteId((PlayerEntity *)e, sid, 1);
}

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
 * SHELVED -- one instruction from a match. The verified C below reproduces
 * the outer/inner branch structure+polarity, the early per-arm `base =
 * &e->sprite.base` arg setup (pinned $a0, matching the target's schedule of
 * arg setup ahead of the reroll store), and every register assignment. The
 * lone residual: the long-timer reroll `(rand() & 0x100) + 0x200` -- cc1's
 * combine pass proves the AND and the added constant's bits never overlap
 * and canonicalizes the ADD into an OR (`ori`), but the target keeps a real
 * `addiu`. Confirmed NOT source-reachable: tried commuted operands, an
 * intervening named/pinned temp, a shift+multiply reformulation (folds back
 * to the same AND by constant propagation), and reordering relative to the
 * `base` assignment -- cc1 reaches the identical canonical RTL every time
 * and always emits `ori`. Equivalent C (parked in
 * nonmatchings/PlayerState_IdleRandom):
 *   extern s32 rand(void);
 *   u32 PlayerIdleAnimAMarker asm("D_800A5DA0");
 *   EntityCallback PlayerIdleAnimAFn asm("D_800A5DA4");
 *   u32 PlayerIdleAnimBMarker asm("D_800A5DA8");
 *   EntityCallback PlayerIdleAnimBFn asm("D_800A5DAC");
 *   void PlayerState_IdleRandom(PlayerEntity *e) {
 *       u32 marker; EntityCallback fn; u16 reroll;
 *       Entity *base;
 *       e->fidgetTimerLong -= 1;
 *       if (e->fidgetTimerLong == 0) {
 *           base = &e->sprite.base;
 *           reroll = (rand() & 0x100) + 0x200;  // target: addiu; cc1: ori
 *           marker = PlayerIdleAnimAMarker; fn = PlayerIdleAnimAFn;
 *           e->fidgetTimerLong = reroll;
 *       } else {
 *           e->fidgetTimerShort -= 1;
 *           if (e->fidgetTimerShort != 0) goto skip;
 *           base = &e->sprite.base;
 *           reroll = (rand() & 0x7F) + 0x30;
 *           marker = PlayerIdleAnimBMarker; fn = PlayerIdleAnimBFn;
 *           e->fidgetTimerShort = reroll;
 *       }
 *       EntitySetState(base, marker, fn);
 *   skip:
 *       PlayerTickCallback(e);
 *   } */
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

/* Pickup/pull-in transition tick. Unless the "pickup in progress" game flag is
 * set, transitions the player into the death FSM state (when the active sprite
 * id is the death marker 0x1E28E0D4) or the pickup state otherwise, flags the
 * sprite context visible (+0xA) and marks the gamestate grounded (+0x170).
 * Then dispatches the current input-state callback (standard marker decode on
 * inputStateMarker/inputStateCallback), runs the standard entity update, and
 * decays the invincibility timer.
 *
 * Register pins (byte-match levers, see Game/fsm_dispatch.h):
 *  - `one`/$v0: cse invalidates hard regs (not pseudos) at calls, so pinning
 *    the stored 1 forces `li v0,1` to re-materialize on both sides of the
 *    EntitySetState call; unpinned, cc1 folds both 1s into one pseudo live
 *    across the call, promotes it to s0, and rotates every saved register.
 *  - `base`/$a0: assigned first in the else arm; cse deletes the fallthrough
 *    arm's copy (a0 still holds e from entry), leaving the target's
 *    else-arm-only `move a0,s0`, which also blocks the delay-filler from
 *    hoisting the per-arm `li v0,1` into the bne delay slot.
 *  - decode block: standard FSM-slot dispatch recipe — fn2/$a1 home with
 *    $s3 relay, slotArg/$s2, separate `s16 s` survivor copy (a0->v0->a2),
 *    xOff/$v0 so the cast chain doesn't compute in-place in s2. */
extern void EntitySetState(Entity *e, u32 marker, EntityCallback fn);
u32 PLAYER_DEATH_STATE_MARKER asm("D_800A5D98");
EntityCallback PLAYER_DEATH_STATE_CALLBACK asm("D_800A5D9C");
u32 PLAYER_PICKUP_STATE_MARKER asm("D_800A5DB0");
EntityCallback PLAYER_PICKUP_STATE_CALLBACK asm("D_800A5DB4");

void PlayerState_TransitionToPickup(PlayerEntity *e) {
    s16 markerHi;
    s32 markerLo;
    FSM_REG(s32, xOff, "$2");
    if (!(g_GameFlags & 1)) {
        u8 *sctx; u32 marker; EntityCallback stateFn;
        FSM_REG(s32, one, "$2");
        FSM_REG(Entity *, base, "$4");
        if (e->sprite.currentSpriteId == 0x1E28E0D4) {
            marker = PLAYER_DEATH_STATE_MARKER;
            stateFn = PLAYER_DEATH_STATE_CALLBACK;
            sctx = e->sprite.base.spriteContext;
            base = &e->sprite.base;
            one = 1;
        } else {
            base = &e->sprite.base;
            marker = PLAYER_PICKUP_STATE_MARKER;
            stateFn = PLAYER_PICKUP_STATE_CALLBACK;
            sctx = e->sprite.base.spriteContext;
            one = 1;
            e->disableScale = 0;
        }
        sctx[0xA] = one;
        EntitySetState(base, marker, stateFn);
        one = 1;
        ((u8 *)g_pGameState)[0x170] = one;
    }
    markerHi = *(s16 *)((u8 *)e + 0x106);
    if (markerHi != 0) {
        s16 s = markerHi;
        FSM_REG(EntityCallback, fn2, "$5");
        FSM_REG(s32, slotArgS, "$18");
        if (markerHi > 0) {
            FSM_REG(EntityCallback, ft, "$19");
            u8 *tbl = *(u8 **)((u8 *)e + *(s16 *)((u8 *)e + 0x108));
            slotArgS = *(s32 *)(tbl + markerHi * 8 - 8);
            ft = *(EntityCallback *)(tbl + markerHi * 8 - 4);
            FSM_RELAY(fn2, ft);
        } else {
            fn2 = *(EntityCallback *)((u8 *)e + 0x108);
        }
        markerLo = *(s16 *)((u8 *)e + 0x104);
        if (s > 0) xOff = (s16)slotArgS + markerLo;
        else xOff = markerLo;
        fn2((Entity *)((u8 *)e + xOff));
    }
    EntityUpdateCallback(&e->sprite.base);
    if (e->invincibilityTimer != 0) e->invincibilityTimer -= 1;
}

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

/* Per-tick sync of the player's attached rider entity: copies the player's
 * world X/Y (Y biased by attachedYOff) onto the attached entity, mirrors the
 * player's current RGB tint into the attached entity's sprite context
 * (+0x34..+0x36), and sets the context's glow flag (+0x38) from the player's
 * powerup timer. */
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

/* Bounce collision callback: forwards to PlayerEntityCollisionHandler (return
 * value passed through), drains the callback queue on event 2, and on event 1
 * with arg2 matching the bounce-source hash 0x1A800898 applies a hitstun
 * bounce: landingTimer=5, cushionVelY=-0x1200, velocityY -= 6.0 (16.16 fixed,
 * clamped to -9.0), zeroes bounceLockTimer/jumpHoldCounter/altSpeed, installs
 * PlayerCallback_FallingPhysicsMain as the render-slot FSM callback, and (if
 * an entity is attached via interactEntity) fires its move-Y FSM slot with
 * event 0x1005 ("released") forwarding the (possibly halved) velocity, same
 * as PlayerNotifyEntityRelease. */
extern void PlayerCallback_FallingPhysicsMain(PlayerEntity *e);
typedef void (*BareFn2)();

s32 PlayerBounceCallback(PlayerEntity *e, u32 event, u32 arg2, u32 arg3) {
    FSM_REG(s32, result, "$18");
    FSM_REG(u32, maskedEvent, "$17");
    FSM_REG(u32, arg2s, "$19");
    maskedEvent = event & 0xFFFF;
    arg2s = arg2;
    result = PlayerEntityCollisionHandler(e, maskedEvent, arg2s, arg3);
    if (maskedEvent == 2) {
        EntityProcessCallbackQueue((Entity *)e);
    }
    if (maskedEvent == 1 && arg2s == 0x1A800898) {
        s32 v;
        FSM_REG(Entity *, held, "$4");
        e->landingTimer = 5;
        e->cushionVelY = -0x1200;
        v = e->velocityY_fixed + (s32)0xFFFA0000;
        e->bounceLockTimer = 0;
        e->jumpHoldCounter = 0;
        e->altSpeed = 0;
        e->velocityY_fixed = v;
        if (v < (s32)0xFFF70000) {
            e->velocityY_fixed = (s32)0xFFF70000;
        }
        {
            PadSlot slot;
            FSM_REG(BareFn2, fn, "$3");
            fn = (BareFn2)PlayerCallback_FallingPhysicsMain;
            FSM_KEEP_LIVE(fn);
            slot.s.markerLo = 0; slot.s.markerHi = -1; slot.s.fn = fn;
            *(CallbackSlot *)&e->sprite.base.renderMarker = slot.s;
        }
        held = e->interactEntity;
        if (held != NULL) {
            FSM_REG(s32, rawVelY, "$5");
            s32 vel;
            s16 markerHi;
            s32 markerLo;
            FSM_REG(s32, xOff, "$2");
            FSM_REG(s32, slotArg, "$20");
            FSM_REG(ReleaseNotifyCallback, fn2, "$8");

            rawVelY = e->velocityY_fixed;
            vel = rawVelY;
            if (e->sprite.base.scalePowerupX == 0x8000) {
                vel = (s32)(rawVelY << 16) >> 17;
            }
            FSM_KEEP_LIVE(rawVelY);
            vel = (s16)vel;
            markerHi = *(s16 *)((u8 *)held + 0xA);
            if (markerHi != 0) {
                s16 s = markerHi;
                if (markerHi > 0) {
                    FSM_REG(ReleaseNotifyCallback, ft, "$21");
                    u8 *tbl = *(u8 **)((u8 *)held + *(s16 *)((u8 *)held + 0xC));
                    slotArg = *(s32 *)(tbl + markerHi * 8 - 8);
                    ft = *(ReleaseNotifyCallback *)(tbl + markerHi * 8 - 4);
                    FSM_RELAY(fn2, ft);
                } else {
                    fn2 = *(ReleaseNotifyCallback *)((u8 *)held + 0xC);
                }
                markerLo = *(s16 *)((u8 *)held + 8);
                if (s > 0) xOff = (s16)slotArg + markerLo;
                else xOff = markerLo;
                fn2((u8 *)held + xOff, 0x1005, vel, e);
            }
            e->interactEntity = NULL;
        }
    }
    return result;
}

/* Player event handler installed during the teleporter (level-exit) animation.
 * Forwards the event to PlayerEntityEventHandler (return value passed through),
 * drains the callback queue on event 2, and on event 1 dispatches the current
 * anim marker (arg2) to one of three actions:
 *   - 0x2809A510: spawn the swirl portal entity
 *   - 0x382C92C1: spawn a dust puff behind the player (X +0x14, or -0x14 when
 *                 facing left; Y = worldY + apexVelocity; sprite 0x49289C14)
 *   - 0x221017CA: if a special move is queued, re-arm the swirl anim callback
 *                 and clear the queued flag, else clear the special-move mode.
 *
 * SHELVED: the verified C below reproduces every instruction EXCEPT the
 * InitParticleEntity packedXY setup in the 0x382C92C1 case. The target reloads
 * BOTH pos.x/pos.y from their stack slots (lhu 0x20/0x22) after AllocateFromHeap
 * and fills the store window with the reloads, growing the frame to 0x48; gcc
 * instead keeps pos.x live in a register and emits a 0x40 frame. This is the
 * same non-source-reachable dust-particle reload/spill residual documented on
 * PlayerSpawnDustParticle / SpawnPlayerDeathEffect — no source form (u32 temp,
 * explicit pack, flipped operands, split statements) dislodges it. Every other
 * instruction (the full BST switch, both non-particle cases, the queue drain,
 * and both callback calls) matches byte-for-byte. Retained as INCLUDE_ASM.
 *
 *   extern void SpawnSwirlPortalEntity(PlayerEntity *e);
 *   extern Entity *InitParticleEntity(u8 *mem, u32 spriteId, u32 packedXY,
 *                       u8 facing, s32 scaleRender, s32 arg5, s32 arg6);
 *   s32 PlayerCallback_TeleporterAnimHandler(PlayerEntity *e, u32 event,
 *                                            u32 arg2, u32 arg3) {
 *       struct { u16 x; u16 y; } pos;
 *       u8 *mem; Entity *particle; s32 result;
 *       u32 maskedEvent = event & 0xFFFF;
 *       result = PlayerEntityEventHandler(e, maskedEvent, arg2, arg3);
 *       if (maskedEvent == 2) {
 *           EntityProcessCallbackQueue((Entity *)e);
 *       }
 *       if (maskedEvent == 1) {
 *           switch (arg2) {
 *           case 0x2809A510:
 *               SpawnSwirlPortalEntity(e);
 *               break;
 *           case 0x382C92C1:
 *               pos.x = (u16)e->sprite.base.worldX + 0x14;
 *               if (e->sprite.base.facing != 0) {
 *                   pos.x = (u16)e->sprite.base.worldX - 0x14;
 *               }
 *               pos.y = (u16)e->sprite.base.worldY + (u16)e->apexVelocity;
 *               mem = AllocateFromHeap(g_pBlbHeapBase, 0x108, 1, 0);
 *               particle = InitParticleEntity(mem, 0x49289C14, pos.x | (pos.y << 16),
 *                              e->sprite.base.facing, e->sprite.base.scalePowerupX, 0x3D4, 0);
 *               AddEntityToSortedRenderList((u8 *)g_pGameState, particle);
 *               break;
 *           case 0x221017CA:
 *               if (e->specialMoveQueued != 0) {
 *                   SetAnimationFrameCallback(e, 0x2809A510);
 *                   e->specialMoveQueued = 0;
 *               } else {
 *                   e->specialMoveMode = 0;
 *               }
 *               break;
 *           }
 *       }
 *       return result;
 *   } */
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

u32 PlayerPowerupPhoenixMarker asm("D_800A5DD8");
EntityCallback PlayerPowerupPhoenixFn asm("D_800A5DDC");
u32 PlayerPowerupWillieMarker asm("D_800A5DE0");
EntityCallback PlayerPowerupWillieFn asm("D_800A5DE4");
u32 PlayerPowerupThirdMarker asm("D_800A5DE8");
EntityCallback PlayerPowerupThirdFn asm("D_800A5DEC");
u32 PlayerPowerupFourthMarker asm("D_800A5DF0");
EntityCallback PlayerPowerupFourthFn asm("D_800A5DF4");

/*
 * Reads the freshly-pressed pad buttons and, for whichever powerup button is
 * down (checked in bit order 4/1/8/2), activates the matching powerup if the
 * player owns it (per-powerup count in PLAYER_STATE_DATA): sets the player
 * FSM state and returns 1. If the button is pressed but the powerup is owned
 * with a zero count (or is held-item-blocked), plays the "denied" cue and
 * returns 0.
 */
s32 TryActivatePowerup(PlayerEntity *e) {
    u16 buttons = e->pInput->buttons_pressed;
    s32 avail = 0;

    if (buttons & 4) {
        if (((u8 *)PLAYER_STATE_DATA)[0x14] != 0) {
            EntitySetState((Entity *)e, PlayerPowerupPhoenixMarker,
                           PlayerPowerupPhoenixFn);
            return 1;
        }
        avail = 1;
    } else if (buttons & 1) {
        if (e->interactEntity == NULL) {
            if (((u8 *)PLAYER_STATE_DATA)[0x15] != 0) {
                EntitySetState((Entity *)e, PlayerPowerupWillieMarker,
                               PlayerPowerupWillieFn);
                return 1;
            }
        }
        avail = 1;
    } else if (buttons & 8) {
        if (((u8 *)PLAYER_STATE_DATA)[0x16] != 0) {
            EntitySetState((Entity *)e, PlayerPowerupThirdMarker,
                           PlayerPowerupThirdFn);
            return 1;
        }
        avail = 1;
    } else if (buttons & 2) {
        if (((u8 *)PLAYER_STATE_DATA)[0x1C] != 0) {
            EntitySetState((Entity *)e, PlayerPowerupFourthMarker,
                           PlayerPowerupFourthFn);
            return 1;
        }
        avail = 1;
    }

    if ((u8)avail != 0) {
        PlayEntityPositionSound((Entity *)e, 0x64221E61);
    }
    return 0;
}

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

/* Per-tick input handler while walking-into-a-fall. Picks the directional turn
 * bit from facing (right -> 0x2000, left -> 0x8000). When the FSM allows special
 * input (PLAYER_STATE_DATA[0x13]) and the "look up" mask is pressed, it either
 * queues the special move (if specialMoveMode is set) or ducks. Otherwise, on
 * the "drop through" mask it probes the tile above (worldY-0x40) and hands off
 * to duck/pickup unless blocked (tile 0x65). With no such input it tries a
 * powerup, then either a directional turn (unless the turn-block mask is held)
 * or a land/landAlt hand-off. All seven hand-offs share a single EntitySetState
 * tail.
 *
 * SHELVED: the C below reproduces the exact logic, but two register-allocation
 * residuals at the shared EntitySetState tail-merge are not source-reachable
 * under gcc-2.7.2's regalloc (same class as the sibling PlayerCallback_
 * FallInputHandler, whose flip runs the OPPOSITE direction — proof it is a
 * coin-flip): (1) the original reloads the CheckTileCollisionOverride out-param
 * `tile` into v0 and tests it with `nop; xori v0,v0,0x65; beqz`, whereas cc1
 * parks tile in v1 and materialises `li v0,0x65; bne v1,v0`. That one-slot codegen
 * difference makes cc1's body 4 bytes shorter, cascading a uniform -4 offset drift
 * onto every GP-relative marker/mask load after it (and the PLAYER_STATE_DATA
 * absolute load), so the whole tail mis-diffs. (2) the shared `dirMask & held`
 * test canonicalises to `and v0,v1,s1` (held first) under cc1 but `and v0,s1,v1`
 * (dirMask first) in the original; commutative-operand order is not source-
 * reachable. Neither the goto tail-merge form nor `held & dirMask` reorderings
 * dislodge them, so the original asm is retained to preserve the byte match.
 *
 *   extern s32 CheckTileCollisionOverride(PlayerEntity *entity, u8 *tile);
 *   u32 WalkFallSpecialDuckMarker asm("D_800A5E38");
 *   EntityCallback WalkFallSpecialDuckFn asm("D_800A5E3C");
 *   u32 WalkFallDuckMarker asm("D_800A5E00");
 *   EntityCallback WalkFallDuckFn asm("D_800A5E04");
 *   u32 WalkFallPickupMarker asm("D_800A5DB0");
 *   EntityCallback WalkFallPickupFn asm("D_800A5DB4");
 *   u32 WalkFallLandMarker asm("D_800A5E40");
 *   EntityCallback WalkFallLandFn asm("D_800A5E44");
 *   u32 WalkFallLandAltMarker asm("D_800A5E48");
 *   EntityCallback WalkFallLandAltFn asm("D_800A5E4C");
 *   u32 WalkFallTurnMarker asm("D_800A5E58");
 *   EntityCallback WalkFallTurnFn asm("D_800A5E5C");
 *   extern u16 g_WalkFallLookInputMask asm("D_800A596E");
 *   extern u16 g_WalkFallDropInputMask asm("D_800A596C");
 *   extern u16 g_WalkFallTurnBlockMask asm("D_800A5970");
 *
 *   void PlayerCallback_WalkInputWithFall(PlayerEntity *e) {
 *       u32 dirMask;
 *       u8 tile;
 *       u32 marker;
 *       EntityCallback fn;
 *       u16 held;
 *
 *       dirMask = 0x2000;
 *       if (e->sprite.base.facing != 0) {
 *           dirMask = 0x8000;
 *       }
 *       if (((u8 *)PLAYER_STATE_DATA)[0x13] != 0) {
 *           if ((e->pInput->buttons_pressed & g_WalkFallLookInputMask) != 0) {
 *               if (e->specialMoveMode != 0) {
 *                   e->specialMoveQueued = 1;
 *                   return;
 *               }
 *               marker = WalkFallSpecialDuckMarker;
 *               fn = WalkFallSpecialDuckFn;
 *               goto setState;
 *           }
 *       }
 *       if ((e->pInput->buttons_pressed & g_WalkFallDropInputMask) != 0) {
 *           tile = EntityApplyMovementCallbacks((Entity *)e, e->sprite.base.worldX,
 *                                               (s16)(*(u16 *)&e->sprite.base.worldY - 0x40));
 *           CheckTileCollisionOverride(e, &tile);
 *           if (tile == 0x65) {
 *               return;
 *           }
 *           if (e->specialMoveMode != 0) {
 *               marker = WalkFallDuckMarker;
 *               fn = WalkFallDuckFn;
 *           } else {
 *               marker = WalkFallPickupMarker;
 *               fn = WalkFallPickupFn;
 *           }
 *           goto setState;
 *       }
 *       if ((u8)TryActivatePowerup(e) != 0) {
 *           return;
 *       }
 *       held = e->pInput->buttons_held;
 *       if ((dirMask & held) != 0) {
 *           if ((g_WalkFallTurnBlockMask & held) != 0) {
 *               return;
 *           }
 *           marker = WalkFallTurnMarker;
 *           fn = WalkFallTurnFn;
 *       } else if (e->specialMoveMode != 0) {
 *           marker = WalkFallLandMarker;
 *           fn = WalkFallLandFn;
 *       } else {
 *           marker = WalkFallLandAltMarker;
 *           fn = WalkFallLandAltFn;
 *       }
 *   setState:
 *       EntitySetState((Entity *)e, marker, fn);
 *   }
 */
INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_WalkInputWithFall);

/* Per-tick input handler for the jump/wall-collision state. Picks forward/back
 * turn masks from facing (facing right -> fwd 0x8000/back 0x2000, facing left ->
 * swapped). When the FSM allows special input (PLAYER_STATE_DATA->swirly_q_count)
 * and the "look" mask is pressed, it either queues the special move (specialMoveMode
 * set) or hands off to the special-look state. On the "drop" mask it probes the
 * tile above (worldY-0x40) and ducks/pickups unless blocked (tile 0x65). Otherwise
 * it tries a powerup, then on a forward-turn press wall-checks and (if blocked)
 * flips facing + hands off to the wall-turn state, or on a back press bails, else
 * lands. All seven hand-offs share a single EntitySetState tail.
 *
 * SHELVED: the C below reproduces the exact logic (every branch, all seven
 * dispatch pairs, the facing flip, and both mask tests match byte-for-byte) EXCEPT
 * one non-source-reachable register-allocation residual at the CheckTileCollisionOverride
 * out-param reload — the identical coin-flip already documented on the sibling
 * PlayerCallback_WalkInputWithFall. The original reloads `tile` into v0 and tests
 * it with `lbu v0; nop; xori v0,v0,0x65; beqz` (4 slots), whereas cc1 parks tile
 * in v1 and materialises `lbu v1; li v0,0x65; bne v1,v0` (3 slots). That one-slot
 * codegen difference makes cc1's body 4 bytes shorter, cascading a uniform -4
 * offset drift onto every GP-relative marker/mask load after it, so the whole
 * tail mis-diffs. Neither the goto tail-merge form nor `== 0x65` / `!= 0x65`
 * reorderings dislodge it (same as the sibling), so the asm is retained.
 *
 *   extern s32 CheckWallCollision(Entity *entity, s32 dir);
 *   extern s32 CheckTileCollisionOverride(PlayerEntity *entity, u8 *tile);
 *   extern u16 g_JumpLookInputMask asm("D_800A596E");
 *   extern u16 g_JumpDropInputMask asm("D_800A596C");
 *   u32 JumpSpecialLookMarker asm("D_800A5E38");
 *   EntityCallback JumpSpecialLookFn asm("D_800A5E3C");
 *   u32 JumpDropSpecialMarker asm("D_800A5E00");
 *   EntityCallback JumpDropSpecialFn asm("D_800A5E04");
 *   u32 JumpWallSpecialMarker asm("D_800A5E08");
 *   EntityCallback JumpWallSpecialFn asm("D_800A5E0C");
 *   u32 JumpWallTurnMarker asm("D_800A5E18");
 *   EntityCallback JumpWallTurnFn asm("D_800A5E1C");
 *   u32 JumpLandSpecialMarker asm("D_800A5E40");
 *   EntityCallback JumpLandSpecialFn asm("D_800A5E44");
 *   u32 JumpLandMarker asm("D_800A5E60");
 *   EntityCallback JumpLandFn asm("D_800A5E64");
 *   (pickup pair reuses PLAYER_PICKUP_STATE_MARKER/CALLBACK @ D_800A5DB0/4)
 *
 *   void PlayerCallback_JumpWallCollisionInput(PlayerEntity *e) {
 *       u32 fwdMask; u32 backMask; u8 tile; u32 marker; EntityCallback fn; u16 held;
 *       backMask = 0x2000;
 *       fwdMask = 0x8000;
 *       if (e->sprite.base.facing != 0) {
 *           backMask = 0x8000;
 *           fwdMask = 0x2000;
 *       }
 *       if (PLAYER_STATE_DATA->swirly_q_count != 0) {
 *           if ((e->pInput->buttons_pressed & g_JumpLookInputMask) != 0) {
 *               if (e->specialMoveMode != 0) {
 *                   e->specialMoveQueued = 1;
 *                   return;
 *               }
 *               marker = JumpSpecialLookMarker; fn = JumpSpecialLookFn;
 *               goto setState;
 *           }
 *       }
 *       if ((e->pInput->buttons_pressed & g_JumpDropInputMask) != 0) {
 *           tile = EntityApplyMovementCallbacks((Entity *)e, e->sprite.base.worldX,
 *                       (s16)(*(u16 *)&e->sprite.base.worldY - 0x40));
 *           CheckTileCollisionOverride(e, &tile);
 *           if (tile == 0x65) return;
 *           if (e->specialMoveMode != 0) {
 *               marker = JumpDropSpecialMarker; fn = JumpDropSpecialFn;
 *           } else {
 *               marker = PLAYER_PICKUP_STATE_MARKER; fn = PLAYER_PICKUP_STATE_CALLBACK;
 *           }
 *           goto setState;
 *       }
 *       if ((u8)TryActivatePowerup(e) != 0) return;
 *       held = e->pInput->buttons_held;
 *       if ((fwdMask & held) != 0) {
 *           if ((u8)CheckWallCollision((Entity *)e, e->sprite.base.facing < 1) == 0) return;
 *           {
 *               u8 newFacing = e->sprite.base.facing < 1;
 *               u8 sm = e->specialMoveMode;
 *               e->sprite.base.facing = newFacing;
 *               if (sm != 0) { marker = JumpWallSpecialMarker; fn = JumpWallSpecialFn; }
 *               else         { marker = JumpWallTurnMarker;    fn = JumpWallTurnFn; }
 *           }
 *           goto setState;
 *       }
 *       if ((backMask & held) != 0) return;
 *       if (e->specialMoveMode != 0) { marker = JumpLandSpecialMarker; fn = JumpLandSpecialFn; }
 *       else                        { marker = JumpLandMarker;        fn = JumpLandFn; }
 *   setState:
 *       EntitySetState((Entity *)e, marker, fn);
 *   }
 */
INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_JumpWallCollisionInput);

/* PlayerCallback_JumpInputAndCounters: unit spans 0x800602E0..0x800605D8 — absorbs former split symbols PlayerCallback_JumpDirectionAndCounters (Ghidra labels with no external references; merged 2026-07-02). */
INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_JumpInputAndCounters);


/* PlayerCallback_JumpTickHandler: unit spans 0x800605D8..0x80060890 — absorbs former split symbols PlayerCallback_SetStateAndUpdateCounters, PlayerCallback_JumpCounterUpdate (Ghidra labels with no external references; merged 2026-07-02). */
INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_JumpTickHandler);


/* Per-tick input handler while the player is falling. Reads controller
 * buttons_pressed and hands the FSM off to the appropriate state: a special-move
 * mask (queued when specialMoveMode is set, else the special state), a "look up"
 * mask that probes the tile above (worldY-0x40) and branches to duck/pickup, a
 * left/right turn (SQUARE/CIRCLE toggle facing) — or, with no directional input,
 * a powerup activation attempt followed by a land/landAlt hand-off. All seven
 * hand-off branches share a single EntitySetState tail.
 *
 * SHELVED: the C below reproduces the exact logic, but two register-allocation /
 * scheduling residuals at the shared EntitySetState tail-merge are not
 * source-reachable under gcc-2.7.2's regalloc/sched2 (same class as
 * PlayerCallback_VerticalCollisionCheck): (1) the original defers `a0 = e`
 * (move a0,s0) to the single merge point, placing it in the EntitySetState jal
 * delay slot, whereas cc1 coalesces e into a0 early and pre-positions the move in
 * each predecessor branch (so the facing store becomes sb v0,0x74(a0) instead of
 * the original's sb v0,0x74(s0)); (2) the original parks the CheckTileCollision
 * out-param `tile` in v1 and materialises `li v0,0x65` into the lbu load-delay
 * slot for the == compare, while cc1 parks tile in v0 and stalls with a nop.
 * Neither the shared-var (goto tail-merge) form nor per-branch EntitySetState
 * calls dislodge them, so the original asm is retained to preserve the byte match.
 *
 *   extern s32 CheckTileCollisionOverride(PlayerEntity *entity, u8 *tile);
 *   extern u16 g_PlayerFallSpecialInputMask asm("D_800A596E");
 *   extern u16 g_PlayerFallLookInputMask asm("D_800A596C");
 *   u32 PlayerFallSpecialMarker   asm("D_800A5E88");
 *   EntityCallback PlayerFallSpecialFn  asm("D_800A5E8C");
 *   u32 PlayerFallDuckMarker      asm("D_800A5E00");
 *   EntityCallback PlayerFallDuckFn     asm("D_800A5E04");
 *   u32 PlayerFallPickupMarker    asm("D_800A5DB0");
 *   EntityCallback PlayerFallPickupFn   asm("D_800A5DB4");
 *   u32 PlayerFallTurnMarker      asm("D_800A5E90");
 *   EntityCallback PlayerFallTurnFn     asm("D_800A5E94");
 *   u32 PlayerFallLandMarker      asm("D_800A5E98");
 *   EntityCallback PlayerFallLandFn     asm("D_800A5E9C");
 *   u32 PlayerFallLandAltMarker   asm("D_800A5EA0");
 *   EntityCallback PlayerFallLandAltFn  asm("D_800A5EA4");
 *
 *   void PlayerCallback_FallInputHandler(PlayerEntity *e) {
 *       u16 buttons;
 *       u8 tile;
 *       u32 marker;
 *       EntityCallback fn;
 *       if (((u8 *)PLAYER_STATE_DATA)[0x13] != 0) {
 *           if ((e->pInput->buttons_pressed & g_PlayerFallSpecialInputMask) != 0) {
 *               if (e->specialMoveMode != 0) { e->specialMoveQueued = 1; return; }
 *               marker = PlayerFallSpecialMarker; fn = PlayerFallSpecialFn;
 *               goto setState;
 *           }
 *       }
 *       buttons = e->pInput->buttons_pressed;
 *       if ((buttons & g_PlayerFallLookInputMask) != 0) {
 *           tile = EntityApplyMovementCallbacks((Entity *)e, e->sprite.base.worldX,
 *                      (s16)(e->sprite.base.worldY - 0x40));
 *           CheckTileCollisionOverride(e, &tile);
 *           if (tile == 0x65) return;
 *           if (e->specialMoveMode != 0) { marker = PlayerFallDuckMarker; fn = PlayerFallDuckFn; }
 *           else { marker = PlayerFallPickupMarker; fn = PlayerFallPickupFn; }
 *           goto setState;
 *       }
 *       if ((buttons & 0x8000) != 0) {
 *           if (e->sprite.base.facing != 0) return;
 *           e->sprite.base.facing = 1;
 *           marker = PlayerFallTurnMarker; fn = PlayerFallTurnFn; goto setState;
 *       }
 *       if ((buttons & 0x2000) != 0) {
 *           if (e->sprite.base.facing == 0) return;
 *           e->sprite.base.facing = 0;
 *           marker = PlayerFallTurnMarker; fn = PlayerFallTurnFn; goto setState;
 *       }
 *       if ((u8)TryActivatePowerup(e) != 0) return;
 *       if ((e->pInput->buttons_held & 0x4000) != 0) return;
 *       if (e->specialMoveMode != 0) { marker = PlayerFallLandMarker; fn = PlayerFallLandFn; }
 *       else { marker = PlayerFallLandAltMarker; fn = PlayerFallLandAltFn; }
 *   setState:
 *       EntitySetState((Entity *)e, marker, fn);
 *   }
 */
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
 * ALSO TRIED (2026-07-03, hard-reg-pin arsenal from PlayerState_TransitionToPickup
 * et al.): pinning the boolean to $v1 with a second $v0-pinned `cond = boolVal;`
 * copy, plus FSM_KEEP_LIVE barriers on both -- cc1's compare-and-branch peephole
 * uses whichever register holds the value at the branch regardless of which
 * named/pinned pseudo it's assigned through, and always elides the trivial
 * copy since nothing else ever needs `cond` in a distinct register. Confirms
 * the original verdict from a different angle: not source-reachable.
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

extern s16 TransformYCoordWithScaleSnapped(Entity *e, s32 val);

u32 PlayerKnockbackCeilingMarker asm("D_800A5EC0");
EntityCallback PlayerKnockbackCeilingFn asm("D_800A5EC4");
u32 PlayerKnockbackLandMarker asm("D_800A5EF4");
EntityCallback PlayerKnockbackLandFn asm("D_800A5EF8");

/* Inlined tile-override check (mirrors CheckTileCollisionOverride in player.c):
 * destructible tiles are phased through while invincible (rewriting the probe
 * result to PLAYER_TILE_PASSABLE) and block otherwise. */
static inline u8 KnockbackTileOverride(PlayerEntity *e, u8 *tile) {
    u8 t = *tile;
    if ((u8)(t + 0x4B) < 3 || (t & 0xFF) == 0xC9 || (t & 0xFF) == 0xCB ||
        (u8)(t + 0x23) < 3) {
        if (e->invincibilityTimer != 0) {
            *tile = 0x65;
            return 0;
        }
        return 1;
    }
    return 0;
}

void PlayerCallback_KnockbackPhysics(PlayerEntity *e) {
    s32 posX;
    s32 posY;
    u8 tileUp;
    u8 tileDown;

    posX = (e->sprite.base.worldX << 16) + *(u16 *)&e->sprite.base.velocityX;
    posY = (e->sprite.base.worldY << 16) + *(u16 *)&e->sprite.base.velocityY;
    if (e->sprite.base.facing != 0) {
        posX -= e->sprite.frameMotionX;
    } else {
        posX += e->sprite.frameMotionX;
    }
    if (e->sprite.base.flipY != 0) {
        posY -= e->sprite.frameMotionY;
    } else {
        posY += e->sprite.frameMotionY;
    }
    e->sprite.base.worldX = posX >> 16;
    e->sprite.base.worldY = posY >> 16;
    *(s16 *)&e->sprite.base.velocityX = posX;
    *(s16 *)&e->sprite.base.velocityY = posY;
    if (e->sprite.frameMotionY < 0) {
        tileUp = EntityApplyMovementCallbacks((Entity *)e, e->sprite.base.worldX,
                                              e->sprite.base.worldY - 0x40);
        if (KnockbackTileOverride(e, &tileUp)) {
            g_pGameState->camera_follow_direction = 0;
            if (e->swirlPortalEntity != NULL) {
                RemoveEntityFromAllLists(g_pGameState, e->swirlPortalEntity);
                e->swirlPortalEntity = NULL;
            }
            PlayerProcessBounceCollision(e, tileUp);
        } else if (tileUp != 0x7D) {
            e->sprite.base.worldY = TransformYCoordWithScaleSnapped(
                                        (Entity *)e,
                                        (s16)(e->sprite.base.worldY - 0x30)) + 0x40;
            *(s16 *)&e->sprite.base.velocityY = 0;
            EntitySetState((Entity *)e, PlayerKnockbackCeilingMarker,
                           PlayerKnockbackCeilingFn);
        }
    } else {
        tileDown = EntityApplyMovementCallbacks((Entity *)e,
                                                e->sprite.base.worldX,
                                                e->sprite.base.worldY);
        if (KnockbackTileOverride(e, &tileDown)) {
            g_pGameState->camera_follow_direction = 0;
            if (e->swirlPortalEntity != NULL) {
                RemoveEntityFromAllLists(g_pGameState, e->swirlPortalEntity);
                e->swirlPortalEntity = NULL;
            }
            PlayerProcessBounceCollision(e, tileDown);
        } else if (tileDown != 0x7D) {
            EntitySetState((Entity *)e, PlayerKnockbackLandMarker,
                           PlayerKnockbackLandFn);
        }
    }
}

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

/*
 * PlayerState_IdleLookAround (0x80066F70, 0x15C) — SHELVED 2026-07-03.
 *
 * Installs the idle look-around FSM: tick=PlayerTickCallback,
 * event=PlayerCallback_EventHandlerWithQueue, input=PlayerCallback_IdleInputHandler,
 * render=(e->interactEntity ? PlayerCallback_RidingPlatformPhysics
 *                           : PlayerCallback_HorizontalWallCollision),
 * SetEntitySpriteId(e, 0x5900C41E, 1), then queued=PlayerCallback_SetIdleStateCallbacks.
 *
 * Verified C (instruction-for-instruction identical; the ONLY residual is the
 * stack-frame layout coin-flip — same class as PlayerStateInit_FallingWithInput):
 * target uses frame 0x68 with the six CallbackSlot scratch areas at
 * sp+0x14/0x1C/0x24/0x2C(render)/0x38(cond scratch)/0x44(double-copy temp), i.e.
 * a leading pad plus two internal 4-byte pads before the scratch and temp slots;
 * cc1 packs the six bare CallbackSlot locals tight (frame 0x48, slots at
 * 0x10/0x18/0x20/0x28/0x30/0x38). Wrapping in padded structs 8-aligns the group
 * and shifts scratch/temp the wrong way (proven exhaustively for FallingWithInput).
 * Not source-reachable.
 *
 *   extern void PlayerTickCallback();
 *   extern void PlayerCallback_IdleInputHandler();
 *   extern void PlayerCallback_HorizontalWallCollision();
 *   extern void PlayerCallback_RidingPlatformPhysics();
 *   extern void PlayerCallback_SetIdleStateCallbacks();
 *   // PlayerCallback_EventHandlerWithQueue, SetEntitySpriteId already declared above.
 *
 *   void PlayerState_IdleLookAround(PlayerEntity *e) {
 *       CallbackSlot tick, event, input, render, scratch, cur;
 *       tick.markerLo = 0;  tick.markerHi = -1;  tick.fn = (void (*)())PlayerTickCallback;
 *       event.markerLo = 0; event.markerHi = -1;
 *       event.fn = (void (*)())PlayerCallback_EventHandlerWithQueue;
 *       input.markerLo = 0; input.markerHi = -1;
 *       input.fn = (void (*)())PlayerCallback_IdleInputHandler;
 *       scratch.markerLo = 0; scratch.markerHi = -1;
 *       if (e->interactEntity != NULL)
 *           scratch.fn = (void (*)())PlayerCallback_RidingPlatformPhysics;
 *       else
 *           scratch.fn = (void (*)())PlayerCallback_HorizontalWallCollision;
 *       render = scratch;
 *       cur = tick;   *(CallbackSlot *)&e->sprite.base.tickMarker   = cur;
 *       cur = event;  *(CallbackSlot *)&e->sprite.base.eventMarker  = cur;
 *       cur = input;  *(CallbackSlot *)&e->inputStateMarker         = cur;
 *       cur = render; *(CallbackSlot *)&e->sprite.base.renderMarker = cur;
 *       SetEntitySpriteId(e, 0x5900C41E, 1);
 *       cur.markerLo = 0; cur.markerHi = -1;
 *       cur.fn = (void (*)())PlayerCallback_SetIdleStateCallbacks;
 *       *(CallbackSlot *)&e->sprite.queuedStateMarker = cur;
 *   }
 */
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

/* PlayerState_SpecialMove2 @ 0x80067CF0 (0x138) -- installs the special-move
 * FSM. Sets apexVelocity(0x136)/specialMoveMode(0x135)/carryMotionX(0x10C)
 * scalars, then blocks in five FSM (marker,fn) pairs through a single reused
 * stack scratch (frame 0x28, scratch at sp+0x14): tick(0x0)->PlayerTickCallback,
 * event(0x8)->PlayerCallback_TeleporterAnimHandler, input(0x104)->
 * PlayerCallback_IdleInputHandler, render(0x1C) chosen at runtime by
 * e->interactEntity (RidingPlatformPhysics / HorizontalWallCollision),
 * SetAnimationSpriteFlags(e, 0x8D01C734, 1), queued(0x98)->PlayerStateInit_Idle,
 * then EntitySetCallback(e, D_800A5F04, D_800A5F08).
 *
 * SHELVED (installer register-allocation coin-flip). The verified C below
 * reproduces the 0x28 frame exactly (via PadSlot: scratch at sp+0x14, s0@0x20,
 * ra@0x24), every scalar store, all five (lo,hi,fn) installs with per-slot
 * re-stores, the interactEntity render conditional, and both helper calls with
 * the correct args/offsets. The ONLY residual is register naming: the target
 * holds the constant markerHi (-1) in $a2 across the first four slots (`li a2,-1`
 * once, `sh a2,0x16` x4) because $v0/$v1 are reserved for the two copy temps
 * (`lw v0,0x14; lw v1,0x18; sw v0; sw v1`). cc1 2.7.2 here allocates -1 to $v1
 * instead, forcing the copy temp to $a0, and reloads -1 fresh per region -- a
 * uniform hold=$a2/temp=$v1 vs hold=$v1/temp=$a0 shift, plus the downstream
 * render-slot markerLo scheduling (target hoists `sh zero,0x14` into both branch
 * arms; cc1 puts the fn store in the j-delay slot). Not source-reachable: the
 * markerLo/markerHi must be re-assigned each slot (the block copy reads them),
 * and the held-constant hard-register choice ($a2 vs $v1) is an allocator
 * coin-flip -- same class as the shelved sibling PlayerStateInit_FallingWithInput.
 *
 *   extern void PlayerTickCallback();
 *   extern void PlayerCallback_TeleporterAnimHandler();
 *   extern void PlayerCallback_IdleInputHandler();
 *   extern void PlayerCallback_RidingPlatformPhysics();
 *   extern void PlayerCallback_HorizontalWallCollision();
 *   extern void PlayerStateInit_Idle();
 *   extern void SetAnimationSpriteFlags(PlayerEntity *e, u32 spriteId, s32 flags);
 *   extern void EntitySetCallback(Entity *e, u32 marker, EntityCallback fn);
 *   u32 PlayerSpecialMoveNextMarker asm("D_800A5F04");
 *   EntityCallback PlayerSpecialMoveNextFn asm("D_800A5F08");
 *
 *   void PlayerState_SpecialMove2(PlayerEntity *e) {
 *       PadSlot u;
 *       e->apexVelocity = -0x28;
 *       e->specialMoveMode = 1;
 *       e->carryMotionX = 0;
 *       u.s.markerLo = 0; u.s.markerHi = -1;
 *       u.s.fn = (void (*)())PlayerTickCallback;
 *       *(CallbackSlot *)&e->sprite.base.tickMarker = u.s;
 *       u.s.markerLo = 0; u.s.markerHi = -1;
 *       u.s.fn = (void (*)())PlayerCallback_TeleporterAnimHandler;
 *       *(CallbackSlot *)&e->sprite.base.eventMarker = u.s;
 *       u.s.markerLo = 0; u.s.markerHi = -1;
 *       u.s.fn = (void (*)())PlayerCallback_IdleInputHandler;
 *       *(CallbackSlot *)&e->inputStateMarker = u.s;
 *       u.s.markerLo = 0; u.s.markerHi = -1;
 *       if (e->interactEntity != NULL)
 *           u.s.fn = (void (*)())PlayerCallback_RidingPlatformPhysics;
 *       else
 *           u.s.fn = (void (*)())PlayerCallback_HorizontalWallCollision;
 *       *(CallbackSlot *)&e->sprite.base.renderMarker = u.s;
 *       SetAnimationSpriteFlags(e, 0x8D01C734, 1);
 *       u.s.markerLo = 0; u.s.markerHi = -1;
 *       u.s.fn = (void (*)())PlayerStateInit_Idle;
 *       *(CallbackSlot *)&e->sprite.queuedStateMarker = u.s;
 *       EntitySetCallback((Entity *)e, PlayerSpecialMoveNextMarker,
 *                         PlayerSpecialMoveNextFn);
 *   }
 */
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

/*
 * PlayerState_LevelExitTeleporter (0x8006A214, 0xFC) — SHELVED (installer
 * frame-padding coin-flip). Verified C below reproduces the exact algorithm;
 * the compiled body is instruction-identical to the original EXCEPT the stack
 * frame: gcc-2.7.2 packs the reused 8-byte CallbackSlot staging temp tightly
 * (frame 0x30, temp@0x14, s0@0x20), while the original reserves 0x18 extra
 * bytes of dead padding (frame 0x48, temp@0x14, 28-byte gap 0x1C-0x37, s0@0x38).
 * The frame-size delta cascades into uniform s/i prologue/epilogue offset diffs;
 * there is no within-body diff. Frame padding on these marker-installer routines
 * is a non-source-reachable gcc coin-flip (see the PlayerStateInit_* family),
 * so the asm is retained to preserve the byte-match. The fn-pointer register
 * (target v1) is pinned with `register ... asm("$3")` as in
 * PlayerCallback_CollisionDamageSetup.
 *
 * void PlayerState_LevelExitTeleporter(PlayerEntity *e) {
 *     PadSlot slot;
 *     void *p;
 *     void *q;
 *
 *     *(u8 *)((u8 *)e + 0x1B2) = 1;
 *     StopCDStreaming();
 *     p = *(void **)((u8 *)e + 0x168);
 *     if (p != NULL) {
 *         *(u8 *)((u8 *)p + 0x2C) = 1;
 *         *(void **)((u8 *)e + 0x168) = NULL;
 *     }
 *     *(u8 *)((u8 *)e + 0x128) = 0;
 *     *(u32 *)((u8 *)e + 0x10C) = 0;
 *     *(u32 *)((u8 *)e + 0x110) = 0;
 *
 *     slot.s.markerLo = 0;
 *     slot.s.markerHi = -1;
 *     {
 *         register void (*fn)() asm("$3") = (void (*)())EntityUpdateCallback;
 *         slot.s.fn = fn;
 *     }
 *     *(CallbackSlot *)&e->sprite.base.tickMarker = slot.s;
 *     slot.s.markerLo = 0; slot.s.markerHi = 0; slot.s.fn = 0;
 *     *(CallbackSlot *)&e->sprite.base.eventMarker = slot.s;
 *     slot.s.markerLo = 0; slot.s.markerHi = 0; slot.s.fn = 0;
 *     *(CallbackSlot *)&e->inputStateMarker = slot.s;
 *     slot.s.markerLo = 0; slot.s.markerHi = 0; slot.s.fn = 0;
 *     *(CallbackSlot *)&e->sprite.base.renderMarker = slot.s;
 *
 *     SetAnimationTargetFrameIndex((Entity *)e, e->sprite.currentFrame);
 *     EntitySetRenderFlags((Entity *)e, 0);
 *     q = *(void **)((u8 *)e + 0x34);
 *     *(u8 *)((u8 *)q + 0xA) = 0;
 * }
 */
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
 * SHELVED. Corrects the prior note's diagnosis: the "4-instr prologue
 * rotation" was actually a missing 16 bytes of frame -- a SECOND, unused
 * PadSlot-sized local (`volatile PadSlot unused;`, needed to defeat dead-
 * store elimination) reproduces the target's 0x38 frame exactly, the same
 * "dead pad" idiom as PlayerStateInit_FallingWithInput/PlayerState_JumpApex.
 * With that fixed, the only residual is scheduling: target computes BOTH
 * fn-address temps (tick's `lui a0/addiu a0` AND event's `lui a3/addiu a3`)
 * before the g_pGameState load + `li v0,1` flag-store block; cc1 always
 * defers one of the two (whichever isn't immediately reused) until right
 * before its first use, and reuses that temp's now-dead register for the
 * -1 marker constant instead of allocating a fresh one. Pinning tickFn to
 * $a0 fixes exactly that first block, but the pinned pseudo's extended
 * hard-reg lifetime then perturbs an unrelated tail choice (ctx computed
 * into a reused $s0 instead of $v0) -- a net wash, not source-reachable
 * without a coupled pin covering both temps + the tail, which was not
 * found. Equivalent C (parked in nonmatchings/PlayerState_HideAndClearBounce):
 *   typedef void (*BareFn)();
 *   void PlayerState_HideAndClearBounce(PlayerEntity *e) {
 *       PadSlot slot;
 *       volatile PadSlot unused;
 *       BareFn tickFn, eventFn;
 *       GameState *gs;
 *       RenderSprite *ctx;
 *       tickFn = (BareFn)EntityUpdateCallback;
 *       eventFn = (BareFn)PlayerEvent_ZoneTriggerHandler;
 *       gs = g_pGameState;
 *       gs->bounce_active_flag = 1;
 *       slot.s.markerLo = 0; slot.s.markerHi = -1; slot.s.fn = tickFn;
 *       *(CallbackSlot *)&e->sprite.base.tickMarker = slot.s;
 *       slot.s.markerLo = 0; slot.s.markerHi = -1; slot.s.fn = eventFn;
 *       *(CallbackSlot *)&e->sprite.base.eventMarker = slot.s;
 *       slot.s.markerLo = 0; slot.s.markerHi = 0; slot.s.fn = 0;
 *       *(CallbackSlot *)&e->inputStateMarker = slot.s;
 *       slot.s.markerLo = 0; slot.s.markerHi = 0; slot.s.fn = 0;
 *       *(CallbackSlot *)&e->sprite.base.renderMarker = slot.s;
 *       SetAnimationTargetFrameIndex((Entity *)e, e->sprite.currentFrame);
 *       EntitySetRenderFlags((Entity *)e, 0);
 *       ctx = (RenderSprite *)e->sprite.base.spriteContext;
 *       EntitySetCallback((Entity *)e, PlayerHiddenIdleMarker, PlayerHiddenIdleFn);
 *       ctx->enabled = 0;
 *   } */
INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_HideAndClearBounce);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_CheckpointExit);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerStateInit_FallFromLedge);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerStateInit_TeleportFallVariant);

/* PlayerStateInit_FallingWithInput @ 0x8006A818 -- installs the falling-with-
 * input FSM: four CallbackSlots ({markerLo=0, markerHi=-1, fn}) written to the
 * entity's tick(0x0)/event(0x8)/input(0x104)/render(0x1C) marker fields, then
 * SetEntitySpriteId(e, 0x0E0AC230, 1). The render slot's fn is chosen at
 * runtime: e->interactEntity ? PlayerCallback_RidingPlatformPhysics
 *                            : PlayerCallback_HorizontalWallCollision.
 *
 * Verified C (semantically exact, instruction structure matches byte-for-byte
 * including the build-all-first shared-temp double-copy staging):
 *
 *   void PlayerStateInit_FallingWithInput(PlayerEntity *e) {
 *       CallbackSlot tick, event, input, render, scratch, cur;
 *       tick.markerLo = 0;  tick.markerHi = -1;  tick.fn = PlayerTickCallback;
 *       event.markerLo = 0; event.markerHi = -1;
 *       event.fn = (void (*)())PlayerEntityEventHandler;
 *       input.markerLo = 0; input.markerHi = -1;
 *       input.fn = PlayerCallback_FallInputHandler;
 *       scratch.markerLo = 0; scratch.markerHi = -1;
 *       if (e->interactEntity != NULL)
 *           scratch.fn = PlayerCallback_RidingPlatformPhysics;
 *       else
 *           scratch.fn = PlayerCallback_HorizontalWallCollision;
 *       render = scratch;
 *       cur = tick;   *(CallbackSlot *)&e->sprite.base.tickMarker   = cur;
 *       cur = event;  *(CallbackSlot *)&e->sprite.base.eventMarker  = cur;
 *       cur = input;  *(CallbackSlot *)&e->inputStateMarker         = cur;
 *       cur = render; *(CallbackSlot *)&e->sprite.base.renderMarker = cur;
 *       SetEntitySpriteId(e, 0xE0AC230, 1);
 *   }
 *
 * SHELVED (stack-frame layout coin-flip): the C above reproduces the EXACT
 * instruction sequence -- build-all-first (six distinct slot scratch areas),
 * conditional render via a scratch copied into render, then each slot copied
 * through a shared temp into the entity (slotN -> temp -> field). The ONLY
 * residual is the local stack layout: target frame is 0x60 with the six slot
 * areas at sp+0x14/0x1C/0x24/0x2C(render)/0x38(scratch)/0x44(temp+cur), i.e. a
 * leading 4-byte pad plus 4-byte pads before the scratch and temp slots. cc1
 * 2.7.2 packs six bare CallbackSlot locals tighter (frame 0x48, no leading pad)
 * -> uniform +4 offset shift. Wrapping the first four in a padded group struct
 * plus two PadSlots fixes tick/event/input/render exactly but gcc 8-aligns the
 * group and each PadSlot, pushing scratch/temp +4/+8 high. The precise pad
 * placement is not source-reachable (same class as the shelved sibling
 * PlayerState_JumpApex). */
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

/*
 * PlayerStateInit_ClimbIdle (0x8006B304, 0xF4) — SHELVED.
 *
 * Multi-slot "installer": stages four 8-byte callback slots on the stack, then
 * copies each into the entity through a single shared 8-byte scratch temp
 * (double-copy: slots[i] -> t@sp+0x3C -> entity), then SetEntitySpriteId.
 *
 * Verified C (logic byte-exact; the whole copy body matches as pure stack-offset
 * shifts). Requires these forward decls (moved here so the shelve leaves no
 * stray externs; PlayerEntityEventHandlerAlt is already declared at file top):
 *     extern void PlayerTickCallback();
 *     extern void PlayerCallback_CrouchClimbInputHandler();
 *
 * void PlayerStateInit_ClimbIdle(Entity *e) {
 *     CallbackSlot slots[4];
 *     CallbackSlot t;
 *     slots[0].markerLo = 0; slots[0].markerHi = -1;
 *     slots[0].fn = (void (*)())PlayerTickCallback;
 *     slots[1].markerLo = 0; slots[1].markerHi = -1;
 *     slots[1].fn = (void (*)())PlayerEntityEventHandlerAlt;
 *     slots[2].markerLo = 0; slots[2].markerHi = -1;
 *     slots[2].fn = (void (*)())PlayerCallback_CrouchClimbInputHandler;
 *     slots[3].markerLo = 0; slots[3].markerHi = 0; slots[3].fn = 0;
 *     t = slots[0]; *(CallbackSlot *)((u8 *)e + 0x0)   = t;
 *     t = slots[1]; *(CallbackSlot *)((u8 *)e + 0x8)   = t;
 *     t = slots[2]; *(CallbackSlot *)((u8 *)e + 0x104) = t;
 *     t = slots[3]; *(CallbackSlot *)((u8 *)e + 0x1C)  = t;
 *     SetEntitySpriteId((PlayerEntity *)e, 0x708A4A0, 1);
 * }
 *
 * Residual (non-source-reachable):
 *   - Frame size: original reserves 0x58 (slots@0x14-0x33, shared temp@0x3C,
 *     ra@0x50, with an 8-byte gap 0x34-0x3B and 12-byte pad 0x44-0x4F). The C
 *     above yields frame 0x40 (slots@0x10, temp@0x30) — the extra padding is a
 *     gcc stack-allocation choice with no C-source lever. slots[5]/slots[7]
 *     grow the frame but never reproduce the exact base offsets + gap.
 *   - Top-of-function marker-store scheduling: original loads each slot's fn
 *     then stores (markerLo, markerHi, fn) per slot with v0=-1 hoisted once;
 *     the C batches the fn-pointer stores separately. A gcc scheduling coin-flip.
 * Matching requires the permuter (not run per project policy).
 */
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

/*
 * UniverseEnemaKillAllEnemies (0x8006C278, 0x134) — SHELVED (register-allocation coin-flip).
 *
 * Verified C below is logically and STRUCTURALLY exact: after the `s32 off` fix
 * (forces the signed `lh` load of obj+8, killing a spurious `lhu`), the compiled
 * body is instruction-identical to the original modulo register numbers only.
 *
 * void UniverseEnemaKillAllEnemies(PlayerEntity *player) {
 *     EntityListNode *node;
 *     Entity *obj;
 *     s16 cnt;
 *     void (*fn)();
 *     s32 off;
 *     s32 rec0;
 *
 *     node = g_pGameState->collision_list_head;
 *     while (node != 0) {
 *         obj = node->entity;
 *         if (*(u16 *)((u8 *)obj + 0x12) & 4) {
 *             cnt = *(s16 *)((u8 *)obj + 0xA);
 *             if (cnt != 0) {
 *                 if (cnt > 0) {
 *                     s32 *base = *(s32 **)((u8 *)obj + *(s16 *)((u8 *)obj + 0xC));
 *                     s32 *rec = base + cnt * 2;
 *                     rec0 = rec[-2];
 *                     fn = (void (*)())rec[-1];
 *                 } else {
 *                     fn = (void (*)()) * (s32 *)((u8 *)obj + 0xC);
 *                 }
 *                 off = *(s16 *)((u8 *)obj + 8);
 *                 if (cnt > 0) {
 *                     off = (s16)rec0 + off;
 *                 }
 *                 fn((u8 *)obj + off, 0x1002, 0, player);
 *             }
 *         }
 *         node = node->next;
 *     }
 *     PLAYER_STATE_DATA->universe_enemas--;
 *     g_pGameState->checkpoint_restore_pending = 0;
 *     if (*(u8 *)((u8 *)player + 0x128) == 0) {
 *         *(u8 *)((u8 *)player + 0x128) = 1;
 *     }
 * }
 *
 * Residual (non-source-reachable): gcc-2.7.2 assigns the short-lived `rec0` to s1
 * and copies `player` into s2 *inside* the loop, leaving `fn` in caller-saved t0 —
 * 3 callee-saved regs, frame 0x28. The original copies `player` into s1 at function
 * ENTRY, then rec0->s2 and fn->s3 (with a `move t0,s3` before the call) — 4 callee-
 * saved regs, frame 0x30. This is purely which pseudo wins s1 and where the param
 * copy is inserted; no C-source lever changes it (tried u32/s16 rec0, dead-store
 * rec0=0, s32 off, struct-typed record read, for/while loop forms). Needs permuter.
 * The frame-size delta produces downstream gp-offset size-cascade i-diffs that are
 * not real within-function bugs.
 */
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

extern Entity *InitEntityWithSprite(Entity *entity, u8 *spriteDef, s32 z, s16 x, s16 y);
extern u8 D_8009C254[];
extern u8 D_800117E4[];
extern void EntityTick_ShadowMirror();
extern s32 ScaleXByEntityScale(Entity *entity, s16 value);
extern s32 ScaleYByEntityScale(Entity *entity, s16 value);

/* Allocates+inits a shadow/mirror subentity: sprite from D_8009C254 at
 * z-order 0x424, installs the D_800117E4 collision vtable, stashes the owner
 * at +0x100 and allocSize 0x3E9, then wires EntityTick_ShadowMirror as the
 * per-frame tick and ScaleX/YByEntityScale as the worldX/Y move-transform FSM
 * slots (marker 0xFFFF0000 = direct call). Returns the entity. */
Entity *InitShadowMirrorSubentity(Entity *e, void *owner) {
    TripadSlot u;
    s16 m1;
    void (*fn)();

    InitEntityWithSprite(e, D_8009C254, 0x424, 0, 0);
    e->collisionVtable = D_800117E4;
    *(void **)((u8 *)e + 0x100) = owner;
    e->allocSize = 0x3E9;
    do {} while (0);
    fn = (void (*)())EntityTick_ShadowMirror;
    m1 = -1;
    u.s.markerLo = 0;
    u.s.markerHi = m1;
    u.s.fn = fn;
    *(CallbackSlot *)&e->tickMarker = u.s;
    fn = (void (*)())ScaleXByEntityScale;
    u.s.markerLo = 0;
    u.s.markerHi = m1;
    u.s.fn = fn;
    *(CallbackSlot *)&e->moveMarkerX = u.s;
    fn = (void (*)())ScaleYByEntityScale;
    u.s.markerLo = 0;
    u.s.markerHi = m1;
    u.s.fn = fn;
    *(CallbackSlot *)&e->moveMarkerY = u.s;
    return e;
}

/* EntityTick_ShadowMirror @ 0x8006DBBC (size 0x1DC): per-frame tick for the
 * shadow/mirror subentity. Copies the owner (source player, stashed at +0x100
 * by InitShadowMirrorSubentity) facing/position, remaps the owner's active
 * sprite id through a fixed 6-entry lookup, syncs the animation frame, mirrors
 * the owner's RGB tint (+ a sprite-context flag byte), and inherits its render
 * and powerup scales before running the standard render update.
 *
 * SHELVED: register-allocation coin-flip in the trailing field-copy tail. The
 * switch decision-tree (the source-reachable logic) matches byte-for-byte after
 * ordering the case bodies as below. The residual is entirely non-source:
 *   (1) the SetAnimationFrameIndex arg lands in v0 (re-sign-extended into a1 via
 *       sll/sra) in the target vs directly in a1 (lh) in the build;
 *   (2) the 3-byte RGB copy (tint local + struct-copy to spriteContext+0x34)
 *       keeps all three bytes live in a0/a1/v1 in the target but only v0/v1 in
 *       the build, forcing lbu reloads from the dead sp temp;
 *   (3) that pressure cascades into the +0x38 copy and scale-block scheduling
 *       (a stray pInput reload).
 * None of (1)-(3) is reachable from C source form; verified C below.
 *
 *   extern void SetAnimationFrameIndex(Entity *e, s32 frame);
 *   typedef struct { u8 r; u8 g; u8 b; } RgbTriple;
 *
 *   void EntityTick_ShadowMirror(PlayerEntity *e) {
 *       u32 spriteId;
 *       RgbTriple tint;
 *
 *       e->sprite.base.facing = ((PlayerEntity *)e->pInput)->sprite.base.facing;
 *       *(u16 *)&e->sprite.base.worldX = *(u16 *)&((PlayerEntity *)e->pInput)->sprite.base.worldX;
 *       *(u16 *)&e->sprite.base.worldY = *(u16 *)&((PlayerEntity *)e->pInput)->sprite.base.worldY;
 *
 *       spriteId = e->sprite.currentSpriteId;
 *       switch (((PlayerEntity *)e->pInput)->sprite.currentSpriteId) {
 *       case 0x0628A800: spriteId = 0xA628E009; break;
 *       case 0x112AE088: spriteId = 0x8139A088; break;
 *       case 0x8569A090: spriteId = 0x9629A000; break;
 *       case 0x0708A4A0: spriteId = 0xD70880A4; break;
 *       case 0x052AA082: spriteId = 0x0538A2EA; break;
 *       case 0x8524A880: spriteId = 0x2524E089; break;
 *       }
 *       if (spriteId != e->sprite.currentSpriteId) {
 *           SetEntitySpriteId(e, spriteId, 1);
 *       }
 *
 *       if (((PlayerEntity *)e->pInput)->sprite.currentFrame != e->sprite.currentFrame) {
 *           SetAnimationFrameIndex((Entity *)e, ((PlayerEntity *)e->pInput)->sprite.currentFrame);
 *       }
 *
 *       tint.r = ((PlayerEntity *)e->pInput)->currentRGB[0];
 *       tint.g = ((PlayerEntity *)e->pInput)->currentRGB[1];
 *       tint.b = ((PlayerEntity *)e->pInput)->currentRGB[2];
 *       *(RgbTriple *)((u8 *)e->sprite.base.spriteContext + 0x34) = tint;
 *       *(u8 *)((u8 *)e->sprite.base.spriteContext + 0x38) =
 *           *(u8 *)((u8 *)((PlayerEntity *)e->pInput)->sprite.base.spriteContext + 0x38);
 *
 *       e->sprite.base.scaleRender = ((PlayerEntity *)e->pInput)->sprite.base.scaleRender;
 *       e->sprite.base.scaleRender2 = ((PlayerEntity *)e->pInput)->sprite.base.scaleRender;
 *       e->sprite.base.scalePowerupX = ((PlayerEntity *)e->pInput)->sprite.base.scalePowerupX;
 *       e->sprite.base.scalePowerupY = ((PlayerEntity *)e->pInput)->sprite.base.scalePowerupX;
 *       EntityUpdateCallback((Entity *)e);
 *   }
 */
INCLUDE_ASM("asm/nonmatchings/playst", EntityTick_ShadowMirror);

extern Entity *InitEntitySprite(Entity *entity, u32 spriteId, s32 z, s16 x, s16 y, s32 flags);
extern u8 D_800117C4[];

/* Allocates+inits a HUD animated entity (sprite 0x1E1000B3 at z 0x3E7), installs
 * the D_800117C4 vtable, allocSize 0x3E9, and facing byte. Unless the anchor
 * value is 0x10000 (the "no anchor" sentinel), it stashes the anchor into the
 * four words at +0x50..+0x5C and wires ScaleX/YByEntityScale as the worldX/Y
 * move-transform FSM slots. Finally installs the frame callback 0x182D840C and
 * clears render flags. Returns the entity. */
Entity *InitHUDAnimatedEntity(Entity *e, s16 x, s16 y, s32 facing, s32 anchor) {
    TripadSlot u;
    s16 m1;
    void (*fn)();

    InitEntitySprite(e, 0x1E1000B3, 0x3E7, x, y, 0);
    e->collisionVtable = D_800117C4;
    e->allocSize = 0x3E9;
    *(u8 *)((u8 *)e + 0x74) = facing;
    if (anchor != 0x10000) {
        *(s32 *)((u8 *)e + 0x50) = anchor;
        *(s32 *)((u8 *)e + 0x54) = anchor;
        *(s32 *)((u8 *)e + 0x58) = anchor;
        *(s32 *)((u8 *)e + 0x5C) = anchor;
        do {} while (0);
        fn = (void (*)())ScaleXByEntityScale;
        m1 = -1;
        u.s.markerLo = 0;
        u.s.markerHi = m1;
        u.s.fn = fn;
        *(CallbackSlot *)&e->moveMarkerX = u.s;
        fn = (void (*)())ScaleYByEntityScale;
        u.s.markerLo = 0;
        u.s.markerHi = m1;
        u.s.fn = fn;
        *(CallbackSlot *)&e->moveMarkerY = u.s;
    }
    SetAnimationFrameCallback((PlayerEntity *)e, 0x182D840C);
    EntitySetRenderFlags(e, 0);
    return e;
}

/*
 * CreateHaloEntity(Entity *e, Entity *owner) -> Entity*
 *
 * Sub-entity constructor for the player "halo" indicator. Sets up the menu
 * entity base + destructor vtable, installs the per-frame tick callback
 * (FinnSubentityUpdatePositionFromParent) into the tick slot at e+0x00, heap-
 * allocates and initialises a child HUD-icon entity (stored at e+0x24, with a
 * flag byte set at child+0x1E7), then seeds this entity's screen position from
 * the stored owner (e+0x1C): worldX from owner+0x68, and 0x22 from
 * owner->worldY (owner+0x6A) + owner->renderOffsetY (owner+0x3A). The
 * renderOffsetY read comes via an 8-byte alignment-1 by-value copy of the
 * owner's render-bounds block (owner+0x38..0x3F) into a stack temp that gcc
 * keeps un-scalarised (lwl/lwr + swl/swr), reading back only the halfword at
 * +2.
 *
 * VERIFIED C (logic byte-exact; SHELVED on a stack-layout coin-flip):
 *
 *   extern void InitMenuEntityWithVtable(Entity *entity, s32 arg);
 *   extern void *InitHUDIconEntity(u8 *alloc, s32 arg);
 *   extern void FinnSubentityUpdatePositionFromParent();
 *   extern u8 D_800117A4[];
 *
 *   Entity *CreateHaloEntity(Entity *e, Entity *owner) {
 *       TripadSlot u;
 *       void (*fn)();
 *       Entity *p;
 *       InitMenuEntityWithVtable(e, 0x3E9);
 *       *(u8 **)((u8 *)e + 0x18) = D_800117A4;
 *       *(Entity **)((u8 *)e + 0x1C) = owner;
 *       fn = (void (*)())FinnSubentityUpdatePositionFromParent;
 *       u.s.markerLo = 0; u.s.markerHi = -1; u.s.fn = fn;
 *       *(CallbackSlot *)((u8 *)e + 0) = u.s;
 *       *(void **)((u8 *)e + 0x24) =
 *           InitHUDIconEntity(AllocateFromHeap((u8 *)g_pBlbHeapBase, 0x1E8, 1, 0), 0);
 *       *(u8 *)(*(u8 **)((u8 *)e + 0x24) + 0x1E7) = 1;
 *       *(u16 *)((u8 *)e + 0x20) = *(u16 *)((u8 *)*(Entity **)((u8 *)e + 0x1C) + 0x68);
 *       {
 *           struct HaloBytes8 { u8 b[8]; } tmp;
 *           tmp = *(struct HaloBytes8 *)((u8 *)*(Entity **)((u8 *)e + 0x1C) + 0x38);
 *           *(u8 *)((u8 *)e + 0x2C) = 0;
 *           p = *(Entity **)((u8 *)e + 0x1C);
 *           *(u16 *)((u8 *)e + 0x22) =
 *               *(s16 *)((u8 *)p + 0x6A) + *(u16 *)((u8 *)&tmp + 2);
 *       }
 *       return e;
 *   }
 *
 * Residual (non-source-reachable): the callback-slot staging temp and the
 * 8-byte render-bounds copy temp have disjoint lifetimes; the original packs
 * them into ONE overlapping 12-byte scratch region (slot@sp+0x14 over the low
 * copy@sp+0x10, frame 0x30). Every local layout expressible in C gives one of:
 * two adjacent slots (frame 0x38), a single fully-coalesced slot (frame 0x28),
 * or slot@0x14 with a fresh non-overlapping copy@0x20 (frame 0x38) -- never the
 * partial 0x14/0x10 overlay. There is also a paired lh-vs-lhu sign coin-flip on
 * the owner->worldY read (immediately truncated by the sh store, so gcc is free
 * to pick either). Both are compiler stack-packing/scheduling choices with no
 * C-source lever; matching would require the permuter.
 */
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





