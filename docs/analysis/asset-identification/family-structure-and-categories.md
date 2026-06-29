---
title: "Asset-ID families: structure cracking & categorization (2026-06-16)"
category: analysis/asset-identification
tags: [analysis, asset, identification, family, structure, categories]
---

# Asset-ID families: structure cracking & categorization (2026-06-16)

Follow-on to [`docs/reference/asset-hash-ids.md`](../../reference/asset-hash-ids.md). That doc
establishes the hash (Neverhood `calcHash`, a 32-bit cumulative-shift XOR bit-toggle).

**On "recoverable":** the hash cannot be *inverted* (it's lossy — billions of strings map to each
value), so names are only ever recovered by **guess-and-verify**: propose a plausible string, hash
it, confirm it matches. That succeeds when the naming convention is *guessable* and a hit is
*verifiable*. It worked well for **audio** (`FX_<ENTITY>_<ACTION>_<NN>` — semantic, short, with
sibling families like `FIRE_01/02` to corroborate) and for **literal menu text** (`PAUSED`,`QUIT`).
It has *not* yet worked for gameplay-sprite **base/stem names** — those are internal ToolX art
filenames (non-semantic, ~12 chars), and our vocabularies haven't guessed them. That's "unrecovered
by current methods," **not impossible**: a verified hit is still possible with the right vocabulary,
especially for corroborated targets like the FINN stem (§3). This doc records what we *can* already
recover without the literal strings: the **naming structure** of families and their **roles**.

## 1. One hash, with section prefixes (the "two namespaces" are the same calcHash)

**UPDATE (2026-06-17, Round 21):** there is **one** hash, `calcHash`. What looked like a separate
"WRAP namespace" is just `calcHash` with a fixed **prefix**. Via the concat identity
`calcHash(A+B).h = h_A ^ rotl(calcHash(B).h, sh_A)`, the WRAP formula
`id = 0x28C0E011 ^ rotl(calcHash(name), 27)` is exactly `calcHash(MENU_PREFIX + name)` where the
menu prefix has hash-state `(h=0x28C0E011, sh=27)` — i.e. **`27` is the prefix's leftover shiftValue,
not a magic rotation**. Confirmed: continuing `calcHash` from that state over each menu word
reproduces all six menu IDs exactly. See `../../reference/asset-hash-ids.md` Round 21.

| Model | Formula | Really is | Used by |
|---|---|---|---|
| **RAW** | `id = calcHash(name)` | empty prefix `(0,0)` | audio (`FX_<ENTITY>_<ACTION>[_NN]`) + gameplay sprites/anims |
| **"WRAP"** | `id = 0x28C0E011 ^ rotl(calcHash(name), 27)` | `calcHash(MENU_PREFIX + name)`, prefix state `(0x28C0E011, 27)` | menu text (PAUSED/QUIT/CONTINUE/YES/NO/QUITGAME) |
| pickup | `id = calcHash(ITEM_PREFIX + ITEM)` | prefix state `(0x88200080, 27)` | item pickups (FARTHEAD/GROW/ONE_UP/…) |

Both identified prefixes end at **`sh=27`** (possible shared parent path). `calcHash` is
case-insensitive and ignores non-alphanumerics (`QUIT GAME` == `QUITGAME`, `_` is free). A section
prefix is a **shared bottleneck**: cracking one prefix string unlocks every asset in that section
(suffixes already known). The two ScummVM Klaymen anchors (`0x5900c41e`,`0x9d406340`) are *role*
identifications via shared Neverhood hashes, not recovered strings.

## 2. Blind brute-force doesn't work for long sprite names (but guess+verify can)

