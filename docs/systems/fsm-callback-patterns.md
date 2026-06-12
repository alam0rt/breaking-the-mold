# FSM / Callback Dispatch Patterns

**Date:** 2026-06-12  
**Basis:** Ghidra function/data scans of `SLES_010.90`, plus the current inferred structs in `include/Game/`.  
**Clean-room caveat:** Names are inferred labels. The binary does not preserve original C type names, enum names, or source paths. Treat every state name as a working hypothesis tied to the function and data evidence below.

## Executive summary

The game uses a small number of callback-slot idioms everywhere instead of one central enum-based state machine. The dominant pattern is an 8-byte tagged callback slot:

```c
typedef struct {
    s32 marker;  /* usually 0xFFFF0000 */
    void *fn;    /* callback or table-base offset, depending on marker */
} FSMStateSlot;
```

Ghidra scan results:

- 354 initialized `0xFFFF0000` words found in `0x80095000-0x800A7000`.
- 347 of those are immediately followed by a code-range pointer.
- 345 are followed by a pointer that Ghidra resolves to a function entry.
- 21 contiguous runs of two or more such slots were found, strongly indicating packed state/init tables.
- `EntitySetState @ 0x8001EAAC` has 189 references from 146 unique caller functions.
- `EntitySetCallback @ 0x8001EC18` has 40 references from 39 unique caller functions.

Not every `[0xFFFF0000, function]` pair is necessarily a runtime state transition. The entity type init table also has this shape. Interpret each pair by its caller and table context.

## Marker encoding

The marker word is read as two signed 16-bit halves:

| Marker high half | Meaning | Dispatch effect |
|------------------|---------|-----------------|
| `0` | Disabled / no callback | Slot is skipped. |
| `< 0` | Direct callback | Call `fn(entity + marker_low)`. Common marker `0xFFFF0000` means direct call with offset `0`. |
| `> 0` | Indexed callback table | Treat the `fn` word as an entity-relative table pointer/offset, select an 8-byte table entry, add that entry's offset to `marker_low`, then call the selected function. |

This same marker logic appears in the entity tick loop, collision/event dispatch, movement-transform callbacks, and GameState mode dispatch. The common sentinel `0xFFFF0000` should be read as “direct callback, base object pointer”.

## Main dispatch layers

| Layer | Slot shape | Location / evidence | Purpose |
|-------|------------|---------------------|---------|
| Game mode | `[marker, fn]` | `GameState+0x00/+0x04`; `GameModeCallback @ 0x8007E654` is installed by `InitGameState`, `InitializeAndLoadLevel`, and `SetupAndStartLevel`. | One top-level mode callback per frame. |
| Entity tick | `[marker, fn]` | `Entity+0x00/+0x04`; called by `EntityTickLoop @ 0x80020E1C`. | Per-frame actor update. |
| Entity event | `[marker, fn]` | `Entity+0x08/+0x0C`; called through collision/event paths such as `DispatchEventToCollidingEntity @ 0x800226F8`. | Object-to-object messages, trigger events, damage, pickups. |
| Entity render | `[marker, fn]` | `Entity+0x1C/+0x20`. | Render/update bridge for sprite contexts and layer render contexts. |
| Movement transform X/Y | `[marker, fn]` | `Entity+0x24/+0x28` and `Entity+0x2C/+0x30`; verified by `EntityBroadcastPointCollision @ 0x8001B72C`. | Transform world coordinates before collision/render; used for platform-riding and scale/offset behavior. |
| Animated state transition | `SpriteEntity+0x94..0xAC` sequence/queued/active/exit pairs. | `StartAnimationSequence @ 0x8001E790`, `StepAnimationSequence @ 0x8001E7B8`, `EntityProcessCallbackQueue @ 0x8001E928`, `EntitySetState @ 0x8001EAAC`, `EntitySetCallback @ 0x8001EC18`. | Queues/replaces animated entity states, runs animation callback sequences, and runs exit/finalizer hooks before replacement. |
| Vtable slot | `EntityCallbackSlot {s16 offset; s16 pad; fn}` | Entity/menu/basic vtables. | Destroy/tick/texture/menu callback slots with explicit object-pointer adjustment. |
| Dense pointer array | Raw function pointer array | `PlayerCallbackTable+0x40` bounce handlers. | Indexed dispatch without marker words. |

