# Session 8 — BLB structure study + entity-callback role harvest

## BLB asset model (from evil-engine docs/blb/) — reorganizes the whole corpus
Two DISTINCT id systems in the game, which earlier sessions had partly conflated:

1. **BLB asset TYPES** — small decimal ids (100/200/300/400/500/600/601/700), stored as hex
   (0x064..0x2BC). These are *segment/asset-class* tags, NOT name-hashes. They define the
   file layout (tiles, palettes, geometry, audio bank...).
2. **Container sub-TOC ENTRY ids** — the 32-bit values in OUR ids.csv. These live inside
   container assets 600/601/400 and ARE the calcHash name-hashes:
   - Asset **600** sub-TOC entry id = **Sprite ID (32-bit hash)**  -> our `type=sprite`
   - Asset **601** sub-TOC entry id = **Sample ID (32-bit)**       -> our `type=audio`
   - Asset **400** sub-TOC entry id = palette index (0,1,2..)      -> not in our set

### Consequences (confirmations + corrections)
- Our sprite/audio ids ARE calcHash values — the doc states it outright ("Sprite IDs are
  32-bit hash values, likely generated from asset file names"). Foundation validated again.
- **`type500` / `type700` in ids.csv are RAW per-level data assets** (0x1F4 tile-attribute
  map, 0x2BC legacy SPU), NOT name-hashed. `type700` is doc-CONFIRMED UNUSED. -> STOP trying
  to name these; they have no string. (Removes 16 ids from the "nameable" set, honestly.)
- Asset **503 = "ToolX animation sequence data"** (TOC of {index,size,offset}) — this is the
  exposure-sheet -> sequence-file output. Frame grouping lives here, per-level, by index
  (not by hash) — which is why sprite frames are siblings in hash space but the SEQUENCES
  are indexed, not named.

## Entity-callback role harvest (the actionable yield)
The game has a **121-entry entity-type callback table @ 0x8009d5f8**; each callback calls
`SetEntitySpriteId(entity, <id>, ...)`. Walked all entity/player callbacks in the decomp and
extracted the first sprite id from each.

**Result: 87 id->role bindings; 84 are role-known but unnamed.** Merged with the doc's
entity-type table (ground-truth role labels). Highlights now tied to a concrete role:

| id | role (decomp/doc) | level |
|---|---|---|
| 0x181C3854 | BossMain (entity type 50) | WIZZ |
| 0x8818A018 | BossPart (type 51) | multi |
| 0x0244655D | InitBossEntity | WIZZ |
| 0x1E1000B3 | EnemyA (type 25) | multi |
| 0x09406D8A | Clayball (type 2) | multi |
| 0x0C34AA22 | ItemPickup (type 8) | KLOG |
| 0xB01C25F0 | Portal (type 42) | multi |
| 0xA89D0AD0 | MessageBox (type 45) | multi |
| 0x6A351094 | Sparkle/Timer (type 61) | multi |
| 0xBE/B8/B4/3D... | ExplosionDebris family | multi |
| 0x21842018 / 0x3DA80D13 / 0xCA1B20CB | Player (normal/Finn/Soar variants) | multi |
| 0x168254B5 | Projectile/Particle | FINN |
| 0x088C5011 / 0x344210B1 | FinnDeathExplosion | FINN |
| 0x18298210/292E8480/092B8480/0B2084D0/1B301085... | PlayerState_{WalkLeft,Run,Jump,Fall,Death} | multi |

## Net
- `id_role_map.json` rewritten: **87 bindings**, 84 role-known-but-unnamed (purpose recovered
  even where the string isn't).
- Corpus correctly re-segmented: sprite/audio = hashed names; type500/700 = raw data (un-nameable).
- The boss entity (WIZZ Monkey Mage) now has its type-50/51 main+part sprites identified.

## Files
- `id_role_map.json` (rewritten, 87 entity-callback + doc-table role bindings)

---

## Addendum — Do asset-type numbers affect the hashes? (tested: NO)
Question: could the BLB asset type (100/200/600/601...) be part of the per-namespace key?
- **Type-as-hash:** `calc("600")`, `calc("0x258")`, raw 600/0x258, all rotations — no match to
  any seed.
- **Type-as-suffix: mathematically impossible.** `calc(name+SUF)=calc(name)^rotl(calc(SUF),
  sigma(name))`; sigma(name) varies per name, so a CONSTANT (seed,R) can only arise from a
  fixed PREFIX, never a suffix. The keys ARE fixed prefixes/constants.
- **Namespace != container type (decisive):** RAW spans BOTH sprite(600) AND audio(601);
  TEXT/PICKUP/BOSS are all sprite(600) yet use 3 DIFFERENT keys. If the type drove the seed,
  all type-600 sprites would share one key — they don't. The key tracks **asset ROLE**
  (text/pickup/boss/sfx), not the BLB type number.
- **Conclusion:** asset-type ids (segment-TOC) and name-hashes (container sub-TOC) are
  independent systems. The seed is an opaque per-role prefix constant, not type-derived.
