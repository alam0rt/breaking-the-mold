# Footstep surface-variant remap table (`0x8009d0fc`) — fully cracked

**2026-06-19.** Verified against the ROM (`bin/SLES_010.90`) and the calcHash asset-name scheme.
Extends the gist `docs/gist/asset-system/` Session 13 finding (which named only 5 of these).

## Mechanism

`PlaySoundEffect` intercepts the 5 base Klaymen footstep/run/land sounds and substitutes a
**surface variant** via a table indexed by the current game mode:

```
substituted_id = table_0x8009d0fc[ mode*5 + slot ]
```

- `mode` (0–6) is the world/surface mode set by `SetGameMode(byte)` (7 modes).
- `slot` (0–4) selects the base sound being played.
- Stride confirmed in ROM: 7 rows × 5 × u32 = 140 bytes at RAM `0x8009d0fc` (file `0x8d8fc`).
- **mode 0 is a placeholder/default row** (junk values `0x09090909` / `0x07000700`); the normal
  world plays the base sounds directly and does not index the table.

### 5 base sounds (slots)

| slot | base id | base name |
|---|---|---|
| 0 | `0x70D006D8` | `FX_KLAY_FOOTSTEP_LEFT` |
| 1 | `0x246166FA` | `FX_KLAY_FOOTSTEP_RIGHT` |
| 2 | `0x44D4C8D8` | `FX_KLAY_FOOTSTEP_QUIET` |
| 3 | `0x00248E52` | `FX_KLAY_RUN_FAST` |
| 4 | `0x5860C640` | `FX_KLAY_LAND` |

## The 6 surfaces (modes 1–6) — all cracked

Each row's 5 slots share **one surface suffix**, so the suffix is solved by requiring all 5 ids to
match simultaneously (a combined 160-bit constraint → unambiguous). Each named id round-trips
`id == calcHash(name)` exactly (RAW namespace).

| mode | surface suffix | derivation |
|---|---|---|
| 1 | `_WOOD` | 5/5 row match; BRG1/SOAR (wooden bridges) |
| 2 | `_METAL` | 5/5 row match; BOIL/CSTL/TMPL (metal/stone) |
| 3 | `_ECHO` | 5/5 row match (cave/temple acoustics) |
| 4 | `_SOFT` | 5/5 row match (5 of these were already independently verified) |
| 5 | `_ELECTRIC` | 5/5 row match; ids **not in shipped asset list** (cut/unused mode) |
| 6 | `_SQUISH` | 5/5 row match; ids **not in shipped asset list** (cut/unused mode) |

### Full id → name table (30 entries)

| id | name | shipped? |
|---|---|---|
| `0x40400270` | `FX_KLAY_FOOTSTEP_LEFT_WOOD` | yes ¹ |
| `0x0120E27A` | `FX_KLAY_FOOTSTEP_RIGHT_WOOD` | yes |
| `0x44D6C8CD` | `FX_KLAY_FOOTSTEP_QUIET_WOOD` | yes |
| `0x012484D2` | `FX_KLAY_RUN_FAST_WOOD` | yes |
| `0x5870C6E8` | `FX_KLAY_LAND_WOOD` | yes |
| `0x486002DB` | `FX_KLAY_FOOTSTEP_LEFT_METAL` | yes |
| `0x0478A37A` | `FX_KLAY_FOOTSTEP_RIGHT_METAL` | yes |
| `0x25D2C8D8` | `FX_KLAY_FOOTSTEP_QUIET_METAL` | yes |
| `0x83248E62` | `FX_KLAY_RUN_FAST_METAL` | yes |
| `0x5050C643` | `FX_KLAY_LAND_METAL` | yes |
| `0x001822D8` | `FX_KLAY_FOOTSTEP_LEFT_ECHO` | yes |
| `0x0462E0BB` | `FX_KLAY_FOOTSTEP_RIGHT_ECHO` | yes |
| `0x4CDDCCD8` | `FX_KLAY_FOOTSTEP_QUIET_ECHO` | yes |
| `0x04A68E56` | `FX_KLAY_RUN_FAST_ECHO` | yes |
| `0x1828E640` | `FX_KLAY_LAND_ECHO` | yes |
| `0x401106DA` | `FX_KLAY_FOOTSTEP_LEFT_SOFT` | yes |
| `0x2470E0F2` | `FX_KLAY_FOOTSTEP_RIGHT_SOFT` | yes |
| `0x04DCE858` | `FX_KLAY_FOOTSTEP_QUIET_SOFT` | yes |
| `0x0434CE72` | `FX_KLAY_RUN_FAST_SOFT` | yes |
| `0x5821C242` | `FX_KLAY_LAND_SOFT` | yes |
| `0xCA182248` | `FX_KLAY_FOOTSTEP_LEFT_ELECTRIC` | **no** ² |
| `0x00E4B0BB` | `FX_KLAY_FOOTSTEP_RIGHT_ELECTRIC` | **no** ² |
| `0x559DCCCA` | `FX_KLAY_FOOTSTEP_QUIET_ELECTRIC` | **no** ² |
| `0xA4A6875A` | `FX_KLAY_RUN_FAST_ELECTRIC` | **no** ² |
| `0xD228E6D0` | `FX_KLAY_LAND_ELECTRIC` | **no** ² |
| `0x40550A52` | `FX_KLAY_FOOTSTEP_LEFT_SQUISH` | **no** ² |
| `0x4030E2D2` | `FX_KLAY_FOOTSTEP_RIGHT_SQUISH` | **no** ² |
| `0x04D469C9` | `FX_KLAY_FOOTSTEP_QUIET_SQUISH` | **no** ² |
| `0x007406F2` | `FX_KLAY_RUN_FAST_SQUISH` | **no** ² |
| `0x5865CECA` | `FX_KLAY_LAND_SQUISH` | **no** ² |

¹ **Collision:** `0x40400270` also `== calcHash("FX_FINN_DIE_4")` (an earlier brute-force guess).
The footstep reading wins on hard structural evidence — its position in this ROM table plus 5/5
row coherence — so `FX_FINN_DIE_4` is recorded as the likely false-positive collision.

² **mode 5 (ELECTRIC) and mode 6 (SQUISH) ids are referenced by the table but absent from the
shipped asset-id list** (`cracked_names.csv`). Either the levels that set these modes were cut, or
the assets live in a container we haven't enumerated. They are real ToolX-hashed names (each row
resolves 5/5), analogous to the gist's "3 new asset ids found in decomp but absent from ids.csv".

## Why this is high-confidence (not brute-force noise)

- The table is a hard engine ground-truth: the binary literally indexes these ids as
  surface-substituted footsteps. Slot meaning (LEFT/RIGHT/QUIET/RUN_FAST/LAND) is fixed by the
  base-sound row.
- Each surface row is solved by an **all-5-slots-must-match** constraint with a single coherent
  English word. A wrong word satisfying five independent 32-bit equalities at once is ~2⁻¹⁶⁰.
- 5 SOFT entries were already independently `verified` in the ledger before this pass — they match,
  confirming the table interpretation.

## Status change

`cracked_names.csv`: the 20 shipped ids (WOOD/METAL/ECHO/SOFT) set to `verified`,
`method=footstep_remap_table`. Verified count 119 → 133.
