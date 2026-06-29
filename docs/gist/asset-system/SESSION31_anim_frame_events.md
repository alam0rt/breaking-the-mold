---
title: "Session 31 — Animation frame-event hashes (a 3rd calcHash use + best crack vector)"
category: gist/asset-system
tags: [gist, asset, system, anim, frame, events]
---

# Session 31 — Animation frame-event hashes (a 3rd calcHash use + best crack vector)

## The mechanism (the one genuinely non-obvious system in the codebase)
ToolX can tag specific animation FRAMES with a **calcHash event id**. On playback reaching a
tagged frame, the engine sends NM_ANIMATION_START (message param_2 == 1) carrying the hash as
param_3. The entity handler does `if (param_3 == 0x<hash>)` -> fires the action (play a sound,
spawn a projectile, enable a hitbox, etc.). This syncs gameplay to animation WITHOUT hardcoded
frame numbers. Same pattern ScummVM's Neverhood engine uses (klaymen.cpp NM_ANIMATION_START).

## Harvest: 39 distinct frame-event hashes (see anim_frame_events.json)
- Concentration: ~25 fire from **PlayerCallback_8005e574** (one Klaymen state with many tagged
  frames) + clusters in 8005d0c8 / 8005cce0 / 8005d404. **EntityCallback_80047768** owns
  0x10D86282 + 0xB0C10420 (x4 each) = a boss/enemy with 2 frame events.
- **Cross-validation: 0x146CE002 frame-event == our verified FX_KLAY_DIE_EXPLODE.** Proves
  frame-events share BOTH the calcHash namespace AND the functional naming vocabulary. (Klaymen's
  death animation has a frame tagged FX_KLAY_DIE_EXPLODE; hitting it fires the explosion.)

## Why these are the BEST remaining crack targets (user insight)
UNLIKE asset names (hashed at build, string discarded -> dead end offline), a frame-event fires
at a **known, reproducible gameplay moment**. The hash is observable at its trigger.

### Breakpoint-crack strategy (emulator)
1. Breakpoint each `param_3 ==` site (decomp gives line->fn; addresses derivable).
2. Trigger the moment (e.g. let Klaymen die -> 0x146CE002; specific Klaymen actions ->
   the 8005e574 cluster).
3. Observe what happened on screen at the break -> KNOW the semantic.
4. Collapse the name to a tiny functional vocab (FOOTSTEP/FIRE/HIT/STEP/SHOOT/LAND/SPAWN/...)
   -> brute calcHash over that vocab -> high probability of an exact hit (short functional
   words, exactly like FX_KLAY_DIE_EXPLODE).
5. Bonus: breakpoint the calcHash routine / anim-frame dispatcher and read the event STRING
   directly if the exposure-sheet tag survives in the loaded sequence data.

This is the single best naming attack vector left: live observation point + known semantic +
short name, vs. asset names which have none of those.

## Net
- Documented the hash-keyed animation frame-event system (3rd use of calcHash: assets, cheats,
  and now anim frame-events).
- 39 event hashes catalogued with their handler functions; 1 cross-validated to a known name.
- Identified the emulator-breakpoint approach as the highest-probability remaining crack path.

## Files
- `anim_frame_events.json` (39 events + strategy)
