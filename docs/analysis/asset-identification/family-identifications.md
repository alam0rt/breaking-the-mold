---
title: "Family identifications (human visual ID, 2026-06-17)"
category: analysis/asset-identification
tags: [analysis, asset, identification, family, identifications]
---

# Family identifications (human visual ID, 2026-06-17)

Human (game-expert) visual identifications of the **unnamed** container super-families from
`super_families.csv` / `unnamed.html`. These are **role/entity labels, not cracked names** — the
literal ToolX hash-name is still unknown for every row (the manual/visual name usually is *not* the
ToolX filename; see `asset-hash-ids.md`). They are recorded so future agents can (a) seed targeted
`calcHash` guess-and-verify with the right entity vocabulary, and (b) avoid re-deriving the grouping.

Tooling: families from `tools/scripts/container_sisters.py`; gallery `unnamed.html` via
`tools/scripts/build_family_gallery.py --unnamed`; structure via `tools/scripts/family_structure.py`.

## Identified families

| stem hash | n | kind | levels | entity / role (human) | structure note |
|---|--:|---|---|---|---|
| `0x08ce0610` | 6 | sprite | CSTL;FOOD;SEVN;TMPL | **Background effects** — sun flares / lens blooms, parallaxed in bg | clean STEM+token |
| `0x081c8300` | 6 | sprite | FINN | **Riveted-metal tiles/textures** of the one 3D-ish level (FINN) | clean STEM+token |
| `0x081c8300` | 6 | sprite | RUNN | Same riveted-metal textures, **RUNN** world | clean STEM+token; FINN+RUNN are the special "3D-ish" stages (Asset-100 special level ID 99 = FINN/SEVN) |
| `0xba096800` | 5 | sprite | CAVE;EGGS;GLID | **"WORKER YNT"** enemies (manual name) | loose cluster — may merge a hash-neighbour |
| `0x00489818` | 4 | sprite | CLOU | **Flying enemies "FLAPPER" / "EGG BEATER"** (manual names) | clean STEM+token; `EntityType026_FlyingEnemy_Init` |
| `0x410808f9` | 4 | sprite | FOOD;TMPL | **Projectiles from the "head-shooter monkey"** enemy | **ALIAS cluster** (all 3468 B, byte-identical, +`0x326e0410`); `ProjectileState_HomingActiveVariant2` |
| `0x1cf99931` | 3 | sprite | BOIL;BRG1;CAVE;CLOU;CRYS;CSTL | **KLAYMEN "bounce" animation** (bounces on Ma-Bird + enemies, Mario-style) | **ALIAS cluster** (all 29672 B byte-identical, +`0x52ee0118`) — user confirmed art identical |
| `0x980061a3` | 3 | sprite | BOIL;BRG1 | **MA-BIRD** (checkpoint Klaymen bounces on) | `0x880161a7`≡`0x91106183`≡regional `0x620ec210` (17980 B ALIAS); `0x980861a3` is a distinct pose. **Ties Round-6 localized bank `0x620ec210` to MA-BIRD.** |
| `0xbc68d0c6` | 3 | sprite | BOIL;BRG1;CAVE;CLOU;CRYS;CSTL | **Enemy projectiles** (various enemies — uncertain) | unique art each; `PlayerCallback_DeathDebrisSpawner` / `BossCollision_SpawnDebrisAndLayers` |
| `0xd127c005` | 3 | sprite | PHRO;SCIE | **KLAYMEN idle animations** (plays after idling a while) | loose cluster; unique art each |
| `0x60b91c9c` | 9 | sprite | BRG1;PHRO | **Various monkey enemies** | clean STEM+token; `EnemyPatrolState` |
| `0xba700481` | 4 | sprite | FOOD | **Clayball + clayball-related interactive items** (clayballs = the game's collectible "coins") | loose cluster; `0xb8700ca1` is the clayball; raw-namespace (NOT the power-up pickup prefix) |
| `0xe4830101` | 3 | sprite | BRG1;PHRO | **BARKING BIRD enemy** (dangerous while Klaymen is shrunk; squishable at full size) | loose cluster; `EntityGroundSnapMovementCallback` |

## IMPORTANT: entity ≠ hash-family (2026-06-17 refinement)

Human review found the original `popcount(xor)≤6` clustering **over-merged** — transitive
union-find chained unrelated hash-neighbours (A~B, B~C, but A far from C), and the cross-container
merge snowballed it (e.g. a 57-member blob mixing WILLIE/ONE_UP pickups + a bg chain + the Shriney
Guard boss + tiles). The clusterer was rebuilt to **tight Hamming-ball families**: a family is a
maximal set all within **radius 3** of a common majority-vote stem. Validated against human ground
truth — keeps FINN-directional (16) and KLOG-death (5) intact, drops every flagged outlier
(WILLIE/timer/chain-bg/ONE_UP) to a singleton. Result: 70 → **25 tight super-families**.

This exposed the core truth: **an entity is NOT a hash-family.**
- A **hash-family** = the frame/direction run of ONE animation (`STEM + <single short token>`),
  a tight radius-≤3 ball. The algorithm finds these.
- An **entity** (a boss, an enemy) owns MANY sprites scattered across hash space, because each
  animation's action word (`WALK`/`IDLE`/`ATTACK`…) is multi-character and toggles many bits, so
  different animations of the same creature are NOT hash-neighbours. Grouping these needs human
  knowledge (the tables below) or the action-suffix back-solve (`fx_sprite_families.py`), not
  Hamming distance.

### Tier 1 — tight hash-families (algorithm-confirmed, R≤3), corrected stems

| tight stem | n | entity / role |
|---|--:|---|
| `0x08ce0610` | 6 | background sun-flare / bloom FX |
| `0x081c8300` | 7 | riveted-metal tiles/textures (FINN/RUNN 3D-ish levels; also tagged "curtain/wall") |
| `0x60a81c9c` | 5 | monkey enemies (tight core; `EnemyPatrolState`) |
| `0x410808f9` | 3 | head-shooter-monkey projectile (alias cluster) |
| `0x1cf99931` | 3 | KLAYMEN bounce (alias cluster) |
| `0xbc68d0c6` | 3 | enemy projectiles (various) |
| `0x60a81c9c` | 5 | **monkey enemies (EL BARFO + skull-monkey) — JUMP animation** (BRG1) |
| `0x60b9bcbd` | 3 | **same monkeys — IDLE/static animation** (BRG1) |
| `0x3080860d` | 3 | **MENU "gear" / animated background assets** |

### Entity links across tight families (same entity, different animation)

- **BRG1 monkey entity** (EL BARFO + skull-monkey) owns at least two tight families:
  JUMP `0x60a81c9c` and IDLE `0x60b9bcbd` — they sit **6 bits apart** (a multi-char action-word
  swap), so tight clustering correctly keeps them separate while they are one creature. This is the
  textbook entity≠hash-family case.
- **Action back-solve attempted, vocabulary unconfirmed:** if these were `MONKEY+JUMP` /
  `MONKEY+IDLE`, the stem-XOR (`0x0011a021`, pc 6) would equal `T(sh,"JUMP") ^ T(sh,"IDLE")` at the
  shared post-entity shift. It matches at **no** shift (off-by-6) — JUMP/IDLE are not the literal
  action words (expected; manual≠ToolX). **Triangulation path:** with ≥4 labelled animations of ONE
  entity, majority-vote across their family stems approximates the entity-prefix hash; the per-family
  residual is then a pure action word over a small vocabulary, which IS brute-forceable. Need more
  same-entity animation labels to pin it.

### Tier 2 — thematic entity groups (human knowledge; NOT hash-siblings, dissolved by tight clustering)

These IDs belong to one entity/theme but are spread across hash space (different animations / two
distinct enemies / alias+pose). Keep the label at the **sprite level**, not as a hash-family.

| entity / theme | member IDs | why not a hash-family |
|---|---|---|
| FLAPPER / EGG BEATER flyers | 0x004a981c;0x004c9138;0x024e981c;0x40489938 | **two different enemies** |
| clayball "coin" items | 0x1b301085;0xb8700ca1;0xbb781481;0xfa700ce7 | various distinct interactive items |
| MA-BIRD checkpoint | 0x880161a7;0x91106183(≡regional 0x620ec210);0x980861a3 | 2 aliases + 1 distinct pose |
| BARKING BIRD | 0xb4011101;0xe4834384;0xe6830101 | multiple animations (action-set) |
| WORKER YNT | 0x3a186940;0x3a186980;0xb84968b0;0xba094800;0xf8494814 | multiple animations (action-set) |
| KLAYMEN idle | 0xd167c005;0xd305d045;0xf936c015 | distinct idle animations |

### Monkey Mage boss (WIZZ level) — full animation set (human ID)

Entity = the **Monkey Mage** = the WIZZ-level boss (= "Wizard boss" in older notes). All one entity,
scattered across hash space (entity≠family). Cross-confirmed: `0x492c8430` is a Round-6 localized
WIZZ boss bank.

| id | animation |
|---|---|
| `0x60de1050` | death / being-hurt |
| `0x82421850` | floating / on-its-side |
| `0xa0cc18d0` | the Mage's staff (object) |
| `0x022e3442` ≡ `0x02ed1470` ≡ `0x492c8430` | casting — **alias cluster** (128800 B, 3 hash-names → 1 art) |
| `0x181c3854` | health bar (frame 0 = full … last = empty) |

Triangulation back-solve attempted (majority-vote entity-prefix `h_p≈0x004e1850`, residuals
4–8 bits): **no action vocabulary resolves** — too many simultaneous unknowns per ID (entity string
+ action word + frame token). Confirms name recovery stays blocked even with a fully-labelled boss;
the value is the role/entity catalog, not the literal names.

## Cross-cutting findings

- **Alias clusters ≠ STEM-token variation in art, but DO share a name stem.** Several "families"
  found by hash-neighbour clustering are byte-identical alias clusters (same art, multiple
  hash-names). Hash-neighbour + byte-identity together means the names were authored as
  `STEM + <short variant>` all pointing at one shared art file (e.g. KLAYMEN bounce ×4 names →
  one 29672-byte sprite). So an alias cluster is still a real naming family; cracking one name
  gives the stem for the rest.
- **MA-BIRD = regional bank `0x620ec210`** (one of the 23 Round-6 localized sprite ids). First
  concrete entity tied to the localization analysis.
- **FINN + RUNN share the riveted-metal texture family** (`0x081c8300` stem). These are the
  game's special "3D-ish"/vehicle stages (Asset-100 special level ID 99 = FINN/SEVN).
- **KLAYMEN families** (bounce `0x1cf99931`, idle `0xd127c005`) are Skullmonkeys-specific — they
  are NOT among the 5 ScummVM-Neverhood cross-reference hits, so the Neverhood source won't name
  them directly (unlike idle-blink `0x5900c41e`).
- Direct `calcHash` of the manual names (WORKERYNT, FLAPPER, EGGBEATER, MABIRD, KLAYMEN,
  BARKINGBIRD, CLAYBALL, MONKEY…) and a prefix×stem×suffix sweep hit **0** family members.
  Bit-distance of `calcHash(manual_name)` to the family stem/members is **8–14 bits = the noise
  floor** (no signal) — the literal ToolX strings are not the manual names and are not close.
  Names stay blocked; these labels are wordlist seeds only.
- The **power-up pickup prefix convention** (`id = calcHash(PREFIX + ITEM)`, PREFIX state
  `h=0x88200080, sh=27`, verified against FARTHEAD/GROW/ONEUP/UNIVERSEENEMA1/WILLIE/HAMSTER) does
  **NOT** apply to clayballs — clayballs are basic currency in the raw namespace, a different code
  path (`InitEntityState_Animated`), not the special-item pickup init.

## Role → code mapping (for the decomp, independent of asset names)

The family→entity labels are directly useful to the decompilation even without hash-names: each
family's calling function ties a known entity to engine code. Confirmed pairings:

| entity (human) | family stem | handling function(s) |
|---|---|---|
| monkey enemies | `0x60b91c9c` | `EnemyPatrolState` |
| BARKING BIRD | `0xe4830101` | `EntityGroundSnapMovementCallback` |
| WORKER YNT | `0xba096800` | (table-driven) |
| FLAPPER / EGG BEATER | `0x00489818` | `EntityType026_FlyingEnemy_Init` |
| head-shooter projectile | `0x410808f9` | `ProjectileState_HomingActiveVariant2` |
| Shriney-Guard boss audio | `0x004e44c0` | `ShrineyGuardSoundUpdateTick` |
| clayball currency | `0xba700481` | `InitEntityState_Animated` |