## Per-frame flow

```text
main
  └─ GameState mode slot
       └─ GameModeCallback
            ├─ spawn/load/pause/death/checkpoint logic
            ├─ EntityTickLoop
            │    └─ each Entity tick slot
            │         ├─ may call EntitySetState / EntitySetCallback
            │         ├─ may dispatch collision/event slots
            │         └─ may update animation/render fields
            ├─ collision and trigger-zone checks
            ├─ render list / layer callbacks
            └─ audio/CD/SPU updates
```

The important point is that “state” is usually the current function pointer, not a single enum field. Many state transitions install another function, and that function may install the next one after an animation frame, timer, collision byte, trigger-zone event, or sound/event completion.

## SpriteEntity animated-state queue

The previously ambiguous `SpriteEntity+0x98..0xAC` region is now resolved enough to name conservatively. Ghidra datatype `/Skullmonkeys/SpriteEntity` and [include/Game/entity.h](../../include/Game/entity.h) use these field labels:

| Offset | Field | Evidence / role |
|--------|-------|-----------------|
| `+0x94` | `sequenceTable` | Pointer to a table of 8-byte `[marker, fn]` entries. Written by `StartAnimationSequence @ 0x8001E790`. |
| `+0x98/+0x9C` | `queuedStateMarker`, `queuedStateCallback` | Pending next state. `EntityProcessCallbackQueue @ 0x8001E928` copies this pair into the active pair, clears the queued pair, then dispatches it. |
| `+0xA0/+0xA4` | `activeStateMarker`, `activeStateCallback` | Current state/sequence entry being dispatched. Sequence stepping copies the selected table entry here before calling it. |
| `+0xA8/+0xAC` | `exitCallbackMarker`, `exitCallback` | Exit/finalizer hook installed by `EntitySetCallback @ 0x8001EC18`. It is dispatched and cleared before replacement, before `EntitySetState` installs a new state, and before stepping a sequence entry. |
| `+0xE2/+0xE4` | `sequenceStep`, `sequenceLength` | Current index and total length for `sequenceTable`. |

`EntitySetCallback` is therefore not a generic “secondary state setter”. Its narrow behavior is:

1. If an `exitCallback` hook is already present at `+0xA8/+0xAC`, dispatch it through the normal marker logic.
2. Store the new marker/callback into `+0xA8/+0xAC`.

The caller set matches that interpretation: sound/voice teardown (`HazardStopSound`, `JoeHeadJoeClearVoice`, `PlatformState_StopSound`), player flag cleanup (`PlayerState_ClearInAirFlag`, `PlayerState_ClearBounceFlag`, `PlayerState_ClearBounceAndAirFlag`), hitbox restoration (`PlayerCallback_RestoreNormalHitbox`), attached-entity cleanup, checkpoint restore completion, Universe Enema delayed effect, and SOAR/platform input re-enable hooks.

`EntitySetState` is the heavier primitive: it clears the active state pair, runs/clears the existing exit/finalizer hook, and only if that hook did not install another state does it clear sequence/queued fields, install the new active pair, and immediately dispatch the new state enter handler.

## Global slot-table inventory

Ghidra scan of initialized memory found these contiguous slot runs:

| Start | Slots | Likely role |
|-------|-------|-------------|
| `0x8009B55C` | 27 | Collectible/sparkle/animation sequence tables. |
| `0x8009B7A4` | 20 | Item reveal / special collectible state sequence table. |
| `0x8009D5F8` | 13 | Start of entity type init callback table. |
| `0x8009D680` | 39 | Entity type init callback table continuation. |
| `0x8009D7C0` | 16 | Entity type init callback table continuation. |
| `0x8009D870` | 42 | Entity type init callback table continuation. |
| `0x800A5988` | 3 | Platform ride states. |
| `0x800A59A8` | 7 | Decor state setters / animation starters. |
| `0x800A5A50` | 17 | Enemy/collectible animation and patrol states. |
| `0x800A5AE0` | 10 | Hide/show, platform, bounce-clay, teleporter states. |
| `0x800A5B60` | 34 | Glider/boss/hazard/Shriney/JoeHeadJoe state cluster. |
| `0x800A5C78` | 13 | Enemy/projectile/JoeHeadJoe ball state cluster. |
| `0x800A5CE8` | 4 | Clayball/Shriney guard indicator states. |
| `0x800A5D20` | 53 | Normal player state table. |
| `0x800A5ECC` | 22 | Late player states and player cleanup/transition callbacks. |
| `0x800A5F84` | 3 | FINN state init/death/timer states. |
| `0x800A5FA4` | 10 | Platform / SOAR / RUNN state cluster. |
| `0x800A5FFC` | 2 | RUNN idle/active states. |
| `0x800A601C` | 3 | FINN spawn/active/turn states. |

