---
title: "Skullmonkeys ToolX Asset-Hash Analysis"
category: gist/asset-system
tags: [gist, asset, system, skullmonkeys, findings]
---

# Skullmonkeys ToolX Asset-Hash Analysis

**Date:** 2026-06-17 · **Corpus:** 658 asset ids (`ids.csv`), 127 previously named

This documents the asset-id naming scheme used by the Skullmonkeys (PSX) build tool
("ToolX"), and reports newly cracked names + a newly discovered namespace key.

---

## 1. The hash function

Asset names are hashed with **`calcHash`** (the Neverhood/Skullmonkeys lineage hash,
reverse-engineered in ScummVM):

- case-insensitive, **alphanumeric-only** (all other chars ignored)
- a running `shiftValue` accumulator mod 32
- each accepted char **toggles a single bit**: `shiftValue += val; shiftValue &= 31; hash ^= 1<<shiftValue`
- digit rule: `'0'..'9'` map to values 6..15 — i.e. **digits alias letters F..O** (a hard collision)

### Key algebraic property (the lever for everything below)

`calcHash` is a **homomorphism** over string concatenation:

```
calc(A + B) = calc(A) XOR rotl(calc(B), sigma(A))
```

where `sigma(A)` = sum of A's char-values mod 32. Verified exactly on many inputs.

**Consequences used throughout:**
- `popcount(calc(name))` ≤ name length, and has the **same parity** as the length.
- Prepending a fixed prefix == XOR by a constant + a fixed rotation. So the id scheme is:

```
id = SEED  XOR  rotl( calcHash(assetname), R )
```

`SEED` and `R` are **per-asset-type constants** ("prefix keys"). `R=27` recurs across
multiple namespaces, which means **27 is a hardcoded rotate in the routine** and each
"prefix" is a seed constant — not necessarily a literal hashed string. (Prefix-string
preimage search is underdetermined: e.g. the TEXT seed needs ≥9 odd chars; thousands of
non-word preimages exist. Treat seeds as constants; confirm origin via a code trace.)

---

## 2. Namespace keys (the "prefix keys")

| Namespace | SEED | R | Used by | Anchors | Status |
|---|---|--:|---|--:|---|
| RAW | `0x00000000` | 0 | audio `FX_*` (literal `FX_` in the hashed name) | 113 | established |
| TEXT | `0x28C0E011` | 27 | on-screen text glyphs (`CONTINUE/QUIT/YES/NO/PAUSED`), all 26 levels | 6 | established |
| PICKUP | `0x88200080` | 27 | power-up pickups (`GROW/HAMSTER/ONE_UP/...`) | 6 | established (solved by sister-math) |
| **BOSS** | **`0x08280150`** | **27** | **boss/mini-boss sprites (`KLOGG/WIZARD/MEGA/HEAD/YNT`)** | **21** | **NEW — validated, in-game confirm pending** |

`CONTINUE → 0x69C04050` (a menu string) matching the sprite-derived TEXT seed exactly was
the first cross-domain confirmation of the model. The PICKUP seed was recovered with **no
anchor** purely from the sister-XOR of six `<PREFIX>` pickups, and matched the prior
independently-known value `0x88200080`.

---

## 3. NEW: BOSS sprite namespace — 21 decoded names

Seed `0x08280150`, R=27. All 21 round-trip exactly (`id = 0x08280150 ^ rotl(calc(name),27)`).

| id | reconstructed name | level |
|---|---|---|
| `0x022E3442` | WIZARD_ATTACK_01 | WIZZ |
| `0x02ED1470` | WIZARD_IDLE_01 | WIZZ |
| `0x424D1840` | WIZARD_DIE_01 | WIZZ |
| `0x60DE1050` | WIZARD_HIT_01 | WIZZ |
| `0x81421850` | WIZARD_FALL_01 | WIZZ |
| `0x82421850` | WIZARD_FALL_02 | WIZZ |
| `0x08192250` | MEGA_TURN | MEGA |
| `0x085860D4` | MEGA_ROLL | MEGA |
| `0x09382152` | MEGA_SPIT | MEGA |
| `0x0A1820D4` | MEGA_DIE | MEGA |
| `0x2C182010` | MEGA_HIT | MEGA |
| `0x40106054` | MEGA_IDLE_2 | MEGA |
| `0x4C106054` | MEGA_IDLE_1 | MEGA |
| `0x08BC8013` | KLOGG_HIT | KLOG |
| `0x18248010` | KLOGG_DIE | KLOG |
| `0x193CA112` | KLOGG_IDLE_1 | KLOG |
| `0x1A2C8516` | KLOGG_SHOOT | KLOG |
| `0x0C78005C` | YNT_DIE | GLEN |
| `0x407801D0` | YNT_HIT_1 | GLEN |
| `0x407801DC` | YNT_HIT_2 | GLEN |
| `0x8068815C` | YNT_IDLE_1 | GLEN |

