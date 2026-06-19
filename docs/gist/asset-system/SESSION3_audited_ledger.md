# Session 3 — Confidence audit of all recovered names

## Premise (agreed constraints)
- **No external ground truth exists** (no debug build, no symbol/.MAP, no asset-name strings
  in BLB/binary — all stripped). The original ToolX project files are unavailable.
- `calcHash` is **lossy / many-to-one**: every 32-bit id has astronomically many preimages.
  Therefore brute-force can only ever find *a* preimage, never *prove* it is *the* name.
- **Success bar (chosen): "unique plausible preimage."** A name is treated as effectively
  true iff, within the strings that follow the namespace's *naming convention*, **exactly one**
  hashes to the id. 2+ convention-following collisions => CANDIDATE (alternatives listed).

## Method: collision-multiplicity audit
For each namespace we generate the convention-valid name space and bucket by hash:
- **FX_ audio (RAW seed):** `FX_<CAT>_<SUBJ>[_<token>...]` with CAT/SUBJ/MOD vocab harvested
  from the 113 cracked FX_ names. Self-cancelling artifacts removed: any token XORed with
  itself contributes 0 to the hash, so alts like `..._SPIT_SPIT`, `AMBIENT_AMBIENT_...` are
  NOT distinct names (same hash by construction) — filtered out.
- **BOSS sprites (seed 0x08280150, R27):** `ENTITY{KLOGG,WIZARD,MEGA,HEAD,YNT}+ACTION+frame`,
  stress-tested against a **98-word** action vocabulary.
- **TEXT (seed 0x28C0E011, R27):** short UI words + common phrases.

## Results (graded ledger — see `audited_ledger.json`)
| Grade | Count | Meaning |
|---|--:|---|
| **RECOVERED** | 129 | unique plausible preimage in the naming grammar — effectively true |
| **CANDIDATE** | 11 | exactly one competing convention string collides (listed); original strongly preferred |
| UNGRADED | 2 | ScummVM-xref Neverhood labels, not ToolX strings |

### BOSS-21: all RECOVERED, robust
The 21 boss-sprite names stayed unique even when the action vocabulary was **doubled to 98
words** — zero new collisions. Strongest non-ground-truth confidence available.

### The 11 CANDIDATE names (only one weak alternative each)
```
FX_OBJECT_SPLASH    vs FX_OBJECT_PIERCE_WILLIE_01
FX_PICKUP_1970      vs FX_PICKUP_GUM_7_01
FX_PUFF_FALL_3      vs FX_BOSS_BIRD_7_03
FX_BOSS_MEGA_SPIT   vs FX_BOSS_FLY_8_DN
FX_BOSS_KLOGG_IDLE  vs FX_KLAY_WILLIE_WIZARD_6   (alt is absurd: 3 entities)
FX_KLAY_FART        vs FX_OBJECT_SELECT_1_7
FX_BOSS_MEGA_DIE    vs FX_BOSS_DIE_RUN_END
FX_SKULL_FLAP       vs FX_BOSS_KLOGG_6_2
FX_SKULL_EAT        vs FX_SKULL_SWIM_EAT_04
FX_SKULL_FLY_01     vs FX_SKULL_SKULL_01_FAST
FX_BIRD_FLY_01      vs FX_BIRD_SKULL_01_FAST
```
In every case the originally-recorded name is the more plausible reading; the audit only
notes they are not *mathematically* unique.

## The irreducible caveat
Uniqueness is bounded by **vocabulary completeness**. If ToolX used a word outside the
search space (an internal codename instead of e.g. WIZARD), the true string would be absent
and the id would still show as RECOVERED with a *different* unique hit. With zero ground
truth this cannot be detected. The honest claim is therefore:

> "RECOVERED" = the unique plausible-English convention-following preimage. Among names a
> human would plausibly choose, it is the only one — not a proof of the original string.

The only channel that could upgrade RECOVERED -> externally-confirmed is the **Neverhood /
ScummVM shared-engine cross-reference** (shared `calcHash`, some shared assets). Not yet done.

## Files
- `audited_ledger.json` — every named id with namespace, grade, alternatives, type, levels
