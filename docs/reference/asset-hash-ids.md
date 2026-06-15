# 32-bit Opaque IDs (asset hashes & magic tokens)

Investigation 2026-06-15. Supersedes the "4-letter packed ASCII" description that appears in
several Ghidra plate comments (e.g. `GliderEventHandler`, the multi-segment contact pipeline).
**That description is wrong** — see below.

## What they are

The engine is full of 32-bit constants that look like hashes: `0x21842018` (player sprite),
`0x7003474c` (pickup SFX), `0x8B603980` (burst particle), `0x10D86282` (glider spawn-variant
token). They appear in three roles, but are **one flat namespace**:

| Role | Passed to | Example |
|---|---|---|
| Sprite id | `SetEntitySpriteId`, `InitEntitySprite(entity, id, …)` | `0x21842018` Klaymen |
| Sound id | `PlaySoundEffect(id, pan, force)` | `0x7003474c` item pickup |
| Event payload | dispatched as the event `arg`: "use THIS asset" | `0x8B603980` spawn burst particle on 0x1002 hit |
| Discriminator token | compared for equality to select a code path | `0x10D86282` / `0xB0C10420` glider on/off (event 1) |

The asset **type** is not encoded in the bits — it comes from which BLB container the id is looked
up in (TOC type `600`=sprites, `601`=audio; see `docs/blb/toc-format.md`). The same 32-bit value
space is reused for sprites, sounds, particles, and opaque discriminator tokens.

## The "packed ASCII" claim is false

Byte analysis of the supposed "4-letter packed ASCII IDs":

```
0x1020AC7D -> 10 20 AC 7D   ". .}"   2/4 printable
0x10D86282 -> 10 D8 62 82   "..b."   1/4 printable
0xB0C10420 -> B0 C1 04 20   "... "   1/4 printable
0x8B603980 -> 8B 60 39 80   ".`9."   2/4 printable
0x90810000 -> 90 81 00 00   "...."   0/4 printable
```

These are high-entropy values with non-printable bytes in random positions — not 4-letter strings.
They are **opaque hashes**, almost certainly computed from asset names by an offline build tool.

Likely origin of the mixup: the BLB *header* has genuine `char[4]` "short name" fields
(`docs/blb/header.md` +0x08) and the BLB carries human-readable **level titles**
("Skullmonkey Gate", "Monkey Mage", …). Those are real ASCII — but they are level/section names,
a different thing from these per-asset hashes.

## How they resolve at runtime

There is **no string-hashing function in the binary**. The runtime only does *equality compares*:

- Sprites: `LoadSpriteFramesToVRAM` / the 20-slot layer cache scan `slot.spriteHash == id`
  (@ 0x8009AE58); on miss it parses the matching BLB container entry.
- Sounds: id is matched against BLB audio container (`601`) entries.
- Discriminator tokens: literal `if (eventArg == 0x10D86282)` branches.

So the IDs are baked into both the code and the BLB TOC at build time; the game never reverses them.

## Are the original names recoverable?

**Not from the ROM.** Confirmed:

- No string→hash function exists in the binary (grepped the full decompile; every `spriteHash`
  reference is a *consumer*, never a producer).
- No asset-name strings in rodata — only CD filenames (`\GAME.BLB;1`, `\MUSIC1.XA;1`) and PSY-Q
  library debug strings.
- The BLB asset containers are keyed by hash only; there is no per-asset name directory (the BLB
  *does* hold level titles + 2/4-char section codes, but not a sprite/sound name table).

Recovering original names would require either the external build tool's hash algorithm + a name
wordlist (algorithm unknown; not present here), or an asset-name list from another SKU/build.

## Empirical investigation (2026-06-15) — name recovery attempt

Used the extracted assets (`./extracted/<LEVEL>/.../601_audio_samples.bin` etc.) where each
container is `count:u32` then `(id, size, offset)` triples, sorted ascending by id (binary-search
lookup). Findings:

- **IDs are globally stable per asset.** Across 117 audio containers / 227 unique sample ids,
  **226 are byte-identical** wherever they appear. So an id is a deterministic function of asset
  *identity*, not of pool position (rules out per-level sequential numbering).
- **Exactly one collision:** id `0x60830860` is `SNOW` (34528 bytes) vs `RUNN` (7456 bytes) — two
  different sounds. A uniform 32-bit *content* hash colliding even once among 227 items is ~6e-6
  likely, so this is the signature of a **name hash** (a shared-name asset overridden per level),
  not a content hash.
- **Content-checksum ruled out:** CRC32 / CRC32^-1 / Adler-32 / FNV-1a / byte-sum over each
  sample's raw bytes match **0/227** ids.
- **No hash function exists in the game binary.** Every `spriteHash` reference is a consumer;
  asset lookup is binary search over the pre-baked sorted ids. The password system
  (`DecodePassword` @0x80025e48) uses bit-permutation tables + a 3-bit checksum, not a string hash.
- **Dictionary attack negative.** 8 algorithms (crc32, crc32^-1, djb2, djb2a, sdbm, fnv1, fnv1a,
  elf) × ~1600 candidate strings (level codes, official enemy names, SFX vocabulary, with
  case + `.vag/.spr/.tim/...` extension transforms) against the full 639-id pool → **0 hits**.
- **No name table in any BLB.** retail / `beta.blb` / `slps-01501` (JP) / `papx-90053` (demo) contain
  no asset-name strings — only level *titles* and the header's 2/4-char section codes; the apparent
  "strings" in asset bodies are PCM/pixel bytes that fall in the printable range.

**Conclusion:** the ids are opaque, build-time, name-derived hashes (likely a non-standard or
seeded algorithm), and the generator + name list lived only in the lost asset-build pipeline.
**Original names are not recoverable from any shipped artifact.** Pursue role/behavior labels
instead. (A future long-shot: a multi-GB brute force of a seeded/custom hash, or locating the
original Neverhood/DreamWorks asset toolchain.)

## Practical naming

Since true names are unrecoverable, label these IDs by **role/behavior**, which is already
underway in the plate comments and reference tables:

- Sounds: `0x182D840C` = 1-up "jingle" (shared: checkpoint save, bonus life); `0x06E0A824` =
  "splash"; see `docs/reference/sound-ids-complete.md`.
- Sprites/particles: see `docs/reference/sprite-ids-complete.md`,
  `docs/reference/entity-sprite-id-mapping.md`.
- Discriminator tokens: name by the branch they select (e.g. `GLIDER_TOKEN_ENABLE = 0x10D86282`,
  `GLIDER_TOKEN_DISABLE = 0xB0C10420`).

## Cleanup note

The "4-letter packed ASCII" phrasing should be corrected wherever it appears in Ghidra plate
comments — it has propagated to several handlers (`GliderEventHandler`, multi-segment contact
pipeline). Replace with "opaque 32-bit asset-hash / discriminator token (not ASCII — see
docs/reference/asset-hash-ids.md)".
