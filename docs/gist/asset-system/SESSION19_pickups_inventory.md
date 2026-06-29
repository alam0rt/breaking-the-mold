---
title: "Session 19 — Pickup system + GROUND-TRUTH inventory map (cheat table)"
category: gist/asset-system
tags: [gist, asset, system, pickups, inventory]
---

# Session 19 — Pickup system + GROUND-TRUTH inventory map (cheat table)

## The cheat-code table = ground-truth item labels (CheckCheatCodeInput @ FUN_80081fd0)
22 cheat handlers (switch @ 0x80012700) with IN-CODE COMMENTS naming each player-state
inventory field. This is hard ground truth for the player-state layout AND cross-validates
our cracked pickup names:

| g_pPlayerState | item | cheat max | matching cracked name |
|---|---|--:|---|
| **[0x11]** | **LIVES** | 99 | (init 5; DecrementPlayerLives) |
| [0x13] | Swirly Q's | 20 | — |
| [0x14] | **Phoenix Hands** | 7 | FX_PICKUP_PHOENIX ✓ |
| [0x15] | **Phart Heads** | 7 | FX_PICKUP_FARTHEAD ✓ |
| [0x16] | **Universe Enemas** | 7 | FX_PICKUP_UNIVERSE_ENEMA / <PREFIX>UNIVERSE_ENEMA_1 ✓ |
| [0x19] | 1970s Items | 3 | FX_PICKUP_1970 ✓ |
| [0x1A] | Green Bullets | 3 | — |
| [0x1B] | (ammo/count, =0x30) | 48 | — |
| [0x1C] | **Super Willies** | 7 | FX_PICKUP_SUPER_WILLIE ✓ |
| [0x17] | flags bitfield | — | — |

**5 verified FX_PICKUP_* names independently confirmed** by matching the cheat-table items.

## CORRECTION (final) to the HUD/lives question
- **LIVES = g_pPlayerState[0x11]** (init=5 via FUN_800261d4; cheat sets 99). Settled.
- HUD digit rows display [0x12], [0x13]=Swirly Q's, [0x14]=Phoenix Hands (CreateMenuEntities).
- Session 17's "[0x13]=lives" was wrong; Session 18 corrected to [0x11]; this session confirms
  via the cheat table AND the start-value (5 lives).

## PICKUP namespace re-verified (exact)
All 6 `<PREFIX>` pickups satisfy `id = 0x88200080 ^ rotl(calc(ITEM),27)`:
GROW/FARTHEAD/WILLIE/HAMSTER/ONE_UP/UNIVERSE_ENEMA_1 — exact.

## Naming attempt (negative, honest)
Predicted PICKUP-namespace ids for cheat items not yet named (PHOENIX, GLIDEY, SHIELD,
SWIRLY_Q, GREEN_BULLET, ...) — **none land in ids.csv**. The collectible item SPRITES use a
different prefix than the pickup sound/<PREFIX> namespace (or have no standalone pickup sprite).
No new names; clean negative.

## State init: FUN_800261d4
Inits player block: [0x11]=5 (lives), [0x10]=1, [0x12..0x1d]=0. The canonical new-game reset.

## Net
- Ground-truth player inventory layout recovered from the cheat table (9 item fields labeled).
- 5 FX_PICKUP_* names cross-validated against in-code item names.
- Lives field definitively = [0x11] (correcting Sessions 17/18).
- PICKUP namespace formula re-verified on all 6 anchors.

## Files
- `inventory_map.json`
