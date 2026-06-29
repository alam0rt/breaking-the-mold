---
title: "Skullmonkeys (PSX) Asset-Hash Reverse Engineering"
category: gist/asset-system
tags: [gist, asset, system]
---

# Skullmonkeys (PSX) Asset-Hash Reverse Engineering

Cracking the ToolX asset-ID naming scheme for Skullmonkeys (1998, The Neverhood Inc. / EA),
working from a 658-row asset-ID list (`ids.csv`) and the `alam0rt/evil-engine` Ghidra decomp.
This README ties together 12 working sessions. Files are listed at the bottom.

> **Confidence note.** `calcHash` is lossy/many-to-one and the shipped game has **no asset-name
> strings** (hashed away at build time; confirmed via BLB format + ScummVM). So "recovered"
> means **unique plausible preimage** within the naming grammar — effectively true, but only
> externally *proven* for the 3 ScummVM-confirmed assets. Roles (what an asset is) are RE
> labels, not original strings.

---

## 1. The hash function (validated byte-exact vs ScummVM)

`calcHash`: case-insensitive, alphanumeric-only, per-char **single-bit XOR toggle** with a
running `shiftValue` accumulator mod 32; digits map to values 6–15 (so digits alias letters
F–O). Verified identical to ScummVM's reverse-engineered Neverhood `calcHash` (resource.cpp).

**Key property — it's a homomorphism over concatenation:**
```
calc(A + B) = calc(A) XOR rotl(calc(B), sigma(A))      sigma = sum of char-values mod 32
```
Consequences used throughout:
- `popcount(calc(name))` <= name length, and equals it in parity (a length oracle).
- A fixed prefix == XOR by a constant + a fixed rotation.

## 2. The namespace model

```
id = SEED  XOR  rotl( calcHash(assetname), R )
```
`SEED` and `R` are **per-asset-ROLE prefix constants** ("prefix keys"). Four are known:

| Namespace | SEED | R | Used by | Anchors |
|---|---|--:|---|--:|
| RAW | `0x00000000` | 0 | audio `FX_*` (literal `FX_` in name) + some sprites | 113 |
| TEXT | `0x28C0E011` | 27 | on-screen text glyphs (all 26 levels) | 6 |
| PICKUP | `0x88200080` | 27 | power-up pickups + in-level HUD icons | 6 |
| BOSS | `0x08280150` | 27 | boss/mini-boss sprites | 21 |

- `R=27` is shared by 3 namespaces ⇒ it's a **hardcoded rotate in the routine**; the prefix
  is a per-role seed constant.
- The seed is **NOT derived from the BLB asset-type number** (100/600/601…). Proven: type
  numbers don't hash to any seed; type-as-suffix is mathematically impossible; and RAW spans
  both container types 600+601 while three different keys all live inside type 600. Namespace
  tracks **role**, not type. (Session 8.)
- Prefix *strings* are not recoverable (opaque constants; no word/folder preimage).

## 3. Names recovered (audited ledger)

| Grade | Count | Meaning |
|---|--:|---|
| RECOVERED | 131 | unique plausible preimage in the naming grammar |
| RECOVERED-EXT | 3 | + externally confirmed via ScummVM/Neverhood |
| CANDIDATE | 11 | one weak alternative collides (listed) |

Method: collision-multiplicity audit — generate the convention-valid name space, bucket by
hash, accept only unique hits; self-cancelling token artifacts filtered. BOSS-21 stayed unique
under a doubled (98-word) action vocab. (Sessions 1, 3, 4.)

New categories/names cracked across sessions: the **BOSS sprite namespace** (21 boss
animations: WIZARD/MEGA/KLOGG/YNT) via a leave-one-boss-out–validated seed (210× signal vs
null, cross-confirmed by matching MEGA's sprite actions to its verified audio actions); the
**HUD digit font** `STATUSNUMBERS` (PICKUP namespace); and **`FX_BUTTON_PAUSE` / `FX_BUTTON_UNPAUSE`**
— triple-validated via trigger context (played by `PauseGameAndShowMenu` / `PauseGameWithFadeOut`),
verified `FX_BUTTON_*` grammar, and uniqueness. (Session 10.)

**The strongest result class = trigger-context + uniqueness agreeing.** A hash hit alone is
not enough; PAUSE/UNPAUSE qualified because the function that plays them names the action.
Grammar-only "unique" hits with no trigger validation were caught as chance collisions and
rejected (Session 11) — discipline preserved.

## 4. Structure (durable, independent of name recovery)

- **Sister families** (`master_families.json`): 52 radius-≤3 hash families covering 167
  uncracked ids. Members share a base name, differ only in a frame/direction token. Naming one
  names the whole family.
- **Entity groups** (`entity_groups.json`): radius-≤5 + level-guarded grouping reassembles a
  single creature's multiple animations (validated: rebuilds the full MEGA/YNT/WIZARD sets;
  added new siblings e.g. a 5th YNT animation, 3 player-swim frames).