`calcHash` is lossy — **billions of strings map to each 32-bit id** — so a *blind* dictionary/grammar
sweep over unknown long names drowns in collisions. Evidence (don't repeat blind sweeps): rockyou
(3.76M words) + a 10M-name semantic grammar, both models, vs all 405 uncracked sprites → **0 hits on
the 19 visually-labelled ids**; a 2-word meet-in-the-middle over the full 13.4M rockyou list on the
"Loading" sprite `0x8ab92024` → only username-salad; a preimage enumeration of the FINN stem hash →
209 valid-but-gibberish word-salads (`finn_stem_candidates.csv`). This shows blind brute is the wrong
tool — **not** that the names are unguessable: audio cracked precisely because the `FX_` convention +
short semantic words + sibling corroboration made guess-and-verify tractable. A confirmed sprite stem
(or the ToolX naming convention) would make sprite guess-and-verify tractable too. The beta binary
(`disks/prototype/ext/SLUS_006.01`) carries **no asset-name strings** — only CD filenames, PSY-Q
symbols, and debug strings that print *ids*
(`MISSING SEQ $%x`). Literal names require an external corpus (the lost ToolX asset tree).

## 3. Family STRUCTURE *is* recoverable (the breakthrough)

A **family** = several ids that are the same object differing by one short token (direction /
frame number / variant). Because `calcHash(STEM + TOKEN) = h(STEM) ^ rotl(h(TOKEN), endshift(STEM))`,
a **per-bit majority vote** over the family's ids recovers the shared stem's hash, and each id's
residual `id ^ stemhash` collapses to a tiny value (`rotl(h(token), K)`) iff the family really is
`STEM+TOKEN`. Tool: `tools/scripts/family_structure.py <hex ids…>`.

### Proven: FINN player directional family (16 ids, level FINN)
- shared stem `calcHash = 0x10b95810`, end-shift `K = 2`
- the 9 cardinal directions are **single digits 1–9, clockwise from "up"**: `up=STEM+"1"`,
  `up-right=+"3"`, `right=+"5"`, `down-right=+"7"`, `down=+"8"/"9"` (verified to the bit: "up" →
  residual bit 9 = `K + val('1')` = `2 + 7`).
- the 6 left-facing directions add a constant `0x0c` marker.
- 15/15 fit exactly — mathematically real, not a collision. The literal STEM string is a ~12-char
  name we cannot invert.

### Proven: enemy homing-projectile family (`ProjectileState_HomingActiveVariant3`)
- ids `0x410800f9 / 0x410808f9 / 0x410818f9`, shared stem `calcHash = 0x410808f9`, residuals are
  single bits at consecutive positions 11/12 → `STEM + <sequential char>`. Same convention as FINN.
- **Sprites use `<base><sequential-token>` for directional/frame variants** — confirmed twice.

### The alias phenomenon
Distinct ids can share **byte-identical art** while having unrelated names (huge residuals). E.g.
the projectile art also appears as `0x000cc8f8` and `0x326e0410` (residual popcount 6 and 16). The
engine reuses one art asset under multiple hashed names (per-enemy / per-level references). Aliases
cannot be cracked jointly — there is no shared stem. (Detected via identical-frame md5 grouping;
note the largest "identical" groups are blank/filler tiles, an extraction artifact, not a clue.)

## 4. Categorizing families by code reference (no names needed)

`tools/scripts/categorize_families.py` joins each XOR-sibling family (`cluster_families.csv`
connected components, size ≥ 3) to the decompiled functions that reference its member ids
(`asset_usage_raw.csv`) and infers a category. Output: `family_categories.csv` / `.md`.

- **68 families** (size ≥ 3); **47 are sequence-style**; 350/658 ids are referenced in decompiled
  code (call type tells sprite vs audio vs discriminator-token: `SetEntitySpriteId`/`InitEntitySprite`
  = sprite, `PlaySoundEffect` = audio, `== 0x…` = token).
- Categorized via code refs:

| stem_hash | n | category | driving function | level |
|---|---|---|---|---|
| `0x800da112` | 5 | Boss | `KloggDeathEventHandler` | KLOG |
| `0x18e88110` | 3 | Boss | `GlennYntisDeathEventHandler` | GLEN |
| `0x2e819804` | 3 | Enemy | `FinnDamageEventHandler` | KLOG |
| `0x400c981d` | 3 | Enemy | `InitEnemyAnimatedWithDeathSpawn` | BOIL… |
| `0x410808f9` | 3 | Projectile | `ProjectileState_HomingActiveVariant3` | FOOD;TMPL |
| `0x463c6040` | 4 | Projectile | `ClayballState_DestroyWithDebris` | BOIL… |
| `0x5a99815f` | 3 | Effect | `SpawnCollectibleParticles` | — |
| `0x1cf99931` | 3 | Player | `PlayerStateInit_JumpFromPlatform` | — |
| `0x3080860d`,`0x40b08011`,`0x38a0c119` | 3 | Menu/UI | `InitMenuStage1` | MENU |

- **Known from visual ID** (user-confirmed, refd=0 because loaded via data tables): the
  `0x10b95810` family (16 ids, FINN) = **Player directional aim**; Ma-Bird/checkpoint
  (`0x09406d8a…`) and pickups (1-up `0xa9240484`, clay-ball `0xb8700ca1`, green-bullet `0xe8628689`,
  Willie-trombone `0x902c0002`, swirly-Q `0x80e85ea0`) are NOT sequence-style (named per-state with
  whole different words, or aliases).
- 54 families are `Uncategorized`, almost all `refd=0` — their ids are loaded from data tables, not
  direct code calls, so no function references them. Visual identification (the `klay/` review site)
  is the way to label these.

## 5. Full asset-access catalog (every known id)

`tools/scripts/scan_asset_access.py` scans all 144,869 lines of `export/SLES_010.90.c`, finds every
occurrence of each known asset id, tracks the **enclosing function**, classifies the **access
mechanism**, and assigns a category from the referencing function names. Output:
`asset_access_catalog.csv` (per-id) and `asset_access_summary.md`.

- **365 / 658 ids are referenced in decompiled code; 293 are loaded via data tables** (no direct
  code reference — these need visual ID via the `klay/` review site).
- Access mechanisms (same id can appear many ways): `setspr` (SetEntitySpriteId) 245, `init`
  (InitEntitySprite) 147, `sound` (PlaySoundEffect) 42, `token` (`== 0x…` discriminator) 38,
  `alloc`/`table` rare, plus wrapped/`other` call sites. So most assets are reached as a **sprite
  on an entity**; a smaller set are **discriminator tokens** compared in event/state code.
- Categories (referenced ids), inferred from the driving function:

| category | n | example driver |
|---|---|---|
| Boss | 85 | `InitKloggChaseState`, `JoeHeadJoeBall*`, `GlennYntisDeathEventHandler` |
| Player | 70 | `PlayerState_*`, `PlayerProcessTileCollision`, `Soar*FlightMode` |
| Enemy | 65 | `EntityType026_FlyingEnemy_Init`, `Glider*State`, `Finn*State` |
| Collectible | 39 | `CollectiblePhartHeadTickCallback`, `CheckpointSwampTickCallback` |
| Projectile | 33 | `ProjectileState_HomingActiveVariant3`, `EntityEventHandlerSpawnProjectile` |
| Menu/UI | 24 | `InitMenuStage1`, `ToggleMenuOptionState` |
| Teleport/Decor | 22 | `TeleporterPortalEventHandler`, `DecorStartWithRandomTimer` |
| Effect/Particle | 12 | `SpawnCollectibleParticles`, `EntityDestroyWithEffects` |
| Uncategorized (referenced) | 13 | generic `InitEntity*` |
| **Table-only (no code ref)** | **293** | loaded from level/entity data tables |

This confirms several user visual-IDs from the code side: Ma-Bird `0x09406d8a` → `CheckpointSwampTickCallback`;
the projectile alias `0x000cc8f8` → `ProjectileState_HomingActiveVariant3`; power-up `0x08624580`
→ `CollectiblePhartHeadTickCallback` (Phart-Head). The 293 table-only ids are the visual-ID frontier.

## 6. BLB side: entity-type census (the other reference layer)

The 293 "table-only" ids are **not** unused — they're referenced in **BLB data**, not code
literals (274/293 have BLB level coverage; the `levels` field is derived from the BLB container
TOCs). There are two reference layers:

| Layer | Mechanism | Coverage |
|---|---|---|
| **Code literal** | `InitEntitySprite(e, 0x…, …)` hardcoded in a function | 365 ids (§5) |
| **BLB data** | id stored in a level's 600/601 container; loaded at runtime, used as a variable | the 274 |

**Key structural finding:** BLB **Asset 501 entity records** carry `entity_type` + position, **not
asset ids** — so there is no direct entity→asset-id table in the BLB. Asset ids are supplied by the
entity's `EntityType###_*_Init` function (code; the 365) or via in-BLB sprite-animation
cross-references (Asset 600/503). So the 274 can't be given a precise per-id role from the BLB alone.

What we *can* build (`tools/scripts/entity_census.py`, output `entity_census.csv`/`.md`): a per-level
**entity-type census** — every entity placement across all 104 stages, mapped to a role via the
`EntityType###_*_Init` symbol names. This gives the role *context* for each level's assets:

- Game-wide, type 2 = `GreenBullets` (5727 placements — ammo everywhere), then pickups, platforms,
  `UniverseEnema`/`PhartHead` power-ups, `ClayKeeperEnemy`, `ExtraLife`, particles.
- **Boss levels each have exactly one unique high-type entity** = the boss: WIZZ→type141, MEGA→142,
  GLEN→143, HEAD→144, KLOG→145. So those levels' single-level assets are that boss's assets.
- FINN (swimming): GreenBullets + PhartHead + special types — the directional-player family
  (`0x10b95810`, §3) lives here. RUNN (auto-runner): 604 GreenBullets. MENU: types 161–166.

So a single-level asset inherits strong role context from its level's entity roster (a boss level's
unique assets are boss assets; FINN's are swim-mode; etc.), even without a precise per-id link.
`RemapEntityTypesForLevel @0x8008150c` makes `entity_type` layer-dependent — we apply the documented
25/27 layer-1 pickup remaps; other gameplay (layer-2) types are identity. See
[`docs/blb/asset-types.md`](../../blb/asset-types.md) §501 and
[`docs/reference/ENTITY_REMAPPING_CORRECTION.md`](../../reference/ENTITY_REMAPPING_CORRECTION.md).

## 7. BREAKTHROUGH: sprite names are `STEM + <ACTION>`, actions = the FX vocabulary (2026-06-17)

The biggest crack into gameplay-sprite naming so far. Method and findings:

**The idea.** A character's **sound** family (e.g. `FX_BOSS_HEAD_HIT/TURN/WALK/IDLE_01`) and its
**sprite** animation family use the *same action suffixes* (`HIT`, `TURN`, `WALK`, …). The sprite
family is `STEM_sprite + ACTION`; the sound family is `FX_<entity>_ + ACTION`. Both encode the
identical action words, so the two families have the **same internal hash spacing** — and a single
`(seed, rotation)` transform maps one onto the other. Sweeping all 32 rotations for a transform that
lands ≥2 verified FX cores on real sprite IDs surfaces these matches.

**Validation discriminator (critical).** A real match must also agree with **code identity + level**.
- ✅ `BOSS_HEAD_HIT/TURN/WALK` → sprite IDs all in the **HEAD** level, all referenced by **`JoeHeadJoe*`**
  functions (the boss-head boss is JoeHeadJoe).
- ✅ `KLAY_DIE_FALL/DUCK_DOWN/RUN_FAST` → all **Player** sprites, referenced by `PlayerState_*`.
- ❌ A bare-item pickup "triple" (`FARTHEAD/GROW/ONE_UP`) hit 3 real IDs too but **failed** this test
  (mapped `FARTHEAD`→ the *PhoenixHand* sprite; its prefix reversed to garbage) → coincidence.
  So code-identity validation is what separates real families from collisions.

**Solving a family's stem.** With the action suffixes known, `id_action = h(STEM) ^ rotl(h(action),
sh_STEM)`; the `sh_STEM` that makes all members agree on one `h(STEM)` gives the stem hash, and then
**predicting `STEM + other_action` finds the rest of the family**:
- **Boss head** — `stem=0x0a3809b2, sh=11`, 7 sprites: `HIT 0x1a3109b2`, `TURN 0x8a3809f2`,
  `WALK 0x0e3889be`, `IDLE_1 0x0b290ba2`, `DIE 0x2b3889b2`, `ATTACK_1 0x0b081dbb`, `ATTACK_2 0x0b0811bb`.
- **Player (KLAY)** — `stem=0x18288010, sh=21`: `DIE_FALL 0x1e28e0d4`, `DUCK_DOWN 0x0a3a4051`,
  `RUN_FAST 0x092b8480`, `FALL 0x00388110`.
  (`IDLE_1/DIE/ATTACK_1/2`, `FALL` were *predicted* then confirmed present — not chance.)

**What's recovered vs still opaque.** The **action half of the name is recovered** (real strings:
`HIT, TURN, WALK, IDLE, DIE, ATTACK, FALL, DIE_FALL, DUCK_DOWN, RUN_FAST`). The **entity STEM is still
opaque** — it is *not* the sound's entity word (sounds say `BOSS_HEAD`/`KLAY`; the sprite stem is a
different ~6–11-char string that doesn't reverse to a clean word). So full literal names still need
the stem, but every entity's sprite *set* can now be grouped + action-labeled, and new IDs predicted.

Tools: `tools/scripts/fx_sprite_families.py` (transform sweep + stem solve + family map), output
`fx_sprite_families.csv`/`.md`.

## 8. BREAKTHROUGH: pickup sprite naming convention cracked via code co-occurrence (2026-06-17)

The first *cracked sprite naming convention* beyond menu text — found by tracing code behavior,
not guessing.

**Method (the key move).** Gameplay sounds aren't code literals, but they DO appear in the same
function that sets the entity's sprite (via `PlayEntityPositionSound` etc.). So a function that
references verified `FX_PICKUP_FARTHEAD` and calls `SetEntitySpriteId(e, 0x8c510186)` tells us
`0x8c510186` *is* the Phart-Head pickup sprite — behaviorally, independent of the (guessed)
function name. Tool: `tools/scripts/trace_sound_sprite.py`. This gave **correct (item ↔ sprite-id)
pairings** (which we'd been guessing wrong all along).

**The convention.** With correct pairings, the pickup sprites solve to
`sprite_id = calcHash(PREFIX + ITEM)` where `calcHash(PREFIX) = 0x88200080, end-shift 27`. Confirmed
by **5 exact matches**:

| pickup | sprite id | item string |
|---|---|---|
| Phart-Head | `0x8c510186` | `…FARTHEAD` |
| Grow | `0x8c30008c` | `…GROW` |
| 1-up | `0xa9240484` | `…ONE_UP` |
| Universe-Enema | `0x6a351094` | `…UNIVERSE_ENEMA_1` (note the frame suffix) |
| Super-Willie | `0x902c0002` | `…WILLIE` (sprite drops the `SUPER_`) |

5 items fitting one transform on independently-code-derived IDs is not chance. (1970/GLIDEY/PHOENIX
use different item strings — their sprites are set in `Init1970IconEntity` / `InitYellowBirdCollectible`
/ `InitPhoenixHandCollectible`, so the words are `…ICON`/`…YELLOWBIRD`/`…HAND`-ish, not yet pinned.)

**Status.** The pickup-sprite **PREFIX** (`calcHash 0x88200080`, sh27, popcount 4 → ≥4 chars) is the
single shared unknown — it does not reverse to a clean word (brute → garbage), like the entity stems.
One prefix word would complete all pickup names and likely generalize. The earlier "FARTHEAD→0x8c510186
coincidence" I dismissed was actually this real convention — code identity (`InitPhartHeadCollectible`)
confirmed it.

Tools: `tools/scripts/trace_sound_sprite.py` (sound↔sprite code co-occurrence).

## Tooling
- `tools/scripts/entity_census.py` — per-level entity-type → role census from BLB Asset 501 → `entity_census.csv`/`.md`.
- `tools/scripts/family_structure.py` — bit-vote a family's ids → shared stem hash + residual/token structure.
- `tools/scripts/categorize_families.py` — family ↔ function ↔ category synthesis → `family_categories.csv`/`.md`.
- `tools/scripts/scan_asset_access.py` — full per-id access catalog → `asset_access_catalog.csv`/`asset_access_summary.md`.
- `tools/scripts/finn_stem_brute.py` — preimage enumeration (demonstrates name-recovery futility).
- `tools/scripts/build_review_sheets.py`, `build_anim_gallery.py` — visual review sheets/animations (served at `/mnt/share/public/www/klay/`).
