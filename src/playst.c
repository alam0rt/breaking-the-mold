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
extern s32 PlayEntityPositionSound(Entity *e, u32 soundId);
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
    *(SetupSlot *)&e->eventMarker = u.s;
    u.s = slot104;
    *(SetupSlot *)((u8 *)e + 0x104) = u.s;
    u.s = render;
    *(SetupSlot *)&e->renderMarker = u.s;
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
 * MATCHED via rodata migration (pilot, 2026-07-04). The switch jump table
 * jtbl_80011AE4 is emitted by gcc into build/src/playst.o(.rodata) and spliced
 * into the linker script at 0x80011AE4 by tools/migrate_rodata.py (registered in
 * tools/rodata_migrations.json, run post-splat by the Makefile). See
 * docs/plans/rodata-sdata-migration.md and the rodata-migrate skill.
 * Two codegen levers were needed on top of the migration: the 0x100F warp reads
 * src->worldY through an s32 temp to force `lh` (signed) feeding +0x20/sh; and the
 * 0x1017 query is written `result = 0; if (disableScale == 0) result = (...)` so
 * the result=0 fills the bnez delay slot. */
extern void PlayerEnterShrinkZone(void);
extern void PlayerExitShrinkZone(void);
u32 PlayerZoneEnterMarker asm("D_800A5DB8");
EntityCallback PlayerZoneEnterFn asm("D_800A5DBC");
u32 PlayerZoneWarpMarker asm("D_800A5DC0");
EntityCallback PlayerZoneWarpFn asm("D_800A5DC4");
s32 PlayerEvent_ZoneTriggerHandler(PlayerEntity *e, u32 event, u32 arg2,
                                   Entity *src) {
    s32 result = 0;
    switch (event & 0xFFFF) {
    case 0x100C: PlayerEnterShrinkZone(); break;
    case 0x100D: PlayerExitShrinkZone(); break;
    case 0x1010:
        if (e->disableScale != 0) break;
        if (e->levelExitFlag != 0) break;
        EntitySetState((Entity *)e, PlayerZoneEnterMarker, PlayerZoneEnterFn);
        break;
    case 0x100F: {
        s32 warpY;
        e->sprite.base.worldX = src->worldX;
        warpY = src->worldY;
        e->sprite.base.worldY = warpY + 0x20;
        EntitySetState((Entity *)e, PlayerZoneWarpMarker, PlayerZoneWarpFn);
        break;
    }
    case 0x1017:
        result = 0;
        if (e->disableScale == 0) {
            result = (e->levelExitFlag == 0);
        }
        break;
    }
    return result;
}


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
        register void (*fn)() PSX_REG("$3") = (void (*)())PlayerEntityCollisionHandler;
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

extern void PlayerState_IdleRandom();
extern void PlayerCallback_IdleInputHandler();
extern void PlayerCallback_HorizontalWallCollision();
extern void PlayerCallback_RidingPlatformPhysics();

void PlayerStateInit_Idle(PlayerEntity *e) {
    struct { s32 lead; CallbackSlot tick, event, input, render; } g;
    CallbackSlot scratch;
    struct { s32 pad; CallbackSlot s; s32 tail[3]; } curP;
    void (*rfn)();
    void (*fn)();
    s16 m1;

    e->fidgetTimerShort = (rand() & 0x7F) + 0x30;
    e->fidgetTimerLong = (rand() & 0xFF) + 0x100;
    e->velocityY_fixed = 0;
    e->carryMotionX = 0;
    do {} while (0);

    FSM_STAGE_SLOT_FIRST(fn, g.tick,  m1, PlayerState_IdleRandom);
    FSM_STAGE_SLOT(fn, g.event, m1, PlayerEntityEventHandler);
    FSM_STAGE_SLOT(fn, g.input, m1, PlayerCallback_IdleInputHandler);

    FSM_STAGE_RENDER(rfn, scratch, g.render, m1, e->interactEntity != NULL, PlayerCallback_HorizontalWallCollision, PlayerCallback_RidingPlatformPhysics);

    FSM_COMMIT_SLOT(curP.s, g.tick,   e->sprite.base.tickMarker);
    FSM_COMMIT_SLOT(curP.s, g.event,  e->sprite.base.eventMarker);
    FSM_COMMIT_SLOT(curP.s, g.input,  e->inputStateMarker);
    FSM_COMMIT_SLOT(curP.s, g.render, e->sprite.base.renderMarker);
    SetEntitySpriteId(e, 0x48204012, 1);
}

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_SetIdleStateCallbacks);

/*
 * PlayerState_IdleLookAround (0x80066F60, 0x15C) — MATCHED 2026-07-04.
 * Frame-0x68 FSM installer, structural twin of PlayerState_WalkingRight but
 * with NO jumpParam store. Cracked with the same idiom: aggregate-struct slot
 * group + per-slot fn-into-KEEP_LIVE-temp-before-markers (tick fn computed
 * first). See [[playst-fsm-frame-coinflip]].
 */
extern void PlayerTickCallback();
extern void PlayerCallback_IdleInputHandler();
extern void PlayerCallback_HorizontalWallCollision();
extern void PlayerCallback_RidingPlatformPhysics();
extern void PlayerCallback_SetIdleStateCallbacks();

void PlayerState_IdleLookAround(PlayerEntity *e) {
    struct { s32 lead; CallbackSlot tick, event, input, render; } g;
    CallbackSlot scratch;
    struct { s32 pad; CallbackSlot s; s32 tail[2]; } curP;
    void (*rfn)();
    void (*fn)();
    register s16 m1 PSX_REG("$17");
    do {} while (0);
    FSM_STAGE_SLOT_FIRST(fn, g.tick, m1, PlayerTickCallback);
    FSM_STAGE_SLOT(fn, g.event, m1, PlayerCallback_EventHandlerWithQueue);
    FSM_STAGE_SLOT(fn, g.input, m1, PlayerCallback_IdleInputHandler);
    FSM_STAGE_RENDER(rfn, scratch, g.render, m1, e->interactEntity != NULL, PlayerCallback_HorizontalWallCollision, PlayerCallback_RidingPlatformPhysics);
    FSM_COMMIT_SLOT(curP.s, g.tick, e->sprite.base.tickMarker);
    FSM_COMMIT_SLOT(curP.s, g.event, e->sprite.base.eventMarker);
    FSM_COMMIT_SLOT(curP.s, g.input, e->inputStateMarker);
    FSM_COMMIT_SLOT(curP.s, g.render, e->sprite.base.renderMarker);
    SetEntitySpriteId(e, 0x5900C41E, 1);
    FSM_STAGE_COMMIT(g.tick, m1, PlayerCallback_SetIdleStateCallbacks, e->sprite.queuedStateMarker);
}

INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_PlayRandomIdleAnimation);

extern s32 GetTPage(s32 tp, s32 abr, s32 x, s32 y);

