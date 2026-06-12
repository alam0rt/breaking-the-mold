# Player State FSM

**Date:** 2026-06-12  
**Basis:** Ghidra `.sdata` state-slot scan, `EntitySetState @ 0x8001EAAC` xrefs, and current inferred `PlayerEntity` / `PlayerState` structs.  
**Clean-room caveat:** State names are inferred labels from Ghidra. They are not original source names, and some labels describe observed behavior only partially.

## Summary

The normal platformer player uses the same 8-byte `FSMStateSlot` pattern described in `../fsm-callback-patterns.md`. Its primary contiguous state table begins at `0x800A5D20` and contains 53 direct slots before the late-player/cleanup run that begins at `0x800A5ECC`.

The player is not driven by one enum field. The active behavior is a callback installed through `EntitySetState`, while many state-init functions install secondary callbacks with `EntitySetCallback`. Input, physics, animation, collision, and powerup effects are layered through callbacks and fields in `PlayerEntity`.

## Core structures involved

| Struct / region | Important fields | Role |
|-----------------|------------------|------|
| `Entity+0x00/+0x04` | `tickMarker`, `tickCallback` | Per-frame dispatch through `EntityTickLoop`. |
| `Entity+0x08/+0x0C` | `eventMarker`, `eventCallback` | Collision/trigger/damage event handler. |
| `SpriteEntity+0x98..0xAC` | `queuedState*`, `activeState*`, `exitCallback*` pairs | Used by `EntitySetState`, `EntityProcessCallbackQueue`, and `EntitySetCallback`; `+0xA8/+0xAC` is an exit/finalizer hook, not the primary state pair. |
| `PlayerEntity+0x100` | `pInput` | Active controller input pointer. |
| `PlayerEntity+0x104/+0x108` | `inputStateMarker`, `inputStateCallback` | Player-specific input/secondary state slot. |
| `PlayerEntity+0x110/+0x114` | fixed-point Y/X velocity | Main platformer movement accumulators. |
| `PlayerState @ 0x8009B1D8` | inventory/powerup/boss fields | Persistent state pointed to by `g_pPlayerState @ 0x800A597C`. |

## Primary player state slots

These are direct `[0xFFFF0000, handler]` slots. The table below groups them by observed behavior rather than claiming original enum categories.

### Baseline / idle / movement

| Slot | Handler | Notes |
|------|---------|-------|
| `0x800A5D20` | `PlayerStateInit_Idle @ 0x80066CE0` | Idle entry / baseline state init. |
| `0x800A5DA0` | `PlayerState_PlayRandomIdleAnimation @ 0x800670BC` | Idle animation variation. |
| `0x800A5DA8` | `PlayerState_IdleLookAround @ 0x80066F60` | Idle look-around variant. |
| `0x800A5DD0` | `PlayerState_Running @ 0x8006762C` | Running / primary grounded motion. |
| `0x800A5E10` | `PlayerState_WalkingRight @ 0x8006736C` | Walking right. |
| `0x800A5E18` | `PlayerState_WalkingLeft @ 0x800674CC` | Walking left. |
| `0x800A5E20` | `PlayerStateInit_Walking @ 0x80067FC8` | Walking state initializer. |
| `0x800A5E60` | `PlayerStateInit_ResumeWalking @ 0x80068130` | Return to walking after another state. |
| `0x800A5E68` | `PlayerState_SpecialIdleAnim @ 0x80069754` | Special idle animation branch. |
| `0x800A5EEC` | `PlayerState_StandingIdle @ 0x8006888C` | Standing idle / neutral state. |

### Jumping / falling / landing

| Slot | Handler | Notes |
|------|---------|-------|
| `0x800A5DC8` | `PlayerStateInit_JumpFromPlatform @ 0x80069200` | Jump from platform/ride state. |
| `0x800A5E08` | `PlayerState_JumpApex @ 0x80067798` | Apex/airborne transition. |
| `0x800A5E30` | `PlayerStateInit_FallFromLedge @ 0x8006A584` | Fall entry. |
| `0x800A5E48` | `PlayerEnterLandingState @ 0x80067A34` | Landing transition. |
| `0x800A5E50` | `PlayerState_Jump @ 0x80067E28` | Jump state. |
| `0x800A5E58` | `PlayerState_Falling @ 0x800678D4` | Falling state. |
| `0x800A5E90` | `PlayerStateInit_Falling @ 0x8006A940` | Falling initializer. |
| `0x800A5EDC` | `PlayerState_LandingFromRide @ 0x80068D38` | Landing after platform/ride motion. |

### Bounce / collision response

