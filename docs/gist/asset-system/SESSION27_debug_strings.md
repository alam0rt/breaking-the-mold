# Session 27 — Debug leftovers, retained strings, and the XA music system

Swept the decomp for debug code / weirdness / readable strings. Findings:

## NEW SUBSYSTEM: XA streaming music (3rd audio tier)
The audio model is THREE tiers, not two:
1. SPU SFX — ADPCM in BLB Asset 601 (hashed names) — mapped earlier.
2. **XA streaming music — 5 files `\MUSIC1.XA;1` .. `\MUSIC5.XA;1` on the disc** (CD-XA ADPCM).
3. STR movies — `.STR` FMV w/ interleaved XA.
- **LoadGameAssetLocations** (line 12800): at boot, `CdSearchFile` + `CdPosToInt` resolve
  GAME.BLB and all 5 MUSIC*.XA to SECTOR positions (cached at 0x8009b3d8..e8). This is the
  classic "resolve name->sector once" PS1 optimization (from our day-1 hardware analysis).
- Playback: current-track byte `null_00h_800a59ec` indexes a 24-byte CdlLOC array @
  0x8009b43c; `CdControl(0x1B=CdlReadS, &arr + idx*0x18, ...)` seeks + streams that XA.
  Driven by `StGetNext` (Sony streaming lib). So music = pick track index -> stream that XA
  file. Entirely separate from SPU/BLB audio.
- `SsUtReverbOn/Off` (Sony sound-util reverb) toggled in main + by cheat 0x10.

## Confirmed disc layout (ISO-9660 paths in the binary)
`\GAME.BLB;1` (the one asset archive), `\MUSIC1-5.XA;1` (music), `\MVLOGO.STR;1` + the 13
movies (Movie Table, Session 20). GAME.BLB resolved to g_GameBLBSector at boot.

## DEVELOPER DEBUG MENU (left in the retail binary)
- Enabled by **g_GameFlags & 0x80**; a scrollable **level/movie SELECT** (`ProcessDebugMenuInput`).
- Cursor g_DebugMenuCursor, scroll g_DebugMenuScrollTop, shows 20 items.
- Items 0-9 = direct level select; 10..10+LevelCount = levels from g_LevelNameTable; rest =
  movies. Cross (0x40) selects -> SetSequenceIndexByMode(ctx, mode, index) -> loads it.
- **g_LevelNameTable / g_MenuItemNames** would hold human-readable level names — but they're
  .data tables (values not in the .c; same wall as 0x8009b144). If dumpable from the EXE,
  these are literal LEVEL NAMES.
- Debug-graphics flags: g_GameFlags 0x40 (debug graphics) + 0x80 (debug menu/overlay);
  SetGraphDebug() present. Cheat 0x00 toggles debug graphics/overlay bits.

## Retained Psy-Q library (bottom ~third of the file, with original symbols)
From ~line 43000+ the file is the SN Systems/Sony Psy-Q libs left in with names + debug
printf strings: CdInit/CdRead/CdSearchFile ("CdInit: Init failed", "DiskError:", "CD_newmedia"
), GPU (DrawOTag/PutDrawEnv/ResetGraph debug traces), MDEC (MDEC_in_sync), streaming
(StGetNext), Fnt debug font (FntPrint/FntFlush). These are library leftovers, not game text.

## Other weirdness
- **456 `trap()` calls** = MIPS div-by-zero/overflow guards (trap 0x1c00/0x1800) auto-inserted
  by the compiler around integer divides — not asserts, just the R3000 divide-checks.
- `FntPrint()` calls in-game (lines 22222+) with NO string arg in decomp = debug HUD text
  whose format strings are .data (the debug font overlay).
- Leftover symbol fragments: "CRD2", "GetTileHeaderField08", "Tinted Klaymen" (cheat name).
- `checkRECT("ClearImage"/"LoadImage"/...)` = Psy-Q GPU rect-validation debug (library).

## Net
- **XA streaming-music system discovered** (5 disc files + sector-cache + CdlReadS streaming) —
  completes the audio picture as 3 tiers.
- Developer debug level-select menu documented (g_GameFlags&0x80); g_LevelNameTable flagged as
  a 3rd .data table holding real LEVEL NAMES (EXE-dump target).
- Game/library boundary mapped (~line 43000); retained Sony symbols + debug strings catalogued.
- trap()s explained (divide guards, not asserts).

## Files
- (findings documented here)
