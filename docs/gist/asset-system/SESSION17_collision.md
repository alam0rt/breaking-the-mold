---
title: "Session 17 — Collision system + sounds + HUD counters"
category: gist/asset-system
tags: [gist, asset, system, collision]
---

# Session 17 — Collision system + sounds + HUD counters

## Collision-triggered sounds (mapped)
| sound id | name | trigger |
|---|---|---|
| 0x50F08207 | **FX_KLAY_HIT_HEAD** (verified) | collision message 'e' (0x65 head-bonk) in `PlayerCallback_800650c4` @ line 25650 |
| 0x00248E52 | **FX_KLAY_RUN_FAST** (verified) | `PlayerProcessTileCollision`, tile values 2-7 (solid/walk surface) while in PlayerState_Jump |
| 0x7003474C | (uncracked) | `PlayerProcessTileCollision`, **tile-zone trigger** tile-values 0x32-0x3B, gated by a per-zone `g_pPlayerState` flag. ROLE: tile-zone trigger sound (NOT a footstep — earlier guess corrected). Needs 12+ char name. |

## Tile-attribute collision values (Asset 500 tile map) confirmed in code
`PlayerProcessTileCollision` switch handles tile values: 2-7 (surface/walk -> RUN_FAST),
0x2a (death), 0x3d-0x41 (wind), 0x51/0x52/0x65/0x66/0x79/0x7a (spawn/trigger zones),
0x32-0x3B (per-zone trigger -> 0x7003474C). Matches BLB doc (0=passable,2=solid,0x2A=death,
0x3D-41=wind,0x51-7A=spawn).

## Player death / debris emitter — FUN_8005ad54
On death: allocates a debris sprite (**0xCA1B20CB**, or **0x3D348056** = ExplosionDebris by
flag +0x1b1), adds to render list, and **decrements g_pPlayerState[0x13]** => `[0x13]` = LIVES.
NOTE/correction: 0xCA1B20CB is a shared **death-burst/debris** sprite (used here AND at line
35487 by CreateSoarPlayerEntity) — NOT specifically a Soar-player sprite. Role updated.

## HUD counters identified (ties to STATUSNUMBERS, Session 4)
`CreateMenuEntities` reads three player-state bytes into the digit-display sprites
(0x00E2F188 STATUSNUMBERS) at the mapped x-positions:
- **g_pPlayerState[0x11]** = HUD counter (clayballs or score)
- **g_pPlayerState[0x12]** = HUD counter
- **g_pPlayerState[0x13]** = LIVES (decremented on death by FUN_8005ad54; checked ==0 for
  game-over at line 21143)

## Net
- 3 collision sounds mapped (2 verified-confirmed-in-context, 1 role-corrected: 0x7003474C =
  tile-zone trigger).
- Tile-attribute value -> behavior/sound switch decoded; matches BLB doc.
- 0xCA1B20CB reclassified: shared death-debris burst (not Soar-player-specific).
- HUD counter fields identified: [0x13]=lives, [0x11]/[0x12]=score/clayball counters.

## Files
- (findings documented here; no new JSON artifact this session)