void PlayerState_UpdateTPageAndHideHalo(PlayerEntity *e) {
    RenderSpriteObj *ctx = (RenderSpriteObj *)e->sprite.base.spriteContext;
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

/* PlayerState_WalkingRight / PlayerState_WalkingLeft (0x160 each)
 *
 * Installs the walking FSM: clears jumpParam, then tick/event/input/render
 * callback slots (render routed to WalkingOnPlatform when standing on an
 * interact entity, else the normal movement/collision handler),
 * SetEntitySpriteId(e, <spriteId>, 1), then queues the next state.
 * Same slot-install shape as PlayerState_IdleLookAround.
 *
 * The two functions are a structural TWIN pair (identical modulo four
 * literals): swap PlayerCallback_EventHandlerWithQueue -> _WalkToRunTransition
 * (event fn), spriteId 0x292E8480 -> 0x18298210, and PlayerState_Falling ->
 * PlayerStateInit_Idle (queued-state fn). The verified C below is byte-exact
 * for both bodies with only those literals changed (see docs/analysis/
 * function-families.md — exemplar-sweep candidate).
 *
 * SHELVED at a ~6-instruction prologue-scheduling residual (was the whole
 * frame layout — now CRACKED). Everything matches: frame size 0x68, all six
 * stack-slot offsets, the full install body, the render conditional, the
 * sprite call, and the tail. Remaining diff is only prologue instruction order
 * (sw ra vs sw s1, li s1 position, fn-load vs marker-store within the first
 * slots) — a sched1 coin-flip; decomp-permuter floors at ~325 from base 1080.
 *
 * FRAME CRACK (the reusable idiom): separate CallbackSlot locals are each
 * BLKmode → assign_stack_local forces 8-align (gcc mips.h BIGGEST_ALIGNMENT
 * 64), so they pack tight at sp+0x10 and can NEVER hit the target's 4-aligned
 * origin (sp+0x14). The target's tick/event/input/render are FIELDS OF ONE
 * aggregate whose 8-aligned base + a leading s32 puts the first slot at 0x14
 * and pushes scratch to 0x38; a 4-pad staging struct lands the copy-temp at
 * 0x44 with the right trailing pad. This reproduces the exact 6-slot layout
 * that ~8 hand attempts and 11k permuter iterations on separate locals could
 * not. Should transfer to the whole playst frame-0x68 FSM family (see
 * docs/analysis/function-families.md, [[playst-fsm-frame-coinflip]]).
 *
 * The two functions are a structural TWIN pair (identical modulo four
 * literals): event.fn EventHandlerWithQueue->WalkToRunTransition, spriteId
 * 0x292E8480->0x18298210, tick.fn(fall) PlayerState_Falling->PlayerStateInit_Idle.
 * Verified near-match C (byte-exact but for the prologue-schedule residual;
 * WalkingLeft is the same body with those three literals swapped):
 *
 *   extern void PlayerTickCallback(), PlayerCallback_WalkInputHandler(),
 *               PlayerCallback_WalkingOnPlatform(),
 *               PlayerCallback_HandleMovementAndCollision(), PlayerState_Falling();
 *   // PlayerCallback_EventHandlerWithQueue, SetEntitySpriteId declared above.
 *
 *   void PlayerState_WalkingRight(PlayerEntity *e) {
 *       struct { s32 lead; CallbackSlot tick, event, input, render; } g;
 *       CallbackSlot scratch;
 *       struct { s32 pad; CallbackSlot s; s32 tail[2]; } curP;  // staging temp @0x44 + trailing pad
 *       void (*rfn)();
 *       register s16 m1 asm("$17");         // markerHi=-1, survives sprite call
 *       e->jumpParam = 0;
 *       m1 = -1;
 *       g.tick.markerLo = 0;  g.tick.markerHi = m1;  g.tick.fn = (void (*)())PlayerTickCallback;
 *       do {} while (0);                    // per-slot fences: keep marker+fn stores together
 *       g.event.markerLo = 0; g.event.markerHi = m1; g.event.fn = (void (*)())PlayerCallback_EventHandlerWithQueue;
 *       do {} while (0);
 *       g.input.markerLo = 0; g.input.markerHi = m1; g.input.fn = (void (*)())PlayerCallback_WalkInputHandler;
 *       do {} while (0);
 *       scratch.markerLo = 0; scratch.markerHi = m1;
 *       rfn = (void (*)())PlayerCallback_HandleMovementAndCollision;  // default; one store
 *       if (e->interactEntity != NULL) rfn = (void (*)())PlayerCallback_WalkingOnPlatform;
 *       scratch.fn = rfn;
 *       g.render = scratch;
 *       FSM_COMMIT_SLOT(curP.s, g.tick, e->sprite.base.tickMarker);
 *       FSM_COMMIT_SLOT(curP.s, g.event, e->sprite.base.eventMarker);
 *       FSM_COMMIT_SLOT(curP.s, g.input, e->inputStateMarker);
 *       FSM_COMMIT_SLOT(curP.s, g.render, e->sprite.base.renderMarker);
 *       SetEntitySpriteId(e, 0x292E8480, 1);
 *       g.tick.markerLo = 0; g.tick.markerHi = m1;
 *       g.tick.fn = (void (*)())PlayerState_Falling;
 *       *(CallbackSlot *)&e->sprite.queuedStateMarker = g.tick;  // fall slot: DIRECT copy
 *   }
 */
extern void PlayerTickCallback(), PlayerCallback_WalkInputHandler(),
            PlayerCallback_WalkingOnPlatform(),
            PlayerCallback_HandleMovementAndCollision(), PlayerState_Falling();

void PlayerState_WalkingRight(PlayerEntity *e) {
    struct { s32 lead; CallbackSlot tick, event, input, render; } g;
    CallbackSlot scratch;
    struct { s32 pad; CallbackSlot s; s32 tail[2]; } curP;
    void (*rfn)();
    void (*fn)();
    register s16 m1 PSX_REG("$17");
    e->jumpParam = 0;
    do {} while (0);
    FSM_STAGE_SLOT_FIRST(fn, g.tick, m1, PlayerTickCallback);
    FSM_STAGE_SLOT(fn, g.event, m1, PlayerCallback_EventHandlerWithQueue);
    FSM_STAGE_SLOT(fn, g.input, m1, PlayerCallback_WalkInputHandler);
    FSM_STAGE_RENDER(rfn, scratch, g.render, m1, e->interactEntity != NULL, PlayerCallback_HandleMovementAndCollision, PlayerCallback_WalkingOnPlatform);
    FSM_COMMIT_SLOT(curP.s, g.tick, e->sprite.base.tickMarker);
    FSM_COMMIT_SLOT(curP.s, g.event, e->sprite.base.eventMarker);
    FSM_COMMIT_SLOT(curP.s, g.input, e->inputStateMarker);
    FSM_COMMIT_SLOT(curP.s, g.render, e->sprite.base.renderMarker);
    SetEntitySpriteId(e, 0x292E8480, 1);
    FSM_STAGE_COMMIT(g.tick, m1, PlayerState_Falling, e->sprite.queuedStateMarker);
}

extern void PlayerStateInit_Idle();

void PlayerState_WalkingLeft(PlayerEntity *e) {
    struct { s32 lead; CallbackSlot tick, event, input, render; } g;
    CallbackSlot scratch;
    struct { s32 pad; CallbackSlot s; s32 tail[2]; } curP;
    void (*rfn)();
    void (*fn)();
    register s16 m1 PSX_REG("$17");
    e->jumpParam = 0;
    do {} while (0);
    FSM_STAGE_SLOT_FIRST(fn, g.tick, m1, PlayerTickCallback);
    FSM_STAGE_SLOT(fn, g.event, m1, PlayerCallback_WalkToRunTransition);
    FSM_STAGE_SLOT(fn, g.input, m1, PlayerCallback_WalkInputHandler);
    FSM_STAGE_RENDER(rfn, scratch, g.render, m1, e->interactEntity != NULL, PlayerCallback_HandleMovementAndCollision, PlayerCallback_WalkingOnPlatform);
    FSM_COMMIT_SLOT(curP.s, g.tick, e->sprite.base.tickMarker);
    FSM_COMMIT_SLOT(curP.s, g.event, e->sprite.base.eventMarker);
    FSM_COMMIT_SLOT(curP.s, g.input, e->inputStateMarker);
    FSM_COMMIT_SLOT(curP.s, g.render, e->sprite.base.renderMarker);
    SetEntitySpriteId(e, 0x18298210, 1);
    FSM_STAGE_COMMIT(g.tick, m1, PlayerStateInit_Idle, e->sprite.queuedStateMarker);
}

extern void SetAnimationFrameIndex(Entity *e, s32 frame);

/*
 * PlayerState_Running (0x8006762C, 0x16C) — MATCHED 2026-07-04.
 * Twin of PlayerState_WalkingRight (jumpParam variant) with event=
 * WalkToRunTransition and an extra SetAnimationFrameIndex(e,1) after the
 * sprite call. See [[playst-fsm-frame-coinflip]].
 */
void PlayerState_Running(PlayerEntity *e) {
    struct { s32 lead; CallbackSlot tick, event, input, render; } g;
    CallbackSlot scratch;
    struct { s32 pad; CallbackSlot s; s32 tail[2]; } curP;
    void (*rfn)();
    void (*fn)();
    register s16 m1 PSX_REG("$17");
    e->jumpParam = 0;
    do {} while (0);
    FSM_STAGE_SLOT_FIRST(fn, g.tick, m1, PlayerTickCallback);
    FSM_STAGE_SLOT(fn, g.event, m1, PlayerCallback_WalkToRunTransition);
    FSM_STAGE_SLOT(fn, g.input, m1, PlayerCallback_WalkInputHandler);
    FSM_STAGE_RENDER(rfn, scratch, g.render, m1, e->interactEntity != NULL, PlayerCallback_HandleMovementAndCollision, PlayerCallback_WalkingOnPlatform);
    FSM_COMMIT_SLOT(curP.s, g.tick, e->sprite.base.tickMarker);
    FSM_COMMIT_SLOT(curP.s, g.event, e->sprite.base.eventMarker);
    FSM_COMMIT_SLOT(curP.s, g.input, e->inputStateMarker);
    FSM_COMMIT_SLOT(curP.s, g.render, e->sprite.base.renderMarker);
    SetEntitySpriteId(e, 0x292E8480, 1);
    SetAnimationFrameIndex((Entity *)e, 1);
    FSM_STAGE_COMMIT(g.tick, m1, PlayerState_Falling, e->sprite.queuedStateMarker);
}

extern void PlayerCallback_TeleporterAnimHandler();
extern void SetAnimationSpriteFlags(PlayerEntity *e, u32 spriteId, s32 flags);
extern void EntitySetCallback(Entity *e, u32 marker, EntityCallback fn);
u32 PlayerSpecialMoveNextMarker asm("D_800A5F04");
EntityCallback PlayerSpecialMoveNextFn asm("D_800A5F08");

/* PlayerState_JumpApex @ 0x80067798 (0x13C, frame 0x28) -- MATCHED. Member of
 * the frame-0x28 special-move installer family (twin of SpecialMove1/2). Seeds
 * apexVelocity(0x136)=-0x28, velocityY_fixed(0x110)=0, jumpParam(0x156)=0,
 * specialMoveMode(0x135)=1, then installs five (markerLo,markerHi,fn) slots via
 * a single reused PadSlot scratch (sp+0x14): tick(0x0)->PlayerTickCallback,
 * event(0x8)->PlayerCallback_TeleporterAnimHandler, input(0x104)->
 * PlayerCallback_WalkInputHandler, render(0x1C) if/else on interactEntity
 * (WalkingOnPlatform / HandleMovementAndCollision), SetAnimationSpriteFlags(
 * e,0x88B9833C,1), queued(0x98)->PlayerState_Falling, then
 * EntitySetCallback(e, D_800A5F04, D_800A5F08).
 * KEY LEVER: `register s16 m1 asm("$6")` pins the held -1 to $a2 across the four
 * fixed slots (cracks the coin-flip that had shelved this). Tick fn via `rfn`
 * temp before m1=-1 (prologue const order); render fn via per-arm `rfn`/
 * `markerLo=0` (asymmetric delay-slot fill); queued fn via `qfn` pinned with
 * FSM_KEEP_LIVE (fn in $v1 before fresh `li v0,-1`). */
void PlayerState_JumpApex(PlayerEntity *e) {
    PadSlot u;
    void (*rfn)();
    void (*qfn)();
    register s16 m1 PSX_REG("$6");
    e->apexVelocity = -0x28;
    e->velocityY_fixed = 0;
    e->jumpParam = 0;
    e->specialMoveMode = 1;
    FSM_KEEP_LIVE(e);
    rfn = (void (*)())PlayerTickCallback;
    m1 = -1;
    u.s.markerLo = 0; u.s.markerHi = m1;
    u.s.fn = rfn;
    *(CallbackSlot *)&e->sprite.base.tickMarker = u.s;
    u.s.markerLo = 0; u.s.markerHi = m1;
    u.s.fn = (void (*)())PlayerCallback_TeleporterAnimHandler;
    *(CallbackSlot *)&e->sprite.base.eventMarker = u.s;
    u.s.markerLo = 0; u.s.markerHi = m1;
    u.s.fn = (void (*)())PlayerCallback_WalkInputHandler;
    *(CallbackSlot *)&e->inputStateMarker = u.s;
    if (e->interactEntity != NULL) {
        rfn = (void (*)())PlayerCallback_WalkingOnPlatform;
        u.s.markerLo = 0;
    } else {
        rfn = (void (*)())PlayerCallback_HandleMovementAndCollision;
        u.s.markerLo = 0;
    }
    u.s.markerHi = m1;
    u.s.fn = rfn;
    *(CallbackSlot *)&e->sprite.base.renderMarker = u.s;
    SetAnimationSpriteFlags(e, 0x88B9833C, 1);
    qfn = (void (*)())PlayerState_Falling;
    FSM_KEEP_LIVE(qfn);
    u.s.markerLo = 0; u.s.markerHi = -1;
    u.s.fn = qfn;
    *(CallbackSlot *)&e->sprite.queuedStateMarker = u.s;
    EntitySetCallback((Entity *)e, PlayerSpecialMoveNextMarker,
                      PlayerSpecialMoveNextFn);
}

extern void PlayerCallback_WalkInputWithJump();
extern void PlayerCallback_WalkingOnPlatform();
extern void PlayerCallback_HandleMovementAndCollision();
/*
 * PlayerState_Falling (0x800678D4, 0x160) — MATCHED 2026-07-06.
 * Lightweight falling-state installer (entity kept in a caller-saved reg, no
 * early calls). Prologue zeroes Y velocity and seeds the jump-hold window
 * (jumpParam = 0xC when the previous sprite was one of the two jump banks,
 * else 0). Then installs the four state callbacks with a render slot that
 * flips to WalkingOnPlatform while interacting with a platform entity.
 */
void PlayerState_Falling(PlayerEntity *e) {
    struct { s32 lead; CallbackSlot tick, event, input, render; } g;
    CallbackSlot scratch;
    struct { s32 pad; CallbackSlot s; s32 tail[3]; } curP;
    void (*rfn)();
    void (*fn)();
    s16 m1;
    e->velocityY_fixed = 0;
    if (e->sprite.currentSpriteId == 0x92B8480 || e->sprite.currentSpriteId == 0x88B9833C)
        e->jumpParam = 0xC;
    else
        e->jumpParam = 0;
    e->velocityY_fixed = 0;
    do {} while (0);
    FSM_STAGE_SLOT_FIRST(fn, g.tick, m1, PlayerState_FrameCountTick);
    FSM_STAGE_SLOT(fn, g.event, m1, PlayerEntityEventHandler);
    FSM_STAGE_SLOT(fn, g.input, m1, PlayerCallback_WalkInputWithJump);
    FSM_STAGE_RENDER(rfn, scratch, g.render, m1, e->interactEntity != NULL, PlayerCallback_HandleMovementAndCollision, PlayerCallback_WalkingOnPlatform);
    FSM_COMMIT_SLOT(curP.s, g.tick, e->sprite.base.tickMarker);
    FSM_COMMIT_SLOT(curP.s, g.event, e->sprite.base.eventMarker);
    FSM_COMMIT_SLOT(curP.s, g.input, e->inputStateMarker);
    FSM_COMMIT_SLOT(curP.s, g.render, e->sprite.base.renderMarker);
    SetEntitySpriteId(e, 0x0B2084D0, 1);
}

INCLUDE_ASM("asm/nonmatchings/playst", PlayerEnterLandingState);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerSetupIdleState);

/* PlayerState_SpecialMove2 @ 0x80067CF0 (0x138, frame 0x28) -- MATCHED. Sibling
 * of SpecialMove1; identical installer minus the velocityY store (this one only
 * zeroes carryMotionX). Single reused PadSlot scratch at sp+0x14. Sets
 * apexVelocity(0x136)=-0x28, specialMoveMode(0x135)=1, carryMotionX(0x10C)=0,
 * then five (markerLo,markerHi,fn) installs: tick(0x0)->PlayerTickCallback,
 * event(0x8)->PlayerCallback_TeleporterAnimHandler, input(0x104)->
 * PlayerCallback_IdleInputHandler, render(0x1C) if/else on interactEntity
 * (RidingPlatformPhysics / HorizontalWallCollision), SetAnimationSpriteFlags(
 * e,0x8D01C734,1), queued(0x98)->PlayerStateInit_Idle, then
 * EntitySetCallback(e, D_800A5F04, D_800A5F08).
 * KEY LEVER: `register s16 m1 asm("$6")` pins the held -1 to $a2 across the four
 * fixed slots (cracks the $a2-vs-$v1 coin-flip that shelved this for months).
 * Tick fn via `rfn` temp before m1=-1 (prologue const order); render fn via
 * per-arm `rfn`/`markerLo=0` (asymmetric delay-slot fill); queued fn via `qfn`
 * pinned with FSM_KEEP_LIVE (fn in $v1 before fresh `li v0,-1`). */
void PlayerState_SpecialMove2(PlayerEntity *e) {
    PadSlot u;
    void (*rfn)();
    void (*qfn)();
    register s16 m1 PSX_REG("$6");
    e->apexVelocity = -0x28;
    e->specialMoveMode = 1;
    e->carryMotionX = 0;
    FSM_KEEP_LIVE(e);
    rfn = (void (*)())PlayerTickCallback;
    m1 = -1;
    u.s.markerLo = 0; u.s.markerHi = m1;
    u.s.fn = rfn;
    *(CallbackSlot *)&e->sprite.base.tickMarker = u.s;
    u.s.markerLo = 0; u.s.markerHi = m1;
    u.s.fn = (void (*)())PlayerCallback_TeleporterAnimHandler;
    *(CallbackSlot *)&e->sprite.base.eventMarker = u.s;
    u.s.markerLo = 0; u.s.markerHi = m1;
    u.s.fn = (void (*)())PlayerCallback_IdleInputHandler;
    *(CallbackSlot *)&e->inputStateMarker = u.s;
    if (e->interactEntity != NULL) {
        rfn = (void (*)())PlayerCallback_RidingPlatformPhysics;
        u.s.markerLo = 0;
    } else {
        rfn = (void (*)())PlayerCallback_HorizontalWallCollision;
        u.s.markerLo = 0;
    }
    u.s.markerHi = m1;
    u.s.fn = rfn;
    *(CallbackSlot *)&e->sprite.base.renderMarker = u.s;
    SetAnimationSpriteFlags(e, 0x8D01C734, 1);
    qfn = (void (*)())PlayerStateInit_Idle;
    FSM_KEEP_LIVE(qfn);
    u.s.markerLo = 0; u.s.markerHi = -1;
    u.s.fn = qfn;
    *(CallbackSlot *)&e->sprite.queuedStateMarker = u.s;
    EntitySetCallback((Entity *)e, PlayerSpecialMoveNextMarker,
                      PlayerSpecialMoveNextFn);
}