| Slot | Handler | Notes |
|------|---------|-------|
| `0x800A5D30` | `PlayerStateCallback_2 @ 0x8006864C` | Collision/physics branch; name still broad. |
| `0x800A5D38` | `PlayerState_SetupBounceRight @ 0x8006AE0C` | Selected by `PlayerProcessBounceCollision`. |
| `0x800A5D40` | `PlayerState_SetupBounceUp @ 0x8006AE58` | Bounce up setup. |
| `0x800A5D48` | `PlayerState_SetupBounceUpWithVelocity @ 0x8006AE94` | Bounce up with extra velocity. |
| `0x800A5D50` | `PlayerState_SetupBounceDown @ 0x8006ADBC` | Bounce down setup. |
| `0x800A5D58` | `PlayerState_QuickTurn @ 0x8006AEDC` | Quick-turn/collision response. |
| `0x800A5D60` | `PlayerState_BounceLaunch @ 0x8006AD70` | Launching bounce. |
| `0x800A5D68` | `PlayerBounceCallback_Primary @ 0x8006AD34` | Primary bouncy-surface callback. |
| `0x800A5D70` | `PlayerState_SetupBounceDownFast @ 0x8006AF28` | Alternate bounce down / fast response. |
| `0x800A5E00` | `PlayerStateInit_BounceFromEnemy @ 0x80068F80` | Bounce after enemy interaction. |
| `0x800A5F14` | `PlayerState_ClearBounceFlag @ 0x800691D8` | Bounce flag cleanup. |
| `0x800A5F1C` | `PlayerState_ClearBounceAndAirFlag @ 0x800691E8` | Bounce + airborne cleanup. |

### Death / respawn / damage

| Slot | Handler | Notes |
|------|---------|-------|
| `0x800A5D78` | `PlayerStateInit_RespawnAtCheckpoint @ 0x8006C95C` | Respawn/checkpoint entry. |
| `0x800A5D80` | `PlayerState_Death @ 0x8006A0B8` | Death state. |
| `0x800A5D98` | `PlayerState_DeathStart @ 0x80069EF4` | Death start / setup. |
| `0x800A5EA8` | `PlayerStateInit_HeavyDamageLaunch @ 0x8006B974` | Heavy damage launch. |
| `0x800A5EB0` | `PlayerStateInit_DamageKnockback @ 0x8006B3F8` | Damage knockback init. |
| `0x800A5EB8` | `PlayerState_DamageKnockback @ 0x8006B4F4` | Damage knockback active. |
| `0x800A5EF4` | `PlayerStateInit_DamageRecoilFall @ 0x8006BC04` | Damage recoil into fall. |
| `0x800A5F34` | `PlayerDestroyVoiceCallback @ 0x8006B634` | Sound/voice cleanup callback. |

### Teleport / checkpoint / level exit

| Slot | Handler | Notes |
|------|---------|-------|
| `0x800A5DB8` | `PlayerState_LevelExitTeleporter @ 0x8006A214` | Level-exit teleporter. |
| `0x800A5DC0` | `PlayerState_CheckpointExit @ 0x8006A3F8` | Checkpoint exit transition. |
| `0x800A5DE0` | `PlayerStateInit_CheckpointRestore @ 0x8006BF2C` | Restore checkpoint data. |
| `0x800A5DF0` | `PlayerStateInit_CheckpointEntry @ 0x8006C3AC` | Checkpoint entry. |
| `0x800A5DF8` | `PlayerStateInit_TeleportIdleOnPlatform @ 0x800698B8` | Teleport to idle/platform state. |
| `0x800A5E28` | `PlayerStateInit_TeleportFallVariant @ 0x8006A6E4` | Teleport/fall variant. |
| `0x800A5E38` | `PlayerStateInit_TeleportWalking @ 0x80069A3C` | Teleport into walking. |
| `0x800A5E88` | `PlayerStateInit_TeleportFalling @ 0x80069BBC` | Teleport into falling. |
| `0x800A5E98` | `PlayerStateInit_TeleportEntry @ 0x8006ABF8` | Teleport entry setup. |
| `0x800A5EA0` | `PlayerStateInit_TeleportLanding @ 0x8006AA9C` | Teleport landing. |
| `0x800A5F24` | `PlayerState_TransitionToLevelExit @ 0x8006B2DC` | Begin level-exit transition. |
| `0x800A5F2C` | `PlayerState_LevelExitComplete @ 0x80069EA4` | Level-exit completion. |
| `0x800A5F44` | `PlayerState_CheckpointRestoreComplete @ 0x8006C0AC` | Restore completion callback. |
| `0x800A5F54` | `PlayerState_ClearCheckpointAndSetInvincible @ 0x8006C7CC` | Checkpoint cleanup + invincibility setup. |

