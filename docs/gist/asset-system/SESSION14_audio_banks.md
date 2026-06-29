---
title: "Session 14 — Audio loading architecture (Asset 601/602 -> SPU)"
category: gist/asset-system
tags: [gist, asset, system, audio, banks]
---

# Session 14 — Audio loading architecture (Asset 601/602 -> SPU)

Traced the full runtime audio pipeline; explains the audio namespace structure.

## Loading chain
`InitializeAndLoadLevel`:
  LevelDataParser -> GetAsset601Ptr/Size + GetAsset602Ptr -> **UploadAudioToSPU(bank, volpan, size)**
Called **TWICE** per level:
1. PRIMARY segment bank (shared/global sounds) — loaded once.
2. SECONDARY segment bank (level-specific) — after LoadAssetContainer.

## g_SoundTable layout (UploadAudioToSPU @ 0x8007bfb8)
Each entry = **12 bytes (3 words)**, built in Asset-601 container order:
- word0 `&g_SoundTable[i*3]` = **sound ID** (from 601 sub-TOC, in TOC order)
- `+8` (DAT_8009cc64) = SPU RAM address
- `+8/+0xc` (DAT_8009cc68/6a) = **volume (default 0x3FFF) + pan/flags** ← from Asset 602
  (confirms doc relation: Asset602 size = 601 count x 4).
- `g_SoundTableCount` caps at **0x46 (70)** simultaneous SPU table entries.
`SetGameMode(0-6)` selects the footstep-remap mode (Session 13).

## Bank structure explains the namespace split
- **Shared/primary bank** (33 audio in >=18 levels): mostly **KLAY (12)** player sounds +
  global UI (MENU/BUTTON/EXPLODE/PICKUP). These are the always-loaded core.
- **Level-specific/secondary bank** (160 in <=4 levels): BOSS (27), AMBIENT (18), SKULL (13),
  per-level enemy/object sounds.
- **FX_AMBIENT_* is strictly level-themed** (each environmental loop in its world):
  BLIZZARD->SNOW, CRYSTAL->CRYS, CAVERN->CAVE, BOILER->BOIL, LIGHTNING->SCIE, WIND->BRG1,
  DRIP/RIVER/CHAINS/WATERFALL_LARGE->TMPL, FOUNTAIN->PHRO, WATERFALL_SMALL->MEGA, TORCH->HEAD/SOAR.

## Naming attempt (disciplined)
Cracked level-specific ambience via level-themed env vocab:
- **0xD0B04444 = FX_AMBIENT_DRAIN** (FINN, underwater swim stage) — unique water-AMBIENT
  preimage, level-coherent. BUT single-axis (not in decomp -> no trigger/mechanism backing),
  so recorded as **CANDIDATE**, not RECOVERED. (Holds the bar: footstep METAL/WOOD had a
  proving mechanism + multi-id level-material match; this has neither.)

## Net
- Full audio pipeline mapped: dual-bank (shared + level-specific), 601 TOC-order load, 602
  volume/pan, 70-entry SPU cap, mode-based footstep remap.
- Namespace split explained structurally (shared KLAY/UI vs level-specific BOSS/AMBIENT/enemy).
- +1 CANDIDATE (FX_AMBIENT_DRAIN); ledger 136 RECOVERED / 12 CANDIDATE / 3 EXT.