extern void PlayerCallback_WalkInputWithFall();
u32 PlayerJumpNextMarker asm("D_800A5F0C");
EntityCallback PlayerJumpNextFn asm("D_800A5F10");

/* PlayerState_Jump @ 0x80067E28 (0x168, frame 0x60) -- MATCHED. Aggregate-struct
 * installer variant (same shape as PlayerState_WalkingRight): four slots staged
 * into a `g` struct then block-copied via a `curP` scratch. Prologue stops the
 * active SPU voice (soundHandle@0x174), starts the jump sound and stores the new
 * handle, sets jumpParam(0x156)=0xC. Slots: tick(0x0)->PlayerTickCallback,
 * event(0x8)->PlayerEntityEventHandler, input(0x104)->
 * PlayerCallback_WalkInputWithFall, render(0x1C) default->HandleMovementAndCollision
 * overridden to WalkingOnPlatform when interactEntity!=NULL. No queued store;
 * ends with SetEntitySpriteId(e,0x92B8480,1) then
 * EntitySetCallback(e, D_800A5F0C, D_800A5F10). Held -1 is a caller-saved TEMP
 * ($v0 pin) because it dies before the SetEntitySpriteId call (no queued store
 * to keep it live -- contrast WalkingRight which needs the $s1 pin). */
void PlayerState_Jump(PlayerEntity *e) {
    struct { s32 lead; CallbackSlot tick, event, input, render; } g;
    CallbackSlot scratch;
    struct { s32 pad; CallbackSlot s; s32 tail[2]; } curP;
    void (*rfn)();
    void (*fn)();
    register s16 m1 PSX_REG("$2");
    StopSPUVoice(e->soundHandle);
    e->soundHandle = PlayEntityPositionSound((Entity *)e, 0x248E52);
    e->jumpParam = 0xC;
    do {} while (0);
    FSM_STAGE_SLOT_FIRST(fn, g.tick, m1, PlayerTickCallback);
    FSM_STAGE_SLOT(fn, g.event, m1, PlayerEntityEventHandler);
    FSM_STAGE_SLOT(fn, g.input, m1, PlayerCallback_WalkInputWithFall);
    FSM_STAGE_RENDER(rfn, scratch, g.render, m1, e->interactEntity != NULL, PlayerCallback_HandleMovementAndCollision, PlayerCallback_WalkingOnPlatform);
    FSM_COMMIT_SLOT(curP.s, g.tick, e->sprite.base.tickMarker);
    FSM_COMMIT_SLOT(curP.s, g.event, e->sprite.base.eventMarker);
    FSM_COMMIT_SLOT(curP.s, g.input, e->inputStateMarker);
    FSM_COMMIT_SLOT(curP.s, g.render, e->sprite.base.renderMarker);
    SetEntitySpriteId(e, 0x92B8480, 1);
    EntitySetCallback((Entity *)e, PlayerJumpNextMarker, PlayerJumpNextFn);
}

void PlayerState_StopSoundAndClear(PlayerEntity *e) {
    StopSPUVoice(e->soundHandle);
    e->soundHandle = -1;
}

extern void PlayerCallback_JumpWallCollisionInput();
extern void SetAnimationSpriteCallback(PlayerEntity *e, u32 animId);
/*
 * PlayerStateInit_Walking (0x80067FC8, 0x168) — MATCHED.
 * Walking-state installer (frame 0x60, no queued store). Seeds jumpParam=0xC
 * and clears velocityY_fixed, installs the four state callbacks (tick=Player-
 * TickCallback, event=PlayerEntityEventHandler, input=JumpWallCollisionInput,
 * render=HandleMovementAndCollision / WalkingOnPlatform when riding an interact
 * entity), then sets sprite id 0x1E089010 and arms the walk animation via three
 * tail calls (SetAnimationSpriteCallback / SetAnimationLoopFrame /
 * SetAnimationFrameIndex).
 *
 * Register/scheduling notes (this variant has no queued store, so the shared
 * markerHi -1 dies before any call and lives in a *temp*, not the $s1 pin used
 * by the queued-store siblings). Matching required three levers together:
 *   - `register s16 m1 asm("$2")` pins -1 to $v0 so it time-shares the reg that
 *     held the 0xC jumpParam literal (both are short-lived temps in $v0).
 *   - `FSM_KEEP_LIVE(e)` after the velocityY store is a scheduling barrier that
 *     stops `li $v0,-1` from hoisting above the jumpParam store (which would
 *     evict 0xC from $v0).
 *   - the first `fn = PlayerTickCallback` is emitted *before* `m1 = -1`, and a
 *     per-slot `FSM_KEEP_LIVE(fn)` sits between the markerLo/Hi writes and the
 *     fn store, forcing the target's per-slot interleave
 *     (fn-load, markerLo, markerHi, fn-store) instead of clumping.
 */
void PlayerStateInit_Walking(PlayerEntity *e) {
    struct { s32 lead; CallbackSlot tick, event, input, render; } g;
    CallbackSlot scratch;
    struct { s32 pad; CallbackSlot s; s32 tail[3]; } curP;
    void (*rfn)();
    void (*fn)();
    register s16 m1 PSX_REG("$2");
    e->jumpParam = 0xC;
    e->velocityY_fixed = 0;
    FSM_KEEP_LIVE(e);
    FSM_STAGE_SLOT_KL_FIRST(fn, g.tick, m1, PlayerTickCallback);
    FSM_STAGE_SLOT_KL(fn, g.event, m1, PlayerEntityEventHandler);
    FSM_STAGE_SLOT_KL(fn, g.input, m1, PlayerCallback_JumpWallCollisionInput);
    FSM_STAGE_RENDER(rfn, scratch, g.render, m1, e->interactEntity != NULL, PlayerCallback_HandleMovementAndCollision, PlayerCallback_WalkingOnPlatform);
    FSM_COMMIT_SLOT(curP.s, g.tick, e->sprite.base.tickMarker);
    FSM_COMMIT_SLOT(curP.s, g.event, e->sprite.base.eventMarker);
    FSM_COMMIT_SLOT(curP.s, g.input, e->inputStateMarker);
    FSM_COMMIT_SLOT(curP.s, g.render, e->sprite.base.renderMarker);
    SetEntitySpriteId(e, 0x1E089010, 1);
    SetAnimationSpriteCallback(e, 0x2421405);
    SetAnimationLoopFrame(e, 0x70D006D8);
    SetAnimationFrameIndex((Entity *)e, 0);
}

/*
 * PlayerStateInit_ResumeWalking (0x80068130, 0x174) — MATCHED 2026-07-04.
 * WalkingRight-family installer: jumpParam=0xC + velocityY_fixed=0 prologue,
 * event=EventHandlerWithQueue, input=IdleInputHandler, render Horizontal/Riding,
 * spriteId 0x1E089010, SetAnimationFrameIndex(e,0xE), queued=Init_Idle.
 */
void PlayerStateInit_ResumeWalking(PlayerEntity *e) {
    struct { s32 lead; CallbackSlot tick, event, input, render; } g;
    CallbackSlot scratch;
    struct { s32 pad; CallbackSlot s; s32 tail[2]; } curP;
    void (*rfn)();
    void (*fn)();
    register s16 m1 PSX_REG("$17");
    e->jumpParam = 0xC;
    e->velocityY_fixed = 0;
    do {} while (0);
    FSM_STAGE_SLOT_FIRST(fn, g.tick, m1, PlayerTickCallback);
    FSM_STAGE_SLOT(fn, g.event, m1, PlayerCallback_EventHandlerWithQueue);
    FSM_STAGE_SLOT(fn, g.input, m1, PlayerCallback_IdleInputHandler);
    FSM_STAGE_RENDER(rfn, scratch, g.render, m1, e->interactEntity != NULL, PlayerCallback_HorizontalWallCollision, PlayerCallback_RidingPlatformPhysics);
    FSM_COMMIT_SLOT(curP.s, g.tick, e->sprite.base.tickMarker);
    FSM_COMMIT_SLOT(curP.s, g.event, e->sprite.base.eventMarker);
    FSM_COMMIT_SLOT(curP.s, g.input, e->inputStateMarker);
    FSM_COMMIT_SLOT(curP.s, g.render, e->sprite.base.renderMarker);
    SetEntitySpriteId(e, 0x1E089010, 1);
    SetAnimationFrameIndex((Entity *)e, 0xE);
    FSM_STAGE_COMMIT(g.tick, m1, PlayerStateInit_Idle, e->sprite.queuedStateMarker);
}

/*
 * PlayerStateInit_CrouchSlide (0x800682A4, 0x130) — MATCHED 2026-07-05.
 * Heavy installer variant: s0=entity, s1=m1(-1). Input slot zeroed inline;
 * render is a direct unconditional callback (no interactEntity branch, no
 * scratch); queued store installs PlayerStateInit_IdleStanding. Frame 0x60 via
 * curP tail[3].
 */
extern void PlayerCallback_HandleMovementAndCollision();
extern void PlayerStateInit_IdleStanding();
void PlayerStateInit_CrouchSlide(PlayerEntity *e) {
    struct { s32 lead; CallbackSlot tick, event, input, render; } g;
    struct { s32 pad; CallbackSlot s; s32 tail[3]; } curP;
    void (*fn)();
    register s16 m1 PSX_REG("$17");
    do {} while (0);
    FSM_STAGE_SLOT_FIRST(fn, g.tick, m1, PlayerTickCallback);
    fn = (void (*)())PlayerCallback_EventHandlerWithQueue; FSM_KEEP_LIVE(fn);
    g.event.markerLo = 0; g.event.markerHi = m1; g.event.fn = fn;
    g.input.markerLo = 0; g.input.markerHi = 0; g.input.fn = (void (*)())0;
    do {} while (0);
    FSM_STAGE_SLOT(fn, g.render, m1, PlayerCallback_HandleMovementAndCollision);
    FSM_COMMIT_SLOT(curP.s, g.tick, e->sprite.base.tickMarker);
    FSM_COMMIT_SLOT(curP.s, g.event, e->sprite.base.eventMarker);
    FSM_COMMIT_SLOT(curP.s, g.input, e->inputStateMarker);
    FSM_COMMIT_SLOT(curP.s, g.render, e->sprite.base.renderMarker);
    SetEntitySpriteId(e, 0x4A298690, 1);
    FSM_STAGE_COMMIT(g.tick, m1, PlayerStateInit_IdleStanding, e->sprite.queuedStateMarker);
}

/*
 * PlayerStateInit_IdleWithZoneTrigger (0x800683D4, 0x140) — MATCHED 2026-07-05.
 * Heavy Variant A: prologue byte store e->_pad17B[1]=3 (anchors reg saves),
 * s0=entity, s1=m1(-1), four real slots, queued=PlayerStateInit_IdleStanding.
 * Frame 0x58 via curP tail[1].
 */
extern void PlayerEvent_ZoneTriggerHandlerAlt();
void PlayerStateInit_IdleWithZoneTrigger(PlayerEntity *e) {
    struct { s32 lead; CallbackSlot tick, event, input, render; } g;
    struct { s32 pad; CallbackSlot s; s32 tail[1]; } curP;
    void (*fn)();
    register s16 m1 PSX_REG("$17");
    e->_pad17B[1] = 3;
    do {} while (0);
    FSM_STAGE_SLOT_FIRST(fn, g.tick, m1, PlayerTickCallback);
    FSM_STAGE_SLOT(fn, g.event, m1, PlayerEvent_ZoneTriggerHandlerAlt);
    FSM_STAGE_SLOT(fn, g.input, m1, PlayerCallback_IdleInputHandler);
    FSM_STAGE_SLOT(fn, g.render, m1, PlayerCallback_HandleMovementAndCollision);
    FSM_COMMIT_SLOT(curP.s, g.tick, e->sprite.base.tickMarker);
    FSM_COMMIT_SLOT(curP.s, g.event, e->sprite.base.eventMarker);
    FSM_COMMIT_SLOT(curP.s, g.input, e->inputStateMarker);
    FSM_COMMIT_SLOT(curP.s, g.render, e->sprite.base.renderMarker);
    SetEntitySpriteId(e, 0x5A2082F2, 1);
    FSM_STAGE_COMMIT(g.tick, m1, PlayerStateInit_IdleStanding, e->sprite.queuedStateMarker);
}

