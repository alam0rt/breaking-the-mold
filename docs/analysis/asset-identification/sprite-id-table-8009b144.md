---
title: "`0x8009b144` — analysed: NOT a clean 121-type roster (mixed sprite-ID data region)"
category: analysis/asset-identification
tags: [analysis, asset, identification, sprite, id, table, 8009, 144]
---

# `0x8009b144` — analysed: NOT a clean 121-type roster (mixed sprite-ID data region)

**2026-06-19.** Investigated the claim that `0x8009b144` is "THE entity-type → sprite-ID lookup,
a 4-byte-per-entry array indexed by entity type holding the sprite HASH for every type (all 121)."
Verified against the ROM (`bin/SLES_010.90`) and the live Ghidra program.

## What's confirmed

The access pattern is real. `InitAlternateEntity @0x80033dd4` (asm `0x80033dc0`):

```
v0 = source[0]                       ; alternate-spawn entity-type index
v0 <<= 2                             ; *4
a1 = *(u32*)(0x8009b144 + v0)        ; lw $a1, %lo(D_8009B144)($at)
SetEntitySpriteId(entity, a1, 1)
```

So `0x8009b144` is read as a `type → sprite-hash` table — but indexed by the **alternate-entity
spawn type** (`source[0]` from `SpawnEntitiesAlternateSystem @0x80081d0c`, the non-Asset-501 path),
**not** the 121-entry main roster. The other supposed accessor `FUN_8007BBEC` is just
`InitSpriteContextWrapper` (the Ghidra plate "get sprite metadata from table" was a misdescription).

## What's wrong with the "complete 121-roster" framing

Dumping the region shows it is **not one clean array** — it is the base of a large, fragmented
`.data` region holding several different structures back-to-back. splat already splits it into many
small referenced items (`D_8009B144` is only 8 bytes, then `D_8009B14C`, `…B160`, `…B174`, …).

Layout of the first ~262 u32 words (the sprite-ID portion), by contiguous run:

| slots | addr | content |
|---|---|---|
| 0–1 | `0x8009b144` | `0x00090000` header/flag words (not sprites) |
| 12–19 | `0x8009b174` | **menu-text sprites** PAUSED/QUITGAME/CONTINUE/YES/QUIT/NO (overlaps the known menu-string table ~`0x8009b160`) |
| 21–36 | `0x8009b198` | small structured byte-fields (counts/indices, not hashes) |
| 52–61 | `0x8009b214` | entity sprites: MA-Bird `0x91106183`, Clayball `0x09406d8a`, `<PREFIX>HAMSTER` `0xa9228088`, … |
| **101–146** | `0x8009b2d8` | **46-entry run of terrain/riveted-metal sprites** (`0x081c830x` / `0x081d8300`, the stem-`0x081c8300` family) |
| 156–160 | `0x8009b3b4` | sun-flare/bloom FX family (`0x0Xce0610`) |
| ~230–260 | `0x8009b4dc` | BRG1 monkey cluster (`0x60b9xxxx`, `0x60a91c9c`, …) |

A ~40-entry run of identical values (`0x081c8302`) is the decisive disproof of "one sprite per
entity type": these are per-tile/per-instance sub-arrays, not a type roster. **Past idx ~262 the
region is no longer sprite IDs** — slots 318+ hold `0xffff0000`/`0x00010001` flag words and slot
572 holds `0x8009b8ec`, a pointer back into the region.

> The actual 121-entry main roster is the **separate** `EntityTypeCallback` table at `0x8009d5f8`
> (8-byte entries: `u32 state_flags + u32 callback`), and the per-layer placement→runtime remap is
> `RemapEntityTypesForLevel` (see gist `entity_type_remap.json`). `0x8009b144` is a different,
> alternate-entity/menu/terrain data region.

## Value extracted (the part that *is* useful)

Dumping it still pays off — `sprite_id_table_8009b144.csv` (this directory):

- **150 sprite-ID slots** in the first 262 words; **91 match known asset IDs**, **58 distinct
  sprite-like IDs are NOT in the 658-row asset ledger** (`cracked_names.csv`) — candidate new/cut
  IDs (like the gist's "3 new IDs found in decomp"), mostly terrain `0x081c830x` and monkey-cluster
  variants.
- **Adjacency = family grouping for free.** Contiguous runs put one entity's animation set / one
  level's sprite bank side by side — the same families the hash-distance clustering reconstructs
  (terrain-metal, sun-flare, BRG1 monkey), here confirmed by physical table order. Naming one member
  of a run labels its neighbours.

## Conclusion

`0x8009b144` is a verified `alt-type → sprite-hash` lookup for the alternate-entity spawn system,
sitting at the head of a mixed sprite/menu/terrain/flag data region — **not** the complete 121-type
entity roster. The dump ties ~150 sprite IDs to concrete table slots and surfaces 58 IDs absent from
the asset ledger, but it does not give a one-to-one entity-type → sprite map. Full per-slot data in
`sprite_id_table_8009b144.csv`.
