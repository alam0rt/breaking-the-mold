---
title: "Session 18 — Damage / death / respawn system"
category: gist/asset-system
tags: [gist, asset, system, damage, death]
---

# Session 18 — Damage / death / respawn system

## Death flow (mapped)
- **PlayerState_Death @ 0x80...32560** sets player sprite **0x1B301085** (the death animation
  — confirms role map) and resets state fields.
- **FUN_8005ad54** (death-burst emitter): allocates debris sprite **0xCA1B20CB** (or
  **0x3D348056** = ExplosionDebris by flag +0x1b1), adds to render list, decrements counter [0x13].
- **RespawnAfterDeath @ 0x8007...40736**: FntLoad, restores saved position (+0x13c), sets
  g_pPlayerState[0x18], calls **DecrementPlayerLives**, LoadBGColorFromTileHeader, reseeks level.
- Game-over check at line 21143: when lives counter == 0.

## DecrementPlayerLives @ 0x800262AC -> CORRECTS the HUD counter assignment
```
*(char*)(state+0x17)=0; *(char*)(state+0x1d)=0;
if (*(char*)(state+0x11) != 0) *(char*)(state+0x11) -= 1;   // <- LIVES = byte [0x11]
```
**CORRECTION to Session 17:** LIVES = `g_pPlayerState[0x11]` (not [0x13]). Session 17 inferred
[0x13] from FUN_8005ad54's decrement, but the canonical `DecrementPlayerLives` decrements
[0x11]. So:
- **g_pPlayerState[0x11] = LIVES** (DecrementPlayerLives; HUD digit row)
- g_pPlayerState[0x12] = HUD counter (clayballs or score)
- g_pPlayerState[0x13] = HUD counter (decremented by death-burst FUN_8005ad54; likely
  clayballs-to-1UP or a secondary tally) — exact semantic still open, but it's NOT lives.

## Damage sounds
- **FX_KLAY_HURT (0x00E49240)** — NOT referenced in decomp (played via data-table path), so no
  inline trigger; role known (player takes damage) but uncrackable from a call site.
- **FX_KLAY_HIT_HEAD (0x50F08207)** — head-bonk, confirmed (Session 17).
- **FX_KLAY_DIE_EXPLODE / FX_KLAY_DIE_FALL** (verified names in corpus) = death-by-explosion /
  death-by-pit, consistent with PlayerState_Death + FUN_8005ad54 debris.

## Shrink/scale damage state
`ScaleYByEntityScale` vtable (entity+0x18) handles entity scaling; g_pPlayerState[0x18] is the
player scale/mode flag (set in RespawnAfterDeath, checked at 17224). Consistent with the
"Klaymen shrinks when hit / grows with powerup" mechanic (GROW pickup = FX_PICKUP_GROW).

## Net
- Death/respawn flow mapped end-to-end (death anim 0x1B301085 -> debris burst 0xCA1B20CB ->
  DecrementPlayerLives -> respawn or game-over).
- **Corrected: LIVES = g_pPlayerState[0x11]** (Session 17 had [0x13]); [0x13] is a different tally.
- FX_KLAY_HURT confirmed data-driven (no inline trigger).