/*
 * PlayerStateInit_IdleStanding (0x80068514, 0x140) — MATCHED 2026-07-05.
 * Heavy Variant B but with SWAPPED saved regs: entity in $s1, m1(-1) in $s0
 * (pin m1 to $16). Four real slots, queued=PlayerStateInit_Idle. Frame 0x58
 * via curP tail[1]; leading fence anchors the reg saves.
 */
void PlayerStateInit_IdleStanding(PlayerEntity *e) {
    struct { s32 lead; CallbackSlot tick, event, input, render; } g;
    struct { s32 pad; CallbackSlot s; s32 tail[1]; } curP;
    void (*fn)();
    register s16 m1 PSX_REG("$16");
    do {} while (0);
    FSM_STAGE_SLOT_FIRST(fn, g.tick, m1, PlayerTickCallback);
    FSM_STAGE_SLOT(fn, g.event, m1, PlayerCallback_EventHandlerWithQueue);
    FSM_STAGE_SLOT(fn, g.input, m1, PlayerCallback_IdleInputHandler);
    FSM_STAGE_SLOT(fn, g.render, m1, PlayerCallback_HandleMovementAndCollision);
    FSM_COMMIT_SLOT(curP.s, g.tick, e->sprite.base.tickMarker);
    FSM_COMMIT_SLOT(curP.s, g.event, e->sprite.base.eventMarker);
    FSM_COMMIT_SLOT(curP.s, g.input, e->inputStateMarker);
    FSM_COMMIT_SLOT(curP.s, g.render, e->sprite.base.renderMarker);
    SetEntitySpriteId(e, 0x0A08C490, 1);
    FSM_STAGE_COMMIT(g.tick, m1, PlayerStateInit_Idle, e->sprite.queuedStateMarker);
}

INCLUDE_ASM("asm/nonmatchings/playst", PlayerStateCallback_2);

/*
 * PlayerState_StandingIdle (0x8006888C, 0x180) — installer (frame 0x68, s1-pin,
 * queued store). Clears velocityY_fixed and carryMotionX, picks the idle sprite
 * id (default 0x3838801A, override 0x1C395196 when e->currentSpriteId==0x388110),
 * installs tick=PlayerTickCallback, event=PlayerCallback_EventHandlerWithQueue,
 * input=PlayerCallback_IdleInputHandler, render chosen by e->interactEntity
 * (HorizontalWallCollision / RidingPlatformPhysics), then SetEntitySpriteId and
 * queues PlayerStateInit_Idle into the queued-state slot (0x98). The sprite-id
 * selection MUST be a ternary (not default+if-override): the ternary evaluates
 * the condition first, so cc1 materialises the compare constant (0x388110)
 * before the two result constants, matching the target's prologue emission order.
 */
void PlayerState_StandingIdle(PlayerEntity *e) {
    struct { s32 lead; CallbackSlot tick, event, input, render; } g;
    CallbackSlot scratch;
    struct { s32 pad; CallbackSlot s; s32 tail[3]; } curP;
    void (*rfn)();
    void (*fn)();
    u32 spriteId;
    register s16 m1 PSX_REG("$17");
    e->velocityY_fixed = 0;
    e->carryMotionX = 0;
    spriteId = (e->sprite.currentSpriteId == 0x388110) ? 0x1C395196 : 0x3838801A;
    do {} while (0);
    FSM_STAGE_SLOT_FIRST(fn, g.tick, m1, PlayerTickCallback);
    FSM_STAGE_SLOT(fn, g.event, m1, PlayerCallback_EventHandlerWithQueue);
    FSM_STAGE_SLOT(fn, g.input, m1, PlayerCallback_IdleInputHandler);
    FSM_STAGE_RENDER(rfn, scratch, g.render, m1, e->interactEntity != NULL, PlayerCallback_HorizontalWallCollision, PlayerCallback_RidingPlatformPhysics);
    FSM_COMMIT_SLOT(curP.s, g.tick, e->sprite.base.tickMarker);
    FSM_COMMIT_SLOT(curP.s, g.event, e->sprite.base.eventMarker);
    FSM_COMMIT_SLOT(curP.s, g.input, e->inputStateMarker);
    FSM_COMMIT_SLOT(curP.s, g.render, e->sprite.base.renderMarker);
    SetEntitySpriteId(e, spriteId, 1);
    FSM_STAGE_COMMIT(g.tick, m1, PlayerStateInit_Idle, e->sprite.queuedStateMarker);
}


/* PlayerState_SpecialMove1 @ 0x80068A0C (0x13C, frame 0x28) -- MATCHED. Twin of
 * the (previously shelved) SpecialMove2. Single reused PadSlot scratch at sp+0x14.
 * Prologue: apexVelocity(0x136)=-0x28, specialMoveMode(0x135)=1,
 * carryMotionX(0x10C)=0, velocityY(0x110)=0. Four slots with the constant
 * markerHi (-1) HELD in $a2 across the region (target reserves $v0/$v1 for the
 * block-copy temps): tick(0x0)->PlayerTickCallback, event(0x8)->
 * PlayerCallback_TeleporterAnimHandler, input(0x104)->PlayerCallback_IdleInputHandler,
 * render(0x1C) if/else on interactEntity (RidingPlatformPhysics /
 * HorizontalWallCollision). SetAnimationSpriteFlags(e,0x8D01C734,1), then a
 * FRESH -1 for queued(0x98)->PlayerStateInit_Idle, then
 * EntitySetCallback(e, D_800A5F04, D_800A5F08).
 * KEY LEVER: `register s16 m1 asm("$6")` pins -1 to $a2 -- this cracks the
 * $a2-vs-$v1 held-constant coin-flip that had shelved the SpecialMove2 twin.
 * The tick fn goes through an `rfn` temp (loaded before m1=-1) so the tick
 * lui/addiu schedules ahead of the `li a2,-1`; the render fn also flows through
 * `rfn` (per-arm value + explicit per-arm markerLo=0) to reproduce the
 * asymmetric branch delay-slot fill; the queued fn goes through a `qfn` temp
 * pinned live with FSM_KEEP_LIVE so it materialises (in $v1) before the fresh
 * `li v0,-1`. */
void PlayerState_SpecialMove1(PlayerEntity *e) {
    PadSlot u;
    void (*rfn)();
    void (*qfn)();
    register s16 m1 PSX_REG("$6");
    e->apexVelocity = -0x28;
    e->specialMoveMode = 1;
    e->carryMotionX = 0;
    e->velocityY_fixed = 0;
    FSM_KEEP_LIVE(e);
    rfn = (void (*)())PlayerTickCallback;
    m1 = -1;
    u.s.markerLo = 0; u.s.markerHi = m1;
    u.s.fn = rfn;
    *(CallbackSlot *)&e->sprite.base.tickMarker = u.s;
    u.s.markerLo = 0; u.s.markerHi = m1;
    u.s.fn = (void (*)())PlayerCallback_TeleporterAnimHandler;
    *(CallbackSlot *)&e->sprite.base.eventMarker = u.s;
    u.s.markerLo = 0; u.s.markerHi = m1;
    u.s.fn = (void (*)())PlayerCallback_IdleInputHandler;
    *(CallbackSlot *)&e->inputStateMarker = u.s;
    if (e->interactEntity != NULL) {
        rfn = (void (*)())PlayerCallback_RidingPlatformPhysics;
        u.s.markerLo = 0;
    } else {
        rfn = (void (*)())PlayerCallback_HorizontalWallCollision;
        u.s.markerLo = 0;
    }
    u.s.markerHi = m1;
    u.s.fn = rfn;
    *(CallbackSlot *)&e->sprite.base.renderMarker = u.s;
    SetAnimationSpriteFlags(e, 0x8D01C734, 1);
    qfn = (void (*)())PlayerStateInit_Idle;
    FSM_KEEP_LIVE(qfn);
    u.s.markerLo = 0; u.s.markerHi = -1;
    u.s.fn = qfn;
    *(CallbackSlot *)&e->sprite.queuedStateMarker = u.s;
    EntitySetCallback((Entity *)e, PlayerSpecialMoveNextMarker,
                      PlayerSpecialMoveNextFn);
}


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

/* SHELVED: PlayerStateInit_BounceJumpAnimation / PlayerStateInit_BounceActive
 * (byte-identical twins, off 59CAC / 59E0C, size 0x154, frame 0x50).
 * These use the FULL constant-hoisting codegen: all 4 callback fns are
 * materialized up front into $a3/$t0/$t1/$t2 and the sprite id into $a1 before
 * the g_pGameState->bounce_active_flag store. R&D findings:
 *   - Register-pinning the 4 fns (register void(*)() asm("$7".."$10") +
 *     FSM_KEEP_LIVE) reproduces the eager fn loads exactly.
 *   - TWO remaining blockers prevented a match:
 *     1. `sw ra` scheduling slot: cc1 flushes ra right after the keep-live
 *        barriers, but the target delays it past sprite+gamestate+li-1.
 *     2. D_800A5F14/D_800A5F18 (PlayerHiddenIdleMarker/Fn) sdata placement:
 *        referencing them as C globals shifts them off by 4 (target gp+1472/
 *        1476, C build gp+1476/1480).
 * Body shape (for a future attempt): 4 pinned fns (CooldownTick, CollisionHandler,
 * JumpInputAndCounters, FallingPhysicsMain), g_pGameState->bounce_active_flag=1,
 * 4 slots via curP.tail[1], SetEntitySpriteId(e,0x1C3AA013,1),
 * EntitySetRenderFlags(e,1), SetAnimationLoopFrame(e,0x1084280),
 * SetAnimationSpriteCallback(e,0x2421405),
 * EntitySetCallback(e, PlayerHiddenIdleMarker, PlayerHiddenIdleFn). */
INCLUDE_ASM("asm/nonmatchings/playst", PlayerStateInit_BounceJumpAnimation);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerStateInit_BounceActive);

/* SHELVED: PlayerState_SpecialIdleAnim @ 0x80069754 (0x164, frame 0x58).
 * Up-front-fn-load aggregate installer — SAME family/wall as the BounceActive
 * twins above. Prologue: g_pGameState->bounce_active_flag=1, apexVelocity(0x136)
 * =-0x27, specialMoveMode(0x135)=1, specialMoveQueued(0x134)=0. Four fixed slots
 * (tick->CooldownTick, event->BounceEventHandler, input->JumpInputAndCounters,
 * render->FallingPhysicsMain) pre-loaded into $a3/$t0/$t1/$t2, sprite id
 * 0xC8099196 into $a1, all BEFORE the flag store. SetEntitySpriteId(e,0xC8099196,
 * 1); queued store (0x98)->PlayerStateInit_BounceActive; EntitySetCallback(e,
 * D_800A5F1C, D_800A5F20).
 *
 * NEAR-MATCH (this session): the winning shape is 4 fn locals pinned to
 * $7/$8/$9/$10 (register void(*)()) + FSM_KEEP_LIVE on all four + `register s16
 * m1 asm("$17")` + curP.tail[1] (frame 0x58). That reproduces the ENTIRE body
 * and the eager fn loads exactly (a3/t0/t1/t2), leaving ONLY a 2-instruction
 * residual: the target delays `sw s1,0x4C`/`sw ra,0x50` past the a1+gamestate
 * loads (saving s1 just before `li s1,-1`), but the FSM_KEEP_LIVE fence that is
 * REQUIRED to force the eager fn loads also flushes the callee-saved reg saves
 * to the top of the prologue. Dropping the barriers fixes the save order but
 * loses the eager fn loads (a1 hoists first / per-slot v0 reloads). No C-level
 * lever separates "eager fn load" from "early reg-save flush" — this is exactly
 * the twins' blocker #1. Shelve until a prologue-scheduling permuter push cracks
 * it (fix should transfer to the whole up-front-fn sub-family). */
INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_SpecialIdleAnim);

/* PlayerStateInit_TeleportIdleOnPlatform @ 0x800698B8 (0x184, frame 0x68) --
 * heavy render-conditional installer (pinned m1 $17 -> s1 saved -> frame 0x68).
 * Variant A 4-store prologue, queued=PlayerStateInit_Idle, trailing deferred
 * install EntitySetCallback(e, D_800A5F04, D_800A5F08). curP.tail[3]. */
