# Session 12 — Going hard on sprites: the naming-structure verdict

Pushed the trigger-context + constrained-solve methods at the full sprite corpus. No new names,
but a definitive, quantified structural conclusion about WHY sprites resist naming.

## Player sprites — decompiler gives the action, but the scheme isn't ENTITY+ACTION
Harvested `PlayerState_*` callback → sprite bindings (action label free from the fn name):
```
0x08208902 CheckpointExit   0x092B8480 Jump        0x0B2084D0 Falling
0x18298210 WalkingLeft      0x1B301085 Death       0x1C3AA013 PickupItem/LandingFromRide
0x292E8480 Running/WalkingRight                    0xC8099196 SpecialIdleAnim
```
**Constrained solve** (find entity+R s.t. sprites = entity+known-action): **NO (entity,R)
decodes >=3 player sprites.** Bare-action RAW (any spelling, any seed): **0 hits.**
- Mean pairwise XOR distance of the 8 player sprites = **10.2 (min 3)** — they are **8
  independent multi-character names**, NOT one base + action variants.
- Earlier corroboration: WalkingLeft/Right don't differ by a LEFT/RIGHT swap; Idle/Idle2 isn't
  base+'2'. => the `PlayerState_*` labels are RE names for the CODE, not the asset strings.

## Boss seed is per-boss-set, not a global enemy key
BOSS seed 0x08280150 tested against **all 260 single-level uncracked sprites** with
level-keyed entity vocab: re-finds exactly the **same 21** (WIZARD/MEGA/KLOGG/YNT animations),
**0 new**. => each non-boss enemy uses its OWN seed/prefix; the boss key doesn't generalize.

## WIZZ boss-part sprites (type 50/51) don't crack and don't cluster
0x181C3854 (BossMain), 0x8818A018 (BossPart), 0x0244655D + 6 other WIZZ entities: no hit under
Monkey-Mage proper-name vocab (names need 8-14 chars); they don't cluster under the BOSS seed
either => independent long names within the namespace.

## The quantified wall
- 84% of uncracked sprites need **>=8-char** names (median floor 10); the short-name
  (statistically crackable) fraction is tiny — and short names are MORE ambiguous, not less.
- Sprite naming is **per-entity**: each creature/object owns a prefix; only the 4 anchored
  bosses + TEXT/PICKUP/HUD namespaces are reachable without a per-entity anchor.

## Method ceiling reached (honest)
Trigger-context cracking works ONLY where: (a) an asset is referenced in code with a
semantically-named function, AND (b) the action word is short, AND (c) it passes the
uniqueness audit. Sprites largely fail (b)+(c): long, independent, per-entity names. The
PAUSE/UNPAUSE wins were the exception (short action + named trigger + verified grammar), not
the rule. No sprite met all three this session.

## What WOULD break it (unchanged)
A per-entity anchor (emulator capture of one string per creature) → that entity's seed →
its whole animation set decodes at once. Offline alone cannot supply it.
