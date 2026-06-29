---
title: "Session 23 — Which asset types use HASHED ids (beyond sprite/audio)?"
category: gist/asset-system
tags: [gist, asset, system, hashed, types]
---

# Session 23 — Which asset types use HASHED ids (beyond sprite/audio)?

Question: are there hash-keyed assets that aren't sprites or audio (CLUT/palette/other graphic)?

## Answer: container key types
| BLB container | Entry-ID key | Hashed? |
|---|---|---|
| 600 Sprites | 32-bit calcHash | **YES** (our `sprite`, 412) |
| 601 Audio | 32-bit calcHash | **YES** (our `audio`, 227) |
| **503 Anim sequences** | 32-bit calcHash (sprite namespace) | **YES** (our `anim`, 3) |
| 400 Palettes / CLUT | plain index 0,1,2... | **NO** — indexed, not hashed |
| 500 tile-attribute / 700 legacy-SPU | raw packed data | **NO** — not name-hashed |

So: **CLUTs/palettes are index-keyed (no hidden hashed graphic type there).** BUT there IS a
third hashed class: **ToolX animation-sequence assets (Asset 503)**.

## Evidence the `anim` type is hash-named in the sprite namespace
- anim `0x60A91D9C` = sprite `0x60A91C9C` ^ bit 8 (1-bit calcHash char toggle)
- anim `0x60AA1E9C` = sprite `0x60AA1C9C` ^ bit 9
- They interleave in the BRG1 monkey cluster: each sprite has a paired anim-sequence asset
  one bit away (the sprite-vs-sequence token in the name). Same calcHash scheme, distinct type.

## type500 / type700 are NOT hashes (confirmed)
- mean popcount 7-8 (vs ~11-13 for real hashes), low-entropy structured values
  (0x006E0046, 0x00C60046, 0x10050221). Match BLB Asset 500 (tile-attribute map) / 700 (legacy
  SPU) = raw per-level data. The `alts` column shows prior hash-crack attempts produced garbage
  ("SHE_HAS_AILS_IT") — confirming they're not name-hashed. -> un-nameable by design.

## Is `anim` a large hidden category mislabeled as sprite? NO
- Only 19/412 sprites have ANY 1-bit neighbour, and the diff-bit positions scatter (24,25,16,
  10,11,12...) with no consistent "anim bit". So animation-sequence assets are a genuinely
  SMALL, distinct class (the 3 are real, rare) — not a systematic mislabel inflating `sprite`.

## Net (answer to the question)
- Three hashed asset classes exist: **sprites (600), audio (601), and animation sequences
  (503)** — all share the calcHash scheme; anim sits in the sprite namespace 1 bit from its
  sprite.
- Palettes/CLUT and tile/SPU data are NOT hash-named (index/raw) -> no other hashed graphic
  type to chase.
- type500/700 confirmed un-nameable (raw data); should be excluded from name-recovery scope.

## Files
- (findings documented here)