void PlayerStateInit_TeleportIdleOnPlatform(PlayerEntity *e) {
    struct { s32 lead; CallbackSlot tick, event, input, render; } g;
    CallbackSlot scratch;
    struct { s32 pad; CallbackSlot s; s32 tail[3]; } curP;
    void (*rfn)();
    void (*fn)();
    register s16 m1 PSX_REG("$17");
    e->apexVelocity = -0x28;
    e->specialMoveMode = 1;
    e->specialMoveQueued = 0;
    e->carryMotionX = 0;
    do {} while (0);
    FSM_STAGE_SLOT_FIRST(fn, g.tick, m1, PlayerTickCallback);
    FSM_STAGE_SLOT(fn, g.event, m1, PlayerCallback_TeleporterAnimHandler);
    FSM_STAGE_SLOT(fn, g.input, m1, PlayerCallback_IdleInputHandler);
    FSM_STAGE_RENDER(rfn, scratch, g.render, m1, e->interactEntity != NULL, PlayerCallback_HorizontalWallCollision, PlayerCallback_RidingPlatformPhysics);
    FSM_COMMIT_SLOT(curP.s, g.tick, e->sprite.base.tickMarker);
    FSM_COMMIT_SLOT(curP.s, g.event, e->sprite.base.eventMarker);
    FSM_COMMIT_SLOT(curP.s, g.input, e->inputStateMarker);
    FSM_COMMIT_SLOT(curP.s, g.render, e->sprite.base.renderMarker);
    SetEntitySpriteId(e, 0x8D01C734, 1);
    FSM_STAGE_COMMIT(g.tick, m1, PlayerStateInit_Idle, e->sprite.queuedStateMarker);
    EntitySetCallback((Entity *)e, PlayerSpecialMoveNextMarker, PlayerSpecialMoveNextFn);
}

/* PlayerStateInit_TeleportWalking @ 0x80069A3C (0x180, frame 0x68) -- sibling of
 * TeleportIdleOnPlatform: same heavy render-cond + deferred-install template,
 * but a 3-store prologue (no carryMotionX), input=WalkInputHandler, render
 * HandleMovementAndCollision/WalkingOnPlatform, sprite 0x88B9833C,
 * queued=PlayerState_Falling. Same D_800A5F04/08 deferred pair. */
extern void PlayerCallback_WalkInputHandler();
extern void PlayerCallback_WalkingOnPlatform();
extern void PlayerState_Falling();
void PlayerStateInit_TeleportWalking(PlayerEntity *e) {
    struct { s32 lead; CallbackSlot tick, event, input, render; } g;
    CallbackSlot scratch;
    struct { s32 pad; CallbackSlot s; s32 tail[3]; } curP;
    void (*rfn)();
    void (*fn)();
    register s16 m1 PSX_REG("$17");
    e->apexVelocity = -0x28;
    e->specialMoveMode = 1;
    e->specialMoveQueued = 0;
    do {} while (0);
    FSM_STAGE_SLOT_FIRST(fn, g.tick, m1, PlayerTickCallback);
    FSM_STAGE_SLOT(fn, g.event, m1, PlayerCallback_TeleporterAnimHandler);
    FSM_STAGE_SLOT(fn, g.input, m1, PlayerCallback_WalkInputHandler);
    FSM_STAGE_RENDER(rfn, scratch, g.render, m1, e->interactEntity != NULL, PlayerCallback_HandleMovementAndCollision, PlayerCallback_WalkingOnPlatform);
    FSM_COMMIT_SLOT(curP.s, g.tick, e->sprite.base.tickMarker);
    FSM_COMMIT_SLOT(curP.s, g.event, e->sprite.base.eventMarker);
    FSM_COMMIT_SLOT(curP.s, g.input, e->inputStateMarker);
    FSM_COMMIT_SLOT(curP.s, g.render, e->sprite.base.renderMarker);
    SetEntitySpriteId(e, 0x88B9833C, 1);
    FSM_STAGE_COMMIT(g.tick, m1, PlayerState_Falling, e->sprite.queuedStateMarker);
    EntitySetCallback((Entity *)e, PlayerSpecialMoveNextMarker, PlayerSpecialMoveNextFn);
}

/* PlayerStateInit_TeleportFalling @ 0x80069BBC (0x180, frame 0x68) -- sibling of
 * the teleport pair: 3-store prologue with apexVelocity=-0x17, input=
 * FallInputHandler, render HorizontalWallCollision/RidingPlatformPhysics,
 * sprite 0xD02DC3D5, queued=PlayerStateInit_FallingWithInput, same deferred
 * D_800A5F04/08 pair. */
extern void PlayerCallback_FallInputHandler();
extern void PlayerStateInit_FallingWithInput();
void PlayerStateInit_TeleportFalling(PlayerEntity *e) {
    struct { s32 lead; CallbackSlot tick, event, input, render; } g;
    CallbackSlot scratch;
    struct { s32 pad; CallbackSlot s; s32 tail[3]; } curP;
    void (*rfn)();
    void (*fn)();
    register s16 m1 PSX_REG("$17");
    e->apexVelocity = -0x17;
    e->specialMoveMode = 1;
    e->specialMoveQueued = 0;
    do {} while (0);
    FSM_STAGE_SLOT_FIRST(fn, g.tick, m1, PlayerTickCallback);
    FSM_STAGE_SLOT(fn, g.event, m1, PlayerCallback_TeleporterAnimHandler);
    FSM_STAGE_SLOT(fn, g.input, m1, PlayerCallback_FallInputHandler);
    FSM_STAGE_RENDER(rfn, scratch, g.render, m1, e->interactEntity != NULL, PlayerCallback_HorizontalWallCollision, PlayerCallback_RidingPlatformPhysics);
    FSM_COMMIT_SLOT(curP.s, g.tick, e->sprite.base.tickMarker);
    FSM_COMMIT_SLOT(curP.s, g.event, e->sprite.base.eventMarker);
    FSM_COMMIT_SLOT(curP.s, g.input, e->inputStateMarker);
    FSM_COMMIT_SLOT(curP.s, g.render, e->sprite.base.renderMarker);
    SetEntitySpriteId(e, 0xD02DC3D5, 1);
    FSM_STAGE_COMMIT(g.tick, m1, PlayerStateInit_FallingWithInput, e->sprite.queuedStateMarker);
    EntitySetCallback((Entity *)e, PlayerSpecialMoveNextMarker, PlayerSpecialMoveNextFn);
}

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

extern void PlayerCallback_FallInputHandler();
extern void PlayerStateInit_FallingWithInput(PlayerEntity *e);

/*
 * PlayerStateInit_FallFromLedge (0x8006A584, 0x160) — MATCHED 2026-07-04.
 * Variant A installer: prologue carryMotionX=0, event=EventHandlerWithQueue,
 * input=FallInputHandler, render Horizontal/Riding, spriteId 0x0A3A4051,
 * queued=PlayerStateInit_FallingWithInput.
 */
void PlayerStateInit_FallFromLedge(PlayerEntity *e) {
    struct { s32 lead; CallbackSlot tick, event, input, render; } g;
    CallbackSlot scratch;
    struct { s32 pad; CallbackSlot s; s32 tail[2]; } curP;
    void (*rfn)();
    void (*fn)();
    register s16 m1 PSX_REG("$17");
    e->carryMotionX = 0;
    do {} while (0);
    FSM_STAGE_SLOT_FIRST(fn, g.tick, m1, PlayerTickCallback);
    FSM_STAGE_SLOT(fn, g.event, m1, PlayerCallback_EventHandlerWithQueue);
    FSM_STAGE_SLOT(fn, g.input, m1, PlayerCallback_FallInputHandler);
    FSM_STAGE_RENDER(rfn, scratch, g.render, m1, e->interactEntity != NULL, PlayerCallback_HorizontalWallCollision, PlayerCallback_RidingPlatformPhysics);
    FSM_COMMIT_SLOT(curP.s, g.tick, e->sprite.base.tickMarker);
    FSM_COMMIT_SLOT(curP.s, g.event, e->sprite.base.eventMarker);
    FSM_COMMIT_SLOT(curP.s, g.input, e->inputStateMarker);
    FSM_COMMIT_SLOT(curP.s, g.render, e->sprite.base.renderMarker);
    SetEntitySpriteId(e, 0x0A3A4051, 1);
    FSM_STAGE_COMMIT(g.tick, m1, PlayerStateInit_FallingWithInput, e->sprite.queuedStateMarker);
}

INCLUDE_ASM("asm/nonmatchings/playst", PlayerStateInit_TeleportFallVariant);

/* PlayerStateInit_FallingWithInput @ 0x8006A818 -- installs the falling-with-
 * input FSM (lightweight: no early calls, entity stays in a caller-saved reg;
 * plain s16 m1 in v0). Four CallbackSlots to tick(0x0)/event(0x8)/input(0x104)/
 * render(0x1C), render fn conditional on e->interactEntity, then
 * SetEntitySpriteId(e, 0xE0AC230, 1). No queued store. Aggregate frame trick:
 * g (lead+4 slots) + curP.tail[3] -> frame 0x60. Variant B -> leading fence. */
extern void PlayerCallback_FallInputHandler();
void PlayerStateInit_FallingWithInput(PlayerEntity *e) {
    struct { s32 lead; CallbackSlot tick, event, input, render; } g;
    CallbackSlot scratch;
    struct { s32 pad; CallbackSlot s; s32 tail[3]; } curP;
    void (*rfn)();
    void (*fn)();
    s16 m1;
    do {} while (0);
    FSM_STAGE_SLOT_FIRST(fn, g.tick, m1, PlayerTickCallback);
    FSM_STAGE_SLOT(fn, g.event, m1, PlayerEntityEventHandler);
    FSM_STAGE_SLOT(fn, g.input, m1, PlayerCallback_FallInputHandler);
    FSM_STAGE_RENDER(rfn, scratch, g.render, m1, e->interactEntity != NULL, PlayerCallback_HorizontalWallCollision, PlayerCallback_RidingPlatformPhysics);
    FSM_COMMIT_SLOT(curP.s, g.tick, e->sprite.base.tickMarker);
    FSM_COMMIT_SLOT(curP.s, g.event, e->sprite.base.eventMarker);
    FSM_COMMIT_SLOT(curP.s, g.input, e->inputStateMarker);
    FSM_COMMIT_SLOT(curP.s, g.render, e->sprite.base.renderMarker);
    SetEntitySpriteId(e, 0xE0AC230, 1);
}

/*
 * PlayerStateInit_Falling (0x8006A940, 0x15C) — MATCHED 2026-07-04.
 * Structural twin of PlayerState_IdleLookAround (no jumpParam, same render
 * pair). See [[playst-fsm-frame-coinflip]].
 */
extern void PlayerCallback_FallInputHandler();
extern void PlayerStateInit_FallingWithInput();

void PlayerStateInit_Falling(PlayerEntity *e) {
    struct { s32 lead; CallbackSlot tick, event, input, render; } g;
    CallbackSlot scratch;
    struct { s32 pad; CallbackSlot s; s32 tail[2]; } curP;
    void (*rfn)();
    void (*fn)();
    register s16 m1 PSX_REG("$17");
    do {} while (0);
    FSM_STAGE_SLOT_FIRST(fn, g.tick, m1, PlayerTickCallback);
    FSM_STAGE_SLOT(fn, g.event, m1, PlayerCallback_EventHandlerWithQueue);
    FSM_STAGE_SLOT(fn, g.input, m1, PlayerCallback_FallInputHandler);
    FSM_STAGE_RENDER(rfn, scratch, g.render, m1, e->interactEntity != NULL, PlayerCallback_HorizontalWallCollision, PlayerCallback_RidingPlatformPhysics);
    FSM_COMMIT_SLOT(curP.s, g.tick, e->sprite.base.tickMarker);
    FSM_COMMIT_SLOT(curP.s, g.event, e->sprite.base.eventMarker);
    FSM_COMMIT_SLOT(curP.s, g.input, e->inputStateMarker);
    FSM_COMMIT_SLOT(curP.s, g.render, e->sprite.base.renderMarker);
    SetEntitySpriteId(e, 0x0AABC010, 1);
    FSM_STAGE_COMMIT(g.tick, m1, PlayerStateInit_FallingWithInput, e->sprite.queuedStateMarker);
}

/*
 * PlayerStateInit_TeleportLanding (0x8006AA9C, 0x15C) — MATCHED 2026-07-04.
 * IdleLookAround-family (Variant B) installer: no jumpParam, leading fence,
 * event=EventHandlerWithQueue, input=IdleInputHandler, render Horizontal/Riding,
 * spriteId 0x1A28C11A, queued=PlayerStateInit_Idle.
 */
void PlayerStateInit_TeleportLanding(PlayerEntity *e) {
    struct { s32 lead; CallbackSlot tick, event, input, render; } g;
    CallbackSlot scratch;
    struct { s32 pad; CallbackSlot s; s32 tail[2]; } curP;
    void (*rfn)();
    void (*fn)();
    register s16 m1 PSX_REG("$17");
    do {} while (0);
    FSM_STAGE_SLOT_FIRST(fn, g.tick, m1, PlayerTickCallback);
    FSM_STAGE_SLOT(fn, g.event, m1, PlayerCallback_EventHandlerWithQueue);
    FSM_STAGE_SLOT(fn, g.input, m1, PlayerCallback_IdleInputHandler);
    FSM_STAGE_RENDER(rfn, scratch, g.render, m1, e->interactEntity != NULL, PlayerCallback_HorizontalWallCollision, PlayerCallback_RidingPlatformPhysics);
    FSM_COMMIT_SLOT(curP.s, g.tick, e->sprite.base.tickMarker);
    FSM_COMMIT_SLOT(curP.s, g.event, e->sprite.base.eventMarker);
    FSM_COMMIT_SLOT(curP.s, g.input, e->inputStateMarker);
    FSM_COMMIT_SLOT(curP.s, g.render, e->sprite.base.renderMarker);
    SetEntitySpriteId(e, 0x1A28C11A, 1);
    FSM_STAGE_COMMIT(g.tick, m1, PlayerStateInit_Idle, e->sprite.queuedStateMarker);
}

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
 * PlayerStateInit_ClimbIdle (0x8006B304, 0xF4) — MATCHED 2026-07-05.
 * Lightweight installer sub-family: entity stays in $a0 (no queued store after
 * the sprite call, so e never needs a callee-saved home) and markerHi(-1) lives
 * in a temp reg (no $s1 pin). Render slot is written zero unconditionally (no
 * interactEntity branch). Frame 0x58 reproduced with the lead-prefixed g
 * aggregate + curP double-buffer padded with tail[3].
 */
