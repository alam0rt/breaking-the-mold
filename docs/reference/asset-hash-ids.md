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

## Hash-family analysis (2026-06-15, round 2)

Goal: identify the hash *algorithm* so names could be brute-forced. Result: narrowed the family,
did not crack it. Engine lineage (per AWN "Our Animation Process", Mike Dietz, Dec 1997):
Skullmonkeys reuses the **same engine as the PC game The Neverhood** (programmers Kenton Leach,
Tim Lorenzen, Brian Bellfield; assets authored in Kenton Leach's **ToolX**). The Neverhood's
resource hash (`calcHash`, ScummVM `engines/neverhood/resource.cpp`) is a sparse **bit-toggle**
hash:

```c
uint32 calcHash(const char *s){ uint32 h=0,sh=0; for(;*s;s++){ char c=*s;
  if(isalnum(c)){ if(islower(c))c-=32; else if(isdigit(c))c+=22; sh+=c-64; if(sh>=32)sh-=32;
                  h ^= 1u<<sh; } } return h; }
```

Findings:

- **The ids are a sparse bit-toggle hash, NOT a content/CRC/multiplicative hash.** Real id
  popcount averages **10.9/32** (every bit position is used, p≈0.34); a CRC/FNV/multiplicative
  hash would average **16/32** (p=0.5). 639 ids put this ~46σ from uniform. `calcHash` of a
  realistic ~15-char filename averages popcount ~10.6 — a direct match. This rules out the entire
  content-hash family and matches the Neverhood bit-toggle family.
- **One unified name-hash namespace.** `sprite_id`, `anim_id` (animations cross-reference other
  sprites by hash), and sound ids are all 32-bit hashes; frame `callback_id`s are small indices
  (1–53), not hashes. Assets reference each other by name-hash — classic hashed-filename system.
- **Per-type hot bits** suggest type-specific name prefixes/extensions: sound ids set bit 6 in 82%
  of cases; sprite ids favour bits 4/19 (~67%). A shared prefix per asset class biases a few bits
  the same way — consistent with `calcHash` over names like `S_*`/`*.SPR`/etc.
- **Could NOT confirm `calcHash` is the exact Skullmonkeys function.** Skullmonkeys' BLB layout
  differs from The Neverhood's (no `0x02004940` magic; different container TOC), so the hash may be
  a variant. Every low-noise known-plaintext test was negative: `calcHash(levelcode)` is not in the
  level's own metadata entry (0/26); a dictionary of game vocabulary × 9 algorithms × transforms
  hit 0/639 ids; whole-binary searches for `calcHash(code)` were statistically indistinguishable
  from random (13/26 real vs 13.9/26 random control).
- **No name corpus exists to confirm against.** Neverhood BLBs store only hashes (pyBLB emits
  synthetic `file{id}` names); no shipped Skullmonkeys/Neverhood artifact carries asset filenames.

**Net:** the ids are sparse bit-toggle **name-hashes**, in one cross-referenced namespace, over
type-prefixed asset names. Recovery still blocked on the missing piece: a list of real asset names
(from ToolX/the build pipeline or a leaked asset tree).

## Round 3 (2026-06-15): calcHash empirically ruled out; no hasher in the binary

A corpus/dictionary attack was run against **11 known-role anchor ids** (verified present in the
extracted pool): player/Klaymen `0x21842018`, Green-Bullets `0xb8700ca1`, Phoenix-Hand `0x9158a0f6`,
Phart-Head `0x8c510186`, 1-up `0xa9240484`, Klogg-ball `0x0c34aa22`, Moving-Platform-A `0x88783718`,
Sparkle `0x6a351094`, jump-sound `0x00248e52`, pickup-sound `0x7003474c`, projectile `0x168254b5`.

- Corpus: ~2000 function-name tokens (the dev's own vocabulary) + manual/enemy/item names.
- Grammar: prefix × token × suffix × extension (`S_`, `SPR`, numbers, `STAND/WALK/...`, `.SPR/.VAG/...`).
- Hashes: `calcHash` + two bit-toggle variants. **~5.8M candidate names × 2 variants → 0 hits** on
  any of the 11 anchors. (An earlier 14.6M-name run against the whole pool produced only collision
  false positives, e.g. `S_JUMP01.SND` → a *menu-sprite* id.)
- **No string→u32 hash function exists in the game.** A structural scan of all ~2400 decompiled
  functions found none that consumes a `char*`/`byte*` and accumulates it (shift/xor/mul) into a
  returned `uint`. Cheats are button sequences, not hashed strings. The game only binary-searches
  pre-baked ids; the hasher was build-time only.

## Round 4 (2026-06-15): multi-version comparison (disks/blb + beta binary)

- **Region builds share assets.** SLES-01090 (PAL) ≡ SLUS-00601 (US) BLB (identical). JP (SLPS-01501),
  demo (PAPX-90053), and SLES-01091/92 differ.
- **Beta vs retail = same names, different data → proves name-hash.** The prototype `GAME.BLB`
  (`disks/prototype/ext/GAME.BLB`, Dec 16 1997) is byte-different from retail yet its container-ID set
  is **100% identical (2820/2820**, all anchors present). Content was re-packed between beta and
  release but the IDs didn't move — so an ID depends only on the asset *name*, not its bytes. Confirms
  the name-hash conclusion independently (cf. the within-retail `0x60830860` collision). The hash +
  asset-name set were frozen ≥6 months before release.
- **Beta binary** (`disks/prototype/ext/SLUS_006.01`, 639 KB debug build) gives the engine's own
  vocabulary via debug strings — `sprt`, `seq` (sequence = ToolX animation file), `sndEfx`, `tile`,
  `clut`, `tpage`, `xtile`, `Master`, `Overlay`, `Layers`, `SubLevels`, `event` — and confirms
  ID-based lookup (`MISSING SEQ $%x`). A disassembly scan found **no bit-toggle hash loop**
  (`sllv 1`+`xor`+`lbu`) and no string→u32 hasher: even the debug build uses pre-baked IDs. No symbol
  table, no asset filenames.

**Net of round 4:** comparison corroborates everything (name-hash, frozen early, ID-based runtime) but
no shipped build (4 region retails, beta, JP, demo) carries a name table, a hasher, or symbols. The
missing piece is not on any disc we have.

## Round 5 (2026-06-15): demo disc (disks/demo, internal codename "N2")

The demo disc bundles two builds: `N1/` = The Neverhood (different game, ignore) and `N2/` =
Skullmonkeys (`disks/demo/N2/`).

- **Demo content = first 8 levels, strict subset.** Demo `GAME.BLB` level table: MENU, PHRO, SCIE,
  TMPL, FINN, MEGA, BOIL, SNOW. The other 18 retail levels (incl. 4 of 5 bosses — HEAD/GLEN/WIZZ/KLOG)
  are absent; **0 demo-only levels**. Container-ID scan: 1605 demo ids, 1488 shared with retail,
  ~1332 retail-only (the missing levels' assets).
- **Useful by-product: per-level ID attribution.** An ID present in the demo belongs to one of the 8
  demo levels or to common assets; an ID absent from the demo belongs to one of the 18 missing levels
  (e.g. CSTL-only IDs are Castle-de-los-Muertos assets). This sharpens role/level labeling and a
  future targeted wordlist — but is not name recovery.
- **`N2.EXE` is a stripped JP-derived demo build** (`BISLPS-01501NH00`, Jul 21 1998): even more
  stripped than the beta SLUS binary — no `MISSING SEQ`/`cant alloc` game strings, no asset names,
  and no string→u32 hasher in its disassembly. Least useful of the three binaries for the hash.

**Net of round 5:** the demo confirms scope and enables level attribution, but carries no names/hasher
either. On-disc avenues (4 region retails + beta + demo + JP, plus 7 BLB variants and 3 binaries) are
exhausted. `disks/demo/N1/` is The Neverhood, not Skullmonkeys; it is useful only as weak
engine-lineage context, not as direct evidence for Skullmonkeys ids.

**Revised conclusion:** the Neverhood `calcHash` is **disfavored** for Skullmonkeys (it fails on every
verified anchor over millions of plausible names) — Skullmonkeys uses a *different, unknown* sparse
hash. A forward dictionary/brute-force attack **cannot proceed** without the algorithm, and the
algorithm is not in any shipped artifact. **Practical name recovery is blocked** pending an external
input: the original asset tree / ToolX build tool, or ≥1 confirmed `(name → id)` pair from a dev or
leaked source (which would pin the algorithm and unlock a verifiable dictionary attack). Repro
scripts for all rounds are in this session's history.

## Round 6 (2026-06-15): PAL localization deltas crack the hash structure

Important scope note: the `disks/demo/N1/` folder is **The Neverhood**, not Skullmonkeys, so it is
not used as evidence here except as engine-history context. This round uses only Skullmonkeys BLBs:
SLES-01090 = English, SLES-01091 = French, SLES-01092 = German.

New repro helper: `tools/scripts/asset_hash_probe.py` parses Skullmonkeys BLBs directly and can
summarize pools / diff located container entries:

```bash
python3 tools/scripts/asset_hash_probe.py --pool
python3 tools/scripts/asset_hash_probe.py --diff disks/blb/sles-01090.blb disks/blb/sles-01091.blb
python3 tools/scripts/asset_hash_probe.py --diff disks/blb/sles-01090.blb disks/blb/sles-01092.blb
python3 tools/scripts/asset_hash_probe.py --regional
python3 tools/scripts/asset_hash_probe.py --dict
python3 tools/scripts/asset_hash_probe.py --lang
```

Findings:

- English retail / US / beta all have the same 642 unique sprite+animation+audio id pool. French
  and German keep the same counts, but replace exactly **23 unique top-level sprite ids**. Audio ids
  and internal animation ids do **not** change. Those 23 replacements appear at 198 located TOC
  entries because the same sprite banks are repeated across many levels.
- Most sprite payloads also differ by a tiny regional CLUT/VRAM value (`0x815d` English,
  `0x815f` French, `0x815b` German), so byte-for-byte sprite diffs are noisy. This does **not**
  explain the 23 id changes: thousands of sprite payloads get the CLUT tweak while their ids stay
  fixed. The id signal is specifically the 23 top-level TOC-key replacements below.
- SLES-01091 (French) and SLES-01092 (German) replace the same English sprite banks with different
  localized ids. Examples:

```text
English     French      German      Role (visual, from extracted English sprites)
0x524ec094  0x13ab9854  0x33aec615  common primary character bank
0x542a8850  0x4a7f0454  0x4a2ae046  monkey/enemy character bank
0x512c8430  0x5e064232  0x5e2cb03b  monkey/enemy character bank
0x620ec210  0x90a2a230  0x920d82a0  repeated stage object / creature bank
0x4aee0118  0xc924b098  0x892e0c1a  MEGA boss bank
0x492c8430  0x46064232  0x462cb03b  WIZZ boss bank
```

The breakthrough: every English→French XOR delta is a rotation of **one** 32-bit mask, and every
English→German delta is a rotation of **one** 32-bit mask:

```text
EN ^ FR ∈ rotations(0x01079563)   # popcount 12
EN ^ DE ∈ rotations(0x00340b0f)   # popcount 10, normalized form
```

Examples:

```text
0x512c8430 ^ 0x5e064232 = 0x0f2ac602 = rotl(0x01079563, 9)
0x524ec094 ^ 0x13ab9854 = 0x41e558c0 = rotl(0x01079563, 6)
0x620ec210 ^ 0x90a2a230 = 0xf2ac6020 = rotl(0x01079563, 13)

0x512c8430 ^ 0x5e2cb03b = 0x0f00340b = rotl(0x00340b0f, 24)
0x524ec094 ^ 0x33aec615 = 0x61e00681 = rotl(0x00340b0f, 21)
0x620ec210 ^ 0x920d82a0 = 0xf00340b0 = rotl(0x00340b0f, 28)
```

For the same English asset, the German rotation is always the French rotation + 15 (with the
normalized masks above). Aligning both substitutions to the same suffix coordinate gives:

```text
FR localized-suffix delta = 0x01079563
DE localized-suffix delta = rotl(0x00340b0f, 15) = 0x0587801a
```

This is the missing mathematical fingerprint. A fixed suffix replacement in a cumulative bit-toggle
hash produces exactly this pattern: a constant suffix-delta mask rotated by the rolling checksum of
the preceding filename. A content hash, CRC, FNV, integer id, or random remap would not make 23
independent old→new pairs collapse into one rotation class.

**Structural model now confirmed:**

```c
u32 toolx_asset_hash(const char *name) {
    u32 h = SEED_MASK_OR_0;
    u32 pos = SEED_POS_OR_0;
    for each normalized name character c:
        pos = (pos + step[c]) & 31;
        h ^= 1u << pos;
    return output_permutation(h);  // likely identity or a fixed bit/byte permutation
}
```

The French/German BLBs show the affected names differed by a **final localized suffix** (not a
prefix): something equivalent to `...<EnglishTag>` → `...<FrenchTag>` / `...<GermanTag>`. If the tag
were a prefix, the rest of the filename would also be re-rotated and the deltas would not be one
constant rotation class.

What remained not pinned at this stage:

- exact `step[c]` table / character normalization (it is **not** confirmed Neverhood `calcHash`);
- exact localized suffix strings (common candidates like `ENGLISH/FRENCH/GERMAN`, `EN/FR/DE`,
  serial tags, and both runtime-`calcHash` / ToolX-punctuation variants do not match the two delta
  masks);
- original asset names.

**Updated conclusion:** the hash mechanism is no longer a black box. It is a ToolX build-time,
cumulative **rotating bit-toggle name hash**. The remaining blocker for true name recovery is not
the family, but the exact character table / suffix tags / real filename corpus. One confirmed
`(asset name → id)` pair from ToolX or source would likely be enough to solve the final parameters;
without that, role labels remain the practical path.

## Visual anchor workflow (2026-06-15)

To turn visual knowledge into hash-cracking anchors, generate focused sprite contact sheets:

```bash
python3 tools/scripts/build_asset_contact_sheets.py --out docs/analysis/asset-identification
```

This produces a small, reviewable chunk rather than every asset:

- `docs/analysis/asset-identification/sprites_regional_01.png` — the 23 English sprite IDs that
  were replaced in French/German.
- `docs/analysis/asset-identification/sprites_focused_hash_neighborhood_*.png` — the same localized
  suspects plus nearby numeric-hash and low-Hamming-distance neighbors.
- `docs/analysis/asset-identification/sprite_identification_template.csv` — fill this with
  `human_role`, `possible_original_name`, `confidence`, and notes.

Hash proximity caveat: numeric sort order is only a weak clue for this sparse bit-toggle family.
The focused sheets include numeric neighbors because they are quick to inspect, but low Hamming
distance and shared level/segment/animation structure are better first-pass similarity signals.
Use visual labels as role anchors first; only treat a `possible_original_name` as a hash-solving
anchor if it is exact enough to test against all regional deltas and known IDs.

## Round 7 (2026-06-15): `YES`/`NO` visual anchors pin a candidate exact formula

User visual identification from the focused/regional sheets found two unusually strong anchors:

```text
0x29c0e211 = text "No"
0x2ad0f011 = text "Yes"
```

These are stronger than role labels such as "cursor" or "monkey" because the likely original text
tokens are exact. They match the canonical Neverhood/ToolX cumulative bit-toggle `calcHash` if
Skullmonkeys applies a fixed output rotation and XOR mask:

```text
calcHash("NO")  = 0x20004000
calcHash("YES") = 0x42020000

observed: 0x29c0e211 ^ 0x2ad0f011 = 0x03101200
expected: rotl(calcHash("NO") ^ calcHash("YES"), 27) = 0x03101200

seed from NO:  0x29c0e211 ^ rotl(0x20004000, 27) = 0x28c0e011
seed from YES: 0x2ad0f011 ^ rotl(0x42020000, 27) = 0x28c0e011
```

Current best candidate formula:

```c
u32 skullmonkeys_asset_hash(const char *name) {
  return 0x28c0e011 ^ rotl(neverhood_calcHash(name), 27);
}
```

Repro:

```bash
python3 tools/scripts/asset_hash_probe.py --seeded
```

The `--seeded` probe reproduces both `NO` and `YES` exactly. It also checks obvious visual-role
guesses (`CURSOR`, `MENU_CURSOR`, `MONKEY`, `SCREAMER_MONKEY`, `SHRINEY_GUARD`, etc.); those do
**not** hit, meaning the user-provided role labels are useful but are not exact ToolX filenames.

Under this candidate output rotation, the Round 6 regional suffix masks map back into pre-output
`calcHash` coordinates as:

```text
FR suffix delta: 0x20f2ac60
DE suffix delta: 0xb0f00340
```

The localized suffix strings are still unknown, but they can now be tested against a concrete hash
candidate rather than a whole family of variants.

## Round 8 (2026-06-15): MENU text anchors confirm the formula

Additional user visual identification from `MENU` primary sprites gave four more exact text anchors.
Important path note: `MENU` is the game level/section string from the BLB table; `primary` is the
extraction bucket name, not an original ToolX directory/name.

```text
0x0ad0f813 = text "Paused"   (visual read from the common all-level text bank)
0x68c0f413 = text "Quit"
0x69c04050 = text "Continue"
0x69c8f473 = text "Quit Game"
```

Together with `YES`/`NO`, all six exact text labels reproduce the same fixed output rotation and
XOR seed:

```text
Name       calcHash(name)  Asset id    Implied seed
NO         0x20004000      0x29c0e211  0x28c0e011
YES        0x42020000      0x2ad0f011  0x28c0e011
PAUSED     0x42030044      0x0ad0f813  0x28c0e011
QUIT       0x00028048      0x68c0f413  0x28c0e011
CONTINUE   0x20140828      0x69c04050  0x28c0e011
QUIT GAME  0x21028c48      0x69c8f473  0x28c0e011
```

`calcHash` ignores non-alphanumeric characters, so `QUIT GAME`, `QUIT_GAME`, and `QUITGAME` are
equivalent for hashing. This moves the formula from "candidate" to confirmed for alphanumeric
asset/text names:

```c
u32 skullmonkeys_asset_hash(const char *name) {
  return 0x28c0e011 ^ rotl(neverhood_calcHash(name), 27);
}
```

Expanded repro:

```bash
python3 tools/scripts/asset_hash_probe.py --seeded
```

The same user pass also identified `0x00e2f188` as a number-glyph sprite: frames 0-9 are the
digits `0` through `9`. Obvious exact-name guesses for that bank (`NUMBERS`, `NUMBER`, `NUM`,
`DIGITS`, `DIGIT`, and broader number/font variants) do **not** hash to `0x00e2f188`, so that
bank's original ToolX name is still unknown. This is expected: visual role labels are not always
source names, while literal menu text labels often are.

Current status: the hash code is cracked. Remaining work is corpus/name recovery: finding the real
ToolX names for non-literal sprites, sounds, animations, and shared banks.

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

## Round 14 (2026-06-16): exhaustive cracking attempts and dead ends

Continued the cracking campaign with multiple attack vectors. **No new names confirmed**, but
several dead ends documented to help future sessions avoid repeating wasted work.

### Attack vectors tried (all yielding 0 confirmed names)

1. **Language suffix MITM (FR/DE delta orbits)** — extended round 6 work:
   - FR delta `0x01079563` (popcount 12): 0 alpha-only pairs at length ≤5, 14,788 pairs at
     length 6 but no plausible language tag pairs (no `USA/FRA`, no `ENG/FRA`, no `EN/FR`).
   - DE delta `0x0587801a` (popcount 10): 4,032 pairs at length ≤5, all garbage.
   - Direct constraint `calcHash(S_FR) XOR calcHash(S_DE) == 0x04801579`: also 0 plausible.
   - **Conclusion**: localized suffix is either >6 chars, uses non-alphanumeric chars, or
     has unusual structure; cannot reverse from the observed delta orbit alone.

2. **Neverhood ScummVM engine vocabulary** — derived from sister-game source:
   - Tested verbatim `asRecFont`, `sqDefault`, `meNumRows`, `meFirstChar`, `meCharWidth`,
     `meCharHeight`, `meTracking` against 658 unknowns: 0 hits.
   - Class prefixes (`As*`, `Ss*`, `Km*`, `Sc*`, `Mo*`) crossed with Skullmonkeys vocab: 0 hits.
   - Scene/Module numeric patterns (`Scene1101`, `Module2700`, etc.): 0 hits.
   - **Conclusion**: Skullmonkeys uses different internal naming than Neverhood despite
     sharing calcHash.

3. **Role-based attacks on visual targets**:
   - 9237 role-specific candidates for 8 role-annotated IDs: 0 hits.
   - 36,010 prefix+role combinations across all 8 targets: 0 hits.
   - 2-token MITM with role vocab × game vocab: 0 hits per target.
   - 3-token MITM against role-annotated targets: 0 hits per target.
   - **Conclusion**: visual role labels (e.g. "Shriney Guard slamming") are NOT the original
     ToolX file names. The build pipeline used different vocabulary.

4. **Function-context MITM with full English wordlist** (148K words):
   - Per-function attack using function-name tokens × dictionary: 9 hits where token-A
     aligns with function context but token-B is dictionary noise (e.g. `RIGHT_BARA`,
     `DOWN_MOUNTLET`, `MENU_ENSEAT`).
   - **Conclusion**: token-A side has signal, but second token vocabulary is incomplete.

5. **Compound 3-token attack** (`compound_hash_attack.py --max-parts 3`): yielded 11 ids
   hit, of which only 5 are non-anchor candidates with structural plausibility:
   - `0x085860d4` GEMCOMMON (MEGA, sprite|anim)
   - `0x18248010` SPIKESSPITTINGD (KLOG, sprite|anim)
   - `0x28a0c119`/`0x30a0c119`/`0x38a0c119` various ROYAL* / IDOL* (MENU, sprite|anim)
   - These are STRUCTURAL matches (level + role + index) but cannot be confirmed without
     ground truth.

6. **kcrack combine + extend** with focused wordlist:
   - Depth 2: only 5 anchors hit.
   - Depth 3: 32 IDs with hits, mostly noise but several level-aware candidates
     (`SHARDS_FX_02`, `RUNN_MUSIC_P2`, `WAYPOINT_PHRO_20`, `COINS_EVIL_25`).

### Practical reality

calcHash collapses 26 letters + 10 digits onto 32 bit positions via running XOR. Each
length-N alphanumeric string has expected ~N bits set in its hash, with collisions abundant:

- Length-7 alpha brute (8 billion strings): ~1M unique hashes out of 2^32.
- Each unknown ID has thousands of valid string preimages.
- 6 confirmed names took human visual inspection of rendered TEXT in sprite atlases.

**Without ground truth (visual inspection of sprite content, leaked source, original asset
list), distinguishing real names from collisions is mathematically impossible.** The 6 anchors
worked because they're literally rendered ASCII text inside sprite frames.

### Tooling additions this round

- `tools/lang_pair_orbit` (compiled, fixed): correct length-L pair search against a delta
  orbit (no popcount filter bug). Replaces buggy `tools/lang_suffix_crack`.
- `tools/scripts/role_target_focused.py`: 2/3-token MITM against role-annotated targets
  using game vocab. Returns 0 hits.
- `tools/scripts/lang_orbit_direct.py`: direct calcHash bucket search for orbit hits.
- `tools/scripts/lang_pair_efficient.py`: pivoted hash-bucket pair search.
- `tools/scripts/level_role_combo.py`, `level_names_brute.py`, `engine_strings_test.py`:
  various focused vocabulary attacks.
- `memories/session/round14_summary.md`: detailed session notes.

### Recommended next moves

- **Crowd-source via klay.iced.cool**: humans can name visual sprites with knowledge no
  algorithm has. The site is already running; manifest covers role-annotated sprites.
- **Visual identification expansion**: identify more rendered-text sprites (any text glyphs
  or readable labels in sprite atlases would be exact anchors).
- **Source code archaeology**: the original ToolX build pipeline lived at The Neverhood Inc.
  Any leaked source, tooling, or asset list from DreamWorks Interactive / Universal Studios
  archives would be definitive.
- **Family-extension from sibling structures**: when one name in a family is found, the
  others differ by a short affix; `kcrack extend` solves this in milliseconds.

## Round 15 (2026-06-16): TWO NAMESPACES — most BLB IDs use raw calcHash

### Decisive finding

The 658 master-CSV asset IDs span **TWO different hash namespaces**:

1. **Wrap namespace** (UI / menu strings only):
   `id = 0x28C0E011 ^ rotl(calcHash(name), 27)`
   - Used by: NO, YES, PAUSED, QUIT, CONTINUE, QUITGAME (Round 7-8 anchors).
   - Lives in the menu string-id table at RAM `0x8009b160`.

2. **Raw namespace** (BLB sprite/anim/audio assets — the vast majority):
   `id = calcHash(name)` — direct, no XOR/rotl wrap.
   - Lives in the asset-id table at RAM `0x8009c100..0x8009c270` (and elsewhere).
   - **60 of 63 entries** in this single table are present in the master CSV.

For Rounds 7-14 we brute-forced all 658 IDs assuming the wrap formula. **That was wrong** for
~99% of them. Every brute-force pass needs to be re-run with raw `calcHash`.

### How it was discovered

Mining 36 ScummVM Neverhood `*.cpp` files (~664 KB of source) for all 32-bit hex literals
yielded 1,929 unique values. Cross-referencing those raw values directly against the
Skullmonkeys master CSV produced **3 hits in `klaymen.cpp`**:

| Skullmonkeys ID | ScummVM call site | Asset role |
|---|---|---|
| `0x5900C41E` | `Klaymen::stIdleBlink` → `startAnimation(0x5900C41E, 0, -1)` | sprite\|anim |
| `0x9D406340` | `Klaymen::hmIdleHeadOff` → `playSound(0, 0x9D406340)` | audio |
| `0x5860C640` | `Klaymen::hmLandOnFeet`/`hmJumpToGrabRelease` → `playSound(0, 0x5860C640)` | audio |

These are **byte-identical raw `calcHash` outputs** — proving Skullmonkeys reused the same
asset-name strings as Neverhood for the player character (Klaymen / Klaymen-derived monkey).

The presence of `0x5900C41E` was independently confirmed in the Skullmonkeys binary at
RAM `0x8009c208` (file offset `0x8c9f0`), in a 60-entry table where every other entry is
also a master-CSV asset ID.

### Why the wrap anchors still hold

The 6 anchors in the wrap namespace are demonstrably real because (a) the formula matches
all 6 simultaneously with one shared seed `0x28C0E011`, and (b) they sit in their own
dedicated 6-slot table in the menu code path. They are simply a *separate namespace* used
by the menu rendering layer, not the BLB resource manager. The two systems were both
inherited from Neverhood — Neverhood's resource manager uses raw `calcHash`, while
Skullmonkeys layers a second hash on top for menu-internal indexing.

### Confirmed Klaymen-derived asset names (cross-game ground truth)

These three IDs are the first in 14 rounds to have **ground-truth provenance** — they
correspond to specific Klaymen animations/sounds documented in the open-source ScummVM
Neverhood engine:

| ID | Role | Klaymen function | Skullmonkeys levels |
|---|---|---|---|
| `0x5900C41E` | sprite\|anim | idle blink (startAnimation) | 21 levels (every gameplay level) |
| `0x9D406340` | audio | head-off idle sound | 12 levels |
| `0x5860C640` | audio | jump-land / grab-release sound | 21 levels |

The "21 levels" coverage maps exactly to every level that contains the player character —
strong corroboration that these are core player assets, just as in Neverhood.

### Tooling updated

- **`tools/kcrack`** now takes `--raw` to use the BLB namespace (raw `calcHash`) for
  `hash`, `match`, `combine`, and `extend` modes. Without `--raw`, the wrap formula is
  used (menu strings only).
- **`tools/scripts/neverhood_cross_reference.py`**: extracts every 32-bit hex literal from
  ScummVM Neverhood source, cross-checks against the master CSV (raw + wrap), emits
  `docs/analysis/asset-identification/neverhood_direct_hits.csv`.
- **`tools/scripts/role_aware_filter.py`**: filters raw-mode combine output by inferred
  asset role (audio / sprite / particle / background / tile / font / palette / music)
  derived from the calling function name in `asset_usage_by_function.csv`.

### What changes next

- Every previous brute-force pass must be re-run with `--raw`.
- The `floor(id) = popcount(id ^ 0x28C0E011)` rule is **wrong for raw-namespace IDs**;
  the correct lower bound is `popcount(id)`. Master-CSV `floor` column is now misleading
  for ~99% of entries.
- Existing wordlists are usable, but ScummVM Neverhood asset names + Skullmonkeys-specific
  level codes / character names are the highest-value seeds.
- ScummVM Neverhood source (in `/tmp/scummvm_neverhood/`) is a shared dictionary — any
  raw hex literal there is a candidate hash that may also appear in Skullmonkeys.

## Round 16 (2026-06-16): Family-targeted brute on FX_X_Y patterns

### Goal

Apply the two-namespace breakthrough to systematically extend the audio FX family
beyond the 11 names cracked in Round 15.

### Approach

1. **Audio-only filter**: restrict targets to `type=audio` uncracked IDs (211 entries).
2. **`FX_<ACTOR>_<VERB>[_<MOD>]` template**: every existing verified audio name follows this
   exact shape, so we brute only this template — ignoring sprite/particle prefixes.
3. **High-coverage actor + verb dictionaries**: ~80 actors (Klay, Skull, Finn, Phart, Boss,
   Bird, Rat, Gum, Pig, Bug, Glid, Robot, Alien, …) × ~250 verbs (movement, vocal sounds,
   damage, item interactions, mechanical/environmental) × ~50 modifiers (1-9, 01-20,
   A-J, _LOOP, _LEFT, _RIGHT, _BIG, _SM, _LO, _HI, _UP, _DN, …).
4. **Single-candidate filter**: only commit hits where exactly one name in the brute hits
   the target ID, AND the level coverage matches the actor/verb context.

### New verified cracks (12 audio names)

#### From `tools/scripts/aggressive_fx_brute.py`

| ID | Name | Levels | Reasoning |
|---|---|---|---|
| `0x7c108242` | `FX_SKULL_FLY_01` | EVIL;FOOD;MOSS;SOAR | flying skull bot in 4 levels incl. SOAR (autoscroll fly) |
| `0x7c108244` | `FX_SKULL_FLY_02` | CLOU;CRYS;SOAR | sister to FLY_01 |
| `0xc8830661` | `FX_BIRD_FLY_01` | BRG1;SOAR | bird in BRG1 + SOAR (sky levels) |
| `0xd914004f` | `FX_BIRD_SQUISH_01` | BRG1;PHRO;SOAR | bird squish in flying-enemy levels |

#### From `tools/scripts/audio_fx_targeted.py`

| ID | Name | Levels | Reasoning |
|---|---|---|---|
| `0x612010c9` | `FX_BOSS_HEAD_HIT` | HEAD only | HEAD level boss fight |
| `0x682080cd` | `FX_BOSS_HEAD_TURN` | HEAD only | sister |
| `0xa06088c9` | `FX_BOSS_HEAD_WALK` | HEAD only | sister |
| `0x603588e8` | `FX_BOSS_HEAD_IDLE_01` | HEAD only | sister |
| `0x71030240` | `FX_RAT_DASH_END` | BRG1 only | BRG1 has rat enemy |
| `0x1000c042` | `FX_SKULL_FOOTSTEP_LEFT` | 9 skullbot-enemy levels | left-foot footstep |
| `0x991003c2` | `FX_SKULL_FOOTSTEP_RIGHT` | 9 skullbot-enemy levels | right-foot footstep (pair) |
| `0xc00200c9` | `FX_GUM_PIERCE_DN` | MEGA only | MEGA has gum enemy |

### Why these are credible

- **Single brute candidate**: with ~31M unique strings tested, finding exactly one match for
  an ID confines the name to a tiny pre-image space.
- **Level coverage matches role**:
  - HEAD-only audio with `FX_BOSS_HEAD_*` prefix is internally consistent (one boss, one level).
  - Skullbot levels (PHRO/SCIE/MOSS/SNOW/CRYS/etc.) for `FX_SKULL_FOOTSTEP_*` matches the
    enemy's appearance set.
  - `FX_BIRD_FLY_01` on BRG1+SOAR matches the only two levels with flying bird enemies.
- **Pair/series consistency**: `FOOTSTEP_LEFT` and `FOOTSTEP_RIGHT` hash to two distinct IDs
  with the *same* 9-level coverage — exactly the pattern of a stereo-alternating walk SFX.
- **All 4 BOSS_HEAD verbs** (HIT, TURN, WALK, IDLE_01) live in the same single-level cohort.

### Running tally

After Round 16:

| Status | Count |
|---|---|
| `verified` | 29 (6 menu anchors + 23 raw-cracked audio) |
| `scummvm_anchor` | 2 (Klaymen anchors with role only) |
| `uncracked` | 627 |
| **Total** | **658** |

### Tooling

- `tools/scripts/aggressive_fx_brute.py` — broad FX_*+SFX_*+SND_* prefix sweep across all
  unknowns, ~31M unique strings.
- `tools/scripts/audio_fx_targeted.py` — audio-only filter (211 targets), `FX_*` only,
  ~8M unique strings.
- `tools/scripts/sister_hunt.py`, `tools/scripts/family_brute.py` — sister-asset probes
  per known family root.
- `tools/scripts/head_only_brute.py` — exhaustive boss-head-verb sweep on the 8
  remaining HEAD-only audio IDs (no hits — names use less-obvious verbs or longer forms).

## Round 17 (2026-06-16): family STRUCTURE cracked (names still blocked)

New approach: stop chasing literal strings, recover family *structure* + *role* instead. Full
write-up in [`docs/analysis/asset-identification/family-structure-and-categories.md`](../analysis/asset-identification/family-structure-and-categories.md).

- **Two namespaces re-confirmed:** RAW `id=calcHash(name)` (audio + gameplay sprites), WRAP
  `id=0x28C0E011^rotl(calcHash(name),27)` (localized menu text only).
- **Family structure is recoverable by per-bit majority vote.** A family (same object, members
  differ by one short token) has `id ^ stemhash = rotl(h(token), K)` → tiny residuals. Proven on
  the **FINN player directional family** (16 ids, stem hash `0x10b95810`, K=2, the 9 cardinal dirs
  = digits 1–9 clockwise from up) and an **enemy homing-projectile** trio (`0x4108xxf9`, stem
  `0x410808f9`). Sprites use `<base><sequential-token>` for directional/frame variants.
- **Aliases:** distinct ids share byte-identical art under unrelated names (the engine reuses art
  under multiple hashed names); they have no shared stem and can't be cracked jointly.
- **Categorization:** `categorize_families.py` joins families → referencing functions
  (`asset_usage_raw.csv`) → role. 68 families (47 sequence-style); code refs label Boss (Klogg,
  Glenn-Yntis), Enemy (Finn), Projectile (homing, clayball-debris), Player, Effect, Menu. 54 stay
  uncategorized (`refd=0`: loaded via data tables, not direct calls) — need visual ID.
- **Literal names remain unrecoverable** (preimage = word-salad; beta binary has no name table).
  Tools: `family_structure.py`, `categorize_families.py`.
