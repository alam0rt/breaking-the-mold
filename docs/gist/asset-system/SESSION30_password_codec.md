# Session 30 — Password system: there is NO codec (verified)

## The question: list every password from the code. Answer: not possible — no codec exists.

## Verified in the decomp (not just the doc)
- Password buffer `DAT_8009cb00` (12 bytes) is referenced in exactly ONE place: `FUN_80075ff4`
  (InitPasswordEntry helper) which stores a pointer (+0x140) and renders the 12-slot input
  display (`while (uVar5 < 0xc)`). It is **only written/displayed, never compared to a table.**
- **No decode, no encode, no password->level lookup function** in the 64,363-line decomp.
- Contrast: the CHEAT system DOES have a real table-compare (table @ 0x8009dae0,
  `null_4000h_8009dae0`, 8-button x 22 cheats, lines ~42560-42600). Passwords have no analog.

## Architecture: fixed lookup with PRE-RENDERED graphics (not algorithmic)
- 16 password screens are hidden BLB containers (offsets 0x00EB7000, 0x01355000, ...,
  0x047DC800 = "YOU WIN"). Each has its 12-button sequence **baked into its tilemap graphics**
  (Asset 300 pixels + Asset 400 CLUT; ~52,368-byte sprite blobs, per-screen tile graphics vary).
- Passwords do **NOT encode player state** (lives/items/score not preserved — only which world
  to resume). So there is nothing combinatorial to enumerate: ~16 FIXED strings, one per
  world-complete screen.
- Password-selectable levels (metadata+0x0D password_flag=1): SCIE(2), TMPL(3), BOIL(6),
  FOOD(8), BRG1(10), GLID(11), CAVE(12), WEED(13) = 8 levels. Built into GameState+0x171
  (list, max 10) / +0x17B (count) by InitGameState.
- Buttons: O X S T L1 L2 R1 R2 (8 symbols), 12-long. Cursor 0x3099991b, digit 0xec95689b,
  back 0x10094096 (Session 9 password screen).

## Known passwords (from GameFAQs, per doc — not derivable from code)
- Castle De Los Muertos: O X X R2 R2 R2 R1 R2 R2 R1 O L2
- Elevated Structure:    O L1 S L2 O R1 R2 L1 T R1 S
- Evil Engine #9:        O R1 T S L2 O R2 R2 R1 O O R1

## How to get the FULL list (asset extraction, not decomp)
Extract the 16 password-screen tilemaps from GAME.BLB (Asset 300 + 400, format mapped earlier)
and READ the button icons off the rendered image. It's an image-extraction job, because the
passwords are pixels, not code/data the decomp can yield. (If a password->level match table
exists, it's in .data — same wall as 0x8009b144 / g_LevelNameTable.)

## Net
- CONFIRMED: passwords are a pre-rendered lookup, NOT an algorithmic codec. No encode/decode in
  code. Cannot enumerate from the decomp; the strings live as tilemap graphics in the BLB.
- 8 password-selectable levels identified; password screen entity/sprites mapped.

---

## CORRECTION/CLARIFICATION (cheats vs passwords are DIFFERENT systems)
Prompted by "what about passwords that set Swirly-Q to 48?" — important distinction:

- **CHEAT CODES** (button sequences on pause menu): buffer @ GameState+0x17c (8-entry ring,
  +0x18c cursor), compared vs cheat table **0x8009dae0** (22 cheats x 8 buttons). On match,
  DIRECTLY writes player state. e.g. **All-Powerups cheat (handler 0x02) sets
  g_pPlayerState[0x1b]=0x30 (Swirly=48)** at line 42618. THIS is algorithmic and DOES set state.
- **PASSWORDS** (12-button, menu stage 2): buffer @ DAT_8009cb00, only consumed by the display
  function (FUN_80075ff4). Different buffer, no table compare, sets only the resume level.
  NOT algorithmic, does NOT grant items.

So: the "set Swirly to 48" sequence is the **All-Powerups CHEAT**, not a password. The two are
distinct input systems that merely look alike to the player. My earlier "no codec" statement is
correct ONLY for passwords; CHEATS have a real codec (table compare -> state write).

## The enumerable list IS the cheat table (effects known; button sequences in .data)
22 cheats @ table 0x8009dae0. Effects decoded from handlers (Sessions 19/24):
- 0x02 All-Powerups: lives=99, Swirly[0x1b]=48, Phoenix/Phart/Enema/SuperWillie=7, etc.
- 0x03 Max Swirly Qs [0x13]=20 · 0x05 Max Lives [0x11]=99 · 0x06 Universe Enemas [0x16]=7
- 0x07 Phoenix Hands [0x14]=7 · 0x08 Super Willies [0x1c]=7 · 0x09 Phart Heads [0x15]=7
- 0x0A Green Bullets [0x1a]=3 · 0x0B 1970s [0x19]=3 · 0x0E Invincibility (g_GameFlags|1)
- 0x10 Tinted Klaymen (rainbow) · 0x12 Turbo/frameskip · 0x00 Debug graphics/overlay
- (+ ~8 more handlers). Activation sound 0x90810000 (FX_MENU_SELECT).
- The 8-button SEQUENCES per cheat are in .data table 0x8009dae0 (not in the .c dump — EXE-dump
  target). EFFECTS are known; the button combos need the table bytes.
