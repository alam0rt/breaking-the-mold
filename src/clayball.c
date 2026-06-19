#include "common.h"
#include "functions.h"
#include "globals.h"
#include "Game/entity_events.h"

extern u8 CLAYBALL_TEARDOWN_VTABLE[] asm("D_800116E8");
extern u8 SMALL_CLAYBALL_HELPER_VTABLE[] asm("D_80011708");

typedef struct SwitchClayballEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 *spawnDef;
    /* 0x104 */ u8 pad104[0x11A - 0x104];
    /* 0x11A */ u8 active;
    /* 0x11B */ u8 pad11B[0x124 - 0x11B];
    /* 0x124 */ Entity *linkedEntity;
    /* 0x128 */ s32 signalPayload;
} SwitchClayballEntity;

typedef struct SoundClayballEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x128 - 0x100];
    /* 0x128 */ s32 rollingVoice;
    /* 0x12C */ s32 impactVoice;
} SoundClayballEntity;

/* Per-tick path update for the looping clayball/circular-platform entity:
 * advances a timed path via InterpolateTimedPathPosition, writes the
 * resulting screen pos into the entity, and on the final waypoint zeroes
 * velocity, swaps in the PathFollower fall callbacks, and forwards event
 * 0x1006 ("path complete") to the linked switch-block at 0x124. */
INCLUDE_ASM("asm/nonmatchings/clayball", CircularPlatformUpdatePath);

/* Constructor for the circular-platform / looping-clayball entity variant
 * (sprite 0x3C0): installs ClayballTickWithParticleSpawn as tick,
 * CircularPlatformEventHandler as event handler, and
 * CircularPlatformUpdateWithMirror as movement callback. */
INCLUDE_ASM("asm/nonmatchings/clayball", InitCircularPlatformEntity);

/* Standard clayball per-tick callback: runs the sprite/animation tick,
 * then (if the "active" byte at +0x122 is set) dispatches a damage event
 * (code 3) to each player in the active list and spawns a trailing dust
 * particle (effect id 0x108) ~0x22 px below the ball — the rolling FX. */
INCLUDE_ASM("asm/nonmatchings/clayball", ClayballTickWithParticleSpawn);

/* Event handler for the circular-platform clayball: on the per-frame
 * "tick" event (1 / 0x100B) and once the linked switch (0x11C) is unset,
 * resolves spawn coords via SetEntitySpawnData and lazily creates the
 * paired SwitchBlock (event 0x114 = allocate-switch). */
INCLUDE_ASM("asm/nonmatchings/clayball", CircularPlatformEventHandler);

/* Movement callback wrapping InterpolateTimedPathPosition with an optional
 * X-axis mirror (negate sample.x when mirror flag at +0x12C ∈ {0..2}),
 * so the path can be reused for left/right-facing platforms.
 * @ 8005894C — arg ordering and s16 arithmetic diffs vs cc1. */
INCLUDE_ASM("asm/nonmatchings/clayball", CircularPlatformUpdateWithMirror);

extern SpriteEntity *InitEntitySprite(SpriteEntity *entity, u8 *def, s32 spriteId, s16 x, s16 y, s32 unused);
extern void GenericSpriteEntityInitCallback(SpriteEntity *entity, u16 param, u8 flags, s32 zero);
extern void ClayballResetState(SwitchClayballEntity *entity);
extern u8 SWITCH_CLAYBALL_GAMEPLAY_VTABLE[] asm("D_80011648");

/* Constructor for the switch-triggered clayball variants (entity types
 * 090/091/092): sets up sprite 0x3C0, temporarily swaps the collision
 * vtable to CLAYBALL_TEARDOWN_VTABLE for GenericSpriteEntityInitCallback, restores the
 * gameplay vtable SWITCH_CLAYBALL_GAMEPLAY_VTABLE, clears live state (+0x11A,+0x124) and calls
 * ClayballResetState to drop any leftover linked switch-block. */