- **Role map** (`id_role_map.json`): **87 sprite-id → role bindings** (84 role-known but
  unnamed), from the 121-entry entity-callback table + the doc entity-type table. Identifies
  BossMain/BossPart, EnemyA, Clayball, Portal, MessageBox, ExplosionDebris, Player variants,
  etc. — purpose recovered even where the string isn't.
- **Usage context** (Sessions 9–11): sprite↔sound↔particle bindings (e.g. WIZARD_FALL plays
  SFX `0x424129A1`); render-layer z-order bands (10000=HUD, 2000=entities, 1000=bg props);
  the **password-entry screen** category (`CreateBossPlayerEntity` is a decompiler misname —
  cursor/digit/back-button/symbol-grid sprites); the **boss component tree** (`InitBossEntity`
  wires BossMain+BossPart+sub via state callbacks); **player-variant→stage map**
  (`player_variants.json`: normal / Finn-swim / Soar-fly / Glide / Runn-run forms).
- **3 new asset IDs** found in the decomp but absent from `ids.csv` (`0x4A298690`,
  `0xAA0DA270`, `0xE4AC9451` — the 8×4 password symbol grid) — `decomp_discoveries.json`.

## 5. What's blocked, and why (honest boundary)

- `type500`/`type700` ids are **RAW per-level data assets** (tile-attribute maps / legacy SPU),
  **not name-hashed** — un-nameable by design (type700 is doc-confirmed unused).
- Opaque sprite/enemy/tile names: long + no naming grammar to constrain a guess, lossy hash,
  no on-disc string table. Short names are *worse* (popcount-≤5 ids have dozens of gibberish
  preimages each). **84% of uncracked sprites need ≥8-char names** (median floor 10). (Session 12.)
- **Sprite naming is per-entity, proven (Session 12):** the 8 player sprites have mean pairwise
  XOR distance 10.2 ⇒ independent multi-char names, NOT base+action (no entity+R decodes ≥3 of
  them; bare-action RAW = 0 hits). The BOSS seed decodes exactly its 4 bosses and **0 of 260**
  other single-level sprites ⇒ each non-boss enemy owns its own seed. `PlayerState_*` labels
  name the *code*, not the asset.
- Audio is mostly **data-table-driven**: only 12 of 114 uncracked sounds have a hardcoded
  trigger call site, capping the trigger-context oracle. (Session 11.)
- Skullmonkeys' Klaymen was re-animated from scratch: of ScummVM's 126 Neverhood Klaymen
  hashes, only **3** appear here — no shared player corpus to mine.

**Only path to more TRUE names:** a **per-entity anchor** — an emulator capture of one string
per creature → that entity's seed → its whole animation set decodes at once — or original
ToolX project files. Neither exists offline. The offline name-recovery surface is exhausted;
remaining sessions add *structure/role/usage* knowledge, not new strings.

---

## Files
| File | Contents |
|---|---|
| `skullmonkeys_findings.md` / `.json` | Session 1: scheme + namespace keys + initial 21 boss names |
| `decoder.py` | hash / encode / probe tool for all 4 namespace keys |
| `audited_ledger.json` | every name graded RECOVERED / CANDIDATE / EXT (+ alternatives) |
| `master_families.json` | 52 radius-3 frame-sibling families |
| `entity_groups.json` | radius-5 entity-level groups (ledger-anchored) |
| `sister_clusters.json` | full-corpus sister clustering |
| `id_role_map.json` | 87 id→role bindings (entity callbacks + doc entity-type table) |
| `sprite_roles.json` | function → spawned-ids role map |
| `decomp_discoveries.json` | 3 new asset ids, password-screen category, sprite↔sound bindings, boss tree |
| `player_variants.json` | Klaymen per-stage init functions → sprites |
| `SESSION2..12_*.md` | per-session writeups |

## Session index
2 decomp role map · 3 confidence audit · 4 HUD/menu (`STATUSNUMBERS`) · 5 ScummVM/Neverhood
ground-truth xref + calcHash validation · 6 sister/entity clustering · 7 per-level + radius-
tiered grouping · 8 BLB structure + entity-callback harvest + type-seed test · 9 callback
context + password screen + 3 new ids · 10 usage context oracle (`FX_BUTTON_PAUSE/UNPAUSE`) ·
11 full-corpus sound sweep + player variants + false-positive discipline · 12 sprite
naming-structure verdict (per-entity, quantified wall).