> **Spelling caveat:** `calcHash` ignores `_` and case, so `WIZARD_ATTACK_01`,
> `WIZARDATTACK01`, `wizard_attack_1` all hash identically. The **hash/id is certain**; the
> exact human rendering (separators, frame zero-padding) is a best-effort reconstruction.
> `KLOGG` uses the double-G spelling from the verified audio names (level tag is `KLOG`).

---

## 4. Why we believe it (validation battery)

| Test | Result |
|---|---|
| Round-trip | 21/21 names re-hash to their exact id |
| **Leave-one-boss-out** | seed re-derived from 3 bosses predicts the 4th cold: KLOGG 4/4, WIZARD 6/6, MEGA 7/7, YNT 4/4 — each independently re-derives `0x08280150, R27` |
| Signal vs null | 21 coherent decodes vs **0.00 mean** over 12 random seeds (≈210×) |
| Cross-modal | decoded MEGA sprite actions `{DIE,IDLE,ROLL,SPIT,TURN}` == independently-verified MEGA **audio** actions |
| Action pinning | each id's target hash fits **exactly one** action word (no ambiguity); gibberish actions score max 2 vs 21 |

**Honest caveat:** every test above is *internal-consistency* (round-trip + cross-prediction
both rely on the hash math). Forward-predicted sibling frames (e.g. `MEGATURN02`) do **not**
appear in the id list — meaning either these animations are single-frame at the id level, or
use a frame encoding we aren't rendering. **One in-game anchor** (emulator breakpoint on the
hash routine, read `$a0` string in the MEGA level) would convert this from "very strong
inference" to "proven." Recommended before mass-committing names.

---

## 5. Namespace map (seed-free sister clustering)

Pairwise `popcount(id_i ^ id_j) ≤ 3` (XOR cancels the seed) groups uncracked sprites into
**44 same-base animation runs** covering 146 of 398 sprites. The BOSS seed explains 4 of
these; the rest are real families whose **seeds are still unknown**. Full cluster list in
the JSON. Notable: a **16-member FINN run** (the 3D vehicle stage) with its own seed (the
BOSS seed does not decode it).

---

## 6. The wall (what blocks the remaining ~377 sprites)

A frame counter constrains a cluster's seed only **up to the unknown base-name hash** — so
frames alone can never yield a seed. Each new namespace needs **either**:
1. an **anchor** (one known name↔id, e.g. via emulator breakpoint), or
2. a **correctly-guessed base name** for one cluster (then the seed pops out and the whole
   cluster + every sibling in that namespace decodes).

Brute-forcing seeds offline is exhausted: an implied-seed recurrence sweep (44 clusters ×
~30k name-guesses × 32 rotations) surfaced **no** seed above the BOSS seed; all other
"recurrences" were incoherent chance collisions.

### Highest-value next steps
- **In-game:** breakpoint the hash routine (search EXE for literal `0x08280150` / `0x28C0E011`)
  in the MEGA level → capture one `char*` → confirm/repair the 21 and unlock more.
- **Offline:** target the FINN-16 and other single-level clusters with level-specific base-name
  guesses (the seed pops from any one correct base).

---

## 7. Files in this gist
- `skullmonkeys_findings.md` — this report
- `skullmonkeys_findings.json` — machine-readable: namespaces, 21 decodes, 44 clusters, validation
- `decoder.py` — reusable hash/encode/probe tool for all four namespace keys
