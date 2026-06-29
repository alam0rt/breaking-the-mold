---
title: "Session 6 — Banked ledger + deepened sister/entity grouping"
category: gist/asset-system
tags: [gist, asset, system, clusters, entities]
---

# Session 6 — Banked ledger + deepened sister/entity grouping

## Banked
- `audited_ledger.json` synced: **129 RECOVERED + 11 CANDIDATE + 3 RECOVERED-EXT** (the
  3 ScummVM-confirmed Klaymen assets now flagged `external_confirmation`), + STATUSNUMBERS
  added. Namespace keys recorded in the JSON.

## Full-corpus sister clustering (all 531 uncracked ids, every type)
`sister_clusters.json` — union-find on `popcount(id_i ^ id_j) <= 3`. XOR cancels the seed,
so members share a base name and differ only in a frame/direction token (`varying_bits`).
- **52 multi-member clusters covering 167/531 uncracked ids.**
- Naming ONE member names the whole cluster.

## Tight-ball clustering + entity grouping (the upgrade)
`entity_groups.json` — avoids the transitive-chain over-merge (radius<=3 balls, no chaining:
54 clusters / 165 ids), then groups clusters by **identical level signature**. Co-located
families = one entity's multiple animations (the "entity != hash-family" principle).

### Validated against ground truth
The level-grouping correctly reassembles KNOWN entities (cross-checked vs the ledger):
- **MEGA group** (4 ids) = MEGA_IDLE/ROLL/DIE — 100% MEGA. ✓
- **GLEN group** contains YNT_HIT_1/2 + 5 sibling families. ✓
- **WIZZ group** contains WIZARD_FALL_01/02 + the staff (0xA0CC18D0) + 2 more families. ✓

### New structural knowledge (unnamed ids now tied to a named boss/level)
| level | named anchor | adjacent UNNAMED families (same entity) |
|---|---|---|
| WIZZ | WIZARD_FALL | 0x4C2A8850/542A8850, 0xA0CC18D0/1CD0 (staff) |
| MEGA | MEGA_* | (group complete) |
| GLEN | YNT_HIT | 0x18E88310/510/910, 0x4A4EC094/524EC094 |
| KLOG | (KLOGG nearby) | 0x800DA1xx (5), 0x2E80xx04 (3), 0xC864xx60 (3) |

### FINN 16-cluster (largest unsolved family)
center 0x10B91810; **bit 2 splits 7 vs 9 = a direction flag**; remaining varying bits =
frame index. Structure = ~8 frames x 2 directions of ONE FINN object animation. Fixed core
bits [4,19,20,21,23,28] = the base-name signature. Naming any 1 member names all 16.

## Net
Even where names stay blocked, the corpus is now **structurally grouped**: 167 uncracked ids
belong to 52 known sister-families, and the major bosses' full animation sets (named +
unnamed) are assembled. This is durable, citable RE knowledge independent of name recovery.

## Files added
- `sister_clusters.json` (52 frame-sibling families)
- `entity_groups.json` (8 level-grouped multi-family entities)
