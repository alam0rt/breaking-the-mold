# Session 10 — Asset usage context as a naming oracle

Mapped HOW assets are used (render layer, sound triggers) to both validate names and crack new ones.

## Render layering (z-order in InitEntitySprite / AllocSpriteRenderContext)
Clean depth bands confirmed:
- **z=10000** UI/HUD foreground (STATUSNUMBERS, menu icons) — 24 placements
- **z=2000** entities/bosses — 15
- **z=1000** background props — 13
- **z=999/9000/30000** special cases; **z<1000** behind player

## Sound trigger contexts (PlaySoundEffect / FUN_8001c4a4 call sites)
Each SFX's trigger function gives its behavioral meaning. Examples (validation — names match
their triggers):
- FX_KLAY_RUN_FAST <- PlayerProcessTileCollision / PlayerState_* (running on ground) ✓
- FX_KLAY_HIT_HEAD <- PlayerCallback_800650c4 ✓
- FX_MENU_SELECT <- CheckCheatCodeInput (menu/cheat feedback) ✓
- FX_PUFF_FALL_3 <- FinnMainTickHandler (FINN swim stage) ✓
- FX_PICKUP_SHIELD <- PlayerTickCallback ✓

## NEW NAMES cracked via trigger context (RECOVERED — triple-validated)
The trigger function names the action; cracked + uniqueness-audited:

| id | name | trigger function | validation |
|---|---|---|---|
| **0x65281E40** | **FX_BUTTON_PAUSE** | `PauseGameAndShowMenu` | exact hash + unique in FX_BUTTON_* grammar + trigger-semantic |
| **0x4C60F249** | **FX_BUTTON_UNPAUSE** | `PauseGameWithFadeOut` | exact hash + unique + pairs with PAUSE; matches FX_BUTTON_POWERUP grammar |

Both multiplicity-1 in the FX_BUTTON_* convention space. This is the strongest result class:
**context + uniqueness agree** (not just a hash collision).

## Still-blocked context sounds (role known, name too long to brute)
- 0x7003474C <- PlayerProcessTileCollision (footstep/land, 12+ chars)
- 0x40E28045 <- PlayerTickCallback ; 0x64221E61 <- 3 PlayerCallbacks
- 0xCC6C8070 <- CreateFinnPlayerEntity (Finn swim) ; 0x40F8C274 <- RUNN runner
Roles recorded; names need >=9-13 char strings beyond current vocab.

## Net
- +2 RECOVERED names (FX_BUTTON_PAUSE/UNPAUSE), trigger-context-validated.
- Sound→trigger map: behavioral role for many SFX; render-layer bands documented.
- The trigger-context method is a repeatable oracle: any uncracked SFX with a semantically-
  named trigger + a short action word is crackable at high confidence.