extern void PlayerCallback_CrouchClimbInputHandler();
void PlayerStateInit_ClimbIdle(PlayerEntity *e) {
    struct { s32 lead; CallbackSlot tick, event, input, render; } g;
    struct { s32 pad; CallbackSlot s; s32 tail[3]; } curP;
    void (*fn)();
    s16 m1;
    do {} while (0);
    FSM_STAGE_SLOT_FIRST(fn, g.tick, m1, PlayerTickCallback);
    FSM_STAGE_SLOT(fn, g.event, m1, PlayerEntityEventHandlerAlt);
    FSM_STAGE_SLOT(fn, g.input, m1, PlayerCallback_CrouchClimbInputHandler);
    g.render.markerLo = 0; g.render.markerHi = 0; g.render.fn = (void (*)())0;
    FSM_COMMIT_SLOT(curP.s, g.tick, e->sprite.base.tickMarker);
    FSM_COMMIT_SLOT(curP.s, g.event, e->sprite.base.eventMarker);
    FSM_COMMIT_SLOT(curP.s, g.input, e->inputStateMarker);
    FSM_COMMIT_SLOT(curP.s, g.render, e->sprite.base.renderMarker);
    SetEntitySpriteId(e, 0x0708A4A0, 1);
}

/*
 * PlayerStateInit_DamageKnockback (0x8006B3F8, 0xFC) — MATCHED 2026-07-05.
 * Lightweight installer twin; all four slots are real callbacks (markerHi=-1),
 * render is PlayerCallback_KnockbackPhysics (not zeroed). Frame 0x50 via curP
 * tail[1]. No queued store.
 */
extern void PlayerCallback_CrouchClimbTickHandler();
void PlayerStateInit_DamageKnockback(PlayerEntity *e) {
    struct { s32 lead; CallbackSlot tick, event, input, render; } g;
    struct { s32 pad; CallbackSlot s; s32 tail[1]; } curP;
    void (*fn)();
    s16 m1;
    do {} while (0);
    FSM_STAGE_SLOT_FIRST(fn, g.tick, m1, PlayerTickCallback);
    FSM_STAGE_SLOT(fn, g.event, m1, PlayerEntityEventHandlerAlt);
    FSM_STAGE_SLOT(fn, g.input, m1, PlayerCallback_CrouchClimbTickHandler);
    FSM_STAGE_SLOT(fn, g.render, m1, PlayerCallback_KnockbackPhysics);
    FSM_COMMIT_SLOT(curP.s, g.tick, e->sprite.base.tickMarker);
    FSM_COMMIT_SLOT(curP.s, g.event, e->sprite.base.eventMarker);
    FSM_COMMIT_SLOT(curP.s, g.input, e->inputStateMarker);
    FSM_COMMIT_SLOT(curP.s, g.render, e->sprite.base.renderMarker);
    SetEntitySpriteId(e, 0x052AA082, 1);
}

extern s32 PlaySoundEffectRet(u32 soundId, s32 volume, s32 channel) asm("PlaySoundEffect");
u32 PlayerDamageKnockbackNextMarker asm("D_800A5F34");
EntityCallback PlayerDamageKnockbackNextFn asm("D_800A5F38");
/*
 * PlayerState_DamageKnockback (0x8006B4F4, 0x140) — MATCHED 2026-07-06.
 * Damage-knockback installer: silences the active SPU voice, retriggers the
 * knockback sound effect, installs the four state callbacks (frame 0x50 via
 * curP tail[1], same as the lightweight DamageKnockback twin) and queues the
 * deferred next-state install via EntitySetCallback(D_800A5F34/D_800A5F38).
 */
void PlayerState_DamageKnockback(PlayerEntity *e) {
    struct { s32 lead; CallbackSlot tick, event, input, render; } g;
    struct { s32 pad; CallbackSlot s; s32 tail[1]; } curP;
    void (*fn)();
    s16 m1;
    StopSPUVoice(e->soundHandle);
    e->soundHandle = PlaySoundEffectRet(0x421586C2, 0xA0, 0);
    do {} while (0);
    FSM_STAGE_SLOT_FIRST(fn, g.tick, m1, PlayerTickCallback);
    FSM_STAGE_SLOT(fn, g.event, m1, PlayerEntityEventHandlerAlt);
    FSM_STAGE_SLOT(fn, g.input, m1, PlayerCallback_CrouchClimbTickHandler);
    FSM_STAGE_SLOT(fn, g.render, m1, PlayerCallback_KnockbackPhysics);
    FSM_COMMIT_SLOT(curP.s, g.tick, e->sprite.base.tickMarker);
    FSM_COMMIT_SLOT(curP.s, g.event, e->sprite.base.eventMarker);
    FSM_COMMIT_SLOT(curP.s, g.input, e->inputStateMarker);
    FSM_COMMIT_SLOT(curP.s, g.render, e->sprite.base.renderMarker);
    SetEntitySpriteId(e, 0x8569A090, 1);
    EntitySetCallback((Entity *)e, PlayerDamageKnockbackNextMarker, PlayerDamageKnockbackNextFn);
}

void PlayerDestroyVoiceCallback(PlayerEntity *e) {
    StopSPUVoice(e->soundHandle);
}

INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_CrouchOrClimbTransition);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerStateInit_ClimbWithMirror);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerStateInit_HeavyDamageLaunch);

extern void PlayerCallback_JumpInputAndCounters();
extern u16 g_WalkFallTurnBlockMask[] asm("D_800A5970");
u32 PlayerDamageRecoilNextMarker asm("D_800A5F3C");
EntityCallback PlayerDamageRecoilNextFn asm("D_800A5F40");
/*
 * PlayerStateInit_DamageRecoilFall (0x8006BC04, 0x17C) — MATCHED 2026-07-06.
 * Damage-recoil fall setup: seeds the velocity cap from the turn-block button
 * mask, clears the landing/bounce/jump counters and alt-speed, floors the Y
 * velocity to -0x90000, then installs the four state callbacks (frame 0x50)
 * and queues the deferred next-state install (D_800A5F3C/D_800A5F40).
 */
void PlayerStateInit_DamageRecoilFall(PlayerEntity *e) {
    struct { s32 lead; CallbackSlot tick, event, input, render; } g;
    struct { s32 pad; CallbackSlot s; s32 tail[1]; } curP;
    void (*fn)();
    s16 m1;
    s32 vy;
    if (*(u16 *)e->pInput & g_WalkFallTurnBlockMask[0])
        e->maxVelocity = 0x38000;
    else
        e->maxVelocity = 0x28000;
    vy = e->velocityY_fixed;
    e->landingTimer = 0;
    e->bounceLockTimer = 0;
    e->jumpHoldCounter = 0;
    e->altSpeed = 0;
    e->cushionVelY = 0;
    e->velocityX_fixed = 0;
    e->timer13C = 0;
    if (vy < (s32)0xFFF70000)
        e->velocityY_fixed = (s32)0xFFF70000;
    do {} while (0);
    FSM_STAGE_SLOT_FIRST(fn, g.tick, m1, PlayerState_CooldownTick);
    FSM_STAGE_SLOT(fn, g.event, m1, PlayerCallback_CollisionDamageSetup);
    FSM_STAGE_SLOT(fn, g.input, m1, PlayerCallback_JumpInputAndCounters);
    FSM_STAGE_SLOT(fn, g.render, m1, PlayerCallback_FallingPhysicsMain);
    FSM_COMMIT_SLOT(curP.s, g.tick, e->sprite.base.tickMarker);
    FSM_COMMIT_SLOT(curP.s, g.event, e->sprite.base.eventMarker);
    FSM_COMMIT_SLOT(curP.s, g.input, e->inputStateMarker);
    FSM_COMMIT_SLOT(curP.s, g.render, e->sprite.base.renderMarker);
    SetEntitySpriteId(e, 0x8524A880, 1);
    EntitySetCallback((Entity *)e, PlayerDamageRecoilNextMarker, PlayerDamageRecoilNextFn);
}

void PlayerState_RemoveAttachedEntity(PlayerEntity *e) {
    Entity *portal;
    g_pGameState->camera_follow_direction = 0;
    portal = e->swirlPortalEntity;
    if (portal != NULL) {
        RemoveEntityFromAllLists(g_pGameState, portal);
        e->swirlPortalEntity = NULL;
    }
}

/*
 * PlayerStateInit_ThrowProjectile (0x8006BDD4, 0x158) — MATCHED 2026-07-04.
 * Variant A installer with a NULL input slot (markerLo=0, markerHi=0, fn=NULL).
 * The zeroed input slot makes the target reserve an extra 8-byte stack scratch
 * (frame 0x70 vs 0x68); reproduced by enlarging curP.tail to [4]. event=Throw-
 * EventHandler, render Horizontal/Riding, spriteId 0x04084011, queued=Init_Idle.
 */
extern void PlayerCallback_ThrowEventHandler();
void PlayerStateInit_ThrowProjectile(PlayerEntity *e) {
    struct { s32 lead; CallbackSlot tick, event, input, render; } g;
    CallbackSlot scratch;
    struct { s32 pad; CallbackSlot s; s32 tail[4]; } curP;
    void (*rfn)();
    void (*fn)();
    register s16 m1 PSX_REG("$17");
    e->carryMotionX = 0;
    do {} while (0);
    FSM_STAGE_SLOT_FIRST(fn, g.tick, m1, PlayerTickCallback);
    fn = (void (*)())PlayerCallback_ThrowEventHandler; FSM_KEEP_LIVE(fn);
    g.event.markerLo = 0; g.event.markerHi = m1; g.event.fn = fn;
    g.input.markerLo = 0; g.input.markerHi = 0; g.input.fn = (void (*)())0;
    FSM_STAGE_RENDER(rfn, scratch, g.render, m1, e->interactEntity != NULL, PlayerCallback_HorizontalWallCollision, PlayerCallback_RidingPlatformPhysics);
    FSM_COMMIT_SLOT(curP.s, g.tick, e->sprite.base.tickMarker);
    FSM_COMMIT_SLOT(curP.s, g.event, e->sprite.base.eventMarker);
    FSM_COMMIT_SLOT(curP.s, g.input, e->inputStateMarker);
    FSM_COMMIT_SLOT(curP.s, g.render, e->sprite.base.renderMarker);
    SetEntitySpriteId(e, 0x04084011, 1);
    FSM_STAGE_COMMIT(g.tick, m1, PlayerStateInit_Idle, e->sprite.queuedStateMarker);
}

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

/*
 * PlayerStateInit_CheckpointEntry (0x8006C3AC, 0x168) — NON-MATCHING (scheduler
 * residual, candidate for permuter).
 * Enters the checkpoint cutscene state: flags the global checkpoint restore as
 * pending (g_pGameState->checkpoint_restore_pending = 1), zeroes the carried-
 * platform motion (e->carryMotionX), installs the four state callbacks (frame
 * 0x70; tick=PlayerTickCallback, event=PlayerCallback_ProjectileEventHandler,
 * input=NULL, render=VerticalCollisionCheck / PlatformFollowUpdate when
 * e->interactEntity != NULL), sets sprite id 0x1C8C2437, and queues
 * PlayerStateInit_ProjectileThrowAnim as the next installer.
 *
 * All substantive codegen matches byte-for-byte. The only residual is a
 * ~4-instruction prologue weave: the target uses barrier-free natural gcc
 * scheduling that hoists both fn address-loads to a0/a1 up top AND interleaves
 * the callee-save stores (sw s1 / sw ra) late with the g_pGameState load and
 * li v0 / li s1. Reproducing the a0/a1 hoist requires FSM_KEEP_LIVE barriers,
 * but a volatile-asm barrier fragments the scheduling region and forces the
 * saves to land right after a1 (early) instead of woven late — the two
 * requirements are mutually exclusive in C source. Deferred to the permuter.
 */
INCLUDE_ASM("asm/nonmatchings/playst", PlayerStateInit_CheckpointEntry);

/*
 * PlayerStateInit_ProjectileThrowAnim (0x8006C514, 0x154) — MATCHED 2026-07-05.
 * Clean-template Variant B installer with a zeroed input slot (frame 0x70 via
 * curP.tail[4]). event=ProjectileEventHandler, render VerticalCollisionCheck/
 * PlatformFollowUpdate, spriteId 0x3838801A, queued=PlayerStateInit_PostThrow-
 * Recovery.
 */
