# Session 7 — Per-level + radius-tiered grouping (deeper families)

## Per-level clustering (answer to "what about other levels?")
Clustered within EACH of the 26 levels' uncracked populations, not just by global
level-intersection. Result: **same 167 ids / 52 families as the global pass** — the radius-3
sister structure is COMPLETE and stable (two independent methods agree). No hidden R<=3
families were being missed; the global view just displays the maximal common-level set.

## Radius sweep (the real lever for MORE grouping)
Coverage of uncracked sprites vs Hamming radius, with a level-coherence guard
(merge only if members share >=1 level — kills transitive chaining):

| R | raw ids/fams (biggest) | level-guarded ids/fams (biggest) |
|--|--|--|
| 3 | 146 / 44 (16) | 133 / 42 (16) |
| 4 | 174 / 50 (16) | 150 / 46 (16) |
| **5** | 244 / 49 (**43 = chaining**) | **204 / 53 (16, clean)** |
| 6 | 328 / 18 (284 blob) | 283 / 44 (102) |

**R=3** = frame-counter siblings (1-2 char change). **R=5 level-guarded** = ENTITY-level
grouping (same creature, different *action word* — a multi-char change). Raw R>=5 explodes
via chaining; the level guard keeps it meaningful. R>=6 over-merges even guarded — stop at 5.

## R=5 entity grouping (validated against ground truth)
54 entity-groups / 210 sprites. With ledger names as anchors, groups correctly reassemble
known entities and pull in their unnamed animation siblings:

- **MEGA** = all 6 MEGA animations (TURN/ROLL/DIE/HIT/IDLEx2) — was 2 separate R=3 pairs. ✓
- **YNT** = YNT_DIE/HIT/IDLE **+ newly-tagged 0x48F843D4** (a 5th YNT animation). ✓
- **WIZARD** = WIZARD_FALL pair (other Wizard families are >R5 away — distinct animations).
- **FX_PLAYER_SWIM** = FX_PLAYER_SWIM_PRE **+ 0x462C6040 / 0x46386040 / 0x463E6040**
  (the player swim-frame siblings — strong naming lead: SWIM frames).
- (TEXT glyphs PAUSED/NO/YES/QUIT cluster too — correctly grouped as a *namespace-family*,
  not a creature; labeled as such.)

## Net new this session
- **4 uncracked sprites newly tied to a NAMED entity** (0x48F843D4 -> YNT; 3x -> player swim).
- The full MEGA / YNT boss animation sets are now assembled (named + unnamed).
- Confirmed the R=3 family set is complete (no per-level families were hiding).

## Files
- `master_families.json` — 52 R=3 frame-sibling families (complete)
- `entity_groups.json` — 54 R=5 entity-level groups (updated, ledger-anchored)
