# Entity Event IDs

Event ids passed to entity event-FSM slots (`eventMarker`/`eventCallback` @ +0x08/+0x0C, and the
extended slot @ +0x104/+0x108). Dispatch uses the standard tagged-union encoding (see
`include/Game/entity.h`). Handlers receive `(target, eventId, arg, source)` (arg counts vary).

The mirror of this table lives in `include/Game/entity_events.h` (`EntityEventId` enum) — keep the
two in sync.

CLEAN-ROOM: ids are behavioral observations; names are ours.

## Key pattern: event ids are a *receiver-defined* message vocabulary

The same numeric id means **different things to different entity types**.
`DispatchEventToCollidingEntity` @ 0x800226F8 broadcasts `(id, arg, source)` to every colliding
entity; each entity's handler `switch`es on the id and does whatever that type wants. So:

- An id has no single global meaning — only a *dominant* meaning plus type-specific overloads.
- The "Meaning" column below is the dominant interpretation; "Overloads" lists the divergent ones.
- A flat semantic enum (`EntityEventId`) is therefore a convenience, not ground truth. Prefer the
  dominant name and add a comment when wiring an overloaded id at a site that uses a minority
  meaning.

## Lifecycle ids (1–3) — sent by the engine core

| ID | Meaning | Verified at |
|---|---|---|
| 1 | Spawn/init (create linked child) | `CircularPlatformEventHandler` @ 0x800586B4 |
| 2 | Tick / post-tick wakeup (drains the deferred callback queue) | many; e.g. `EntityEventHandlerCountdownToWalkWithSprite` @ 0x8003F888 |
| 3 | Game-progression notification (sent to **GameState**'s event slot @ +0x08/+0x0C) | `TriggerCollectible100CTickCallback` @ 0x8002F03C, `InvokeGameStateCallback` @ 0x80034B2C |

## Inter-entity messages (0x10xx block)

All entries verified against decompiled handler/dispatch sites (2026-06 re-document campaign;
catalog expanded 2026-06-15 from the full CppExporter decompile).

| ID | Dominant meaning | Receiver-specific overloads |
|---|---|---|
| 0x1000 | Damage/hit (generic — enemy collision, hazard, projectile) | — (very consistent) |
| 0x1001 | Hit / set-ready flag (+0x106 = 1) | state-transition variants |
| 0x1002 | **Projectile/kill hit** (spawn burst particle, deflect-pending) | token query/sync (returns entity ptr, clears on match) |
| 0x1004 | Rider attach/notify (forwarded to linked entity @ +0x124; stores rider) | path-follower: switch tick to speed-up |
| 0x1005 | Rider detach (clears stored rider @ +0x128) | path-follower: switch tick to speed-down |
| 0x1006 | "Child destroyed" / "path complete" notification to parent | velocity push (adds arg to velocityY) |
| 0x1007 | Enemy hit with bounds check → `CheckPlayerHitByEnemy` | — |
| 0x1008 | Token claim / link request (store token if slot @ +0x108 free; recolor switch green) | queue projectile attack (stores source in +1.eventMarker) |
| 0x1009 | **Destroy/death notification** ("I am being destroyed") | token release (clear on match); "mark" (+0x108 = 1); queue-process trigger |
| 0x100A | Player platform-bounce notify (increments a counter while in JumpFromPlatform/BounceJump) | — (1 site, low confidence) |
| 0x100B | Identity query — handler returns its own entity pointer ("are you a switch?") | — (consistent) |
| 0x100C | **Enter shrink/scale zone** → `PlayerEnterShrinkZone` (the sender is a scale-trigger "collectible") | — |
| 0x100D | Exit shrink zone / reset scale → `PlayerExitShrinkZone` | "reset scale" message to player |
| 0x100F | Checkpoint teleport (player: snap to sender pos +0x20 Y, enter CheckpointExit) | multipart: show (prim+0xA = 1); platform: attach/ride-to-entity |
| 0x1010 | Level-exit teleporter (player → LevelExitTeleporter, if not invincible/exiting) | multipart: hide (prim+0xA = 0); player: "try-idle" query |
| 0x1011 | Recolor indicator (yellow variant, RGB 40/A0/00) | — |
| 0x1013 | HUD pickup-notify / award (`PICKUP_NOTIFY`; arg = subtype) | — |
| 0x1014 | Named-trigger signal fire (carries signal id; clayball on-demand spawn) | pinged to player by sound-emitter tick |
| 0x1015 | Sound-emitter end notification (sent to player) | — |
| 0x1016 | Special activation trigger / set state flag (+0x122 = 1, or +0x101 = 1) | collected-flag: clear collisionMask, return entity |
| 0x1017 | Query "can interact / teleport?" → returns true if enabled, not invincible, not exiting | — (consistent) |
| 0x1018 | World freeze broadcast (Universe Enema activation) | — |

### Verified handler sites (representative)

- `PlayerEvent_ZoneTriggerHandler` — 0x100C/0x100D/0x100F/0x1010/0x1017 (the player zone family)
- `MultiPartEntityMessageHandler` @ 0x800389ac — 0x1009 (mark) / 0x100F (show) / 0x1010 (hide)
- `CircularPlatformEventHandler` @ 0x800586B4 — 1, 0x1004, 0x1005, 0x100B, 0x1008, 0x1011, 0x1016
- `PlatformEntityOnCollision` @ 0x80071560 — 0x1000 (damage), 0x100F (attach/ride)
- `EntityEventHandlerCountdownToWalkWithSprite` @ 0x8003F888 — 2, 0x1001, 0x1002, 0x1008
- `AwardSwirlyQsAndHamsters` @ 0x80026954 — 0x1013

## Token protocol (subset of the above)

Events 0x1002/0x1008/0x1009 implement a claim/release handshake on a per-entity token word
(observed at +0x108): 0x1008 claims (stores token if slot is 0, returns 1), 0x1009 releases
(clears on match), 0x1002 queries (returns the entity pointer, clears on match). Used to serialize
interactions (e.g. one rider per platform). Note that 0x1002 and 0x1009 are **overloaded** — at
most sites they are projectile-hit and death-notify respectively, not the token operations.

## Corrections applied 2026-06-15 (vs the pre-expansion table)

- **0x100C was "collectible pickup" → now "enter shrink/scale zone."** The player's authoritative
  handler maps 0x100C→`PlayerEnterShrinkZone`. The "collectible" that sends it is a scale-trigger;
  sender-name vs receiver-action were two views of one event.
- **0x100D added** (exit shrink zone) — the missing partner of 0x100C.
- **0x1007, 0x100A, 0x1010, 0x1017 added** — were absent from the prior table. 0x1010 in particular
  is the id that `MultiPartEntityMessageHandler` (`effects.c`) leaves numeric.
- 0x1002 / 0x1009 dominant meanings clarified (projectile-hit / death-notify); the token-protocol
  names cover only a minority of sites.

## NOT event ids: packed-ASCII eventArg sentinels

Several constants seen near event dispatch are the event **argument** (`arg`/`param3`), not the id:
`0x1020AC7D`, `0x105080E`, `0x1084280`, `0x10D86282`, `0xB0C10420`, `0x4260C8B5`, `0x8B603980`,
`0x90810000`, … Despite plate comments calling them "4-letter packed ASCII", they are **not** ASCII
— they are opaque 32-bit asset hashes / discriminator tokens (sprite/sound/particle keys, or magic
spawn-variant selectors). See `docs/reference/asset-hash-ids.md` for the full debunk and analysis.

## Notes

- Event 2 doubles as the per-frame tick for entities whose tick flows through the event slot.
- Event 3 is the only id observed sent to GameState rather than an entity.
- The 0x10xx block is reserved for inter-entity messages; 1–3 for engine lifecycle.
- Source of the 2026-06-15 expansion: full Ghidra CppExporter decompile (`/tmp/sm_export.c`), all
  `== 0x10xx` / `case 0x10xx` sites cross-referenced with handler plate comments.