extern void PlayerCallback_ProjectileEventHandler();
extern void PlayerCallback_VerticalCollisionCheck();
extern void PlayerCallback_PlatformFollowUpdate();
void PlayerStateInit_PostThrowRecovery(PlayerEntity *e);
void PlayerStateInit_ProjectileThrowAnim(PlayerEntity *e) {
    struct { s32 lead; CallbackSlot tick, event, input, render; } g;
    CallbackSlot scratch;
    struct { s32 pad; CallbackSlot s; s32 tail[4]; } curP;
    void (*rfn)();
    void (*fn)();
    register s16 m1 PSX_REG("$17");
    do {} while (0);
    FSM_STAGE_SLOT_FIRST(fn, g.tick, m1, PlayerTickCallback);
    fn = (void (*)())PlayerCallback_ProjectileEventHandler; FSM_KEEP_LIVE(fn);
    g.event.markerLo = 0; g.event.markerHi = m1; g.event.fn = fn;
    g.input.markerLo = 0; g.input.markerHi = 0; g.input.fn = (void (*)())0;
    FSM_STAGE_RENDER(rfn, scratch, g.render, m1, e->interactEntity != NULL, PlayerCallback_VerticalCollisionCheck, PlayerCallback_PlatformFollowUpdate);
    FSM_COMMIT_SLOT(curP.s, g.tick, e->sprite.base.tickMarker);
    FSM_COMMIT_SLOT(curP.s, g.event, e->sprite.base.eventMarker);
    FSM_COMMIT_SLOT(curP.s, g.input, e->inputStateMarker);
    FSM_COMMIT_SLOT(curP.s, g.render, e->sprite.base.renderMarker);
    SetEntitySpriteId(e, 0x3838801A, 1);
    FSM_STAGE_COMMIT(g.tick, m1, PlayerStateInit_PostThrowRecovery, e->sprite.queuedStateMarker);
}

/*
 * PlayerStateInit_PostThrowRecovery (0x8006C668, 0x164) — MATCHED 2026-07-05.
 * Variant B installer (no prologue store → leading fence) with a zeroed input
 * slot (frame 0x70 via curP.tail[4]). event=BaseEventHandler, render Vertical-
 * CollisionCheck/PlatformFollowUpdate, spriteId 0x48204012. After the sprite id
 * it installs the deferred next-state via EntitySetCallback(e, D_800A5F54,
 * D_800A5F58), then queues PlayerStateInit_Idle.
 */
extern void PlayerCallback_BaseEventHandler();
extern void PlayerCallback_VerticalCollisionCheck();
extern void PlayerCallback_PlatformFollowUpdate();
u32 PlayerPostThrowNextMarker asm("D_800A5F54");
EntityCallback PlayerPostThrowNextFn asm("D_800A5F58");
void PlayerStateInit_PostThrowRecovery(PlayerEntity *e) {
    struct { s32 lead; CallbackSlot tick, event, input, render; } g;
    CallbackSlot scratch;
    struct { s32 pad; CallbackSlot s; s32 tail[4]; } curP;
    void (*rfn)();
    void (*fn)();
    register s16 m1 PSX_REG("$17");
    do {} while (0);
    FSM_STAGE_SLOT_FIRST(fn, g.tick, m1, PlayerTickCallback);
    fn = (void (*)())PlayerCallback_BaseEventHandler; FSM_KEEP_LIVE(fn);
    g.event.markerLo = 0; g.event.markerHi = m1; g.event.fn = fn;
    g.input.markerLo = 0; g.input.markerHi = 0; g.input.fn = (void (*)())0;
    FSM_STAGE_RENDER(rfn, scratch, g.render, m1, e->interactEntity != NULL, PlayerCallback_VerticalCollisionCheck, PlayerCallback_PlatformFollowUpdate);
    FSM_COMMIT_SLOT(curP.s, g.tick, e->sprite.base.tickMarker);
    FSM_COMMIT_SLOT(curP.s, g.event, e->sprite.base.eventMarker);
    FSM_COMMIT_SLOT(curP.s, g.input, e->inputStateMarker);
    FSM_COMMIT_SLOT(curP.s, g.render, e->sprite.base.renderMarker);
    SetEntitySpriteId(e, 0x48204012, 1);
    EntitySetCallback((Entity *)e, PlayerPostThrowNextMarker, PlayerPostThrowNextFn);
    FSM_STAGE_COMMIT(g.tick, m1, PlayerStateInit_Idle, e->sprite.queuedStateMarker);
}

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

extern void ClearEntityStateFlag();
u32 PlayerRestoreShrinkNextMarker asm("D_800A5F5C");
EntityCallback PlayerRestoreShrinkNextFn asm("D_800A5F60");
/*
 * PlayerStateInit_RestoreNormalFromShrink (0x8006D270, 0x158, frame 0x50) --
 * up-front-fn installer, but WITHOUT the $s1 reg-save conflict that shelves the
 * SpecialIdleAnim/BounceActive twins (here only s0/ra are saved, so the eager-
 * load barriers flushing them to the top is exactly right). Clears the glide
 * entity's state flag, sets groundedFlag=1, resets the global bounce flag and
 * the player hitbox (width 0x28, y-offset -0x10), then installs the four state
 * callbacks (tick->CooldownTick, event->PlayerEntityCollisionHandler,
 * input->JumpInputAndCounters, render->FallingPhysicsMain) and the deferred
 * next-state install (D_800A5F5C/D_800A5F60). No queued store.
 */
