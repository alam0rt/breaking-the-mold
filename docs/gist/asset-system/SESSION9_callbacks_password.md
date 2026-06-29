---
title: "Session 9 — Callback context, co-occurrence bindings, password screen, new IDs"
category: gist/asset-system
tags: [gist, asset, system, callbacks, password]
---

# Session 9 — Callback context, co-occurrence bindings, password screen, new IDs

Continued shining light on BLB asset usage via the decomp callback bodies.

## 1. Callback context reveals sprite + sound + sub-sprite per animation
Each boss-animation callback wires a sprite, its SFX, and its particle/sub-sprite together.
Confirmed `FUN_8001c4a4` -> `PlaySoundEffect` (decomp line 5365), so its 2nd arg is a real SFX:

| callback | named sprite | paired SFX (uncracked) | paired sub-sprite |
|---|---|---|---|
| EntityCallback_800493d8 | WIZARD_FALL | **0x424129A1** | — |
| EntityCallback_80049550 | WIZARD_FALL (2) | **0x66017821** | — |
| Callback_80048f40 | WIZARD_HIT | — | 0xA0CC18D0 (particle) |
| Callback_8004906c | WIZARD_DIE | — | 0xA0CC1CD0 (particle) |

=> **2 uncracked SOUNDS now bound to a named animation** (they're the Wizard fall SFX);
**WIZARD_HIT/DIE particle sub-sprites identified** (0xA0CC18D0/1CD0). Names still need 10-12
char words (longer than our vocab), but roles are now fixed.

## 2. Boss component tree (InitBossEntity)
The WIZZ Monkey Mage is assembled from: 0x181C3854 (BossMain/type50) + 0x8818A018
(BossPart/type51) + 0x0244655D (sub-sprite), each attached via `EntitySetState` -> a
sub-callback. This is the boss's entity hierarchy.

## 3. Password-entry screen — a whole new UI category (decomp function MISNAMED)
`CreateBossPlayerEntity` is **not** a boss — per `password-system.md` (`InitPasswordEntry`
@0x80075ff4) it builds the MENU password screen:

| id | role | in ids.csv? |
|---|---|---|
| 0x3099991B | password cursor (moves between slots) | yes (MENU) |
| 0xEC95689B | password digit display (x12) | yes (MENU) |
| 0x10094096 | password back button | yes (MENU) |
| 0xE4AC9451 | password button/symbol grid (8 cols x 4 rows, 18 placements) | **NO — new** |
| 0xAA0DA270 | password screen sprite | **NO — new** |

## 4. NEW asset IDs found in decomp, ABSENT from ids.csv (extraction gaps)
- **0x4A298690** — sprite in Callback_800682a4
- **0xAA0DA270** — sprite in password screen
- **0xE4AC9451** — the 8x4 password grid sprite
These were referenced by `(Set/Init)EntitySprite` but missing from the 658-row asset list —
the original BLB extraction under-counted. (~69 other hash-like decomp constants are also not
in ids.csv; most are likely message/state hashes, not assets — flagged for triage.)

## Net
- 2 sounds + 4 sprites newly role-bound; 3 brand-new asset IDs surfaced; password-screen
  category mapped; boss component tree documented; 1 decomp function corrected (password, not boss).

## Files
- `decomp_discoveries.json` — new ids, password category, sprite-sound bindings, boss tree