Several runs begin on 8-byte boundaries, but not all. The run at `0x800A5ECC` is 4-byte shifted relative to the previous run, so scans should not assume 8-byte alignment globally.

## Example state clusters

### Normal player states

The dense player state run begins at `0x800A5D20` and includes direct state slots such as:

| Slot | Handler |
|------|---------|
| `0x800A5D20` | `PlayerStateInit_Idle @ 0x80066CE0` |
| `0x800A5D80` | `PlayerState_Death @ 0x8006A0B8` |
| `0x800A5DD0` | `PlayerState_Running @ 0x8006762C` |
| `0x800A5E08` | `PlayerState_JumpApex @ 0x80067798` |
| `0x800A5E10` | `PlayerState_WalkingRight @ 0x8006736C` |
| `0x800A5E18` | `PlayerState_WalkingLeft @ 0x800674CC` |
| `0x800A5E50` | `PlayerState_Jump @ 0x80067E28` |
| `0x800A5E58` | `PlayerState_Falling @ 0x800678D4` |
| `0x800A5EB0` | `PlayerStateInit_DamageKnockback @ 0x8006B3F8` |
| `0x800A5EC0` | `PlayerState_CrouchOrClimbTransition @ 0x8006B658` |

Player state code uses both `EntitySetState` and `EntitySetCallback`. `EntitySetCallback` appears in many init/recovery states, suggesting a secondary or follow-up callback slot around the `+0xA8/+0xAC` transition region.

### Player bounce/collision states

`PlayerProcessBounceCollision @ 0x8005A630` is a clear state-selection example. It switches on a collision byte and chooses one of several global slots before calling `EntitySetState`:

| Collision byte | Slot | Handler |
|----------------|------|---------|
| `0xB5` / `-0x4B` | `0x800A5D38` | `PlayerState_SetupBounceRight @ 0x8006AE0C` |
| `0xB6` / `-0x4A` | `0x800A5D40` | `PlayerState_SetupBounceUp @ 0x8006AE58` |
| `0xB7` / `-0x49` | `0x800A5D48` | `PlayerState_SetupBounceUpWithVelocity @ 0x8006AE94` |
| `0xC9` / `-0x37` | `0x800A5D50` | `PlayerState_SetupBounceDown @ 0x8006ADBC` |
| `0xCB` / `-0x35` | `0x800A5D58` | `PlayerState_QuickTurn @ 0x8006AEDC` |
| `0xDD` / `-0x23` | `0x800A5D60` | `PlayerState_BounceLaunch @ 0x8006AD70` |
| `0xDE` / `-0x22` | `0x800A5D68` | `PlayerBounceCallback_Primary @ 0x8006AD34` |
| `0xDF` / `-0x21` | `0x800A5D70` | `PlayerState_SetupBounceDownFast @ 0x8006AF28` |

This is the preferred way to model many player “enums”: use named `FSMStateSlot` globals and let the slot names carry the inferred meaning.

### Boss and projectile states

Boss functions are also state-slot driven. The `0x800A5B60` and `0x800A5C78` runs include:

| Slot | Handler |
|------|---------|
| `0x800A5BE8` | `BossRandomAttackChoice @ 0x8004BA74` |
| `0x800A5BF8` | `ShrineyGuardDeathState @ 0x8004BFC4` |
| `0x800A5C18` | `ShrineyGuardIdleState @ 0x8004BC90` |
| `0x800A5C20` | `JoeHeadJoeSelectAttackPattern @ 0x8004CE74` |
| `0x800A5C28` | `JoeHeadJoeDeathAnimState @ 0x8004D790` |
| `0x800A5C88` | `ProjectileEnterActiveState @ 0x80051184` |
| `0x800A5C90` | `ProjectileState_HomingActive @ 0x80051D44` |
| `0x800A5CA8` | `ProjectileState_HomingMissileTrack @ 0x800521C8` |