void PlayerStateInit_RestoreNormalFromShrink(PlayerEntity *e) {
    struct { s32 lead; CallbackSlot tick, event, input, render; } g;
    struct { s32 pad; CallbackSlot s; s32 tail[1]; } curP;
    register void (*f0)() PSX_REG("$7");
    register void (*f1)() PSX_REG("$8");
    register void (*f2)() PSX_REG("$9");
    register void (*f3)() PSX_REG("$10");
    s16 m1;
    ClearEntityStateFlag(e->glideEntity);
    f0 = (void (*)())PlayerState_CooldownTick;
    f1 = (void (*)())PlayerEntityCollisionHandler;
    f2 = (void (*)())PlayerCallback_JumpInputAndCounters;
    f3 = (void (*)())PlayerCallback_FallingPhysicsMain;
    FSM_KEEP_LIVE(f0); FSM_KEEP_LIVE(f1); FSM_KEEP_LIVE(f2); FSM_KEEP_LIVE(f3);
    e->groundedFlag = 1;
    g_pGameState->bounce_active_flag = 0;
    g_pGameState->player_hitbox_width = 0x28;
    g_pGameState->player_hitbox_y_offset = -0x10;
    m1 = -1;
    g.tick.markerLo = 0;  g.tick.markerHi = m1;  g.tick.fn = f0;
    g.event.markerLo = 0; g.event.markerHi = m1; g.event.fn = f1;
    g.input.markerLo = 0; g.input.markerHi = m1; g.input.fn = f2;
    g.render.markerLo = 0; g.render.markerHi = m1; g.render.fn = f3;
    FSM_COMMIT_SLOT(curP.s, g.tick, e->sprite.base.tickMarker);
    FSM_COMMIT_SLOT(curP.s, g.event, e->sprite.base.eventMarker);
    FSM_COMMIT_SLOT(curP.s, g.input, e->inputStateMarker);
    FSM_COMMIT_SLOT(curP.s, g.render, e->sprite.base.renderMarker);
    SetEntitySpriteId(e, 0xD0A5F094, 1);
    EntitySetCallback((Entity *)e, PlayerRestoreShrinkNextMarker, PlayerRestoreShrinkNextFn);
}

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
    e->facing = facing;
    if (anchor != 0x10000) {
        e->scaleRender = anchor;
        e->scaleRender2 = anchor;
        e->scalePowerupX = anchor;
        e->scalePowerupY = anchor;
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






/* playst .sdata (0x800A5D38..0x800A5F64): {marker=0xFFFF0000, callback} descriptor
 * pairs for the player state machine (bounce / respawn / death / powerup / swim /
 * knockback / jump / pickup / level-exit transitions). Migrated from the pooled asm
 * sdata blob (sdata-under-split Phase 4). Address order == declaration order (cc1
 * 2.7.2 emits initialized .sdata in decl order). Friendly-named pairs have
 * uninitialized tentative-defs earlier in this file; a same-TU (same C name)
 * tentative-def + initialized strong-def merges to one .sdata entry, so those
 * decls are left in place. Callbacks defined elsewhere are extern-declared; all
 * cast to EntityCallback. */
extern void PlayerStateInit_RespawnAtCheckpoint();
extern void PlayerState_Death();
extern void HamsterSpriteCallback();
extern void PlayerState_DeathStart();
extern void PlayerState_PlayRandomIdleAnimation();
extern void PlayerState_PickupItem();
extern void PlayerState_LevelExitTeleporter();
extern void PlayerState_CheckpointExit();
extern void PlayerStateInit_JumpFromPlatform();
extern void PlayerStateInit_CheckpointRestore();
extern void UniverseEnemaActivate();
extern void PlayerStateInit_CheckpointEntry();
extern void PlayerStateInit_BounceFromEnemy();
extern void PlayerStateInit_TeleportFallVariant();
extern void PlayerEnterLandingState();
extern void PlayerState_SpecialIdleAnim();
extern void PlayerStateInit_ExitShrinkWithRestore();
extern void PlayerStateInit_ShrinkAndFall();
extern void PlayerStateInit_GrowFromShrink();
extern void PlayerStateInit_TeleportEntry();
extern void PlayerStateInit_HeavyDamageLaunch();
extern void PlayerState_CrouchOrClimbTransition();
extern void PlayerState_StartSwimming();
extern void PlayerStateInit_ClimbWithMirror();
extern void PlayerState_LandingFromRide();
extern void UniverseEnemaKillAllEnemies();
u32 D_800A5D38 asm("D_800A5D38") = 0xFFFF0000;
EntityCallback D_800A5D3C asm("D_800A5D3C") = (EntityCallback)PlayerState_SetupBounceRight;
u32 D_800A5D40 asm("D_800A5D40") = 0xFFFF0000;
EntityCallback D_800A5D44 asm("D_800A5D44") = (EntityCallback)PlayerState_SetupBounceUp;
u32 D_800A5D48 asm("D_800A5D48") = 0xFFFF0000;
EntityCallback D_800A5D4C asm("D_800A5D4C") = (EntityCallback)PlayerState_SetupBounceUpWithVelocity;
u32 PlayerBounceDownMarker asm("D_800A5D50") = 0xFFFF0000;
EntityCallback PlayerBounceDownFn asm("D_800A5D54") = (EntityCallback)PlayerState_SetupBounceDown;
u32 PlayerQuickTurnMarker asm("D_800A5D58") = 0xFFFF0000;
EntityCallback PlayerQuickTurnFn asm("D_800A5D5C") = (EntityCallback)PlayerState_QuickTurn;
u32 D_800A5D60 asm("D_800A5D60") = 0xFFFF0000;
EntityCallback D_800A5D64 asm("D_800A5D64") = (EntityCallback)PlayerState_BounceLaunch;
u32 D_800A5D68 asm("D_800A5D68") = 0xFFFF0000;
EntityCallback D_800A5D6C asm("D_800A5D6C") = (EntityCallback)PlayerBounceCallback_Primary;
u32 D_800A5D70 asm("D_800A5D70") = 0xFFFF0000;
EntityCallback D_800A5D74 asm("D_800A5D74") = (EntityCallback)PlayerState_SetupBounceDownFast;
u32 PlayerRespawnMarker asm("D_800A5D78") = 0xFFFF0000;
EntityCallback PlayerRespawnFn asm("D_800A5D7C") = (EntityCallback)PlayerStateInit_RespawnAtCheckpoint;
u32 PlayerDeathMarker asm("D_800A5D80") = 0xFFFF0000;
EntityCallback PlayerDeathFn asm("D_800A5D84") = (EntityCallback)PlayerState_Death;
u32 D_800A5D88 asm("D_800A5D88") = 0xFFFF0000;
EntityCallback D_800A5D8C asm("D_800A5D8C") = (EntityCallback)PlayerStateInit_CrouchSlide;
u32 D_800A5D90 asm("D_800A5D90") = 0xFFFF0000;
EntityCallback D_800A5D94 asm("D_800A5D94") = (EntityCallback)HamsterSpriteCallback;
u32 PLAYER_DEATH_STATE_MARKER asm("D_800A5D98") = 0xFFFF0000;
EntityCallback PLAYER_DEATH_STATE_CALLBACK asm("D_800A5D9C") = (EntityCallback)PlayerState_DeathStart;
u32 D_800A5DA0 asm("D_800A5DA0") = 0xFFFF0000;
EntityCallback D_800A5DA4 asm("D_800A5DA4") = (EntityCallback)PlayerState_PlayRandomIdleAnimation;
u32 D_800A5DA8 asm("D_800A5DA8") = 0xFFFF0000;
EntityCallback D_800A5DAC asm("D_800A5DAC") = (EntityCallback)PlayerState_IdleLookAround;
u32 PLAYER_PICKUP_STATE_MARKER asm("D_800A5DB0") = 0xFFFF0000;
EntityCallback PLAYER_PICKUP_STATE_CALLBACK asm("D_800A5DB4") = (EntityCallback)PlayerState_PickupItem;
u32 PlayerZoneEnterMarker asm("D_800A5DB8") = 0xFFFF0000;
EntityCallback PlayerZoneEnterFn asm("D_800A5DBC") = (EntityCallback)PlayerState_LevelExitTeleporter;
u32 PlayerZoneWarpMarker asm("D_800A5DC0") = 0xFFFF0000;
EntityCallback PlayerZoneWarpFn asm("D_800A5DC4") = (EntityCallback)PlayerState_CheckpointExit;
u32 D_800A5DC8 asm("D_800A5DC8") = 0xFFFF0000;
EntityCallback D_800A5DCC asm("D_800A5DCC") = (EntityCallback)PlayerStateInit_JumpFromPlatform;
u32 PlayerRunningStateMarker asm("D_800A5DD0") = 0xFFFF0000;
EntityCallback PlayerRunningStateFn asm("D_800A5DD4") = (EntityCallback)PlayerState_Running;
u32 PlayerPowerupPhoenixMarker asm("D_800A5DD8") = 0xFFFF0000;
EntityCallback PlayerPowerupPhoenixFn asm("D_800A5DDC") = (EntityCallback)PlayerStateInit_ThrowProjectile;
u32 PlayerPowerupWillieMarker asm("D_800A5DE0") = 0xFFFF0000;
EntityCallback PlayerPowerupWillieFn asm("D_800A5DE4") = (EntityCallback)PlayerStateInit_CheckpointRestore;
u32 PlayerPowerupThirdMarker asm("D_800A5DE8") = 0xFFFF0000;
EntityCallback PlayerPowerupThirdFn asm("D_800A5DEC") = (EntityCallback)UniverseEnemaActivate;
u32 PlayerPowerupFourthMarker asm("D_800A5DF0") = 0xFFFF0000;
EntityCallback PlayerPowerupFourthFn asm("D_800A5DF4") = (EntityCallback)PlayerStateInit_CheckpointEntry;
u32 D_800A5DF8 asm("D_800A5DF8") = 0xFFFF0000;
EntityCallback D_800A5DFC asm("D_800A5DFC") = (EntityCallback)PlayerStateInit_TeleportIdleOnPlatform;
u32 D_800A5E00 asm("D_800A5E00") = 0xFFFF0000;
EntityCallback D_800A5E04 asm("D_800A5E04") = (EntityCallback)PlayerStateInit_BounceFromEnemy;
u32 D_800A5E08 asm("D_800A5E08") = 0xFFFF0000;
EntityCallback D_800A5E0C asm("D_800A5E0C") = (EntityCallback)PlayerState_JumpApex;
u32 D_800A5E10 asm("D_800A5E10") = 0xFFFF0000;
EntityCallback D_800A5E14 asm("D_800A5E14") = (EntityCallback)PlayerState_WalkingRight;
u32 D_800A5E18 asm("D_800A5E18") = 0xFFFF0000;
EntityCallback D_800A5E1C asm("D_800A5E1C") = (EntityCallback)PlayerState_WalkingLeft;
u32 D_800A5E20 asm("D_800A5E20") = 0xFFFF0000;
EntityCallback D_800A5E24 asm("D_800A5E24") = (EntityCallback)PlayerStateInit_Walking;
u32 D_800A5E28 asm("D_800A5E28") = 0xFFFF0000;
EntityCallback D_800A5E2C asm("D_800A5E2C") = (EntityCallback)PlayerStateInit_TeleportFallVariant;
u32 D_800A5E30 asm("D_800A5E30") = 0xFFFF0000;
EntityCallback D_800A5E34 asm("D_800A5E34") = (EntityCallback)PlayerStateInit_FallFromLedge;
u32 D_800A5E38 asm("D_800A5E38") = 0xFFFF0000;
EntityCallback D_800A5E3C asm("D_800A5E3C") = (EntityCallback)PlayerStateInit_TeleportWalking;
u32 D_800A5E40 asm("D_800A5E40") = 0xFFFF0000;
EntityCallback D_800A5E44 asm("D_800A5E44") = (EntityCallback)PlayerState_SpecialMove2;
u32 D_800A5E48 asm("D_800A5E48") = 0xFFFF0000;
EntityCallback D_800A5E4C asm("D_800A5E4C") = (EntityCallback)PlayerEnterLandingState;
u32 D_800A5E50 asm("D_800A5E50") = 0xFFFF0000;
EntityCallback D_800A5E54 asm("D_800A5E54") = (EntityCallback)PlayerState_Jump;
u32 D_800A5E58 asm("D_800A5E58") = 0xFFFF0000;
EntityCallback D_800A5E5C asm("D_800A5E5C") = (EntityCallback)PlayerState_Falling;
u32 D_800A5E60 asm("D_800A5E60") = 0xFFFF0000;
EntityCallback D_800A5E64 asm("D_800A5E64") = (EntityCallback)PlayerStateInit_ResumeWalking;
u32 D_800A5E68 asm("D_800A5E68") = 0xFFFF0000;
EntityCallback D_800A5E6C asm("D_800A5E6C") = (EntityCallback)PlayerState_SpecialIdleAnim;
u32 D_800A5E70 asm("D_800A5E70") = 0xFFFF0000;
EntityCallback D_800A5E74 asm("D_800A5E74") = (EntityCallback)PlayerStateInit_ExitShrinkWithRestore;
u32 D_800A5E78 asm("D_800A5E78") = 0xFFFF0000;
EntityCallback D_800A5E7C asm("D_800A5E7C") = (EntityCallback)PlayerStateInit_ShrinkAndFall;
u32 D_800A5E80 asm("D_800A5E80") = 0xFFFF0000;
EntityCallback D_800A5E84 asm("D_800A5E84") = (EntityCallback)PlayerStateInit_GrowFromShrink;
u32 D_800A5E88 asm("D_800A5E88") = 0xFFFF0000;
EntityCallback D_800A5E8C asm("D_800A5E8C") = (EntityCallback)PlayerStateInit_TeleportFalling;
u32 D_800A5E90 asm("D_800A5E90") = 0xFFFF0000;
EntityCallback D_800A5E94 asm("D_800A5E94") = (EntityCallback)PlayerStateInit_Falling;
u32 D_800A5E98 asm("D_800A5E98") = 0xFFFF0000;
EntityCallback D_800A5E9C asm("D_800A5E9C") = (EntityCallback)PlayerStateInit_TeleportEntry;
u32 D_800A5EA0 asm("D_800A5EA0") = 0xFFFF0000;
EntityCallback D_800A5EA4 asm("D_800A5EA4") = (EntityCallback)PlayerStateInit_TeleportLanding;
u32 D_800A5EA8 asm("D_800A5EA8") = 0xFFFF0000;
EntityCallback D_800A5EAC asm("D_800A5EAC") = (EntityCallback)PlayerStateInit_HeavyDamageLaunch;
u32 D_800A5EB0 asm("D_800A5EB0") = 0xFFFF0000;
EntityCallback D_800A5EB4 asm("D_800A5EB4") = (EntityCallback)PlayerStateInit_DamageKnockback;
u32 D_800A5EB8 asm("D_800A5EB8") = 0xFFFF0000;
EntityCallback D_800A5EBC asm("D_800A5EBC") = (EntityCallback)PlayerState_DamageKnockback;
u32 PlayerKnockbackCeilingMarker asm("D_800A5EC0") = 0xFFFF0000;
EntityCallback PlayerKnockbackCeilingFn asm("D_800A5EC4") = (EntityCallback)PlayerState_CrouchOrClimbTransition;
char g_FntFmt_PercentS[4] asm("g_FntFmt_PercentS") = "%s";
u32 D_800A5ECC asm("D_800A5ECC") = 0xFFFF0000;
EntityCallback D_800A5ED0 asm("D_800A5ED0") = (EntityCallback)PlayerState_StartSwimming;
u32 D_800A5ED4 asm("D_800A5ED4") = 0xFFFF0000;
EntityCallback D_800A5ED8 asm("D_800A5ED8") = (EntityCallback)PlayerStateInit_ClimbWithMirror;
u32 D_800A5EDC asm("D_800A5EDC") = 0xFFFF0000;
EntityCallback D_800A5EE0 asm("D_800A5EE0") = (EntityCallback)PlayerState_LandingFromRide;
u32 D_800A5EE4 asm("D_800A5EE4") = 0xFFFF0000;
EntityCallback D_800A5EE8 asm("D_800A5EE8") = (EntityCallback)PlayerState_SpecialMove1;
u32 D_800A5EEC asm("D_800A5EEC") = 0xFFFF0000;
EntityCallback D_800A5EF0 asm("D_800A5EF0") = (EntityCallback)PlayerState_StandingIdle;
u32 PlayerKnockbackLandMarker asm("D_800A5EF4") = 0xFFFF0000;
EntityCallback PlayerKnockbackLandFn asm("D_800A5EF8") = (EntityCallback)PlayerStateInit_DamageRecoilFall;
u32 D_800A5EFC asm("D_800A5EFC") = 0xFFFF0000;
EntityCallback D_800A5F00 asm("D_800A5F00") = (EntityCallback)PlayerState_UpdateTPageAndHideHalo;
u32 PlayerSpecialMoveNextMarker asm("D_800A5F04") = 0xFFFF0000;
EntityCallback PlayerSpecialMoveNextFn asm("D_800A5F08") = (EntityCallback)PlayerState_ClearInAirFlag;
u32 PlayerJumpNextMarker asm("D_800A5F0C") = 0xFFFF0000;
EntityCallback PlayerJumpNextFn asm("D_800A5F10") = (EntityCallback)PlayerState_StopSoundAndClear;
u32 PlayerHiddenIdleMarker asm("D_800A5F14") = 0xFFFF0000;
EntityCallback PlayerHiddenIdleFn asm("D_800A5F18") = (EntityCallback)PlayerState_ClearBounceFlag;
u32 D_800A5F1C asm("D_800A5F1C") = 0xFFFF0000;
EntityCallback D_800A5F20 asm("D_800A5F20") = (EntityCallback)PlayerState_ClearBounceAndAirFlag;
u32 D_800A5F24 asm("D_800A5F24") = 0xFFFF0000;
EntityCallback D_800A5F28 asm("D_800A5F28") = (EntityCallback)PlayerState_TransitionToLevelExit;
u32 PlayerLevelExitMarker asm("D_800A5F2C") = 0xFFFF0000;
EntityCallback PlayerLevelExitFn asm("D_800A5F30") = (EntityCallback)PlayerState_LevelExitComplete;
u32 PlayerDamageKnockbackNextMarker asm("D_800A5F34") = 0xFFFF0000;
EntityCallback PlayerDamageKnockbackNextFn asm("D_800A5F38") = (EntityCallback)PlayerDestroyVoiceCallback;
u32 PlayerDamageRecoilNextMarker asm("D_800A5F3C") = 0xFFFF0000;
EntityCallback PlayerDamageRecoilNextFn asm("D_800A5F40") = (EntityCallback)PlayerState_RemoveAttachedEntity;
u32 D_800A5F44 asm("D_800A5F44") = 0xFFFF0000;
EntityCallback D_800A5F48 asm("D_800A5F48") = (EntityCallback)PlayerState_CheckpointRestoreComplete;
u32 D_800A5F4C asm("D_800A5F4C") = 0xFFFF0000;
EntityCallback D_800A5F50 asm("D_800A5F50") = (EntityCallback)UniverseEnemaKillAllEnemies;
u32 PlayerPostThrowNextMarker asm("D_800A5F54") = 0xFFFF0000;
EntityCallback PlayerPostThrowNextFn asm("D_800A5F58") = (EntityCallback)PlayerState_ClearCheckpointAndSetInvincible;
u32 PlayerRestoreShrinkNextMarker asm("D_800A5F5C") = 0xFFFF0000;
EntityCallback PlayerRestoreShrinkNextFn asm("D_800A5F60") = (EntityCallback)PlayerCallback_RestoreNormalHitbox;

/* .data island 0x8009C254..0x8009C270 (28B, playst table) migrated from asm. */
/* group island: 0-byte pad at 0x8009C254, 1 aliased symbol(s); anchor D_8009C254 (28B). */
u8 D_8009C254[28] asm("D_8009C254") = {
    0x88, 0xA0, 0x39, 0x81, 0x00, 0xA0, 0x29, 0x96,
    0xA4, 0x80, 0x08, 0xD7, 0x09, 0xE0, 0x28, 0xA6,
    0xEA, 0xA2, 0x38, 0x05, 0x89, 0xE0, 0x24, 0x25,
    0x00, 0x00, 0x00, 0x00,
};