SwitchClayballEntity *InitClayballWithSwitchBlock(SwitchClayballEntity *entity, u8 *def, u8 *spawnContext, u8 flags) {
    InitEntitySprite(&entity->sprite, spawnContext, 0x3C0, *(s16 *)(def + 0x8), (s16)(*(u16 *)(def + 0xA) - 1), 0);
    entity->sprite.base.collisionVtable = CLAYBALL_TEARDOWN_VTABLE;
    entity->spawnDef = def;
    GenericSpriteEntityInitCallback(&entity->sprite, *(u16 *)(def + 0xC), flags, 0);
    entity->sprite.base.collisionVtable = SWITCH_CLAYBALL_GAMEPLAY_VTABLE;
    entity->active = 0;
    entity->linkedEntity = NULL;
    ClayballResetState(entity);
    return entity;
}

/* Event handler for the switch-triggered clayball: routes a small set of
 * messages — 1004 ("switch-block attached", store sender ptr at 0x124),
 * 1005 ("switch-block detached", clear 0x124), 100B (no-op pass-through),
 * 1015 ("player signal": if def signal id at +0x15 matches, ClayballResetState).
 * @ 80058AAC — switch case layout diff; cc1 puts stores in j delay slots,
 * GCC doesn't reproduce. */
INCLUDE_ASM("asm/nonmatchings/clayball", ClayballSwitchEventHandler);

extern void ClayballSpawnSwitchBlock(SwitchClayballEntity *entity);

/* Signal listener for the on-demand clayball spawn: when global event
 * 0x1014 arrives carrying the def's signal id (def[0x15]), record the
 * caller's payload at +0x128 and trigger ClayballSpawnSwitchBlock — i.e.
 * the clayball remains dormant until its named trigger fires. */
s32 ClayballSpawnOnSignalHandler(SwitchClayballEntity *entity, u16 event, u32 param, s32 arg3) {
    if (event == EVT_SIGNAL_FIRE) {
        u8 *sub = entity->spawnDef;
        if (param == sub[0x15]) {
            entity->signalPayload = arg3;
            ClayballSpawnSwitchBlock(entity);
        }
    }
    return 0;
}

/* Teardown helper: if a paired switch-block exists at +0x11C remove it
 * from all entity lists and clear the slot, and if a linked follower
 * entity at +0x124 is bound dispatch event 0x1006 ("unbind / detach")
 * to it before zeroing the link. Returns the clayball to a clean state. */
INCLUDE_ASM("asm/nonmatchings/clayball", ClayballResetState);

/* Allocates and inits the SwitchBlock paired with this clayball, picking
 * one of three sprite ids based on def[0x12] (0x5A/0x5B/default → the
 * three switch-clayball flavours), wires it into the sorted render list,
 * and back-links it to the parent clayball at 0x11C / 0x100. */
INCLUDE_ASM("asm/nonmatchings/clayball", ClayballSpawnSwitchBlock);

/* Constructor for the "bonus" clayball (entity type 093). Despite the
 * name in symbol_addrs, the engine wires it up with the ShrineyGuard
 * sound-emitting callbacks (ShrineyGuardEventWithSound +
 * ShrineyGuardSoundUpdateTick) — so the bonus clayball is the
 * audio-driven rolling variant that uses two SPU voices for its rolling
 * and impact sounds. */
INCLUDE_ASM("asm/nonmatchings/clayball", InitBonusClayballEntity);

/* Per-tick callback for the sound-emitting ("ShrineyGuard") clayball:
 * keeps the 3D pan of the rolling-loop voice at +0x12C in sync with the
 * entity's screen pos, fires a one-shot impact sound on the +0x126 trigger
 * (and pings the player via event 0x1014), decrements the windup counter
 * at +0x124 to cross-fade into a shorter loop, then on expiry stops both
 * voices, plays the end SFX, sends event 0x1015 to the player, and finally
 * runs the generic sprite tick. */
INCLUDE_ASM("asm/nonmatchings/clayball", ShrineyGuardSoundUpdateTick);