Boss event handlers call `EntitySetState` in response to damage, animation, and projectile events. The boss HP storage and exact phase timers are separate from the slot mechanism and should be documented per boss.

### Vehicle / special movement states

The later `.sdata` state slots cover player variants:

| Slot | Handler |
|------|---------|
| `0x800A5F64` | `FinnSubState_SetIdleSpriteAndPause @ 0x8006E8F4` |
| `0x800A5F84` | `FinnStateInit_SetSpriteByAngle @ 0x8006FDF4` |
| `0x800A5F8C` | `FinnDeathExplosion @ 0x8006FE94` |
| `0x800A5FB4` | `SoarStateInit_BeginFlightMode @ 0x80072A98` |
| `0x800A5FBC` | `RunnEntityDamageEffect @ 0x80072DE0` |
| `0x800A5FFC` | `RunnSetIdleState @ 0x80073D8C` |
| `0x800A6004` | `RunnSetActiveState @ 0x80073EDC` |
| `0x800A601C` | `FinnState_Spawn @ 0x80074BF4` |
| `0x800A6024` | `FinnSetActiveState @ 0x80074DB0` |
| `0x800A602C` | `FinnSetTurnState @ 0x80074C84` |

These functions are close in address to the FINN/RUNN/SOAR code cluster, matching the high-level source architecture hypothesis.

## Callback-table families

### `FSMStateSlot`

Use this for state globals passed as the marker/function pair into `EntitySetState` or similar marker dispatchers. Common direct slots are initialized as `[0xFFFF0000, handler]`.

### `EntityCallbackSlot`

Use this for vtable entries that have explicit object-pointer adjustment:

```c
typedef struct {
    s16 entity_offset;
    s16 pad;
    void (*func)(void *adjusted_entity);
} EntityCallbackSlot;
```

These appear in `EntityCallbackTableBase`, `MenuCallbackTable`, `BasicEntityVtable`, and similar structures. They are not the same as `FSMStateSlot`, even though both are 8 bytes.

### Entity type init table

The table beginning at `0x8009D5F8` is an array of 8-byte entries mapping BLB entity type IDs to init functions, e.g. `EntityType000_003_004_PickupVariant_Init`, `EntityType001_BossEntity_Init`, and so on. Its first word often resembles the same `0xFFFF0000` sentinel, but table context matters: this is spawn-type dispatch, not a per-frame actor state.

### Dense pointer arrays

The player bounce table in `PlayerCallbackTable+0x40` is a dense array of raw function pointers. Do not model it as `FSMStateSlot` or `EntityCallbackSlot`.

## Practical decompilation guidance

1. When a function takes two arguments that decompile as `null_FFFF0000h_*` and `PTR_*`, check whether those two globals are adjacent. If yes, model them as one `FSMStateSlot` and pass `&slot.marker` / `slot.fn` only if needed for matching.
2. Prefer names like `fsmSlot_PlayerState_Running` or `stateSlot_PlayerState_Running` for global slot pairs. Avoid claiming the function name is an original enum value.
3. If a table entry is `{s16 offset, s16 pad, fn}`, model it as `EntityCallbackSlot`, not `FSMStateSlot`.
4. If a table entry is `{flags_or_marker, init_fn}` and is indexed by BLB entity type, keep it in the entity type table even if the first word is `0xFFFF0000`.
5. Do not assume all state-slot tables are 8-byte aligned. At least one confirmed run starts at `0x800A5ECC`.
6. Treat `EntitySetCallback` as an exit/finalizer hook installer for `SpriteEntity+0xA8/+0xAC`, not as the same operation as `EntitySetState`.

## Open follow-ups

- Rename the global `null_FFFF0000h_*` / `PTR_*` pairs in Ghidra as `FSMStateSlot` instances, starting with player and boss tables.
- Produce a player-state graph from all `EntitySetState` call sites in the `0x80058000-0x8006DFFF` range.
- Separate true actor state slots from spawn-type init entries in the `0x8009D5F8` entity type table.
- Update stale docs that describe entity state as enum-like instead of callback-slot-driven.