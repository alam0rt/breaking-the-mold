# Entity Event IDs

Event ids passed to entity event-FSM slots (`eventMarker`/`eventCallback` @ +0x08/+0x0C, and the
extended slot @ +0x104/+0x108). Dispatch uses the standard tagged-union encoding (see
`include/Game/entity.h`). Handlers receive `(target, eventId, arg, source)` (arg counts vary).

All entries below were verified against decompiled handler/dispatch sites during the 2026-06
re-document campaign. CLEAN-ROOM: ids are behavioral observations; names are ours.

| ID | Meaning | Verified at |
|---|---|---|
| 1 | Spawn/init (create linked child) | `CircularPlatformEventHandler` @ 0x800586B4 |
| 2 | Tick/countdown step | `EntityEventHandlerCountdownToWalkWithSprite` @ 0x8003F888, `EntityEventHandler_TokenReleaseAndQueueTick` @ 0x8004FD68 |
| 3 | Game-progression notification (sent to **GameState**'s event slot @ +0x08/+0x0C) | `TriggerCollectible100CTickCallback` @ 0x8002F03C, `JoeHeadJoeState_HideAndNotifyGameState` @ 0x80053FB8, `InvokeGameStateCallback` @ 0x80034B2C, `FINNCallback_DispatchToStateHandler` @ 0x8006E14C |
| 0x1000 | Damage/hit | `PlatformEntityOnCollision` @ 0x80071560 |
| 0x1001 | Set ready flag | `EntityEventHandlerCountdownToWalkWithSprite` @ 0x8003F888 |
| 0x1002 | Token query/sync (returns entity ptr) | same |
| 0x1004 | Rider attach/notify (forwarded to linked entity; stores rider) | `CircularPlatformEventHandler` @ 0x800586B4 |
| 0x1005 | Rider detach (clears stored rider) | same |
| 0x1008 | Token claim (stores token if slot free; recolors switch indicator green) | `EntityEventHandlerCountdownToWalkWithSprite`, `CircularPlatformEventHandler` |
| 0x1009 | Token release (clears token on match) | `EntityEventHandler_TokenReleaseAndQueueTick` @ 0x8004FD68 |
| 0x100B | Identity query (handler returns its own entity pointer) | `CircularPlatformEventHandler` |
| 0x100C | Collectible pickup (sent to the player with the collectible as arg) | `TriggerCollectible100CTickCallback` @ 0x8002F03C |
| 0x100F | Attach/ride-to-entity (snap to source's transformed position) | `PlatformEntityOnCollision` @ 0x80071560 |
| 0x1011 | Recolor indicator (yellow variant) | `CircularPlatformEventHandler` |
| 0x1013 | HUD award notification (args 5, 0) | `AwardSwirlyQsAndHamsters` @ 0x80026954 |
| 0x1016 | Set state flag (+0x122) | `CircularPlatformEventHandler` |
| 0x1018 | World freeze broadcast (Universe Enema activation) | `UniverseEnemaActivate` @ 0x8006C0D8 |

## Token protocol

Events 0x1002/0x1008/0x1009 implement a claim/release handshake on a per-entity token word
(observed at +0x108 in the countdown/walk family): 0x1008 claims (stores the token if the slot
is 0, returns 1), 0x1009 releases (clears on match), 0x1002 queries (returns the entity pointer
and clears on match). Used to serialize interactions (e.g. one rider per platform).

## Notes

- Event 2 doubles as the per-frame tick for entities whose tick flows through the event slot.
- Event 3 is the only id observed sent to GameState rather than an entity; its handler payload
  varies by site (entity +0x20 or +0x24 word). Semantics: "notable game event" (pickup, boss
  phase) — pin down by cataloguing GameState event-slot installers.
- The 0x10xx block appears reserved for inter-entity messages; 1-3 for engine lifecycle.