### Powerup / special action / misc

| Slot | Handler | Notes |
|------|---------|-------|
| `0x800A5D88` | `PlayerStateInit_CrouchSlide @ 0x800682A4` | Crouch/slide entry. |
| `0x800A5D90` | `HamsterSpriteCallback @ 0x8006D910` | Hamster shield visual callback. |
| `0x800A5DB0` | `PlayerState_PickupItem @ 0x80068B48` | Pickup animation/state. |
| `0x800A5DD8` | `PlayerStateInit_ThrowProjectile @ 0x8006BDD4` | Projectile throw / powerup action. |
| `0x800A5DE8` | `UniverseEnemaActivate @ 0x8006C0D8` | Universe Enema powerup activation. |
| `0x800A5E40` | `PlayerState_SpecialMove2 @ 0x80067CF0` | Special movement/action branch. |
| `0x800A5E70` | `PlayerStateInit_ExitShrinkWithRestore @ 0x8006D048` | Leave shrink mode and restore. |
| `0x800A5E78` | `PlayerStateInit_ShrinkAndFall @ 0x8006D548` | Shrink/fall transition. |
| `0x800A5E80` | `PlayerStateInit_GrowFromShrink @ 0x8006D3C8` | Grow/restore transition. |
| `0x800A5EC0` | `PlayerState_CrouchOrClimbTransition @ 0x8006B658` | Crouch/climb transition. |
| `0x800A5ECC` | `PlayerState_StartSwimming @ 0x8006D724` | Swimming start. |
| `0x800A5ED4` | `PlayerStateInit_ClimbWithMirror @ 0x8006B7F8` | Climb/mirror transition. |
| `0x800A5EE4` | `PlayerState_SpecialMove1 @ 0x80068A0C` | Special movement/action branch. |
| `0x800A5EFC` | `PlayerState_UpdateTPageAndHideHalo @ 0x800672F4` | Render/halo side-effect callback. |
| `0x800A5F04` | `PlayerState_ClearInAirFlag @ 0x80069D3C` | Clears airborne state flag. |
| `0x800A5F0C` | `PlayerState_StopSoundAndClear @ 0x80067F90` | Sound cleanup + state cleanup. |
| `0x800A5F3C` | `PlayerState_RemoveAttachedEntity @ 0x8006BD80` | Removes attached helper entity. |
| `0x800A5F4C` | `UniverseEnemaKillAllEnemies @ 0x8006C278` | Universe Enema effect. |
| `0x800A5F5C` | `PlayerCallback_RestoreNormalHitbox @ 0x8006D6C8` | Restore normal hitbox after mode/powerup. |

## Transition patterns observed

1. **Input-driven transitions:** grounded/airborne state handlers inspect `PlayerEntity.pInput` and choose a new state slot with `EntitySetState`.
2. **Collision-byte transitions:** `PlayerProcessBounceCollision @ 0x8005A630` maps tile/collision bytes to bounce state slots.
3. **Animation/timer finalizers:** many state init functions call `EntitySetCallback`, installing an exit/finalizer hook in `SpriteEntity+0xA8/+0xAC`. That hook runs before being replaced, before `EntitySetState` installs a new state, and before a callback sequence step. Player examples clear airborne/bounce flags, stop sounds, restore hitboxes, remove attached helper entities, or complete checkpoint/powerup effects.
4. **Event-handler transitions:** trigger zones, hazards, pickups, and checkpoints call `EntitySetState` from event handlers rather than directly changing an enum.
5. **Powerup overlay transitions:** shrink, hamster, halo, Universe Enema, and projectile actions use the same player state mechanism but also modify persistent `PlayerState` fields and child entities.

## What this replaces in older notes

Older player docs sometimes describe “4 main state callbacks” or model `PlayerEntity+0x104/+0x108` as a simple secondary state enum. Ghidra now shows a broader callback-slot architecture:

- `+0x104/+0x108` is the player-specific input/secondary callback slot.
- The normal player has more than 50 named state slots in `.sdata`.
- Transitions are callback installs, not enum assignments.
- Some table entries are cleanup/follow-up callbacks, not long-lived movement states.

## Follow-ups

- Build an edge graph by decompiling every `EntitySetState` caller in the `0x80058000-0x8006DFFF` range.
- Rename ambiguous handlers such as `PlayerStateCallback_2` after tracing their installed sub-callbacks.
- Build a focused `EntitySetCallback` table for the player range that separates flag cleanup, sound cleanup, hitbox restoration, and delayed powerup effects.
- Merge this table with runtime traces from `game_watcher.lua` to separate frequent gameplay states from rare cutscene/death/checkpoint states.
