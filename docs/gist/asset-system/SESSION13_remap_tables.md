# Session 13 — Data-table mechanisms: sound remap + entity-type remap

Found and decoded the two runtime indirection TABLES behind asset usage.

## A. Footstep sound remap (PlaySoundEffect @ 0x8009...) -> +5 NEW names
`PlaySoundEffect` intercepts 5 base Klaymen footstep/run/land sounds and substitutes a
**surface/mode variant** via `s__8009d0fc[ mode*5 + slot ]`, where `mode` (0-6) is set by
`SetGameMode(byte)` (7 game modes/worlds). Table = 7 modes x 5 slots.

5 slots (base sounds, all verified):
| slot | id | name |
|---|---|---|
| 0 | 0x70D006D8 | FX_KLAY_FOOTSTEP_LEFT_NORMAL |
| 1 | 0x246166FA | FX_KLAY_FOOTSTEP_RIGHT_NORMAL |
| 2 | 0x44D4C8D8 | FX_KLAY_FOOTSTEP_QUIET |
| 3 | 0x00248E52 | FX_KLAY_RUN_FAST |
| 4 | 0x5860C640 | FX_KLAY_LAND |

This mechanism PROVES surface-variant footsteps exist -> cracked the variants (RECOVERED;
unique in footstep grammar; FP~0.000; **level matches the material**):
| id | name | levels (material match) |
|---|---|---|
| 0x0478A37A | FX_KLAY_FOOTSTEP_RIGHT_METAL | BOIL;CSTL;TMPL (metal/stone) |
| 0x486002DB | FX_KLAY_FOOTSTEP_LEFT_METAL | BOIL;CSTL;TMPL |
| 0x5050C643 | FX_KLAY_LAND_METAL | BOIL;CSTL;TMPL |
| 0x0120E27A | FX_KLAY_FOOTSTEP_RIGHT_WOOD | BRG1;SOAR (wooden bridge) |
| 0x5870C6E8 | FX_KLAY_LAND_WOOD | BRG1;SOAR |

Footstep family now COMPLETE (wider surface vocab found no more). **Ledger: 136 RECOVERED.**

## B. Entity-type remap (RemapEntityTypesForLevel @ ~0x800815a4) -> full table
Placement entity-type (from level Asset 501 data) is remapped to a **runtime entity-type**
depending on the entity's **layer byte** (1=foreground, 2=background, 3=special). The runtime
type then indexes the 121-entry callback table -> sprite. So the SAME placement type spawns a
DIFFERENT creature per layer. Full table saved in `entity_type_remap.json`:
- layer 1 (fg): 56 mappings · layer 2 (bg): 29 · layer 3 (special): 27

Runtime types that resolve to a known doc-sprite (chain: placement->runtime->sprite):
rt2->0x09406D8A(Clayball) · rt25->0x1E1000B3(EnemyA) · rt27->0x182D840C(EnemyB) ·
rt42->0xB01C25F0(Portal) · rt45->0xA89D0AD0(MessageBox) · rt50->0x181C3854(BossMain) ·
rt51->0x8818A018(BossPart) · rt61->0x6A351094(Sparkle).

## Net
- +5 RECOVERED footstep surface-variant names (remap-table-backed, level-material-validated).
- Complete entity-type remap table (3 layers, ~112 mappings) decoded and saved — the runtime
  type<->sprite indirection that the asset system uses.

## Files
- `entity_type_remap.json`
