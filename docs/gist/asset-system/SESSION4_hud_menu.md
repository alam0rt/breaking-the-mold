# Session 4 — HUD / menu asset category

## Finding: the in-level HUD reuses the PICKUP namespace key (0x88200080, R27)
`CreateMenuEntities` (decomp @ line 10390) spawns the status overlay. Two of its sprites are
already-verified PICKUP-namespace names (`<PREFIX>WILLIE` 0x902C0002, `<PREFIX>ONE_UP`
0xA9240484), which fixes the namespace for the rest.

## HUD layout decoded from InitEntitySprite x/y/z args + g_pPlayerState reads
The HUD is rows of `[icon][digit][digit]`, where ONE digit-font sprite is re-drawn at many
x-positions, each reading a `g_pPlayerState[0x11..0x13]` counter value:

| id | x-pos(s) | role |
|---|---|---|
| 0x00E2F188 | 0x25,0x31,0x92,0x9E,0xFF,0x10B | **digit/number font** (renders all counters) |
| 0xB8700CA1 | 0x18 | counter icon, row 1 (left) |
| 0xE8628689 | 0x98 | counter icon, row 2 (left) |
| 0xA9240484 | 0x118 | ONE_UP icon (verified) |

## NEW recovered name (graded to the project standard)
- **0x00E2F188 = `STATUSNUMBERS`** — namespace PICKUP. RECOVERED:
  - exact hash match; length-parity correct (pc=11 -> even-length, 13 chars).
  - **multiplicity 1** within a HUD vocabulary, and stays unique under an expanded 62-word
    vocab (robustness check, same bar as the BOSS-21).
  - corroborated by decomp role: it IS the digit font drawn across the status counters.

## Still blocked (roles known, names not)
- 0xB8700CA1, 0xE8628689, 0x80E85EA0, 0x88A28194, 0x9158A0F6 — menu/HUD icons; need 6-14 char
  compound vocab (item-specific words) not yet guessed.

## Method validated on a 2nd independent category
decomp role -> infer namespace from co-spawned KNOWN ids -> constrained vocab attack ->
uniqueness audit. Proven on BOSS sprites (session 1-3) and now HUD (this session).