/* Event handler twin of CircularPlatformEventHandler for the
 * sound-emitting clayball: same lazy SwitchBlock creation logic on the
 * tick events, just kept separate so the bonus/audio variant can carry
 * its own per-event behaviour without disturbing the platform path. */
INCLUDE_ASM("asm/nonmatchings/clayball", ShrineyGuardEventWithSound);

extern u8 SOUND_CLAYBALL_SHUTDOWN_VTABLE[] asm("D_80011628");
extern void FreeEntityNoTeardown_80059674(Entity *entity, s32 size);

/* Destructor for the sound-emitting clayball: silences the two managed
 * SPU voices (rolling loop at +0x128, impact/short loop at +0x12C),
 * marks both voice handles as -1, swaps back to the teardown vtable,
 * runs the generic destroy, and frees from the BLB heap when requested. */
void ShrineyGuardDestroyWithSoundCleanup(SoundClayballEntity *entity, s32 flags) {
    entity->sprite.base.collisionVtable = SOUND_CLAYBALL_SHUTDOWN_VTABLE;
    StopSPUVoice(entity->rollingVoice);
    entity->rollingVoice = -1;
    StopSPUVoice(entity->impactVoice);
    entity->impactVoice = -1;
    entity->sprite.base.collisionVtable = CLAYBALL_TEARDOWN_VTABLE;
    DestroyEntityAndFreeMemory(&entity->sprite, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

/* Generic destructor for the clayball entity family: restores the
 * teardown vtable (CLAYBALL_TEARDOWN_VTABLE), runs DestroyEntityAndFreeMemory, and
 * releases the entity back to the BLB heap when bit 0 of flags is set.
 * The next five copies are byte-identical duplicates of this body — the
 * compiler emitted one per clayball-variant init site because each one
 * referenced this destructor as a separate static callback. */
void EntityDestroyCallback_Vt800116E8(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = CLAYBALL_TEARDOWN_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

/* Duplicate of EntityDestroyCallback_Vt800116E8 (clayball-variant copy). */
void EntityDestroyCallback_Vt800116E8_800594a0(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = CLAYBALL_TEARDOWN_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

/* Duplicate of EntityDestroyCallback_Vt800116E8 (clayball-variant copy). */
void EntityDestroyCallback_Vt800116E8_80059504(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = CLAYBALL_TEARDOWN_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

/* Duplicate of EntityDestroyCallback_Vt800116E8 (clayball-variant copy). */
void EntityDestroyCallback_Vt800116E8_80059568(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = CLAYBALL_TEARDOWN_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

/* Duplicate of EntityDestroyCallback_Vt800116E8 (clayball-variant copy). */
void EntityDestroyCallback_Vt800116E8_800595cc(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = CLAYBALL_TEARDOWN_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

/* No-op callback used as a null/placeholder slot in a clayball vtable
 * (e.g. an unused movement or pre-tick hook). */
void func_80059630(void) {
}

/* No-op callback used as a null/placeholder slot in a clayball vtable
 * (paired with func_80059630 — likely the matching event/post-tick stub). */
void func_80059638(void) {
}

/* Lightweight destructor variant used by the simpler 0x1C-byte clayball
 * helper entity: swaps in vtable D_80011708 and conditionally frees via
 * the no-teardown path. The naming "Simple11" reflects the D_8001170*8
 * vtable it installs; likely should be EntityDestroy_SmallClayballHelper. */
void EntityDestructor_Simple11(Entity *entity, s32 flags) {
    entity->collisionVtable = SMALL_CLAYBALL_HELPER_VTABLE;
    if (flags & 1) {
        FreeEntityNoTeardown_80059674(entity, 0x1C);
    }
}

/* Bare heap release used by EntityDestructor_Simple11 — frees `entity`
 * from g_pBlbHeapBase without running any per-field teardown. The `size`
 * parameter is accepted but ignored (caller passes 0x1C). */
void FreeEntityNoTeardown_80059674(Entity *entity, s32 size) {
    FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
}
