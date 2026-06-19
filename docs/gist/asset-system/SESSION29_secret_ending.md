# Session 29 — Secret ending system + sprite-ids-complete.md cross-check + subsystem inventory

## Secret ending (NEW subsystem)
Source: secret-ending-system.md + decomp lines 37985-38049.
- Skullmonkeys has a **secret ending (END2 / MVWIN.STR)**, gated on **Swirly-Q TOTAL >= 48**.
- **g_pPlayerState[0x1b] = lifetime Swirly-Q total** (NEW field; distinct from [0x13] = current
  bonus-room swirl count, max 20). Cheat 0x02 sets [0x1b]=0x30 (48).
- Check: `if ((byte)g_pPlayerState[0x1b] > 0x2f)` -> spawn secret-ending entity sprite
  **0xAA0DA270** (z=1000), add to render list. Also a button-gated path (D-Pad Right 0x10).
- Normal ending END1 always plays; END2 is the conditional secret.
- **Resolves a session-9 loose end:** 0xAA0DA270 (decomp-only id, absent from ids.csv) =
  the secret-ending trigger entity. Its role is now known.

## Player-state inventory map — UPDATED
- [0x11]=lives, [0x13]=current Swirly Q (bonus, max 20), **[0x1b]=Swirly Q lifetime total
  (secret-ending gate)**, [0x14]=Phoenix Hands, [0x15]=Phart Heads, [0x16]=Universe Enemas,
  [0x19]=1970s, [0x1A]=Green Bullets, [0x1C]=Super Willies, [0x1D]=boss HP(=5).

## sprite-ids-complete.md — does it add missing sprite ids? Mostly NO
- 34 sprite ids in the doc: **32 already in ids.csv**; the 2 NOT present are
  **0xAA0DA270** (secret-ending) and **0xE4AC9451** (password 8x4 grid) — exactly the
  decomp-only ids flagged in Session 9. So: no NEW ids beyond those two; it confirms both are
  genuinely absent from the asset list and gives roles.
- Minor role refinements gained: 0xE289C059 + 0x81100030 = menu elements; 0x28C080DF +
  0x8AB92024 confirmed top-layer UI (z=30000); 0x10094096 = nav/back button (11 uses);
  0x3099991B = password cursor; 0xEC95689B = password digits (confirms Session 9).
- Otherwise corroborates our existing role map (player table, projectile 0x168254b5,
  EnemyA 0x1e1000b3, boss 0x181c3854/0x8818a018).

## Subsystem inventory (what we've covered vs not)
COVERED: entities, enemy AI, collision/tile, damage/death, audio (SFX+XA music+banks),
level loading/sequence, teleporter branching, pickups/inventory, HUD, menu/password screen,
BLB format, sprites/anim, cheats, entity-types, secret ending.
NOT yet traced (candidate gaps): **camera/scroll/parallax** (3 funcs), **checkpoint-system**,
**password ENCODING** (how progress serializes to the 12-symbol code; we have the screen, not
the codec), **tiles-and-tilemaps layer rendering**, **projectiles** (player weapon details),
**rendering-order/OT**. (Demo/attract-mode intentionally skipped.)

## Net
- Secret-ending system documented; 0xAA0DA270 role resolved; [0x1b] field identified.
- sprite-ids-complete.md adds no new ids (only the 2 known decomp-only ones) + minor role
  refinements.
- Subsystem gap list produced: password-codec, camera, checkpoint, tilemap-render, projectiles.

## Files
- (updates decomp_discoveries.json; findings here)
