# Name-cracking feasibility: why brute-force / SMT cannot recover sprite names

_2026-06-17. Companion to [`asset-hash-ids.md`](../../reference/asset-hash-ids.md) (the round-by-round
log) and [`family-identifications.md`](family-identifications.md) (human role labels)._

This page settles, with reproducible evidence, **why** the literal ToolX asset names cannot be
recovered by brute force or by a constraint solver (SMT/BDD), even though the audio-FX names *were*
recovered. The blocker is information-theoretic, not a missing hash variant or a tooling gap.

## Background

Asset IDs (BLB sprites/anims/audio) are `id = calcHash(name)`. (Menu text uses the same `calcHash`
with a fixed **section prefix** — the old "WRAP namespace" `0x28C0E011 ^ rotl(calcHash(name),27)` is
just `calcHash(MENU_PREFIX + name)` with prefix end-shift 27; see `asset-hash-ids.md` Round 21.)
`calcHash` is a cumulative bit-toggle:

```c
uint32_t calcHash(const char *s){ uint32_t h=0; int sh=0;
  for(; *s; s++){ int c=*s;
    if(c>='A'&&c<='Z'); else if(c>='a'&&c<='z') c-=32; else if(c>='0'&&c<='9') c+=22; else continue;
    sh=(sh+(c-64))&31; h ^= 1u<<sh; }
  return h; }
```

Each alphanumeric character toggles exactly one bit, at position `sh` (the running char-value sum
mod 32). Non-alphanumerics (incl. `_`) are skipped, so `TAUNT_1` ≡ `TAUNT1`. **Verified exact**:
all 10 sampled "verified" audio names reproduce their IDs byte-for-byte (e.g.
`calcHash("FX_BOSS_WIZARD_TAUNT_1") == 0x0e041001`). There is **one** hash, shared by sprites and
audio — no second transform.

## Two structural facts about `calcHash`

1. **popcount(id) = name_length − 2 × cancellations.** Two characters that toggle the same bit
   position cancel. Therefore:
   - **Low popcount does NOT mean a short name.** The cracked `FX_SKULL_FOOTSTEP_LEFT` is **19
     chars** with popcount **5**; `FX_BOSS_WIZARD_TAUNT_1` is **18 chars**, popcount **6**. A
     sprite ID at popcount 5 almost certainly has a ~15-char name, not a 5-char one.
   - **Consequence:** exhaustive short-string brute (`tools/invert_short`, capped ≤7 chars) can
     never contain the real names; the full long-name space (36¹⁵ ≈ 10²³) is unenumerable.
2. **Length parity is pinned:** `name_length ≡ popcount(id) (mod 2)`, and `length ≥ popcount`.
   (A weak but real constraint — useful to bound solver/MITM length.)

## SMT / solver analysis (z3 4.16)

A correct z3 bit-vector model of `calcHash` was built and **validated** (it inverts known names
exactly). Three results, in order of increasing constraint strength:

| Approach | Result |
|---|---|
| Invert a bare ID (free chars) | SAT, but the lossy hash has astronomically many preimages — pure garbage. |
| **Invert + "contains a known word"** | **Non-discriminating.** For target `0x02098010` ("chain"), z3 returns SAT for substring `CHAIN`, for random `ZQXJV`, *and* for made-up `WUMBO`. Any substring pads to any target → a SAT result is zero evidence. |
| **Invert + "entire string is meaningful vocab"** (the only discriminating option) | Equivalent to full **token-composition** = meet-in-the-middle (`tools/prefix_brute`). Still a collision salad (next section). |

So a solver inverts the hash mechanically, but cannot *discriminate* — the same lossiness that
defeats brute force defeats SMT. "Grep for a known string in low-popcount IDs" fails because
substring containment is satisfiable for every target.

## Full-vocabulary MITM is still a collision salad

`tools/prefix_brute` composes known game-vocabulary tokens via the concat-hash identity
`calcHash(A+B) = (h_A ^ rotl(h_B, sh_A), (sh_A+sh_B)&31)`. Restricting to **entirely meaningful**
names and sweeping all 32 end-shifts at depth ≤4 yields, per single sprite target:

| target | role | distinct fully-meaningful compositions |
|---|---|---|
| `0x12800031` | timer platform | **36** |
| `0x02098010` | chain (bg) | **68** |
| `0x18248010` | KLOG spikes | **53** |

Examples for the timer ID: `EVIL_GET_KEY`, `GROW_WILLIE_BACK_02`, `DASH_WORM_WIZZ_01`,
`KEY_GRUNT_COUNT_BEE` — all real game words, all hashing to `0x12800031`, none the real name.
~50 plausible meaningful preimages per target, with no algorithmic way to choose.

## Why audio cracked but sprites don't

Same hash, same lossiness. Audio names fell only where the **tight `FX_<ACTOR>_<VERB>` grammar +
level-affinity** happened to leave a **single** vocabulary composition — and where the vocabulary
(`FX`, `BOSS`, `WIZARD`, `KLAY`, `DIE`, `EXPLODE`) was guessable English. Sprites have **no
confirmed template anchor** like `FX_`, opaque non-dictionary entity stems, and ~50 candidates per
ID. It was never name *length* or *entropy* — it is **vocabulary + a unique-candidate grammar**.

## The information-theoretic bottom line

A 32-bit lossy hash maps ~10²³ possible names onto 2³² IDs. The preimage set of any ID — **even
filtered to meaningful vocabulary** — is ~50 names. The information distinguishing them was
destroyed at hash time. **No brute force, BDD, or SMT can recover it**; this is a property of the
hash, not of the search method.

## Discriminators that DO beat the lossiness

1. **Rendered on-screen text** — the name is literally drawn in the sprite (gave every confirmed
   sprite text name: `YES`/`NO`/`PAUSED`/`QUIT`/`CONTINUE`/`QUIT GAME`/`LOADING`). One candidate.
2. **Cross-game ground truth** — a `calcHash` hash that also appears in the open-source ScummVM
   Neverhood engine (gave the Klaymen anchors, e.g. `0x5900c41e`).
3. **An external `(name → id)` pair** or the original ToolX asset tree — would pin parameters and
   make a dictionary attack verifiable.

Everything else (blind brute, short brute, solver grep, vocabulary MITM) produces candidate sets
too large to resolve. The practical path is the **role/entity catalog** (see
`family-identifications.md`), which serves the decompilation directly.

## Reproduction

```bash
# popcount/length of cracked audio names (low popcount, long names):
#   see the demo in this session; calcHash exact for all verified audio.
gcc -O3 -o tools/invert_short tools/invert_short.c           # exhaustive short-string inverter
./tools/invert_short 7 36 <target_hex...>                    # ≤7-char preimages (finds only garbage)
# SMT model + discrimination test: z3 bitvector model of calcHash (this session's transcript).
# Full-vocabulary MITM collision counts:
for sh in $(seq 0 31); do ./tools/prefix_brute 0x12800031 $sh -L 2 -n -q; done | sort -u | wc -l
```
